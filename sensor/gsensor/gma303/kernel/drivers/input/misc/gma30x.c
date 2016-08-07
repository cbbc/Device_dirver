/**
 * @file drivers/misc/gma30x.c
 * @brief GMA302/GMA303 g-sensor Linux device driver
 * @author Globalmems Technology Co., Ltd
 * @version 1.2
 * @date 2014/02/12
 *
 * Copyright (c) 2014 Globalmems, Inc.  All rights reserved.
 *
 * This source is subject to the Globalmems Software License.
 * This software is protected by Copyright and the information and source code
 * contained herein is confidential. The software including the source code
 * may not be copied and the information contained herein may not be used or
 * disclosed except with the written permission of Globalmems Inc.
 * All other rights reserved.
 *
 * This code and information are provided "as is" without warranty of any
 * kind, either expressed or implied, including but not limited to the
 * implied warranties of merchantability and/or fitness for a
 * particular purpose.
 *
 * The following software/firmware and/or related documentation ("Globalmems Software")
 * have been modified by Globalmems Inc. All revisions are subject to any receiver's
 * applicable license agreements with Globalmems Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 //#define VERBOSE_DEBUG
#define DEBUG	/**< if define : Enable gma->client->dev debug data .*/
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/average.h>
#include <linux/input/gma30x.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
	#include <linux/earlysuspend.h>
#endif

#define GMA_PERMISSION_THREAD
#include <linux/kthread.h>
#include <linux/syscalls.h>


struct gma_data {
  	struct class 			*class;
  	struct input_dev 		*input;
	struct i2c_client 		*client;
	struct delayed_work 	work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend 	early_suspend;
#endif	
	struct mutex 	sensor_mutex;
	int8_t			sense_data[SENSOR_DATA_SIZE];
	struct mutex 	accel_mutex;
	raw_data 		accel_data;	///< ACC Data
	raw_data 		offset;		///< Offset
	
	wait_queue_head_t	drdy_wq;
	wait_queue_head_t	open_wq;
	
	atomic_t	active;
	atomic_t 	calib;	///< 1:do calibration gsensor
	atomic_t 	delay;	///< HAL delay
	atomic_t 	enable;	///< HAL 1:enable	0:disable
	atomic_t	addr;	///< register address
	atomic_t	drdy;
	atomic_t	position;///< must int type ,for Kconfig setup
	int	irq;
	int	rstn;
#ifdef SMA_FILTER
	int	sma_filter;		///< Set AVG sensor data :range(1~16)
	int	sum[SENSOR_DATA_SIZE];	///< SMA Filter sum
	int	bufferave[3][16];
#endif
#ifdef EWMA_FILTER
	int	ewma_filter[SENSOR_DATA_SIZE];	///< Set EWMA WEIGHT :value(2,4,8,16)
#endif
#ifdef GMA_DEBUG_DATA
	struct mutex 	suspend_mutex;
	atomic_t		suspend;
#endif
	struct list_head devfile_list;
};
static struct gma_data *s_gma;

#ifdef EWMA_FILTER
struct ewma average[SENSOR_DATA_SIZE];
#endif
static unsigned int interval;
static int GMA_WriteCalibration(struct gma_data *gma, char*);
void GMA_ReadCalibration(struct gma_data *gma);

char DIR_SENSOR[] = "/data/misc/sensor";				///< Need Create sensor folder
char GMA_Offset_TXT[] = "/data/misc/sensor/offset.txt";	///< SAVE FILE PATH offset.txt
char GMA_Offset_TXT_PATH2[] = "/data/misc/gsensor_offset.txt";	///< SAVE FILE PATH2 offset.txt because Level APP no write premission
char SH_GSS[] = "system/bin/gss.sh";					///< shell script PATH
char EXEC_GMAD[] = "system/bin/gmad";					///< Execute file PATH
#ifdef GMA302
char CHAR_DEV[] = "/dev/gma302";						///< Character device PATH
#else
char CHAR_DEV[] = "/dev/gma303";						///< Character device PATH
#endif
static int gma_acc_init(void);
static void gma_acc_exit(void);

static int gma_open(struct inode*, struct file*);
static long gma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int gma_close(struct inode*, struct file*);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sensor_suspend(struct early_suspend *h);
static void sensor_resume(struct early_suspend *h);
#else
static int gma_acc_suspend(struct i2c_client *, pm_message_t);
static int gma_acc_resume(struct i2c_client *);
#endif
static int  gma_acc_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int  gma_acc_remove(struct i2c_client *client);
static int gma_i2c_rxdata(struct i2c_client *client, unsigned char *rxDat, int length);
static int gma_i2c_txdata(struct i2c_client *client, unsigned char *txData, int length);

static int gma_acc_measure(struct gma_data *gma, int *xyz);
static int gma_set_position(struct gma_data *gma, int);
static int gma_acc_calibration(struct gma_data *gma, int);
/*
static void gma_acc_set_data(struct gma_data *gma)
{
	s_gma = gma;
}
*/
static int gma_acc_input_init(struct gma_data *gma);
static void gma_acc_input_fini(struct gma_data *gma);

/* This function will block a process until the latest measurement data is available. */
static int GMA_IO_GetData( struct gma_data *gma, 
							char *rbuf, 
							int size)
{
	int err;
	err = wait_event_interruptible_timeout( gma->drdy_wq, 
								atomic_read(&gma->drdy), 
								GMA301_DRDY_TIMEOUT);

	if (err < 0) {
		dev_err(&gma->client->dev,"%s: wait_event failed (%d).", __func__, err);
		return -1;
	}
	if (!atomic_read(&gma->drdy)) {
		dev_err(&gma->client->dev,"%s: DRDY is not set.", __func__);
		return -1;
	}

	mutex_lock(&gma->sensor_mutex);
	memcpy(rbuf, gma->sense_data, size);
	atomic_set(&gma->drdy, 0);
	mutex_unlock(&gma->sensor_mutex);

	return 0;
}

static int GmaGetOpenStatus(struct gma_data *gma){
	dev_dbg(&gma->client->dev, "start active=%d , &gma->active=%d\n",
					gma->active.counter, atomic_read(&gma->active));
	return wait_event_interruptible(gma->open_wq, (atomic_read(&gma->active) != 0));
}

static int GmaGetCloseStatus(struct gma_data *gma){
	dev_dbg(&gma->client->dev, "start active=%d , gma->active=%d\n",
					gma->active.counter, atomic_read(&gma->active));
	return wait_event_interruptible(gma->open_wq, (atomic_read(&gma->active) <= 0));
}

static void gma_sysfs_update_active_status(struct gma_data *gma , 
											int enable)
{
	unsigned long delay;
	if(enable){
		delay=msecs_to_jiffies(atomic_read(&gma->delay));
		if(delay < 1)
			delay = 1;

		dev_info(&gma->client->dev, "schedule_delayed_work start with delay time=%lu\n", delay);
		schedule_delayed_work(&gma->work, delay);
	}
	else
		cancel_delayed_work_sync(&gma->work);
}

static bool get_value_as_int(char const *buf, 
								size_t size, 
								int *value)
{
	long tmp;
	if (size == 0)
		return false;
	/* maybe text format value */
	if ((buf[0] == '0') && (size > 1)) {
		if ((buf[1] == 'x') || (buf[1] == 'X')) {
			/* hexadecimal format */
			if (0 != strict_strtol(buf, 16, &tmp))
				return false;
		} else {
			/* octal format */
			if (0 != strict_strtol(buf, 8, &tmp))
				return false;
		}
	} else {
		/* decimal format */
		if (0 != strict_strtol(buf, 10, &tmp))
			return false;
	}

	if (tmp > INT_MAX)
		return false;

	*value = tmp;
	return true;
}
static bool get_value_as_int64(char const *buf, 
								size_t size, 
								long long *value)
{
	long long tmp;
	if (size == 0)
		return false;
	/* maybe text format value */
	if ((buf[0] == '0') && (size > 1)) {
		if ((buf[1] == 'x') || (buf[1] == 'X')) {
			/* hexadecimal format */
			if (0 != strict_strtoll(buf, 16, &tmp))
				return false;
		} else {
			/* octal format */
			if (0 != strict_strtoll(buf, 8, &tmp))
				return false;
		}
	} else {
		/* decimal format */
		if (0 != strict_strtoll(buf, 10, &tmp))
			return false;
	}

	if (tmp > LLONG_MAX)
		return false;

	*value = tmp;
	return true;
}

/* sysfs enable show & store */
static ssize_t gma_sysfs_enable_show( struct gma_data *gma, 
										char *buf, 
										int pos)
{
	char str[2][16]={"ACC enable OFF","ACC enable ON"};
	int flag;
	flag=atomic_read(&gma->enable);
	return sprintf(buf, "%s\n", str[flag]);
}

static ssize_t gma_sysfs_enable_store( struct gma_data *gma, 
									char const *buf, 
									size_t count, 
									int pos)
{
	int en = 0;
	if (NULL == buf)
		return -EINVAL;
	dev_dbg(&gma->client->dev, "buf=%x %x\n", buf[0], buf[1]);
	if (0 == count)
		return 0;

	if (false == get_value_as_int(buf, count, &en))
		return -EINVAL;

	en = en ? 1 : 0;

	atomic_set(&gma->enable,en);
	gma_sysfs_update_active_status(gma , en);
	return count;
}

