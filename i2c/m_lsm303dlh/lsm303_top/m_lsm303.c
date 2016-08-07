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

#define DEV_NAME "lsm303_magnetic"	
#define I2C_ADDRESS    0x1E
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
 * ======================Magnetic Register======================
 */
/*
 * CRA_REG_M Register
 */
static void LSM303_M_Enable_Temperature(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CRA_REG_M(client,&buf);
	buf = buf | 0x80;
	LSM303_Write_CRA_REG_M(client,&buf);
}
static void LSM303_M_Disable_Temperature(struct i2c_client *client)
{
	/* Must use this function for correct working of the device */
	char buf;
	LSM303_Read_CRA_REG_M(client,&buf);
	buf = buf & 0x7F;
	LSM303_Write_CRA_REG_M(client,&buf);
}
/*
 * output rate
 */
static void LSM303_M_Data_Rate(struct i2c_client *client,int num)
{
	/* 
	 * Data output rate bits.These bits set the rate at which data is wri-
	 * tten to all three data output register.
	 */
	char buf;
	
	if(num < 0 || num > 7)
	{
		printk(KERN_INFO "Over the data rate in M\n");
		return;
	}
	LSM303_Read_CRA_REG_M(client,&buf);
	num = num & 0x07;
	num = num<<2;
	buf = buf & 0xE3;
	buf = buf | num;
	LSM303_Write_CRA_REG_M(client,&buf);
}
static void LSM303_M_Get_Data_Rate(struct i2c_client *client)
{
	char buf;
	LSM303_Read_CRA_REG_M(client,&buf);
	buf = buf >>2;
	buf = buf & 0x07;
	switch(buf)
	{
		case 0x00:
			printk("The data out rate is 0.75 Hz\n");
			break;
		case 0x01:
			printk("The data out rate is 1.5 Hz\n");
			break;
		case 0x02:
			printk("The data out rate is 3.0 Hz\n");
			break;
		case 0x03:
			printk("The data out rate is 7.5 Hz\n");
			break;
		case 0x04:
			printk("The data out rate is 15 Hz\n");
			break;
		case 0x05:
			printk("The data out rate is 30 Hz\n");
			break;
		case 0x06:
			printk("The data out rate is 75 Hz\n");
			break;
		case 0x07:
			printk("The data out rate is 220 Hz\n");
			break;
	}
}
/*
 * Gain setting
 */
static void LSM303_M_Set_Gain(struct i2c_client *client,int num)
{
	char buf;
	
	if(num < 1 || num > 7)
	{
		printk(KERN_INFO "Over the rate in Gain Setting\n");
		return;
	}
	LSM303_Read_CRB_REG_M(client,&buf);
	num = num & 0x07;
	num = num <<5;
	buf = buf & 0x1F;
	buf = buf | num;
	LSM303_Write_CRB_REG_M(client,&buf);
}
static void LSM303_M_Get_Gain(struct i2c_client *client,char *buf)
{
	int num;
	LSM303_Read_CRB_REG_M(client,buf);
	*buf = *buf >>5;
	*buf = *buf & 0x07;
#if DEBUG
	printk(KERN_INFO "The Gain are %03x\n",*buf);
#endif
}
/*
 * Read Register
 */
 static void LSM303_M_Read_Register(struct i2c_client *client)
{
	char reg[12];
	int i;
	char addr = 0x00;
	for(i = 0;i<12;i++)
		buddy_i2c_read(client,addr+i,reg+i,1);
	for(i = 0;i<14;i++)
	{
		printk(KERN_INFO "Reg[%d]=>%02x\n",i,*(reg + i));
	}
}
/*
 * M Mode
 */
