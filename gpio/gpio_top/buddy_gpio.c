#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <mach/hardware.h>

#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-core.h>  
#include <linux/delay.h>  
#include <linux/sched.h>  
#include <linux/mm.h>  
#include <linux/fs.h>  
#include <linux/types.h>  
#include <linux/errno.h>  
#include <linux/ioctl.h>  
#include <linux/cdev.h>  
#include <linux/string.h>  
#include <linux/list.h>  
#include <asm/unistd.h>  
#include <mach/map.h>  
#include <mach/regs-clock.h>  
#include <mach/regs-gpio.h>  
#include <plat/gpio-cfg.h>  
#include <mach/gpio-bank-e.h>

#define DEV_NAME "buddy"
#define GPIO_INPUT_0   S3C64XX_GPN(0)
#define GPIO_OUTPUT_0  S3C64XX_GPK(5)
static int major;




/*
 * buddy read
 */
static ssize_t buddy_read(struct file *filp,char __user *buf,size_t count,loff_t *offset)
{
	int ret;
	char *tmp;
	
	/*
	 * alloc buffer memory
	 */
	tmp = kmalloc(count,GFP_KERNEL);
	if(tmp == NULL)
	{
		printk(KERN_INFO "Unable to get free memory from system\n");
		ret = -ENOMEM;
		goto out;
	}
	printk(KERN_INFO "Succeed to get memory\n");
	
	/*
	 * read io state
	 */

	kfree(tmp);
	return 0;
out:
	return ret;

}
/*
 * buddy write
 */
static ssize_t buddy_write(struct file *filp,const char __user *buf,size_t count,loff_t *offset)
{
	int ret;
	char *tmp;

	tmp = kmalloc(count,GFP_KERNEL);
	if(tmp == NULL)
	{
		printk(KERN_INFO "Unable to get memroy from system\n");
		ret = -ENOMEM;
		goto out;
	}
	/*
	 * transfer data between kernel and user,return number of imcomplete
	 */
	ret = copy_from_user(tmp,buf,count);
	if(ret)
	{
		printk(KERN_INFO "loss %d bytes\n",ret);
		goto out_free;
	}
	printk(KERN_INFO "Succeed to copy data\n");
	kfree(tmp);
	return count;
out_free:
	kfree(tmp);
out:
	return ret;

}
/*
 * buddy open
 */
static int buddy_open(struct inode *indoe,struct file *filp)
{
	return 0;
}
/*
 * buddy release
 */
static int buddy_release(struct inode *inode,struct file *filp)
{

	return 0;
}

/*
 * struct file operations
 */
static struct file_operations buddy_fops = {
	.owner     = THIS_MODULE,
	.read      = buddy_read,
	.write     = buddy_write,
	.open      = buddy_open,
	.release   = buddy_release,
};
/*
 * input pin init
 */
static int input_pin_init(void)
{
	int ret;
	/*
	 * check is available
	 */
	ret = gpio_is_valid(GPIO_INPUT_0);
	if(ret == 0)
	{
		printk(KERN_INFO "The pin %s cannot use\n","GPN0");
		ret = -EINVAL;
		goto out;
	}
	printk(KERN_INFO "The pin %s can use\n","GPN0");
	/*
	 * pull pin
	 */
	s3c_gpio_setpull(GPIO_INPUT_0,S3C_GPIO_PULL_DOWN);
	/*
	 * set as input
	 */
	s3c_gpio_cfgpin(GPIO_INPUT_0,S3C_GPIO_SFN(0));
	return 0;
out:
	return ret;
}
/*
 * output pin init
 */
static int output_pin_init(void)
{
	int ret;
	/*
	 * check is available
	 */
	ret = gpio_is_valid(GPIO_OUTPUT_0);
	if(ret == 0)
	{
		printk(KERN_INFO "The Pin %s cannot use\n","GPK5");
		ret = -EINVAL;
		goto out;
	}
	printk(KERN_INFO "The Pin %s can use\n","GPK5");

	return 0;
out:
	return ret;
}
/*
 * init module
 */
static __init int buddy_init(void)
{
	int ret;
	/*
	 * pin alloc
	 */
	input_pin_init();
	output_pin_init();
	/*
	 * register char device
	 */
	major = register_chrdev(0,DEV_NAME,&buddy_fops);
	if(major == 0)
	{
		printk(KERN_INFO "Unable to inserch into system\n");
		return -EFAULT;
	}
	printk(KERN_INFO "Succeed to add cdev,the major[%d]\n",major);
	/*
	 * alloc gpio
	 */

	return 0;
}
/*
 * exit module
 */
static __exit void buddy_exit(void)
{
	unregister_chrdev(major,DEV_NAME);
}

module_init(buddy_init);
module_exit(buddy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Buddy Zhang <514981221@qq.com>");
