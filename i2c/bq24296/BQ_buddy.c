/*
 * Copyright@2014 Buddy Zhangrenze
 */
/*
 * The BQ24298 is a 4KMHz fixed-frequency synchronous boost converter plus
 * 1.5A constant current driver for a high-current white LED.The high-side
 * current source allows for grounded cathode LED operation providing Flash
 * current up to 1.5A.An adaptive regulation method ensures the current 
 * source remains in regulation and maximizes efficiency.
 * The BQ24298 is controlled via an I2C-compatible interface.Features inclu-
 * de a hardware flash enable(STROBE) allowing a logic input to trigger the
 * flash pulse into a low-current Torch Mode,allowing for synchronization 
 * to RF power amplifier events or other high-current conditions.
 * The 4 MHz switching frequency,over-voltage protection and adjustable 
 * current limit settings allow the use of tiny,low-profile inductors and
 * (10 uF) ceramic caoacitors.
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

#define DEV_NAME "BQ24298"
#define DEBUG          1
#define DEV_ID      0x6B
#define I2C_BUS_ID     2
#define BUDDY_DEBUG    1

static struct class *i2c_class;
static struct i2c_client *my_client;
static int i2c_major;

#if DEBUG
static void BQ24298_debug(struct i2c_client *client,char *name)
{
	printk(KERN_INFO "======================%s==============="
			 "========\n=>client->name:%s\n=>client->addr:"
			 "%08x\n=>adapter->name:%s\n",
		name,client->name,client->addr,client->adapter->name);
}
#endif 
/*
 * i2c read
 */
static int buddy_i2c_read(struct i2c_client *client,char address,
				char *buf,int len)
{
	int ret;
	struct i2c_msg msg[2];
	/* dummp write */
	msg[0].addr   = client->addr;
	msg[0].flags  = client->flags ;//| I2C_M_TEN;
	msg[0].buf    = &address;
	msg[0].len    = 1;
	/* read initial */
	msg[1].addr   = client->addr;
	msg[1].flags  = client->flags ;//| I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].buf    = buf;
	msg[1].len    = len;

	ret = i2c_transfer(client->adapter,msg,2);
	if(ret != 2)
	{
		printk(KERN_INFO "BQ Unable to transfer data for read\n");
		return 1;
	}else
	{
		printk(KERN_INFO "BQ Succeed to read data\n");
		return 0;
	}
}
/*
 * i2c_write
 */
static int buddy_i2c_write(struct i2c_client *client,char address,
				char *buf,int len)
{
	int ret;
	char tmp[len+1];
	struct i2c_msg msg;

	tmp[0] = address;
	for(ret = 1;ret <= len ;ret++)
	{
		tmp[ret] = buf[ret-1];
	}
	/* write initial */
	msg.addr    = client->addr;
//	msg.flags   = client->flags | I2C_M_TEN;
	msg.flags   = client->flags;
	msg.buf     = tmp;
	msg.len     = len+1;

	ret = 0;
	ret = i2c_transfer(client->adapter,&msg,1);
	if(ret != 1)
	{
		printk(KERN_INFO "BQ Unable to write data to i2c bus\n");
		return 1;
	} else
	{
		printk(KERN_INFO "BQ Succeed to write data to i2c bus\n");
		return 0;
	}
}
/*
 * The operation for set fixed bit.
 */
static void buddy_set(struct i2c_client *client,char address,int num)
{
	char mybuf;
	int mask;

	buddy_i2c_read(client,address,&mybuf,1);
	mask = ((1UL << num) & 0xFF);
	mybuf |= mask;
	buddy_i2c_write(client,address,&mybuf,1);
}
/*
 * The operation for clear fixed bit.
 */
static void buddy_clear(struct i2c_client *client,char address,int num)
{
	char mybuf;
	int mask;

	buddy_i2c_read(client,address,&mybuf,1);
	mask = (~(1UL << num) & 0xFF);
	mybuf &= mask;
	buddy_i2c_write(client,address,&mybuf,1);
}
/*
 * BUG_OOPS
 */
static void BUG_OOPS(int num,int max)
{
	if(num < 0 || num > max)
	{
		printk(KERN_INFO "BQ[%s]Ba_operation\n",__FUNCTION__);
	}
}
/*
 * Mask 
 */
