/*
 * Copyright@2014 Buddy Zhangrenze
 */
/*
 * The LM3642 is a 4KMHz fixed-frequency synchronous boost converter plus
 * 1.5A constant current driver for a high-current white LED.The high-side
 * current source allows for grounded cathode LED operation providing Flash
 * current up to 1.5A.An adaptive regulation method ensures the current 
 * source remains in regulation and maximizes efficiency.
 * The LM3642 is controlled via an I2C-compatible interface.Features inclu-
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

#define DEV_NAME "lm3642"
#define DEBUG          1

static struct class *i2c_class;
static struct i2c_client *my_client;
static int i2c_major;

#if DEBUG
static void lm3642_debug(struct i2c_client *client,char *name)
{
	printk(KERN_INFO "======================%s=======================\n=>client->name:%s\n=>client->addr:%08x\n=>adapter->name:%s\n",
		name,client->name,client->addr,client->adapter->name);
}
#endif 
/*
 * The LM3642 is a high-power white LED flash driver caoable of delivering 
 * up to 1.5A into a single high-powered LED.The LM3642 has two logic input
 * includeing a hardware Flash Enable(STROBE) and a Flash interrupt input
 * (Tx/TORCH) designed to interrupt the flash pulse during high battery 
 * current conditions.Both logic inputs has internal 300K.pulldown resis-
 * tors to GND.
 * Control of the LM3642 is done via an I2C-compathible interface.The incl-
 * udes adjustment of the Flash and Torch current levels,changing the Flash
 * Timeout Duration and changing the switch current limit.Additionally,the-
 * re flag and status bits that indicate flash current time-out,LED failure
 * (open/short).
 */
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
	msg[0].flags  = client->flags | I2C_M_TEN;
	msg[0].buf    = &address;
	msg[0].len    = 1;
	/* read initial */
	msg[1].addr   = client->addr;
	msg[1].flags  = client->flags | I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].buf    = buf;
	msg[1].len    = len;

	ret = i2c_transfer(client->adapter,msg,2);
	if(ret != 2)
	{
		printk(KERN_INFO "Unable to transfer data for read\n");
		return 1;
	}else
	{
		printk(KERN_INFO "Succeed to read data\n");
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
		printk(KERN_INFO "Unable to write data to i2c bus\n");
		return 1;
	} else
	{
		printk(KERN_INFO "Succeed to write data to i2c bus\n");
		return 0;
	}
}
/*
 * LM3642 Enable Register
 */
static void LM3642_Read_Enable_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x0A,buf,1);
}
static void LM3642_Write_Enable_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x0A,buf,1);
}
/*
 * LM3642 Flags Register
 */
static void LM3642_Read_Flags_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x0B,buf,1);
}
static void LM3642_Write_Flags_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x0B,buf,1);
}
/*
 * LM3642 Flash Feature Register
 */
static void LM3642_Read_Flash_Feature_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x08,buf,1);
}
static void LM3642_Write_Flash_Feature_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x08,buf,1);
}
/*
 * LM3642 Current Control Register
 */
static void LM3642_Read_Current_Control_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x09,buf,1);
}
static void LM3642_Write_Current_Control_Register(
		struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x09,buf,1);
}
/*
 * LM3642 IVFM Mode Register
 */
static void LM3642_Read_IVFM_Mode_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_read(client,0x01,buf,1);
}
static void LM3642_Write_IVFM_Mode_Register(struct i2c_client *client,
		char *buf)
{
	buddy_i2c_write(client,0x01,buf,1);
}
/*
 * LM3642 Torch Ramp Time Register
 */
static void LM3642_Read_Torch_Ramp_Time_Register(
		struct i2c_client *client,char *buf)
{
	buddy_i2c_read(client,0x06,buf,1);
}
static void LM3642_Write_Torch_Ramp_Time_Register(
		struct i2c_client *client,char *buf)
{
	buddy_i2c_write(client,0x06,buf,1);
}
/*
 * ===========================Flash Mode==============================
 * In Flash Mode,the LED current source provides 16 target current levels
 * from 93.75mA to 1500 mA.The Flash current are adjusted via the Current
 * Control Register.Flash Mode is activated by the Enable Register.Once the
 * Flash sequence is activated the current source(LED) will ramp up to the
 * programmed Flash current by stepping through all current steps until the
 * programmed current is reached.
 * When the part is enabled in the Flash Mode through the Enable Register,
 * all mode bits in the Enable Register are cleared after a flash time-out
 * event.
 * If the STROBE pin is used to enable the Flash Mode.Mode bits are cleared
 * after a single flash.To reflash,0x23 will have to be writen to 0x0A.
 * Flash Mode is activated also by pulling the STROBE pin HIGH. 
 */
