/*
 * Written by Buddy.Zhang@aliyun.com
 *
 * The ISL29125 is a low power,high sensitivity,RED,GREEN and BLUE 
 * color light sensor(RGB) with an I2C(SMBus compatible)interface.
 * Its stat-of-the-art photodiode array provides an accurate RGB spectral 
 * response and excellent light source to light source vaiation(LS2LS).
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/irq.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/math64.h>
#include <mach/irqs.h>
#include <linux/miscdevice.h>
#include <linux/suspend.h>
#include <linux/atomic.h>
#include <linux/lockdep.h>


#define DEV_NAME "ISL29125"
#define I2C_ADDR 0x44 
#define ID_REG   0x00
static int i2c_major;


#define DEBUG 0

#define POLL_DELAY 150
#define ABS_LUX       ABS_MISC
#define ABS_GREEN     ABS_MISC + 1
#define ABS_GREENIR   ABS_MISC + 2


/*** Date aera ***/
enum Register_Map {
	R_Device_ID = 0x00,
	R_Device_Reset = 0x00,
	R_CFG1 = 0x01,
	R_CFG2 = 0x02,
	R_CFG3 = 0x03,
	R_L_TH_L = 0x04,
	R_L_TH_H = 0x05,
	R_H_TH_L = 0x06,
	R_H_TH_H = 0x07,
	R_SFR    = 0x08,
	R_DT_G_L = 0x09,
	R_DT_G_H = 0x0A,
	R_DT_R_L = 0x0B,
	R_DT_R_H = 0x0A,
	R_DT_B_L = 0x0D,
	R_DT_B_H = 0x0E
};

enum als_range {
	RangeLo = 0,
	RangeHi,
	RangMax
};
enum resolution {
	Bit16 = 0,
	Bit12,
	BitMax
};
static s32 XYZCCM_RangeW[3][3] ={
	{   4425L,  6807L,  -774L},// X col
	{   1561L,  8811L,  42L },// Y col
	{   -1852L, -1439L, 21845L}, // Z col
};

static s32 XYZCCM_RangeB[3][3] ={
	{   -4143L, 10349L, -3994L},// X col
	{   -1724L, 8230L,  -2475L },// Y col
	{   -21845L, 14760L, 3742L}, // Z col
};

#define ALS_SAMPLE_COUNT      4
#define record_light_values(arry)   \
	do {    \
		int i = 0;  \
		for(i = 0 ; i < ALS_SAMPLE_COUNT ;  i++) { \
			arry[i] = -1;  \
		} \
	} while(0)

/*
 * Debug function
 */
#define show()  \
{                   \
	printk(KERN_INFO "[%s]LINE[%d]\n",__FUNCTION__,__LINE__);   \
}

/* Mode operation */
#define POWN_DOWN_MODE           0
#define GREEN_ONLY_MODE          1
#define RED_ONLY_MODE            2
#define BLUE_ONLY_MODE           3
#define STAND_BY_MODE            4
#define RGB_MODE                 5
#define RG_MODE                  6
#define BG_MODE                  7
/* Date Ranges */
#define RANGE_375                0
#define RANGE_10000              1
/* ADC Resolution */
#define ADC_16bit                0
#define ADC_12bit                1
/* ADC Start mode*/
#define ADC_START_I2C            0
#define ADC_START_INT            1
/* Interrupt mode */
#define INT_NO_INT               0
#define INT_GREEN                1
#define INT_RED                  2
#define INT_BLUE                 3

#define INTR_PERSIST_1           0       
#define INTR_PERSIST_2           1
#define INTR_PERSIST_4           2
#define INTR_PERSIST_8           3
/* default data */
#define DEFAULT_IR_COMP          0
#define DEFAULT_IR_INDICATOR_GREEN    4000
#define DEFAULT_KR               13354
#define DEFAULT_KB               7022
#define DEFAULT_CONVERSION_TIME  100 

#define WHITE_HIGH_LUX_COEF     15874
#define BLACK_HIGH_LUX_COEF     11577
#define WHITE_LOW_LUX_COEF      472
#define BLACK_LOW_LUX_COEF      245

#define BLACK_RANGE_SWITCH_H  (BLACK_HIGH_LUX_COEF * 0xCCC / 2560LL)
#define BLACK_RANGE_SWITCH_L  (BLACK_LOW_LUX_COEF * 0xCCC / 2560LL)
#define WHITE_RANGE_SWITCH_H  (BLACK_HIGH_LUX_COEF * 0xCCC / 256LL)
#define WHITE_RANGE_SWITCH_L  (BLACK_LOW_LUX_COEF * 0xCCC / 2560LL)

#define ISL_SYSFS_PERMISSION    00664

enum work_status {
	WORK_NONE,
	W1_CONVERSION_GREEN_RED_BLUE,
	W1_CONVERSION_GREEN_IRCOMP
};

static struct class *i2c_class;
struct i2c_client *my_client;
static int i2c_major;
typedef unsigned char reg_t;
static atomic_t isl_als_start = ATOMIC_INIT(0);

static struct ISL_data {
	bool sensor_enable;
	int poll_delay;  /* poll delay set by HAL */

	spinlock_t lock;
	struct i2c_client *client_data;
	struct input_dev *sensor_input;
	struct delayed_work sensor_dwork; /* For ALS polling */
	struct workqueue_struct *wq;
	int tptype;
	int len;
	char tp[32];
	int first_flags;
	atomic_t suspend_state;
	u16 conversion_time;
	enum work_status wstatus;
	/* read data */
	unsigned long cache_red;
	unsigned long cache_green;
	unsigned long cache_blue;
	unsigned long raw_green_ircomp;
	unsigned long long lux;
	unsigned long long prev_lux;
	/* cct calculating data */
	u8 ir_comp;
	u32 IR_indicator_green;
	s32 kr;
	s32 kb;
	s16 mx;
	s16 my;
	signed long lux_coef;

	u16 cct;
	signed long long X;
	signed long long Y;
	signed long long Z;
	unsigned long x;
	unsigned long y;
	unsigned long long record_arry[4];
	int record_count;
};

static struct ISL_data *ISL_info = NULL;

/*** Helper Function ***/
#if DEBUG
void debug(struct i2c_client *client,char *s)
{
}
#else
void debug(struct i2c_client *client,char *s)
{
	printk(KERN_INFO "[%s]name:%s\n"
			"[%s]addr:%08x\n"
			"[%s]adapter:%s\n",s,client->name,
			s,client->addr,s,client->adapter->name);
}
#endif
/*
 * I2C_read
 */
static int i2c_read(reg_t addr,char *buf,int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = my_client;
	/* Dump write */
	msg[0].addr  = client->addr;
	msg[0].flags = client->flags | I2C_M_TEN;
	msg[0].buf   = &addr;
	msg[0].len   = 1;
	/* read operation */
	msg[1].addr  = client->addr;
	msg[1].flags = client->flags | I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].buf   = buf;
	msg[1].len   = len;

	ret = i2c_transfer(client->adapter,msg,2);
	if(ret != 2)
	{
		printk(KERN_INFO "ERR[%s]I2C_Read\n",__FUNCTION__);
		return -EFAULT;
	}
	return 0;
}
/*
 * I2C_write
 */