static void MASK(char *buf,int num,int shift,int len)
{
	int mask;

	mask = ((1UL << len) - 1);
	mask = mask << shift;
	num = num << shift;
	num = num & mask;
	*buf = *buf & ~(mask);
	*buf = *buf | num;
}
/*
 * GET
 */
static void LAP(char *buf,int shift,int len)
{
	int mask;

	mask = ((1UL << len) - 1);
	mask = mask << shift;
	*buf = *buf & mask;
	*buf = *buf >> shift;
	mask = mask >> shift;
	*buf = *buf & mask;
}
/*
 * DEN
 */
static int DENY(struct i2c_client *client,char address,
		int shift,int mask_len)
{
	char mybuf;

	buddy_i2c_read(client,address,&mybuf,1);

	LAP(&mybuf,shift,mask_len);

	return (int)mybuf;
}
/*
 * RO
 */
static int RO(struct i2c_client *client,char address,
		int shift)
{
	int value;

	value = DENY(client,address,shift,1);

	return !!(value);
}
/*
 * Band operation
 */
static void BAND(struct i2c_client *client,char address,int num,
		int shift,int mask_len,int max)
{
	char mybuf;

	buddy_i2c_read(client,address,&mybuf,1);

	BUG_OOPS(num,max);
	MASK(&mybuf,num,shift,mask_len);

	buddy_i2c_write(client,address,&mybuf,1);
}
/*
 * SEG
 */
static void SEG(struct i2c_client *client,char address,int anon,int num)
{
	if(num == 0)
		buddy_clear(client,address,anon);
	else
		buddy_set(client,address,anon);
}
/*
 * The limit of input voltage.
 * Range:  [3.88V - 5.08V].
 * Offset: 3.88V.
 */
static void Input_Voltage_Limit(struct i2c_client *client,int num)
{
	BAND(client,0x00,num,3,4,15);
}
/*
 * Set the minimum system voltage limit.
 * Range: [3.0V - 3.7V]
 * Offset: 3.0V
 */
static void Minimum_System_Voltage_Limit(struct i2c_client *client,
		int num)
{
	BAND(client,0x01,num,1,3,7);
}
/*
 * The limit of the fast charge current.
 * Range: [512mA - 3008mA]
 * Offset: 512mA
 */
static void Fast_Charge_Current_Limit(struct i2c_client *client,int num)
{
	BAND(client,0x02,num,2,6,63);
}
/*
 * The limit of Pre-Charge.
 * Range: [128mA - 2048mA]
 * Offset: 128mA.
 */
static void PreChage_Current_Limit(struct i2c_client *client,int num)
{
	BAND(client,0x03,num,4,4,15);
}
/*
 * The limit of termination current.
 * Range: [128mA - 2048mA]
 * Offset: 128mA.
 */
static void Termination_Current_Limit(struct i2c_client *client,int num)
{
	BAND(client,0x03,num,0,4,15);
}
/*
 * Charge voltage limit.
 */
static void Charge_Voltage_Limit(struct i2c_client *client,int num)
{
	BAND(client,0x04,num,2,6,63);
}
/*
 * The limit of input current.
 */
static void Input_Current_Limit(struct i2c_client *client,int num)
{
	BAND(client,0x00,num,0,3,7);
}
/*
 * Register reset.
 * @num:
 * 0 - Keep current register setting.
 * 1 - Reset to default.
 */
static void register_reset(struct i2c_client *client,int num)
{
	SEG(client,0x01,7,num);
}
/*
 * I2C watchdog timer reset.
 * 0 - Normal.
 * 1 - Reset.
 */
static void watchdog_timer_reset(struct i2c_client *client,int num)
{
	SEG(client,0x01,6,num);
}
/*
 * I2C watchdog time setting
 * 00 - Disable timer.
 * 01 - 40s
 * 10 - 80s
 * 11 - 160s
 */
static void Watching_Timer_Setting(struct i2c_client *client,int num)
{
	BAND(client,0x05,num,4,2,4);
}
static void Watching_Timer_disable(struct i2c_client *client)
{
	SEG(client,0x05,4,0);
	SEG(client,0x05,5,0);
}
/*
 * Charge Safety Timer Setting.
 */
