/*
 * Copyright@2014 Buddy Zhang
 */
/*
 * The LSM303DLHC is a system-in-package featuring a 3D digital linear acc-
 * eleration sensor and a 3D digital magnetic sensor.The system inclued sp-
 * ecific sensing elements and an IC interface capable of measuring both t-
 * he linear acceleration and magnetic field applied on it and to provide a
 * signal to the external world throght an I2C serial interface with separ-
 * ated digital output.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>

#define Acceleration       0
#define Magnetic            1
#define DEV_NAME "lsm303"
#if Acceleration
	#define I2C_ADDRESS    0x19
#else
	#define I2C_ADDRESS    0x1E
#endif
#define DEBUG          1

static struct class *i2c_class;
static struct i2c_client *my_client;
static int i2c_major;

#if DEBUG
static void lsm303_debug(struct i2c_client *client,char *name)
{
	printk(KERN_INFO "=====================%s=====================\n=>client->name:%s\n=>client->addr:%08x\n=>adapter->name:%s\n",
			name,client->name,client->addr,client->adapter->name);
}
#endif
/*
 * i2c_read
 */
static int buddy_i2c_read(struct i2c_client *client,char address,
		char *buf,int len)
{
	int ret;
	struct i2c_msg msg[2];
	/* dummp write */
	msg[0].addr    = client->addr;
	msg[0].flags   = client->flags | I2C_M_TEN;
	msg[0].buf     = &address;
	msg[0].len     = 1;
	/* read initial */
	msg[1].addr    = client->addr;
	msg[1].flags   = client->flags | I2C_M_TEN;
	msg[1].flags  |= I2C_M_RD;
	msg[1].buf     = buf;
	msg[1].len     = len;

	ret = i2c_transfer(client->adapter,msg,2);
	if(ret != 2)
	{
		printk(KERN_INFO "Unable read data from i2c bus\n");
		return -1;
	}else
	{
		//printk(KERN_INFO "Succeed to read data from i2c bus\n");
		return 0;
	}
}
/*
 * i2c write
 */
static int buddy_i2c_write(struct i2c_client *client,char address,
		char *buf,int len)
{	
	int ret;
	char tmp[len+1];
	struct i2c_msg msg;
	tmp[0] = address;
	
	for(ret = 1;ret <= len;ret++)
	{
		tmp[ret] = buf[ret - 1];
	}
	/* write initial */
	msg.addr    = client->addr;
	msg.flags   = client->flags | I2C_M_TEN;
	msg.buf     = tmp;
	msg.len     = len+1;

	ret = 0;
	ret = i2c_transfer(client->adapter,&msg,1);
	if(ret != 1)
	{
		printk(KERN_INFO "Unable to write data to i2c bus\n");
		return -1;
	}else
	{
		//printk(KERN_INFO "Succeed to write data to i2c bus\n");
		return 0;
	}
}
/*
 * open device
 */
static int lsm303_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>lsm303 open<<<<<\n");
	filp->private_data = my_client;
	return 0;
}
/*
 * =========================Register Operation=========================
 */
/*
 * CTRL_REG1_A
 */
static void LSM303_Read_CTRL_REG1_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x20,buf,1);
}
static void LSM303_Write_CTRL_REG1_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x20,buf,1);
}
/*
 * CTRL_REG2_A
 */
static void LSM303_Read_CTRL_REG2_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x21,buf,1);
}
static void LSM303_Write_CTRL_REG2_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x21,buf,1);
}
/*
 * CTRL_REG3_A
 */
static void LSM303_Read_CTRL_REG3_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x22,buf,1);
}
static void LSM303_Write_CTRL_REG3_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x22,buf,1);
}
/*
 * CTRL_REG4_A
 */
static void LSM303_Read_CTRL_REG4_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x23,buf,1);
}
static void LSM303_Write_CTRL_REG4_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x23,buf,1);
}
/*
 * CTRL_REG5_A
 */
static void LSM303_Read_CTRL_REG5_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x24,buf,1);
}
static void LSM303_Write_CTRL_REG5_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x24,buf,1);
}
/*
 * CTRL_REG6_A
 */
static void LSM303_Read_CTRL_REG6_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x25,buf,1);
}
static void LSM303_Write_CTRL_REG6_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x25,buf,1);
}
/*
 * REFERENCE_A
 */
