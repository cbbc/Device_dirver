/*
 * Philips UDA1341 Audio Device Driver for S3C2410 Linux
 *
 * Copyright (C) 2002 MIZI Research, Inc.

 * history:
 * 	2004-6-18 chang function audio_set_dsp_speed() for other audio sample
 *		by threewater<threewater@up-tech.com>
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>

#include <linux/pm.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>
#include <asm/arch/cpu_s3c2410.h>


//#define DEBUG
#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif

static void init_s3c2410_iis_bus_rx(void);
static void init_s3c2410_iis_bus_tx(void);
static void init_s3c2410_iis_bus_rxtx(void);

#define DEF_VOLUME      65

/* UDA1341 Register bits */
#define UDA1341_ADDR		0x14

#define UDA1341_REG_DATA0	(UDA1341_ADDR + 0)
#define UDA1341_REG_STATUS	(UDA1341_ADDR + 2)

/* status control */
#define STAT0			(0x00)
#define STAT0_RST               (1 << 6)
#define STAT0_SC_MASK           (3 << 4)
#define STAT0_SC_512FS          (0 << 4)
#define STAT0_SC_384FS          (1 << 4)
#define STAT0_SC_256FS          (2 << 4)
#define STAT0_IF_MASK           (7 << 1)
#define STAT0_IF_I2S            (0 << 1)
#define STAT0_IF_LSB16          (1 << 1)
#define STAT0_IF_LSB18          (2 << 1)
#define STAT0_IF_LSB20          (3 << 1)
#define STAT0_IF_MSB            (4 << 1)
#define STAT0_IF_LSB16MSB       (5 << 1)
#define STAT0_IF_LSB18MSB       (6 << 1)
#define STAT0_IF_LSB20MSB       (7 << 1)
#define STAT0_DC_FILTER         (1 << 0)
#define STAT0_DC_NO_FILTER	(0 << 0)

#define STAT1			(0x80)
#define STAT1_DAC_GAIN          (1 << 6)        /* gain of DAC */
#define STAT1_ADC_GAIN          (1 << 5)        /* gain of ADC */
#define STAT1_ADC_POL           (1 << 4)        /* polarity of ADC */
#define STAT1_DAC_POL           (1 << 3)        /* polarity of DAC */
#define STAT1_DBL_SPD           (1 << 2)        /* double speed playback */
#define STAT1_ADC_ON            (1 << 1)        /* ADC powered */
#define STAT1_DAC_ON            (1 << 0)        /* DAC powered */

/* data0 direct control */
#define DATA0     		(0x00)
#define DATA0_VOLUME_MASK       (0x3f)
#define DATA0_VOLUME(x)         (x)

#define DATA1     		(0x40)
#define DATA1_BASS(x)           ((x) << 2)
#define DATA1_BASS_MASK         (15 << 2)
#define DATA1_TREBLE(x)         ((x))
#define DATA1_TREBLE_MASK       (3)

#define DATA2     		(0x80)
#define DATA2_PEAKAFTER         (0x1 << 5)
#define DATA2_DEEMP_NONE        (0x0 << 3)
#define DATA2_DEEMP_32KHz       (0x1 << 3)
#define DATA2_DEEMP_44KHz       (0x2 << 3)
#define DATA2_DEEMP_48KHz       (0x3 << 3)
#define DATA2_MUTE              (0x1 << 2)
#define DATA2_FILTER_FLAT       (0x0 << 0)
#define DATA2_FILTER_MIN        (0x1 << 0)
#define DATA2_FILTER_MAX        (0x3 << 0)
/* data0 extend control */
#define EXTADDR(n)              (0xc0 | (n))
#define EXTDATA(d)              (0xe0 | (d))

#define EXT0                    0
#define EXT0_CH1_GAIN(x)        (x)
#define EXT1                    1
#define EXT1_CH2_GAIN(x)        (x)
#define EXT2                    2
#define EXT2_MIC_GAIN_MASK      (7 << 2)
#define EXT2_MIC_GAIN(x)        ((x) << 2)
#define EXT2_MIXMODE_DOUBLEDIFF (0)
#define EXT2_MIXMODE_CH1        (1)
#define EXT2_MIXMODE_CH2        (2)
#define EXT2_MIXMODE_MIX        (3)
#define EXT4                    4
#define EXT4_AGC_ENABLE         (1 << 4)
#define EXT4_INPUT_GAIN_MASK    (3)
#define EXT4_INPUT_GAIN(x)      ((x) & 3)
#define EXT5                    5
#define EXT5_INPUT_GAIN(x)      ((x) >> 2)
#define EXT6                    6
#define EXT6_AGC_CONSTANT_MASK  (7 << 2)
#define EXT6_AGC_CONSTANT(x)    ((x) << 2)
#define EXT6_AGC_LEVEL_MASK     (3)
#define EXT6_AGC_LEVEL(x)       (x)

#if defined(CONFIG_SMDK_THREEWATER1)
#define GPIO_L3CLOCK            (GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_G9)
#define GPIO_L3DATA             (GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_G10)
#define GPIO_L3MODE             (GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_G8)
#else
#define GPIO_L3CLOCK            (GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_B4)
#define GPIO_L3DATA             (GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_B3)
#define GPIO_L3MODE             (GPIO_MODE_OUT | GPIO_PULLUP_DIS | GPIO_B2)
#endif
#define AUDIO_NAME		"UDA1341"
#define AUDIO_NAME_VERBOSE	"UDA1341 audio driver"

