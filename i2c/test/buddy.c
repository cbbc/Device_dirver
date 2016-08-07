/*
 * Written by Buddy.Zhang@aliyun.com
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define DEV_NAME "ISL29125"
#define I2C_ADDR 0x64 
#define ID_REG   0x08

#define DEBUG 0

static struct class *i2c_class;
struct i2c_client *my_client;
static int i2c_major;
typedef unsigned char reg_t;
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
/*
 * Read operation for file_operations.
 */
static ssize_t ISL29125_read(struct file *filp,char __user *buf,
		size_t count,loff_t *offset)
{
	printk(KERN_INFO "[%s]\n",__FUNCTION__);
	
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
	int i;

	printk(KERN_INFO "[%s]\n",__FUNCTION__);
	filp->private_data = (void *)my_client;

	for(i = 0x08 ; i < 20 ; i++)
		printk(KERN_INFO "Register[%p] %p\n",(void *)i,(void *)get_register(i));
	return 0;
}
/*
 * Close operation for file_operations
 */
static int ISL29125_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "[%s]\n",__FUNCTION__);

	return 0;
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
};
/*
 * device id table.
 */
static struct i2c_device_id ISL29125_id[] = {
	{DEV_NAME,I2C_ADDR},
	{},
};
/*
 * I2C probe
 */
static int ISL29125_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int res;
	struct device *dev;
	reg_t addr = ID_REG;
	char buffer = 0xff;

	printk(KERN_INFO "[%s]Probe\n",__FUNCTION__);
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
	debug(client,__FUNCTION__);
	i2c_read(addr,&buffer,4);

	printk(KERN_INFO "[%s]Device ID %08x\n",__FUNCTION__,buffer);
	printk(KERN_INFO "[%s]Probe finish\n",__FUNCTION__);
	return 0;
 
error:
	return res;
}
/*
 * I2C remove
 */
static int ISL29125_remove(struct i2c_client *client)
{
	printk(KERN_INFO "[%s]I2C Remove\n",__FUNCTION__);
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver.
 */
static struct i2c_driver ISL29125_driver = {
	.driver = {
		.name = DEV_NAME,
	},
	.probe    = ISL29125_probe,
	.id_table = ISL29125_id,
	.remove   = ISL29125_remove,
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

	printk(KERN_INFO "[%s]Initialize Module\n",__FUNCTION__);

	/*
	 * I2C Board information.
	 */
	adap = i2c_get_adapter(0);
	if(adap == NULL)
	{
		printk(KERN_INFO "ERR[%s]Unable to get I2C adapter\n",
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
		printk(KERN_INFO "ERR[%s]Unable to get new i2c device\n",
				__FUNCTION__);
		ret = -ENODEV;
		goto out;
	}
	/*
	 * Register the char device
	 */
	i2c_major = register_chrdev(0,DEV_NAME,&ISL29125_fops);
	if(i2c_major == 0)
	{
		printk(KERN_INFO "ERR[%s]Can't register char driver\n",
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
		printk(KERN_INFO "ERR[%s][%d]Unable to create class\n",
				__FUNCTION__,ret);
		goto out_register;
	}
	/*
	 * Add new i2c driver.Add driver into 
	 * /sys/bus/i2c/device/I2C_ADDr/driver
	 */
	ret = i2c_add_driver(&ISL29125_driver);
	if(ret)
	{
		printk(KERN_INFO "ERR[%s]Unable to add new driver to I2C\n",
				__FUNCTION__);
		ret = -EBUSY;
		goto out_class;
	}
	printk(KERN_INFO "[%s]Succeed to init\n",__FUNCTION__);
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
static __exit void ISL29125_exit(void)
{
	printk(KERN_INFO "[%s]Exit Module\n",__FUNCTION__);
	i2c_del_driver(&ISL29125_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,DEV_NAME);
	i2c_unregister_device(my_client);
}

MODULE_AUTHOR("Buddy <Buddy.D.Zhang@outlook.com>");
MODULE_DESCRIPTION("ISL29125 Gsensor\n");
MODULE_LICENSE("GPL");

module_init(ISL29125_init);
module_exit(ISL29125_exit);