static void Charging_Safety_Timer_Enable(struct i2c_client *client)
{
	SEG(client,0x05,3,1);
}
static void Charging_Safety_Timer_Disable(struct i2c_client *client)
{
	SEG(client,0x05,3,0);
}
/*
 * Timer Setting of Fast Charge.
 * 00 - 5hrs
 * 01 - 8hrs
 * 10 - 12hrs
 * 11 - 20hrs
 */
static void Fast_Charge_Timer_Setting(struct i2c_client *client,int num)
{
	BAND(client,0x05,num,1,2,3);
}
/*
 * OTG configure.
 * 0 - Disable OTG.
 * 1 - Enable OTG.
 */
static void OTG_enable(struct i2c_client *client)
{
	SEG(client,0x01,5,1);
}
static void OTG_disable(struct i2c_client *client)
{
	SEG(client,0x01,5,0);
}
/*
 * Charge configure.
 * 0 - Charge disable.
 * 1 - Charge enable.
 */
static void Charge_enable(struct i2c_client *client)
{
	SEG(client,0x01,4,1);
}
static void Charge_disable(struct i2c_client *client)
{
	SEG(client,0x01,4,0);
}
/*
 * The limit of Boost Mode.
 * 0 - 1A.
 * 1 - 1.5A.
 */
static void Boost_mode_current_limit(struct i2c_client *client,int num)
{
	SEG(client,0x01,0,num);
}
/*
 * Enable of charge termination.
 */
static void Charge_Termination_Enable(struct i2c_client *client)
{
	SEG(client,0x05,7,1);
}
static void Charge_Termination_Disable(struct i2c_client *client)
{
	SEG(client,0x05,7,0);
}
/*
 * BOOSTV
 */
static void BOOSTV(struct i2c_client *client,int num)
{
	BAND(client,0x06,num,4,4,15);
}
/*
 * Thermal Regulation.
 */
static void Thermal_Regulation_Threshold(struct i2c_client *client,int num)
{
	BAND(client,0x06,num,0,2,3);
}
/*
 * The state of VBUS
 */
static int VBUS_STAT(struct i2c_client *client)
{
	int value;

	value = DENY(client,0x08,6,2);

#if BUDDY_DEBUG
	if(value == 0)
		printk(KERN_INFO "BQ[%s]no input or DPDM",__FUNCTION__);
	else if(value == 1)
		printk(KERN_INFO "BQ[%s]USB host\n",__FUNCTION__);
	else if(value == 2)
		printk(KERN_INFO "BQ[%s]Adapter port\n",__FUNCTION__);
	else if(value == 3)
		printk(KERN_INFO "BQ[%s]OTG\n",__FUNCTION__);
#endif
	return value;
}
/*
 * The state of CHRG
 */
static int CHRG_STAT(struct i2c_client *client)
{
	int value;

	value = DENY(client,0x08,4,2);

#if BUDDY_DEBUG
	if(value == 0)
		printk(KERN_INFO "BQ[%s]Not Charging\n",__FUNCTION__);
	else if(value == 1)
		printk(KERN_INFO "BQ[%s]Pre-charge\n",__FUNCTION__);
	else if(value == 2)
		printk(KERN_INFO "BQ[%s]Fast-Charge\n",__FUNCTION__);
	else if(value == 3)
		printk(KERN_INFO "BQ[%s]Charge Termination Done\n",__FUNCTION__);
#endif
	return value;
}
/*
 * Power state
 */
static int PG_STAT(struct i2c_client *client)
{
	int value;

	value = RO(client,0x08,2);

#if BUDDY_DEBUG
	if(value)
		printk(KERN_INFO "BQ[%s]Power Good\n",__FUNCTION__);
	else
		printk(KERN_INFO "BQ[%s]Not Power Good\n",__FUNCTION__);
#endif
	return value;
}
/*
 * Vendor information
 */
static void Vender(struct i2c_client *client)
{
	int value;

	value = DENY(client,0x0A,5,3);

#if BUDDY_DEBUG
	if(value == 1)
		printk(KERN_INFO "BQ[%s]BQ24296\n",__FUNCTION__);
	else if(value == 3)
		printk(KERN_INFO "BQ[%s]BQ24297\n",__FUNCTION__);
#endif
}
#if ppppppppppp
#endif
/*
 * write operation 
 */