static int i2c_write(reg_t addr,char *buf,int len)
{
	int ret;
	char tmp[len + 1];
	struct i2c_msg msg;
	struct i2c_client *client = my_client;

	tmp[0] = addr;
	for(ret = 1 ; ret <= len ; ret++ )
		tmp[ret] = buf[ret - 1];
	/* write initial */
	msg.addr  = client->addr;
	msg.flags = client->flags | I2C_M_TEN;
	msg.buf   = tmp;
	msg.len   = len + 1;

	ret = 0;
	ret = i2c_transfer(client->adapter,&msg,1);
	if(ret != 1)
	{
		printk(KERN_INFO "ERR[%s]I2C_Write\n",__FUNCTION__);
		return -EFAULT;
	}
	return 0;
}
/*
 * Get bits
 */
reg_t GET_BITS(int high_bit,int low_bit,reg_t addr)
{
	unsigned long mask_high,mask_low;
	reg_t data;

	mask_high = ((1 << (high_bit + 1)) - 1);
	mask_low  = ~((1 << low_bit) - 1);
	mask_low &= mask_high;
	i2c_read(addr,&data,1);
	data &= mask_low;
	data >>= low_bit; 
	return data;
}
/*
 * Set bits
 */
int SET_BITS(int high_bit,int low_bit,reg_t addr,reg_t data)
{
	unsigned long mask;
	reg_t buf;

	mask = ((1UL << (high_bit - low_bit + 1)) - 1);
	data &= mask; // ignore overflow!
	data <<= low_bit;
	mask <<= low_bit;
	mask = ~mask;
	i2c_read(addr,&buf,1);
	buf &= mask;
	buf |= data;
	
	return 0 == i2c_write(addr,&buf,1);
}
#define SET_BIT(n,addr)     SET_BITS(n,n,addr,1)
#define CLEAR_BIT(n,addr)   SET_BITS(n,n,addr,0)
#define GET_BIT(n,addr)     GET_BITS(n,n,addr)
#define get_register(addr)  GET_BITS(7,0,addr)
#define set_register(addr,data) SET_BITS(7,0,addr,data)

/**** Register Operation ****/
/*
 * Reset all registers to their default states.
 */
void ISL_reset(void)
{
	set_register(R_Device_Reset,0x46);
}
/*
 * RGB mode selection.
 * This device has various RGB operation modes.These modes are
 * selectd by setting B2:B0 bits in follow table.The deive powers
 * up on a disable mode.All operation modes are in continuous ADC
 * conversion.The following bits are used to enable the operating
 * mode
 *   B2:B0                    OPERATION
 * -----------------------------------------------
 *    000              PowerDown(ADC conversion)
 *    001              GREEN Only
 *    010              RED Only
 *    011              BLUE Only
 *    100              Stand by(No ADC converion)
 *    101              GREEN/RED/BLUE
 *    110              GREEN/RED
 *    111              GREEN/BLUE
 */
void ISL_Mode_Set(reg_t mode)
{
	SET_BITS(2,0,R_CFG1,mode);
}
reg_t Get_ISL_Mode(void)
{
	return GET_BITS(2,0,R_CFG1) & 0x07;
}
/*
 * RGB Daate Sensing Range.
 * The full scale RGB range has two different selectable ranges at
 * bit 3.The range determines the ADC resolution(12 bits and 16 
 * bits).Each range has a maximum allowable lux value.Higher range
 * values offer better resolution and wider lux value.
 *          SENSING RANGES
 *      B3               RANGES
 *      0               575 lux
 *      1             10000 lux
 */
void ISL_Date_Range(reg_t range)
{
	SET_BITS(3,3,R_CFG1,range);
}
bool is_Range_575(void)
{
	return 0 == GET_BIT(3,R_CFG1);
}
bool is_Range_10000(void)
{
	return 1 == GET_BIT(3,R_CFG1);
}
void Set_Range_575(void)
{
	CLEAR_BIT(3,R_CFG1);
}
void Set_Range_10000(void)
{
	SET_BIT(3,R_CFG1);
}
/*
 * ADC Resolution.
 * ADC's resoulution and the number of clock cycles per conversion is
 * determined by this bit in follow Table.Changing the resolution of 
 * the ADC,changes the number of clock cycles of the ADC which in turn
 * changes the integration time.Interation time is the period the ADC
 * samples the photodiode current signal for a measurement.
 *            ADC RESOLUTIONS
 *     B4                 RESOLUTION
 *     0                   16 bits
 *     1                   12 bits
 */
void ISL_ADC_Res(reg_t res)
{
	SET_BITS(4,4,R_CFG1,res);
}
bool is_ADC_16bit(void)
{
	return !GET_BIT(4,R_CFG1);
}
bool is_ADC_12bit(void)
{
	return !!GET_BIT(4,R_CFG1);
}
/*
 * RGB Start Synced at INT PIn
 * SYNC has two different selectable modes at bit5,B5 sets to 0
 * then the INT pin gets asserted whenever the sensor interrupts.B5
 * sets to 1 then the INT pin becomes input pin.On the rising edge
 * at INT pin,SYNC starts ADC conversion.The INT pin set to Interrupt 
 * mode by default.More information about SYNC at "Principles of 
 * Operation".
 *           SYNCED AT INT
 *      B5          OPERATION
 *      0       ADC start at I2C write 0x01
 *      1        ADC start at rising INT
 */
void ISL_ADC_Start_Mode(reg_t mode)
{
	SET_BITS(5,5,R_CFG1,mode);
}
reg_t is_INT_start(void)
{
	return 1 == GET_BIT(5,R_CFG1);
}
reg_t is_I2C_start(void)
{
	return 0 == GET_BIT(5,R_CFG1);
}
/*
 * Active Infrared compensation
 * The device is designed for opeartion under dark glass cover which 
 * sighificantly attenuates visible light and pass the infrared light
 * without much attenuation.The device has an on chip passive optical 
 * filter designed to block most of the incident infra Red.In addition,
 * the device provides a programmable active IR compensation which 
 * allows fine tuning of residual infrared components from the output
 * which allows optimizing the measurement variation between differing 
 * IR-content light sources.B7 is "IR Comp Offset" and B[5:0] is "IR
 * Comp Adjust" which provides means for adjusting IR compensation.B7=
 * '0' + B[5:0] is the effective IR compensation from 0 to 63 codes and 
 * B7 set to '1' + B[5:0] the effective IR compensation is from 106 to 
 * 169.Follow Table show lightweight for each IR compensation bit and
 * Figure is a typical system measure for both IR Comp Adjust and IR 
 * Comp Offset.More detail about how to IR compensation.see IR compensation
 * in Applications information.
 * Recommended to set BF at register 0x02 to max out IR compensation
 * value.It make High range reach more than 10000 lux.
 * B7: IR Comp Offset.
 * B[5:0]: IR Comp Adjust.
 * Total Comp = B7 * 100 + B[5:0].
 */
void ISL_IR_Comp(reg_t Offset,reg_t Adjust)
{
	SET_BITS(7,7,R_CFG2,Offset);
	SET_BITS(5,0,R_CFG2,Adjust);
}
/*
 * Interrupt Threshold assingment.
 * The interrupt status bit(RGBTHF) bit0 at Reg0x08 is a status bit 
 * for light intensity detection.The bit is set to logic HIGH when the 
 * light intensity crosses the interrupt thresholds window(register
 * address 0x04 - 0x07) and set to logic LOW when its within the 
 * interrupt thresholds window.Once the interrupt is triggered,the 
 * INT pin goes low and the interrupt status bit goes HIGH until the 
 * status bit is polled through the I2C read command.Both the INT
 * pin and the interrupt status bit are automatically cleared at the
 * end of the 8-bit Device Register byt 0x08 transfer.Follow Table show.
 *                INTERRUPT STATUS
 *       B1:0           INTERRUPT STATUS
 *        00              No interrupt
 *        01              GREED interrupt
 *        10              RED interrupt
 *        11              BLUE interrupt
 */