#define AUDIO_FMT_MASK          (AFMT_S16_LE)
#define AUDIO_FMT_DEFAULT       (AFMT_S16_LE)

#define AUDIO_CHANNELS_DEFAULT	2
#define AUDIO_RATE_DEFAULT	22050

#define AUDIO_NBFRAGS_DEFAULT	8
#define AUDIO_FRAGSIZE_DEFAULT	8192

#define S_CLOCK_FREQ	384
#define PCM_ABS(a) (a < 0 ? -a : a)

typedef struct {
	int size;		/* buffer size */
	char *start;		/* point to actual buffer */
	dma_addr_t dma_addr;	/* physical buffer address */
	struct semaphore sem;	/* down before touching the buffer */
	int master;		/* owner for buffer allocation, contain size when true */
} audio_buf_t;

typedef struct {
	audio_buf_t *buffers;	/* pointer to audio buffer structures */
	audio_buf_t *buf;	/* current buffer used by read/write */
	u_int buf_idx;		/* index for the pointer above */
	u_int fragsize;		/* fragment i.e. buffer size */
	u_int nbfrags;		/* nbr of fragments */
	dmach_t dma_ch;		/* DMA channel (channel2 for audio) */
} audio_stream_t;

static audio_stream_t output_stream;
static audio_stream_t input_stream; /* input */

#define NEXT_BUF(_s_,_b_) { \
        (_s_)->_b_##_idx++; \
        (_s_)->_b_##_idx %= (_s_)->nbfrags; \
        (_s_)->_b_ = (_s_)->buffers + (_s_)->_b_##_idx; }


static u_int audio_rate;
static int audio_channels;
static int audio_fmt;
static u_int audio_fragsize;
static u_int audio_nbfrags;


static int audio_rd_refcount;
static int audio_wr_refcount;
#define audio_active		(audio_rd_refcount | audio_wr_refcount)

static int audio_dev_dsp;
static int audio_dev_mixer;
static int audio_mix_modcnt;

static int uda1341_volume;
static u8 uda_sampling;
static int uda1341_boost;
static int mixer_igain=0x4; /* -6db*/

static void uda1341_l3_address(u8 data)
{
	int i;
	int flags;

	local_irq_save(flags);

	write_gpio_bit(GPIO_L3MODE, 0);
	write_gpio_bit(GPIO_L3DATA, 0);
	write_gpio_bit(GPIO_L3CLOCK, 1);
	udelay(1);
	
	for (i = 0; i < 8; i++) {
		if (data & 0x1) {
			write_gpio_bit(GPIO_L3CLOCK, 0);
			udelay(1);
			write_gpio_bit(GPIO_L3DATA, 1);
			udelay(1);
			write_gpio_bit(GPIO_L3CLOCK, 1);
			udelay(1);
		} else {
			write_gpio_bit(GPIO_L3CLOCK, 0);
			udelay(1);
			write_gpio_bit(GPIO_L3DATA, 0);
			udelay(1);
			write_gpio_bit(GPIO_L3CLOCK, 1);
			udelay(1);
		}
		data >>= 1;
	}

	write_gpio_bit(GPIO_L3MODE, 1);
	udelay(1);
	local_irq_restore(flags);
}

static void uda1341_l3_data(u8 data)
{
	int i;
	int flags;

	local_irq_save(flags);

	write_gpio_bit(GPIO_L3MODE, 1);
	udelay(1);

	write_gpio_bit(GPIO_L3MODE, 0);
	udelay(1);
	write_gpio_bit(GPIO_L3MODE, 1);

	for (i = 0; i < 8; i++) {
		if (data & 0x1) {
			write_gpio_bit(GPIO_L3CLOCK, 0);
			udelay(1);
			write_gpio_bit(GPIO_L3DATA, 1);
			udelay(1);
			write_gpio_bit(GPIO_L3CLOCK, 1);
			udelay(1);
		} else {
			write_gpio_bit(GPIO_L3CLOCK, 0);
			udelay(1);
			write_gpio_bit(GPIO_L3DATA, 0);
			udelay(1);
			write_gpio_bit(GPIO_L3CLOCK, 1);
			udelay(1);
		}

		data >>= 1;
	}

	write_gpio_bit(GPIO_L3MODE, 1);
	write_gpio_bit(GPIO_L3MODE, 0);
	udelay(1);
	write_gpio_bit(GPIO_L3MODE, 1);
	local_irq_restore(flags);
}

static void audio_clear_buf(audio_stream_t * s)
{
    	DPRINTK("audio_clear_buf\n");

	s3c2410_dma_flush_all(s->dma_ch);

	if (s->buffers) {
		int frag;

		for (frag = 0; frag < s->nbfrags; frag++) {
			if (!s->buffers[frag].master)
				continue;
			consistent_free(s->buffers[frag].start,
					s->buffers[frag].master,
					s->buffers[frag].dma_addr);
		}
		kfree(s->buffers);
		s->buffers = NULL;
	}

	s->buf_idx = 0;
	s->buf = NULL;
}

