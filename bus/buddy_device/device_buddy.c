/*
 * Copyright @2014 
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
//#include "../buddy_bus/buddy.h"
typedef struct {
	char *name;
	struct device dev;
}buddy_dev;
static buddy_dev *dev;
/*
 * show device attribute
 */
static int buddy_show_attr(struct device *dev,struct device_attribute *attr,char *buf)
{
	return !snprintf(buf,PAGE_SIZE,"buddy-device-new");
}
DEVICE_ATTR(buddy,S_IRUGO,buddy_show_attr,NULL);
/*
 * device releas
 */
static void buddy_release(struct device *dev)
{
	printk(KERN_INFO "buddy-release\n");
}

/*
 * module init
 */
static __init int buddy_init(void)
{
	int ret;
	/*
	 × get a free buddy_dev
	 */
	dev = kmalloc(sizeof(buddy_dev),GFP_KERNEL);
	if(!dev)
	{
		printk(KERN_INFO "NO more free memory\n");
		return -ENOMEM;
	}
	dev->name = "buddy_zhangrenze";
	printk(KERN_INFO "buddy_alloc_memory\n");
	//dev->dev.bus     = &buddy_bus_type;
	//dev->dev.parent  = NULL;
	dev->dev.release = buddy_release;
	printk(KERN_INFO "debug1\n");
	/*
	 * register device
	 */
	ret = device_register(&dev->dev);
	printk(KERN_INFO "debug2\n");
	if(ret)
	{
		printk(KERN_INFO "Cannot register device\n");
		return ret;
	}
	printk(KERN_INFO "debug3\n");
	if(device_create_file(&dev->dev,&dev_attr_buddy))
		printk(KERN_INFO "Cannot devicec reate file\n");
	printk(KERN_INFO "debug4\n");
	return ret;
}
/*
 * module_exit
 */
static __exit void buddy_exit(void)
{
	device_unregister(&dev->dev);
	kfree(dev);
}

module_init(buddy_init);
module_exit(buddy_exit);


MODULE_AUTHOR("Buddy Zhang");
MODULE_LICENSE("Dual BSD/GPL");