void ISL_INT_STATUS(reg_t mode)
{
	SET_BITS(1,0,R_CFG3,mode);
}
reg_t Get_ISL_INT_STATUS(void)
{
	return GET_BITS(1,0,R_CFG3) & 0x03;
}
/*
 * Interrupt Persist Control
 * To minimize interrup events due to 'transient' conditions,an
 * interrupt persistency option is available.IN the event of transient
 * condition an 'X-consecutive' number of interrupt must happen
 * before the interrupt flag and pint(INT) pin gets driven low.The 
 * register(Addr:0x08) is read to CLEAR the bit(s)
 *           INTERRUPT PERSIST
 *      B3:2         NUMBER OF INTEGRATION CYCLE
 *       00                    1
 *       01                    2
 *       10                    4
 *       11                    8
 */
void ISL_INT_Cycle(reg_t cycle)
{
	SET_BITS(3,2,R_CFG3,cycle);
}
reg_t get_INT_Cycle(void)
{
	return GET_BITS(3,2,R_CFG3) & 0x03;
}
/*
 * RGB CONVERSION DONE TO INT CONTROL
 *    B4      CONVERSION DONE
 *    0         Disable
 *    1         Enable
 */
void ISL_Conv_to_INT_disable(void)
{
	CLEAR_BIT(4,R_CFG3);
}
void ISL_Conv_to_INT_enable(void)
{
	SET_BIT(4,R_CFG3);
}
bool is_Conv_to_INT_enable(void)
{
	return !!GET_BIT(4,R_CFG3);
}
void Set_Conv_to_Intr(int mode)
{
	SET_BITS(4,4,R_CFG3,mode);
}
/*
 * Interrupt Threshold.
 * The interrupt threshold level is a 16-bit number(Low Threshold-1
 * and Low Threshold-2).The lower interrupt threshold register are
 * used to set the lower trigger point for interrupt generation.If 
 * ALS value crosses below or is equal to the lower threshold,an
 * interrupt is asserted on the interrupt pin(LOW) and the interrupt
 * status bit(HIGH).Register Low Threshold-1(0x4 or 0x6) and low
 * Threshold-2(0x5 or 0x7) provide the low and high bytes,respectively
 * for the interrupt threshold.The interrupt threshold registers 
 * default to 0x00 upon power up.The user can also configure the 
 * persistency for the interrup pin.This reduces the possibilty of 
 * false triggers.such as noise or sudden spikes in ambient light
 * conditions or an unexpected camera flash,for example,can be ignored
 * by setting the persistency to 8 integration cycles.
 */

/*
 * Status Flag Register.
 * RGBTHF [B0]
 * This is the status bit of the interrupt.The bit is set to logic high
 * when the interrupt thresholds have been triggered(out ot threshold
 * window) and logic low when not yet triggered.Once activated and
 * the interrupt is triggered,the INT pin goes low and the interrupt
 * status bit goes high until the status bit is polled through the
 * I2C read command.Both the INT output and the interrupt status
 * bit are automatically cleared at the end of the 8-bit command
 * register transfer.
 *       INTERRUPT FLAG
 *     B0         OPERATION
 *     0      Interrupt is cleared or not triggered yet
 *     1      Interrupt is triggered.
 */
bool is_interrupt(void)
{
	reg_t reg;

	reg = GET_BIT(0,R_SFR);

	return !!reg;
}
/*
 * CONVENF[B1]
 * This is the status bit of conversion.The bit is set to logic high
 * when the conversion have been completed and logic low when
 * the conversion is not done or not converion.
 */
bool is_convey_complete(void)
{
	reg_t reg;

	reg = GET_BIT(1,R_SFR);

	return !!reg;
}
/*
 * BOUTF[B2]
 * Bit2 on register address 0x08 is a status bit for brownout
 * condition(BOUT).The default value of this bit is HIGH,BOUT = 1,
 * during the initial power up.This indicates the device may possibly
 * have gone through a brownout condition.Therefore,the status
 * bit should be reset to LOW,BOUT = 0,by and I2C write command
 * during the initial confilguration of the device.The default register
 * value is ox04 at power-on.
 *         BROWNOUT FLAG
 *    B2          OPERATION
 *    0           No Brownout
 *    1           Power down or Brownout occurred
 */
void Set_No_Brownout(void)
{
	CLEAR_BIT(2,R_SFR);
}
/*
 * RGBCF[B5:B4]
 * B[5:4] are flags bits to display either Red or Green or Blue is under
 * conversion process at follow Table.
 *                CONVERSION FLAG
 *    B5:4           RGB UNDER CONVERSION
 *     00              No Operation
 *     01              GREEN
 *     10              RED
 *     11              BLUE
 */
bool is_green(void)
{
	return 1 == GET_BITS(5,4,R_SFR);
}
bool is_red(void)
{
	return 2 == GET_BITS(5,4,R_SFR);
}
bool is_blue(void)
{
	return 3 == GET_BITS(5,4,R_SFR);
}
/*
 * Date Register.
 * The ISL29125 has two 8-bit read-only register to hold the higher
 * and lower byte of the ADC value.The lower byte and higher bytes are
 * accessed at address respectively.For 16-bit resolution,the data is 
 * from D0 to D15;for 12-bit resoulution,the data is from Do to D11.
 * The register are refreshed after every conversion cycle.The default 
 * register value is 0x00 at power-on.Because all the regiser are double
 * buffered the data is always valid on the data register.
 */
unsigned int Get_RGB_Data(int R_H,int R_L)
{
	reg_t high,low;
	unsigned int max = 0;

	high = get_register(R_H);
	low  = get_register(R_L);

	if(is_ADC_12bit())
		high &= 0x0F;
	max &= high;
	max <<= 8;
	max |= low;

	return max;
}
/* Get data of Green */
#define Get_Green_Data()   Get_RGB_Data(R_DT_G_H,R_DT_G_L)
#define Get_Green_LData()  get_register(R_DT_G_L);
#define Get_Green_HData()  get_register(R_DT_G_H);
/* Get data of Red */
#define Get_Red_Data()     Get_RGB_Data(R_DT_R_H,R_DT_R_L)
#define Get_Red_LData()    get_register(R_DT_R_L)
#define Get_Red_HData()    get_register(R_DT_R_H)
/* Get data of Blue */
#define Get_Blue_Data()    Get_RGB_Data(R_DT_B_H,R_DT_B_L)
#define Get_Blue_LData()   get_register(R_DT_B_L)
#define Get_Blue_HData()   get_register(R_DT_B_H)

/**** Driver Operation ****/
/*
 * Read operation for file_operations.
 */
static ssize_t ISL29125_read(struct file *filp,char __user *buf,
		size_t count,loff_t *offset)
{
	printk(KERN_INFO "[%s]\n",__FUNCTION__);
	ISL_Mode_Set(RED_ONLY_MODE);

	printk(KERN_INFO "Red data[%p]\n",(void *)Get_Red_Data());
	printk(KERN_INFO "Green data[%p]\n",(void *)Get_Green_Data());
	printk(KERN_INFO "Blue data[%p]\n",(void *)Get_Blue_Data());
	return 0;
}
/*
 * Write operation for file_operations.
 */
