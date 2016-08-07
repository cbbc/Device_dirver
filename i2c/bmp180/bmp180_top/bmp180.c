/*
 * Copyright@2014 Buddy Zhangrenze
 */
/*
 * General description
 * The BMP180 consists of a piezo-resistive sensor,an analog to digital 
 * converter and a control unit with EEPROM and a serial I2C interface.
 * The BMP180 delivers the the uncomprnsated value of pressure and temperature.
 * The EEPROM has stored 176 bit of individual calibration data.This is 
 * used to compensate offset,temperature dependence and other parameters 
 * of the sensor.
 * UP = pressure data(16 to 19 bit)
 * UT = temperature(16 bit)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/delay.h>

#define DEV_NAME "bmp180"
#define DEBUG          0
#define TEST           0
/*
 * BMP180 register address
 * | Parmenter |  MSB  |  LSB  |
 * |    AC1    | 0xAA  | 0xAB  |
 * |    AC2    | 0xAC  | 0xAD  |
 * |    AC3    | 0xAE  | 0xAF  |
 * |    AC4    | 0xB0  | 0xB1  |
 * |    AC5    | 0xB2  | 0xB3  |
 * |    AC6    | 0xB4  | 0xB5  |
 * |    B1     | 0xB6  | 0xB7  |
 * |    B2     | 0xB8  | 0xB9  |
 * |    MB     | 0xBA  | 0xBB  |
 * |    MC     | 0xBC  | 0xBD  |
 * |    MD     | 0xBE  | 0xBF  |
 * MSB first.
 */
static struct class *i2c_class;
static struct i2c_client *my_client;
static int i2c_major;

#if DEBUG
static void bmp180_debug(struct i2c_client *client,char *name)
{
	printk(KERN_INFO "======================%s=======================\n=>client->name:%s\n=>client->addr:%08x\n=>adapter->name:%s\n",
			name,client->name,client->addr,client->adapter->name);
}
#endif 
/*
 * Register Operation
 */
/*
 * I2C read - read one byte
 * @client: an i2c client device.
 * @addr:register address.
 * @buf:read buffer.
 * @num:buffer length.
 * return 0 success or 1 failure.
 */
int buddy_i2c_read(struct i2c_client *client,int addr,char *buf,int num)
{
	struct i2c_msg msg[2];
	int ret;
	/* write operation */
	msg[0].addr   = client->addr;
	msg[0].flags  = I2C_M_TEN | client->flags;
	msg[0].buf    = (void *)&addr;
	msg[0].len    = 1;
	/* read operation */
	msg[1].addr   = client->addr;
	msg[1].flags  = I2C_M_TEN | client->flags;
	msg[1].flags |= I2C_M_RD;
	msg[1].buf    = buf;
	msg[1].len    = num;
	
	ret = i2c_transfer(client->adapter,msg,2);
	if(ret != 2)
	{
		printk(KERN_INFO "Fail to transfer\n");
		return 1;
	} else
	//	printk(KERN_INFO "Succeed to transfer\n");
	return 0;

}
/*
 * I2C write - write one byte
 * @client:a i2c client.
 * @addr:register address.
 * @buf:write buffer.
 * @num:buffer's length.
 * return 0 succeess or 1 failure.
 */
int buddy_i2c_write(struct i2c_client *client,int addr,char *buf,int num)
{
	int ret;
	struct i2c_msg msg;
	char data[2];
	data[0] = (char)addr;
	data[1] = *buf;
	/* write register address */
	msg.addr   = client->addr;
	msg.flags  = client->flags | I2C_M_TEN;
	msg.buf    = (void *)data;
	msg.len    = 2;

	ret = i2c_transfer(client->adapter,&msg,1);
	if(ret != 1)
	{
		printk(KERN_INFO "Fail to transfer data\n");
		return 1;
	} else
	//	printk(KERN_INFO "Succeed to transfer data\n");
	return 0;
}
/*
 * Chip-id operation - an read-only register.
 * Get chip's id.
 */
