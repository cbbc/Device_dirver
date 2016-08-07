#include <linux/platform_device.h> 
#include <linux/miscdevice.h>

#include <linux/interrupt.h>
#include <asm/arch/map.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <asm/semaphore.h>

#include <linux/module.h>
#include <linux/fs.h>

#include "lddbus.h"

//指向系统所拥有的资源信息，此信息为公用信息
//可以被多用户共同使用
static struct resource *mini_mem;
static struct resource *mini_irq;
static void __iomem *mini_base;

static unsigned char wq_flag = 0 ; //wait queue 队列的唤醒标志

//设备的数据结构，独立于platform的数据结构，此数据结构
//为驱动开发人员所要重点考虑的
//数据为用户公用的
struct Mini_Dev 
{
	struct miscdevice mdev;
	wait_queue_head_t wq;
	struct semaphore sem;
};

struct Mini_Dev *p_mdev;



static ssize_t s3c2440mini_read(struct file * file, char __user * userbuf,
		size_t count, loff_t * off)
{
	printk ("MINI TEST ..............READ\n");
	return 0;
}

static ssize_t s3c2440mini_write(struct file *file, const char __user *data,
		size_t len, loff_t *ppos)
{
	printk ("MINI TEST ..............write\n");
	return 0;
}

#define IOCTL_MINI_WAITIRQ _IOR('M',1,int)
#define IOCTL_MINI_SENDDATA _IOR('M',2,int)

static int s3c2440mini_ioctl(struct inode *inode, struct file *file,	
		unsigned int cmd, unsigned long arg)
{
	int i;
	switch(cmd)
		{
		case IOCTL_MINI_WAITIRQ:
			wait_event_interruptible(p_mdev->wq, (wq_flag)&0x01);
			wq_flag = 0;
			break;
		case IOCTL_MINI_SENDDATA:
			for ( i = 0 ; i < 0x1000000; i ++)
				{
				writeb(0xff,mini_base);
				}
			break;
		}
	return 0;
}

static int s3c2440mini_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int s3c2440mini_release(struct inode *inode, struct file *file)
{
	printk ("MINI TEST ..............release\n");
	return 0;
}

//设备所具有的file 操作结构是驱动的工作重点
//同时也是设备功能实现的地方
static struct file_operations mini_ops = {
	.owner  	= THIS_MODULE,
	.write 	= s3c2440mini_write,
	.read 	= s3c2440mini_read,
	.ioctl		= s3c2440mini_ioctl,
	.release	= s3c2440mini_release,
	.open	= s3c2440mini_open,
};


//kernel interface


//platform 驱动数据结构，提供了探测、移除、挂起、回复和关闭的
//的系统接口，使系统设备更加的规范
//.driver 挂接着设备的数据结构和资源信息，这些信息已经提前被
//注册到系统里，只有在.name相同的情况下调用platform_driver_register才能
//注册成功

static struct ldd_device mini_device = {
	.name 	= "mini",
};

static int mini_probe (struct ldd_device * dev)
{
	printk("mini_probe %s\n",dev->name);
	lddbus_kill(dev);
	return 0;
}


static struct ldd_driver mini_driver = {
	.version = "$Revision: 1.21 $",
	.module = THIS_MODULE,
	.probe	= mini_probe,
	.driver = {
		.name = "mini",
	},
};


static int __init mini_init(void) 
{ 
	register_ldd_device(&mini_device);
	return register_ldd_driver(&mini_driver);
} 

static void __exit mini_exit(void) 
{ 
	unregister_ldd_device(&mini_device);
	return 	unregister_ldd_driver(&mini_driver);
} 

module_init(mini_init); 
module_exit(mini_exit); 

MODULE_AUTHOR("ljf");
MODULE_LICENSE("Dual BSD/GPL");