static ssize_t ISL29125_write(struct file *filp,const char __user *buf,
		size_t count,loff_t *offset)
{
	printk(KERN_INFO "[%s]\n",__FUNCTION__);

	return 0;
}
/*
 * Open operation for file_operations
 */
static int ISL29125_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "[%s]\n",__FUNCTION__);
	filp->private_data = ISL_info->client_data;
	if(!filp->private_data)
	{
		printk(KERN_INFO "NULL pointer!!\n");
		return -EINVAL;
	}

	return nonseekable_open(inode,filp);
}
/*
 * Close operation for file_operations
 */
static int ISL29125_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "[%s]\n",__FUNCTION__);
	filp->private_data = NULL;

	return 0;
}
/*
 * IOC
 */
static long ISL29125_unlocked_ioctl(struct file *filp,unsigned int cmd,
		unsigned long arg)
{
	int err = 0;
	switch(cmd)
	{
		case 0:
		case 1:
			break;

		default:
			printk(KERN_INFO "[%s]not support %p\n",__FUNCTION__,(void *)cmd);
			err = -ENOIOCTLCMD;
			break;
	}
	return err;
}
/*
 * File_operations
 */
static const struct file_operations ISL29125_fops = {
	.owner       = THIS_MODULE,
	.read        = ISL29125_read,
	.write       = ISL29125_write,
	.open        = ISL29125_open,
	.release     = ISL29125_release,
	.unlocked_ioctl = ISL29125_unlocked_ioctl,
};
static struct miscdevice ISL_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEV_NAME,
	.fops  = &ISL29125_fops,
};
/*
 * device id table.
 */
static struct i2c_device_id ISL29125_id[] = {
	{DEV_NAME,I2C_ADDR},
	{},
};
/***** Self Operation *****/
static s32 cal_cct(struct ISL_data *data)
{
	u16 cct;
	signed long long X0,Y0,Z0,sum0;
	signed long long x,y,n,xe,ye;
	u8 Range;
	u8 bits;
	unsigned long als_r,als_g,als_b;
	signed long long tmp;

	als_r = data->cache_red;
	als_g = data->cache_green;
	als_b = data->cache_blue;

	bits = 0;
	/* Black */
	if(ISL_info->tptype == 1)
	{
		X0 = (XYZCCM_RangeB[0][0] * als_r + XYZCCM_RangeB[0][1] * als_g +
				XYZCCM_RangeB[0][2] * als_b);
		Y0 = (XYZCCM_RangeB[1][0] * als_r + XYZCCM_RangeB[1][1] * als_g +
				XYZCCM_RangeB[1][2] * als_b);
		Z0 = (XYZCCM_RangeB[2][0] * als_r + XYZCCM_RangeB[2][1] * als_g +
				XYZCCM_RangeB[2][2] * als_b);
	} else
	{
		X0 = (XYZCCM_RangeW[0][0] * als_r + XYZCCM_RangeW[0][1] * als_g +
				XYZCCM_RangeW[0][2] * als_b);
		Y0 = (XYZCCM_RangeW[1][0] * als_r + XYZCCM_RangeW[1][1] * als_g +
				XYZCCM_RangeW[1][2] * als_b);
		Z0 = (XYZCCM_RangeB[2][0] * als_r + XYZCCM_RangeB[2][1] * als_g +
				XYZCCM_RangeB[2][2] * als_b);
	}
	//printk("[%s]X %p Y %p Z %p\n",__FUNCTION__,(void *)X0,(void *)Y0,(void *)Z0);

	sum0 = X0 + Y0 + Z0;
	if(sum0 == 0)
	{
		printk(KERN_INFO "sum0 value is 0\n");
		return -1;
	}
	x = div64_s64(X0 * 10000,sum0);
	y = div64_s64(Y0 * 10000,sum0);
	xe = 3320; // 0.3320
	ye = 1858; // 0.1858
	data->x = x;
	data->y = y;

	n = div64_s64((x - xe) * 10000,(y - ye));
	tmp = div64_s64(-449 * n,10000);
	tmp = div64_s64((tmp + 3525) * n,10000);
	tmp = div64_s64((tmp - 6823) * n,10000);
	cct = tmp + 5520;

	data->X = X0;
	data->Y = Y0;
	data->Z = Z0;

	if(cct < 0)
		cct = 0;
	data->cct = cct;

	return data->cct;
}
/*
 * Calculate the lux.
 */
static unsigned long long cal_lux(struct ISL_data *data)
{
	data->lux = ((data->raw_green_ircomp * data->lux_coef) >> 8) / 10;

	return data->lux;
}
/*
 * Autorange.
 */
void autorange(unsigned long green,unsigned long long lux)
{
	int ret;
	u64 switch_h,switch_l;
	unsigned int adc_resolution,optical_range;
	int debounce_flag = 0;
	struct ISL_data *data = ISL_info;

	if(data->tptype == 1)
	{
		switch_l = BLACK_RANGE_SWITCH_L;
		switch_h = BLACK_RANGE_SWITCH_H;
	} else
	{
		switch_l = WHITE_RANGE_SWITCH_L;
		switch_h = WHITE_RANGE_SWITCH_H;
	}
	if(lux > switch_l && lux < switch_h)
		debounce_flag = 1;
	else
		debounce_flag = 0;
	
	if(is_ADC_12bit()) {
		if(is_Range_575()) {
			/* Switch to 10000 lux */
			if(green > 0xCCC) {
				if(data->tptype == 1)
					data->lux_coef = BLACK_HIGH_LUX_COEF;
				else
					data->lux_coef = WHITE_HIGH_LUX_COEF;
				Set_Range_10000();
			}
		} else {
			/* Switch to 575 lux */
			if(green < 0xCC && !debounce_flag) {
				if(data->tptype == 1)
					data->lux_coef = BLACK_LOW_LUX_COEF;
				else
					data->lux_coef = WHITE_LOW_LUX_COEF;
				Set_Range_575();
			}
		}
	} else {  /* ADC 16bit */
		if(is_Range_10000()) {
			/* switch to 575 lux */
			if(green < 0xCCC && !debounce_flag) {
				if(data->tptype == 1)
					data->lux_coef = BLACK_LOW_LUX_COEF;
				else
					data->lux_coef = WHITE_LOW_LUX_COEF;
				Set_Range_575();
			} else {
				if(data->tptype == 1)
					data->lux_coef = BLACK_HIGH_LUX_COEF;
				else
					data->lux_coef = WHITE_HIGH_LUX_COEF;
			}
		} else { /* Range 575 */
			if(green > 0xCCC) {
				if(data->tptype == 1)
					data->lux_coef = BLACK_HIGH_LUX_COEF;
				else
					data->lux_coef = WHITE_HIGH_LUX_COEF;
				Set_Range_10000();
			}
		}
	}

	
}
/*
 * Initialise the ISL device.
 */