void bmp180_ID(struct i2c_client *client,char *id)
{
	int addr = 0xD0;
	buddy_i2c_read(client,addr,id,1);
}
/*
 * Check chip is communication.
 * return 0 success or 1 failure.
 */
int bmp180_com_state(struct i2c_client *client)
{
	char bmp180_id = 0;
	bmp180_ID(client,&bmp180_id);
	if(bmp180_id == 0x55)
		return 0;
	else
		return 1;
}
/*
 * Read AC1 register - Get AC1 data.
 */
void bmp180_AC1_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xAA,buf,1);
#if TEST
	printk(KERN_INFO "=>AC1.MSB[%d]\n",*buf);
#endif
}
void bmp180_AC1_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xAB,buf,1);
#if TEST
	printk(KERN_INFO "=>AC1.LSB[%d]\n",*buf);
#endif
}
void bmp180_AC1(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_AC1_MSB(client,&buf_msb);
	bmp180_AC1_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf<<=8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>AC1.[%d]\n",*buf);
#endif
}
/*
 * Read AC2 register - Get AC2 data.
 */
void bmp180_AC2_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xAC,buf,1);
#if TEST
	printk(KERN_INFO "=>AC2.MSB[%d]\n",*buf);
#endif
}
void bmp180_AC2_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xAD,buf,1);
#if TEST
	printk(KERN_INFO "=>AC2.LSB[%d]\n",*buf);
#endif
}
void bmp180_AC2(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_AC2_MSB(client,&buf_msb);
	bmp180_AC2_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf<<=8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>AC2.[%d]\n",*buf);
#endif
	
}
/*
 * Read AC3 register - Get AC3 data.
 */
void bmp180_AC3_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xAE,buf,1);
#if TEST
	printk(KERN_INFO "=>AC3.MSB[%d]\n",*buf);
#endif
}
void bmp180_AC3_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xAF,buf,1);
#if TEST
	printk(KERN_INFO "=>AC3.LSB[%d]\n",*buf);
#endif
}
void bmp180_AC3(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_AC3_MSB(client,&buf_msb);
	bmp180_AC3_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>AC3.[%d]\n",*buf);
#endif
}
/*
 * Read AC4 register - Get AC4 data.
 */
void bmp180_AC4_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB0,buf,1);
#if TEST
	printk(KERN_INFO "=>AC4.MSB[%d]\n",*buf);
#endif
}
void bmp180_AC4_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB1,buf,1);
#if TEST
	printk(KERN_INFO "=>AC4.LSB[%d]\n",*buf);
#endif
}
void bmp180_AC4(struct i2c_client *client,unsigned short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_AC4_MSB(client,&buf_msb);
	bmp180_AC4_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>AC4.[%d]\n",*buf);
#endif
}
/*
 * Read AC5 register - Get AC5 data.
 */
void bmp180_AC5_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB2,buf,1);
#if TEST
	printk(KERN_INFO "=>AC5.MSB[%d]\n",*buf);
#endif
}
void bmp180_AC5_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB3,buf,1);
#if TEST
	printk(KERN_INFO "=>AC5.LSB[%d]\n",*buf);
#endif
}
void bmp180_AC5(struct i2c_client *client,unsigned short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_AC5_MSB(client,&buf_msb);
	bmp180_AC5_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb; 
#if TEST
	printk(KERN_INFO "=>AC5.[%d]\n",*buf);
#endif
}
/*
 * Read AC6 register - Get AC6 data.
 */
void bmp180_AC6_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB4,buf,1);
#if TEST
	printk(KERN_INFO "=>AC6.MSB[%d]\n",*buf);
#endif
}
void bmp180_AC6_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB5,buf,1);
#if TEST
	printk(KERN_INFO "=>AC6.LSB[%d]\n",*buf);
