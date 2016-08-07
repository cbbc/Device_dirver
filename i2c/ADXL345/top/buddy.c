#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "map.h"

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <linux/delay.h>

#include <linux/input.h>

#define DEV_NAME "ADXL345"
#define I2C_ADDR 0x53 

typedef unsigned char reg_t;
static struct class *i2c_class;
struct i2c_client *my_client;
static int i2c_major;
static struct input_dev *ADXL345_dev;

#define DEBUG 0

#if DEBUG
void debug(char *s)
{
}
#define mm_debug
#else
#define mm_debug printk
void debug(char *s)
{
	struct i2c_client *client = my_client;

	mm_debug(KERN_INFO "[%s]name:%s\n"
			"[%s]addr:%08x\n"
			"[%s]adapter:%s\n",s,client->name,
			s,client->addr,s,client->adapter->name);
}
#endif
#define mm_err printk

/*
 * I2C_read
 */
static int i2c_read(reg_t addr,reg_t *buf,int len)
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
		mm_err(KERN_INFO "ERR[%s]I2C_Read\n",__FUNCTION__);
		return -EFAULT;
	}
	return 0;
}
/*
 * I2C_write
 */
static int i2c_write(reg_t addr,reg_t *buf,int len)
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
		mm_err(KERN_INFO "ERR[%s]I2C_Write\n",__FUNCTION__);
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
	mm_debug(KERN_INFO "[%s][%d - %d]%08x\n",Register_name[addr - 0x1D],
			low_bit,high_bit,data);
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
/*
 * The operation of Chip register.
 */
reg_t chip_id(void)
{
	reg_t id;

	i2c_read(DEVID,&id,1);
	mm_debug(KERN_INFO "[%s]Chip ID %02x\n",__FUNCTION__,id);

	return id;
}
/*
 * Register: THRESH_TAP
 * The THRESH_TAP register is eight bit and holds the threshold
 * value for tap interrupts.The data format is unsigned,therefore,
 * the magnitude of the tap event is compared with the value
 * in THRESH_TAP for normal tap detection.The scale factor is 
 * 625 mg/LSB(that is,0xFF = 16g).A value of 0 may reseult in 
 * undesirable behavior if single tap/double tap interrupts are
 * enable.
 */
#define get_THRESH_TAP() get_register(THRESH_TAP) 
/*
 * Set some bit not all.
 */
#define set_tap_threshold(data) set_register(THRESH_TAP,data)
/*
 * Register: OFSX,OFSY,OFSZ.
 * The OFSX,OFSY,and OFSZ registers are each eight bits and 
 * offer user-set offset adjuments in twos complement format
 * with a scale factor of 15.6mg/LSB(that is,0x7F = 2g).The
 * value stored in the offset register value is automatically
 * added to the acceleration data,and the resulting value is 
 * stored in the output data registers.For additional information
 * regarding offset calibration and the use of the offset register,
 * refer to the Offset Calibration section.
 */
#define set_x_offset(offset) set_register(OFSX,offset)
#define set_y_offset(offset) set_register(OFSY,offset)
#define set_z_offset(offset) set_register(OFSZ,offset)
#define get_x_offset() get_register(OFSX)
#define get_y_offset() get_register(OFSY)
#define get_z_offset() get_register(OFSZ)
/*
 * Register: DUR
 * The DUR register is eigth bits and contains an unsigned time
 * value representing the maximum time that an event must be
 * above the THRESH_TAP threshold to qualify as a tap event.The
 * scale factor is 625us/LSB.A value of 0 disable the single tap/
 * double tap function.
 */
#define get_DUR()      get_register(DUR)
#define get_max_time() get_register(DUR)
#define set_max_time(max) set_register(DUR,max)
/*
 * Register: Latent
 * The latent register is eight bits and contain an unsigned time
 * value representing the wait time from the detection of a tap
 * event to the start of the time window(define by the window
 * register) during which a possible second tap event can be detected.
 * The scale factor is 1.25ms/LSB.A value of 0 disables the double tap
 * function.
 */
#define set_Latent(data) set_register(Latent,data)
#define get_Latent() get_register(Latent)
/*
 * Register: Window
 * The window register is eight bits and contains an unsigned time
 * value representing the amount of time after the expiration of the 
 * latency time(determined by the latent register) during which a 
 * second valid tap can begin.The scale factor is 1.25ms/LSB.A
 * value of 0 disable the double tap function.
 */
#define set_window(data) set_register(Window,data)
#define get_window() get_register(Window)
/*
 * Register: THRESH_ACT
 * The THRESH_ACT register is eight bits and holds the threshold
 * value for detecting activity.The data formate is unsigned,so the
 * magnitude of the activity event is compared with the value in
 * the THRESH_ACT register.The scale factor is 62.5mg/LSB.
 * A value of 0 may result in undesirable behaviro if the activity
 * interrupt is enabled.
 */
#define set_THRESH_ACT(data) set_register(THRESH_ACT,data)
#define get_THRESH_ACT() get_register(THRESH_ACT)
/*
 * Register: THRESH_INACT
 * The THRESH_INACT register is eight bits and hold the threshold
 * value for detecting inactivity.The data format is unsigned,so
 * the magnitude of the inactivity event is compared with the value
 * in the THRESH_INACT register.The scale factor is 62.5 mg/LSB.
 * A value of 0 may result in undesirable behavior if the inactivity
 * interrupt is enabled.
 */
