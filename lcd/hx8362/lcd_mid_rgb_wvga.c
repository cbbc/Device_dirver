/* drivers/video/sc8810/lcd_nt35516.c
 *
 *
 *
 *
 * Copyright (C) 2010 Spreadtrum
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include "../sprdfb_panel.h"
//#include <linux/sprd_baker_printk.h>
#define  LCD_DEBUG
#ifdef LCD_DEBUG
#define LCD_PRINT printk
#else
#define LCD_PRINT(...)
#endif

//#define LCM_TAG_SHIFT 24
//#define LCM_SLEEP(ms) ((2 << LCM_TAG_SHIFT)| ms)
 
#define HX8362_SpiWriteCmd(cmd) \ 
{ \
	spi_send_cmd((cmd& 0xFF));\
}

#define  HX8362_SpiWriteData(data)\
{ \
	spi_send_data((data& 0xFF));\
}

extern void set_rgblcd_power(uint32_t value);//modified by hzy,2013411

static int32_t hx8362_init(struct panel_spec *self)
{
	uint32_t data = 0;
	spi_send_cmd_t spi_send_cmd = self->info.rgb->bus_info.spi->ops->spi_send_cmd; 
	spi_send_data_t spi_send_data = self->info.rgb->bus_info.spi->ops->spi_send_data; 
	spi_read_t spi_read = self->info.rgb->bus_info.spi->ops->spi_read; 

	printk("[baker]hx8362_init\n");
	set_rgblcd_power(1);
		msleep(120);
		return 0;//modified by hzy,2013411

	

}

static int32_t hx8362_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	spi_send_cmd_t spi_send_cmd = self->info.rgb->bus_info.spi->ops->spi_send_cmd; 
	spi_send_data_t spi_send_data = self->info.rgb->bus_info.spi->ops->spi_send_data; 
	
	if(is_sleep==1){

		set_rgblcd_power(0);//modified by hzy,2013411
	msleep(120);
	return 0;
	}else{
	set_rgblcd_power(1);//modified by hzy,2013411
	mdelay(120);
	return 0;
	}
}




static int32_t hx8362_set_window(struct panel_spec *self,
		uint16_t left, uint16_t top, uint16_t right, uint16_t bottom)
{
	spi_send_cmd_t spi_send_cmd = self->info.rgb->bus_info.spi->ops->spi_send_cmd; 
	spi_send_data_t spi_send_data = self->info.rgb->bus_info.spi->ops->spi_send_data; 

	LCD_PRINT("nt35516_set_window: %d, %d, %d, %d\n",left, top, right, bottom);
return 0;//modified by hzy,2013411

}
static int32_t hx8362_invalidate(struct panel_spec *self)
{
	LCD_PRINT("nt35516_invalidate\n");

	return self->ops->panel_set_window(self, 0, 0,
		self->width - 1, self->height - 1);
}



static int32_t hx8362_invalidate_rect(struct panel_spec *self,
				uint16_t left, uint16_t top,
				uint16_t right, uint16_t bottom)
{
	LCD_PRINT("hx8362_invalidate_rect \n");

	return self->ops->panel_set_window(self, left, top,
			right, bottom);
}

static int32_t hx8362_read_id(struct panel_spec *self)
{
	int32_t id  = 0xAA;
	LCD_PRINT("hx8362_read id \n");

	return id;
}

static struct panel_operations lcd_hx8362_operations = {
	.panel_init = hx8362_init,
	.panel_set_window = hx8362_set_window,
	.panel_invalidate_rect= hx8362_invalidate_rect,
	.panel_invalidate = hx8362_invalidate,
	.panel_enter_sleep = hx8362_enter_sleep,
	.panel_readid          = hx8362_read_id
};
#if 0
#if 0
static struct timing_rgb lcd_hx8362_rgb_timing = {
	.hfp = 210,//10,  /* unit: pixel */
	.hbp = 46,//6,
	.hsync = 33,//8,
	.vfp = 7,//3, /*unit: line*/
	.vbp = 23,//3,
	.vsync = 20,//4,
};
#endif
static struct timing_rgb lcd_hx8362_rgb_timing = {
	.hfp = 10,//10,  /* unit: pixel */
	.hbp = 6,//6,
	.hsync = 10,//8,
	.vfp = 3,//3, /*unit: line*/
	.vbp = 3,//3,
	.vsync = 12,//4,
};
#else



static struct timing_rgb lcd_hx8362_rgb_timing = {
#if 0
	.hfp = 16,//210,//40,//10,  /* unit: pixel */
	.hbp = 46,//40,//88,//6,
	.hsync = 1,//32,//6,//48,//8, // baker ≈‰÷√(x=0) µƒœ‘ æŒª÷√BP+Hsync=56
	.vfp = 7,//13,//3, /*unit: line*/
	.vbp = 23,//32,//3,
	.vsync = 1,//
#endif
	// WTC0503G03-12
	/*.hfp = 40,//10,  // unit: pixel
	.hbp = 40,//6,
	.hsync = 48,//8,
	.vfp = 13,//3, // unit: line
	.vbp = 29,//3,
	.vsync = 3,//4,*/

	// WTQ05027D03-12
	.hfp = 210,
	.hbp = 46,
	.hsync = 20,
	.vfp = 22,
	.vbp = 23,
	.vsync = 10,

	/*.hfp = 40,//10,  // unit: pixel
	.hbp = 40,//6,
	.hsync = 48,//8,
	.vfp = 13,//3, // unit: line
	.vbp = 29,//3,
	.vsync = 3,//4,*/
};

#endif

static struct spi_info lcd_nt35516_rgb_spi_info = {
	.ops = NULL,
};

static struct info_rgb lcd_hx8362_rgb_info = {
	.cmd_bus_mode  = SPRDFB_RGB_BUS_TYPE_SPI,
	.video_bus_width = 24, /*18,16*/
	.h_sync_pol = SPRDFB_POLARITY_NEG,
	.v_sync_pol = SPRDFB_POLARITY_NEG,
	.de_pol = SPRDFB_POLARITY_POS,
	.timing = &lcd_hx8362_rgb_timing,
	.bus_info = {
		.spi = &lcd_nt35516_rgb_spi_info,
	}
};

//lcd_panel_nt35516
#if 0
struct panel_spec lcd_mid_rgb_spec = {
	.width = 800,
	.height = 480,//854,//modified by hzy,2013411
	.fps = 58,
	.type = LCD_MODE_RGB,
	.direction = LCD_DIRECT_NORMAL,
	.info = {
		.rgb = &lcd_hx8362_rgb_info
	},
	.ops = &lcd_hx8362_operations,
};
#else
struct panel_spec lcd_mid_rgb_spec = {
	.width = 800,
	.height = 480,//854,//modified by hzy,2013411 
	.fps = 70,//70,// 63,//58,//ptk 20130604  
	.type = LCD_MODE_RGB,
	.direction = LCD_DIRECT_NORMAL,//LCD_DIRECT_ROT_90,//LCD_DIRECT_NORMAL,
	.info = {
		.rgb = &lcd_hx8362_rgb_info
	},
	.ops = &lcd_hx8362_operations,
};

#endif

struct panel_cfg lcd_mid_rgb = {
	/* this panel can only be main lcd */
	.dev_id = SPRDFB_MAINLCD_ID,
	.lcd_id = 0xaa,
	.lcd_name = "lcd_mid_rgb",
	.panel = &lcd_mid_rgb_spec,
};

static int __init lcd_mid_rgb_init(void)
{
	return sprdfb_panel_register(&lcd_mid_rgb);
}

subsys_initcall(lcd_mid_rgb_init);