#endif
}
void bmp180_AC6(struct i2c_client *client,unsigned short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_AC6_MSB(client,&buf_msb);
	bmp180_AC6_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>AC6.[%d]\n",*buf);
#endif
}
/*
 * Read B1 register - Get B1 data.
 */
void bmp180_B1_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB6,buf,1);
#if TEST
	printk(KERN_INFO "=>B1.MSB[%d]\n",*buf);
#endif
}
void bmp180_B1_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB7,buf,1);
#if TEST
	printk(KERN_INFO "=>B1.LSB[%d]\n",*buf);
#endif
}
void bmp180_B1(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_B1_MSB(client,&buf_msb);
	bmp180_B1_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>B1.[%d]\n",*buf);
#endif
}
/*
 * Read B2 register - Get B2 data.
 */
void bmp180_B2_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB8,buf,1);
#if TEST
	printk(KERN_INFO "=>B2.MSB[%d]\n",*buf);
#endif
}
void bmp180_B2_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xB9,buf,1);
#if TEST
	printk(KERN_INFO "=>B2.LSB[%d]\n",*buf);
#endif
}
void bmp180_B2(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_B2_MSB(client,&buf_msb);
	bmp180_B2_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>B2.[%d]\n",*buf);
#endif
}
/*
 * Read MB register - Get MB data.
 */
void bmp180_MB_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xBA,buf,1);
#if TEST
	printk(KERN_INFO "=>MB.MSB[%d]\n",*buf);
#endif
}
void bmp180_MB_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xBB,buf,1);
#if TEST
	printk(KERN_INFO "=>MB.LSB.[%d]\n",*buf);
#endif
}
void bmp180_MB(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_MB_MSB(client,&buf_msb);
	bmp180_MB_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>MB.[%d]\n",*buf);
#endif
}
/*
 * Read MC register - Get MC data.
 */
void bmp180_MC_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xBC,buf,1);
#if TEST
	printk(KERN_INFO "=>MC.MSB[%d]\n",*buf);
#endif
}
void bmp180_MC_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xBD,buf,1);
#if TEST
	printk(KERN_INFO "=>MC.LSB[%d]\n",*buf);
#endif
}
void bmp180_MC(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_MC_MSB(client,&buf_msb);
	bmp180_MC_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>MC.[%d]\n",*buf);
#endif
}
/*
 * Read MD register - Get MD data.
 */
void bmp180_MD_MSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xBE,buf,1);
#if TEST
	printk(KERN_INFO "=>MD.MSB[%d]\n",*buf);
#endif
}
void bmp180_MD_LSB(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xBF,buf,1);
#if TEST
	printk(KERN_INFO "=>MD.LSB[%d]\n",*buf);
#endif
}
void bmp180_MD(struct i2c_client *client,short *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_MD_MSB(client,&buf_msb);
	bmp180_MD_LSB(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xFF00;
	*buf |= buf_lsb;
#if TEST
	printk(KERN_INFO "=>MD.[%d]\n",*buf);
#endif
}
/*
 * Measurement control register
 */
void bmp180_MeasureCtr_read(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xF4,buf,1);
	*buf = *buf & 0x1F;
#if TEST
	printk(KERN_INFO "=>MeasureCtr.[%08x]\n",*buf);
#endif
}
void tmp180_MeasureCtr_write(struct i2c_client *client,char *buf)
{
	char buf_tmp;
	buddy_i2c_read(client,0xF4,&buf_tmp,1);
	buf_tmp = buf_tmp & 0xE0;
	*buf     = *buf & 0x1F;
	buf_tmp |= *buf;
	buddy_i2c_write(client,0xF4,&buf_tmp,1);
}
/*
 * Sco register - start of conversion.
 * The value of this bit stays "1" during conversion and is reset to 
 * "0" after conversion is complete(data register are filled).
 */
