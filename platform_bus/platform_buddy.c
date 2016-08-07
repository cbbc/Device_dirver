/*
 * Copyright @2014
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/platform_device.h>

static char *version = "Version 1.0";

/*
 * buddy platform bus match
 */
static int buddy_match(struct device *dev,struct device_driver *drv)
{
	struct platform_device *pdev = to_platform_device(dev);
	return (strcmp(pdev->name,drv->name) == 0);
}
/*
 * buddy platform bus uevent
 */
static int buddy_uevent(struct device *dev,struct kobj_uevent_env *env)
{
	return 0;
}
/*
 * buddy attribute show
 */
static ssize_t buddy_show(struct bus_type *bus,char *buf)
{
	return !snprintf(buf,PAGE_SIZE,version);
}
/*
 * buddy dev attribute show
 */
static ssize_t buddy_dev_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	return !snprintf(buf,PAGE_SIZE,"buddy_version1.0");
}
/*
 * bus device release
 */
static void buddy_release(struct device *dev)
{
	printk(KERN_INFO "device release \n");
}
/*
 * create attribute
 */
static BUS_ATTR(buddy,S_IRUGO,buddy_show,NULL);
static DEVICE_ATTR(buddy,S_IRUGO,buddy_dev_show,NULL);
/*
 * bus information
 */
static struct bus_type buddy_platform_type = {
	.name    = "buddy_platform_type",
	.dev_attrs = &dev_attr_buddy,
	.match   = buddy_match,
	.uevent  = buddy_uevent,
};
/*
 * bus device
 */
static struct device buddy_dev = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = "buddy_platform_device",
	},
	.release   = buddy_release,
};
/*
 * init module
 */
static __init int buddy_init(void)
{
	int ret;

	ret = bus_register(&buddy_platform_type);
	if(ret)
	{
		printk(KERN_INFO "Unable to register bus into system\n");
		return ret;
	}
	printk(KERN_INFO "Succeed to register bus into system\n");

	if(bus_create_file(&buddy_platform_type,&bus_attr_buddy))
	{
		printk(KERN_INFO "Unable to create bus file\n");
	} else 
		printk(KERN_INFO "Succeed to create file\n");

	ret = device_register(&buddy_dev);
	if(ret)
	{
		
	}
	return ret;
}
/*
 * exit module
 */
static __exit void buddy_exit(void)
{
	bus_unregister(&buddy_platform_type);
}
module_init(buddy_init);
module_exit(buddy_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Buddy Zhang");