static void LM3642_Flash_Mode(struct i2c_client *client)
{
	char buf;
	LM3642_Read_Enable_Register(client,&buf);
	buf = buf & 0xFC;
	buf = buf | 0x03;
	LM3642_Write_Enable_Register(client,&buf);
}
static void LM3642_Clear_Enable_Register_On_STROBE(
		struct i2c_client *client)
{
	char buf = 0x23;
	LM3642_Write_Enable_Register(client,&buf);
}
static void LM3642_Flash_Set_Leve(struct i2c_client *client,int num)
{
	char buf;
	if(num< 0 || num > 16)
	{
		printk(KERN_INFO "Over the Flash leve\n");
		return;
	}
	LM3642_Read_Current_Control_Register(client,&buf);
	buf = buf & 0xF0;
	buf = buf | num;
	LM3642_Write_Current_Control_Register(client,&buf);
}
/*
 * Flash Time-Out
 * The Flash Time-out period set the amount of time that the Flash Current
 * is being sourced from the current source(LED).The LM3642 has 8 time-out
 * levels ranging 100ms to 800ms in 100ms steps.The Flash Time-out period
 * is controlled in the FLASH FEATURES REGISTER.Flash Time-Out only appli-
 * es to the Flash Mode operation.The mode bits in the Enable Register are
 * cleared upon a Flash Time-out.
 */
static void LM3642_Flash_Set_Time_Out(struct i2c_client *client,int num)
{
	char buf;
	if(num < 0 || num > 8)
	{
		printk(KERN_INFO "Over the Flash Time-out\n");
		return;
	}
	LM3642_Read_Flash_Feature_Register(client,&buf);
	buf = buf & 0xF8;
	num = num & 0x07;
	buf = buf | num;
	LM3642_Write_Flash_Feature_Register(client,&buf);
}
static void LM3642_Flash_Set_Ramp_Time(struct i2c_client *client,int num)
{
	char buf;
	if(num < 0 || num > 8)
	{
		printk(KERN_INFO "Over the Flash Ramp Time\n");
		return;
	}
	LM3642_Read_Flash_Feature_Register(client,&buf);
	buf = buf & 0xC7;
	num = num & 0x07;
	num = num<<3;
	buf = buf | num;
	LM3642_Write_Flash_Feature_Register(client,&buf);
}
/*
 * Turn On the Flash
 */
static void LM3642_Flash_Turn_On(struct i2c_client *client)
{
	LM3642_Flash_Set_Ramp_Time(client,8);
	LM3642_Flash_Set_Time_Out(client,8);
	LM3642_Flash_Set_Leve(client,16);
	LM3642_Flash_Mode(client);
}
/*
 * =============================TORCH MODE===============================
 * In Torch Mode,the current source(LED) is programmed via the Current Con-
 * trol Register.Torch Mode is activated by the Enable Register and/or by
 * by Enabling the part in TX/Torch pin configuration.Once the Torch Mode
 * is enabled the current source wil ramp up to the programmed Torch curre-
 * nt level.The Ramp-Up and Ramp-Down times are independently adjustable
 * via the Torch Ramp Register.Torch Mode is not affected by Flash Timeout
 * In the LM3642,The programmable torch current ranges from 48.4mA to 375
 * mA.In the LM3642LT,the programmable torch current ranges from 24mA to
 * 187mA.
 */
static void LM3642_Torch_Mode(struct i2c_client *client)
{
	char buf;
	LM3642_Read_Enable_Register(client,&buf);
	buf = buf & 0xFC;
	buf = buf | 0x02;
	LM3642_Write_Enable_Register(client,&buf);
}
static void LM3642_Torch_Set_Leve(struct i2c_client *client,int num)
{
	char buf;
	if(num < 0 || num > 8)
	{
		printk(KERN_INFO "Over the Torch leve\n");
		return;
	}
	LM3642_Read_Current_Control_Register(client,&buf);
	buf = buf & 0x8F;
	num = num & 0x07;
	num = num<<4;
	num = num & 0x70;
	buf = buf | num;
	LM3642_Write_Current_Control_Register(client,&buf);
}
static void LM3642_Torch_Ramp_Up_Time(struct i2c_client *client,int num)
{
	char buf;
	if(num < 0 || num > 8)
	{
		printk(KERN_INFO "Over the Torch Ramp-Up Time\n");
		return;
	}
	LM3642_Read_Torch_Ramp_Time_Register(client,&buf);
	buf = buf & 0xC7;
	num = num & 0x07;
	num = num<<3;
	buf = buf | num;
	LM3642_Write_Torch_Ramp_Time_Register(client,&buf);
}
static void LM3642_Torch_Ramp_Down_Time(struct i2c_client *client,int num)
{
	char buf;
	if(num < 0 || num > 8)
	{
		printk(KERN_INFO "Over the Torch Ramp-Down Time\n");
		return;
	}
	LM3642_Read_Torch_Ramp_Time_Register(client,&buf);
	buf = buf & 0xF8;
	num = num & 0x07;
	buf = buf | num;
	LM3642_Write_Torch_Ramp_Time_Register(client,&buf);
}
static void LM3642_Torch_Turn_On(struct i2c_client *client)
{
	LM3642_Torch_Ramp_Down_Time(client,7);
	LM3642_Torch_Ramp_Up_Time(client,7);
	LM3642_Torch_Set_Leve(client,8);
	LM3642_Torch_Mode(client);
}
/*
 * ============================INDICATOR MODE============================
 * This mode is activate by the Enable Register.The LM3642 can be program-
 * ed to a current leve that is 1/8th the torch current value in the Curr-
 * ent Control Register.LM3642LT has only one setting of indicator current
 * at 5mA.
 */