void bmp180_Start_Conv(struct i2c_client *client)
{
	char buf;
	char tmp = 1;
	tmp <<= 5;
	tmp = tmp & 0x20; //0010 0000
	buddy_i2c_read(client,0xF4,&buf,1);
	buf |= tmp;
	buddy_i2c_write(client,0xF4,&buf,1);
}
void bmp180_Stop_Conv(struct i2c_client *client)
{
	char buf;
	char tmp = 1;
	tmp <<= 5;
	tmp = ~tmp; // 1101 1111
	buddy_i2c_read(client,0xF4,&buf,1);
	buf = buf & tmp;
	buddy_i2c_write(client,0xF4,&buf,1);
}
/* return 0 is convering and 1 is stop conver */
int bmp180_State_Conv(struct i2c_client *client)
{
	char buf;
	buddy_i2c_read(client,0xF4,&buf,1);
	if((buf & 0x20) != 0)
		return 0;
	else 
		return 1;
}
/*
 * Oss register - Set oversapmling ratio of the preessure measurement.
 * 0: single.
 * 1: 2 times.
 * 2: 4 times.
 * 3: 8 times. 
 */
void bmp180_set_oversample(struct i2c_client *client,int n)
{
	char buf;
	buddy_i2c_read(client,0xF4,&buf,1);
	buf &= 0x3F;
	if(n == 1)
		buf |= 0x40;
	else if(n == 2)
		buf |= 0x80;
	else if(n == 3)
		buf |= 0xC0;
	buddy_i2c_write(client,0xF4,&buf,1);
}
int bmp180_get_oversample(struct i2c_client *client)
{
	char buf;
	buddy_i2c_read(client,0xF4,&buf,1);
#if TEST
	printk(KERN_INFO "=>Oversapmle.[%08x]\n",buf);
#endif
	if(buf & 0xC0)
		return 8;
	else if(buf & 0x80)
		return 4;
	else if(buf &0x40)
		return 2;
	else
		return 0;

}
void bmp180_write_ctrl_meas(struct i2c_client *client,char buf)
{
	char register_addr = 0xF4;
	if(buddy_i2c_write(client,register_addr,&buf,1) == 1)
		printk(KERN_INFO "bad write\n");
#if TEST
	printk(KERN_INFO "write ctrl register buf is[%08x]\n",buf);
#endif
}
void bmp180_read_ctrl_meas(struct i2c_client *client,char *buf)
{
	char register_addr = 0xF4;
	if(buddy_i2c_read(client,register_addr,buf,1) == 1)
		printk(KERN_INFO "bad read\n");
#if TEST
	printk(KERN_INFO "=>Ctrl_meas.[%08x]\n",*buf);
#endif
}
/*
 * Soft reset - Write only register.If set to 0xB6,will perform the same
 * sequence as power on reset.
 */
void bmp180_reset(struct i2c_client *client)
{
	char buf = 0xB6;
	buddy_i2c_write(client,0xE0,&buf,1);
}
/*
 * Read out_xlsb register
 */
void bmp180_outxlsb(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xF8,buf,1);
#if TEST
	printk(KERN_INFO "=>Outxlsb.[%d]\n",*buf);
#endif
}
/*
 * Read out_lsb register
 */
void bmp180_outlsb(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xF7,buf,1);
#if TEST
	printk(KERN_INFO "=>Outlsb.[%d]\n",*buf);
#endif
}
/*
 * Read out_msb register
 */
void bmp180_outmsb(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0xF6,buf,1);
#if TEST
	printk(KERN_INFO "=>Outmsb.[%d]\n",*buf);
#endif
}
/*
 * Read out data
 */
