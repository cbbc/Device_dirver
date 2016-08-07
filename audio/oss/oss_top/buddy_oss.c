/*
 * Copyright@2014 Buddy Zhangrenze
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/shed.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/pm.h>
#include <linux/types.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-iis.h>
#include <asm/hardware/clock.h>
#include <asm/arch/regs-clock.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
#include <asm/arch/hardware.h>
#include <asm/arch/map.h>
#include <asm/arch/S3C2410.h>
#define PFX "s3c2410-uda1341-superlp: "

#define MAX_DMA_CHANNELS 0




/*
 * buddy_mixer_open
 */
static int buddy_mixer_open(struct inode *inode,struct file *filp)
{
	return 0;
}
/*
 * buddy_mixer_release
 */
static int buddy_mixer_release(struct inode *inode,struct file *filp)
{
	return 0;
}
/*
 * buddy_mixer_ioctl
 */
static int buddy_mixer_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg)
{
	int ret;
	long val = 0;
	switch(cmd) 
	{
		case SOUND_MIXER_INFO:
			{
				mixer_info info;
				strncpy(info.id,"UDA1341",sizeof(info.id));
				strncpy(info.name,"Philips UDA1341",sizeof(info.name));
				info.modify_counter = audio_mix_modcnt;
				return copy_to_user((void *)arg,&info,sizeof(info));
			}
		case SOUND_OLD_MIXER_INFO:
			{
				_old_mixer_info info;
				strncpy(info.id,"UDA1341",sizeof(info.id));
				strncpy(info.name,"Philips UDA1341",sizeof(info.name));
				return copy_to_user((void *)arg,&info,sizeof(info));
			}
		case SOUND_MIXER_READ_STEREODEVS:
			return put_user(0,(long *)arg);
		case SOUND_MIXER_READ_CAPS:
			{
				val = SOUND_CAP_EXCL_INPUT;
				return put_user(val,(long *)arg);
			}
		case SOUND_MIXER_WRITE_VOLUME:
			{
				ret = get_user(val,(long *)arg);
				if(ret)
					return ret;
				uda1341_volume = 63 - (((val & 0xff) + 1) * 63) / 100;
				uda1341_l3_address(UDA1341_REG_DATA0);
				uda1341_l3_data(uda1341_volume);
			}
		case SOUND_MIXER_READ_VOLUME:
			{
				val = ((63 - uda1341_volume) * 100) / 63;
				val |= val << 8;
				return put_user(val,(long *)arg);
			}
		case SOUND_MIXER_READ_IGAIN:
			{
				val = ((31 - mixer_igain) * 100) /31;
				return put_user(val,(int *)arg);
			}
		case SOUND_MIXER_WRITE_IGAIN:
			{
				ret = get_user(val,(int *)arg);
				if(ret)
					return ret;
				mixer_igain = 31 - (val * 31 / 100);
				/*
				 * use mixer gain channel 1
				 */
				uda1341_l3_address(UDA1341_REG_DATA0);
				uda1341_l3_data(EXTADDR(EXT0));
				uda1341_l3_data(EXTDATA(EXT0_CH1_GAIN(mixer_igain)));
				break;
			}
		default:
			return -ENOSYS;
	}
	return 0;
}
/*
 * dsp
 */
static struct file_operations buddy_dsp_fops = {
	.llseek     = buddy_dsp_llseek,
	.write      = buddy_dsp_write,
	.read       = buddy_dsp_read,
	.poll       = buddy_dsp_poll,
	.open       = buddy_dsp_open,
	.release    = buddy_dsp_release,
};
/*
 * mixer
 */
static struct file_operations buddy_mixer_fops = {
	.ioctl      = buddy_mixer_ioctl,
	.open       = buddy_mixer_open,
	.release    = buddy_release,
};
/*
 * probe function
 */
static int buddy_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;
	unsigned long flags;
	/*
	 * get resource
	 */
	res = platform_get_resource(pdev,IORESOURCE_MEM,0);
	/*
	 * iis base address
	 */
	iis_base = (void *)S3C2410_VA_IIS;
	/*
	 * get clock
	 */
	iis_clock = clk_get(dev,"iis");
	clk_use(iis_clock);
	/*
	 * setup gpio
	 */
	
	/*
	 * init iis bus
	 */

	/*
	 * init uda1341
	 */

	/*
	 * inti dma
	 */
	
	/*
	 * register dsp
	 */
	audio_dev_dsp = register_sound_dsp(&buddy_dsp_fops,-1);
	/*
	 * register mixer
	 */
	audio_dev_mixer = register_sound_mixer(&buddy_mixer_fops,-1);
	return 0;
}
/*
 * driver remove
 */
static int buddy_remove(struct device *dev)
{
	unregister_sound_mixer(audio_dev_mixer);
	unregister_sound_dsp(audio_dev_dsp);
	/*
	 * remove some resource
	 */
	return 0;
}
/*
 * driver struct 
 */
static struct device_driver buddy_driver = {
	.name    = "s3c2410-iis",
	.bus     = &platform_bus_type,
	.probe   = buddy_probe,
	.remove  = buddy_remove,
};
/*
 * init module
 */
static __init int oss_init(void)
{
	return driver_register(&buddy_driver);
}
/*
 * exit module
 */
static __exit void oss_exit(void)
{
	driver_unregister(&buddy_driver);
}
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Buddy <514981221@qq.com>");