static ssize_t gma_enable_show( struct device *dev, 
							struct device_attribute *attr, 
							char *buf)
{
	return gma_sysfs_enable_show( dev_get_drvdata(dev), buf, ACC_DATA_FLAG);
}

static ssize_t gma_enable_store( struct device *dev, 
								struct device_attribute *attr, 
								char const *buf, size_t count)
{
	return gma_sysfs_enable_store( dev_get_drvdata(dev), buf, count, ACC_DATA_FLAG);
}

/* sysfs delay show & store*/
static ssize_t gma_sysfs_delay_show( struct gma_data *gma, 
										char *buf,
										int pos)
{
	return sprintf(buf, "%d\n", atomic_read(&gma->delay));
}

static ssize_t gma_sysfs_delay_store( struct gma_data *gma,
										char const *buf,
										size_t count,
										int pos)
{
	long long val = 0;
	
	if (NULL == buf)
		return -EINVAL;

	if (0 == count)
		return 0;
	
	if (false == get_value_as_int64(buf, count, &val))
		return -EINVAL;
	
	atomic_set(&gma->delay, (unsigned int) val);

	return count;
}

static ssize_t gma_delay_show( struct device *dev,
							struct device_attribute *attr,
							char *buf)
{
	return gma_sysfs_delay_show( dev_get_drvdata(dev), buf, ACC_DATA_FLAG);
}

static ssize_t gma_delay_store( struct device *dev,
								struct device_attribute *attr,
								char const *buf,
								size_t count)
{
	return gma_sysfs_delay_store( dev_get_drvdata(dev), buf, count, ACC_DATA_FLAG);
}

/* sysfs calibration show & store*/
static ssize_t gma_calib_show( struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	return sprintf(buf, "%d\n", atomic_read(&gma->calib));
}

static ssize_t gma_calib_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	int i ;
	unsigned long side;
	int ret;

	ret = strict_strtoul(buf, 10, &side);
    if (!((side >= 1) && (side <= 9)))
		return -1;
	gma_acc_calibration(gma, side);
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		dev_info(&gma->client->dev, "side =%ld, offset.v[%d]=%d : %d\n", side , i, gma->offset.v[i], __LINE__);
	/* save file */
	GMA_WriteCalibration(gma, GMA_Offset_TXT);
	GMA_WriteCalibration(gma, GMA_Offset_TXT_PATH2);
	return count;
}

/* sysfs SMA show & store */
#ifdef SMA_FILTER
static ssize_t gma_sma_show( struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);

	return sprintf(buf, "%d\n", gma->sma_filter);
}

static ssize_t gma_sma_store( struct device *dev,
							struct device_attribute *attr,
							const char *buf,
							size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	unsigned long filter;

	if (strict_strtoul(buf, 10, &filter) < 0)
		return count;

	if (!((filter >= 1) && (filter <= 16)))
		return -1;
	gma->sma_filter = filter;
	return count;
}
#endif
/* sysfs EWMA show & store */
#ifdef EWMA_FILTER
static ssize_t gma_ewma_show( struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	return sprintf(buf, "%d %d %d\n", gma->ewma_filter[0], gma->ewma_filter[1], gma->ewma_filter[2]);
}

static ssize_t gma_ewma_store( struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);

	sscanf(buf, "%d %d %d", (int *)&gma->ewma_filter[0], (int *)&gma->ewma_filter[1], (int *)&gma->ewma_filter[2]);
	if ((gma->ewma_filter[0] == 2) || (gma->ewma_filter[0] == 4) || (gma->ewma_filter[0] == 8) || (gma->ewma_filter[0] == 16))
	{
		ewma_init(&average[0], EWMA_FACTOR, gma->ewma_filter[0]);
		dev_info(&gma->client->dev, "ewma_init: gma->ewma_filter[0]=%d \n", gma->ewma_filter[0]);
	}
	if ((gma->ewma_filter[1] == 2) || (gma->ewma_filter[1] == 4) || (gma->ewma_filter[1] == 8) || (gma->ewma_filter[1] == 16))
	{
		ewma_init(&average[1], EWMA_FACTOR, gma->ewma_filter[1]);
		dev_info(&gma->client->dev, "ewma_init: gma->ewma_filter[1]=%d \n", gma->ewma_filter[1]);
	}
	if ((gma->ewma_filter[2] == 2) || (gma->ewma_filter[2] == 4) || (gma->ewma_filter[2] == 8) || (gma->ewma_filter[2] == 16))
	{
		ewma_init(&average[2], EWMA_FACTOR, gma->ewma_filter[2]);
		dev_info(&gma->client->dev, "ewma_init: gma->ewma_filter[2]=%d \n", gma->ewma_filter[2]);
	}
	return count;
}
#endif
/* sysfs position show & store */
static ssize_t gma_position_show( struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
    int data;
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);

    data = atomic_read(&(gma->position));

    return sprintf(buf, "%d\n", data);
}

static ssize_t gma_position_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	unsigned long position;
	int ret;

	ret = strict_strtol(buf, 10, &position);
	if (ret < 0)
		return count;
	dev_dbg(&gma->client->dev, "position=%ld :%d\n", position, __LINE__);
	gma_set_position(gma, position);
	return count;
}
/* sysfs offset show & store */
static ssize_t gma_offset_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	return sprintf(buf, "%d %d %d\n", 
				gma->offset.u.x, 
				gma->offset.u.y, 
				gma->offset.u.z);
}

static ssize_t gma_offset_store( struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	sscanf(buf, "%d %d %d", (int *)&gma->offset.v[0], (int *)&gma->offset.v[1], (int *)&gma->offset.v[2]);
	GMA_WriteCalibration(gma, GMA_Offset_TXT);
	GMA_WriteCalibration(gma, GMA_Offset_TXT_PATH2);
	return count;
}

/* sysfs data show */
static ssize_t gma_acc_private_data_show( struct device *dev,
								struct device_attribute *attr,
								char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	raw_data accel;

	mutex_lock(&gma->accel_mutex);
	accel = gma->accel_data;
	mutex_unlock(&gma->accel_mutex);

	return sprintf(buf, "%d %d %d\n", gma->accel_data.v[0], gma->accel_data.v[1], gma->accel_data.v[2]);
}

static ssize_t gma_register_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	int address, value;
	unsigned char buffer[2];
	
    sscanf(buf, "[0x%x]=0x%x", &address, &value);
    
	buffer[0] = address;
	buffer[1] = value;
	gma_i2c_txdata(gma->client, buffer, 2);

    return count;
}

static ssize_t gma_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
    size_t count = 0;
    u8 reg[0x10];
    int i;
    
    // read reg: 0x00 0x01
	reg[0] = GMA1302_REG_PID;
	gma_i2c_rxdata(gma->client, reg, 1);
	count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_PID, reg[0]);
	// read reg: 0x01
	reg[0] = GMA1302_REG_PD;
	gma_i2c_rxdata(gma->client, reg, 1);
	count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_PD, reg[0]);
	// read reg: 0x02
	reg[0] = GMA1302_REG_ACTR;
	gma_i2c_rxdata(gma->client, reg, 1);
	count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_ACTR, reg[0]);
	// read reg: 0x03
	reg[0] = GMA1302_REG_MTHR;
	gma_i2c_rxdata(gma->client, reg, 1);
	count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_MTHR, reg[0]);
    // read reg:  0x04 [0x0055]
	reg[0] = GMA1302_REG_STADR;
    gma_i2c_rxdata(gma->client, reg, 1);
    count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_STADR, reg[0]);
    // read reg: 0x05 ~ 0x0c
	reg[0] = GMA1302_REG_STATUS;
    gma_i2c_rxdata(gma->client, reg, 9);
    for (i = 0 ; i < 9; i++)
        count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_STATUS+i, reg[i]);
	// read reg: 0x15 ~ 0x17
	reg[0] = GMA1302_REG_INTCR;
    gma_i2c_rxdata(gma->client, reg, 3);
    for (i = 0 ; i < 3; i++)
        count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_INTCR+i, reg[i]);
	// read reg: 0x18
	reg[0] = GMA1302_REG_CONTR3;
    gma_i2c_rxdata(gma->client, reg, 1);
	count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_CONTR3, reg[0]);
	// read reg: 0x38
	reg[0] = GMA1302_REG_OSM;
    gma_i2c_rxdata(gma->client, reg, 1);
	count += sprintf(&buf[count], "0x%02x: 0x%02x\n", GMA1302_REG_OSM, reg[0]);
    return count;
}

/* sysfs id show */
static ssize_t gma_chipinfo_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	char str[8]={GSENSOR_ID};
	return sprintf(buf, "%s\n", str);
}
/* sysfs debug_suspend show & store */
#ifdef GMA_DEBUG_DATA
static ssize_t gma_debug_suspend_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	unsigned long suspend;//gma->suspend;
	suspend = atomic_read(&gma->suspend);
	
	mutex_lock(&gma->suspend_mutex);
	suspend = sprintf(buf, "%ld\n", suspend);
	mutex_unlock(&gma->suspend_mutex);

	return suspend;
}