static void LSM303_Read_REFERENCE_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x26,buf,1);
}
static void LSM303_Write_REFERENCE_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x26,buf,1);
}
/*
 * STATUS_REG_A
 */
static void LSM303_Read_STATUS_REG_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x27,buf,1);
}
/*
 * OUT X-LSB A
 */
static void LSM303_Read_OUT_X_L_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x28,buf,1);
}
/*
 * OUT X-MSB A
 */
static void LSM303_Read_OUT_X_H_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x29,buf,1);
}
/*
 * OUT Y-LSB A
 */
static void LSM303_Read_OUT_Y_L_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x2A,buf,1);
}
/*
 * OUT Y-MSB A
 */
static void LSM303_Read_OUT_Y_H_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x2B,buf,1);
}
/*
 * OUT Z-MSB A
 */
static void LSM303_Read_OUT_Z_L_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x2C,buf,1);
}
/*
 * OUT Z-MSB A
 */
static void LSM303_Read_OUT_Z_H_A(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x2D,buf,1);
}
/*
 * FIFO_CTRL_REG_A
 */
static void LSM303_Read_FIFO_CTRL_REG_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x2E,buf,1);
}
static void LSM303_Write_FIFO_CTRL_REG_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x2E,buf,1);
}
/*
 * FIFO_SRC_REG_A
 */
static void LSM303_Read_FIFO_SRC_REG_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x2F,buf,1);
}
/*
 * INIT1_CFG_A 
 */
static void LSM303_Read_INIT1_CFG_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x30,buf,1);
}
static void LSM303_Write_INIT1_CFG_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x30,buf,1);
}
/*
 * INIT1_SOURCE_A
 */
static void LSM303_Read_INIT1_SOURCE_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x31,buf,1);
}
/*
 * INIT1_THS_A 
 */
static void LSM303_Read_INIT1_THS_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x32,buf,1);
}
static void LSM303_Write_INIT1_THS_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x32,buf,1);
}
/*
 * INIT1_DURATION_A
 */
static void LSM303_Read_INIT1_DURATION_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x33,buf,1);
}
static void LSM303_Write_INIT1_DURATION_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x33,buf,1);
}
/*
 * INIT2CFG_A 
 */
static void LSM303_Read_INIT2_CFG_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x34,buf,1);
}
static void LSM303_Write_INIT2_CFG_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x34,buf,1);
}
/*
 * INIT2_SOURCE_A
 */
static void LSM303_Read_INIT2_SOURCE_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x35,buf,1);
}
/*
 * INIT2_THS_A 
 */
static void LSM303_Read_INIT2_THS_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x36,buf,1);
}
static void LSM303_Write_INIT2_THS_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x36,buf,1);
}
/*
 * INIT2_DURATION_A
 */
static void LSM303_Read_INIT2_DURATION_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x37,buf,1);
}
static void LSM303_Write_INIT2_DURATION_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x37,buf,1);
}
/*
 * CLICK_CFG_A 
 */
static void LSM303_Read_CLICK_CFG_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x38,buf,1);
}
static void LSM303_Write_CLICK_CFG_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x38,buf,1);
}
/*
 * CLICK_SRC_A
 */
static void LSM303_Read_CLICK_SRC_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x39,buf,1);
}
static void LSM303_Write_CLICK_SRC_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x39,buf,1);
}
/*
 * CLICK_THS_A
 */
static void LSM303_Read_CLICK_THS_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x3A,buf,1);
}
static void LSM303_Write_CLICK_THS_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x3A,buf,1);
}
/*
 * TIME_LIMIT_A
 */
static void LSM303_Read_TIME_LIMIT_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x3B,buf,1);
}
static void LSM303_Write_TIME_LIMIT_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x3B,buf,1);
}
/*
 * TIME_LATENCY_A
 */
static void LSM303_Read_TIME_LATENCY_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x3C,buf,1);
}
static void LSM303_Write_TIME_LATENCY_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x3C,buf,1);
}
/*
 * TIME_WINDOW_A
 */
static void LSM303_Read_TIME_WINDOW_A(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x3D,buf,1);
}
static void LSM303_Write_TIME_WINDOW_A(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x3D,buf,1);
}
/*
 * CRA_REG_M
 */
static void LSM303_Read_CRA_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x00,buf,1);
}
static void LSM303_Write_CRA_REG_M(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x00,buf,1);
}
/*
 * CRB_REG_M
 */
