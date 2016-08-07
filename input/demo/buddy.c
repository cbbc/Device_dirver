/*
 * Copyright@2015.04.22
 */
/*
 * This is a simple input device driver,we can use a Button 
 * to create a input event.
 */
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <asm/gpio.h>

static struct input_dev *button_dev;


/*
 * handler for interrupt
 */
static irqreturn_t button_interrupt(int irq,void *dummy)
{
	printk(KERN_INFO "Buddy interrupt\n");
	printk(KERN_INFO "Inb %d\n",S3C64XX_GPN(0));
	input_report_key(button_dev,BTN_0,inb(S3C64XX_GPN(0)) & 1);
	input_sync(button_dev);
	return IRQ_HANDLED;
}
/*
 * open operate
 */
static int button_open(struct input_dev *dev)
{
	printk(KERN_INFO "Input open\n");
	if(request_irq(IRQ_EINT(0),button_interrupt,IRQF_TRIGGER_FALLING,
				"button",NULL))
	{
		printk(KERN_INFO "Can't allocate irq %d\n",IRQ_EINT(0));
		return -EBUSY;
	}
	return 0;
}
/*
 * close operation
 */
static void button_close(struct input_dev *dev)
{
	printk(KERN_INFO "Input close\n");
	free_irq(IRQ_EINT(0),button_interrupt);
}
/*
 * init function
 */
static int __init button_init(void)
{
	int error;
	/*
	 * declare a new input device.
	 */
	button_dev = input_allocate_device();
	if(!button_dev)
	{
		printk(KERN_INFO "Not enough memory\n");
		error = -ENOMEM;
		goto out_input;
	}
	/*
	 * initial the input device
	 */
	button_dev->name     = "buddy_input";
	button_dev->evbit[0] = BIT_MASK(EV_KEY);
	button_dev->keybit[BIT_WORD(BTN_0)] = BIT_MASK(BTN_0);
	button_dev->open     = button_open;
	button_dev->close    = button_close;
	/*
	 * add the input device into input subsystem.
	 */
	error = input_register_device(button_dev);
	if(error)
	{
		printk(KERN_INFO "Failed to register device\n");
		goto out_register;
	}
	return 0;
out_register:
	input_free_device(button_dev);
out_input:
	free_irq(IRQ_EINT(0),button_interrupt);
	return error;
}
/*
 * Exit module
 */
static void __exit button_exit(void)
{
	input_free_device(button_dev);
	free_irq(IRQ_EINT(0),button_interrupt);
}

module_init(button_init);
module_exit(button_exit);