#define set_THRESH_INACT(data) set_register(THRESH_INACT,data)
#define get_THRESH_INACT() get_register(THRESH_INACT)
/*
 * Register: TIME_INACT
 * The TIME_INACT register is eight bits and contains an unsigned 
 * time value representing the amount of time that acceleartion
 * must be less than the value in the THRESH_INACT register for
 * inactivity to be declared.The scale factor is 1 sec/LSB.Unlike
 * the other interrupt functions,which use unfiltered data(see the 
 * Threshold section),the inactivity function uses filtered output
 * data.At least one output sample must be generated for the 
 * inactivity interrupt to be triggered.This results in the function
 * appearing unresponsive if the TIME_INACT register is set to a
 * value less than the time contant of the output data rate.A value
 * of 0 result in an interrupt when the output data is less than the 
 * vlaue int the THRESH_INACT register.
 */
#define set_TIME_INACT(time) set_register(TIME_INACT,time)
#define get_TIME_INACT() get_register(TIME_INACT)
/*
 * Register: ACT_INACT_CTL
 * D7: ACT ac/dc
 * D3: INACT ac/dc
 * D6 - D4: ACT_x/y/z enable
 * D2 - D0: INACT_x/y/z enable
 */
#define get_ACT_INACT_CTL()     get_register(ACT_INACT_CTL)
#define set_ACT_INACT_CTL(data) set_register(ACT_INACT_CTL,data)
/*
 * A setting of 0 selects dc-coupled operation.and a setting of 1
 * enables ac-coupled operation.In dc-coupled operation,the
 * current acceleration magnitude is compared directly with
 * THRESH_ACT and THRESH_INACT to determine whether
 * a activity or inactivity is detected.
 */
#define DC_ACT_mode() CLEAR_BIT(7,ACT_INACT_CTL)
#define AC_ACT_mode() SET_BIT(7,ACT_INACT_CTL)
#define DC_INACT_mode() CLEAR_BIT(3,ACT_INACT_CTL)
#define AC_INACT_mode() SET_BIT(3,ACT_INACT_CTL)
/*
 * A setting of 1 enable x-,y-z-axis participation in deteting
 * activity or inactivity.
 * A setting of 0 excludes the selected axis from participation.
 */
#define ACT_X_enable()  SET_BIT(6,ACT_INACT_CTL)
#define ACT_X_disable() CLEAR_BIT(6,ACT_INACT_CTL)
#define ACT_Y_enable()  SET_BIT(5,ACT_INACT_CTL)
#define ACT_Y_disable() CLEAR_BIT(5,ACT_INACT_CTL)
#define ACT_Z_enable()  SET_BIT(4,ACT_INACT_CTL)
#define ACT_Z_disable() CLEAR_BIT(4,ACT_INACT_CTL)

#define INACT_X_enable()  SET_BIT(2,ACT_INACT_CTL)
#define INACT_X_disable() CLEAR_BIT(2,ACT_INACT_CTL)
#define INACT_Y_enable()  SET_BIT(1,ACT_INACT_CTL)
#define INACT_Y_disable() CLEAR_BIT(1,ACT_INACT_CTL)
#define INACT_Z_enable()  SET_BIT(0,ACT_INACT_CTL)
#define INACT_Z_disable() CLEAR_BIT(0,ACT_INACT_CTL)
/*
 * Register: THRESH_FF
 * The THRESH_FF register is eight bits and hild the threshold
 * value,in unsigned format,for free-fall detection.The acceleration on
 * all axes is compared with the value in THRESH_FF to determine if
 * a free-fall event occurred.The scale factor is 62.5 mg/LSB.Note
 * that a value of 0 mg may result in undesirable behavior if the free-
 * fall interrupt is enabled.Values between 300 mg and 600 mg
 * (0x05 to 0x09) are recommended.
 */
#define get_THRESH_FF() get_register(THRESH_FF)
#define set_THRESH_FF(data) set_register(THRESH_FF,data)
/*
 * Register: TIME_FF
 * The TIME_FF register is eight bits abd stores an unsigned time
 * value representing the minimum time that the value of all axes
 * must be less than THRESH_FF to generate a free-fall interrupt.
 * The scale factor is 5ms/LSB.A value of 0 may result in undesirable
 * behavior if the free-fall interrupt is enabled.Values between 100 ms
 * and 350 ms(0x14 to 0x46) are recommended.
 */
#define get_TIME_FF() get_register(TIME_FF)
#define set_TIME_FF(data) set_register(TIME_FF,data)
/*
 * Setting the suppress bit suppresses double tap detection if 
 * acceleration greater than the value in THRESH_TAP is present
 * between taps.
 */
#define set_suppress() SET_BIT(3,TAP_AXES)
#define clear_suppress() CLEAR_BIT(3,TAP_AXES)
/*
 * TAP_x enable 
 * A setting of 1 in the TAP_X enable,TAP_Y enable,or TAP_Z
 * enable bit enables x-,y-,or z-axis participation in tao detection.
 * A setting of 0 exclude the selected axis from participation in 
 * tap detection.
 */