static int audio_setup_buf(audio_stream_t * s)
{
	int frag;
	int dmasize = 0;
	char *dmabuf = 0;
	dma_addr_t dmaphys = 0;

	if (s->buffers)
		return -EBUSY;

	s->nbfrags = audio_nbfrags;
	s->fragsize = audio_fragsize;

	s->buffers = (audio_buf_t *)
	    kmalloc(sizeof(audio_buf_t) * s->nbfrags, GFP_KERNEL);
	if (!s->buffers)
		goto err;
	memset(s->buffers, 0, sizeof(audio_buf_t) * s->nbfrags);

	for (frag = 0; frag < s->nbfrags; frag++) {
		audio_buf_t *b = &s->buffers[frag];

		if (!dmasize) {
			dmasize = (s->nbfrags - frag) * s->fragsize;
			do {
				dmabuf = consistent_alloc(GFP_KERNEL|GFP_DMA,
							  dmasize, &dmaphys);
				if (!dmabuf) 
				    	dmasize -= s->fragsize;
			} while (!dmabuf && dmasize);
			if (!dmabuf)
				goto err;
			b->master = dmasize;
		}

		b->start = dmabuf;
		b->dma_addr = dmaphys;
		sema_init(&b->sem, 1);
		DPRINTK("buf %d: start %p dma %d\n", frag, b->start, b->dma_addr);

		dmabuf += s->fragsize;
		dmaphys += s->fragsize;
		dmasize -= s->fragsize;
	}

	s->buf_idx = 0;
	s->buf = &s->buffers[0];

	return 0;

      err:
	printk(AUDIO_NAME ": unable to allocate audio memory\n ");
	audio_clear_buf(s);
	return -ENOMEM;
}

static void audio_dmaout_done_callback(void *buf_id, int size)
{
	audio_buf_t *b = (audio_buf_t *) buf_id;
	
        up(&b->sem);
	wake_up(&b->sem.wait);
}

static void audio_dmain_done_callback(void *buf_id, int size)
{
	audio_buf_t *b = (audio_buf_t *) buf_id;
	
        b->size = size;
	up(&b->sem);
	wake_up(&b->sem.wait);		
}
 /* using when write */
static int audio_sync(struct file *file)
{
	audio_stream_t *s = &output_stream;
	audio_buf_t *b = s->buf;

	DPRINTK("audio_sync\n");

	if (!s->buffers)
		return 0;

	if (b->size != 0) {
		down(&b->sem);
		s3c2410_dma_queue_buffer(s->dma_ch, (void *) b,
					 b->dma_addr, b->size, DMA_BUF_WR);
		b->size = 0;
		NEXT_BUF(s, buf);
	}

	b = s->buffers + ((s->nbfrags + s->buf_idx - 1) % s->nbfrags);
	if (down_interruptible(&b->sem))
		return -EINTR;
	up(&b->sem);

	return 0;
}

static inline int copy_from_user_mono_stereo(char *to, const char *from, int count)
{
	u_int *dst = (u_int *)to;
	const char *end = from + count;

	if (verify_area(VERIFY_READ, from, count))
		return -EFAULT;

	if ((int)from & 0x2) {
		u_int v;
		__get_user(v, (const u_short *)from); from += 2;
		*dst++ = v | (v << 16);
	}

	while (from < end-2) {
		u_int v, x, y;
		__get_user(v, (const u_int *)from); from += 4;
		x = v << 16;
		x |= x >> 16;
		y = v >> 16;
		y |= y << 16;
		*dst++ = x;
		*dst++ = y;
	}

	if (from < end) {
		u_int v;
		__get_user(v, (const u_short *)from);
		*dst = v | (v << 16);
	}

	return 0;
}


static ssize_t smdk2410_audio_write(struct file *file, const char *buffer, 
				    size_t count, loff_t * ppos)
{
	const char *buffer0 = buffer;
	audio_stream_t *s = &output_stream;
	int chunksize, ret = 0;

	DPRINTK("audio_write : start count=%d\n", count);

	switch (file->f_flags & O_ACCMODE) {
	  	case O_WRONLY:
	  	case O_RDWR:
			break;
	  	default:
		  	return -EPERM;
	}

	if (!s->buffers && audio_setup_buf(s))
		return -ENOMEM;

	count &= ~0x03;

	while (count > 0) {
		audio_buf_t *b = s->buf;

		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			if (down_trylock(&b->sem))
				break;
		} else {
			ret = -ERESTARTSYS;
			if (down_interruptible(&b->sem))
				break;
		}

		if (audio_channels == 2) {
			chunksize = s->fragsize - b->size;
			if (chunksize > count)
				chunksize = count;
			DPRINTK("write %d to %d\n", chunksize, s->buf_idx);
			if (copy_from_user(b->start + b->size, buffer, chunksize)) {
				up(&b->sem);
				return -EFAULT;
			}
			b->size += chunksize;
		} else {
			chunksize = (s->fragsize - b->size) >> 1;

			if (chunksize > count)
				chunksize = count;
			DPRINTK("write %d to %d\n", chunksize*2, s->buf_idx);
			if (copy_from_user_mono_stereo(b->start + b->size, 
				    		       buffer, chunksize)) {
				up(&b->sem);
				return -EFAULT;
			}

			b->size += chunksize*2;
		}

		buffer += chunksize;
		count -= chunksize;
		if (b->size < s->fragsize) {
			up(&b->sem);
			break;
		}

		s3c2410_dma_queue_buffer(s->dma_ch, (void *) b,
					   b->dma_addr, b->size, DMA_BUF_WR);
		b->size = 0;
		NEXT_BUF(s, buf);
	}
	if ((buffer - buffer0))
		ret = buffer - buffer0;

	DPRINTK("audio_write : end count=%d\n\n", ret);

	return ret;
}