void INIT_ISL(struct i2c_client *client)
{
	unsigned char reg;

	ISL_info->ir_comp = DEFAULT_IR_COMP;
	ISL_info->IR_indicator_green = DEFAULT_IR_INDICATOR_GREEN;
	ISL_info->kr = DEFAULT_KR;
	ISL_info->kb = DEFAULT_KB;
	ISL_info->conversion_time = DEFAULT_CONVERSION_TIME;
	ISL_info->wstatus = WORK_NONE;
	ISL_info->tptype = 1;
	ISL_info->record_count = 0;
	ISL_info->first_flags = 1;
	
	/* Set device mode to RGB. */
	ISL_Mode_Set(RGB_MODE);
	/* RGB Date sensing range 10000 Lux(High Range) */
	ISL_Date_Range(RANGE_10000);
	/* ADC resolution 16-bit */
	ISL_ADC_Res(ADC_16bit);
	/* ADC start at INTb start */
	ISL_ADC_Start_Mode(ADC_START_I2C);
	/* Default IR Active compenstation */
	/* Default IR compensation control */
	ISL_IR_Comp(0,0);
	/* Interrupt threshold assignment for Green. */
	ISL_INT_STATUS(INT_GREEN);
	/* Interrupt persistency as 8 conversion data out of window */
	ISL_INT_Cycle(3);
	/* RGB CONVERSION DONE TO INT CONTROL */
	ISL_Conv_to_INT_disable();
	/* Writing interrupt low threshold as 0xCCC (%s)*/
	set_register(R_H_TH_L,0xCC);
	set_register(R_H_TH_H,0xCC);
	/* Clear the bronout status flag */
	Set_No_Brownout();
}
/***** Input System Operation *****/
/*
 * Input attribute: raw_adc
 * show adc.
 */
ssize_t show_raw_adc(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	u32 lux;
	struct ISL_data *data = dev_get_drvdata(dev);

	sprintf(buf,"R0 %p,G0 %p B0 %p GIR %p\n",(void *)data->cache_red,
			(void *)data->cache_green,(void *)data->cache_blue,
			(void *)data->raw_green_ircomp);
	return strlen(buf);
}
/*
 * Input attribute: reg_dump
 * Dump all register.
 */
static ssize_t show_reg_dump(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	reg_t reg;
	int i;
	
	*buf = 0;
	for(i = 0 ; i < 15 ; i++) {
		reg = get_register(i);
		sprintf(buf,"%s reg 0x%p(%p)\n",buf,(void *)i,(void *)reg);
	}

	return strlen(buf);
}
/*
 * Input attribute: reg_dump.
 * Store a register.
 */
static ssize_t store_reg_dump(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	unsigned int reg,dat;
	int ret;

	sscanf(buf,"%02x %02x",&reg,&dat);
	set_register(reg,dat);

	return count;
}
/*
 * Input attribute: mode
 * Get the work mode of ISL29125
 */
static ssize_t show_mode(struct device *dev,struct device_attribute *attr,char *buf)
{
	switch(Get_ISL_Mode()) {
		case 0:
			sprintf(buf,"%s\n","PWDN");
			break;
		case 1:
			sprintf(buf,"%s\n","GREEN");
			break;
		case 2:
			sprintf(buf,"%s\n","RED");
			break;
		case 3:
			sprintf(buf,"%s\n","BLUE");
			break;
		case 4:
			sprintf(buf,"%s\n","STANDBY");
			break;
		case 5:
			sprintf(buf,"%s\n","RGB MODE");
			break;
		case 6:
			sprintf(buf,"%s\n","GREEN.RED");
			break;
		case 7:
			sprintf(buf,"%s\n","GREEN.BLUE");
			break;
	}
	return strlen(buf);
}
/*
 * Input attribute: mode
 * Stroe the mode of ISL29125
 */
static ssize_t store_mode(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
	int val;
	int mode;

	val = simple_strtoul(buf,NULL,10);
	if(val == 0)
		mode = POWN_DOWN_MODE;
	else if(val == 1)
		mode == GREEN_ONLY_MODE;
	else if(val == 2)
		mode == RED_ONLY_MODE;
	else if(val == 3)
		mode == BLUE_ONLY_MODE;
	else if(val == 4)
		mode == STAND_BY_MODE;
	else if(val == 5)
		mode == RGB_MODE;
	else if(val == 6)
		mode == RG_MODE;
	else if(val == 7)
		mode == BG_MODE;
	else {
		printk(KERN_ERR "%s:Invalid value\n",__FUNCTION__);
		return -1;
	}

	ISL_Mode_Set(mode);

	
	return strlen(buf);
}
/*
 * Input attribute: cct
 * show cct.
 */
static ssize_t show_cct(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	s32 cct;
	struct ISL_data *data = dev_get_drvdata(dev);

	cct = cal_cct(data);
	
	if(cct < 0)
		return sprintf(buf,"cct_err: high IR light source.\n");
	else if(cct == 0)
		return sprintf(buf,"cct_err: out of range\n");
	else
		return sprintf(buf,"%d\n",cct);
}
/*
 * Input attribute: lux
 * show lux
 */
ssize_t show_lux(struct device *dev,struct device_attribute *attr,char *buf)
{
	long long lux = -1;
	struct ISL_data *data = dev_get_drvdata(dev);

	spin_lock(&data->lock);

	if(!atomic_read(&isl_als_start))
		lux = cal_lux(data);
	sprintf(buf,"%lld\n",lux);
	
	spin_unlock(&data->lock);
	return strlen(buf);
}
/*
 * Input attribute: xy_value.
 * show x y value.
 */
static ssize_t show_xy_value(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	struct ISL_data *data = dev_get_drvdata(dev);

	return sprintf(buf,"x %p y %p\n",(void *)data->x,
			(void *)data->y);
}
/*
 * Input attribute: optical_range
 * show optical range.
 */
static ssize_t show_optical_range(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	sprintf(buf,"%d",is_Range_10000());
	
	return strlen(buf);
}
/*
 * Input attribute: adc_resolution
 * show_adc_resolution.
 */
static ssize_t show_adc_resolution_bits(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	sprintf(buf,"%d",is_ADC_12bit());

	return strlen(buf);
}
/*
 * Input attribute:adc_resolution
 * store the bit of ADC resolution.
 */
static ssize_t store_adc_resolution_bits(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	int val;
	reg_t reg;

	val = simple_strtoul(buf,NULL,10);
	if(val == 0)
		reg = 0;
	else if(val == 1)
		reg = 1;
	else {
		printk(KERN_ERR "%s:Invalid input\n",__FUNCTION__);
	}
	ISL_ADC_Res(reg);

	return strlen(buf);
}
/*
 * Input attribute: intr_threshold
 * show the interrupt threshold of ISL29125.
 */
static ssize_t show_intr_threshold_high(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	sprintf(buf,"%d",get_register(R_H_TH_L));

	return strlen(buf);
}
/*
 * Input attribute: intr_threshold
 * store the interrupt threshold of ISL29125.
 */
static ssize_t store_intr_threshold_high(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	unsigned short reg;

	reg = simple_strtoul(buf,NULL,10);
	if(reg == 0 || reg > 65535) {
		printk(KERN_ERR "%s:Invalid input value\n",__FUNCTION__);
		return -1;
	}
	set_register(R_H_TH_L,reg);

	return strlen(buf);
}
/*
 * Input attribute: intr_threshold
 * show the interrupt threshold of ISL29125.
 */
static ssize_t show_intr_threshold_low(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	sprintf(buf,"%d",get_register(R_L_TH_L));

	return strlen(buf);
}
/*
 * Input attribute: intr_threshold
 * store the interrupt threshold of ISL29125.
 */