static void LSM303_Read_CRB_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x01,buf,1);
}
static void LSM303_Write_CRB_REG_M(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x01,buf,1);
}
/*
 * MR_REG_M
 */
static void LSM303_Read_MR_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x02,buf,1);
}
static void LSM303_Write_MR_REG_M(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x02,buf,1);
}
/*
 * OUT X-LSB M
 */
static void LSM303_Read_OUT_X_L_M(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x04,buf,1);
}
/*
 * OUT X-MSB M
 */
static void LSM303_Read_OUT_X_H_M(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x03,buf,1);
}
/*
 * OUT Y-LSB M
 */
static void LSM303_Read_OUT_Y_L_M(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x08,buf,1);
}
/*
 * OUT Y-MSB M
 */
static void LSM303_Read_OUT_Y_H_M(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x07,buf,1);
}
/*
 * OUT Z-MSB A
 */
static void LSM303_Read_OUT_Z_L_M(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x06,buf,1);
}
/*
 * OUT Z-MSB M
 */
static void LSM303_Read_OUT_Z_H_M(struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x05,buf,1);
}
/*
 * SR_REG_Mg 
 */
static void LSM303_Read_SR_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x09,buf,1);
}
/*
 * IRA_REG_M 
 */
static void LSM303_Read_IRA_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x0A,buf,1);
}
/*
 * IRB_REG_M 
 */
static void LSM303_Read_IRB_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x0B,buf,1);
}
/*
 * IRC_REG_M 
 */
static void LSM303_Read_IRC_REG_M(struct i2c_client *client ,
		char *buf)
{
	buddy_i2c_read(client,0x0C,buf,1);
}
/*
 * ======================Acceleration==========================
 */
/*
 * Linear acceleration operation mode
 * LSM303 provides two different acceleration operation modes,respecitively
 * reported as "normal mode" and "low-power mode".While normal mode guara-
 * ntees high resolution,low-power mode reduces further the current consu-
 * mption.
 */
static void LSM303_AC_Set_Mode(struct i2c_client *client,int num)
{
	char buf1,buf2;
	LSM303_Read_CTRL_REG1_A(client,&buf1);
	LSM303_Read_CTRL_REG4_A(client,&buf2);
	if(num == 0)
	{
	/* Low-power mode */
		buf1 = buf1 | 0x08;
		buf2 = buf2 & 0xF7;
	} else if(num == 1)
	{
	/* normal mode */
		buf1 = buf1 & 0xF7;
		buf2 = buf2 | 0x08;
	}
	LSM303_Write_CTRL_REG1_A(client,&buf1);
	LSM303_Write_CTRL_REG4_A(client,&buf2);

}
static int LSM303_AC_Get_Mode(struct i2c_client *client)
{
	char buf1,buf2;
	LSM303_Read_CTRL_REG1_A(client,&buf1);
	LSM303_Read_CTRL_REG4_A(client,&buf2);


	buf1 = buf1 & 0x08;
	buf2 = buf2 & 0x08;
	if((buf1 == 0x08) && (buf2 == 0x00))
	{
		/* low power */
		printk(KERN_INFO "Low Power\n");
		return 0;
	} else if((buf1 == 0x00) && (buf2 == 0x08))
	{
		/* normal mode */
		printk(KERN_INFO "Normal mode\n");
		return 1;
	}
	printk(KERN_INFO "Get Mode fail\n");
	return -1;
}
/*
 * Power mode selection
 */