static ssize_t smdk2410_audio_read(struct file *file, char *buffer, 
                                        size_t count, loff_t * ppos)
{
	const char *buffer0 = buffer;
        audio_stream_t *s = &input_stream; 
        int chunksize, ret = 0;

        DPRINTK("audio_read: count=%d\n", count);

        if (ppos != &file->f_pos)
                return -ESPIPE;
	
	
	if (!s->buffers) {
		int i;
 
                 if (audio_setup_buf(s))
                         return -ENOMEM;
 
                 for (i = 0; i < s->nbfrags; i++) {
                         audio_buf_t *b = s->buf; 
                         down(&b->sem); 
                         s3c2410_dma_queue_buffer(s->dma_ch, (void *) b,
                                         b->dma_addr, s->fragsize, DMA_BUF_RD);
                         NEXT_BUF(s, buf);
                 }
         }
	//mdelay(1);
	if((IISFIFOC & IISFCON_RX_EN)==0){//start RX FIFO
	  //IISFIFOC&=~(IISFCON_RX_DMA|IISFCON_RX_EN);
	  //udelay(1);
	  //IISFIFOC|=(IISFCON_RX_DMA|IISFCON_RX_EN);
	  IISFIFOC |=IISFCON_RX_EN;
	  DPRINTK("audio_read: start RX\n");
	  
	}	
		DPRINTK("audio_read: DISRCC1=%x, DIDSTC1=%x, DCON1=%x,DSTAT1=%x,DMSKTRIG1=%x, DISRC1=%x, DIDST1=%x, DCDST1=%x, DCSRC1=%x\n", DISRCC1,DIDSTC1, DCON1, DSTAT1, DMTRIG1, DISRC1, DIDST1, DCDST1, DCSRC1);

        while (count > 0) {
                audio_buf_t *b = s->buf;

                /* Wait for a buffer to become full */
                if (file->f_flags & O_NONBLOCK) {
                        ret = -EAGAIN;
                        if (down_trylock(&b->sem))
                                break;
                } else {
                        ret = -ERESTARTSYS;
                        if (down_interruptible(&b->sem))
                                break;
                }

                chunksize = b->size;
		if (chunksize > count)
                        chunksize = count;
                DPRINTK("read %d from %d\n", chunksize, s->buf_idx);
                if (copy_to_user(buffer, b->start + s->fragsize - b->size,
				       	chunksize)) {
                        up(&b->sem);
                        return -EFAULT;
                }
		
                b->size -= chunksize;

                buffer += chunksize;
                count -= chunksize;
                if (b->size > 0) {
                        up(&b->sem);
                        break;
                }

                /* Make current buffer available for DMA again */
                s3c2410_dma_queue_buffer(s->dma_ch, (void *) b,
					 b->dma_addr, s->fragsize, DMA_BUF_RD);

                NEXT_BUF(s, buf);
        }
	
        if ((buffer - buffer0))
                ret = buffer - buffer0;

     //   DPRINTK("audio_read: return=%d\n", ret);

        return ret;
}


static unsigned int smdk2410_audio_poll(struct file *file, 
					struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	int i;

	DPRINTK("audio_poll(): mode=%s\n",
		(file->f_mode & FMODE_WRITE) ? "w" : "");

	if (file->f_mode & FMODE_READ) {
                if (!input_stream.buffers && audio_setup_buf(&input_stream))
                        return -ENOMEM;
                poll_wait(file, &input_stream.buf->sem.wait, wait);

                for (i = 0; i < input_stream.nbfrags; i++) {
                        if (atomic_read(&input_stream.buffers[i].sem.count) > 0)
                                mask |= POLLIN | POLLWRNORM;
				break;
                }
        }


	if (file->f_mode & FMODE_WRITE) {
		if (!output_stream.buffers && audio_setup_buf(&output_stream))
			return -ENOMEM;
		poll_wait(file, &output_stream.buf->sem.wait, wait);

		for (i = 0; i < output_stream.nbfrags; i++) {
			if (atomic_read(&output_stream.buffers[i].sem.count) > 0)
				mask |= POLLOUT | POLLWRNORM;
				break;
		}
	}

	DPRINTK("audio_poll() returned mask of %s\n",
		(mask & POLLOUT) ? "w" : "");

	return mask;
}


static loff_t smdk2410_audio_llseek(struct file *file, loff_t offset, 
				    int origin)
{
            return -ESPIPE;
}


