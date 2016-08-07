#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <asm/gpio.h>
#include <mach/gpio-bank-k.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>
#include <linux/io.h>
#include <plat/gpio-cfg.h>

#define DEV_NAME "buddy_test"
/*
 * look for free gpio
 */
static void buddy_get_free_gpio_list(void)
{
	int ret = 0;
	int i;
	for(i = 0;i<187;i++)
	{
		ret = gpio_request(i,"Buddy");
		if(ret == 1)
			printk(KERN_INFO "Find free GPIO[%3d]\n",i);
		else
			printk(KERN_INFO "GPIO[%3d] not free\n",i);
	}
}
/*
 * GPIO bank size
 */
static void buddy_GPIO_bank_size(void)
{
	printk(KERN_INFO "S3C64XX_GPIO_A_NR[%2d]\n",S3C64XX_GPIO_A_NR);
	printk(KERN_INFO "S3C64XX_GPIO_B_NR[%2d]\n",S3C64XX_GPIO_B_NR);
	printk(KERN_INFO "S3C64XX_GPIO_C_NR[%2d]\n",S3C64XX_GPIO_C_NR);
	printk(KERN_INFO "S3C64XX_GPIO_D_NR[%2d]\n",S3C64XX_GPIO_D_NR);
	printk(KERN_INFO "S3C64XX_GPIO_E_NR[%2d]\n",S3C64XX_GPIO_E_NR);
	printk(KERN_INFO "S3C64XX_GPIO_F_NR[%2d]\n",S3C64XX_GPIO_F_NR);
	printk(KERN_INFO "S3C64XX_GPIO_G_NR[%2d]\n",S3C64XX_GPIO_G_NR);
	printk(KERN_INFO "S3C64XX_GPIO_H_NR[%2d]\n",S3C64XX_GPIO_H_NR);
	printk(KERN_INFO "S3C64XX_GPIO_I_NR[%2d]\n",S3C64XX_GPIO_I_NR);
	printk(KERN_INFO "S3C64XX_GPIO_J_NR[%2d]\n",S3C64XX_GPIO_J_NR);
	printk(KERN_INFO "S3C64XX_GPIO_K_NR[%2d]\n",S3C64XX_GPIO_K_NR);
	printk(KERN_INFO "S3C64XX_GPIO_L_NR[%2d]\n",S3C64XX_GPIO_L_NR);
	printk(KERN_INFO "S3C64XX_GPIO_M_NR[%2d]\n",S3C64XX_GPIO_M_NR);
	printk(KERN_INFO "S3C64XX_GPIO_N_NR[%2d]\n",S3C64XX_GPIO_N_NR);
	printk(KERN_INFO "S3C64XX_GPIO_O_NR[%2d]\n",S3C64XX_GPIO_O_NR);
	printk(KERN_INFO "S3C64XX_GPIO_P_NR[%2d]\n",S3C64XX_GPIO_P_NR);
	printk(KERN_INFO "S3C64XX_GPIO_Q_NR[%2d]\n",S3C64XX_GPIO_Q_NR);
}
/*
 * Get GPIO_GPKDAT value
 */
static void buddy_get_GPKDAT(void)
{
	printk(KERN_INFO "GPKDAT[%2d]\n",readl(S3C64XX_GPKDAT));
	printk(KERN_INFO "GPK00[%2d]\n",gpio_get_value(S3C64XX_GPK(0)));
	printk(KERN_INFO "GPK01[%2d]\n",gpio_get_value(S3C64XX_GPK(1)));
	printk(KERN_INFO "GPK02[%2d]\n",gpio_get_value(S3C64XX_GPK(2)));
	printk(KERN_INFO "GPK03[%2d]\n",gpio_get_value(S3C64XX_GPK(3)));
	printk(KERN_INFO "GPK04[%2d]\n",gpio_get_value(S3C64XX_GPK(4)));
	printk(KERN_INFO "GPK05[%2d]\n",gpio_get_value(S3C64XX_GPK(5)));
	printk(KERN_INFO "GPK06[%2d]\n",gpio_get_value(S3C64XX_GPK(6)));
	printk(KERN_INFO "GPK07[%2d]\n",gpio_get_value(S3C64XX_GPK(7)));
	printk(KERN_INFO "GPK08[%2d]\n",gpio_get_value(S3C64XX_GPK(8)));
	printk(KERN_INFO "GPK09[%2d]\n",gpio_get_value(S3C64XX_GPK(9)));
	printk(KERN_INFO "GPK10[%2d]\n",gpio_get_value(S3C64XX_GPK(10)));
	printk(KERN_INFO "GPK11[%2d]\n",gpio_get_value(S3C64XX_GPK(11)));
	printk(KERN_INFO "GPK12[%2d]\n",gpio_get_value(S3C64XX_GPK(12)));
	printk(KERN_INFO "GPK13[%2d]\n",gpio_get_value(S3C64XX_GPK(13)));
	printk(KERN_INFO "GPK14[%2d]\n",gpio_get_value(S3C64XX_GPK(14)));
	printk(KERN_INFO "GPK15[%2d]\n",gpio_get_value(S3C64XX_GPK(15)));
}
/*
 * set gpio value
 */
static void buddy_set_gpio(int value)
{
	/*
	 * Set pin as output and set value.
	 */
	s3c_gpio_cfgpin(S3C64XX_GPK(2),S3C_GPIO_OUTPUT);
	gpio_set_value(S3C64XX_GPK(2),1);
	/*
	 * Out value and pin was set as output directly.
	 */
	gpio_direction_output(S3C64XX_GPK(6),1);
}
/*
 * key value
 */
static void buddy_get_key(void)
{
	/*
	 * Get value for gpio.
	 */
	printk(KERN_INFO "Key03[%2d]\n",gpio_get_value(S3C64XX_GPN(2)));
	/*
	 * Get irq number for gpio.
	 */
	printk(KERN_INFO "Key03 irq is[%d]\n",gpio_to_irq(S3C64XX_GPN(2)));
}
/*
 * open operation
 */
static int buddy_open(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "Open device\n");
	return 0;
}
/*
 * release opertion 
 */
static int buddy_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "Close devuce\n");
	return 0;
}
/*
 * read operation
 */
static ssize_t buddy_read(struct file *filp,char __user *buffer,size_t count,
		loff_t *offset)
{
	printk(KERN_INFO "read device\n");
	/*
	 * check this gpio if can use.
	 */
	buddy_get_key();
	return 0;
}
/*
 * write operation
 */
static ssize_t buddy_write(struct file *filp,const char __user *buf,
		size_t count,loff_t *offset)
{
	printk(KERN_INFO "Write device\n");
	return 0;
}
/*
 * file_operations
 */
static struct file_operations buddy_fops = {
	.owner     = THIS_MODULE,
	.open      = buddy_open,
	.release   = buddy_release,
	.write     = buddy_write,
	.read      = buddy_read,
};
/*
 * misc struct 
 */

static struct miscdevice buddy_misc = {
	.minor    = MISC_DYNAMIC_MINOR,
	.name     = DEV_NAME,
	.fops     = &buddy_fops,
};
/*
 * Init module
 */
static __init int buddy_init(void)
{
	misc_register(&buddy_misc);
	printk("buddy_test\n");
	return 0;
}
/*
 * Exit module
 */
static __exit void buddy_exit(void)
{
	misc_deregister(&buddy_misc);
}
/*
 * module information
 */
module_init(buddy_init);
module_exit(buddy_exit);

MODULE_LICENSE("GPL");
