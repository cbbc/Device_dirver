/*
 * Copyright@2014 Buddy Zhang
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

#define DEV_NAME       
#define I2C_ADDRESS     

#define DEBUG          1

static struct class *i2c_class;
static struct i2c_client *my_client;
static int i2c_major;

#if DEBUG
static void buddy_debug(struct i2c_client *client,char *name)
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
static int buddy_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>open<<<<<\n");
	filp->private_data = my_client;
	return 0;
}
/*
 * =========================Register Operation=========================
 */
/*
 * User API
 */
static void Use_API(struct i2c_client *client)
{
}
static void Test(struct i2c_client *client)
{
	Use_API(client);
}
/*
 * release device
 */
static int buddy_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO ">>>>>release<<<<<\n");
	filp->private_data = NULL;
	return 0;
}
/*
 * read device
 */
static ssize_t buddy_read(struct file *filp,char __user *buf,
		size_t count,loff_t *offset)
{
	struct i2c_client *client = filp->private_data;
#if DEBUG
	buddy_debug(client,"read");
	Test(client);
#endif
       
	return 0;
}
/*
 * file operations
 */
static struct file_operations buddy_fops = {
	.owner     = THIS_MODULE,
	.open      = buddy_open,
	.release   = buddy_release,
	.read      = buddy_read,
};
/*
 * device id table
 */
static struct i2c_device_id buddy_id[] = {
	{DEV_NAME,I2C_ADDRESS},
	{},
};
/*
 * device probe
 */
static int buddy_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int res;
	struct device *dev;
	my_client = client;
	printk(KERN_INFO ">>>>>>probe<<<<<<\n");
	dev = device_create(i2c_class,&my_client->dev,MKDEV(i2c_major,0),
			NULL,DEV_NAME);
	if(IS_ERR(dev))
	{
		res = PTR_ERR(dev);
		goto error;
	}
#if DEBUG
	buddy_debug(client,"probe");
#endif
	return 0;
error:
	return res;
}
/*
 * device remove
 */
static int buddy_remove(struct i2c_client *client)
{
	printk(KERN_INFO ">>>>remove<<<<\n");
	device_destroy(i2c_class,MKDEV(i2c_major,0));
	return 0;
}
/*
 * i2c_driver
 */
static struct i2c_driver buddy_driver = {
	.driver   = {
		.name = DEV_NAME,
	},
	.probe    = buddy_probe,
	.id_table = buddy_id,
	.remove   = buddy_remove,
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
	buddy_debug(my_client,"init");
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
	i2c_major = register_chrdev(0,DEV_NAME,&buddy_fops);
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
	ret = i2c_add_driver(&buddy_driver);
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
	printk(KERN_INFO ">>>>>>>exit<<<<<<<");
	i2c_del_driver(&buddy_driver);
	class_destroy(i2c_class);
	unregister_chrdev(i2c_major,DEV_NAME);
}

module_init(buddy_init);
module_exit(buddy_exit);

MODULE_AUTHOR("Buddy Zhang<51498122@qq.com>");
MODULE_DESCRIPTION(DEV_NAME);
MODULE_LICENSE("GPL");