static int smdk2410_mixer_ioctl(struct inode *inode, struct file *file, 
                                unsigned int cmd, unsigned long arg)
{
        int ret;
        long val = 0;

	switch (cmd) {
		case SOUND_MIXER_INFO:
		{
			mixer_info info;
			strncpy(info.id, "UDA1341", sizeof(info.id));
			strncpy(info.name,"Philips UDA1341", sizeof(info.name));
			info.modify_counter = audio_mix_modcnt;
			return copy_to_user((void *)arg, &info, sizeof(info));
		}
	
		case SOUND_OLD_MIXER_INFO:
		{
			_old_mixer_info info;
			strncpy(info.id, "UDA1341", sizeof(info.id));
			strncpy(info.name,"Philips UDA1341", sizeof(info.name));
			return copy_to_user((void *)arg, &info, sizeof(info));
		}

		case SOUND_MIXER_READ_STEREODEVS:
			return put_user(0, (long *) arg);

		case SOUND_MIXER_READ_CAPS:
			val = SOUND_CAP_EXCL_INPUT;
			return put_user(val, (long *) arg);

		case SOUND_MIXER_WRITE_VOLUME:
			ret = get_user(val, (long *) arg);
			if (ret)
				return ret;
			uda1341_volume = 63 - (((val & 0xff) + 1) * 63) / 100;
			uda1341_l3_address(UDA1341_REG_DATA0);
			uda1341_l3_data(uda1341_volume);
			break;
		
		case SOUND_MIXER_READ_VOLUME:
			val = ((63 - uda1341_volume) * 100) / 63;
			val |= val << 8;
			return put_user(val, (long *) arg);
	
		case SOUND_MIXER_READ_IGAIN:
			val = ((31- mixer_igain) * 100) / 31;
			return put_user(val, (int *) arg);

		case SOUND_MIXER_WRITE_IGAIN:
			ret = get_user(val, (int *) arg);
			if (ret)
				return ret;
			mixer_igain = 31 - (val * 31 / 100);		
			/* use mixer gain channel 1*/
			uda1341_l3_address(UDA1341_REG_DATA0);
			uda1341_l3_data(EXTADDR(EXT0));
			uda1341_l3_data(EXTDATA(EXT0_CH1_GAIN(mixer_igain)));			
			break;

		default:
			DPRINTK("mixer ioctl %u unknown\n", cmd);
			return -ENOSYS;
	}			

	audio_mix_modcnt++;
	return 0;
}


static int iispsr_value(int s_bit_clock, int sample_rate)
{
        int i, prescaler = 0;
        unsigned long tmpval;
        unsigned long tmpval384;
        unsigned long tmpval384min = 0xffff;

 	tmpval384 = s3c2410_get_bus_clk(GET_PCLK) / s_bit_clock;

        for (i = 0; i < 32; i++) {
                tmpval = tmpval384/(i+1);
                if (PCM_ABS((sample_rate - tmpval)) < tmpval384min) {
                        tmpval384min = PCM_ABS((sample_rate - tmpval));
                        prescaler = i;
                }
        }

        DPRINTK("prescaler = %d\n", prescaler);

        return prescaler;
}

static long audio_set_dsp_speed(long val)
{
	switch (val) {
		case 48000:
		case 44100:
			IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 44100)) 
				| IISPSR_B(iispsr_value(S_CLOCK_FREQ, 44100)));
			break;
		case 22050:
			IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 22050)) 
				| IISPSR_B(iispsr_value(S_CLOCK_FREQ, 22050)));
			break;
		case 11025:
			IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 11025)) 
				| IISPSR_B(iispsr_value(S_CLOCK_FREQ, 11025)));
			break;
		case 8000:
			IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 8000)) 
				| IISPSR_B(iispsr_value(S_CLOCK_FREQ, 8000)));
			break;
		default:	//add by threewater
		{
			unsigned long psr;
                        psr= (IISPSR_A(iispsr_value(S_CLOCK_FREQ, val))
                                | IISPSR_B(iispsr_value(S_CLOCK_FREQ, val)));
			if(psr==0)
				return -1;
			IISPSR = psr;
		}
	}

	audio_rate = val;
	
	return audio_rate;
}

