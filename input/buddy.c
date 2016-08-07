#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/input.h>

static struct input_dev *button_devp;
static char *name = "s3c6410_buttons";
/*
 * irq handler
 */
static irqreturn_t button_interrupt(int irq,void *dev_id)
{
	int report = 0;
	printk(KERN_INFO "This is button irq handle\n");
	input_report_key(button_devp,BTN_0,report);
	input_sync(button_devp);
	return IRQ_HANDLED;
}
/*
 * init 
 */
static int __init buddy_init(void)
{
	int ret;
	struct input_dev *input_dev;
	/*
	 * alloc input device
	 */
	input_dev = input_allocate_device();
	if(!input_dev)
		printk(KERN_INFO "Unable to alloc the input device.\n");
	button_devp = input_dev;
	button_devp->name = name;
	/*
	 * request irq for button.
	 */
	ret = request_irq(IRQ_EINT(0),button_interrupt,IRQF_TRIGGER_RISING,
			"buddy_irq",NULL);
	if(ret)
		printk(KERN_INFO "Unable request irq\n");
	/*
	 * fill input struct
	 */
	set_bit(EV_KEY,button_devp->evbit);
	set_bit(BTN_0, button_devp->keybit);
	set_bit(BTN_1, button_devp->keybit);
	set_bit(BTN_2, button_devp->keybit);
	set_bit(BTN_3, button_devp->keybit);
	set_bit(BTN_4, button_devp->keybit);
	set_bit(BTN_5, button_devp->keybit);
	set_bit(BTN_6, button_devp->keybit);
	set_bit(BTN_7, button_devp->keybit);
	/*
	 * register input device
	 */
	ret = input_register_device(button_devp);
	if(ret < 0)
		printk(KERN_INFO "Unable to register device into input-system\n");
	printk(KERN_INFO "Succeed to init input device\n");
	return ret;
}
/*
 * exit module
 */
static void __exit buddy_exit(void)
{
	free_irq(IRQ_EINT(0),NULL);
	input_unregister_device(button_devp);
}
module_init(buddy_init);
module_exit(buddy_exit);

MODULE_LICENSE("GPL");