static ssize_t BQ24298_write(struct file *filp,const char __user *buf,size_t count,loff_t *offset)
{
	return 0;
}
/*
 * read operation
 */
static ssize_t BQ24298_read(struct file *filp,char __user *buf,size_t count,loff_t *offset)
{
	int ret;
	struct i2c_client *client = filp->private_data;
	char mybuf;
	int addr = 0x00;
	int i;

#if BUDDY_DEBUG
	Vender(client);
	PG_STAT(client);
	CHRG_STAT(client);
	VBUS_STAT(client);
	Input_Voltage_Limit(client,12);
#endif

	while(addr < 0x0B)
	{
		//mybuf = 0;
		memset(&mybuf,0,1);
 		buddy_i2c_read(client,addr,&mybuf,1);
		printk(KERN_INFO "BuddyBQ-Register[%p] %p\n",(void *)addr,(void *)mybuf);
		addr++;
	}
	
	return 0;
}
/*
 * open operation
 */
static int BQ24298_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>BQ24298 open<<<<<\n");
	filp->private_data = my_client;
	return 0;
}
/*
 * release operation
 */
static int BQ24298_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>BQ24298 release<<<<<\n");
	filp->private_data = NULL;
	return 0;
}
/*
 * BQ24298 filp operation
 */
static const struct file_operations BQ24298_fops = {
	.owner    = THIS_MODULE,
	.read     = BQ24298_read,
	.write    = BQ24298_write,
	.open     = BQ24298_open,
	.release  = BQ24298_release,
};

/*
 * device id table
 */
static struct i2c_device_id BQ24298_id[] = {
	{"BQ24298",0x6B},
	{},
};
/*
 * device probe
 */
static int BQ24298_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int res;
	struct device *dev;
	my_client = client;
	printk(KERN_INFO ">>>>>BQ24298 probe<<<<<\n");
	dev = device_create(i2c_class,&my_client->dev,MKDEV(i2c_major,0),NULL,DEV_NAME);
	if(IS_ERR(dev))
	{
		res = PTR_ERR(dev);
		goto error;
	}
#if DEBUG
	BQ24298_debug(client,"probe");
#endif
	return 0;
error:
	return res;
}
/*
 * device remove
 */
static int BQ24298_remove(struct i2c_client *client)
{
	printk(KERN_INFO ">>>>>BQ24298 remove<<<<<\n");
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver
 */
static struct i2c_driver BQ24298_driver = {
	.driver = {
		.name = "BQ24298",
	},
	.probe    = BQ24298_probe,
	.id_table = BQ24298_id,
	.remove   = BQ24298_remove,
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

	adap = i2c_get_adapter(I2C_BUS_ID);
	if(adap == NULL)
	{
		printk(KERN_INFO "Unable to get i2c adapter\n");
		ret = -EFAULT;
		goto out;
	}
	printk(KERN_INFO "Succeed to get i2c adapter[%s]\n",adap->name);
	
	memset(&i2c_info,0,sizeof(struct i2c_board_info));
	strlcpy(i2c_info.type,"BQ24298",I2C_NAME_SIZE);
	i2c_info.addr = DEV_ID;

	my_client = i2c_new_device(adap,&i2c_info);
#if DEBUG
	BQ24298_debug(my_client,"init");
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
	i2c_major = register_chrdev(0,DEV_NAME,&BQ24298_fops);
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
	ret = i2c_add_driver(&BQ24298_driver);
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
	unregister_chrdev(i2c_major,"BQ24298");
out:
	return ret;

}
/*
 * exit module
 */
static __exit void buddy_exit(void)
{
	printk(KERN_INFO ">>>>>BQ24298 exit<<<<<\n");
	i2c_del_driver(&BQ24298_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,"BQ24298");
}



MODULE_AUTHOR("Buddy <514981221@qq.com>");
MODULE_DESCRIPTION("BQ24298 driver");
MODULE_LICENSE("GPL");

module_init(buddy_init);
module_exit(buddy_exit);
