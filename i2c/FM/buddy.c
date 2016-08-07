#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/irq.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/math64.h>
#include <linux/miscdevice.h>
#include <linux/suspend.h>
#include <linux/atomic.h>
#include <linux/lockdep.h>

#define I2C_ADDR 0x63
#define DEV_NAME "TEA6831"
#define I2C_NAME_SIZE 20

typedef unsigned char reg_t;

struct i2c_client *my_client;

static int i2c_read(reg_t addr,char *buf,int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = my_client;
	/* Dump write */
	msg[0].addr  = client->addr;
	msg[0].flags = client->flags;
	msg[0].buf   = &addr;
	msg[0].len   = 1;
	/* read operation */
	msg[1].addr  = client->addr;
	msg[1].flags = client->flags;
	msg[1].flags |= I2C_M_RD;
	msg[1].buf   = buf;
	msg[1].len   = len;

	ret = i2c_transfer(client->adapter,msg,2);
	if(ret != 2)
	{
		printk(KERN_INFO "ERR[%s]I2C_Read\n",__FUNCTION__);
		return -EFAULT;
	}
	return 0;
}
/*
 * I2C_write
 */
static int i2c_write(reg_t addr,char *buf,int len)
{
	int ret;
	char tmp[len + 1];
	struct i2c_msg msg;
	struct i2c_client *client = my_client;

	tmp[0] = addr;
	for(ret = 1 ; ret <= len ; ret++ )
		tmp[ret] = buf[ret - 1];
	/* write initial */
	msg.addr  = client->addr;
	msg.flags = client->flags;
	msg.buf   = tmp;
	msg.len   = len + 1;

	ret = 0;
	ret = i2c_transfer(client->adapter,&msg,1);
	if(ret != 1)
	{
		printk(KERN_INFO "ERR[%s]I2C_Write\n",__FUNCTION__);
		return -EFAULT;
	}
	return 0;
}

static int TEA6831_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
		int i = 0;
		unsigned char wvalue,rvalue;
		int j;

		printk(KERN_INFO "[jeremy]:%s\n",__FUNCTION__);

		my_client = client;

		for(j = 0 ; j < 0x10 ; j++) {
			wvalue = 0xFF;
			i2c_write(j,&wvalue,1);
			i2c_read(j,&rvalue,1);
			printk(KERN_INFO "[jeremy]:rvalue = %x\n",rvalue);
		}
		

//		INIT_TEA();

		return 0;
}

static int TEA6831_remove(struct i2c_client *client)
{
		printk(KERN_INFO "[jeremy]:%s\n",__FUNCTION__);

		return 0;
};

static struct i2c_device_id TEA6831_id[] = {
		{DEV_NAME,I2C_ADDR},
		{},
};

static struct i2c_driver TEA6831_driver = {
		.driver = {
				.name = DEV_NAME,
				.owner = THIS_MODULE,
		},
		.id_table = TEA6831_id,
		.probe = TEA6831_probe,
		.remove = TEA6831_remove,
};

static __init int TEA6831_init(void)
{
		struct i2c_adapter *adap;
		struct i2c_board_info i2c_info;

		printk(KERN_INFO "[jeremy]:[%s]\n",__FUNCTION__);
		
		adap = i2c_get_adapter(0);
		
		memset(&i2c_info,0,sizeof(struct i2c_board_info));
		strlcpy(i2c_info.type,DEV_NAME,I2C_NAME_SIZE);
		i2c_info.addr= I2C_ADDR;
		
		my_client = i2c_new_device(adap,&i2c_info);
		i2c_put_adapter(adap);
		
		return i2c_add_driver(&TEA6831_driver);
};

static __exit void TEA6831_exit(void)
{
		printk(KERN_INFO "[%s]\n",__FUNCTION__);

		i2c_del_driver(&TEA6831_driver);
		i2c_unregister_device(my_client);
};


MODULE_AUTHOR("Jeremy<Jeremy.Yuan@wpi-group.com>");
MODULE_DESCRIPTION("TEA6831 driver");
MODULE_LICENSE("GPL");

module_init(TEA6831_init);
module_exit(TEA6831_exit);
