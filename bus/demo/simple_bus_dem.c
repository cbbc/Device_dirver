/*
 * Copyright @2014
 */
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

static char *version = "$Revision: 1.9 $";
/*
 * buddy bus match
 */
static int buddy_match(struct device *dev,struct device_driver *driver)
{
	printk(KERN_INFO "buddy-bus match\n");
	return 0;
}
/*
 * buddy bus uevent
 */
static int buddy_hotplug(struct device *dev,struct kobj_uevnet_env *env)
{
	return 0;
}

struct bus_type buddy_bus_type = {
	.name   = "buddy",
	.match  = buddy_match, 
	.uevent = buddy_hotplug,
};

/*
 * Export a simple attribute
 */
static ssize_t show_bus_version(struct bus_type *bus,char *buf)
{
	return snprintf(buf,PAGE_SIZE,"buddy-%s\n",version);
}
static BUS_ATTR(version,S_IRUGO,show_bus_version,NULL);

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
	return ret;
}
/*
 * module exit
 */
static __exit void buddy_bus_exit(void)
{
	bus_unregister(&buddy_bus_type);
}
module_init(buddy_bus_init);
module_exit(buddy_bus_exit);

MODULE_AUTHOR("Buddy Zhang");
MODULE_LICENSE("Dual BSD/GPL");