static ssize_t gma_debug_suspend_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct gma_data *gma = input_get_drvdata(input);
	//struct i2c_client *client = gma->client;
	unsigned long suspend;
	pm_message_t msg;
	int ret;
	ret = strict_strtol(buf, 10, &suspend);
	if (ret < 0)
		return count;

	memset(&msg, 0, sizeof(pm_message_t));

	mutex_lock(&gma->suspend_mutex);
	if (suspend) {
		gma_acc_suspend(gma->client, msg);
		atomic_set(&gma->suspend,suspend);
	} else {
		gma_acc_resume(gma->client);
		atomic_set(&gma->suspend,suspend);
	}
	mutex_unlock(&gma->suspend_mutex);

	return count;
}
#endif /* DEBUG */
/* sysfs reg_read show & store */
static ssize_t gma_reg_read_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct gma_data *gma = dev_get_drvdata(dev);
	int err;
	unsigned char i2c[6];

	i2c[0] = (unsigned char)atomic_read(&gma->addr);
	err = gma_i2c_rxdata(gma->client, i2c, 6);
	if (err < 0)
		return err;

	return sprintf(buf, "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
						i2c[0], i2c[1], i2c[2], i2c[3], i2c[4], i2c[5]);
}

static ssize_t gma_reg_read_store(
	struct device *dev,
	struct device_attribute *attr,
	char const *buf,
	size_t count)
{
	struct gma_data *gma = dev_get_drvdata(dev);
	int addr = 0;

	if (NULL == buf)
		return -EINVAL;
	
	if (0 == count)
		return 0;

	if (false == get_value_as_int(buf, count, &addr))
		return -EINVAL;
	dev_info(&gma->client->dev, "addr=%d  \n", addr);
	if (addr < 0 || 128 < addr)
		return -EINVAL;

	atomic_set(&gma->addr, addr);

	return count;
}
/* sysfs reg_write show & store */
static ssize_t gma_reg_write_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct gma_data *gma = dev_get_drvdata(dev);
	int err;
	unsigned char i2c[6];

	i2c[0] = (unsigned char)atomic_read(&gma->addr);
	err = gma_i2c_rxdata(gma->client, i2c, 6);
	if (err < 0)
		return err;

	return sprintf(buf, "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
						i2c[0], i2c[1], i2c[2], i2c[3], i2c[4], i2c[5]);
}

static ssize_t gma_reg_write_store(
	struct device *dev,
	struct device_attribute *attr,
	char const *buf,
	size_t count)
{
	struct gma_data *gma = dev_get_drvdata(dev);
	int value = 0;
	unsigned char i2c[2];
	if (NULL == buf)
		return -EINVAL;
	
	if (0 == count)
		return 0;

	if (false == get_value_as_int(buf, count, &value))
		return -EINVAL;
	//dev_info(&gma->client->dev, "value=%d\n", value);
	if (value < 0 || 256 < value)
		return -EINVAL;

	/* set value to reg */
	i2c[0] = (unsigned char)atomic_read(&gma->addr);
	i2c[1] = value;
	gma_i2c_txdata(gma->client, i2c, 2);
	//dev_dbg(&gma->client->dev, "i2c[0]=%d,i2c[1]=%d\n",i2c[0],i2c[1]);
	return count;
}
/*********************************************************************
 *
 * SysFS attribute functions
 *
 * directory : /sys/class/input/inputX/
 * files :
 *  - enable	    [rw]    [t] : enable flag for accelerometer
 *  - delay		    [rw]	[t] : delay in nanosecond for accelerometer
 *  - position		[rw]	[t] : chip mounting position
 *  - offset		[rw]	[t] : show calibration offset
 *  - calibration	[rw]	[t] : G sensor calibration (1~9)
 *  - sma			[rw]	[t] : Simple Moving Average sensor data(1~16)
 *  - ewma			[rw]	[t] : Exponentially weighted moving average sensor data(2,4,8,16)
 *  - data			[r]	    [t] : sensor data
 *  - reg			[r]	    [t] : Displays the current register value
 *  - chipinfo		[r]		[t] : chip information
 *
 * debug :
 *  - debug_suspend	[w]		[t] : suspend test
 *  - reg_rx		[rw] 	[t] : cat: Read from register(show value) , echo : Setting the register to be read
 *  - reg_tx		[rw] 	[t] : cat: Read from register(show value) , echo : The value currently being written to the register
 *
 * [rw]= read/write
 * [r] = read only
 * [w] = write only
 * [b] = binary format
 * [t] = text format
 */

static DEVICE_ATTR(enable,
		   S_IRUGO|S_IWUSR|S_IWGRP,
		   gma_enable_show,
		   gma_enable_store
		   );
static DEVICE_ATTR(delay,
		   S_IRUGO|S_IWUSR|S_IWGRP,
		   gma_delay_show,
		   gma_delay_store
		   );
static DEVICE_ATTR(offset,
		   S_IRUGO|S_IWUGO,
		   gma_offset_show,
		   gma_offset_store
		   );
static DEVICE_ATTR(position,
		   S_IRUGO|S_IWUSR,
		   gma_position_show,
		   gma_position_store
		   );
static DEVICE_ATTR(calibration,
		   S_IRUGO|S_IWUGO,
		   gma_calib_show,
		   gma_calib_store
		   );
#ifdef SMA_FILTER
static DEVICE_ATTR(sma,
		   S_IRUGO|S_IWUGO,
		   gma_sma_show,
		   gma_sma_store
		   );
#endif
#ifdef EWMA_FILTER
static DEVICE_ATTR(ewma,
		   S_IRUGO|S_IWUGO,
		   gma_ewma_show,
		   gma_ewma_store
		   );
#endif
static DEVICE_ATTR(data,
		   S_IRUGO,
		   gma_acc_private_data_show,
		   NULL);
static DEVICE_ATTR(reg, 
			S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
			gma_register_show, gma_register_store);
static DEVICE_ATTR(chipinfo,
		   S_IRUGO,
		   gma_chipinfo_show,
		   NULL);
static DEVICE_ATTR(reg_rx,
		   S_IRUGO|S_IWUGO,
		   gma_reg_read_show,
		   gma_reg_read_store
		   );
static DEVICE_ATTR(reg_tx,
		   S_IRUGO|S_IWUGO,
		   gma_reg_write_show,
		   gma_reg_write_store
		   );
#ifdef GMA_DEBUG_DATA
static DEVICE_ATTR(debug_suspend,
		   S_IRUGO|S_IWUGO,
		   gma_debug_suspend_show,
		   gma_debug_suspend_store
		   );
#endif /* DEBUG */

static struct attribute *gma_acc_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	&dev_attr_offset.attr,
	&dev_attr_position.attr,
	&dev_attr_chipinfo.attr,
	&dev_attr_calibration.attr,
#ifdef SMA_FILTER
	&dev_attr_sma.attr,
#endif /* Simple Moving Average */
#ifdef EWMA_FILTER
	&dev_attr_ewma.attr,
#endif 
	&dev_attr_data.attr,
	&dev_attr_reg.attr,
	&dev_attr_reg_rx.attr,
	&dev_attr_reg_tx.attr,
#ifdef GMA_DEBUG_DATA
	&dev_attr_debug_suspend.attr,
#endif /* DEBUG */
	NULL
};

static struct attribute_group gma_acc_attribute_group = {
	.attrs = gma_acc_attributes
};

/*  Input device interface */
static int gma_acc_input_init(struct gma_data *gma){
	struct input_dev *dev;
	int err;
	//const char *path;
	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;

	dev->name = INPUT_NAME_ACC; /* Setup Input Device Name  */
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_capability(dev, EV_ABS, ABS_RUDDER);
	input_set_abs_params(dev, ABS_X, GMS_GMA30x_ABSMAX_16G, -GMS_GMA30x_ABSMAX_16G, 0, 0);
	input_set_abs_params(dev, ABS_Y, GMS_GMA30x_ABSMAX_16G, -GMS_GMA30x_ABSMAX_16G, 0, 0);
	input_set_abs_params(dev, ABS_Z, GMS_GMA30x_ABSMAX_16G, -GMS_GMA30x_ABSMAX_16G, 0, 0);
	input_set_drvdata(dev, s_gma);
	/* Register */
	err = input_register_device(dev);
	if (err < 0){
		dev_err(&gma->client->dev, "input_register_device ERROR !!\n");
		input_free_device(dev);
		return err;
	}
	
	gma->input = dev;
	//path = kobject_get_path(&gma->input->dev.kobj, GFP_KERNEL);
	//dev_info(&gma->client->dev, "input_register_device SUCCESS %d !!  %s \n",err, path ? path : "N/A");
	//strlen(path)
	return 0;//err;
}