void bmp180_outdata(struct i2c_client *client,int *buf)
{
	char buf_msb;
	char buf_lsb;
	bmp180_outmsb(client,&buf_msb);
	bmp180_outlsb(client,&buf_lsb);
	*buf = buf_msb;
	*buf <<= 8;
	*buf  = *buf & 0xF0;
	*buf |= buf_lsb;
}
/*
 * Measurement of pressure and temperature
 * The microcontroller sends a start sequence to start a pressure or 
 * temperture measurement.After converting time(wait 4.5 ms),the result 
 * value(UP or UT,respectively) can be read via the I2C interface.For 
 * calculating temperature in C and pressure in hPa,the calibration data 
 * has to be used.These constants can be read out from the BMP180 EEPROM 
 * via the I2C interface at software initialization.
 * The sampling rate can be increased up to 128 samples per second
 * (standard mode) for dynamic measurement.In this case,it is sufficient 
 * to measure the temperature only once per second and to use this value 
 * for all pressure measurement during the same period.
 */

/*
 * The BMP180 detail algorithm
 * 1. Start.
 * 2. Read calibration data from the EEPROM of the BMP180.Read out EEPROM 
 *    register,16 bit and MSB first.
 * 3. Read uncompensated temperature value.
 *    (1) Write 0x2E into regster 0xF4,wait 4.5ms
 *    (2) read register 0xF6(MSB),0xF7(LSB).
 *    (3) UT = MSB<<8 + LSB.
 * 4. Read uncompensated pressure valure.
 *    (1) Write 0x34 + (oss << 6) into regiter 0xF4,wait.
 *    (2) Read register 0xF6(MSB),0xF7(LSB),0xF8(XLSB).
 *    (3) UP = (MSB<<16 + LSB<<8 ) >>(8-oss).
 * 5. Calculate true temperature
 *    (1) X1 = (UT - AC6) * AC5 / 2^15.
 *	  (2) X2 = MC * 2^11 / (X1 + MD).
 *    (3) B5 = X1 + X2.
 *    (4) T  = (B5 + 8) / 2^4.
 * 6. Calculate true pressure
 *    (1) B6 = B5 - 4000.
 *    (2) X1 = (B2 *(B6 * B6 / 2^12)) / 2^11.
 *    (3) X2 = AC2 * B6 / 2^11.
 *    (4) X3 = X1 + X2.
 *    (5) B3 = (((AC1 * 4 + X3) << oss) + 2) / 4.
 *    (6) X1 = AC3 * B6 / 2^13.
 *    (7) X2 = (B1 * (B6 * B6 / 2^12)) / 2^16.
 *    (8) X3 = ((X1 + X2) + 2) / 2^2.
 *    (9) B4 = AC4 * (unsigned long)(X3 + 32768) / 2^15.
 *    (10) B7 = ((unsigned long)UP - B3) * (50000 >> oss)
 *    (11) if(B7 < 0x80000000) 
 *            p = (B7 * 2) / B4.
 *         else
 *            p = (B7 / B4) * 2.
 *    (12) X1 = (p / 2^8) * (p / 2^8).
 *    (13) X1 = (X1 * 3038) / 2^16.
 *    (14) X2 = (-7357 * p) / 2^15.
 *    (15) p = p + (X1 + X2 + 3791) / 2^4.
 * 7. Display temperature and pressure valure.
 * 8. loop
 */