static ssize_t store_intr_threshold_low(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	unsigned short reg;

	reg = simple_strtoul(buf,NULL,10);
	if(reg == 0 || reg > 65535) {
		printk(KERN_ERR "%s:Invalid input value\n",__FUNCTION__);
		return -1;
	}
	set_register(R_L_TH_L,reg);

	return strlen(buf);
}
/*
 * Input attribute: intr_threshold_assign.
 * show the interrupt threahold for assign.
 */
static ssize_t show_intr_threshold_assign(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	switch(Get_ISL_INT_STATUS()) {
		case 0:
			sprintf(buf,"%s","none");
			break;
		case 1:
			sprintf(buf,"%s","green");
			break;
		case 2:
			sprintf(buf,"%s","red");
			break;
		case 3:
			sprintf(buf,"%s","blue");
	}

	return strlen(buf);
}
/*
 * Input attribute: intr_threshold_assign.
 * store the interrupt threshold for assign.
 */
static ssize_t store_intr_threshold_assign(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	short int threshold_assign;
	
	if(!strcmp(buf,"none")) 
		threshold_assign = INT_NO_INT;
	else if(!strcmp(buf,"green")) 
		threshold_assign = INT_GREEN;
	else if(!strcmp(buf,"red"))
		threshold_assign = INT_RED;
	else if(!strcmp(buf,"blue"))
		threshold_assign = INT_BLUE;
	else {
		printk(KERN_ERR "%s:Invalid value\n",__FUNCTION__);
		return -1;
	}
	ISL_INT_STATUS(threshold_assign);

	return strlen(buf);
}
/*
 * Input attribute: intr_persistency
 * Show the perisitency of interrupt.
 */
static ssize_t show_intr_persistency(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	short int intr_persist;

	switch(get_INT_Cycle()) {
		case 0:
			intr_persist = 1;
			break;
		case 1:
			intr_persist = 2;
			break;
		case 2:
			intr_persist = 4;
			break;
		case 3:
			intr_persist = 8;
			break;
	}
	sprintf(buf,"%d",intr_persist);

	return strlen(buf);
}
/*
 * Input attribute: intr_persistency.
 * Store the persistency.
 */
static ssize_t store_intr_persistency(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
	short int intr_persist;

	
	intr_persist = simple_strtoul(buf,NULL,10);
	if(intr_persist == 8)
		intr_persist = INTR_PERSIST_8;
	else if(intr_persist == 4)
		intr_persist = INTR_PERSIST_4;
	else if(intr_persist == 2)
		intr_persist = INTR_PERSIST_2;
	else if(intr_persist == 1)
		intr_persist = INTR_PERSIST_1;
	else {
		printk(KERN_ERR "%s: Invalid value\n",__FUNCTION__);
		return -1;
	}
	ISL_INT_Cycle(intr_persist);

	return strlen(buf);
}
/*
 * Input attribute: rgb_conv_intr
 * show rgb conv intr
 */
static ssize_t show_rgb_conv_intr(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	sprintf(buf,"%s", !is_Conv_to_INT_enable()? "disable" : "enable");

	return strlen(buf);
}
/*
 * Input attribute: rgb_conv_intr
 * Store the state of interrupt endable.
 */
static ssize_t store_rgb_conv_intr(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	int ret;
	int rgb_conv_intr;

	ret = simple_strtoul(buf,NULL,10);
	if(ret == 1)
		rgb_conv_intr = 0;
	else if(ret == 0)
		rgb_conv_intr = 1;
	else {
		printk(KERN_ERR "%s:Invalid input for rgb conversion interrup\n",
				__FUNCTION__);
		return -1;
	}
	Set_Conv_to_Intr(rgb_conv_intr);
	
	return strlen(buf);
}
/*
 * Input attribute: adc_start_sync.
 * show adc start sync.
 */
static ssize_t show_adc_start_sync(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	sprintf(buf,"%s", is_INT_start() ? "risingIntb" : "i2cwrite");
}
/*
 * Input attribute: adc_start_sync.
 * store adc start sync.
 */
static ssize_t store_adc_start_sync(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	int ret;
	int adc_start_sync;

	ret = simple_strtoul(buf,NULL,10);
	if(ret == 0)
		adc_start_sync = 0;
	else if(ret == 1)
		adc_start_sync = 1;
	else {
		printk(KERN_ERR "%s: Invalid value for adc start sync\n",__FUNCTION__);
		return -1;
	}
	ISL_ADC_Start_Mode(adc_start_sync);

	return strlen(buf);
}
/*
 * Input attribute: enable_sensor
 */
static ssize_t ISL_show_enable_sensor(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	return sprintf(buf,"%d\n",ISL_info->sensor_enable);
}
/*
 * Input attribute: enable_sensor
 */
static ssize_t ISL_store_enable_sensor(struct device *dev,
		struct device_attribute *attr,const char *buf,
		size_t count)
{
	unsigned long val = simple_strtoul(buf,NULL,10);

	if((val != 0) && (val != 1))
		return count;
	ISL_info->sensor_enable = val;

	if(val == 1) {
		spin_lock(&ISL_info->lock);
		atomic_set(&isl_als_start,1);
		ISL_info->first_flags = 1;
		ISL_info->prev_lux = 0;
		ISL_info->record_count = 0;
		record_light_values(ISL_info->record_arry);
		ISL_info->wstatus = W1_CONVERSION_GREEN_RED_BLUE;
		if(!atomic_read(&ISL_info->suspend_state)) {
			queue_delayed_work(ISL_info->wq,
					&ISL_info->sensor_dwork,
					msecs_to_jiffies(POLL_DELAY));
		}
		spin_unlock(&ISL_info->lock);
	} else {
		spin_lock(&ISL_info->lock);
		cancel_delayed_work_sync(&ISL_info->record_arry);
		ISL_info->record_count = 0;
		record_light_values(ISL_info->record_arry);
		spin_unlock(&ISL_info->lock);
	}

		return count;
}
/*
 * Input attribute: Delay
 */
static ssize_t ISL_show_delay(struct device *dev,
		struct device_attribute *attr,char *buf)
{
	return sprintf(buf,"%d\n",POLL_DELAY);
}
/*
 * Input attribute: Tptype
 */
static ssize_t show_tptype(struct device *dev,struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf,"%d\n",ISL_info->tptype);
}
static ssize_t store_tptype(struct device *dev,struct device_attribute *attr,
		const char *buf,size_t count)
{
	int len,i;
	bool c_exist = false;

	memset(ISL_info->tp,'\0',sizeof(ISL_info->tp));
	strncpy(ISL_info->tp,buf,sizeof(ISL_info->tp));
	len = strlen(ISL_info->tp);
	ISL_info->len = len;

	for(i = 0 ; i < len ; i++) {
		if(ISL_info->tp[i] == ':') {
			if(ISL_info->tp[i - 1] == 'B')
				ISL_info->tptype = 1;
			else if(ISL_info->tp[i - 1] == 'W')
				ISL_info->tptype = 0;
			c_exist = true;
			break;
		}
	}

	if(!c_exist) {
		if(ISL_info->tp[len - 2] == 'B')
			ISL_info->tptype = 1;
		else if(ISL_info->tp[len - 2] == 'W')
			ISL_info->tptype = 0;
	}

	return count;
}
/*
 * Input attribute: batch
 */
static ssize_t ISL_store_batch(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	return count;
}
/*
 * Input attribute: Als_flush
 */
