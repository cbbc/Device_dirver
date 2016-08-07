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

//ָ��ϵͳ��ӵ�е���Դ��Ϣ������ϢΪ������Ϣ
//���Ա����û���ͬʹ��
static struct resource *mini_mem;
static struct resource *mini_irq;
static void __iomem *mini_base;

static unsigned char wq_flag = 0 ; //wait queue ���еĻ��ѱ�־

//�豸�����ݽṹ��������platform�����ݽṹ�������ݽṹ
//Ϊ����������Ա��Ҫ�ص㿼�ǵ�
//����Ϊ�û����õ�
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

//�豸�����е�file �����ṹ�������Ĺ����ص�
//ͬʱҲ���豸����ʵ�ֵĵط�
static struct file_operations mini_ops = {
	.owner  	= THIS_MODULE,
	.write 	= s3c2440mini_write,
	.read 	= s3c2440mini_read,
	.ioctl		= s3c2440mini_ioctl,
	.release	= s3c2440mini_release,
	.open	= s3c2440mini_open,
};


//kernel interface


//platform �������ݽṹ���ṩ��̽�⡢�Ƴ������𡢻ظ��͹رյ�
//��ϵͳ�ӿڣ�ʹϵͳ�豸���ӵĹ淶
//.driver �ҽ����豸�����ݽṹ����Դ��Ϣ����Щ��Ϣ�Ѿ���ǰ��
//ע�ᵽϵͳ�ֻ����.name��ͬ������µ���platform_driver_register����
//ע��ɹ�

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