static int bmp180_calculate_temp_and_pressure(struct i2c_client *client,
		long *temp,long *pressure)
{
	short data_ac1 = 0;
	short data_ac2 = 0;
	short data_ac3 = 0;
	unsigned short data_ac4 = 0;
	unsigned short data_ac5 = 0;
	unsigned short data_ac6 = 0;
	short data_b1 = 0;
	short data_b2 = 0;
	short data_mb = 0;
	short data_mc = 0;
	short data_md = 0;
	int  data_outlsb = 0;
	int  data_outmsb = 0;
	int  data_outxlsb = 0;
	long UT = 0;
	long UP = 0;
	short oss = 0;
	long p;
	long X1,X2,B5,T;
	long B4,B6,B7,B3,X3;
	char bu;
	/*
	 * Read calibration data from the EEPROM of the BMP180.
	 */
	bmp180_AC1(client,&data_ac1);
	bmp180_AC2(client,&data_ac2);
	bmp180_AC3(client,&data_ac3);
	bmp180_AC4(client,&data_ac4);
	bmp180_AC5(client,&data_ac5);
	bmp180_AC6(client,&data_ac6);
	bmp180_B1(client,&data_b1);
	bmp180_B2(client,&data_b2);
	bmp180_MB(client,&data_mb);
	bmp180_MC(client,&data_mc);
	bmp180_MD(client,&data_md);
	/*
	 * Read uncompensated temperature value.
	 */
	bmp180_write_ctrl_meas(client,0x2E);
#if TEST
	bmp180_read_ctrl_meas(client,&bu);
#endif
	mdelay(5);
	bmp180_outmsb(client,(char *)&data_outmsb);
	bmp180_outlsb(client,(char *)&data_outlsb);
	UT = (data_outmsb<<8) + data_outlsb;
#if TEST
	printk(KERN_INFO "=>UT.[%ld]\n",UT);
#endif
	/*
	 * Read uncompensated pressure value.
	 */
	oss = oss<<6;
	bmp180_write_ctrl_meas(client,0x34 + oss);
#if TEST
	bu &= 0;
	bmp180_read_ctrl_meas(client,&bu);
#endif
	mdelay(5);
	bmp180_outmsb(client,(char *)&data_outmsb);
	bmp180_outlsb(client,(char *)&data_outlsb);
	bmp180_outxlsb(client,(char *)&data_outxlsb);
	UP = ((data_outmsb<<16) + (data_outlsb<<8) + data_outxlsb) >>(8 - oss);
#if TEST
	printk(KERN_INFO "=>UP.[%ld]\n",UP);
#endif
	/*
	 * Calculate true tempture
	 */
	X1 = (UT - data_ac6) * data_ac5 / (1<<15);
	X2 = data_mc * (1<<11) / (X1 + data_md);
	B5 = X1 + X2;
	T  = (B5 + 8) / (1<<4);
	/*
	 * calculate true pressure
	 */
	B6 = B5 - 4000;
	X1 = (data_b2 *(B6 * B6 / (1<<12) )) / (1<<11);
	X2 = data_ac2 * B6 / (1<<11);
	X3 = X1 + X2;
	B3 = (((data_ac1 * 4 + X3)<<oss) +2) / 4;
	X1 = data_ac3 * B6 / (1<<13);
	X2 = (data_b1 * (B6 * B6 / (1<<12))) / (1<<16);
	X3 = ((X1 + X2) + 2) / 4;
	B4 = data_ac4 * (unsigned long)(X3 + 32768) / (1<<15);
	B7 = ((unsigned long)UP - B3) * (50000>>oss);
	if(B7 < 0x80000000)
		p = (B7 * 2) / B4;
	else
		p = (B7 / B4) * 2;
	X1 = (p / (1<<8)) * (p / (1<<8));
	X1 = (X1 * 3038) / (1<<16);
	X2 = (-7357 * p) / (1<<16);
	p = p + (X1 + X2 + 3791) / (1<<4);
	/*
	 * store data
	 */
	*temp = T;
	*pressure = p;
	return 0;
}
/*
 * Read A/D conversion result or EEPROM data
 * To read out the temperature data word UT(16 bit),the pressure data 
 * word UP(16 to 19 bit) and the EEPROM data proceed as follow:
 * 1. After the start condition the master sends the module address write
 *    command and register address.The register address selects the 
 *    read register:
 *    EEPROM data register 0xAA to 0xBF.
 *    Temperature or pressure value UT or UP 0xF6(MSB),0xF7(LSB),
 *    optionally 0xF8(XLSB)
 * 2. Then the master sends a restart condition followed by the module
 *    address read that will be acknowledged the BMP180(ACKS).The BMP180
 *    sends first the 8 MSB,acknowledged by the master(ACKM),then the 8
 *    LSB.The master sends a "not acknowledge"(NACKM) and finally a stop
 *    condition.
 * 3. Optionally for ultra high resolution,the XLSB register with address
 *    0xF8 can be read to extend the 16 bit word to up to 19 bits;refer to
 *    the application programming interface(API) software.
 */