static ssize_t ISL_store_flush(struct device *dev,
		struct device_attribute *attr,const char *buf,size_t count)
{
	struct input_dev *sensor_input = ISL_info->sensor_input;
	unsigned long val = simple_strtoul(buf,NULL,10);

	printk("%s:***********************val %d\n",__func__,val);
	input_report_abs(sensor_input, ABS_LUX , -1);
	input_sync(sensor_input);

	return count;
}
/*
 * Debugging and testing
 */
static DEVICE_ATTR(raw_adc,ISL_SYSFS_PERMISSION, show_raw_adc,NULL);
static DEVICE_ATTR(reg_dump,ISL_SYSFS_PERMISSION, show_reg_dump, store_reg_dump);
/*
 * Attribute of ISL29125 RGB sensor.
 */
/* Main sysfs */
static DEVICE_ATTR(cct,ISL_SYSFS_PERMISSION, show_cct , NULL);
static DEVICE_ATTR(lux,ISL_SYSFS_PERMISSION, show_lux , NULL);
static DEVICE_ATTR(xy_value,ISL_SYSFS_PERMISSION, show_xy_value , NULL);
/* optional */
static DEVICE_ATTR(mode,ISL_SYSFS_PERMISSION,show_mode,store_mode);
static DEVICE_ATTR(optical_range,ISL_SYSFS_PERMISSION, show_optical_range , NULL);
static DEVICE_ATTR(adc_resolution_bits,ISL_SYSFS_PERMISSION,
		show_adc_resolution_bits , store_adc_resolution_bits);
static DEVICE_ATTR(intr_threshold_high,ISL_SYSFS_PERMISSION, 
		show_intr_threshold_high, store_intr_threshold_high);
static DEVICE_ATTR(intr_threshold_low,ISL_SYSFS_PERMISSION, 
		show_intr_threshold_low, store_intr_threshold_low);
static DEVICE_ATTR(intr_threshold_assign,ISL_SYSFS_PERMISSION, 
		show_intr_threshold_assign, store_intr_threshold_assign);
static DEVICE_ATTR(intr_persistency,ISL_SYSFS_PERMISSION, 
		show_intr_persistency, store_intr_persistency);
static DEVICE_ATTR(rgb_conv_intr,ISL_SYSFS_PERMISSION, 
		show_rgb_conv_intr, store_rgb_conv_intr);
static DEVICE_ATTR(adc_start_sync,ISL_SYSFS_PERMISSION, 
		show_adc_start_sync, store_adc_start_sync);
/* Mandatory for Android */
static DEVICE_ATTR(sensor_enable,ISL_SYSFS_PERMISSION, ISL_show_enable_sensor, 
		ISL_store_enable_sensor);
static DEVICE_ATTR(poll_delay,ISL_SYSFS_PERMISSION, ISL_show_delay, NULL);
static DEVICE_ATTR(tptype,ISL_SYSFS_PERMISSION, show_tptype, store_tptype);
static DEVICE_ATTR(batch,ISL_SYSFS_PERMISSION, NULL , ISL_store_batch);
static DEVICE_ATTR(als_flush,ISL_SYSFS_PERMISSION, NULL , ISL_store_flush);


static struct attribute *ISL_attributes[] = {
	/* read RGB value attributes */
	&dev_attr_cct.attr,
	&dev_attr_lux.attr,
	&dev_attr_xy_value.attr,
	/* Device operation mode */
	&dev_attr_optical_range.attr,
	/* Current adc resolution */
	&dev_attr_adc_resolution_bits.attr,
	/* Interrupt related attributes */
	&dev_attr_intr_threshold_high.attr,
	&dev_attr_intr_threshold_low.attr,
	&dev_attr_intr_threshold_assign.attr,
	&dev_attr_intr_persistency.attr,
	&dev_attr_rgb_conv_intr.attr,
	&dev_attr_adc_start_sync.attr,

	&dev_attr_sensor_enable.attr,
	&dev_attr_poll_delay.attr,
	&dev_attr_reg_dump.attr,
	&dev_attr_raw_adc.attr,
	&dev_attr_tptype.attr,
	&dev_attr_batch.attr,
	&dev_attr_als_flush.attr,
		NULL
};
static struct attribute_group ISL_attr_group = {
	.attrs = ISL_attributes
};
static void ISL_input_create(struct ISL_data *data)
{
	int ret;

	data->sensor_input = input_allocate_device();
	if(!data->sensor_input)
		printk("[%s]Failed to allocate input device als\n",__FUNCTION__);
	set_bit(EV_ABS,data->sensor_input->evbit);
	input_set_capability(data->sensor_input,EV_ABS,ABS_LUX);
	input_set_capability(data->sensor_input,EV_ABS,ABS_GREEN);
	input_set_capability(data->sensor_input,EV_ABS,ABS_GREENIR);
	input_set_abs_params(data->sensor_input,ABS_LUX,0,0xFFFF,0,0);
	input_set_abs_params(data->sensor_input,ABS_GREEN,0,0xFFFF,0,0);
	input_set_abs_params(data->sensor_input,ABS_GREENIR,0,0xFFF,0,0);

	data->sensor_input->name = "ISL29125";
	data->sensor_input->dev.parent = &data->client_data->dev;

	ret = input_register_device(data->sensor_input);
	if(ret)
		printk("[%s]:Unable to register input device als:%s\n",
				__FUNCTION__,data->sensor_input->name);
	input_set_drvdata(data->sensor_input,data);
}
/*
 * Queue handler
 */
static void ISL_work_handler(struct work_struct *work)
{
	struct ISL_data *data = 
		container_of(work,struct ISL_data,sensor_dwork.work);
	struct input_dev *sensor_input = data->sensor_input;
	int ret;
	u8 dbg;
	int cct,res,range;
	unsigned long long lux;
	int report_lux = -1,suspend_state;
	static int fast_flag = 0;

	/* Read date of low bits of Green */
	data->cache_green = Get_Green_LData();
	/* Read data of low bits of Read */
	data->cache_red  = Get_Red_LData();
	/* Read data of low bits of Blue */
	data->cache_blue = Get_Blue_LData();
	/* Get the Green data */
	data->raw_green_ircomp = data->cache_green;
	
	lux = cal_lux(data);
	/* LOW and HIGH range switch */
	autorange(data->raw_green_ircomp,lux);

	data->cct = cal_cct(data);

	if((data->record_arry[0] == -1) || (data->record_arry[1] == -1) ||
			(data->record_arry[2]) == -1 || (data->record_arry[3] == -1)) {
		data->record_arry[data->record_count] = lux;
		data->record_count++;
	} else {
		if(data->prev_lux != lux) {
			data->record_count = ALS_SAMPLE_COUNT - 3;

			do {
				data->record_arry[data->record_count - 1] = 
					data->record_arry[data->record_count];
				data->record_count++;
			} while(data->record_count < ALS_SAMPLE_COUNT);

			data->record_arry[data->record_count - 1] = lux;
		}

		if((data->record_arry[0] == data->record_arry[2]) 
				&& (data->record_arry[1] == data->record_arry[3]))
			lux = max(data->record_arry[2],data->record_arry[3]);
	}

	if(atomic_read(&isl_als_start)) {
		lux += 1;
		atomic_set(&isl_als_start,0);
		fast_flag = 10;
	}

	data->lux = lux;
	data->prev_lux = lux;

	suspend_state = atomic_read(&data->suspend_state);
//	printk(KERN_INFO "[ISL] lux %p,cache_green %p,suspend_state %p\n",
//			(void *)data->lux,(void *)data->cache_green,(void *)suspend_state);
	if(!suspend_state) {
		/* Not in suspend */
		input_report_abs(sensor_input,ABS_LUX,data->lux);
		input_report_abs(sensor_input,ABS_GREEN,data->cache_green);
		input_report_abs(sensor_input,ABS_GREENIR,data->cct);
		input_sync(sensor_input);
	}

	if(data->sensor_enable && !atomic_read(&data->suspend_state)) {
		if(fast_flag > 0) {
			fast_flag--;
			data->conversion_time = 50;
		} else
			data->conversion_time = DEFAULT_CONVERSION_TIME * 4;
		queue_delayed_work(data->wq,&data->sensor_dwork,
				msecs_to_jiffies(data->conversion_time));
	}
}
#if 0
/*
 * Suspend early.
 */