#define TAP_X_enable() SET_BIT(2,TAP_AXES)
#define TAP_Y_enable() SET_BIT(1,TAP_AXES)
#define TAP_Z_enable() SET_BIT(0,TAP_AXES)
#define TAP_X_disable() CLEAR_BIT(2,TAP_AXES)
#define TAP_Y_disable() CLEAR_BIT(1,TAP_AXES)
#define TAP_Z_disable() CLEAR_BIT(0,TAP_AXES)
/*
 * Register: ACT_TAP_STATUS
 * ACT_x Source and TAP_x Source Bits
 * These bits indicate the first axis involved in a tap or activity
 * event.A setting of 1 corresponds to involvement in the event,
 * and a setting of 0 corresponds to no involvent.When new
 * data is available,these bits are not cleared but are overwriten by
 * the new data.The ACT_TAP_STATUS register should be read
 * before clearing the interrupt.Disabling an axis from participation
 * clears the correspomding source bit when the next activity or 
 * single tap/double tap event occurs.
 */
#define get_ACT_X_source() GET_BIT(6,ACT_TAP_STATUS)
#define get_ACT_Y_source() GET_BIT(5,ACT_TAP_STATUS)
#define get_ACT_Z_source() GET_BIT(4,ACT_TAP_STATUS)
#define get_TAP_X_source() GET_BIT(2,ACT_TAP_STATUS)
#define get_TAP_Y_source() GET_BIT(1,ACT_TAP_STATUS)
#define get_TAP_Z_source() GET_BIT(0,ACT_TAP_STATUS)
/*
 * Asleep bit
 * A setting of 1 in the asleep bit indicates that the part is asleep,
 * and a setting of 0 indicates that the part is not asleep.This bit
 * toggles only if the device is configured for auto sleep.
 */
#define get_Asleep()     GET_BIT(3,ACT_TAP_STATUS)
/*
 * Register: BW_RATE
 * LOW_POWER Bit
 * A setting of 0 in the LOW_POWER bit selects normal operation,
 * and a setting of 1 selects reduced power operation,which has
 * somewhat higher noise.
 */
#define low_power_enable()    SET_BIT(4,BW_RATE)
#define low_power_disable()   CLEAR_BIT(4,BW_RATE)
#define get_low_power_state() GET_BIT(4,BW_RATE)
/*
 * Rate Bits
 * These bits select the device bandwith and output data rate.
 * The default value is 0x0A,Which translates to a 100Hz output 
 * data rate.An output data rate should be select that is appropriate
 * for the communication protocol and frequency selected.Selecting too 
 * high of an output data rate with a low communication speed 
 * results in samples being discarded.
 */
#define get_bandrate()      GET_BITS(3,0,BW_RATE)
#define set_bandrate(rate)  SET_BITS(3,0,BW_RATE,rate)
/*
 * Register: POWER_CTL
 * Link Bit
 * A setting of 1 in the link bit with both the activity and inactivity
 * function enabled delays the start of the activity function until
 * inactivity is detected.After activity is detected,inactivity detection
 * begins,preventing the detection of activity.This bit serially links
 * the activity and inactivity functions.When thisbit is set to 0,
 * the inactivity and inactivity functions are concurrent.Additional
 * information can be found in the Link Mode section.
 */
#define activity_mode()     SET_BIT(5,POWER_CTL)
#define inactivity_mode()   SET_BIT(5,POWER_CTL)
#define standby_mode()      CLEAR_BIT(5,POWER_CTL)
/*
 * Auto Sleep Bit
 * If the link bit is set,a setting of 1 in the AUTO_SLEEP bit enables
 * the auto-sleep functionality.In this mode,the ADXL345 
 * automatically switches to sleep mode if the incativity function is
 * enable and inactivity is detected(that is,when acceleration is
 * below the THRESH_INACT value for at least the time indicated
 * by TIME_INACT).If activity is also enabled,the ADXL345
 * automatically wakes up from sleep after detecting activity and
 * returns to operation at the output data rate set in the BW_RATE
 * register.A setting of 0 in the AUTO_SLEEP bit disables automatic 
 * switching to sleep mode.
 */
#define AUTO_SLEEP_mode()   SET_BITS(5,4,POWER_CTL,0x03)
#define is_AUTO_SLEEP_mode()     (0x03 == GET_BITS(5,4,POWER_CTL))
/*
 * Measure Bit
 * A setting of 0 in the measure bit place the part into standby mode,
 * and a setting of 1 places the part into measurement mode.The 
 * ADXL345 power up in standby mode with minimum power consumption.
 */