static void LSM303_M_Set_Mode(struct i2c_client *client,int num)
{
	char buf;
	
	if(num < 0 || num > 3)
	{
		printk(KERN_INFO "Over the rang in M Mode set\n");
		return;
	}
	LSM303_Read_MR_REG_M(client,&buf);
	num = num & 0x03;
	buf = buf & 0xFC;
	buf = buf | num;
	LSM303_Write_MR_REG_M(client,&buf);
}
static void LSM303_M_Get_Mode(struct i2c_client *client)
{
	char buf;
	
	LSM303_Read_MR_REG_M(client,&buf);
	buf = buf & 0x03;
	switch(buf)
	{
		case 0x00:
			printk(KERN_INFO "M:Continuous-conversion mode\n");
			break;
		case 0x01:
			printk(KERN_INFO "M:Single-conversion mode\n");
			break;
		case 0x02:
			printk(KERN_INFO "M:Sleep-mode.Device is placed in sleep-mode.\n");
			break;
		case 0x03:
			printk(KERN_INFO "M:Sleep-mode.Device is placed in sleep-mode.\n");
			break;
	}
}
/*
 * Out data
 */
static void LSM303_M_Out_X(struct i2c_client *client,short *data)
{
	char buf1,buf2;
	
	LSM303_Read_OUT_X_H_M(client,&buf1);
	LSM303_Read_OUT_X_L_M(client,&buf2);
	*data = 0;
	*data = buf1;
	*data = *data<<8;
	*data = *data | buf2;
}
static void LSM303_M_Out_Y(struct i2c_client *client,short *data)
{
	char buf1,buf2;
	
	LSM303_Read_OUT_Y_H_M(client,&buf1);
	LSM303_Read_OUT_Y_L_M(client,&buf2);
	*data = 0;
	*data = buf1;
	*data = *data<<8;
	*data = *data | buf2;
}
static void LSM303_M_Out_Z(struct i2c_client *client,short *data)
{
	char buf1,buf2;
	
	LSM303_Read_OUT_Z_H_M(client,&buf1);
	LSM303_Read_OUT_Z_L_M(client,&buf2);
	*data = 0;
	*data = buf1;
	*data = *data<<8;
	*data = *data | buf2;
}
static void LSM303_M_Out_Data(struct i2c_client *client,short *data)
{
	LSM303_M_Out_X(client,data);
	LSM303_M_Out_Y(client,(data+ 1 ));
	LSM303_M_Out_Z(client,(data + 2 ));
#if DEBUG
	printk(KERN_INFO "The M X %04x\n",*data);
	printk(KERN_INFO "The M Y %04x\n",*(data+1));
	printk(KERN_INFO "The M Z %04x\n",*(data+2));
#endif
}
/*
 * SR register
 */
static int LSM303_M_LOCK(struct i2c_client *client)
{
	/* Data output register lock */
	char buf;
	
	LSM303_Read_SR_REG_M(client,&buf);
	if((buf & 0x02) != 0)
	{
		/*
		 * A new set of measurements is available
		 */
		printk(KERN_INFO "M:Have data can read\n");
		return 0;
	}
	printk(KERN_INFO "M:No hava available data\n");
	return -1;
}
static int LSM303_M_Data_Ready(struct i2c_client *client)
{
	char buf;

	LSM303_Read_SR_REG_M(client,&buf);
	if((buf & 0x01) != 0)
	{
		/* a new set of measurements are available */
#if DEBUG
		printk(KERN_INFO "Data ready\n");
		return 0;
#endif
	}
	printk(KERN_INFO "Data no ready\n");
	return -1;
}
/*
 * User API
 */
static void LSM303_M_Use_API(struct i2c_client *client)
{
	char buf;
	short data[3] = {0,0,0};
	/* Setep 1: Set data output rate */
	LSM303_M_Data_Rate(client, 3);
#if DEBUG
	LSM303_M_Get_Data_Rate(client);
#endif
	/* Setep 2: Set Gain */
	LSM303_M_Set_Gain(client, 2);
#if DEBUG
	LSM303_M_Get_Gain(client, &buf);
#endif
	/* Setep 3: Set Mode */
 	LSM303_M_Set_Mode(client,0 );
#if DEBUG
	LSM303_M_Get_Mode(client);
#endif
	/* Setep 4: Output data */
	LSM303_M_Out_Data(client,data);
}
static void LSM303_M_Test(struct i2c_client *client)
{
	int a = 20;
	while(a--)
	{
		msleep(1000);
		LSM303_M_Use_API(client);
		//LSM303_M_Read_Register(client);
	}
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
	LSM303_M_Test(client);
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