static void gma_acc_input_fini(struct gma_data *gma){
	struct input_dev *dev = gma->input;
	input_unregister_device(dev);
	input_free_device(dev);
}
/* calculate delta offset */
static int gma_acc_calibration(struct gma_data *gma, int gAxis){
// add by Steve 20150202********
	bool compare_avg=false;
	int avg_Z=0;
	int cycle=0;
	int threshold=GMS_DEFAULT_SENSITIVITY/20;
// *****************************
	raw_data avg;
	int i, j, xyz[SENSOR_DATA_SIZE];
	long xyz_acc[SENSOR_DATA_SIZE];
	/* initialize the offset value */
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		gma->offset.v[i] = 0;
// add by Steve 20150202********
	for(cycle=0;cycle<3;cycle++)
	{		
// *****************************		  	
		/* initialize the accumulation buffer */
	  	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
			xyz_acc[i] = 0;
	
		for(i = 0; i < AVG_NUM; i++) {
			gma_acc_measure(gma, (int *)&xyz);
			for(j = 0; j < SENSOR_DATA_SIZE; ++j)
				xyz_acc[j] += xyz[j];
	  	}
		/* calculate averages */
	  	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
			avg.v[i] = xyz_acc[i] / AVG_NUM;
		
// add by Steve 20150202********
		if(abs(avg.u.z-avg_Z) > threshold)
		{
			dev_info(&gma->client->dev, "Sensor unstable cycle %d , try again automatically: %d\n",cycle , __LINE__);
			compare_avg=false;
			avg_Z=avg.u.z;	
		}			
		else
		{
			dev_info(&gma->client->dev, "Sensor Stable in comparsion cycle %d : %d\n",cycle , __LINE__);
			compare_avg=true;
			break;	
		}
	}//end of for(c_cycle=0;c_cycle<2;c_cycle++)r	
	if(compare_avg==false)
	{
		dev_info(&gma->client->dev, "Sensor unstable,Please try again %d : %d\n", gAxis, __LINE__);
		return -1;
	}
// *****************************
	switch(gAxis){
		case GRAVITY_ON_Z_NEGATIVE:
			gma->offset.u.x =  avg.v[0];
			gma->offset.u.y =  avg.v[1];
			gma->offset.u.z =  avg.v[2] + GMS_DEFAULT_SENSITIVITY;
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
			break;
		case GRAVITY_ON_X_POSITIVE:
			gma->offset.u.x =  avg.v[0] - GMS_DEFAULT_SENSITIVITY;    
			gma->offset.u.y =  avg.v[1];
			gma->offset.u.z =  avg.v[2];
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
		 	break;
		case GRAVITY_ON_Z_POSITIVE:
			gma->offset.u.x =  avg.v[0] ;
			gma->offset.u.y =  avg.v[1] ;
			gma->offset.u.z =  avg.v[2] - GMS_DEFAULT_SENSITIVITY;
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
		 	break;
		case GRAVITY_ON_X_NEGATIVE:
			gma->offset.u.x =  avg.v[0] + GMS_DEFAULT_SENSITIVITY;    
			gma->offset.u.y =  avg.v[1];
			gma->offset.u.z =  avg.v[2];
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
		 	break;
		case GRAVITY_ON_Y_NEGATIVE:
			gma->offset.u.x =  avg.v[0];    
			gma->offset.u.y =  avg.v[1] - GMS_DEFAULT_SENSITIVITY;
			gma->offset.u.z =  avg.v[2];
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
		 	break;
		case GRAVITY_ON_Y_POSITIVE:
			gma->offset.u.x =  avg.v[0];    
			gma->offset.u.y =  avg.v[1] + GMS_DEFAULT_SENSITIVITY;
			gma->offset.u.z =  avg.v[2];
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
		 	break;
		case GRAVITY_ON_X_AUTO:
			if(avg.v[0] < 0){
				gma->offset.u.x =  avg.v[0] + GMS_DEFAULT_SENSITIVITY;
				gma->offset.u.y =  avg.v[1];
				gma->offset.u.z =  avg.v[2];
			}
			else{
				gma->offset.u.x =  avg.v[0] - GMS_DEFAULT_SENSITIVITY;
				gma->offset.u.y =  avg.v[1];
				gma->offset.u.z =  avg.v[2];
			}
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
			break;
	    case GRAVITY_ON_Y_AUTO:
			if(avg.v[1] < 0){
				gma->offset.u.x =  avg.v[0];
				gma->offset.u.y =  avg.v[1] + GMS_DEFAULT_SENSITIVITY;
				gma->offset.u.z =  avg.v[2];
			}
			else{
				gma->offset.u.x =  avg.v[0];
				gma->offset.u.y =  avg.v[1] - GMS_DEFAULT_SENSITIVITY;
				gma->offset.u.z =  avg.v[2];
			}
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
			break;
		case GRAVITY_ON_Z_AUTO: 
			if(avg.v[2] < 0){
				gma->offset.u.x =  avg.v[0];
				gma->offset.u.y =  avg.v[1];
				gma->offset.u.z =  avg.v[2] + GMS_DEFAULT_SENSITIVITY;
			}
			else{
				gma->offset.u.x =  avg.v[0];
				gma->offset.u.y =  avg.v[1];
				gma->offset.u.z =  avg.v[2] - GMS_DEFAULT_SENSITIVITY;
			}
			dev_info(&gma->client->dev, "gAxis = %d : %d\n", gAxis, __LINE__);
			break;
		default:  
			return -ENOTTY;
	}
	return 0;
}

int gma30x_init(struct gma_data *gma){
	unsigned char buffer[7];
	/* 1. Powerdown reset */
	buffer[0] = GMA1302_REG_PD;
	buffer[1] = GMA1302_MODE_RESET;
	gma_i2c_txdata(gma->client, buffer, 2);
	/* 2. check GMA1302_REG_PID(0x00) */
	buffer[0] = GMA1302_REG_PID;
	gma_i2c_rxdata(gma->client, buffer, 1);
	if(buffer[0] == GMA302_VAL_WMI)
		dev_info(&gma->client->dev, "%s: PID = 0x%x, GMA302 accelerometer\n", __func__, buffer[0]);
	else if(buffer[0] == GMA303_VAL_WMI)
		dev_info(&gma->client->dev, "%s: PID = 0x%x, GMA303 accelerometer\n", __func__, buffer[0]);
	else if(buffer[0] == GMA303_VAL_WMI_RD)
		dev_info(&gma->client->dev, "%s: PID = 0x%x, GMA303_RD sample\n", __func__, buffer[0]);
	else{
		dev_err(&gma->client->dev, "%s: PID = 0x%x, The device is not GlobalMems accelerometer.", __func__, buffer[0]);
		return -ENXIO;
	}
	/* 3. turn off the high-pass filter */
	buffer[0] = GMA1302_REG_CONTR1;
	buffer[1] = GMA1302_VAL_OFF;//GMA1302_VAL_LPF_ON;
	gma_i2c_txdata(gma->client, buffer, 2);
	/* 4. turn on the offset temperature compensation */
	buffer[0] = GMA1302_REG_CONTR3;
	buffer[1] = GMA1302_VAL_OFFSET_TC_ON;
	gma_i2c_txdata(gma->client, buffer, 2);
	/* 5. turn off the data ready interrupt and configure the INT pin to active high, push-pull type */
	buffer[0] = GMA1302_REG_INTCR;
	buffer[1] = GMA1302_VAL_OFF;//GMA1302_VAL_DATA_READY_ON;
	gma_i2c_txdata(gma->client, buffer, 2);
	/* 6. treshold set to max */
    buffer[0] = GMA1302_REG_MTHR;
    buffer[1] = GMA1302_VAL_TRESHOLD_MAX;
    gma_i2c_txdata(gma->client, buffer, 2);
	/* 7. Oversampling mode & Set Action register */
	buffer[0] = GMA1302_REG_OSM;
	buffer[1] = GMA1302_VAL_LOW_NOISE; //Low noise
	gma_i2c_txdata(gma->client, buffer, 2);
	buffer[0] = GMA1302_REG_ACTR;
	buffer[1] = GMA1302_VAL_ACTR_CONTINUOUS;
	buffer[2] = GMA1302_VAL_ACTR_RESET;
	buffer[3] = GMA1302_VAL_ACTR_NON_CONTINUOUS;
	buffer[4] = GMA1302_VAL_ACTR_RESET;
	gma_i2c_txdata(gma->client, buffer, 5);
	
#ifdef EWMA_FILTER
	ewma_init(&average[0], EWMA_FACTOR, EWMA_WEIGHT_X);
	ewma_init(&average[1], EWMA_FACTOR, EWMA_WEIGHT_Y);
	ewma_init(&average[2], EWMA_FACTOR, EWMA_WEIGHT_Z);
	gma->ewma_filter[0] = EWMA_WEIGHT_X;
	gma->ewma_filter[1] = EWMA_WEIGHT_Y;
	gma->ewma_filter[2] = EWMA_WEIGHT_Z;
	dev_info(&gma->client->dev, "EWMA_FILTER: %d %d %d\n", gma->ewma_filter[0], gma->ewma_filter[1], gma->ewma_filter[2]);
#endif
	return 0;
}

