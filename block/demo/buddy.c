/*
 * Copyright@2015 Buddy Zhang.
 * Sample disk driver
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/hdreg.h>
#include <linux/vmalloc.h>

static int buddy_major = 0;
/*
 * How big the driver is
 */
static int nsectors    = 25600;
static int ndevices    = 1;
static int hardsect_size = 512;
/*
 * The different request modes we can use
 */
enum {
	RM_SIMPLE  = 0,
	RM_FULL    = 1,
	RM_NOQUEUE = 2,
};
static int request_mode = RM_SIMPLE;
/*
 * Minor number and partition management.
 */
#define BUDDY_MINORS 16
#define MINOR_SHIFT  4
/*
 * We can tweak out hardware sector size,but the kernel
 * talks to us in terms of small sectors,always.
 */
#define KERNEL_SECTOR_SIZE 512
/*
 * After this much idle time,the driver will simulate 
 * a media change.
 */
#define INVALIDATE_DELAY  60*HZ
/*
 * The internal representation of our device.
 */
struct buddy_dev {
	int size;            /* Device size in sectors */
	u8 *data;            /* The data array */
	short users;         /* How many users */
	short media_change;  /* Flag a  media change */
	spinlock_t lock;     /* For mutual exclusion */
	struct request_queue *queue; /* The device request queue */
	struct gendisk *gd;  /* The gendisk struct */
	struct timer_list timer; /* For simulated media changes */
};
static struct buddy_dev *bdev = NULL;



/*
 * Handle an I/O request.
 */
static void buddy_transfer(struct buddy_dev *dev,unsigned long sector,
		unsigned long nsect,char *buffer,int write)
{
	unsigned long offset = sector * KERNEL_SECTOR_SIZE;
	unsigned long nbytes = nsect  * KERNEL_SECTOR_SIZE;
	if((offset + nbytes) > dev->size)
	{
		printk(KERN_INFO "Buddy-blk:Beyond-end write\n");
		return;
	}
	if(write)
		memcpy(dev->data + offset,buffer,nbytes);
	else
		memcpy(buffer,dev->data + offset,nbytes);
}

/*
 * The simple form of the request function.
 */
static void buddy_request(struct request_queue *q)
{
	struct request *req;
	/*
	 * Get next request from reqeust queue.
	 */
	while((req = blk_fetch_request(q)) != NULL)
	{
		struct buddy_dev *dev = req->rq_disk->private_data;
#if 0
		/*
		 * If not file system request and end request.
		 */
		if(!blk_fs_request(req))
		{
			printk(KERN_INFO "Buddy-blk:Skip non-fs request\n");
			end_request(req,0);
			continue;
		}
#endif
		buddy_transfer(dev,req->__sector,0 /* req->current_nr_sectors */,
				req->buffer,rq_data_dir(req));
		/*
		 * Finish this request.
		 */
		blk_end_request_all(req,0);
	}
}
/*
 * Transfer a single BIO
 */
static int buddy_xfer_bio(struct buddy_dev *dev,struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	sector_t sector = bio->bi_sector;
	/*
	 * Do each segment independently
	 */
	bio_for_each_segment(bvec,bio,i)
	{
		/*
		 * Alloc bio memory
		 */
		char *buffer = __bio_kmap_atomic(bio,i,KM_USER0);
		/*
		 * Transfer data
		 */
		buddy_transfer(dev,sector,bio_cur_bytes(bio) >> 9,
				buffer,bio_data_dir(bio) == WRITE);
		sector += bio_cur_bytes(bio) >> 9;
		/*
		 * Free memory
		 */
		__bio_kunmap_atomic(bio,KM_USER0);
	}
	return 0;
}
/*
 * Transfer a full request
 */
static int buddy_xfer_request(struct buddy_dev *dev,struct request *rq)
{
	struct bio *bio;
	int nsect = 0;

	__rq_for_each_bio(bio,rq)
	{
		/* deal with all bio */
		buddy_xfer_bio(dev,bio);
		/* the number of has done */
		nsect += bio->bi_size / KERNEL_SECTOR_SIZE;
	}
	return nsect;
}
/*
 * Smarter request function that "handles clustering."
 */