#define STANDBY_mode()			SET_BIT(3,POWER_CTL)
#define is_STANDBY_mode()       (0x01 == GET_BIT(3,POWER_CTL))
#define MEASUREMENT_mode()      CLEAR_BIT(3,POWER_CTL)
#define is_MEASUREMENT_mode()   (0x00 == GET_BIT(3,POWER_CTL))
/*
 * Sleep Bit
 * A setting of 0 in the sleep bit puts the part into the normal mode
 * of operation,and a setting of 1 places the part into sleep mode.
 * Sleep mode suppress DATA_READY,stops transmission of data to FIFO,
 * and switches the sampling rate to one specified by the wakeup bits.
 * In sleep mode,only the activity function can be used.
 * When the DATA_READY interrupt is suppressed,the output data register
 * are still updated at the sampling rate set by the wakeup bits(D1:D0)
 */
#define set_NORMAL_mode()          SET_BIT(2,POWER_CTL)
#define is_NORMAL_mode()           (0x01 == GET_BIT(2,POWER_CTL))
#define set_SLEEP_mode()           CLEAR_BIT(2,POWER_CTL)
#define is_SLEEP_mode()            (0x00 == GET_BIT(2,POWER_CTL))
/*
 * Wakeup Bits
 * These bits control the frequency of reading in sleep mode as
 * D1    D0    Frequency(Hz)
 * 0     0       8
 * 0     1       4
 * 1     0       2
 * 1     1       1
 */
#define set_wakeup_rate(rate) SET_BITS(1,0,POWER_CTL,rate)
#define get_wakeup_rate()     GET_BITS(1,0,POWER_CTL)
/*
 * Register: INT_ENABLE
 * Setting bits in this register to a value of 1 enables their respective
 * function to generate interrupts,whereas a value of 0 prevents
 * the functions from generating interrupts.The DATA_READY,
 * watermark,and overrun bits enable only the interrupt output;
 * the functions are always enabled.it is recommended that interrupts
 * be configured before enabling their outputs.
 */
#define set_INT_ENABLE(data)       set_register(INT_ENABLE,data)
#define INT_DATA_READ_enable()     SET_BIT(7,INT_ENABLE)
#define INT_DATA_READ_disable()    CLEAR_BIT(7,INT_ENABLE)
#define INT_SINGLE_TAP_enable()    SET_BIT(6,INT_ENABLE)
#define INT_SINGLE_TAP_disable()   CLEAR_BIT(6,INT_ENABLE)
#define INT_DOUBLE_TAP_enable()    SET_BIT(5,INT_ENABLE)
#define INT_DOUBLE_TAP_disable()   CLEAR_BIT(5,INT_ENABLE)
#define INT_Activity_enable()      SET_BIT(4,INT_ENABLE)
#define INT_Activity_disable()     CLEAR_BIT(4,INT_ENABLE)
#define INT_Inactivity_enable()    SET_BIT(3,INT_ENABLE)
#define INT_Inactivity_disable()   CLEAR_BIT(3,INT_ENABLE)
#define INT_FREE_FALL_enable()     SET_BIT(2,INT_ENABLE)
#define INT_FREE_FALL_disable()    CLEAR_BIT(2,INT_ENABLE)
#define INT_Watermark_enable()     SET_BIT(1,INT_ENABLE)
#define INT_watermark_disable()    CLEAR_BIT(1,INT_ENABLE)
#define INT_Overrun_enable()       SET_BIT(0,INT_ENABLE)
#define INT_Overrun_disable()      CLEAR_BIT(0,INT_ENABLE)
/*
 * Register: INT_MAP
 * Any bits set to 0 in this register send their respective interrupts to
 * the INT1 pin,whereas bits set to 1 send their respective interrupts
 * to the INT2 pin.All selected interrupts for a given pin are OR'ed.
 */
#define set_INT_DATA_READY_INT1()      CLEAR_BIT(7,INT_MAP)
#define set_INT_DATA_READY_INT2()      SET_BIT(7,INT_MAP)
#define is_DATA_READY_INT1()           (0x00 == GET_BIT(7,INT_MAP))
#define is_DATA_READY_INT2()           (0x01 == GET_BIT(7,INT_MAP))

#define set_INT_SINGLE_TAP_INT1()	   CLEAR_BIT(6,INT_MAP)
#define set_INT_SINGLE_TAP_INT2()      SET_BIT(6,INT_MAP)
#define is_SINGLE_TAP_INT1()           (0x00 == GET_BIT(6,INT_MAP))
#define is_SINGLE_TAP_INT2()           (0x01 == GET_BIT(6,INT_MAP))

#define set_INT_DOUBLE_TAP_INT1()      CLEAR_BIT(5,INT_MAP)
#define set_INT_DOUBLE_TAP_INT2()      SET_BIT(5,INT_MAP)
#define is_DOUBLE_TAP_INT1()           (0x00 == GET_BIT(5,INT_MAP))
#define is_DOUBLE_TAP_INT2()           (0x01 == GET_BIT(5,INT_MAP))

#define set_INT_Activity_INT1()        CLEAR_BIT(4,INT_MAP)
#define set_INT_Activity_INT2()        SET_BIT(4,INT_MAP)
#define is_Activity_INT1()             (0x00 == GET_BIT(4,INT_MAP))
#define is_Activity_INT2()             (0x01 == GET_BIT(4,INT_MAP))