static int ISL_early_suspend(struct early_suspend *h)
{
	atomic_set(&data->suspend_state,1);
	if(data->sensor_enable) {
		cancel_delay_work_sync(&ISL_info->sensor_dwork);
		ISL_info->record_count = 0;
		record_light_values(ISL_info->record_arry);
	}
	/* Set Standby mode */
	ISL_Mode_Set(STAND_BY_MODE);

	return 0;
}
/*
 * Resume for ISL.  
 */
static int ISL_late_resume(struct early_suspend *h)
{
	/* Set RGB mode */
	ISL_Mode_Set(RGB_MODE);

	atomic_set(&isl_als_start,1);
	atomic_set(&ISL_info,suspend_state,0);
	if(ISL_info->sensor_enable)
		queue_delayed_work(ISL_info->wq,
				&ISL_info->sensor_dwork,
				msecs_to_jiffies(POLL_DELAY));

	return 0;
}
#endif
/*
 * I2C probe
 */
static int ISL29125_probe(struct i2c_client *client,const struct i2c_device_id *id)
{

	int res;
	struct device *dev;
	reg_t addr = ID_REG;
	char buffer = 0xff;
	struct ISL_data *data;

	data = (struct ISL_data *)kmalloc(sizeof(struct ISL_data *),GFP_KERNEL);
	if(!data)
	{
		printk(KERN_INFO "[%s %d]Failed to alloc memory for module data\n",
				__FUNCTION__,__LINE__);
		return -ENOMEM;
	}
	memset(data,0,sizeof(struct ISL_data));

	printk(KERN_INFO "[%s]Probe\n",__FUNCTION__);
	my_client = client;
	i2c_set_clientdata(client,data);
	data->client_data = client;
	ISL_info = data;

	/* Initialize a mutex for synchronization in sysfs file access */
	data->lock = SPIN_LOCK_UNLOCKED;
	/*
	 * Read ID_Register virtually.
	 */
	i2c_read(addr,&buffer,1);
	if(buffer != 0x7D)
	{
		printk(KERN_ERR "[%s]Invalid device id for ISL\n",__FUNCTION__);
		res = -ENODEV;
		goto out_free;
	}
	printk(KERN_INFO "[%s]Device ID %08x\n",__FUNCTION__,buffer);
	/*
	 * Register the char device.
	 */
	if(misc_register(&ISL_device)) {
		printk(KERN_INFO "register failed\n");
		res = -ENODEV;
		goto out_free;
	}
	/*
	 * Input system initialize.
	 */
	ISL_input_create(data);
	/* Initialize the delay queue */
	INIT_DELAYED_WORK(&data->sensor_dwork,ISL_work_handler);
	data->wq = create_singlethread_workqueue("ISL");
	if(data->wq == NULL) {
		printk(KERN_INFO "[ISL] create workqueu failed\n");
		res = -EBUSY;
		goto out_input;
	}
	/* Initialize the default configureation for ISL sensor device */
	INIT_ISL(client);

	/* Register sysfs hooks */
	res = sysfs_create_group(&client->dev.kobj,&ISL_attr_group);
	if(res) {
		printk(KERN_ERR "[%s]Failed to create sysfs\n",__FUNCTION__);
		res = -EBUSY;
		goto out_waitqueue;
	}


	printk(KERN_INFO "[%s]Probe finish\n",__FUNCTION__);
	return 0;

out_waitqueue:
	destroy_workqueue(data->wq);
out_input:
	input_unregister_device(data->sensor_input);
	input_free_device(data->sensor_input);
	misc_deregister(&ISL_device);
out_free:
	kfree(data);

	return res;
}
/*
 * I2C remove
 */
static int ISL29125_remove(struct i2c_client *client)
{
	struct ISL_data *data = i2c_get_clientdata(client);

	printk(KERN_INFO "[%s]I2C Remove\n",__FUNCTION__);

	sysfs_remove_group(&client->dev.kobj, &ISL_attr_group);
	if(data->wq)
		destroy_workqueue(data->wq);

	input_unregister_device(data->sensor_input);
	input_free_device(data->sensor_input);
	misc_deregister(&ISL_device);
	kfree(data);	
	return 0;
}
/*
 * i2c_driver.
 */
static struct i2c_driver ISL29125_driver = {
	.driver = {
		.name  = DEV_NAME,
		.owner = THIS_MODULE,
	},
	.probe    = ISL29125_probe,
	.id_table = ISL29125_id,
	.remove   = ISL29125_remove,
};
static struct i2c_board_info i2c_devs_info[] = {
	{	
		I2C_BOARD_INFO(DEV_NAME,I2C_ADDR),
	}
};
/*
 * Module init.
 */
static __init int ISL29125_init(void)
{
	int ret;
	/*
	 * Add i2c-board information.
	 */
	struct i2c_adapter *adap;
	struct i2c_board_info i2c_info;

	/*
	 * I2C Board information.
	 */
	adap = i2c_get_adapter(0);
	if(adap == NULL) {
		printk(KERN_INFO "ERR[%s]Unable to get I2C adapter\n",
				__FUNCTION__);
		ret = -EBUSY;
		goto out;
	}
	memset(&i2c_info,0,sizeof(struct i2c_board_info));
	strlcpy(i2c_info.type,DEV_NAME,I2C_NAME_SIZE);
	i2c_info.addr = I2C_ADDR;

	/*
	 * Add the device
	 */
	my_client = i2c_new_device(adap,&i2c_info);
	i2c_put_adapter(adap);
	if(my_client == NULL) {
		printk(KERN_INFO "ERR[%s]Unable to get new i2c device\n",
				__FUNCTION__);
		ret = -ENODEV;
		goto out;
	}
	printk(KERN_INFO "[%s]Init Module\n",__FUNCTION__);
	/* Register i2c driver into i2c core */
	return i2c_add_driver(&ISL29125_driver);

out:
	return ret;
}
/*
 * Module exit.
 */
static __exit void ISL29125_exit(void)
{
	printk(KERN_INFO "[%s]Exit Module\n",__FUNCTION__);
	i2c_del_driver(&ISL29125_driver);
	i2c_unregister_device(my_client);
}

MODULE_AUTHOR("Buddy <Buddy.D.Zhang@outlook.com>");
MODULE_DESCRIPTION("ISL29125 Gsensor\n");
MODULE_LICENSE("GPL");

module_init(ISL29125_init);
module_exit(ISL29125_exit);