/*
 * User read operation
 */
static ssize_t bmp180_read(struct file *filp,char __user *buf,size_t count,loff_t *offset)
{
	int ret;
	char *tmp;
	int addr = 0xD0;
	char rbuf;
	int counter = 0;
	long temp;
	long pressure;
	char bu;

	struct i2c_client *client = filp->private_data;
#if DEBUG
	printk(KERN_INFO ">>>>>bmp180 read<<<<<\n");
#endif

	tmp = kmalloc(count,GFP_KERNEL);
	if(tmp == NULL)
	{
		printk(KERN_INFO "Unable to get memory from system\n");
		return -ENOMEM;
	}
#if DEBUG
	printk(KERN_INFO "Succeed to get memory\n");
#endif
#if DEBUG
	bmp180_debug(client,"read");
#endif
	/*
	 * read data
	 */
	ret = buddy_i2c_read(client,addr,&rbuf,1);
	if(ret)
		printk(KERN_INFO "Fail to read ID\n");
	else
#if DEBUG
		printk(KERN_INFO "The ID is %08x\n",rbuf);
#endif
//	while(1)
//	{
//		printk(KERN_INFO "================[%d]================\n",counter);
		bmp180_calculate_temp_and_pressure(client,&temp,&pressure);
#if DEBUG
		printk(KERN_INFO "The BMP180's temp is:[%ld]\n",temp);
		printk(KERN_INFO "The BMP180's pressure is:[%ld]\n",pressure);
#endif
		
//		mdelay(1000);
//		counter++;
//	}
	/*
	 * push data to Userspace
	 */	
	ret = copy_to_user(buf,&temp,count);
	if(ret != 0)
	{
		printk(KERN_INFO "loss the %d bytes\n",(int)(count - ret));
		goto out_free;
	} else
#if DEBUG
		printk(KERN_INFO "Succeed to push %d bytes to Userspace\n",count);
#endif
	kfree(tmp);
	return 0;
out_free:
	kfree(tmp);
	return ret;
}
/*
 * open operation
 */
static int bmp180_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>bmp180 open<<<<<\n");
	filp->private_data = my_client;
	return 0;
}
/*
 * release operation
 */
static int bmp180_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>bmp180 release<<<<<\n");
	filp->private_data = NULL;
	return 0;
}
/*
 * Software pressure sampling accuracy modes
 * For applications where a low noise level is critical,averaging is 
 * recommended if the lower bandwidth is acceptable.Oversampling can be 
 * enable using the software API driver(with OSR = 3).
 * BMP180 software accuracy mode,selected by driver software via the 
 * variable software_oversampling_setting.
 */
/*
 * Calibration coefficients
 * The 176 bit EEPROM is partitioned in 11 words of 16 bit each.These 
 * contain 11 calibration coefficients.Every sensor module has individual 
 * coefficients. Before the first calculation of temperature and pressure,
 * the master reads out the EEPROM data.
 * The data communication can be checkd by checking that none of the words
 * has the value 0 or 0XFFFF.
 */
/*
 * Calculation pressure and temperature
 * The mode(ultra low power,standard,high,ultra high resolution) can be 
 * selected by the variable oversampling_setting(0,1,2,3)in the C code.
 * Calulation of true temperture and pressure in steps of 1Pa(= 0.01hPa = 
 * 0.01mber) and temperature in steps of 0.1 C.
 */
/*
 * bmp180 filp operation
 */
static const struct file_operations bmp180_fops = {
	.owner    = THIS_MODULE,
	.read     = bmp180_read,
	.open     = bmp180_open,
	.release  = bmp180_release,
};