static void LSM303_AC_Set_Power_Mode(struct i2c_client *client,
		int num)
{
	char buf;
	if(num < 0 || num > 10)
	{
		printk(KERN_INFO "Over the Power mode select\n");
		return;
	}
	LSM303_Read_CTRL_REG1_A(client,&buf);
	num = num & 0x0F;
	num = num<<4;
	buf = buf & 0x0F;
	buf = buf | num;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
/*
  * Get Power mode 
  */
static void LSM303_AC_Get_Power_Mode(struct i2c_client *client)
{
	char buf;
	char name[20];
	int ret = LSM303_AC_Get_Mode(client);
	if(ret == 0)
		{
		strncpy(name,"Low-power mode",14);
		name[14] = '\0';
		}
	else
		{
		strncpy(name,"Normal mode",11);
		name[11] = '\0';
		}
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf & 0xF0;
	switch(buf)
	{
		case 0x00:
			printk(KERN_INFO "Power-down\n");
			break;
		case 0x10:
			printk(KERN_INFO "%s (1Hz)\n",name);
			break;
		case 0x20:
			printk(KERN_INFO "%s (10Hz)\n",name);
			break;
		case 0x30:
			printk(KERN_INFO "%s (25Hz)\n",name);
			break;
		case 0x40:
			printk(KERN_INFO "%s (50Hz)\n",name);
			break;
		case 0x50:
			printk(KERN_INFO "%s (100Hz)\n",name);
			break;
		case 0x60:
			printk(KERN_INFO "%s (200Hz)\n",name);
			break;
		case 0x70:
			printk(KERN_INFO "%s (400Hz)\n",name);
			break;
		case 0x80:
			printk(KERN_INFO "Low-power mode(1.620KHz)\n");
			break;
		case 0x90:
			if(ret == 0)
				printk(KERN_INFO "Low-power mode (5.376KHz)\n");
			else
				printk(KERN_INFO "Normal mode (1.344KHz)\n");
			break;
		case 0xA0:
			printk(KERN_INFO "%s (Hz)\n",name);
			break;

	}
}
/*
 * Power down 
 */
static void LSM303_AC_Power_down(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf & 0x0F;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
/*
 * Enable X,Y,Z axis
 */
static void LSM303_AC_Enable_X(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf | 0x01;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
static void LSM303_AC_Disable_X(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf & 0xFE;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
static void LSM303_AC_Enable_Y(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf | 0x02;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
static void LSM303_AC_Disable_Y(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf & 0xFD;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
static void LSM303_AC_Enable_Z(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf | 0x03;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
static void LSM303_AC_Disable_Z(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	buf = buf & 0xFB;
	LSM303_Write_CTRL_REG1_A(client,&buf);
}
static void LSM303_AC_Get_Enable_State(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CTRL_REG1_A(client,&buf);
	if((buf & 0x01) != 0)
		printk(KERN_INFO "The X axis is Enable\n");
	if((buf & 0x02) != 0)
		printk(KERN_INFO "The Y axis is Enable\n");
	if((buf & 0x04) != 0)
		printk(KERN_INFO "The Z axis is Enable\n");
}
static void LSM303_AC_Enable_XYZ(struct i2c_client *client)
{
#if DEBUG
	char buf;
#endif
	/*
	 * Enable X,Y and Z
	 */
	LSM303_AC_Enable_X(client);
	LSM303_AC_Enable_Y(client);
	LSM303_AC_Enable_Z(client);
#if DEBUG
	LSM303_Read_CTRL_REG1_A(client,&buf);
	printk(KERN_INFO "XYZ state is %08x\n",buf);
#endif
}
static void LSM303_AC_Disable_XYZ(struct i2c_client *client)
{
	/*
	 * Disable X,Y and Z
	 */
	LSM303_AC_Disable_X(client);
	LSM303_AC_Disable_Y(client);
	LSM303_AC_Disable_Z(client);
}
static void LSM303_AC_High_Pass_Filter_Mode(struct i2c_client *client,
		int num)
{
	/*
	 * 0 : Normal mode.(reset reading HP_RESET_FILTER)
	 * 1 : Reference signal for filtering.
	 * 2 : Normal mode.
	 * 3 : Autoreset on interrupt event.
	 */
	char buf;
	if(num > 4 || num < 0)
	{
		printk(KERN_INFO "Over the High pass filter mode rang\n");
		return;
	}
	LSM303_Read_CTRL_REG2_A(client,&buf);
	buf = buf & 0x3F;
	num = num & 0x0C;
	num = num<<6;
	buf = buf | num;
	LSM303_Write_CTRL_REG2_A(client,&buf);
}
/*
 * Status 
 */
static int LSM303_AC_ZOR(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x40;
	if(!buf)
		/* Z axis data overrun */
		return 1;
	else 
		/* a new data for Z has overwritten the previous one */
		return 0;
}
static int LSM303_AC_YOR(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x20;
	if(!buf)
		/* Y axis data overrun */
		return 1;
	else 
		/* a new data for Y has overwritten the previous one */
		return 0;
}
static int LSM303_AC_XOR(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x10;
	if(!buf)
		/* X axis data overrun */
		return 1;
	else 
		/* a new data for X has overwritten the previous one */
		return 0;
}
static int LSM303_AC_ZYXDA(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x08;
	if(!buf)
		/* a new set of data is not yet available */
		return 1;
	else 
		/* a new set of data is available */
		return 0;
}
static int LSM303_AC_ZDA(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x04;
	if(!buf)
		/* a new data for the Z is not yet available */
		return 1;
	else 
		/* a new data for the Z is available */
		return 0;
}
static int LSM303_AC_YDA(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x02;
	if(!buf)
		/* a new data for the Y is not yet available */
		return 1;
	else 
		/* a new data for the Y is available */
		return 0;
}
static int LSM303_AC_XDA(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	buf = buf & 0x01;
	if(!buf)
		/* a new data for the X is not yet available */
		return 1;
	else 
		/* a new data for the X is available */
		return 0;
}
/*
 *  Data Status 
 */
 static int LSM303_Get_Data_State(struct i2c_client *client)
{
	char buf;
	LSM303_Read_STATUS_REG_A(client,&buf);
	if((buf & 0x08) != 0)
	{
		if((buf & 0x01) != 0)
			printk(KERN_INFO "The  X axis has new data\n");
		else
			printk(KERN_INFO "The X axis hasn't new data\n");
		if((buf & 0x02) != 0)
			printk(KERN_INFO "The  Y axis has new data\n");
		else
			printk(KERN_INFO "The Y axis hasn't new data\n");
		if((buf & 0x04) != 0)
			printk(KERN_INFO "The  Z axis has new data\n");
		else
			printk(KERN_INFO "The Z axis hasn't new data\n");
		return 0;
	}else
		printk(KERN_INFO "No has new data\n");

	if((buf & 0x80) != 0)
	{
		if((buf & 0x10) != 0)
			printk(KERN_INFO "The  X axis has overwrite\n");
		else
			printk(KERN_INFO "The X axis hasn't overwrite\n");
		if((buf & 0x20) != 0)
			printk(KERN_INFO "The  Y axis has overwrite\n");
		else
			printk(KERN_INFO "The Y axis hasn't overwrite\n");
		if((buf & 0x40) != 0)
			printk(KERN_INFO "The  Z axis has overwrite\n");
		else
			printk(KERN_INFO "The Z axis hasn't overwrite\n");
		return 1;
	}else
		printk(KERN_INFO "No has data overwrite\n");
	return -1;
}
/*
 * Get data
 */
static void LSM303_AC_Get_X(struct i2c_client *client,short *data)
{
	/* Get X data */
	char buf;
	LSM303_Read_OUT_X_H_A(client,&buf);
	*data = 0;
	*data = *data | buf;
	*data = *data<<8;
	LSM303_Read_OUT_X_L_A(client,&buf);
	*data = *data | buf;
}
static void LSM303_AC_Get_Y(struct i2c_client *client,short *data)
{
	/* Get Y data */
	char buf;
	LSM303_Read_OUT_Y_H_A(client,&buf);
	*data = 0;
	*data = *data | buf;
	*data = *data<<8;
	LSM303_Read_OUT_Y_L_A(client,&buf);
	*data = *data | buf;
}
static void LSM303_AC_Get_Z(struct i2c_client *client,short *data)
{
	/* Get Z data */
	char buf;
	LSM303_Read_OUT_Z_H_A(client,&buf);
	*data = 0;
	*data = *data | buf;
	*data = *data<<8;
	LSM303_Read_OUT_Z_L_A(client,&buf);
	*data = *data | buf;
}
/*
 * Read Register
 */
 static void LSM303_AC_Read_Register(struct i2c_client *client)
{
	char reg[14];
	int i;
	char addr = 0x20;
	for(i = 0;i<14;i++)
		buddy_i2c_read(client,addr+i,reg+i,1);
	for(i = 0;i<14;i++)
	{
		printk(KERN_INFO "Reg[%d]=>%02x\n",i,*(reg + i));
	}
}
/*
 * Use Acceleration sensor
 */
static void LSM303_Use_AC(struct i2c_client *client,short *a)
{

	/* 1. Set AC mode */
	LSM303_AC_Set_Mode(client,1);
#if DEBUG
	LSM303_AC_Get_Mode(client);
#endif
	/* 2. Set power mode */
	LSM303_AC_Set_Power_Mode(client,6);
#if DEBUG
	LSM303_AC_Get_Power_Mode(client);
#endif
	/* 3. Enable XYZ */
	LSM303_AC_Enable_XYZ(client);
#if DEBUG
	LSM303_AC_Get_Enable_State(client);
#endif
	/* 4. Wait little time */
	while((LSM303_Get_Data_State(client) == -1))
	{
		mdelay(1000);
		printk(KERN_INFO "Wait\n");
		//LSM303_AC_Read_Register(client);
	}
	/* 5. Get data from X,Y and Z */
	LSM303_AC_Get_X(client,a);printk(KERN_INFO "X:%d\n",*a);
	LSM303_AC_Get_Y(client,a+1);printk(KERN_INFO "Y:%d\n",*(a+1));
	LSM303_AC_Get_Z(client,a+2);printk(KERN_INFO "Z:%d\n",*(a+2));
}
/*
  * Test Mode
 */
 static void LSM303_Test(struct i2c_client *client)
{
	short a[3] = {0,0,0};
	LSM303_Use_AC(client,a);
	printk(KERN_INFO "The X is %d\n",*a);
	printk(KERN_INFO "The Y is %d\n",*(a+1));
	printk(KERN_INFO "The Z is %d\n",*(a+2));
}

/*
 * release device
 */
static int lsm303_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>lsm303 release<<<<<\n");
	filp->private_data = NULL;
	return 0;
}
/*
 * read device
 */
static ssize_t lsm303_read(struct file *filp,char __user *buf,
		size_t count,loff_t *offset)
{
	short a[3] = {0,0,0};
	char tmp1,tmp2;
	struct i2c_client *client = filp->private_data;
#if DEBUG
	lsm303_debug(client,"read");
	LSM303_Test(client);
#endif
       
	return 0;
}
/*
 * file operations
 */
static struct file_operations lsm303_fops = {
	.owner     = THIS_MODULE,
	.open      = lsm303_open,
	.release   = lsm303_release,
	.read      = lsm303_read,
};
/*
 * device id table
 */
static struct i2c_device_id lsm303_id[] = {
	{DEV_NAME,I2C_ADDRESS},
	{},
};
/*
 * device probe
 */
static int lsm303_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int res;
	struct device *dev;
	my_client = client;
	printk(KERN_INFO ">>>>>>lsm303 probe<<<<<<\n");
	dev = device_create(i2c_class,&my_client->dev,MKDEV(i2c_major,0),
			NULL,DEV_NAME);
	if(IS_ERR(dev))
	{
		res = PTR_ERR(dev);
		goto error;
	}
#if DEBUG
	lsm303_debug(client,"probe");
#endif
	return 0;
error:
	return res;
}
/*
 * device remove
 */
static int lsm303_remove(struct i2c_client *client)
{
	printk(KERN_INFO ">>>>lsm303 remove<<<<\n");
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver
 */
static struct i2c_driver lsm303_driver = {
	.driver   = {
		.name = DEV_NAME,
	},
	.probe    = lsm303_probe,
	.id_table = lsm303_id,
	.remove   = lsm303_remove,
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
	strlcpy(i2c_info.type,DEV_NAME,I2C_NAME_SIZE);
	i2c_info.addr = I2C_ADDRESS;

	my_client = i2c_new_device(adap,&i2c_info);
#if DEBUG
	lsm303_debug(my_client,"init");
#endif
	if(my_client == NULL)
	{
		printk(KERN_INFO "Unable to get new i2c device\n");
		ret = -ENODEV;
		i2c_put_adapter(0);
		goto out;
	}
	i2c_put_adapter(adap);
	printk(KERN_INFO ">>>>>>>>module init<<<<<<<<<<<<\n");
	/*
	 * Register char device
	 */
	i2c_major = register_chrdev(0,DEV_NAME,&lsm303_fops);
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
		printk(KERN_INFO "Unablt to create class file\n");
		ret = PTR_ERR(i2c_class);
		goto out_unregister;
	}
	printk(KERN_INFO "Succeed to create class file\n");
	/*
	 * create i2c driver
	 */
	ret = i2c_add_driver(&lsm303_driver);
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
	unregister_chrdev(i2c_major,DEV_NAME);
out:
	return ret;
}
/*
 * exit module
 */
static __exit void buddy_exit(void)
{
	printk(KERN_INFO ">>>>>>>Lsm303 exit<<<<<<<");
	i2c_del_driver(&lsm303_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,DEV_NAME);
}

module_init(buddy_init);
module_exit(buddy_exit);

MODULE_AUTHOR("Buddy Zhang<51498122@qq.com>");
MODULE_DESCRIPTION("LSM303 driver");
MODULE_LICENSE("GPL");