static int sensor_reset( struct gma_data *gma, int hard){
	int err = 0;
	if (hard != 0) {
		gpio_set_value(gma->rstn, 0);
		udelay(5);
		gpio_set_value(gma->rstn, 1);
	} 
	else {
		/* Set flag */
		atomic_set(&gma->drdy, 0);
	}
	err = gma30x_init(gma);
		if (err < 0) 
			dev_err(&gma->client->dev, "%s: Can not set SRST .", __func__);
		else 
			dev_info(&gma->client->dev, "Soft reset is done.");
	/* Device will be accessible 100 us after */
	udelay(100);

	return err;
}

void gma_set_offset(struct gma_data *gma, int val[3]){
	int i;
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		gma->offset.v[i] = val[i];
}

struct file_operations sensor_fops = {
	.owner = THIS_MODULE,
	.open = gma_open,
	.release = gma_close,
	.unlocked_ioctl = gma_ioctl,
};

static struct miscdevice gma_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = GSENSOR_ID,
	.fops = &sensor_fops,
};

static int sensor_close_dev(struct gma_data *gma){
	char buffer[2];
	buffer[0] = GMA1302_REG_PD;
	buffer[1] = GMA1302_MODE_POWERDOWN;
	gma_i2c_txdata(gma->client, buffer, 2);
/*	buffer[0] = GMA1302_REG_PD;
	gma_i2c_rxdata(gma->client, buffer, 1);
	dev_dbg(&gma->client->dev, "Power down control=0x%x", buffer[0]);
*/	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sensor_suspend(struct early_suspend *h)
{
	struct gma_data *gma = 
			container_of(h, struct gma_data, early_suspend);
	
	gma_sysfs_update_active_status(gma , 0);
	sensor_close_dev(gma);
}

static void sensor_resume(struct early_suspend *h)
{
	struct gma_data *gma = 
			container_of(h, struct gma_data, early_suspend);

	int en = atomic_read(&gma->enable);
	dev_info(&gma->client->dev, "gma->enable=%d", en);
	gma30x_init(gma);
	atomic_set(&gma->enable,en);
	gma_sysfs_update_active_status(gma , en);
}
#else
static int gma_acc_suspend(struct i2c_client *client, pm_message_t mesg){
	struct gma_data *gma = i2c_get_clientdata(client);
	gma_sysfs_update_active_status(gma , 0);
	return sensor_close_dev(gma);
}

static int gma_acc_resume(struct i2c_client *client){
	struct gma_data *gma = i2c_get_clientdata(client);
	int en = atomic_read(&gma->enable);
	dev_info(&client->dev, "gma->enable=%d", en);
	gma30x_init(gma);
	atomic_set(&gma->enable,en);
	gma_sysfs_update_active_status(gma , en);
	return 0;
}
#endif
//static SIMPLE_DEV_PM_OPS(gma_pm_ops, gma_acc_suspend, gma_acc_resume);

static int  gma_acc_remove(struct i2c_client *client){
	struct gma_data *gma = i2c_get_clientdata(client);
	sysfs_remove_group(&gma->input->dev.kobj, &gma_acc_attribute_group);
	return 0;
}

static const struct i2c_device_id gma_i2c_ids[] ={
	{ GSENSOR_ID, 0},
	{ }
};

MODULE_DEVICE_TABLE(i2c, gma_i2c_ids);

static struct i2c_driver gma_i2c_driver ={
	.driver	= {
		.owner = THIS_MODULE,
		.name = GSENSOR_ID,
		//.pm = &gma_pm_ops,
		},
	.id_table = gma_i2c_ids,
	.probe = gma_acc_probe,
	.remove	= gma_acc_remove,
/*
#ifndef CONFIG_ANDROID_POWER
	.suspend = gma_acc_suspend,
	.resume = gma_acc_resume,
#endif
*/
};

static int gma_open(struct inode *inode, struct file *filp){
	filp->private_data = s_gma;
	return nonseekable_open(inode, filp);
}

static long gma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	void __user *argp = (void __user *)arg;
	struct gma_data *gma = filp->private_data;
	int temperature;		/* for GET_TEMPERATURE */
	int layout;				/* for GET_LAYOUT */
	int8_t sensor_buf[38];	/* for GETDATA */
	int32_t ypr_buf[12];	/* for SET_YPR */
	char i2c_buf[16];		/* for READ/WRITE */
	//int64_t delay;			/* for GET_DELAY */
	int err = 0, i, intBuf[SENSOR_DATA_SIZE];
	u8 buffer[2];
	// check type
	if (_IOC_TYPE(cmd) != GMA_IOCTL) return -ENOTTY;

	// check user space pointer is valid
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, argp, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, argp, _IOC_SIZE(cmd));
	if (err){
		dev_err(&gma->client->dev, "access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
		return -EFAULT;
	}

	switch(cmd) {
		case GMA_IOCTL_READ:
		case GMA_IOCTL_WRITE:
			if (argp == NULL) {
				dev_err(&gma->client->dev, "invalid argument.");
				return -EINVAL;
			}
			if (copy_from_user(&i2c_buf, argp, sizeof(i2c_buf))){
				dev_err(&gma->client->dev, "copy_from_user failed.");
				return -EFAULT;
			}
			break;
		case GMA_IOCTL_GETDATA:
			dev_dbg(&gma->client->dev, "IOCTL_GETDATA called.");
			err = GMA_IO_GetData(gma, sensor_buf, SENSOR_DATA_SIZE);
			if (err < 0)
				return err;
			if (copy_to_user(argp, &sensor_buf, sizeof(sensor_buf))){
				dev_err(&gma->client->dev, "copy_to_user failed.");
				return -EFAULT;
			}
			break;
		case GMA_IOCTL_RESET:
			err = sensor_reset(gma, gma->rstn);
			if (err < 0)
				return err;
			break;
		case GMA_IOCTL_CALIBRATION:
			// get orientation info
			if(copy_from_user(&intBuf, (int*)argp, sizeof(intBuf))) return -EFAULT;
			dev_info(&gma->client->dev, "intBuf[0]= %d\n", intBuf[0]);
			gma_acc_calibration(gma, intBuf[0]);
			dev_info(&gma->client->dev, "offset.u.x= %d, offset.u.y= %d, offset.u.z= %d\n", 
				gma->offset.u.x, gma->offset.u.y, gma->offset.u.z);
			/* save file */
			GMA_WriteCalibration(gma, GMA_Offset_TXT);
			GMA_WriteCalibration(gma, GMA_Offset_TXT_PATH2);
			/* return the offset */
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = gma->offset.v[i];

			err = copy_to_user((int *)argp, &intBuf, sizeof(intBuf));
			break;
		case GMA_IOCTL_GET_OFFSET:
			/* get gma_data from file */
			GMA_ReadCalibration(gma);
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = gma->offset.v[i];

			err = copy_to_user((int *)argp, &intBuf, sizeof(intBuf));
			break;
		case GMA_IOCTL_SET_OFFSET:
			err = copy_from_user(&intBuf, (int *)argp, sizeof(intBuf));
			gma_set_offset(gma , intBuf);
			/* write into file */
			GMA_WriteCalibration(gma, GMA_Offset_TXT);
			GMA_WriteCalibration(gma, GMA_Offset_TXT_PATH2);
			break;
		case GMA_IOCTL_READ_ACCEL_RAW_XYZ:
			gma_acc_measure(gma, (int *)&intBuf);
		  	err = copy_to_user((int*)argp, &intBuf, sizeof(intBuf));
			dev_dbg(&gma->client->dev, "X/Y/Z: %d , %d , %d ,delay=%d\n",
				intBuf[0], intBuf[1], intBuf[2], atomic_read(&gma->delay));
			break;
		case GMA_IOCTL_READ_ACCEL_XYZ:
			//gma_acc_measure(gma, (int *)&xyz);
			//for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				//intBuf[i] = xyz[i] - gma->offset.v[i];
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				intBuf[i] = gma->accel_data.v[i];
		  	err = copy_to_user((int*)argp, &intBuf, sizeof(intBuf));
			dev_dbg(&gma->client->dev, "X/Y/Z: %d , %d , %d ,delay=%d\n",
				gma->accel_data.u.x, gma->accel_data.u.y, gma->accel_data.u.z, atomic_read(&gma->delay));
			mdelay(20);
			break;
		case GMA_IOCTL_SETYPR:
			if (argp == NULL) {
				dev_err(&gma->client->dev, "invalid argument.");
				return -EINVAL;
			}
			if (copy_from_user(&ypr_buf, argp, sizeof(ypr_buf))) {
				dev_err(&gma->client->dev, "copy_from_user failed.");
				return -EFAULT;
			}
			dev_dbg(&gma->client->dev, "IOCTL_SET_YPR called.");
			//GMA_SetYPR(gma, ypr_buf);
			break;
		case GMA_IOCTL_GET_OPEN_STATUS:
			dev_dbg(&gma->client->dev, "IOCTL_GET_OPEN_STATUS called.\n");
			err = GmaGetOpenStatus(gma);
			if (err < 0) 
				dev_err(&gma->client->dev, "Get Open returns error (%d).", err);
			break;
		case GMA_IOCTL_GET_CLOSE_STATUS:
			dev_dbg(&gma->client->dev, "IOCTL_GET_CLOSE_STATUS called.\n");
			err = GmaGetCloseStatus(gma);
			if (err < 0) 
				dev_err(&gma->client->dev, "Get Close returns error (%d).", err);
			break;
		case GMA_IOCTL_GET_DELAY:
		  	err = copy_to_user((int*)argp, &interval, sizeof(interval));
			break;
		case GMA_IOCTL_GET_LAYOUT:
			dev_dbg(&gma->client->dev, "IOCTL_GET_LAYOUT called.");
			layout = atomic_read(&gma->position);
			if (copy_to_user(argp, &layout, sizeof(layout))) {
				dev_err(&gma->client->dev, "copy_to_user failed.");
				return -EFAULT;
			}
			break;
		case GMA_IOCTL_GET_TEMPERATURE:
			/* get DT high/low bytes, 0x0c 0x0*/
			buffer[0] = 0x0c;
			/* Read acceleration data */
			if (gma_i2c_rxdata(gma->client, buffer, 2)!= 0)
				dev_err(&gma->client->dev, "Read acceleration data fail\n");
			else{
				/* merge xyz high/low bytes */
				mutex_lock(&gma->accel_mutex);
				temperature = ((int)((buffer[1] << 8)) | buffer[0] );
				mutex_unlock(&gma->accel_mutex);
				dev_dbg(&gma->client->dev, "temperature = %d\n",temperature);
			}		
			if (copy_to_user(argp, &temperature, sizeof(temperature))) {
				dev_err(&gma->client->dev, "copy_to_user failed.");
				return -EFAULT;
			}
			break;
		default:  // redundant, as cmd was checked against MAXNR
			return -ENOTTY;
	}

	return 0;
}