/*
 * device id table
 */
static struct i2c_device_id bmp180_id[] = {
	{"bmp180",0x77},
	{},
};
/*
 * device probe
 */
static int bmp180_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int res;
	struct device *dev;
	my_client = client;
	printk(KERN_INFO ">>>>>bmp180 probe<<<<<\n");
	dev = device_create(i2c_class,&my_client->dev,MKDEV(i2c_major,0),NULL,DEV_NAME);
	if(IS_ERR(dev))
	{
		res = PTR_ERR(dev);
		goto error;
	}
#if DEBUG
	bmp180_debug(client,"probe");
#endif
	return 0;
error:
	return res;
}
/*
 * device remove
 */
static int bmp180_remove(struct i2c_client *client)
{
	printk(KERN_INFO ">>>>>bmp180 remove<<<<<\n");
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver
 */
static struct i2c_driver bmp180_driver = {
	.driver = {
		.name = "bmp180",
	},
	.probe    = bmp180_probe,
	.id_table = bmp180_id,
	.remove   = bmp180_remove,
};
/*
 * init module
 */
static __init int buddy_init(void)
{
	int ret;
	/*
	 * add i2c-board information
	 */
	struct i2c_adapter *adap;
	struct i2c_board_info i2c_info;

	adap = i2c_get_adapter(0);
	if(adap == NULL)
	{
		printk(KERN_INFO "Unable to get i2c adapter\n");
		ret = -EFAULT;
		goto out;
	}
	printk(KERN_INFO "Succeed to get i2c adapter[%s]\n",adap->name);
	
	memset(&i2c_info,0,sizeof(struct i2c_board_info));
	strlcpy(i2c_info.type,"bmp180",I2C_NAME_SIZE);
	i2c_info.addr = 0x77;

	my_client = i2c_new_device(adap,&i2c_info);
#if DEBUG
	bmp180_debug(my_client,"init");
#endif
	if(my_client == NULL)
	{
		printk(KERN_INFO "Unable to get new i2c device\n");
		ret = -ENODEV;
		i2c_put_adapter(0);
		goto out;
	}
	i2c_put_adapter(adap);

	printk(KERN_INFO ">>>>>module init<<<<<<\n");
	/*
	 * Register char device
	 */
	i2c_major = register_chrdev(0,DEV_NAME,&bmp180_fops);
	if(i2c_major == 0)
	{
		printk(KERN_INFO "Unable to register driver into char-driver\n");
		ret = -EFAULT;
		goto out;
	}	
	printk(KERN_INFO "Succeed to register driver,char-driver is %d\n",i2c_major);
	/*
	 * create device class
	 */
	i2c_class = class_create(THIS_MODULE,DEV_NAME);
	if(IS_ERR(i2c_class))
	{
		printk(KERN_INFO "Unable to create class file\n");
		ret = PTR_ERR(i2c_class);
		goto out_unregister;
	}
	printk(KERN_INFO "Succeed to create class file\n");
	/*
	 * create i2c driver
	 */
	ret = i2c_add_driver(&bmp180_driver);
	if(ret)
	{
		printk(KERN_INFO "Unable to add i2c driver\n");
		ret = -EFAULT;
		goto out_class;
	}
	printk(KERN_INFO "Succeed to add i2c driver\n");
	return 0;
out_class:
	class_destroy(i2c_class);
out_unregister:
	unregister_chrdev(i2c_major,"bmp180");
out:
	return ret;

}
/*
 * exit module
 */
static __exit void buddy_exit(void)
{
	printk(KERN_INFO ">>>>>BMP180 exit<<<<<\n");
	i2c_del_driver(&bmp180_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,"bmp180");
}



MODULE_AUTHOR("Buddy <514981221@qq.com>");
MODULE_DESCRIPTION("BMP180 driver");
MODULE_LICENSE("GPL");

module_init(buddy_init);
module_exit(buddy_exit);