static int smdk2410_audio_ioctl(struct inode *inode, struct file *file, 
                                uint cmd, ulong arg)
{
	long val;

	switch (cmd) {
	  	case SNDCTL_DSP_SETFMT:
			get_user(val, (long *) arg);
		  	if (val & AUDIO_FMT_MASK) {
			    	audio_fmt = val;
			    	break;
		  	} else
				return -EINVAL;

	  	case SNDCTL_DSP_CHANNELS:
	  	case SNDCTL_DSP_STEREO:
		  	get_user(val, (long *) arg);
		  	if (cmd == SNDCTL_DSP_STEREO)
			  	val = val ? 2 : 1;
		  	if (val != 1 && val != 2)
			  	return -EINVAL;
		  	audio_channels = val;
		  	break;

	  	case SOUND_PCM_READ_CHANNELS:
		  	put_user(audio_channels, (long *) arg);
		 	break;

	  	case SNDCTL_DSP_SPEED:
		  	get_user(val, (long *) arg);
		  	val = audio_set_dsp_speed(val);
                        if (val < 0) 
				return -EINVAL;
		  	put_user(val, (long *) arg);
		  	break;

	  	case SOUND_PCM_READ_RATE:
		  	put_user(audio_rate, (long *) arg);
		  	break;

	  	case SNDCTL_DSP_GETFMTS:
		  	put_user(AUDIO_FMT_MASK, (long *) arg);
		  	break;

	  	case SNDCTL_DSP_GETBLKSIZE:
			if(file->f_mode & FMODE_WRITE)
		  		return put_user(audio_fragsize, (long *) arg);
			else		
				return put_user(audio_fragsize, (int *) arg);

	  	case SNDCTL_DSP_SETFRAGMENT:
		        if (file->f_mode & FMODE_WRITE) {	
		  		if (output_stream.buffers)
			  		return -EBUSY;
		  		get_user(val, (long *) arg);
		  		audio_fragsize = 1 << (val & 0xFFFF);
		  		if (audio_fragsize < 16)
			  		audio_fragsize = 16;
		  		if (audio_fragsize > 16384)
			  		audio_fragsize = 16384;
		  		audio_nbfrags = (val >> 16) & 0x7FFF;
				if (audio_nbfrags < 2)
					audio_nbfrags = 2;
		  		if (audio_nbfrags * audio_fragsize > 128 * 1024)
			  		audio_nbfrags = 128 * 1024 / audio_fragsize;
		  		if (audio_setup_buf(&output_stream))
			  		return -ENOMEM;
	
			}
			if (file->f_mode & FMODE_READ) {
				if (input_stream.buffers)
					return -EBUSY;
				get_user(val, (int *) arg);
				audio_fragsize =  1 << (val & 0xFFFF);
				if (audio_fragsize < 16)
					audio_fragsize = 16;
				if (audio_fragsize > 16384)
                                        audio_fragsize = 16384;
                                audio_nbfrags = (val >> 16) & 0x7FFF;
                                if (audio_nbfrags < 2)
                                        audio_nbfrags = 2;
                                if (audio_nbfrags * audio_fragsize > 128 * 1024)
                                        audio_nbfrags = 128 * 1024 / audio_fragsize;
                                if (audio_setup_buf(&input_stream))
                                        return -ENOMEM;

			}
		 	break;

	  	case SNDCTL_DSP_SYNC:
		  	return audio_sync(file);

	  	case SNDCTL_DSP_GETOSPACE:
		{
			audio_stream_t *s = &output_stream;
			audio_buf_info *inf = (audio_buf_info *) arg;
			int err = verify_area(VERIFY_WRITE, inf, sizeof(*inf));
			int i;
			int frags = 0, bytes = 0;

			if (err)
				return err;
			for (i = 0; i < s->nbfrags; i++) {
				if (atomic_read(&s->buffers[i].sem.count) > 0) {
					if (s->buffers[i].size == 0) frags++;
					bytes += s->fragsize - s->buffers[i].size;
				}
			}
			put_user(frags, &inf->fragments);
			put_user(s->nbfrags, &inf->fragstotal);
			put_user(s->fragsize, &inf->fragsize);
			put_user(bytes, &inf->bytes);
			break;
		}

		case SNDCTL_DSP_GETISPACE:
		{
			audio_stream_t *s = &input_stream;
			audio_buf_info *inf = (audio_buf_info *) arg;
			int err = verify_area(VERIFY_WRITE, inf, sizeof(*inf));
			int i;
			int frags = 0, bytes = 0;

			if (!(file->f_mode & FMODE_READ))
                                return -EINVAL;

			if (err)
				return err;
			for(i = 0; i < s->nbfrags; i++){
			if (atomic_read(&s->buffers[i].sem.count) > 0)
                                {
                                        if (s->buffers[i].size == s->fragsize)
                                                frags++;
                                        bytes += s->buffers[i].size;
                                }
                        }
			put_user(frags, &inf->fragments);
                        put_user(s->nbfrags, &inf->fragstotal);
                        put_user(s->fragsize, &inf->fragsize);
                        put_user(bytes, &inf->bytes);
                        break;
		}
	  	case SNDCTL_DSP_RESET:
			if (file->f_mode & FMODE_READ) {
                                audio_clear_buf(&input_stream);
                        }
                        if (file->f_mode & FMODE_WRITE) {
                                audio_clear_buf(&output_stream);
                        }
                        return 0;
		case SNDCTL_DSP_NONBLOCK:
			file->f_flags |= O_NONBLOCK;
                        return 0;
	 	case SNDCTL_DSP_POST:
	      	case SNDCTL_DSP_SUBDIVIDE:
	      	case SNDCTL_DSP_GETCAPS:
	      	case SNDCTL_DSP_GETTRIGGER:
	      	case SNDCTL_DSP_SETTRIGGER:
	      	case SNDCTL_DSP_GETIPTR:
	      	case SNDCTL_DSP_GETOPTR:
	      	case SNDCTL_DSP_MAPINBUF:
	      	case SNDCTL_DSP_MAPOUTBUF:
	      	case SNDCTL_DSP_SETSYNCRO:
	      	case SNDCTL_DSP_SETDUPLEX:
		  	return -ENOSYS;
	  	default:
		  	return smdk2410_mixer_ioctl(inode, file, cmd, arg);
	}

	return 0;
}


static int smdk2410_audio_open(struct inode *inode, struct file *file)
{
	int cold = !audio_active;

	DPRINTK("audio_open\n");

	if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
		if (audio_rd_refcount || audio_wr_refcount)
			return -EBUSY;
		audio_rd_refcount++;
	} else if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
		if (audio_wr_refcount)
			return -EBUSY;
		audio_wr_refcount++;
	} else if ((file->f_flags & O_ACCMODE) == O_RDWR) {
		if (audio_rd_refcount || audio_wr_refcount)
			return -EBUSY;
		audio_rd_refcount++;
		audio_wr_refcount++;
	} else
		return -EINVAL;

	if (cold) {
		audio_rate = AUDIO_RATE_DEFAULT;
		audio_channels = AUDIO_CHANNELS_DEFAULT;
		audio_fragsize = AUDIO_FRAGSIZE_DEFAULT;
		audio_nbfrags = AUDIO_NBFRAGS_DEFAULT;
		//by threewater 2003-12-8
                if ((file->f_mode & (FMODE_WRITE|FMODE_READ))==(FMODE_WRITE|FMODE_READ)){
                        init_s3c2410_iis_bus_rxtx();
                        audio_clear_buf(&output_stream);
			audio_clear_buf(&input_stream);
                        DPRINTK("sounddev open write and read: by threewater\n");
                }
		else if ((file->f_mode & FMODE_WRITE)){
			init_s3c2410_iis_bus_tx();
			audio_clear_buf(&output_stream);
        		DPRINTK("sounddev open write: by threewater\n");
		}
		else if ((file->f_mode & FMODE_READ)){
			init_s3c2410_iis_bus_rx();
			audio_clear_buf(&input_stream);
        		DPRINTK("sounddev open read: by threewater\n");
		}
	}

	MOD_INC_USE_COUNT;

	return 0;
}