static int gma_close(struct inode *inode, struct file *filp){
	return 0;
}

/***** I2C I/O function ***********************************************/
static int gma_i2c_rxdata( struct i2c_client *client, unsigned char *rxData, int length){
	struct i2c_msg msgs[] = 
	{
		{.addr = client->addr, .flags = 0, .len = 1, .buf = rxData,},
		{.addr = client->addr, .flags = I2C_M_RD, .len = length, .buf = rxData,},
	};
#ifdef GMA_DEBUG_DATA
	unsigned char addr = rxData[0];
#endif
	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
		dev_err(&client->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}
#ifdef GMA_DEBUG_DATA
	dev_dbg(&client->dev, "RxData: len=%02x, addr=%02x, data=%02x\n",
		length, addr, rxData[0]);
#endif
	return 0;
}

static int gma_i2c_txdata( struct i2c_client *client, unsigned char *txData, int length){
	struct i2c_msg msg[] = {
		{.addr = client->addr, .flags = 0, .len = length, .buf = txData,},
	};

	if (i2c_transfer(client->adapter, msg, 1) < 0) {
		dev_err(&client->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}
#ifdef GMA_DEBUG_DATA
	dev_dbg(&client->dev, "TxData: len=%02x, addr=%02x data=%02x\n",
		length, txData[0], txData[1]);
#endif
	return 0;
}

/** 
 *	@brief check_horizontal_state
 *  @param gma_data description
 *  @return 0
 */
#if 0//AutoZeroZ	//run time calibration AutoZeroZ 
static int check_horizontal_state(struct gma_data *gma)
{
	static int MaxRange = LevelValueRange_0_0625;
	int i;
	raw_data offset; 		/* Record old offset */
	static int stable_flag =0; //for auto zero .Analyzing horizontal state
	/* 1.Condition: Analyzing horizontal state */
	if( ABS(gma->accel_data.u.x) < MaxRange && ABS(gma->accel_data.u.y) < MaxRange  && stable_flag < 10)
		stable_flag++;
	else{
		if(stable_flag < 0)
			stable_flag = 0;
		else if(stable_flag < 10)
			stable_flag--;
		else if(stable_flag > 10)
			stable_flag=11;
		else
			stable_flag++;
	}
	//dev_dbg(&gma->client->dev, "a.stable_flag= %d, (rawdata - offset) x/y/z = %03d %03d %03d , MaxRange/981 =%d(LSB)\n", stable_flag, gma->axis.x/981, gma->axis.y/981, gma->axis.z/981, MaxRange/981);
	if(stable_flag == 10){
		//dev_dbg(&gma->client->dev, "stable_flag=%d\n", stable_flag);
		/* 2.Condition: Analyzing horizontal state check again*/
		if( ABS(gma->accel_data.u.x) <= LevelValueRange_0_015625  && ABS(gma->accel_data.u.y) <= LevelValueRange_0_015625 ){
		//dev_dbg(&gma->client->dev, "b.(rawdata - offset) x/y/z = %03d %03d %03d , MaxRange/981 =%d(LSB)\n", gma->axis.x/981, gma->axis.y/981, gma->axis.z/981, MaxRange/981);
			/* 3.record last time offset */
			for(i = 0; i < SENSOR_DATA_SIZE; ++i)
				offset.v[i] = gma->offset.v[i];
			/* 4.Calculate new offset */
			gma_acc_calibration(gma, GRAVITY_ON_Z_AUTO);
			/* 5.offset(X& Y): |new - last_time| < LevelValueRange_0_0078125 */
			if(ABS(gma->offset.u.x - offset.u.x) <= LevelValueRange_0_0078125 && ABS(gma->offset.u.y - offset.u.y) <= LevelValueRange_0_0078125 ){
				for(i = 0; i < SENSOR_DATA_SIZE; ++i)
					gma->offset.v[i] = offset.v[i];	/* use last time offset */
			}
			else{
			/* 6.offset(Z): |last time| > 1g (Prevent calibration errors) */
				if(ABS(offset.u.z) > GMS_DEFAULT_SENSITIVITY)
					gma->offset.v[2] = offset.v[2];	/*(of.z) use last time offset */
					
				dev_dbg(&gma->client->dev, "d.gma->offset.v[2] =%d :%d\n", gma->offset.v[2], __LINE__);
				GMA_WriteCalibration(gma , GMA_Offset_TXT);
			}
			MaxRange /= 2;
			stable_flag = 0;
		}
	}
	
	return stable_flag;
}
#endif
/** 
 *	@brief Simple moving average
 *  @param gma_data description
 *  @param int *xyz
 *  @return 0
 */
#ifdef SMA_FILTER
/* for Simple Moving Average */
static int SMA( struct gma_data *gma, int *xyz){
	int i, j;
	static s8	pointer = -1;				/* last update data */

	/* init gma->sum */
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		gma->sum[i] = 0;

	pointer++;
	pointer %= gma->sma_filter;
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		gma->bufferave[i][pointer] = xyz[i];

    for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		for(j = 0; j < gma->sma_filter; ++j)
			gma->sum[i] += gma->bufferave[i][j];

	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		xyz[i] = gma->sum[i] / gma->sma_filter;

	return 0;
}
#endif

/** 
 *	@brief get XYZ rawdata & Redefined 1G =1024
 *  @param gma_data description
 *  @param xyz_p
 *  @return 0
 */
static int gma_acc_measure(struct gma_data *gma, int *xyz_p){
	u8 buffer[11];
	s16 xyzTmp[SENSOR_DATA_SIZE];
	int i;
	/* get xyz high/low bytes, 0x04 */
	buffer[0] = GMA1302_REG_STADR;
	for(i = 0; i < SENSOR_DATA_SIZE; ++i){
		xyz_p[i] = 0;
		xyzTmp[i] = 0;
	}
	/* Read acceleration data */
	if (gma_i2c_rxdata(gma->client, buffer, 11)!= 0)
		dev_err(&gma->client->dev, "Read acceleration data fail\n");
	else{
		for(i = 0; i < SENSOR_DATA_SIZE; ++i){
			/* merge xyz high/low bytes(13bit) & 1g = 512 *2 = GMS_DEFAULT_SENSITIVITY */
			mutex_lock(&gma->accel_mutex);
			xyzTmp[i] = ((buffer[2*(i+2)] << 8) | buffer[2*(i+1)+1] ) << 1;
			mutex_unlock(&gma->accel_mutex);
		}
	}
	/* enable ewma filter */ 
#ifdef EWMA_FILTER
	for(i = 0; i < SENSOR_DATA_SIZE; ++i){
		/* rawdata + EWMA_POSITIVE = unsigned long data */
	    ewma_add(&average[i], (unsigned long) (xyzTmp[i] + EWMA_POSITIVE));
	    xyz_p[i] = (int) (ewma_read(&average[i]) - EWMA_POSITIVE);
	}
#else
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		xyz_p[i] = xyzTmp[i];
#endif
	return 0;
}
/* Transformation matrix for chip mounting position 
first pin top:Positive  bottom:Negative 
	2: top/upper-left	(-y,-x,-z)
	3: top/upper-right	(-x, y,-z)
	4: top/lower-right	( y,-x,-z)
	1: top/lower-left	( x,-y,-z)
	-3: bottom/upper-left  ( x,  y, z)
	-2: bottom/upper-right ( y, -x, z)
	-1: bottom/lower-right (-x, -y, z)
	-4: bottom/lower-left  (-y, -x, z)
*/
static int gma_axis_remap(struct gma_data *gma)
{
    int swap;
    int position = atomic_read(&gma->position);
    switch (abs(position)) {
		case 1:
			swap = gma->accel_data.v[0];
            gma->accel_data.v[0] = -(gma->accel_data.v[1]);
            gma->accel_data.v[1] = -swap;
			gma->accel_data.v[2] = -(gma->accel_data.v[2]);
            break;
        case 2:
            gma->accel_data.v[0] = -(gma->accel_data.v[0]);
            gma->accel_data.v[1] = gma->accel_data.v[1];
			gma->accel_data.v[2] = -(gma->accel_data.v[2]);
            break;
        case 3:
			swap = gma->accel_data.v[0];
            gma->accel_data.v[0] = gma->accel_data.v[1];
            gma->accel_data.v[1] = swap;
			gma->accel_data.v[2] = -(gma->accel_data.v[2]);
            break;
            
        case 4:
            gma->accel_data.v[0] = gma->accel_data.v[0];
            gma->accel_data.v[1] = -(gma->accel_data.v[1]);
			gma->accel_data.v[2] = -(gma->accel_data.v[2]);
            break;
    }
    
    if (position < 0) {
        gma->accel_data.v[2] = -(gma->accel_data.v[2]);
        gma->accel_data.v[0] = -(gma->accel_data.v[0]);
    }
    //dev_dbg(&gma->client->dev, "gma->position= %d :%d\n", position, __LINE__);
    return 0;
}

static void gma_work_func(struct work_struct *work){
	struct gma_data *gma = container_of((struct delayed_work *)work, struct gma_data, work);
	static bool firsttime=true;
	raw_data xyz;
	int i;
  	unsigned long delay = msecs_to_jiffies(atomic_read(&gma->delay));
  	int ret = gma_acc_measure(gma, (int *)&xyz.v);
#ifdef SMA_FILTER
	if (xyz.v[0] != 0 && xyz.v[1] != 0 && xyz.v[2] != 0)
		SMA( gma, (int *)&xyz.v);
	//dev_dbg(&gma->client->dev, "SMA_FILTER  xyz.v: %3d , %3d , %3d\n", xyz.v[0], xyz.v[1], xyz.v[2]);
#endif

	/* data->accel_data = RawData - Offset) , redefine 1g = 1024 */
  	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
     	gma->accel_data.v[i] = xyz.v[i] - gma->offset.v[i];
	
	if (ret == 0) 
		gma_axis_remap(gma);
	dev_dbg(&gma->client->dev, "after gma_axis_remap X/Y/Z  %05d,%05d,%05d !\n"
			, gma->accel_data.v[0], gma->accel_data.v[1], gma->accel_data.v[2]);
	if(firsttime){
		GMA_ReadCalibration(gma);
	 	firsttime=false;
	}

#if 0//run time calibration AutoZeroZ
	int flag = 0;
	if(flag != 11)	//Check the level of state
		flag = check_horizontal_state(gma);
	dev_dbg(&gma->client->dev, "flag= %d , gma->accel_data.u.x/y/z = %5d  %5d  %5d :%d\n"
			, flag, gma->accel_data.v[0], gma->accel_data.v[1], gma->accel_data.v[2], __LINE__);
#endif

    input_report_abs(gma->input, ABS_X, gma->accel_data.v[0]);
	input_report_abs(gma->input, ABS_Y, gma->accel_data.v[1]);
	input_report_abs(gma->input, ABS_Z, gma->accel_data.v[2]);
	input_sync(gma->input);
    

	if(delay < 8)
		delay = 8;
	schedule_delayed_work(&gma->work, delay);
}

#ifdef GMA_PERMISSION_THREAD
static struct task_struct *GMAPermissionThread = NULL;

static int gma_permission_thread(void *data)
{
	int ret = 0;
	int retry = 0;
	char input_number[2];
	char input_enable[30];
	char input_delay[30];
	char input_calibration[35];
	const char *path;
	mm_segment_t fs = get_fs();
	set_fs(KERNEL_DS);	
	/* check uid */
	//int uid  = sys_getuid();
	//dev_dbg(&s_gma->client->dev, "\nUID %d",uid);
	/* get input the registration number */
	path = kobject_get_path(&s_gma->input->dev.kobj, GFP_KERNEL);
	//dev_dbg(&s_gma->client->dev, "input_register_device SUCCESS %s :%d\n", path ? path : "N/A", __LINE__);
	ret = strlen(path);
	if(path[ret-2] == 't'){
		input_number[1] = path[ret-1];
		sprintf(input_enable, "/sys/class/input/input%c/enable", input_number[1]);
		sprintf(input_delay, "/sys/class/input/input%c/delay", input_number[1]);
		sprintf(input_calibration, "/sys/class/input/input%c/calibration", input_number[1]);
		dev_dbg(&s_gma->client->dev, "input_enable = %s\n""input_delay = %s\n""input_calibration = %s :%d\n"
									, input_enable, input_delay, input_calibration, __LINE__);
	}
	else if(path[ret-3] == 't'){
		input_number[0] = path[ret-2];
		input_number[1] = path[ret-1];
		sprintf(input_enable, "/sys/class/input/input%c%c/enable", input_number[1],input_number[0]);
		sprintf(input_delay, "/sys/class/input/input%c%c/enable", input_number[1],input_number[0]);
		sprintf(input_calibration, "/sys/class/input/input%c%c/calibration", input_number[1],input_number[0]);
		dev_dbg(&s_gma->client->dev, "input_enable = %s\n""input_delay = %s\n""input_calibration = %s:%d\n"
									, input_enable, input_delay, input_calibration, __LINE__);
	}
	else
		goto ERR;
		
	//msleep(5000);
	do{
	    msleep(2000);
		ret = sys_fchmodat(AT_FDCWD, CHAR_DEV , 0666);
		ret = sys_fchmodat(AT_FDCWD, SH_GSS , 0755);
		ret = sys_fchmodat(AT_FDCWD, EXEC_GMAD , 0755);
		ret = sys_chown(SH_GSS , 1000, 2000);	//system uid 1000 shell gid 2000
		ret = sys_chown(EXEC_GMAD , 1000, 2000);
		ret = sys_chown(input_enable , 1000, 1000);	
		ret = sys_chown(input_delay , 1000, 1000);
		ret = sys_mkdir(DIR_SENSOR , 01777);
		if(ret < 0)
			dev_err(&s_gma->client->dev, "fail to execute sys_mkdir,If the DIR exists, do not mind. ret = %d :%d\n"
					, ret, __LINE__);
		ret = sys_fchmodat(AT_FDCWD, input_calibration , 0666);
		ret = sys_fchmodat(AT_FDCWD, GMA_Offset_TXT , 0666);

		if(retry++ != 0)
			break;
	}while(ret == -ENOENT);
	set_fs(fs);
	//dev_dbg(&s_gma->client->dev, "%s exit, retry=%d\n", __func__, retry);
	return 0;
ERR:
	return ret;
}
#endif	/*	#ifdef GMA_PERMISSION_THREAD	*/

static int gma_acc_probe(struct i2c_client *client, const struct i2c_device_id *id){
	int i, err = 0, position;
#ifdef SMA_FILTER
	int j;
#endif
	struct gma_platform_data *pdata;
	dev_info(&client->dev, "start probing.");
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s: check_functionality failed.", __func__);
		err = -ENODEV;
		goto ERR0;
	}
	
    /* Setup private data */
	s_gma = kzalloc(sizeof(struct gma_data), GFP_KERNEL);
	//memset(s_gma, 0, sizeof(struct gma_data));
	if (!s_gma) {
		dev_err(&client->dev, "%s: memory allocation failed.", __func__);
		err = -ENOMEM;
		goto ERR1;
	}
	//gma_acc_set_data(s_gma);

	/* Set layout information */
	pdata = client->dev.platform_data;
	if (pdata) {
		/* Platform data is available. copy its value to local. */
		position = atomic_set(&s_gma->position, pdata->layout);
		s_gma->rstn = pdata->gpio_RSTN;
		dev_info(&client->dev, "s_gma->position= %d, pdata->layout= %d\n", position, pdata->layout);
	} 
	else {
		/*	Platform data is not available.
			Layout information should be set by each application.
			Acceleration Sensor Mounting Position on Board */
		position = atomic_set(&s_gma->position, GMS_GMA30x_DEFAULT_POSITION);
		dev_info(&client->dev, "No platform data. Use GMS_GMA30x_DEFAULT_POSITION. s_gma->position = %d\n", position);
		s_gma->rstn = 0;
	}
	
	/* initialize variables in s_gma */
	mutex_init(&s_gma->sensor_mutex);
	mutex_init(&s_gma->accel_mutex);
#ifdef GMA_DEBUG_DATA
	mutex_init(&s_gma->suspend_mutex);
	atomic_set(&s_gma->suspend, 0);
#endif
	init_waitqueue_head(&s_gma->open_wq);
	atomic_set(&s_gma->active, 0);
	atomic_set(&s_gma->drdy, 0);
	atomic_set(&s_gma->enable, 0);
	atomic_set(&s_gma->delay, 0);
	atomic_set(&s_gma->addr, 0);
	for(i = 0; i < SENSOR_DATA_SIZE; ++i)
		s_gma->offset.v[i] = 0;
		
#ifdef SMA_FILTER
	for(i = 0; i < SENSOR_DATA_SIZE; ++i){
		s_gma->sum[i] = 0;
		for(j = 0; j < SMA_AVG; ++j)
			s_gma->bufferave[i][j] = 0;
	}
	s_gma->sma_filter = SMA_AVG;
	dev_info(&client->dev, "Moving AVG FILTER = %d\n", s_gma->sma_filter);
#endif
	/* Setup i2c client */
	s_gma->client = client;
	err = sensor_reset(s_gma, s_gma->rstn);
	if (err < 0)
		goto ERR2;

	/* set client data */
	i2c_set_clientdata(client, s_gma);
	
	/* Setup driver interface */
	INIT_DELAYED_WORK(&s_gma->work, gma_work_func);
	dev_info(&client->dev, "INIT_DELAYED_WORK\n");

	/* Setup input device interface */
	err = gma_acc_input_init(s_gma);
	if (err < 0){
	    dev_err(&client->dev, "%s: input_dev register failed", __func__);
		goto ERR3;
	}

	/* Setup sysfs */
    if (sysfs_create_group(&s_gma->input->dev.kobj, &gma_acc_attribute_group) < 0){
		dev_err(&client->dev, "create sysfs failed.");
        goto ERR3;
    }

	INIT_LIST_HEAD(&s_gma->devfile_list);
	if (misc_register(&gma_device) < 0){
		dev_err(&client->dev, "gma301 device register failed\n");
		goto ERR4;
	}
#ifdef GMA_PERMISSION_THREAD
	GMAPermissionThread = kthread_run(gma_permission_thread,"gma","Permissionthread");
	if(IS_ERR(GMAPermissionThread))
		GMAPermissionThread = NULL;
#endif // GMA_PERMISSION_THREAD
#ifdef CONFIG_HAS_EARLYSUSPEND
	s_gma->early_suspend.suspend = sensor_suspend;
	s_gma->early_suspend.resume = sensor_resume;
	s_gma->early_suspend.level = 0x02;
	register_early_suspend(&s_gma->early_suspend);
#endif

	return 0;

ERR4:
	misc_deregister(&gma_device);
ERR3:
	gma_acc_input_fini(s_gma);
ERR2:
	kfree(s_gma);
ERR1:
ERR0:
	return err;
}