static void buddy_full_request(struct request_queue *q)
{
	struct request *req;
	int sectors_xferred;
	struct buddy_dev *dev = q->queuedata;

	while((req = blk_fetch_request(q)) != NULL)
	{
#if 0
		if(!blk_fs_request(req))
		{
			printk(KERN_INFO "Buddy-blk:Skip non-fs request\n");
			end_request(req,0);
			continue;
		}
#endif
		sectors_xferred = buddy_xfer_request(dev,req);
		__blk_end_request(req,0,sectors_xferred << 9);
	}
}

/*
 * The direct make request version
 */
static int buddy_make_request(struct request_queue *q,struct bio *bio)
{
	struct buddy_dev *dev = q->queuedata;
	int status;

	status = buddy_xfer_bio(dev,bio);
	bio_endio(bio,status);
	return 0;
}
/*
 * open operations
 */
static int buddy_open(struct block_device *bdev,fmode_t mode)
{
	/*
	 * Get dev via list of block.It work in file-system.
	 */
	struct buddy_dev *dev = bdev->bd_disk->private_data;
	del_timer_sync(&dev->timer);
	spin_lock(&dev->lock);
	if(!dev->users)
		check_disk_change(bdev);
	dev->users++;
	spin_unlock(&dev->lock);
	spin_unlock(&dev->lock);
	return 0;
}
/*
 * release operation
 */
static int buddy_release(struct gendisk *disk,fmode_t mode)
{
	/*
	 * Get dev via list of block device,work in file-system.
	 */
	struct buddy_dev *dev = disk->private_data;
	spin_lock(&dev->lock);
	dev->users--;
	if(!dev->users)
	{
		dev->timer.expires = jiffies + INVALIDATE_DELAY;
		add_timer(&dev->timer);
	}
	spin_unlock(&dev->lock);
	return 0;
}
/*
 * Revalidate.We don't take the lock here,for fear of deadlocking
 * with open.That needs to be reevaluated.
 */
static int buddy_revalidate(struct gendisk *gd)
{
	struct buddy_dev *dev = gd->private_data;
	return dev->media_change;
}
/*
 * The invalidata function runs out of the device timer.it sets
 * a flags to simulate the removal of the media.
 */
void buddy_invalidate(unsigned long ldev)
{
	struct buddy_dev *dev = (struct buddy_dev *)ldev;
	spin_lock(&dev->lock);
	if(dev->users || !dev->data)
		printk(KERN_INFO "Buddy-blk:timer sanity check failed\n");
	else
		dev->media_change = 1;
	spin_unlock(&dev->lock);
}
/*
 * Buddy ioctl
 */
static int buddy_ioctl(struct block_device *bdev,fmode_t mode,
		unsigned long cmd)
{
	long size;
	struct hd_geometry geo;

	switch(cmd)
	{
		/*
		 * Get geometry:since we are a virtual device,we have to make
		 * up somthing plausible.So we claim 16 sectors,four heads,
		 * and calculate the corresponding number of cylinders.We set
		 * the start of data at sector four.
		 */
		case HDIO_GETGEO:
			return 0;
	}
	return -ENOTTY;
}
/*
 *
 */
static int buddy_getgeo(struct block_device *dev,struct hd_geometry *geo)
{
	unsigned long size;
	struct buddy_dev *pdev = dev->bd_disk->private_data;
	size = pdev->size;

	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads     = 4;
	geo->sectors   = 16;
	geo->start     = 0;
	return 0;
}
/*
 * The device operations structure.
 */
static struct block_device_operations buddy_ops = {
	.owner   = THIS_MODULE,
	.open    = buddy_open,
	.release = buddy_release,
	.ioctl   = buddy_ioctl,
	.getgeo  = buddy_getgeo,
};
/*
 * Setup our internal device
 */