static int smdk2410_mixer_open(struct inode *inode, struct file *file)
{
	MOD_INC_USE_COUNT;
	return 0;
}


static int smdk2410_audio_release(struct inode *inode, struct file *file)
{
	DPRINTK("audio_release\n");

	if (file->f_mode & FMODE_READ) {
		  	if (audio_rd_refcount == 1)
				audio_clear_buf(&input_stream);
			  	audio_rd_refcount = 0;
	}

	if(file->f_mode & FMODE_WRITE) {
		  	if (audio_wr_refcount == 1) {
			    	audio_sync(file);
			    	audio_clear_buf(&output_stream);
			    	audio_wr_refcount = 0;
		    	}
	  	}

	MOD_DEC_USE_COUNT;

	return 0;
}


static int smdk2410_mixer_release(struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;
	return 0;
}


static struct file_operations smdk2410_audio_fops = {
	llseek:		smdk2410_audio_llseek,
	write:		smdk2410_audio_write,
	read:		smdk2410_audio_read,
	poll:		smdk2410_audio_poll,
	ioctl:		smdk2410_audio_ioctl,
	open:		smdk2410_audio_open,
	release:	smdk2410_audio_release
};

static struct file_operations smdk2410_mixer_fops = {
	ioctl:		smdk2410_mixer_ioctl,
	open:		smdk2410_mixer_open,
	release:	smdk2410_mixer_release
};

static void init_uda1341(void)
{
	int flags;

  	uda1341_volume = 62 - ((DEF_VOLUME * 61) / 100);
	uda1341_boost = 0;
  	uda_sampling = DATA2_DEEMP_NONE;
	uda_sampling &= ~(DATA2_MUTE);
	

	local_irq_save(flags);

	write_gpio_bit(GPIO_L3MODE, 1);
	write_gpio_bit(GPIO_L3CLOCK, 1);

	local_irq_restore(flags);
	
	uda1341_l3_address(UDA1341_REG_STATUS);
        uda1341_l3_data(STAT0_SC_384FS | STAT0_IF_MSB);     // set 384 system clock, MSB
        uda1341_l3_data(STAT1 | STAT1_DAC_GAIN | STAT1_ADC_GAIN | STAT1_ADC_ON | STAT1_DAC_ON);

        uda1341_l3_address(UDA1341_REG_DATA0);
	uda1341_l3_data(DATA0 |DATA0_VOLUME(uda1341_volume));  // maximum volume
	uda1341_l3_data(DATA1 |DATA1_BASS(uda1341_boost)| DATA1_TREBLE(0));
        uda1341_l3_data(uda_sampling); /* --;;*/ 
	uda1341_l3_data(EXTADDR(EXT2));
	//uda1341_l3_data(EXTDATA(EXT2_MIC_GAIN(0x6)) | EXT2_MIXMODE_CH1);
	uda1341_l3_data(EXTDATA(EXT2_MIC_GAIN(0x6)| EXT2_MIXMODE_CH2));
	uda1341_l3_data(EXTADDR(EXT5));
	uda1341_l3_data(EXTDATA(0x7));
}

static void init_s3c2410_iis_bus_rx(void){
	
	IISCON = 0;
        IISMOD = 0;
        IISFIFOC = 0;


        /* 44 KHz , 384fs */
        IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 44100))
                | IISPSR_B(iispsr_value(S_CLOCK_FREQ, 44100)));

        IISCON = (IISCON_RX_DMA         /* Transmit DMA service request */
                |IISCON_TX_IDLE         /* Receive Channel idle */
                |IISCON_PRESCALE);      /* IIS Prescaler Enable */

        IISMOD = (IISMOD_SEL_MA         /* Master mode */
                | IISMOD_SEL_RX         /* Transmit */
                | IISMOD_CH_RIGHT       /* Low for left channel */
                | IISMOD_FMT_MSB        /* MSB-justified format */
                | IISMOD_BIT_16         /* Serial data bit/channel is 16 bit */
                | IISMOD_FREQ_384       /* Master clock freq = 384 fs */
                | IISMOD_SFREQ_32);     /* 32 fs */

        IISFIFOC = (IISFCON_RX_DMA      /* Transmit FIFO access mode: DMA */
               );       /* Transmit FIFO enable */

        IISCON |= IISCON_EN;            /* IIS enable(start) */
}