static int gma_set_position(struct gma_data *gma, int position){
    dev_info(&gma->client->dev, "position=%d :%d\n", position, __LINE__);
	if (!((position >= -4) && (position <= 4)))
		return -1;
	atomic_set(&gma->position, position);
	//dev_info(&gma->client->dev, "gma->position=%d :%d\n",atomic_read(gma->position), __LINE__);
	return 0;
}

static int GMA_WriteCalibration(struct gma_data *gma, char * offset){
	char w_buf[20] = {0};
	struct file *fp;
	mm_segment_t fs;
	ssize_t ret;

	sprintf(w_buf,"%d %d %d", gma->offset.u.x, gma->offset.u.y, gma->offset.u.z);
	dev_err(&gma->client->dev, "%d %d %d", gma->offset.u.x, gma->offset.u.y, gma->offset.u.z);
	/* Set segment descriptor associated to kernel space */
	fp = filp_open(offset, O_RDWR | O_CREAT, 0666);
	if(IS_ERR(fp))
		dev_err(&gma->client->dev, "filp_open %s error!!.\n", offset);
	else{
		fs = get_fs();
		//set_fs(KERNEL_DS);
		set_fs(get_ds());
		dev_info(&gma->client->dev, "filp_open %s SUCCESS!!.\n", offset);
 		ret = fp->f_op->write(fp,w_buf,20,&fp->f_pos);
	}
	filp_close(fp,NULL);
/*#ifdef GMA_PERMISSION_THREAD
	fs = get_fs();
	set_fs(KERNEL_DS);
	ret = sys_chmod(offset , 0666);
	ret = sys_fchmodat(AT_FDCWD, offset , 0666);
	set_fs(fs);
#endif*/
	return 0;
}