static void setup_device(struct buddy_dev *dev,int which)
{
	/*
	 * Get some memory
	 */
	memset(dev,0,sizeof(struct buddy_dev));
	/*
	 * Set size for disk.
	 * The size of sector often are 512 bytes.
	 */
	dev->size = nsectors * hardsect_size;
	/*
	 * Allocate virtual address
	 */
	dev->data = vmalloc(dev->size);
	if(dev->data == NULL)
	{
		printk(KERN_INFO "vmalloc failure.\n");
		return;
	}
	spin_lock_init(&dev->lock);
	/*
	 * The timer which "invalidates" the device
	 */
	switch(request_mode)
	{
		/*
		 * It,like Flash and U-disk,will not use request queue.We will 
		 * create virtual request queue,and make virtual request.
		 */
		case RM_NOQUEUE:
			/*
			 * Create a virtual request queue.
			 */
			dev->queue = blk_alloc_queue(GFP_KERNEL);
			if(dev->queue == NULL)
				goto out_free;
			/*
			 * Make virtual request.
			 */
			blk_queue_make_request(dev->queue,buddy_make_request);
			break;
		/*
		 * It,like mechine disk,will use reqeust queue to deal I/O request,
		 * We will create and initial a request queue to disk.
		 */
		case RM_FULL:
			dev->queue = blk_init_queue(buddy_full_request,&dev->lock);
			/*
			 * Check return value,it may be failed.
			 */
			if(dev->queue == NULL)
				goto out_free;
			break;
		/*
		 * It will use a request queue in default.
		 */
		default:
			printk(KERN_INFO "Buddy-blk:Bad request mode %d,using simple\n",
					request_mode);
		/*
		 * The simple request queue is different from FULL,they use different
		 * I/O scheduler.
		 */
		case RM_SIMPLE:
			dev->queue = blk_init_queue(buddy_request,&dev->lock);
			if(dev->queue == NULL)
				goto out_free;
			break;
	}
	/*
	 * Set size for sector in request queue.
	 */
	blk_queue_logical_block_size(dev->queue,hardsect_size);
	/*
	 * Add private data.
	 */
	dev->queue->queuedata = dev;
	/*
	 * And the gendisk structure.
	 */
	dev->gd = alloc_disk(BUDDY_MINORS);
	if(!dev->gd)
	{
		printk(KERN_INFO "alloc_disk failure\n");
		goto out_free;
	}
	dev->gd->major = buddy_major; /* The major of disk */
	dev->gd->first_minor = which*BUDDY_MINORS; /* The disk of minor */
	dev->gd->fops = &buddy_ops; /* block device operation */
	dev->gd->queue = dev->queue; /* block request queue */
	dev->gd->private_data = dev; /* private data */
	snprintf(dev->gd->disk_name,32,"Buddy%c",which + 'a'); /* The block name */
	/*
	 * Set number of sector in disk.
	 */
	set_capacity(dev->gd,nsectors*(hardsect_size / KERNEL_SECTOR_SIZE));
	/*
	 * Add disk into system,and it will work.
	 */
	add_disk(dev->gd);
	return;

out_free:
	if(dev->data)
		vfree(dev->data);
}

/* 
 * Buddy init
 */
static int __init buddy_init(void)
{
	int i;
	/*
	 * Get register.
	 */
	buddy_major = register_blkdev(buddy_major,"buddy-blk");
	if(buddy_major <= 0) 
	{
		printk(KERN_INFO "Buddy-blk:Unable to get major number.\n");
		return -EBUSY;
	}
	else
		printk(KERN_INFO "Buddy-blk:Succeed to get major[%d]\n",buddy_major);
	/*
	 * Allocate the device array,and initialize each one.
	 */
	bdev = kmalloc(ndevices * sizeof(struct buddy_dev),GFP_KERNEL);
	if(bdev == NULL)
		goto out_unregister;
	for(i=0;i<ndevices;i++)
		/*
		 * Initial requeset queue and gendisk.
		 */
		setup_device(bdev+i,i);

	return 0;
out_unregister:
	unregister_blkdev(buddy_major,"buddy-blk");
	return -ENOMEM;
}
/*
 * Buddy exit
 */
static void __exit buddy_exit(void)
{
	int i;
	for(i = 0;i<ndevices;i++)
	{
		struct buddy_dev *dev = bdev + i;
		del_timer_sync(&dev->timer);
		if(dev->gd)
		{
			del_gendisk(dev->gd);
			put_disk(dev->gd);
		}
		if(dev->queue)
		{
			if(request_mode == RM_NOQUEUE)
				kobject_put(&(dev->queue)->kobj);
			else
				blk_cleanup_queue(dev->queue);
		}
		if(dev->data)
			vfree(dev->data);
	}
	unregister_blkdev(buddy_major,"buddy-blk");
	kfree(bdev);
}

module_init(buddy_init);
module_exit(buddy_exit);