#define set_INT_Inactivity_INT1()      CLEAR_BIT(3,INT_MAP)
#define set_INT_Inactivity_INT2()      SET_BIT(3,INT_MAP)
#define is_Inactivity_INT1()           (0x00 == GET_BIT(3,INT_MAP))
#define is_Inactivity_INT2()           (0x01 == GET_BIT(3,INT_MAP))

#define set_INT_FREE_FALL_INT1()       CLEAR_BIT(2,INT_MAP)
#define set_INT_FREE_FALL_INT2()       SET_BIT(2,INT_MAP)
#define is_FREE_FALL_INT1()            (0x00 == GET_BIT(2,INT_MAP))
#define is_FREE_FALL_INT2()            (0x01 == GET_BIT(2,INT_MAP))

#define set_INT_Watermark_INT1()       CLEAR_BIT(1,INT_MAP)
#define set_INT_Watermark_INT2()       SET_BIT(1,INT_MAP)
#define is_Watermark_INT1()            (0x00 == GET_BIT(1,INT_MAP))
#define is_Watermark_INT2()            (0x01 == GET_BIT(1,INT_MAP))

#define set_INT_Overrun_INT1()         CLEAR_BIT(0,INT_MAP)
#define set_INT_Overrun_INT2()         SET_BIT(0,INT_MAP)
#define is_Overrun_INT1()              (0x00 == GET_BIT(0,INT_MAP))
#define is_Overrun_INT2()              (0x01 == GET_BIT(0,INT_MAP))
/*
 * Register: INT_SOURCE
 * Bits set to 1 in this register indicate that their respective functions
 * have triggered an event,whereas a value of 0 indicates that the
 * corresponding event has not occurred.The DATA_READY,
 * watermark,and overrun bits are always set if the corresponding
 * events occur,regardless of the INT_ENABLE register settings,
 * and are cleared by reading data from the DATAX,DATAY,and DATAZ
 * register.The DATA_READY and watermark bits may require multiple reads,
 * as indicated in the FIFO mode descriptions in the FIFO section.Other 
 * bits,and the corresponding interrupts,are cleared by reading the 
 * INT_SOURCE register.
 */
#define is_INT_DATA_READY()       GET_BIT(7,INT_SOURCE)
#define is_INT_SINGLE_TAP()       GET_BIT(6,INT_SOURCE)
#define is_INT_DOUBLE_TAP()       GET_BIT(5,INT_SOURCE)
#define is_INT_Activity()         GET_BIT(4,INT_SOURCE)
#define is_INT_Inactivity()       GET_BIT(3,INT_SOURCE)
#define is_INT_FREE_FALL()        GET_BIT(2,INT_SOURCE)
#define is_INT_Watermark()        GET_BIT(1,INT_SOURCE)
#define is_INT_Overrun()          GET_BIT(0,INT_SOURCE)
/*
 * Register: DATA_FORMAT
 * The DATA_FORMAT register controls the presentation of data
 * to Register 0x32 throught Register 0x37.All data,except that for
 * the +16/-16 g range,must be clipped to avoid rollover.
 */
#define get_DATA_FORMAT()   get_register(DATA_FORMAT)
/* 
 * SELF_TEST Bit.
 * A setting of 1 in the SELF_TEST bit applies a self-test force to 
 * the sensor,causing a shift in the output data.A value of 0 disable
 * the self-get force.
 */
#define self_test_enable()     SET_BIT(7,DATA_FORMAT)
#define self_test_disable()    CLEAR_BIT(7,DATA_FORMAT)
#define is_self_test()         (0x01 == GET_BIT(7,DATA_FORMAT))
/*
 * SPI Bit
 * A value of 1 in the SPI bit sets the device to 3-wire SPI mode,
 * and a value of 0 sets the device to 4-write SPI mode.
 */
#define Wire3_SPI_mode()        SET_BIT(6,DATA_FORMAT)
#define Wire4_SPI_mode()        CLEAR_BIT(6,DATA_FORMAT)
#define is_SPI3_mode()          (0x01 == GET_BIT(6,DATA_FORMAT))
#define is_SPI4_mode()          (0x00 == GET_BIT(6,DATA_FORMAT))
/*
 * INT_INVERT Bit
 * A value of 0 in the INT_INVERT bit sets the interrupts to active
 * high,and a value of 1 set the interrupts to active low.
 */
#define set_INT_high()           CLEAR_BIT(5,DATA_FORMAT)
#define set_INT_low()            SET_BIT(5,DATA_FORMAT)
#define is_INT_high()            (0x00 == GET_BIT(5,DATA_FORMAT))
#define is_INT_low()             (0x01 == GET_BIT(5,DATA_FORMAT))
/*
 * FULL_RES Bit 
 * When this bit is set to a value of 1,the device is in full resolution
 * mode,where the output resolution increases with the g range 
 * set by the range bits to maintain a 4 mg/LSB scale factor.When
 * the FULL_RES bit is set to 0,the device is in 10-bit mode,and 
 * the range bits determine the maximum g range and scale factor.
 */
#define set_FULL_RES_mode()          SET_BIT(3,DATA_FORMAT)
#define set_10_bit_mode()            CLEAR_BIT(3,DATA_FORMAT)
#define is_full_resolution_mode()    (0x01 == GET_BIT(3,DATA_FORMAT))
#define is_10_bit_mode()             (0x00 == GET_BIT(3,DATA_FORMAT))
/*
 * Justify Bit 
 * A setting of 1 in the justify bit selects left-justified(MSB) mode,
 * and a setting of 0 selects right-justified mode with sign extension.
 */
