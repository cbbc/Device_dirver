/*
 * Copyright @2014
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "buddy.h"
static char *version = "$Revision: 1.9 $";
/*
 * buddy bus match
 */
static int buddy_match(struct device *dev,struct device_driver *driver)
{
	/*
	 * match device - copy dirver's name to device'name
	 */
	return !strncmp(dev_name(dev),driver->name,strlen(driver->name));
}
/*
 * buddy bus uevent
 */
static int buddy_hotplug(struct device *dev,struct kobj_uevent_env *env)
{
	return 0;
}
/*
 * device release
 */
static void buddy_bus_release(struct device *dev)
{
	printk(KERN_INFO "buddy release\n");
}
static struct bus_type buddy_bus_type = {
	.name   = "buddy",
	.match  = buddy_match, 
	.uevent = buddy_hotplug,
};
static struct device buddy_bus = {
	.init_name = "buddy-bus0",
	.release   = buddy_bus_release
};
/*
 * Export a simple attribute
 */
static ssize_t show_bus_version(struct bus_type *bus,char *buf)
{
	return snprintf(buf,PAGE_SIZE,"buddy-%s\n",version);
}
/*
 * bus device attribute
 */
static ssize_t show_bus_device(struct device *dev,struct device_attribute *attr,char *buf)
{
	return snprintf(buf,PAGE_SIZE,"buddy-new device\n");
}
/*
 * device attribute 
 */
static BUS_ATTR(version,S_IRUGO,show_bus_version,NULL);
static DEVICE_ATTR(buddy,S_IRUGO,show_bus_device,NULL);
/*
 * module init
 */
static __init int buddy_bus_init(void)
{
	int ret;
	/*
	 * register a bus 
	 */
	ret = bus_register(&buddy_bus_type);
	if(ret)
	{
		printk(KERN_INFO "fail to register bus into kernel\n");
		return ret;
	}
	printk(KERN_INFO "Succeed to register bus into kernel\n");
	/*
	 * add bus attribute
	 */
	if(bus_create_file(&buddy_bus_type,&bus_attr_version))
		printk(KERN_INFO "Unable to create version attribute\n");
	printk(KERN_INFO "succeed to create bus attribute\n");
	/*
	 * register bus device
	 */
	ret = device_register(&buddy_bus);
	if(ret)
		printk(KERN_INFO "Unable to register buddy bus\n");
	printk(KERN_INFO "succeed to register device\n");
	/*
	 * create bus device attribute
	 */
	if(device_create_file(&buddy_bus,&dev_attr_buddy))
		printk(KERN_INFO "Unable to create device attribute\n");
	printk(KERN_INFO "succeed to create device attribute\n");
	return ret;
}
/*
 * module exit
 */
static __exit void buddy_bus_exit(void)
{
	device_unregister(&buddy_bus);
	bus_unregister(&buddy_bus_type);
}
module_init(buddy_bus_init);
module_exit(buddy_bus_exit);

MODULE_AUTHOR("Buddy Zhang");
MODULE_LICENSE("Dual BSD/GPL");