static void LM3642_Indicator_Mode(struct i2c_client *client)
{
	char buf;
	LM3642_Read_Enable_Register(client,&buf);
	buf = buf & 0xFC;
	buf = buf | 0x01;
	LM3642_Write_Enable_Register(client,&buf);
}
static void LM3642_Indicator_Set_Leve(struct i2c_client *client,int num)
{
	char buf;
	if(num < 0 || num > 8)
	{
		printk(KERN_INFO "Over the indicator leve\n");
		return;
	}
	LM3642_Read_Current_Control_Register(client,&buf);
	buf = buf & 0x8F;
	num = num & 0x08;
	num = num<<4;
	buf = buf | num;
	LM3642_Write_Current_Control_Register(client,&buf);
}
/*
 * ==========================Fault Operation============================
 * Upon entering a fault condition,the LM3642 will set the appropriate fla-
 * g in the Flags Register.
 */
/*
 * write operation 
 */
static ssize_t lm3642_write(struct file *filp,const char __user *buf,size_t count,loff_t *offset)
{
	return 0;
}
/*
 * read operation
 */
static ssize_t lm3642_read(struct file *filp,char __user *buf,size_t count,loff_t *offset)
{
	int ret;
	struct i2c_client *client = filp->private_data;
#if DEBUG
	lm3642_debug(client,"read");
#endif
	LM3642_Torch_Turn_On(client);
	return 0;
}
/*
 * open operation
 */
static int lm3642_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>lm3642 open<<<<<\n");
	filp->private_data = my_client;
	return 0;
}
/*
 * release operation
 */
static int lm3642_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>lm3642 release<<<<<\n");
	filp->private_data = NULL;
	return 0;
}
/*
 * lm3642 filp operation
 */
static const struct file_operations lm3642_fops = {
	.owner    = THIS_MODULE,
	.read     = lm3642_read,
	.write    = lm3642_write,
	.open     = lm3642_open,
	.release  = lm3642_release,
};

/*
 * device id table
 */
static struct i2c_device_id lm3642_id[] = {
	{"lm3642",0x63},
	{},
};
/*
 * device probe
 */
static int lm3642_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int res;
	struct device *dev;
	my_client = client;
	printk(KERN_INFO ">>>>>lm3642 probe<<<<<\n");
	dev = device_create(i2c_class,&my_client->dev,MKDEV(i2c_major,0),NULL,DEV_NAME);
	if(IS_ERR(dev))
	{
		res = PTR_ERR(dev);
		goto error;
	}
#if DEBUG
	lm3642_debug(client,"probe");
#endif
	return 0;
error:
	return res;
}
/*
 * device remove
 */
static int lm3642_remove(struct i2c_client *client)
{
	printk(KERN_INFO ">>>>>lm3642 remove<<<<<\n");
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver
 */
static struct i2c_driver lm3642_driver = {
	.driver = {
		.name = "lm3642",
	},
	.probe    = lm3642_probe,
	.id_table = lm3642_id,
	.remove   = lm3642_remove,
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
	strlcpy(i2c_info.type,"lm3642",I2C_NAME_SIZE);
	i2c_info.addr = 0x63;

	my_client = i2c_new_device(adap,&i2c_info);
#if DEBUG
	lm3642_debug(my_client,"init");
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
	i2c_major = register_chrdev(0,DEV_NAME,&lm3642_fops);
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
	ret = i2c_add_driver(&lm3642_driver);
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
	unregister_chrdev(i2c_major,"lm3642");
out:
	return ret;

}
/*
 * exit module
 */
static __exit void buddy_exit(void)
{
	printk(KERN_INFO ">>>>>lm3642 exit<<<<<\n");
	i2c_del_driver(&lm3642_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,"lm3642");
}



MODULE_AUTHOR("Buddy <514981221@qq.com>");
MODULE_DESCRIPTION("lm3642 driver");
MODULE_LICENSE("GPL");

module_init(buddy_init);
module_exit(buddy_exit);