#define set_MSB_mode()           SET_BIT(1,DATA_FORMAT)
#define set_LSB_mode()           CLEAR_BIT(1,DATA_FORMAT)
#define is_MSB_mode()            (0x01 == GET_BIT(1,DATA_FORMAT))
#define is_LSB_mode()            (0x00 == GET_BIT(1,DATA_FORMAT))
/*
 * Range Bits
 * These bits set the g range as:
 * D1    D0     gRange
 * 0     0      +/- 2g
 * 0     1      +/- 4g
 * 1     0      +/- 8g
 * 1     1      +/- 16g
 */
#define set_g_range(range) SET_BITS(1,0,DATA_FORMAT,range)
#define get_g_range()      GET_BITS(1,0,DATA_FORMAT)
/*
 * Register: DATAX0 -- DATZ1
 * These six bytes are eight bits each and hold the output data for 
 * each axis.The output data is twos complement,with DATAx0 as the
 * least significant byte and DATAx1 as the most significant byte,
 * where x represent X,Y,or Z.The DATA_FORMAT register controls
 * the format of the data.It is recommended that a multiple-byte
 * read of all register be preformed to prevent a change in data
 * between reads of sequential reigsters.
 */
#define get_X0_data() get_register(DATAX0)
#define get_X1_data() get_register(DATAX1)
#define get_Y0_data() get_register(DATAY0)
#define get_Y1_data() get_register(DATAY1)
#define get_Z0_data() get_register(DATAZ0)
#define get_Z1_data() get_register(DATAZ1)
/*
 * Register: FIFO_CTL
 */
#define get_FIFO_CTL() get_register(FIFO_CTL)
/*
 * FIFO_MODE Bits
 * These bits set the FIFO mode
 *  D7  D6   Mode      Function
 *  0   0    Bypass    FIFO is bypassed
 *  0   1    FIFO      FIFO collects up to 32 values and then
 *                     stops collecting data,collecting new data
 *                     only when FIFO is not full.
 *  1   0    Stream    FIFO holds the last 32 data values.When
 *                     FIFO is full,the oldest data is overwriten
 *                     with newer data.
 *  1   1    Trigger   When triggered by the trigger bit,FIFO
 *                     holds the last data samples before the
 *                     trigger event and then continues to collect
 *                     data until full.New data is collected only
 *                     when FIFO is not full.
 */
#define FIFO_Bypass_mode()        SET_BITS(7,6,FIFO_CTL,0x00)
#define is_FIFO_Bypass_mode()     (0x00 == GET_BITS(7,6,FIFO_CTL))

#define FIFO_FIFO_mode()		  SET_BITS(7,6,FIFO_CTL,0x01)
#define is_FIFO_FIFO_mode()       (0x01 == GET_BITS(7,6,FIFO_CTL))

#define FIFO_Stream_mode()        SET_BITS(7,6,FIFO_CTL,0x02)
#define is_FIFO_Stream_mode()     (0x02 == GET_BITS(7,6,FIFO_CTL))

#define FIFO_Trigger_mode()       SET_BITS(7,6,FIFO_CTL,0x03)
#define is_FIFO_Trigger_mode()    (0x03 == GET_BITS(7,6,FIFO_CTL))
/*
 * Trigger Bit
 * A value of 0 in the trigger bit links the trigger event of trigger mode
 * to INT1,and a value of 1 links the trigger event to INT2.
 */
#define set_INT_Trigger_INT1()    CLEAR_BIT(5,FIFO_CTL)
#define set_INT_Trigger_INT2()    SET_BIT(5,FIFO_CTL)
#define is_Trigger_INT1()         (0x00 == GET_BIT(5,FIFO_CTL))
#define is_Trigger_INT2()         (0x01 == GET_BIT(5,FIFO_CTL))
/*
 * Samples Bits
 * The function of these bits depends on the FIFO mode selected.
 * Entering a value of 0 in the samples bits immediately set the 
 * watermark status bit in the INT_SOURCE register,regardless of 
 * which FIFO mode is selected.Undesirable operation may occur if 
 * a value of 0 is used for the samples bits when trigger mode is
 * used.
 * FIFO Mode    Sample Bits Function
 * Bypass       None
 * FIFO         Specifies how many FIFO entries are needed to
 *              trigger a watermark interrupt.
 * Stream       Specifies how many FIFO entries are needed to
 *              trigger a watermark interrupt.
 * Trigger      Specifies how many FIFO samples are retained in 
 *              the FIFO buffer before a trigger event.
 */
#define set_sample_data(data) SET_BITS(4,0,FIFO_CTL,data)
#define get_sample_data()     GET_BITS(4,0,FIFO_CTL,data)
/*
 * Register: FIFO_STATUS
 * FIFO_TRIG Bit
 * A 1 in the FIFO_TRIG bit corresponds to a trigger event occurring,
 * and a 0 means that a FIFO trigger event has not occurred.
 */