static void init_s3c2410_iis_bus_tx(void)
{
        IISCON = 0;
        IISMOD = 0;
        IISFIFOC = 0;


    	/* 44 KHz , 384fs */
	IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 44100)) 
		| IISPSR_B(iispsr_value(S_CLOCK_FREQ, 44100)));

	IISCON = (IISCON_TX_DMA		/* Transmit DMA service request */
	  	|IISCON_RX_IDLE		/* Receive Channel idle */
		|IISCON_PRESCALE);	/* IIS Prescaler Enable */

 	IISMOD = (IISMOD_SEL_MA         /* Master mode */
  		| IISMOD_SEL_TX         /* Transmit */
		| IISMOD_CH_RIGHT       /* Low for left channel */
		| IISMOD_FMT_MSB        /* MSB-justified format */
		| IISMOD_BIT_16         /* Serial data bit/channel is 16 bit */
		| IISMOD_FREQ_384       /* Master clock freq = 384 fs */
		| IISMOD_SFREQ_32);     /* 32 fs */

	IISFIFOC = (IISFCON_TX_DMA      /* Transmit FIFO access mode: DMA */
		| IISFCON_TX_EN);       /* Transmit FIFO enable */

	IISCON |= IISCON_EN;		/* IIS enable(start) */
}

//add by threewater 2003-12-8
static void init_s3c2410_iis_bus_rxtx(void)
{
        IISCON = 0;
        IISMOD = 0;
        IISFIFOC = 0;


    	/* 44 KHz , 384fs */
	IISPSR = (IISPSR_A(iispsr_value(S_CLOCK_FREQ, 44100)) 
		| IISPSR_B(iispsr_value(S_CLOCK_FREQ, 44100)));

	IISCON = (IISCON_TX_DMA		/* Transmit DMA service request */
	  	|IISCON_RX_DMA		/* Receive Channel idle */
		|IISCON_PRESCALE);	/* IIS Prescaler Enable */

 	IISMOD = (IISMOD_SEL_MA         /* Master mode */
  		| IISMOD_SEL_BOTH         /* Transmit and Receive mode*/
		| IISMOD_CH_RIGHT       /* Low for left channel */
		| IISMOD_FMT_MSB        /* MSB-justified format */
		| IISMOD_BIT_16         /* Serial data bit/channel is 16 bit */
		| IISMOD_FREQ_384       /* Master clock freq = 384 fs */
		| IISMOD_SFREQ_32);     /* 32 fs */

	IISFIFOC = (IISFCON_TX_DMA | IISFCON_RX_DMA      /* Transmit and receive FIFO : DMA */
		| IISFCON_TX_EN);       /* Transmit and receive FIFO enable */

	IISCON |= IISCON_EN;		/* IIS enable(start) */
}

static int __init audio_init_dma(audio_stream_t * s, char *desc)
{
	if(s->dma_ch == DMA_CH2)
		return s3c2410_request_dma("I2SSDO", s->dma_ch, audio_dmaout_done_callback, NULL);
	else if(s->dma_ch == DMA_CH1)
		return s3c2410_request_dma("I2SSDI", s->dma_ch, NULL ,audio_dmain_done_callback);	
	else
		return 1;
}

static int audio_clear_dma(audio_stream_t * s)
{
	s3c2410_free_dma(s->dma_ch);

	return 0;
}

int __init s3c2410_uda1341_init(void)
{
	unsigned long flags;

	local_irq_save(flags);

	/* GPB 4: L3CLOCK, OUTPUT */
	set_gpio_ctrl(GPIO_L3CLOCK);
	/* GPB 3: L3DATA, OUTPUT */
	set_gpio_ctrl(GPIO_L3DATA);
	/* GPB 2: L3MODE, OUTPUT */
	set_gpio_ctrl(GPIO_L3MODE);

	/* GPE 3: I2SSDI */
	set_gpio_ctrl(GPIO_E3 | GPIO_PULLUP_EN | GPIO_MODE_I2SSDI);
	/* GPE 0: I2SLRCK */
	set_gpio_ctrl(GPIO_E0 | GPIO_PULLUP_EN | GPIO_MODE_I2SSDI);
	/* GPE 1: I2SSCLK */
	set_gpio_ctrl(GPIO_E1 | GPIO_PULLUP_EN | GPIO_MODE_I2SSCLK);
	/* GPE 2: CDCLK */
	set_gpio_ctrl(GPIO_E2 | GPIO_PULLUP_EN | GPIO_MODE_CDCLK);
	/* GPE 4: I2SSDO */
	set_gpio_ctrl(GPIO_E4 | GPIO_PULLUP_EN | GPIO_MODE_I2SSDO);

	local_irq_restore(flags);

	init_uda1341();

	output_stream.dma_ch = DMA_CH2;

	if (audio_init_dma(&output_stream, "UDA1341 out")) {
		audio_clear_dma(&output_stream);
		printk( KERN_WARNING AUDIO_NAME_VERBOSE 
			": unable to get DMA channels\n" );
		return -EBUSY;
	}

	input_stream.dma_ch = DMA_CH1;

        if (audio_init_dma(&input_stream, "UDA1341 in")) {
                audio_clear_dma(&input_stream);
                printk( KERN_WARNING AUDIO_NAME_VERBOSE
                        ": unable to get DMA channels\n" );
                return -EBUSY;
        }
	
	audio_dev_dsp = register_sound_dsp(&smdk2410_audio_fops, -1);
	audio_dev_mixer = register_sound_mixer(&smdk2410_mixer_fops, -1);

	printk(AUDIO_NAME_VERBOSE " initialized\n");

	return 0;
}

void __exit s3c2410_uda1341_exit(void)
{
	unregister_sound_dsp(audio_dev_dsp);
	unregister_sound_mixer(audio_dev_mixer);
	audio_clear_dma(&output_stream);
	audio_clear_dma(&input_stream); /* input */
	printk(AUDIO_NAME_VERBOSE " unloaded\n");
}

module_init(s3c2410_uda1341_init);
module_exit(s3c2410_uda1341_exit);
