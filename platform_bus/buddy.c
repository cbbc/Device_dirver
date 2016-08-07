#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>

#define DEV_NAME "buddy"

/*
 * open operation
 */
static int buddy_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "Open operation\n");
	return 0;
}
/*
 * release operation
 */
static int buddy_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "Close operation\n");
	return 0;
}
/*
 * cdev struct 
 */
static struct file_operations buddy_ops = {
	.owner = THIS_MODULE,
	.open  = buddy_open,
	.release = buddy_release,
};









/*
 * driver inface
 */
static struct class  *buddy_class  = NULL;
static struct device *buddy_device = NULL;
static int buddy_major;
static dev_t buddy_devno;
static struct cdev buddy_cdev;

static int buddy_probe(struct platform_device *dev)
{
	int ret = 0,err = 0;
	printk(KERN_INFO "buddy probe\n");

	buddy_major = register_chrdev(0,DEV_NAME,&buddy_ops); 
	if(buddy_major == 0)
	{
		printk(KERN_INFO "Faile to register char device\n");
		return -EFAULT;
	}
	printk(KERN_INFO "The character major are %d\n",buddy_major);
	buddy_devno = MKDEV(buddy_major,1);

	/*
	 * class create
	 */
	buddy_class = class_create(THIS_MODULE,DEV_NAME);
	if(buddy_class == NULL)
	{
		printk(KERN_INFO "Class create failed!\n");
		goto out_register;
	}
	/*
	 * device create
	 */
	buddy_device = device_create(buddy_class,NULL,buddy_devno,
			NULL,DEV_NAME);
	if(buddy_device == NULL)
	{
		printk(KERN_INFO "Can't get device\n");
		goto out_class;
	}
	printk(KERN_INFO "device probe ok!\n");
	return 0;
out_class:
	class_destroy(buddy_class);
out_register:
	unregister_chrdev(buddy_major,DEV_NAME);
	return -EFAULT;
}
/*
 * remove
 */
static int buddy_remove(struct platform_device *dev)
{
	printk(KERN_INFO "Remove\n");
	device_destroy(buddy_class,buddy_devno);
	class_destroy(buddy_class);
	unregister_chrdev(buddy_major,DEV_NAME);
	return 0;
}
/*
 * platform device driver
 */
static struct platform_driver buddy_platform_driver = {
	.probe  = buddy_probe,
	.remove = buddy_remove,
	.driver = {
		.name  = DEV_NAME,
		.owner = THIS_MODULE,
	},
};
/*
 * platform device
 */
static struct platform_device buddy_platform_device = {
	.name = DEV_NAME,
	id    = 0,
	.dev  = {
	}
};
/*
 * init function
 */
static __init int buddy_init(void)
{
	int ret = 0;
	printk(KERN_INFO "Init\n");

	ret = platform_device_register(&buddy_platform_device);
	if(ret)
	{
		printk(KERN_INFO "platform device register failed\n");
		return ret;
	}

	ret = platform_driver_register(&buddy_platform_driver);
	if(ret)
	{
		printk(KERN_INFO "platform driver register failed\n");
		return ret;
	}
	printk(KERN_INFO "init ok\n");
	return ret;
}
/*
 * exit
 */
static __exit void buddy_exit(void)
{
	printk(KERN_INFO "exit\n");
	platform_driver_unregister(&buddy_platform_driver);
}

module_init(buddy_init);
module_exit(buddy_exit);