#define is_FIFO_TRIG()   (0x00 == GET_BIT(7,FIFO_STATUS))
/*
 * Entries Bits
 * These bits report how many data values are stored in FIFO.
 * Access to collect the data from FIFO is provided through the
 * DATAX,DATAY,and DATAZ registers.FIFO reads must be done in 
 * burst or multiple-byte mode because each FIFO level is
 * cleared after any read of FIFO.FIFO stores a maximum of 32
 * entries,which equates to a maximum of 33 entries available at 
 * any given time because an additional entry is available at 
 * the output filter of the device.
 */
#define get_FIFO_entries()    GET_BITS(5,0,FIFO_STATUS)
/*
 * Get X-,Y-,and Z-AXIS DATA.
 * Use: 10 bit mode.
 */
static int ADXL345_get_data(unsigned long *data_x,unsigned long *data_y,
		unsigned long *data_z)
{
	unsigned long x,y,z;

	set_10_bit_mode();
    TAP_X_enable();	
    TAP_Y_enable();	
    TAP_Z_enable();	
	x = get_X1_data() & 0x03;
	x <<= 8;
	x |= get_X0_data();

	y = get_Y1_data() & 0x03;
	y <<= 8;
	y |= get_Y0_data();

	z = get_Z1_data() & 0x03;
	z <<= 8;
	z |= get_Z0_data();

	*data_x = x;
	*data_y = y;
	*data_z = z;

	return 0;
}
/*
 * IRQ & INPUT operation
 */
/*
 * IRQ handle -- tasklet
 */
static irqreturn_t ADXL345_handle_tasklet1(int irq,void *dev_id)
{
	mm_debug(KERN_INFO "[%s]Interrupt...\n",__FUNCTION__);
	input_report_key(ADXL345_dev,BTN_0,0x45);
	input_sync(ADXL345_dev);

	return IRQ_HANDLED;
}
static irqreturn_t ADXL345_handle_tasklet2(int irq,void *dev_id)
{
	mm_debug(KERN_INFO "[%s]Interrupt2...\n",__FUNCTION__);

	return IRQ_HANDLED;
}
/*
 * The operation of Char device.
 */
#ifndef pppppppppppp
#endif
/*
 * Read operation for file_operations.
 */
static ssize_t ADXL345_read(struct file *filp,char __user *buf,
		size_t count,loff_t *offset)
{
	unsigned long x,y,z;
	int i = 50;

	STANDBY_mode();
	do {
		mdelay(1000);
		ADXL345_get_data(&x,&y,&z);
		mm_debug(KERN_INFO "[%s]x:%ld y:%ld z:%ld\n",__FUNCTION__,x,y,z);
	} while(i--);
	return 0;
}
/*
 * Write operation for file_operations.
 */
static ssize_t ADXL345_write(struct file *filp,const char __user *buf,
		size_t count,loff_t *offset)
{
	mm_debug(KERN_INFO "[%s]\n",__FUNCTION__);

	return 0;
}
/*
 * Open operation for file_operations
 */
static int ADXL345_open(struct inode *inode,struct file *filp)
{
	mm_debug(KERN_INFO "[%s]\n",__FUNCTION__);
	filp->private_data = (void *)my_client;
	return 0;
}
/*
 * Close operation for file_operations
 */
static int ADXL345_release(struct inode *inode,struct file *filp)
{
	mm_debug(KERN_INFO "[%s]\n",__FUNCTION__);

	return 0;
}
/*
 * File_operations
 */
static const struct file_operations ADXL345_fops = {
	.owner       = THIS_MODULE,
	.read        = ADXL345_read,
	.write       = ADXL345_write,
	.open        = ADXL345_open,
	.release     = ADXL345_release,
};
/*
 * device id table.
 */
static struct i2c_device_id ADXL345_id[] = {
	{DEV_NAME,I2C_ADDR},
	{},
};
/*
 * I2C probe
 */
static int ADXL345_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int res;
	struct device *dev;

	mm_debug(KERN_INFO "[%s]Probe\n",__FUNCTION__);
	my_client = client;
	/*
	 * Atomic create i2c node of /dev/
	 */
	dev = device_create(i2c_class,&my_client->dev,MKDEV(i2c_major,0),NULL,
			DEV_NAME);
	if(IS_ERR(dev))
	{
		res = PTR_ERR(dev);
		goto error;
	}
	/*
	 * Read ID_Register virtually.
	 */
	chip_id();
	/*
	 * Init interrupt
	 */
	res = request_irq(IRQ_EINT(0),ADXL345_handle_tasklet1,IRQF_TRIGGER_RISING,
			"INIT1",NULL);
	if(res != 0)
	{
		mm_err(KERN_INFO "[%s]ERR:Can't allocate IRQ1\n",__FUNCTION__);
		goto out_device;
	}
	res = request_irq(IRQ_EINT(3),ADXL345_handle_tasklet2,IRQF_TRIGGER_RISING,
			"INIT2",NULL);
	if(res != 0)
	{
		mm_err(KERN_INFO "[%s]ERR:Can't allocate IRQ2\n",__FUNCTION__);
		goto out_irq1;
	}
	/*
	 * Init input system.
	 */
	ADXL345_dev = input_allocate_device();
	if(ADXL345_dev == NULL)
	{
		mm_err(KERN_INFO "[%s]ERR:Can't alloc a new input device\n",__FUNCTION__);
		res = -ENOMEM;
		goto out_irq2;
	}
	ADXL345_dev->evbit[0] = BIT_MASK(EV_KEY);
	ADXL345_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);

	res = input_register_device(ADXL345_dev);
	if(res != 0)
	{
		mm_err(KERN_INFO "[%s]ERR:Cant register into input subsystem\n",__FUNCTION__);
		goto out_input;
	}

	/* Finish */
	mm_debug(KERN_INFO "[%s]Probe finish\n",__FUNCTION__);

	return 0;