void GMA_ReadCalibration(struct gma_data *gma){
	unsigned int orgfs;
	char buffer[20];
	struct file *fp,*fp2;/* *fp open offset.txt , *fp2 open path2 offset.txt */
	orgfs = get_fs();
	/* Set segment descriptor associated to kernel space */
	set_fs(KERNEL_DS);
	dev_err(&gma->client->dev, "============= !\n");

	fp = filp_open(GMA_Offset_TXT, O_RDWR , 0);
	if(IS_ERR(fp)){
		dev_err(&gma->client->dev, "Sorry,file open ERROR !\n");

		fp2 = filp_open(GMA_Offset_TXT_PATH2, O_RDWR , 0);
		if(IS_ERR(fp2)){
			dev_err(&gma->client->dev, "Sorry,path2 file open ERROR !\n");

			//dev_info(&gma->client->dev, "ABS(gma->accel_data.u.x)/y/z= %05d %05d %05d\n", 
						//ABS(gma->accel_data.u.x), ABS(gma->accel_data.u.y), ABS(gma->accel_data.u.z));
#if 1//AutoZeroZ
			//if ( ABS(gma->accel_data.u.x) < LevelValueRange_2_0 && ABS(gma->accel_data.u.y) < LevelValueRange_2_0)
				gma_acc_calibration(gma, GRAVITY_ON_Z_POSITIVE);
#if 0// AutoZeroY        
			else if( ABS(gma->accel_data.u.x) > GMS_DEFAULT_SENSITIVITY)
				gma_acc_calibration(gma, GRAVITY_ON_Y_AUTO);
#endif
#if 0//AutoZeroX
			else if( ABS(gma->accel_data.u.y) > GMS_DEFAULT_SENSITIVITY)
				gma_acc_calibration(gma, GRAVITY_ON_X_AUTO);
#endif
			GMA_WriteCalibration(gma , GMA_Offset_TXT);
			GMA_WriteCalibration(gma , GMA_Offset_TXT_PATH2);
#endif
		}
		else{
			dev_dbg(&gma->client->dev, "filp_open %s SUCCESS!!.\n",GMA_Offset_TXT_PATH2);
			fp2->f_op->read( fp2, buffer, 20, &fp2->f_pos); // read offset.txt
			sscanf(buffer,"%d %d %d",&gma->offset.u.x,&gma->offset.u.y,&gma->offset.u.z);
			dev_info(&gma->client->dev, "offset.u.x/offset.u.y/offset.u.z = %d/%d/%d\n",
								gma->offset.u.x, gma->offset.u.y, gma->offset.u.z);
			filp_close(fp2,NULL);
		}
	}
	else{
		dev_dbg(&gma->client->dev, "filp_open %s SUCCESS!!.\n",GMA_Offset_TXT);
		fp->f_op->read( fp, buffer, 20, &fp->f_pos); // read offset.txt
		sscanf(buffer,"%d %d %d",&gma->offset.u.x,&gma->offset.u.y,&gma->offset.u.z);
		dev_info(&gma->client->dev, "offset.u.x/offset.u.y/offset.u.z = %d/%d/%d\n",
							gma->offset.u.x, gma->offset.u.y, gma->offset.u.z);
		filp_close(fp,NULL);
	}
	set_fs(orgfs);
}

/*
static struct i2c_board_info sensor_board_info={
    .type = GSENSOR_ID,
    .addr = SENSOR_I2C_ADDR,
};
 static struct i2c_client *client;
*/
static int __init gma_acc_init(void){
/*
	struct i2c_adapter *i2c_adap;
    unsigned int cfg_i2c_adap_id;
    int    ret;
    cfg_i2c_adap_id = 4;

    i2c_adap = i2c_get_adapter(cfg_i2c_adap_id);
    if (!i2c_adap) {
        printk(KERN_ERR "%s: can't get i2c adapter\n", __func__);
        ret = -ENODEV;
        goto i2c_err;
    }
    client = i2c_new_device(i2c_adap, &sensor_board_info);
    if (!client) {
        printk(KERN_ERR "%s:  can't add i2c device at 0x%x\n",
                __FUNCTION__, (unsigned int)sensor_board_info.addr);
        ret = -ENODEV;
        goto i2c_err;
    }
    i2c_put_adapter(i2c_adap);
	pr_info("ACC driver: init\n");
*/	
    GSE_LOG("GMA30x ACC driver: initialize.\n");
	return i2c_add_driver(&gma_i2c_driver);
//i2c_err:
    //return ret;
}
module_init(gma_acc_init);

static void __exit gma_acc_exit(void){
	i2c_del_driver(&gma_i2c_driver);
	GSE_LOG("GMA30x ACC driver: release.\n");
#ifdef GMA_PERMISSION_THREAD
	if(GMAPermissionThread)
		GMAPermissionThread = NULL;
#endif // GMA_PERMISSION_THREAD	
}
module_exit(gma_acc_exit);

MODULE_AUTHOR("GlobalMems-inc");
MODULE_DESCRIPTION("GMA30x ACC Driver");
MODULE_LICENSE("GPL");