out_input:
	input_free_device(ADXL345_dev);
out_irq2:
	free_irq(IRQ_EINT(3),NULL);
out_irq1:
	free_irq(IRQ_EINT(0),NULL);
out_device:	
	device_destroy(i2c_class,MKDEV(i2c_major,0));
error:
	return res;
}
/*
 * I2C remove
 */
static int ADXL345_remove(struct i2c_client *client)
{
	mm_debug(KERN_INFO "[%s]I2C Remove\n",__FUNCTION__);
	/*
	 * Free irq.
	 */
	input_unregister_device(ADXL345_dev);
	input_free_device(ADXL345_dev);
	free_irq(IRQ_EINT(0),NULL);
	free_irq(IRQ_EINT(3),NULL);
	mm_debug(KERN_INFO "[%s]IRQ Free\n",__FUNCTION__);
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver.
 */
static struct i2c_driver ADXL345_driver = {
	.driver = {
		.name = DEV_NAME,
	},
	.probe    = ADXL345_probe,
	.id_table = ADXL345_id,
	.remove   = ADXL345_remove,
};
/*
 * Module init.
 */
static __init int ADXL345_init(void)
{
	int ret;
	/*
	 * Add i2c-board information.
	 */
	struct i2c_adapter *adap;
	struct i2c_board_info i2c_info;

	mm_debug(KERN_INFO "[%s]Initialize Module\n",__FUNCTION__);

	/*
	 * I2C Board information.
	 */
	adap = i2c_get_adapter(0);
	if(adap == NULL)
	{
		mm_err(KERN_INFO "ERR[%s]Unable to get I2C adapter\n",
				__FUNCTION__);
		ret = -EBUSY;
		goto out;
	}
	memset(&i2c_info,0,sizeof(struct i2c_board_info));
	strlcpy(i2c_info.type,DEV_NAME,I2C_NAME_SIZE);
	i2c_info.addr = I2C_ADDR;

	/* 
	 * Add the device in /sys/bus/i2c/device/I2C_ADDR 
	 */
	my_client = i2c_new_device(adap,&i2c_info);
	i2c_put_adapter(adap);
	if(my_client == NULL)
	{
		mm_err(KERN_INFO "ERR[%s]Unable to get new i2c device\n",
				__FUNCTION__);
		ret = -ENODEV;
		goto out;
	}
	/*
	 * Register the char device
	 */
	i2c_major = register_chrdev(0,DEV_NAME,&ADXL345_fops);
	if(i2c_major == 0)
	{
		mm_err(KERN_INFO "ERR[%s]Can't register char driver\n",
				__FUNCTION__);
		ret = -ENODEV;
		goto out_i2c_dev;
	}
	/*
	 * Add auto create node.Add the device to 
	 * /sys/bus/i2c
	 */
	i2c_class = class_create(THIS_MODULE,DEV_NAME);
	if(IS_ERR(i2c_class))
	{
		ret = PTR_ERR(i2c_class);
		mm_err(KERN_INFO "ERR[%s][%d]Unable to create class\n",
				__FUNCTION__,ret);
		goto out_register;
	}
	/*
	 * Add new i2c driver.Add driver into 
	 * /sys/bus/i2c/device/I2C_ADDr/driver
	 */
	ret = i2c_add_driver(&ADXL345_driver);
	if(ret)
	{
		mm_err(KERN_INFO "ERR[%s]Unable to add new driver to I2C\n",
				__FUNCTION__);
		ret = -EBUSY;
		goto out_class;
	}
	mm_debug(KERN_INFO "[%s]Succeed to init\n",__FUNCTION__);
	return 0;
out_class:
	class_destroy(i2c_class);
out_register:
	unregister_chrdev(i2c_major,DEV_NAME);
out_i2c_dev:
	i2c_unregister_device(my_client);
out:
	return ret;
}
/*
 * Module exit.
 */
static __exit void ADXL345_exit(void)
{
	mm_debug(KERN_INFO "[%s]Exit Module\n",__FUNCTION__);
	i2c_del_driver(&ADXL345_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,DEV_NAME);
	i2c_unregister_device(my_client);
}

MODULE_AUTHOR("Buddy <Buddy.D.Zhang@outlook.com>");
MODULE_DESCRIPTION("ADXL345 Gsensor\n");
MODULE_LICENSE("GPL");

module_init(ADXL345_init);
module_exit(ADXL345_exit);
