/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   lcd.c
 *
 * Project:
 * --------
 *   Maui_sw
 *
 * Description:
 * ------------
 *   This Module defines the LCD driver.
 *
 * Author:
 	Hesong Li
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *
 * removed!
 * removed!
 * removed!
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include "drv_comm.h"
#include "reg_base.h"
#include "lcd_sw_inc.h"
#include "lcd_sw.h"
#include "lcd_hw.h"
/*Serial interface*/
#include "lcd_if_hw.h"
#include "lcd_if.h"
#include "eint.h"

#ifdef LQT_SUPPORT/*Do not remove LQT code segment*/ 
#include "lcd_lqt.h"
extern kal_uint8 lcd_at_mode;
extern kal_bool lcd_update_permission;
#endif/*LQT_SUPPORT*/



kal_bool  lcd_assert_fail=KAL_FALSE;

#ifdef DUAL_LCD
kal_uint16 lcd_power_ctrl_shadow1;
#endif

void LCD_Delay(kal_uint32 delay_count)
{
	volatile kal_uint32 delay;
	for (delay =0;delay <(delay_count*4);delay++) {}
}

extern kal_uint32 drv_get_current_time(void);
extern kal_uint32 drv_get_duration_ms(kal_uint32 previous_time);
void Delayms(kal_uint16 data)
{

    kal_uint32 time1;
    
    time1 = drv_get_current_time();
    while (drv_get_duration_ms(time1) <= data);
}

void LCD_set_reg(kal_uint16 reg_index, kal_uint16 reg_data)
{

}

void spi_ini(void)
{
}

#if 1//gj 
kal_bool afterlog=0;
typedef void (*LCD_REFLESH_FUNC) (kal_uint16 x1,kal_uint16 y1,kal_uint16 x2,kal_uint16 y2);
static LCD_REFLESH_FUNC lcd_reflesh_cb = NULL;
static kal_bool need_reflesh = KAL_FALSE;
static kal_uint16 sx1,sx2,sy1,sy2;

void Lcd_set_callback_parameter(kal_uint16 x1,kal_uint16 y1,kal_uint16 x2, kal_uint16 y2, LCD_REFLESH_FUNC func)
{
	sx1=x1,
	sx2=x2,
	sy1=y1,
	sy2=y2,
	lcd_reflesh_cb = func;
	need_reflesh = KAL_TRUE;
}

kal_bool Lcd_reflesh_handle()
{
	if(need_reflesh && lcd_reflesh_cb != NULL)
	{
		lcd_reflesh_cb(sx1,sy1,sx2,sy2);
		//LCD_SET_SW_TE;
		lcd_reflesh_cb = NULL;
		need_reflesh = KAL_FALSE;
		return KAL_TRUE;
	}
		return KAL_FALSE;

}
#endif


void init_lcd_interface(void)
{
	kal_uint32 backup_val;	
#ifdef DUAL_LCD
	#if (defined(MT6218B)||defined(MT6217)||defined(MT6219))
		GPIO_ModeSetup(24, 1);
	#elif (defined(MT6226)||defined(MT6227)||defined(MT6228)||defined(MT6229)||defined(MT6268T)||defined(MT6268H)||defined(MT6230)||defined(MT6235)||defined(MT6235B)||defined(MT6238)||defined(MT6239)||defined(MT6268A)||defined(MT6239)||defined(MT6268))
		GPIO_ModeSetup(25, 1);
  		#ifdef SERIAL_SUBLCD

		GPIO_ModeSetup(14, 1); //LCD serial clock LSCK
		GPIO_ModeSetup(15, 1); //serial LCD Address Signal LSA0
		GPIO_ModeSetup(16, 1); //serial LCD Data Signal LSDA
		GPIO_ModeSetup(17, 1); //serial LCD chip select 0 LSCE0#
  		#endif
	#endif
#endif    
  /// Turn on LCD clock
#if defined(__OLD_PDN_DEFINE__)
	DRV_Reg(DRVPDN_CON1_CLR) = DRVPDN_CON1_LCD;
	//modifed for new clock gating
#elif defined(__CLKG_DEFINE__)
	DRV_Reg(MMCG_CLR0) = MMCG_CON0_LCD;
#endif

	REG_LCD_ROI_CTRL=0;
#if (defined(MT6218B))
	SET_LCD_PARALLEL_CE2WR_SETUP_TIME(3);
	SET_LCD_PARALLEL_CE2WR_HOLD_TIME(3);
	SET_LCD_PARALLEL_CE2RD_SETUP_TIME(0);
	SET_LCD_PARALLEL_WRITE_WAIT_STATE(5);
	SET_LCD_PARALLEL_READ_LATENCY_TIME(0);
	SET_LCD_ROI_CTRL_CMD_LATENCY(4);

	DISABLE_LCD_PARALLEL_SYNC;
#elif (defined(MT6219))
	SET_LCD_PARALLEL_CE2WR_SETUP_TIME((kal_uint32)0);
	SET_LCD_PARALLEL_CE2WR_HOLD_TIME(0);
	SET_LCD_PARALLEL_CE2RD_SETUP_TIME(0);
	SET_LCD_PARALLEL_WRITE_WAIT_STATE(4);
	SET_LCD_PARALLEL_READ_LATENCY_TIME(2);
	SET_LCD_ROI_CTRL_CMD_LATENCY(2);

	DISABLE_LCD_PARALLEL_SYNC;
#elif (defined(MT6223)||defined(MT6223P))
	CLEAR_LCD_CTRL_RESET_PIN;
	SET_LCD_PARALLEL_CE2WR_SETUP_TIME((kal_uint32)1);
	SET_LCD_PARALLEL_CE2WR_HOLD_TIME(1);
	SET_LCD_PARALLEL_CE2RD_SETUP_TIME(1);
	SET_LCD_PARALLEL_WRITE_WAIT_STATE(2);
	SET_LCD_PARALLEL_READ_LATENCY_TIME(2);
	SET_LCD_ROI_CTRL_CMD_LATENCY(2);

	#if (defined(MAIN_LCD_18BIT_MODE))
		SET_LCD_PARALLEL_18BIT_DATA_BUS;
		GPIO_ModeSetup(55, 1);
		*((volatile unsigned short *) 0x801201B0) |= 0x4000;
		*((volatile unsigned short *) 0x801201D0) |= 0x0001;
		SET_LCD_PARALLEL_18BIT_DATA_BUS;
	#elif (defined(MAIN_LCD_9BIT_MODE))
		SET_LCD_PARALLEL_9BIT_DATA_BUS; 
	#endif


#elif (defined(MT6226)||defined(MT6227)||defined(MT6228)||defined(MT6229)||defined(MT6268T)||defined(MT6268H)||defined(MT6230)||defined(MT6235)||defined(MT6235B)||defined(MT6238)||defined(MT6239)||defined(MT6268A)||defined(MT6239)||defined(MT6253T)||defined(MT6253)||defined(MT6253E)||defined(MT6253L)||defined(MT6252H)||defined(MT6252)||defined(MT6225)||defined(MT6268)||defined(MT6236)||defined(MT6236B)||defined(MT6255)||defined(MT6250)||defined(MT6276)||defined(MT6270A))
	
	#if (defined(MAIN_LCD_8BIT_MODE))
		//SET_LCD_PARALLEL_8BIT_DATA_BUS;
		SET_LCD_PARALLEL_DATA_BUS(0,0);   //gj 0817
	#elif (defined(MAIN_LCD_9BIT_MODE) || defined(MAIN_LCD_16BIT_MODE) || defined(MAIN_LCD_18BIT_MODE))
		#if (defined(MT6253T)||defined(MT6253)||defined(MT6253E)||defined(MT6253L)||defined(MT6252H)||defined(MT6252)||defined(MT6225))

	    	#if defined(MAIN_LCD_16BIT_MODE)
			  SET_LCD_PARALLEL_16BIT_DATA_BUS;
	    	#else
			  SET_LCD_PARALLEL_9BIT_DATA_BUS; 
	    	#endif

		#elif defined(MT6250)
			#if defined(MAIN_LCD_16BIT_MODE)
			  SET_LCD_PARALLEL_DATA_BUS(0,2);
	    	#else
			  SET_LCD_PARALLEL_DATA_BUS(0,1);
	    	#endif
		
		#endif
		#if (defined(MT6228)||defined(MT6229)||defined(MT6268T)||defined(MT6268H)||defined(MT6230))
			#ifndef MT6268H
			GPIO_ModeSetup(10, 1);
			GPIO_ModeSetup(11, 1);
			#endif
			GPIO_ModeSetup(54, 0);
			GPIO_ModeSetup(55, 0);
			GPIO_ModeSetup(56, 0);
			GPIO_ModeSetup(57, 0);
			GPIO_ModeSetup(58, 0);
			GPIO_ModeSetup(59, 0);
			GPIO_ModeSetup(60, 0);
			GPIO_ModeSetup(61, 0);
		#elif (defined(MT6226)||defined(MT6227))
			GPIO_ModeSetup(55, 1);
			*((volatile unsigned short *) 0x801201B0) |= 0x4000;
			*((volatile unsigned short *) 0x801201D0) |= 0x0001;
		#elif (defined(MT6235)||defined(MT6235B)||defined(MT6238)||defined(MT6239)||defined(MT6268A)||defined(MT6239)||defined(MT6253T)||defined(MT6268))
			GPIO_ModeSetup(20, 1);
			GPIO_ModeSetup(21, 1);
		#endif
	#endif

	#if (defined(MT6253T)||defined(MT6253)||defined(MT6225))
		  SET_LCD_PARALLEL_CE2WR_SETUP_TIME((kal_uint32)1);
		  SET_LCD_PARALLEL_CE2WR_HOLD_TIME(1);
		  SET_LCD_PARALLEL_CE2RD_SETUP_TIME(0);
		  SET_LCD_PARALLEL_WRITE_WAIT_STATE(3);
		  SET_LCD_PARALLEL_READ_LATENCY_TIME(10);
		  SET_LCD_ROI_CTRL_CMD_LATENCY(0);
	#elif (defined(MT6253E)||defined(MT6253L)||defined(MT6252H)||defined(MT6252)||defined(MT6250))
		#if defined(_LCM_ON_EMI_)//Tristan
		  SET_EMI_BANK2_CE2WR_SETUP_TIME(2);//2);
		  SET_EMI_BANK2_CE2WR_HOLD_TIME(2);//1);
		  SET_EMI_BANK2_CE2RD_SETUP_TIME(2);
		  SET_EMI_BANK2_WRITE_WAIT_STATE_TIME(3);//5);
		  SET_EMI_BANK2_READ_LATENCY_TIME(10);
		  SET_LCD_ROI_CTRL_CMD_LATENCY(2);	 
	 	#else
  
			#if (defined(MT6250))
				#if defined(__EMI_CLK_130MHZ__)
		  		//SET_LCD_PARALLEL_IF_TIMING(0, LCD_PARALLEL_CLOCK_104MHZ, 0, 1, 3, 2, 2, 16, 2);
		  		SET_LCD_PARALLEL_IF_TIMING(0, LCD_PARALLEL_CLOCK_104MHZ, 3, 3, 6, 2, 2, 16, 2);
				#else
				SET_LCD_PARALLEL_IF_TIMING(0, LCD_PARALLEL_CLOCK_104MHZ, 0, 1, 3, 2, 2, 16, 1);
		  		//SET_LCD_PARALLEL_IF_TIMING(0, LCD_PARALLEL_CLOCK_104MHZ, 3, 3, 6, 2, 2, 16, 2);
				#endif
			#else
				SET_LCD_PARALLEL_CE2WR_SETUP_TIME(2);
				SET_LCD_PARALLEL_CE2WR_HOLD_TIME(2);//1);
				SET_LCD_PARALLEL_CE2RD_SETUP_TIME(1);
				SET_LCD_PARALLEL_WRITE_WAIT_STATE(5);//5);
				SET_LCD_PARALLEL_READ_LATENCY_TIME(10);
				SET_LCD_ROI_CTRL_CMD_LATENCY(5)
		    #endif
	 	#endif
	#elif (defined(MT6268A)||defined(MT6268)||defined(MT6255)||defined(MT6276)) 
		  SET_LCD_PARALLEL_CE2WR_SETUP_TIME(0x1);//1
		  SET_LCD_PARALLEL_CE2WR_HOLD_TIME(0x1);//1
		  SET_LCD_PARALLEL_CE2RD_SETUP_TIME(0x3);//3
		  SET_LCD_PARALLEL_WRITE_WAIT_STATE(0x4);//2
		  SET_LCD_PARALLEL_READ_LATENCY_TIME(0x3);//4	 
		  SET_LCD_ROI_CTRL_CMD_LATENCY(0x0);//4    
	#elif (defined(MT6236)||defined(MT6236B)) 
		  SET_LCD_PARALLEL_CE2WR_SETUP_TIME(0x1);
		  SET_LCD_PARALLEL_CE2WR_HOLD_TIME(0x1);
		  SET_LCD_PARALLEL_CE2RD_SETUP_TIME(0x3);
		  SET_LCD_PARALLEL_WRITE_WAIT_STATE(0x4);
		  SET_LCD_PARALLEL_READ_LATENCY_TIME(0x3);	 
		  SET_LCD_ROI_CTRL_CMD_LATENCY(0x2);//Tianshu's comment for MT6236 ADMUX load		  
	#elif defined(MT6270A)      
		  SET_LCD_PARALLEL_CE2WR_SETUP_TIME((kal_uint32)2);
		  SET_LCD_PARALLEL_CE2WR_HOLD_TIME(1);
		  SET_LCD_PARALLEL_WRITE_WAIT_STATE(5);    
		  SET_LCD_PARALLEL_CE2RD_SETUP_TIME(2);
		  SET_LCD_PARALLEL_READ_LATENCY_TIME(20);
		  SET_LCD_ROI_CTRL_CMD_LATENCY(0);		
	#else
		
		#ifdef __WIFI_SUPPORT__
		  SET_LCD_PARALLEL_CE2WR_SETUP_TIME((kal_uint32)1);
		  SET_LCD_PARALLEL_CE2WR_HOLD_TIME(1);
		  SET_LCD_PARALLEL_CE2RD_SETUP_TIME(2);
		  SET_LCD_PARALLEL_WRITE_WAIT_STATE(3);
		  SET_LCD_PARALLEL_READ_LATENCY_TIME(10);
		  SET_LCD_ROI_CTRL_CMD_LATENCY(4);		
		#else
		  SET_LCD_PARALLEL_CE2WR_SETUP_TIME((kal_uint32)0);
		  SET_LCD_PARALLEL_CE2WR_HOLD_TIME(0);
		  SET_LCD_PARALLEL_CE2RD_SETUP_TIME(2);
		  SET_LCD_PARALLEL_WRITE_WAIT_STATE(2);
		  SET_LCD_PARALLEL_READ_LATENCY_TIME(10);
		  SET_LCD_ROI_CTRL_CMD_LATENCY(4);		
		#endif  

	#endif
	//SET_LCD_PARALLEL_CLOCK_52M;
	#if (defined(MT6236)||defined(MT6236B)) 
	  SET_LCD_PARALLEL0_CLOCK_104M;
	#else
		//SET_LCD_PARALLEL_CLOCK_52M;
	#endif ////(defined(MT6236))     
	  
	#if (defined(MT6236)||defined(MT6236B))
		CONFIG_LCD_CTRL_PIN_DRIVING_CURRNET(LCD_DRIVING_6MA);
		CONFIG_LCD_DATA_PIN_DRIVING_CURRNET(LCD_DRIVING_6MA);
	#elif (defined(MT6253E)||defined(MT6253L)||defined(MT6252H)||defined(MT6252)||defined(MT6250))
		#if (defined(MT6250_EVB))
		set_lcd_driving_current(LCD_DRIVING_8MA);
		#else
		set_lcd_driving_current(LCD_DRIVING_8MA);
		#endif
	#endif
		  
	#if (defined(MT6226)||defined(MT6227)||defined(MT6229)||defined(MT6268T)||defined(MT6268H)||defined(MT6230)||defined(MT6235)||defined(MT6235B)||defined(MT6238)||defined(MT6239)||defined(MT6268A)||defined(MT6239)||defined(MT6253T)||defined(MT6253)||defined(MT6225)||defined(MT6268)||defined(MT6255)||defined(MT6276)||defined(MT6236)||defined(MT6236B)||defined(MT6270A))
	  //MT6253E, MT6253L, MT6255 and MT6276 have no layer gamma
	  SET_LCD_PARALLEL_GAMMA_R_TABLE(LCD_PARALLEL_GAMMA_DISABLE);
	  SET_LCD_PARALLEL_GAMMA_G_TABLE(LCD_PARALLEL_GAMMA_DISABLE);
	  SET_LCD_PARALLEL_GAMMA_B_TABLE(LCD_PARALLEL_GAMMA_DISABLE);
	#endif
	
	#ifdef DUAL_LCD
	  SET_LCD_PARALLEL1_CE2WR_SETUP_TIME((kal_uint32)0);
	  SET_LCD_PARALLEL1_CE2WR_HOLD_TIME(0);
	  SET_LCD_PARALLEL1_CE2RD_SETUP_TIME(2);
	  SET_LCD_PARALLEL1_WRITE_WAIT_STATE(1);
	  SET_LCD_PARALLEL1_READ_LATENCY_TIME(10);
	  SET_LCD_ROI_CTRL_CMD_LATENCY(0);
	  
		#if (defined(SUB_LCD_8BIT_MODE))
			SET_LCD_PARALLEL1_8BIT_DATA_BUS;	  
		#elif (defined(SUB_LCD_9BIT_MODE))
			SET_LCD_PARALLEL1_9BIT_DATA_BUS;
		#elif (defined(SUB_LCD_16BIT_MODE))
			SET_LCD_PARALLEL1_16BIT_DATA_BUS; 
		#elif (defined(SUB_LCD_18BIT_MODE))
			SET_LCD_PARALLEL1_18BIT_DATA_BUS;
		#ifndef MT6268H  
			GPIO_ModeSetup(10, 1);
			GPIO_ModeSetup(11, 1);
		#endif
		#endif
		
	  	SET_LCD_PARALLEL1_CLOCK_52M;
		
		#if (defined(MT6226)||defined(MT6227)||defined(MT6229)||defined(MT6268T)||defined(MT6268H)||defined(MT6230)||defined(MT6235)||defined(MT6235B)||defined(MT6238)||defined(MT6239)||defined(MT6268A)||defined(MT6239)||defined(MT6253T)||defined(MT6253)||defined(MT6225)||defined(MT6268)||defined(MT6255)||defined(MT6276)||defined(MT6236)||defined(MT6236B)||defined(MT6270A))
		  //MT6253E, MT6253L, MT6255 and MT6276 have no layer gamma
		  SET_LCD_PARALLEL1_GAMMA_TABLE(LCD_PARALLEL_GAMMA_DISABLE);
		#endif
	#endif
#elif (defined(MT6255)||defined(MT6276))
	  //************Interface Bus***************
  #if (defined(MAIN_LCD_8BIT_MODE))
	  SET_LCD_PARALLEL_DATA_BUS(0, LCD_PARALLEL_BUS_WIDTH_8BIT);//SET_LCD_PARALLEL_8BIT_DATA_BUS;
  #elif (defined(MAIN_LCD_9BIT_MODE))
	  SET_LCD_PARALLEL_DATA_BUS(0, LCD_PARALLEL_BUS_WIDTH_9BIT);//SET_LCD_PARALLEL_9BIT_DATA_BUS;
  #elif (defined(MAIN_LCD_16BIT_MODE))
	  SET_LCD_PARALLEL_DATA_BUS(0, LCD_PARALLEL_BUS_WIDTH_16BIT);//SET_LCD_PARALLEL_16BIT_DATA_BUS; 		   
  #elif (defined(MAIN_LCD_18BIT_MODE))
	  SET_LCD_PARALLEL_DATA_BUS(0, LCD_PARALLEL_BUS_WIDTH_18BIT);//SET_LCD_PARALLEL_18BIT_DATA_BUS;
  #endif
	  //************Interface Timing*************** 		 
	  SET_LCD_PARALLEL_IF_TIMING(0, 0, 1, 1, 4, 3, 3, 0);		  
	  DISABLE_LCD_PARALLEL0_GAMMA(backup_val);
#elif (defined(MT6217))
#endif


}
	


#ifdef LQT_SUPPORT/*Do not remove LQT code segment*/
/*************************************************************************
* FUNCTION
*   LCD_gamma_test() and LCD_flicker_test()
*
* DESCRIPTION
*   Generating test pattern by AT commands.
*
* PARAMETERS
*   level, color
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void LCD_gamma_test(kal_uint8 level, kal_uint8 color)
{
	kal_uint16 LCD_x;
	kal_uint16 LCD_y;
	kal_uint16 r_color,g_color,b_color,w_color;
    kal_uint32 lcd_layer0_buffer;	

  		  		
	lcd_layer0_buffer=REG_LCD_LAYER0_BUFF_ADDR;
	r_color=(level>>1)<<11;	/* transfer to RGB565 */
	g_color=level<<5;
	b_color=level>>1;
	w_color=(r_color|g_color|b_color);
	for(LCD_y=0;LCD_y<LCD_HEIGHT;LCD_y++)
	{
		for(LCD_x=0;LCD_x<LCD_WIDTH/2;LCD_x++)
		{
			switch(color)
			{
	      case 0:
					*((kal_uint32 *)lcd_layer0_buffer+LCD_y*(LCD_WIDTH/2)+LCD_x)= ((w_color<<16)|w_color);
					break;					
				case 1:
					*((kal_uint32 *)lcd_layer0_buffer+LCD_y*(LCD_WIDTH/2)+LCD_x)= ((r_color<<16)|r_color);
					break;
	      case 2:
					*((kal_uint32 *)lcd_layer0_buffer+LCD_y*(LCD_WIDTH/2)+LCD_x)= ((g_color<<16)|g_color);
					break;
	      case 3:
					*((kal_uint32 *)lcd_layer0_buffer+LCD_y*(LCD_WIDTH/2)+LCD_x)= ((b_color<<16)|b_color);
					break;

	      default:
	         ASSERT(0);
			}
		}
	}		
}

void LCD_flicker_test(kal_uint8 level)
{
		kal_uint16 LCD_x;
		kal_uint16 LCD_y;
		kal_uint16 r_color,g_color,b_color,w_color;
    kal_uint32 lcd_layer0_buffer;	
    	
	  lcd_layer0_buffer=REG_LCD_LAYER0_BUFF_ADDR;
	  r_color=(level>>1)<<11;	/* transfer to RGB565 */
	  g_color=level<<5;
	  b_color=level>>1;
	  w_color=(r_color|g_color|b_color);
		
		for(LCD_y=0;LCD_y<LCD_HEIGHT;LCD_y++)
		{
			if(LCD_y&0x1)
			{
				for(LCD_x=0;LCD_x<LCD_WIDTH/2;LCD_x++)
						*((kal_uint32 *)lcd_layer0_buffer+LCD_y*(LCD_WIDTH/2)+LCD_x)= ((w_color<<16)|w_color);
			}
			else
			{
				for(LCD_x=0;LCD_x<LCD_WIDTH/2;LCD_x++)
					*((kal_uint32 *)lcd_layer0_buffer+LCD_y*(LCD_WIDTH/2)+LCD_x)=0x0;
			}
		}	
}
#endif/*LQT_SUPPORT*/


//LCD OTM4001A
#define SHBS_LCD_IC_OTM8009_ID 0x5420

void LCD_CtrlWrite_OTM8009A(kal_uint16 reg_index)
{
	LCD_CtrlWrite_MAINLCD((reg_index&0xFF00)>>8);
	LCD_CtrlWrite_MAINLCD(reg_index&0xFF);
}


void LCD_DataWrite_OTM8009A(kal_uint16 reg_data)
{
	LCD_DataWrite_MAINLCD(0x00);
	LCD_DataWrite_MAINLCD(reg_data&0xFF);
}

void LCD_SET_REG_OTM8009A(kal_uint16 reg_index, kal_uint16 reg_data)
{
	LCD_CtrlWrite_OTM8009A(reg_index);
	LCD_DataWrite_OTM8009A(reg_data);
}

kal_uint16 Lcd_Read_ID_OTM8009A(void)
{
	kal_uint16 lcd_id=0;
	kal_uint8  lcd_id1=0; 
	kal_uint8  lcd_id2=0;
	kal_uint8  lcd_id3=0;
	kal_uint8  lcd_id4=0;
	kal_uint8  lcd_id5=0;

	
	LCD_CtrlWrite_OTM8009A(0xa100);
	LCD_DataRead_MAINLCD(lcd_id1);
	LCD_DataRead_MAINLCD(lcd_id2);
	LCD_DataRead_MAINLCD(lcd_id3);
	LCD_DataRead_MAINLCD(lcd_id4);
	LCD_DataRead_MAINLCD(lcd_id5);

	lcd_id = (lcd_id4<<8)|lcd_id5;

	return lcd_id;
}

void LCD_Init_OTM8009A(void)
{
//vcom=vcom-1;	
LCD_CtrlWrite_OTM8009A(0xff00);  LCD_DataWrite_OTM8009A(0x80);
LCD_CtrlWrite_OTM8009A(0xff01);  LCD_DataWrite_OTM8009A(0x09);//enable EXTC
LCD_CtrlWrite_OTM8009A(0xff02);  LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0xff80);  LCD_DataWrite_OTM8009A(0x80);//enable Orise mode
LCD_CtrlWrite_OTM8009A(0xff81);  LCD_DataWrite_OTM8009A(0x09);

LCD_CtrlWrite_OTM8009A(0xC582);  LCD_DataWrite_OTM8009A(0xA3);//REG-pump23
LCD_CtrlWrite_OTM8009A(0xC590);  LCD_DataWrite_OTM8009A(0x96);//Pump setting (3x=D6)-->(2x=96)//v02 01/11
LCD_CtrlWrite_OTM8009A(0xC591);  LCD_DataWrite_OTM8009A(0x8F);//Pump setting(VGH/VGL)   	
LCD_CtrlWrite_OTM8009A(0xD800);  LCD_DataWrite_OTM8009A(0x75);//GVDD=4.5V  73   
LCD_CtrlWrite_OTM8009A(0xD801);  LCD_DataWrite_OTM8009A(0x73);//NGVDD=4.5V 71  
//VCOMDC  	
LCD_CtrlWrite_OTM8009A(0xD900);  LCD_DataWrite_OTM8009A(0x4E);//0x73
LCD_CtrlWrite_OTM8009A(0xC0B4);  LCD_DataWrite_OTM8009A(0x50);//column inversion 	                            
								
LCD_CtrlWrite_OTM8009A(0xE100);  LCD_DataWrite_OTM8009A(0x09);
LCD_CtrlWrite_OTM8009A(0xE101);  LCD_DataWrite_OTM8009A(0x0a);
LCD_CtrlWrite_OTM8009A(0xE102);  LCD_DataWrite_OTM8009A(0x0e);
LCD_CtrlWrite_OTM8009A(0xE103);  LCD_DataWrite_OTM8009A(0x0d);
LCD_CtrlWrite_OTM8009A(0xE104);  LCD_DataWrite_OTM8009A(0x07);
LCD_CtrlWrite_OTM8009A(0xE105);  LCD_DataWrite_OTM8009A(0x1A);
LCD_CtrlWrite_OTM8009A(0xE106);  LCD_DataWrite_OTM8009A(0x0d);
LCD_CtrlWrite_OTM8009A(0xE107);  LCD_DataWrite_OTM8009A(0x0d);
LCD_CtrlWrite_OTM8009A(0xE108);  LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xE109);  LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xE10A);  LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xE10B);  LCD_DataWrite_OTM8009A(0x07);
LCD_CtrlWrite_OTM8009A(0xE10C);  LCD_DataWrite_OTM8009A(0x0F);
LCD_CtrlWrite_OTM8009A(0xE10D);  LCD_DataWrite_OTM8009A(0x23);
LCD_CtrlWrite_OTM8009A(0xE10E);  LCD_DataWrite_OTM8009A(0x1E);
LCD_CtrlWrite_OTM8009A(0xE10F);  LCD_DataWrite_OTM8009A(0x05);
                      
LCD_CtrlWrite_OTM8009A(0xE200);  LCD_DataWrite_OTM8009A(0x09);
LCD_CtrlWrite_OTM8009A(0xE201);  LCD_DataWrite_OTM8009A(0x0a);
LCD_CtrlWrite_OTM8009A(0xE202);  LCD_DataWrite_OTM8009A(0x0e);
LCD_CtrlWrite_OTM8009A(0xE203);  LCD_DataWrite_OTM8009A(0x0d);
LCD_CtrlWrite_OTM8009A(0xE204);  LCD_DataWrite_OTM8009A(0x07);
LCD_CtrlWrite_OTM8009A(0xE205);  LCD_DataWrite_OTM8009A(0x1A);
LCD_CtrlWrite_OTM8009A(0xE206);  LCD_DataWrite_OTM8009A(0x0d);
LCD_CtrlWrite_OTM8009A(0xE207);  LCD_DataWrite_OTM8009A(0x0d);
LCD_CtrlWrite_OTM8009A(0xE208);  LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xE209);  LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xE20A);  LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xE20B);  LCD_DataWrite_OTM8009A(0x07);
LCD_CtrlWrite_OTM8009A(0xE20C);  LCD_DataWrite_OTM8009A(0x0F);
LCD_CtrlWrite_OTM8009A(0xE20D);  LCD_DataWrite_OTM8009A(0x23);
LCD_CtrlWrite_OTM8009A(0xE20E);  LCD_DataWrite_OTM8009A(0x1E);
LCD_CtrlWrite_OTM8009A(0xE20F);  LCD_DataWrite_OTM8009A(0x05);						

LCD_CtrlWrite_OTM8009A(0xC181); LCD_DataWrite_OTM8009A(0x77); //Frame rate 65Hz//V02   

// RGB I/F setting VSYNC for OTM8018 0x0e
LCD_CtrlWrite_OTM8009A(0xC1a1); LCD_DataWrite_OTM8009A(0x08); //external Vsync,Hsync,DE
LCD_CtrlWrite_OTM8009A(0xC489); LCD_DataWrite_OTM8009A(0x08);
LCD_CtrlWrite_OTM8009A(0xC0A2); LCD_DataWrite_OTM8009A(0x18);
LCD_CtrlWrite_OTM8009A(0xC0A3); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC0A4); LCD_DataWrite_OTM8009A(0x09);
LCD_CtrlWrite_OTM8009A(0xC481); LCD_DataWrite_OTM8009A(0x83);

LCD_CtrlWrite_OTM8009A(0xC592); LCD_DataWrite_OTM8009A(0x01);//Pump45
LCD_CtrlWrite_OTM8009A(0xC5B1); LCD_DataWrite_OTM8009A(0xA9);//DC voltage setting ;[0]GVDD output, default: 0xa8
	
LCD_CtrlWrite_OTM8009A(0xB392); LCD_DataWrite_OTM8009A(0x45);
LCD_CtrlWrite_OTM8009A(0xB390); LCD_DataWrite_OTM8009A(0x02);

LCD_CtrlWrite_OTM8009A(0xC080); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC081); LCD_DataWrite_OTM8009A(0x58);
LCD_CtrlWrite_OTM8009A(0xC082); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC083); LCD_DataWrite_OTM8009A(0x14);
LCD_CtrlWrite_OTM8009A(0xC084); LCD_DataWrite_OTM8009A(0x16);

LCD_CtrlWrite_OTM8009A(0xF5C1); LCD_DataWrite_OTM8009A(0xC0);
LCD_CtrlWrite_OTM8009A(0xC090); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC091); LCD_DataWrite_OTM8009A(0x44);
LCD_CtrlWrite_OTM8009A(0xC092); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC093); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC094); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC095); LCD_DataWrite_OTM8009A(0x03);

LCD_CtrlWrite_OTM8009A(0xC1A6); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC1A7); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xC1A8); LCD_DataWrite_OTM8009A(0x00);

LCD_CtrlWrite_OTM8009A(0xCE80); LCD_DataWrite_OTM8009A(0x86);
LCD_CtrlWrite_OTM8009A(0xCE81); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCE82); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCE83); LCD_DataWrite_OTM8009A(0x85);
LCD_CtrlWrite_OTM8009A(0xCE84); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCE85); LCD_DataWrite_OTM8009A(0x00);	                         
LCD_CtrlWrite_OTM8009A(0xCE86); LCD_DataWrite_OTM8009A(0x84);	                         
LCD_CtrlWrite_OTM8009A(0xCE87); LCD_DataWrite_OTM8009A(0x03);	                         
LCD_CtrlWrite_OTM8009A(0xCE88); LCD_DataWrite_OTM8009A(0x00);	                         
LCD_CtrlWrite_OTM8009A(0xCE89); LCD_DataWrite_OTM8009A(0x83);
LCD_CtrlWrite_OTM8009A(0xCE8a); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCE8b); LCD_DataWrite_OTM8009A(0x00);

LCD_CtrlWrite_OTM8009A(0xCEa0); LCD_DataWrite_OTM8009A(0x38);
LCD_CtrlWrite_OTM8009A(0xCEa1); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEa2); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEa3); LCD_DataWrite_OTM8009A(0x58);
LCD_CtrlWrite_OTM8009A(0xCEa4); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEa5); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEa6); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEa7); LCD_DataWrite_OTM8009A(0x38);
LCD_CtrlWrite_OTM8009A(0xCEa8); LCD_DataWrite_OTM8009A(0x02);
LCD_CtrlWrite_OTM8009A(0xCEa9); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEaa); LCD_DataWrite_OTM8009A(0x59);
LCD_CtrlWrite_OTM8009A(0xCEab); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEac); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEad); LCD_DataWrite_OTM8009A(0x00);

LCD_CtrlWrite_OTM8009A(0xCEb0); LCD_DataWrite_OTM8009A(0x38);
LCD_CtrlWrite_OTM8009A(0xCEb1); LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0xCEb2); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEb3); LCD_DataWrite_OTM8009A(0x5A);
LCD_CtrlWrite_OTM8009A(0xCEb4); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEb5); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEb6); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEb7); LCD_DataWrite_OTM8009A(0x38);
LCD_CtrlWrite_OTM8009A(0xCEb8); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEb9); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEba); LCD_DataWrite_OTM8009A(0x57);
LCD_CtrlWrite_OTM8009A(0xCEbb); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEbc); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEbd); LCD_DataWrite_OTM8009A(0x00);


LCD_CtrlWrite_OTM8009A(0xCEc0); LCD_DataWrite_OTM8009A(0x30);
LCD_CtrlWrite_OTM8009A(0xCEc1); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEc2); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEc3); LCD_DataWrite_OTM8009A(0x5C);
LCD_CtrlWrite_OTM8009A(0xCEc4); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEc5); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEc6); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEc7); LCD_DataWrite_OTM8009A(0x30);
LCD_CtrlWrite_OTM8009A(0xCEc8); LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0xCEc9); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEca); LCD_DataWrite_OTM8009A(0x5D);
LCD_CtrlWrite_OTM8009A(0xCEcb); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEcc); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEcd); LCD_DataWrite_OTM8009A(0x00);

LCD_CtrlWrite_OTM8009A(0xCEd0); LCD_DataWrite_OTM8009A(0x30);
LCD_CtrlWrite_OTM8009A(0xCEd1); LCD_DataWrite_OTM8009A(0x02);
LCD_CtrlWrite_OTM8009A(0xCEd2); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEd3); LCD_DataWrite_OTM8009A(0x5E);
LCD_CtrlWrite_OTM8009A(0xCEd4); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEd5); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEd6); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEd7); LCD_DataWrite_OTM8009A(0x30);
LCD_CtrlWrite_OTM8009A(0xCEd8); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEd9); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCEda); LCD_DataWrite_OTM8009A(0x5B);
LCD_CtrlWrite_OTM8009A(0xCEdb); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEdc); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCEdd); LCD_DataWrite_OTM8009A(0x00);


LCD_CtrlWrite_OTM8009A(0xCFc7); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0xCFc9); LCD_DataWrite_OTM8009A(0x00);

LCD_CtrlWrite_OTM8009A(0xCBc4); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBc5); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBc6); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBc7); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBc8); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBc9); LCD_DataWrite_OTM8009A(0x04);

LCD_CtrlWrite_OTM8009A(0xCBd9); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBda); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBdb); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBdc); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBdd); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCBde); LCD_DataWrite_OTM8009A(0x04);

LCD_CtrlWrite_OTM8009A(0xCC84); LCD_DataWrite_OTM8009A(0x0C);
LCD_CtrlWrite_OTM8009A(0xCC85); LCD_DataWrite_OTM8009A(0x0A);
LCD_CtrlWrite_OTM8009A(0xCC86); LCD_DataWrite_OTM8009A(0x10);
LCD_CtrlWrite_OTM8009A(0xCC87); LCD_DataWrite_OTM8009A(0x0E);
LCD_CtrlWrite_OTM8009A(0xCC88); LCD_DataWrite_OTM8009A(0x03);
LCD_CtrlWrite_OTM8009A(0xCC89); LCD_DataWrite_OTM8009A(0x04);

LCD_CtrlWrite_OTM8009A(0xCCa0); LCD_DataWrite_OTM8009A(0x09);
LCD_CtrlWrite_OTM8009A(0xCCa1); LCD_DataWrite_OTM8009A(0x0F);
LCD_CtrlWrite_OTM8009A(0xCCa2); LCD_DataWrite_OTM8009A(0x0D);
LCD_CtrlWrite_OTM8009A(0xCCa3); LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0xCCa4); LCD_DataWrite_OTM8009A(0x02);

LCD_CtrlWrite_OTM8009A(0xCCb4); LCD_DataWrite_OTM8009A(0x0D);
LCD_CtrlWrite_OTM8009A(0xCCb5); LCD_DataWrite_OTM8009A(0x0F);
LCD_CtrlWrite_OTM8009A(0xCCb6); LCD_DataWrite_OTM8009A(0x09);
LCD_CtrlWrite_OTM8009A(0xCCb7); LCD_DataWrite_OTM8009A(0x0B);
LCD_CtrlWrite_OTM8009A(0xCCb8); LCD_DataWrite_OTM8009A(0x02);
LCD_CtrlWrite_OTM8009A(0xCCb9); LCD_DataWrite_OTM8009A(0x01);

LCD_CtrlWrite_OTM8009A(0xCCce); LCD_DataWrite_OTM8009A(0x0E);

LCD_CtrlWrite_OTM8009A(0xCCd0); LCD_DataWrite_OTM8009A(0x10);
LCD_CtrlWrite_OTM8009A(0xCCd1); LCD_DataWrite_OTM8009A(0x0A);
LCD_CtrlWrite_OTM8009A(0xCCd2); LCD_DataWrite_OTM8009A(0x0C);
LCD_CtrlWrite_OTM8009A(0xCCd3); LCD_DataWrite_OTM8009A(0x04);
LCD_CtrlWrite_OTM8009A(0xCCd4); LCD_DataWrite_OTM8009A(0x03);

LCD_CtrlWrite_OTM8009A(0xff00); LCD_DataWrite_OTM8009A(0xff); 
LCD_CtrlWrite_OTM8009A(0xff01); LCD_DataWrite_OTM8009A(0xff); 
LCD_CtrlWrite_OTM8009A(0xff02); LCD_DataWrite_OTM8009A(0xff); 

//#ifdef DISPLAY_DIRECTION_0_MODE
LCD_CtrlWrite_OTM8009A(0x3400);
LCD_CtrlWrite_OTM8009A(0x3600);  LCD_DataWrite_OTM8009A(0x00);// Display Direction 0
LCD_CtrlWrite_OTM8009A(0x3500);  LCD_DataWrite_OTM8009A(0x00);// TE( Fmark ) Signal On
LCD_CtrlWrite_OTM8009A(0x4400);  LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0x4401);  LCD_DataWrite_OTM8009A(0x22);// TE( Fmark ) Signal Output Position
//#endif

/*
#ifdef DISPLAY_DIRECTION_180_MODE
LCD_CtrlWrite_OTM8009A(0x3600);  LCD_DataWrite_OTM8009A(0xD0);// Display Direction 180
LCD_CtrlWrite_OTM8009A(0x3500);  LCD_DataWrite_OTM8009A(0x00);// TE( Fmark ) Signal On
LCD_CtrlWrite_OTM8009A(0x4400);  LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0x4401);  LCD_DataWrite_OTM8009A(0xFF);// TE( Fmark ) Signal Output Position
#endif
*/
#ifdef LCD_BACKLIGHT_CONTROL_MODE
LCD_CtrlWrite_OTM8009A(0x5100);  LCD_DataWrite_OTM8009A(0xFF);// Backlight Level Control
LCD_CtrlWrite_OTM8009A(0x5300);  LCD_DataWrite_OTM8009A(0x2C);// Backlight On
LCD_CtrlWrite_OTM8009A(0x5500);  LCD_DataWrite_OTM8009A(0x00);// CABC Function Off
#endif

///=============================
LCD_CtrlWrite_OTM8009A(0x3A00); LCD_DataWrite_OTM8009A(0x55);     //MCU 16bits D[17:0]
LCD_CtrlWrite_OTM8009A(0x1100);
Delayms(150);	
LCD_CtrlWrite_OTM8009A(0x2900);
Delayms(200);
LCD_CtrlWrite_OTM8009A(0x2A00); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0x2A01); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0x2A02); LCD_DataWrite_OTM8009A(0x01);
LCD_CtrlWrite_OTM8009A(0x2A03); LCD_DataWrite_OTM8009A(0xDF);
LCD_CtrlWrite_OTM8009A(0x2B00); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0x2B01); LCD_DataWrite_OTM8009A(0x00);
LCD_CtrlWrite_OTM8009A(0x2B02); LCD_DataWrite_OTM8009A(0x03);  
LCD_CtrlWrite_OTM8009A(0x2B03); LCD_DataWrite_OTM8009A(0x1F);

LCD_CtrlWrite_OTM8009A(0x2C00); 
}

void LCD_EnterSleep_OTM8009A(void)
{
	//LCD_SET_REG_OTM4001A(0X0007,0X0000);
	//Delayms(100);
	//
	//LCD_SET_REG_OTM4001A(0X0102,0xD180);
	///Delayms(100);
}

void LCD_ExitSleep_OTM8009A(void)
{
	//*************Power On sequence ****************//
	LCD_Init_OTM8009A();

}

void LCD_BlockClear_OTM8009A(kal_uint16 x1,kal_uint16 y1,kal_uint16 x2,kal_uint16 y2,kal_uint16 data)
{
	kal_uint16 LCD_x;
	kal_uint16 LCD_y;

	LCD_CtrlWrite_OTM8009A(0x2A00); LCD_DataWrite_OTM8009A((x1&0xFF00)>>8);
	LCD_CtrlWrite_OTM8009A(0x2A01); LCD_DataWrite_OTM8009A(x1&0xFF);
	LCD_CtrlWrite_OTM8009A(0x2A02); LCD_DataWrite_OTM8009A((x2&0xFF00)>>8);
	LCD_CtrlWrite_OTM8009A(0x2A03); LCD_DataWrite_OTM8009A(x2&0xFF);
	LCD_CtrlWrite_OTM8009A(0x2B00); LCD_DataWrite_OTM8009A((y1&0xFF00)>>8);
	LCD_CtrlWrite_OTM8009A(0x2B01); LCD_DataWrite_OTM8009A(y1&0xFF);
	LCD_CtrlWrite_OTM8009A(0x2B02); LCD_DataWrite_OTM8009A((y2&0xFF00)>>8);
	LCD_CtrlWrite_OTM8009A(0x2B03); LCD_DataWrite_OTM8009A(y2&0xFF);


	LCD_CtrlWrite_OTM8009A(0x2C00);   

    for(LCD_y=y1;LCD_y<=y2;LCD_y++)
    {
      for(LCD_x=x1;LCD_x<=x2;LCD_x++)
      {
        #if (defined(MAIN_LCD_8BIT_MODE))
            *((volatile unsigned char *) MAIN_LCD_DATA_ADDR)=(kal_uint8)((data & 0xFF00)>>8);
            *((volatile unsigned char *) MAIN_LCD_DATA_ADDR)=(kal_uint8)(data & 0xFF);      
        #elif (defined(MAIN_LCD_9BIT_MODE))
            *((volatile unsigned short *) MAIN_LCD_DATA_ADDR)=((r_color<<3)|(g_color>>3));
            *((volatile unsigned short *) MAIN_LCD_DATA_ADDR)=((g_color&0x38)<<6)|b_color ;
        #elif (defined(MAIN_LCD_16BIT_MODE) || defined(MAIN_LCD_18BIT_MODE))
        #endif
      }
    }
}

void LCD_BlockWrite_Single_OTM8009A(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{
	lcd_assert_fail = KAL_TRUE;

	LCD_SET_ROI_OFFSET(startx+1024, starty+1024);
	LCD_SET_ROI_SIZE(endx-startx+1,endy-starty+1);
	
	SET_LCD_CMD_PARAMETER(0,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(1,LCD_CMD,0x00);
	
	SET_LCD_CMD_PARAMETER(2,LCD_DATA,0x00);
	SET_LCD_CMD_PARAMETER(3,LCD_DATA,(startx & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(4,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(5,LCD_CMD,0x01);
	
	SET_LCD_CMD_PARAMETER(6,LCD_DATA,0x00);
	SET_LCD_CMD_PARAMETER(7,LCD_DATA,(startx&0xFF));

	SET_LCD_CMD_PARAMETER(8,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(9,LCD_CMD,0x02);
	
	SET_LCD_CMD_PARAMETER(10,LCD_DATA,0x00);
	SET_LCD_CMD_PARAMETER(11,LCD_DATA,(endx & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(12,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(13,LCD_CMD,0x03);
	
	SET_LCD_CMD_PARAMETER(14,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(15,LCD_DATA,(endx&0xFF));

	SET_LCD_CMD_PARAMETER(16,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(17,LCD_CMD,0x00);
	
	SET_LCD_CMD_PARAMETER(18,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(19,LCD_DATA,(starty & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(20,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(21,LCD_CMD,0x01);
	SET_LCD_CMD_PARAMETER(22,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(23,LCD_DATA,(starty & 0xFF));

	SET_LCD_CMD_PARAMETER(24,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(25,LCD_CMD,0x02);
	SET_LCD_CMD_PARAMETER(26,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(27,LCD_DATA,(endy & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(28,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(29,LCD_CMD,0x03);
	SET_LCD_CMD_PARAMETER(30,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(31,LCD_DATA,(endy & 0xFF));

	SET_LCD_CMD_PARAMETER(32,LCD_CMD,0x2C);
	SET_LCD_CMD_PARAMETER(33,LCD_CMD,0x00);

	SET_LCD_ROI_CTRL_NUMBER_OF_CMD(34);
	if(1)
	{
		ENABLE_LCD_TRANSFER_COMPLETE_INT;
	}
	else
	{
		DISABLE_LCD_TRANSFER_COMPLETE_INT;
	}
	ENABLE_LCD_ROI_CTRL_CMD_FIRST;
	START_LCD_TRANSFER;

	lcd_assert_fail = KAL_FALSE;
	
}

void LCD_BlockWrite_OTM8009A(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{

	kal_uint16 h;
	h = endy - starty + 1;
	
	//ASSERT(lcd_assert_fail==KAL_FALSE);
	///lcd_assert_fail = KAL_TRUE;
	while ((LCD_IS_RUNNING)||(LCD_WAIT_TE)||(LCD_BUSY)){}

		//LCD_CLR_SW_TE;

	if(h>480)
	{
		kal_uint16 h1 = 480;
		
		LCD_BlockWrite_Single_OTM8009A(startx,starty,endx,starty+h1-1);
		if(1)
		{
			Lcd_set_callback_parameter(startx,starty+h1,endx,endy,LCD_BlockWrite_Single_OTM8009A);
		}
		else
		{
			afterlog = KAL_TRUE;
			
			while ((LCD_IS_RUNNING)||(LCD_WAIT_TE)||(LCD_BUSY)){}
			LCD_BlockWrite_Single_OTM8009A(startx,starty+h1,endx,endy);
		
		}


	}
	else
	{
		LCD_BlockWrite_Single_OTM8009A(startx,starty,endx,endy);

	}

}


//LCD RM68180
#define SHBS_LCD_IC_RM68180_ID 0x0080

void LCD_CtrlWrite_RM68180A(kal_uint16 reg_index)
{
	LCD_CtrlWrite_MAINLCD((reg_index&0xFF00)>>8);
	LCD_CtrlWrite_MAINLCD(reg_index&0xFF);
}


void LCD_DataWrite_RM68180A(kal_uint16 reg_data)
{
	LCD_DataWrite_MAINLCD((reg_data&0xFF00)>>8);
	LCD_DataWrite_MAINLCD(reg_data&0xFF);
}

void LCD_SET_REG_RM68180A(kal_uint16 reg_index, kal_uint16 reg_data)
{
	LCD_CtrlWrite_RM68180A(reg_index);
	LCD_DataWrite_RM68180A(reg_data);
}

kal_uint16 Lcd_Read_ID_RM68180A(void)
{
	kal_uint16 lcd_id=0;
	kal_uint8  lcd_id1=0; 
	kal_uint8  lcd_id2=0;
	kal_uint8  lcd_id3=0;
	kal_uint8  lcd_id4=0;
	kal_uint8  lcd_id5=0;

	
	LCD_CtrlWrite_RM68180A(0x0400);
	LCD_DataRead_MAINLCD(lcd_id1);
	LCD_DataRead_MAINLCD(lcd_id2);
	LCD_DataRead_MAINLCD(lcd_id3);

	lcd_id = (lcd_id1<<8)|lcd_id2;

	return lcd_id;
}

void LCD_Init_RM68180A(void)
{
	LCD_CtrlWrite_RM68180A(0xF000);
	LCD_DataWrite_RM68180A(0x0055);
	LCD_CtrlWrite_RM68180A(0xF001);
	LCD_DataWrite_RM68180A(0x00AA);
	LCD_CtrlWrite_RM68180A(0xF002);
	LCD_DataWrite_RM68180A(0x0052);
	LCD_CtrlWrite_RM68180A(0xF003);
	LCD_DataWrite_RM68180A(0x0008);
	LCD_CtrlWrite_RM68180A(0xF004);
	LCD_DataWrite_RM68180A(0x0001);

	// Setting AVDD Voltage
	LCD_CtrlWrite_RM68180A(0xB000);
	LCD_DataWrite_RM68180A(0x000D);   
	LCD_CtrlWrite_RM68180A(0xB001);
	LCD_DataWrite_RM68180A(0x000D);
	LCD_CtrlWrite_RM68180A(0xB002);
	LCD_DataWrite_RM68180A(0x000D);

	// Setting AVEE Voltage
	LCD_CtrlWrite_RM68180A(0xB100);
	LCD_DataWrite_RM68180A(0x000D);  
	LCD_CtrlWrite_RM68180A(0xB101);
	LCD_DataWrite_RM68180A(0x000D);
	LCD_CtrlWrite_RM68180A(0xB102);
	LCD_DataWrite_RM68180A(0x000D);

	// Setting VGH Voltag
	LCD_CtrlWrite_RM68180A(0xB300);
	LCD_DataWrite_RM68180A(0x000F);  
	LCD_CtrlWrite_RM68180A(0xB301);
	LCD_DataWrite_RM68180A(0x000F);
	LCD_CtrlWrite_RM68180A(0xB302);
	LCD_DataWrite_RM68180A(0x000F);

	// Setting VGL_REG Voltag
	LCD_CtrlWrite_RM68180A(0xB500);
	LCD_DataWrite_RM68180A(0x0008);
	LCD_CtrlWrite_RM68180A(0xB501);
	LCD_DataWrite_RM68180A(0x0008);
	LCD_CtrlWrite_RM68180A(0xB502);
	LCD_DataWrite_RM68180A(0x0008);

	// AVDD Ratio setting
	LCD_CtrlWrite_RM68180A(0xB600);
	LCD_DataWrite_RM68180A(0x0034);  
	LCD_CtrlWrite_RM68180A(0xB601);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xB602);
	LCD_DataWrite_RM68180A(0x0034);

	// AVEE Ratio setting
	LCD_CtrlWrite_RM68180A(0xB700);
	LCD_DataWrite_RM68180A(0x0034);  
	LCD_CtrlWrite_RM68180A(0xB701);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xB702);
	LCD_DataWrite_RM68180A(0x0034);

	// Power Control for VCL
	LCD_CtrlWrite_RM68180A(0xB800);
	LCD_DataWrite_RM68180A(0x0024);  
	LCD_CtrlWrite_RM68180A(0xB801);
	LCD_DataWrite_RM68180A(0x0024);
	LCD_CtrlWrite_RM68180A(0xB802);
	LCD_DataWrite_RM68180A(0x0024);

	// Setting VGH Ratio
	LCD_CtrlWrite_RM68180A(0xB900);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xB901);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xB902);
	LCD_DataWrite_RM68180A(0x0034);

	// VGLX Ratio
	LCD_CtrlWrite_RM68180A(0xBA00);
	LCD_DataWrite_RM68180A(0x0024);
	LCD_CtrlWrite_RM68180A(0xBA01);
	LCD_DataWrite_RM68180A(0x0024);
	LCD_CtrlWrite_RM68180A(0xBA02);
	LCD_DataWrite_RM68180A(0x0024);

	// Setting VGMP and VGSP Voltag
	LCD_CtrlWrite_RM68180A(0xBC00);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xBC01);
	LCD_DataWrite_RM68180A(0x0078);  
	LCD_CtrlWrite_RM68180A(0xBC02);
	LCD_DataWrite_RM68180A(0x0013);  

	// Setting VGMN and VGSN Voltage
	LCD_CtrlWrite_RM68180A(0xBD00);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xBD01);
	LCD_DataWrite_RM68180A(0x0078);  
	LCD_CtrlWrite_RM68180A(0xBD02);
	LCD_DataWrite_RM68180A(0x0013);  

	// Setting VCOM Offset Voltage
	LCD_CtrlWrite_RM68180A(0xBE00);  
	LCD_DataWrite_RM68180A(0x0000);  
	LCD_CtrlWrite_RM68180A(0xBE01);  
	LCD_DataWrite_RM68180A(0x005F);  

	// Control VGH booster voltage rang
	LCD_CtrlWrite_RM68180A(0xBF00);
	LCD_DataWrite_RM68180A(0x0001);


	//Gamma Setting
	LCD_CtrlWrite_RM68180A(0xD100);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD101);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD102);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD103);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD104);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD105);
	LCD_DataWrite_RM68180A(0x0014);
	LCD_CtrlWrite_RM68180A(0xD106);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD107);
	LCD_DataWrite_RM68180A(0x0031);
	LCD_CtrlWrite_RM68180A(0xD108);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD109);
	LCD_DataWrite_RM68180A(0x0051);
	LCD_CtrlWrite_RM68180A(0xD10A);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD10B);
	LCD_DataWrite_RM68180A(0x008C);
	LCD_CtrlWrite_RM68180A(0xD10C);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD10D);
	LCD_DataWrite_RM68180A(0x00BC);
	LCD_CtrlWrite_RM68180A(0xD10E);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD10F);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD110);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD111);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xD112);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD113);
	LCD_DataWrite_RM68180A(0x007C);
	LCD_CtrlWrite_RM68180A(0xD114);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD115);
	LCD_DataWrite_RM68180A(0x00AE);
	LCD_CtrlWrite_RM68180A(0xD116);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD117);
	LCD_DataWrite_RM68180A(0x00F9);
	LCD_CtrlWrite_RM68180A(0xD118);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD119);
	LCD_DataWrite_RM68180A(0x0032);
	LCD_CtrlWrite_RM68180A(0xD11A);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD11B);
	LCD_DataWrite_RM68180A(0x0033);
	LCD_CtrlWrite_RM68180A(0xD11C);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD11D);
	LCD_DataWrite_RM68180A(0x0065);
	LCD_CtrlWrite_RM68180A(0xD11E);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD11F);
	LCD_DataWrite_RM68180A(0x0096);
	LCD_CtrlWrite_RM68180A(0xD120);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD121);
	LCD_DataWrite_RM68180A(0x00B2);
	LCD_CtrlWrite_RM68180A(0xD122);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD123);
	LCD_DataWrite_RM68180A(0x00D3);
	LCD_CtrlWrite_RM68180A(0xD124);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD125);
	LCD_DataWrite_RM68180A(0x00E7);
	LCD_CtrlWrite_RM68180A(0xD126);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD127);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD128);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD129);
	LCD_DataWrite_RM68180A(0x0011);
	LCD_CtrlWrite_RM68180A(0xD12A);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD12B);
	LCD_DataWrite_RM68180A(0x0027);
	LCD_CtrlWrite_RM68180A(0xD12C);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD12D);
	LCD_DataWrite_RM68180A(0x0036);
	LCD_CtrlWrite_RM68180A(0xD12E);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD12F);
	LCD_DataWrite_RM68180A(0x004D);
	LCD_CtrlWrite_RM68180A(0xD130);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD131);
	LCD_DataWrite_RM68180A(0x007D);
	LCD_CtrlWrite_RM68180A(0xD132);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD133);
	LCD_DataWrite_RM68180A(0x00FF);

	LCD_CtrlWrite_RM68180A(0xD200);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD201);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD202);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD203);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD204);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD205);
	LCD_DataWrite_RM68180A(0x0014);
	LCD_CtrlWrite_RM68180A(0xD206);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD207);
	LCD_DataWrite_RM68180A(0x0031);
	LCD_CtrlWrite_RM68180A(0xD208);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD209);
	LCD_DataWrite_RM68180A(0x0051);
	LCD_CtrlWrite_RM68180A(0xD20A);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD20B);
	LCD_DataWrite_RM68180A(0x008C);
	LCD_CtrlWrite_RM68180A(0xD20C);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD20D);
	LCD_DataWrite_RM68180A(0x00BC);
	LCD_CtrlWrite_RM68180A(0xD20E);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD20F);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD210);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD211);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xD212);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD213);
	LCD_DataWrite_RM68180A(0x007C);
	LCD_CtrlWrite_RM68180A(0xD214);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD215);
	LCD_DataWrite_RM68180A(0x00AE);
	LCD_CtrlWrite_RM68180A(0xD216);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD217);
	LCD_DataWrite_RM68180A(0x00F9);
	LCD_CtrlWrite_RM68180A(0xD218);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD219);
	LCD_DataWrite_RM68180A(0x0032);
	LCD_CtrlWrite_RM68180A(0xD21A);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD21B);
	LCD_DataWrite_RM68180A(0x0033);
	LCD_CtrlWrite_RM68180A(0xD21C);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD21D);
	LCD_DataWrite_RM68180A(0x0065);
	LCD_CtrlWrite_RM68180A(0xD21E);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD21F);
	LCD_DataWrite_RM68180A(0x0096);
	LCD_CtrlWrite_RM68180A(0xD220);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD221);
	LCD_DataWrite_RM68180A(0x00B2);
	LCD_CtrlWrite_RM68180A(0xD222);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD223);
	LCD_DataWrite_RM68180A(0x00D3);
	LCD_CtrlWrite_RM68180A(0xD224);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD225);
	LCD_DataWrite_RM68180A(0x00E7);
	LCD_CtrlWrite_RM68180A(0xD226);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD227);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD228);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD229);
	LCD_DataWrite_RM68180A(0x0011);
	LCD_CtrlWrite_RM68180A(0xD22A);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD22B);
	LCD_DataWrite_RM68180A(0x0027);
	LCD_CtrlWrite_RM68180A(0xD22C);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD22D);
	LCD_DataWrite_RM68180A(0x0036);
	LCD_CtrlWrite_RM68180A(0xD22E);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD22F);
	LCD_DataWrite_RM68180A(0x004D);
	LCD_CtrlWrite_RM68180A(0xD230);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD231);
	LCD_DataWrite_RM68180A(0x007D);
	LCD_CtrlWrite_RM68180A(0xD232);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD233);
	LCD_DataWrite_RM68180A(0x00FF);

	LCD_CtrlWrite_RM68180A(0xD300);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD301);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD302);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD303);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD304);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD305);
	LCD_DataWrite_RM68180A(0x0014);
	LCD_CtrlWrite_RM68180A(0xD306);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD307);
	LCD_DataWrite_RM68180A(0x0031);
	LCD_CtrlWrite_RM68180A(0xD308);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD309);
	LCD_DataWrite_RM68180A(0x0051);
	LCD_CtrlWrite_RM68180A(0xD30A);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD30B);
	LCD_DataWrite_RM68180A(0x008C);
	LCD_CtrlWrite_RM68180A(0xD30C);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD30D);
	LCD_DataWrite_RM68180A(0x00BC);
	LCD_CtrlWrite_RM68180A(0xD30E);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD30F);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD310);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD311);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xD312);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD313);
	LCD_DataWrite_RM68180A(0x007C);
	LCD_CtrlWrite_RM68180A(0xD314);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD315);
	LCD_DataWrite_RM68180A(0x00AE);
	LCD_CtrlWrite_RM68180A(0xD316);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD317);
	LCD_DataWrite_RM68180A(0x00F9);
	LCD_CtrlWrite_RM68180A(0xD318);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD319);
	LCD_DataWrite_RM68180A(0x0032);
	LCD_CtrlWrite_RM68180A(0xD31A);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD31B);
	LCD_DataWrite_RM68180A(0x0033);
	LCD_CtrlWrite_RM68180A(0xD31C);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD31D);
	LCD_DataWrite_RM68180A(0x0065);
	LCD_CtrlWrite_RM68180A(0xD31E);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD31F);
	LCD_DataWrite_RM68180A(0x0096);
	LCD_CtrlWrite_RM68180A(0xD320);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD321);
	LCD_DataWrite_RM68180A(0x00B2);
	LCD_CtrlWrite_RM68180A(0xD322);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD323);
	LCD_DataWrite_RM68180A(0x00D3);
	LCD_CtrlWrite_RM68180A(0xD324);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD325);
	LCD_DataWrite_RM68180A(0x00E7);
	LCD_CtrlWrite_RM68180A(0xD326);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD327);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD328);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD329);
	LCD_DataWrite_RM68180A(0x0011);
	LCD_CtrlWrite_RM68180A(0xD32A);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD32B);
	LCD_DataWrite_RM68180A(0x0027);
	LCD_CtrlWrite_RM68180A(0xD32C);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD32D);
	LCD_DataWrite_RM68180A(0x0036);
	LCD_CtrlWrite_RM68180A(0xD32E);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD32F);
	LCD_DataWrite_RM68180A(0x004D);
	LCD_CtrlWrite_RM68180A(0xD330);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD331);
	LCD_DataWrite_RM68180A(0x007D);
	LCD_CtrlWrite_RM68180A(0xD332);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD333);
	LCD_DataWrite_RM68180A(0x00FF);

	LCD_CtrlWrite_RM68180A(0xD400);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD401);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD402);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD403);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD404);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD405);
	LCD_DataWrite_RM68180A(0x0014);
	LCD_CtrlWrite_RM68180A(0xD406);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD407);
	LCD_DataWrite_RM68180A(0x0031);
	LCD_CtrlWrite_RM68180A(0xD408);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD409);
	LCD_DataWrite_RM68180A(0x0051);
	LCD_CtrlWrite_RM68180A(0xD40A);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD40B);
	LCD_DataWrite_RM68180A(0x008C);
	LCD_CtrlWrite_RM68180A(0xD40C);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD40D);
	LCD_DataWrite_RM68180A(0x00BC);
	LCD_CtrlWrite_RM68180A(0xD40E);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD40F);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD410);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD411);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xD412);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD413);
	LCD_DataWrite_RM68180A(0x007C);
	LCD_CtrlWrite_RM68180A(0xD414);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD415);
	LCD_DataWrite_RM68180A(0x00AE);
	LCD_CtrlWrite_RM68180A(0xD416);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD417);
	LCD_DataWrite_RM68180A(0x00F9);
	LCD_CtrlWrite_RM68180A(0xD418);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD419);
	LCD_DataWrite_RM68180A(0x0032);
	LCD_CtrlWrite_RM68180A(0xD41A);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD41B);
	LCD_DataWrite_RM68180A(0x0033);
	LCD_CtrlWrite_RM68180A(0xD41C);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD41D);
	LCD_DataWrite_RM68180A(0x0065);
	LCD_CtrlWrite_RM68180A(0xD41E);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD41F);
	LCD_DataWrite_RM68180A(0x0096);
	LCD_CtrlWrite_RM68180A(0xD420);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD421);
	LCD_DataWrite_RM68180A(0x00B2);
	LCD_CtrlWrite_RM68180A(0xD422);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD423);
	LCD_DataWrite_RM68180A(0x00D3);
	LCD_CtrlWrite_RM68180A(0xD424);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD425);
	LCD_DataWrite_RM68180A(0x00E7);
	LCD_CtrlWrite_RM68180A(0xD426);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD427);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD428);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD429);
	LCD_DataWrite_RM68180A(0x0011);
	LCD_CtrlWrite_RM68180A(0xD42A);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD42B);
	LCD_DataWrite_RM68180A(0x0027);
	LCD_CtrlWrite_RM68180A(0xD42C);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD42D);
	LCD_DataWrite_RM68180A(0x0036);
	LCD_CtrlWrite_RM68180A(0xD42E);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD42F);
	LCD_DataWrite_RM68180A(0x004D);
	LCD_CtrlWrite_RM68180A(0xD430);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD431);
	LCD_DataWrite_RM68180A(0x007D);
	LCD_CtrlWrite_RM68180A(0xD432);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD433);
	LCD_DataWrite_RM68180A(0x00FF);

	LCD_CtrlWrite_RM68180A(0xD500);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD501);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD502);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD503);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD504);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD505);
	LCD_DataWrite_RM68180A(0x0014);
	LCD_CtrlWrite_RM68180A(0xD506);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD507);
	LCD_DataWrite_RM68180A(0x0031);
	LCD_CtrlWrite_RM68180A(0xD508);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD509);
	LCD_DataWrite_RM68180A(0x0051);
	LCD_CtrlWrite_RM68180A(0xD50A);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD50B);
	LCD_DataWrite_RM68180A(0x008C);
	LCD_CtrlWrite_RM68180A(0xD50C);
	LCD_DataWrite_RM68180A(0x0000);
	LCD_CtrlWrite_RM68180A(0xD50D);
	LCD_DataWrite_RM68180A(0x00BC);
	LCD_CtrlWrite_RM68180A(0xD50E);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD50F);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD510);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD511);
	LCD_DataWrite_RM68180A(0x0034);
	LCD_CtrlWrite_RM68180A(0xD512);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD513);
	LCD_DataWrite_RM68180A(0x007C);
	LCD_CtrlWrite_RM68180A(0xD514);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD515);
	LCD_DataWrite_RM68180A(0x00AE);
	LCD_CtrlWrite_RM68180A(0xD516);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD517);
	LCD_DataWrite_RM68180A(0x00F9);
	LCD_CtrlWrite_RM68180A(0xD518);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD519);
	LCD_DataWrite_RM68180A(0x0032);
	LCD_CtrlWrite_RM68180A(0xD51A);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD51B);
	LCD_DataWrite_RM68180A(0x0033);
	LCD_CtrlWrite_RM68180A(0xD51C);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD51D);
	LCD_DataWrite_RM68180A(0x0065);
	LCD_CtrlWrite_RM68180A(0xD51E);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD51F);
	LCD_DataWrite_RM68180A(0x0096);
	LCD_CtrlWrite_RM68180A(0xD520);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD521);
	LCD_DataWrite_RM68180A(0x00B2);
	LCD_CtrlWrite_RM68180A(0xD522);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD523);
	LCD_DataWrite_RM68180A(0x00D3);
	LCD_CtrlWrite_RM68180A(0xD524);
	LCD_DataWrite_RM68180A(0x0002);
	LCD_CtrlWrite_RM68180A(0xD525);
	LCD_DataWrite_RM68180A(0x00E7);
	LCD_CtrlWrite_RM68180A(0xD526);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD527);
	LCD_DataWrite_RM68180A(0x0001);
	LCD_CtrlWrite_RM68180A(0xD528);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD529);
	LCD_DataWrite_RM68180A(0x0011);
	LCD_CtrlWrite_RM68180A(0xD52A);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD52B);
	LCD_DataWrite_RM68180A(0x0027);
	LCD_CtrlWrite_RM68180A(0xD52C);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD52D);
	LCD_DataWrite_RM68180A(0x0036);
	LCD_CtrlWrite_RM68180A(0xD52E);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD52F);
	LCD_DataWrite_RM68180A(0x004D);
	LCD_CtrlWrite_RM68180A(0xD530);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD531);
	LCD_DataWrite_RM68180A(0x007D);
	LCD_CtrlWrite_RM68180A(0xD532);
	LCD_DataWrite_RM68180A(0x0003);
	LCD_CtrlWrite_RM68180A(0xD533);
	LCD_DataWrite_RM68180A(0x00FF);

	LCD_CtrlWrite_RM68180A(0xD600);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD601);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD602);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD603);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD604);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD605);      
	LCD_DataWrite_RM68180A(0x0014);      
	LCD_CtrlWrite_RM68180A(0xD606);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD607);      
	LCD_DataWrite_RM68180A(0x0031);      
	LCD_CtrlWrite_RM68180A(0xD608);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD609);      
	LCD_DataWrite_RM68180A(0x0051);      
	LCD_CtrlWrite_RM68180A(0xD60A);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD60B);      
	LCD_DataWrite_RM68180A(0x008C);      
	LCD_CtrlWrite_RM68180A(0xD60C);      
	LCD_DataWrite_RM68180A(0x0000);      
	LCD_CtrlWrite_RM68180A(0xD60D);      
	LCD_DataWrite_RM68180A(0x00BC);      
	LCD_CtrlWrite_RM68180A(0xD60E);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD60F);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD610);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD611);      
	LCD_DataWrite_RM68180A(0x0034);      
	LCD_CtrlWrite_RM68180A(0xD612);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD613);      
	LCD_DataWrite_RM68180A(0x007C);      
	LCD_CtrlWrite_RM68180A(0xD614);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD615);      
	LCD_DataWrite_RM68180A(0x00AE);      
	LCD_CtrlWrite_RM68180A(0xD616);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD617);      
	LCD_DataWrite_RM68180A(0x00F9);      
	LCD_CtrlWrite_RM68180A(0xD618);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD619);      
	LCD_DataWrite_RM68180A(0x0032);      
	LCD_CtrlWrite_RM68180A(0xD61A);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD61B);      
	LCD_DataWrite_RM68180A(0x0033);      
	LCD_CtrlWrite_RM68180A(0xD61C);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD61D);      
	LCD_DataWrite_RM68180A(0x0065);      
	LCD_CtrlWrite_RM68180A(0xD61E);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD61F);      
	LCD_DataWrite_RM68180A(0x0096);      
	LCD_CtrlWrite_RM68180A(0xD620);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD621);      
	LCD_DataWrite_RM68180A(0x00B2);      
	LCD_CtrlWrite_RM68180A(0xD622);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD623);      
	LCD_DataWrite_RM68180A(0x00D3);      
	LCD_CtrlWrite_RM68180A(0xD624);      
	LCD_DataWrite_RM68180A(0x0002);      
	LCD_CtrlWrite_RM68180A(0xD625);      
	LCD_DataWrite_RM68180A(0x00E7);      
	LCD_CtrlWrite_RM68180A(0xD626);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD627);      
	LCD_DataWrite_RM68180A(0x0001);      
	LCD_CtrlWrite_RM68180A(0xD628);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD629);      
	LCD_DataWrite_RM68180A(0x0011);      
	LCD_CtrlWrite_RM68180A(0xD62A);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD62B);      
	LCD_DataWrite_RM68180A(0x0027);      
	LCD_CtrlWrite_RM68180A(0xD62C);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD62D);      
	LCD_DataWrite_RM68180A(0x0036);      
	LCD_CtrlWrite_RM68180A(0xD62E);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD62F);      
	LCD_DataWrite_RM68180A(0x004D);      
	LCD_CtrlWrite_RM68180A(0xD630);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD631);      
	LCD_DataWrite_RM68180A(0x007D);      
	LCD_CtrlWrite_RM68180A(0xD632);      
	LCD_DataWrite_RM68180A(0x0003);      
	LCD_CtrlWrite_RM68180A(0xD633);      
	LCD_DataWrite_RM68180A(0x00FF);   

	/** Select Command Page '0' **/
	LCD_CtrlWrite_RM68180A(0xF000);
	LCD_DataWrite_RM68180A(0x0055);
	LCD_CtrlWrite_RM68180A(0xF001);
	LCD_DataWrite_RM68180A(0x00AA);
	LCD_CtrlWrite_RM68180A(0xF002);
	LCD_DataWrite_RM68180A(0x0052);
	LCD_CtrlWrite_RM68180A(0xF003);
	LCD_DataWrite_RM68180A(0x0008);
	LCD_CtrlWrite_RM68180A(0xF004);
	LCD_DataWrite_RM68180A(0x0000);

	// Display Option Control
	LCD_CtrlWrite_RM68180A(0xB100); 
	LCD_DataWrite_RM68180A(0x00CC); 

	LCD_CtrlWrite_RM68180A(0xB101); 
	LCD_DataWrite_RM68180A(0x0004); //top/bottom//left//right


	// Vivid Color
	LCD_CtrlWrite_RM68180A(0xB400); 
	LCD_DataWrite_RM68180A(0x0010); //enhance color

	// Resolution 480x854
	LCD_CtrlWrite_RM68180A(0xB500);
	LCD_DataWrite_RM68180A(0x0050);  //reslution

	// EQ Control Function for Source Driver
	LCD_CtrlWrite_RM68180A(0xB800);
	LCD_DataWrite_RM68180A(0x0000);  
	LCD_CtrlWrite_RM68180A(0xB801);
	LCD_DataWrite_RM68180A(0x0002);  
	LCD_CtrlWrite_RM68180A(0xB802);
	LCD_DataWrite_RM68180A(0x0002);  
	LCD_CtrlWrite_RM68180A(0xB803);
	LCD_DataWrite_RM68180A(0x0002);  

	// Inversion Driver Control
	LCD_CtrlWrite_RM68180A(0xBC00);
	LCD_DataWrite_RM68180A(0x0002);  
	LCD_CtrlWrite_RM68180A(0xBC01);
	LCD_DataWrite_RM68180A(0x0002);  
	LCD_CtrlWrite_RM68180A(0xBC02);
	LCD_DataWrite_RM68180A(0x0002);  

	// LCD_DataWrite_RM68180ASET
	LCD_CtrlWrite_RM68180A(0x2B02); 
	LCD_DataWrite_RM68180A(0x0003); 
	LCD_CtrlWrite_RM68180A(0x2B03); 
	LCD_DataWrite_RM68180A(0x0055); 

	// MATCDL
	LCD_CtrlWrite_RM68180A(0x3600); 
	LCD_DataWrite_RM68180A(0x0000); //0x0000

	//TE ON
	LCD_CtrlWrite_RM68180A(0x3500);
	LCD_DataWrite_RM68180A(0x0000);
	
       LCD_CtrlWrite_RM68180A(0x4400);//TE Scan line
	LCD_DataWrite_RM68180A(0x0000);// Hight 8 bits 512  lines
	LCD_CtrlWrite_RM68180A(0x4401);
	LCD_DataWrite_RM68180A(0x0000);//low 8bits 


	

	//Color mapping
	LCD_CtrlWrite_RM68180A(0x3A00);
	LCD_DataWrite_RM68180A(0x0055);

	//SLPOUT
	LCD_CtrlWrite_RM68180A(0x1100);
	LCD_DataWrite_RM68180A(0x0000);
	Delayms(120);
	//DISPLAY ON
	LCD_CtrlWrite_RM68180A(0x2900);
	LCD_DataWrite_RM68180A(0x0000);
}

void LCD_EnterSleep_RM68180A(void)
{
	LCD_CtrlWrite_RM68180A(0x2800);
	LCD_DataWrite_RM68180A(0x0000);
	Delayms(100);
	//DISPLAY ON
	LCD_CtrlWrite_RM68180A(0x1000);
	LCD_DataWrite_RM68180A(0x0000);
}

void LCD_ExitSleep_RM68180A(void)
{
	//*************Power On sequence ****************//
	LCD_CtrlWrite_RM68180A(0x1100);
	LCD_DataWrite_RM68180A(0x0000);
	Delayms(120);
	//DISPLAY ON
	LCD_CtrlWrite_RM68180A(0x2900);
	LCD_DataWrite_RM68180A(0x0000);
	Delayms(100);
}

void LCD_BlockClear_RM68180A(kal_uint16 x1,kal_uint16 y1,kal_uint16 x2,kal_uint16 y2,kal_uint16 data)
{
	kal_uint16 LCD_x;
	kal_uint16 LCD_y;

	LCD_CtrlWrite_RM68180A(0x2A00); LCD_DataWrite_RM68180A((x1&0xFF00)>>8);
	LCD_CtrlWrite_RM68180A(0x2A01); LCD_DataWrite_RM68180A(x1&0xFF);
	LCD_CtrlWrite_RM68180A(0x2A02); LCD_DataWrite_RM68180A((x2&0xFF00)>>8);
	LCD_CtrlWrite_RM68180A(0x2A03); LCD_DataWrite_RM68180A(x2&0xFF);
	LCD_CtrlWrite_RM68180A(0x2B00); LCD_DataWrite_RM68180A((y1&0xFF00)>>8);
	LCD_CtrlWrite_RM68180A(0x2B01); LCD_DataWrite_RM68180A(y1&0xFF);
	LCD_CtrlWrite_RM68180A(0x2B02); LCD_DataWrite_RM68180A((y2&0xFF00)>>8);
	LCD_CtrlWrite_RM68180A(0x2B03); LCD_DataWrite_RM68180A(y2&0xFF);


	LCD_CtrlWrite_RM68180A(0x2C00);   

    for(LCD_y=y1;LCD_y<=y2;LCD_y++)
    {
      for(LCD_x=x1;LCD_x<=x2;LCD_x++)
      {
        #if (defined(MAIN_LCD_8BIT_MODE))
            *((volatile unsigned char *) MAIN_LCD_DATA_ADDR)=(kal_uint8)((data & 0xFF00)>>8);
            *((volatile unsigned char *) MAIN_LCD_DATA_ADDR)=(kal_uint8)(data & 0xFF);      
        #elif (defined(MAIN_LCD_9BIT_MODE))
            *((volatile unsigned short *) MAIN_LCD_DATA_ADDR)=((r_color<<3)|(g_color>>3));
            *((volatile unsigned short *) MAIN_LCD_DATA_ADDR)=((g_color&0x38)<<6)|b_color ;
        #elif (defined(MAIN_LCD_16BIT_MODE) || defined(MAIN_LCD_18BIT_MODE))
        #endif
      }
    }
}

void LCD_BlockWrite_Single_RM68180A(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{
	lcd_assert_fail = KAL_TRUE;

	LCD_SET_ROI_OFFSET(startx+1024, starty+1024);
	LCD_SET_ROI_SIZE(endx-startx+1,endy-starty+1);
	
	SET_LCD_CMD_PARAMETER(0,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(1,LCD_CMD,0x00);
	
	SET_LCD_CMD_PARAMETER(2,LCD_DATA,0x00);
	SET_LCD_CMD_PARAMETER(3,LCD_DATA,(startx & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(4,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(5,LCD_CMD,0x01);
	
	SET_LCD_CMD_PARAMETER(6,LCD_DATA,0x00);
	SET_LCD_CMD_PARAMETER(7,LCD_DATA,(startx&0xFF));

	SET_LCD_CMD_PARAMETER(8,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(9,LCD_CMD,0x02);
	
	SET_LCD_CMD_PARAMETER(10,LCD_DATA,0x00);
	SET_LCD_CMD_PARAMETER(11,LCD_DATA,(endx & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(12,LCD_CMD,0x2a);
	SET_LCD_CMD_PARAMETER(13,LCD_CMD,0x03);
	
	SET_LCD_CMD_PARAMETER(14,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(15,LCD_DATA,(endx&0xFF));

	SET_LCD_CMD_PARAMETER(16,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(17,LCD_CMD,0x00);
	
	SET_LCD_CMD_PARAMETER(18,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(19,LCD_DATA,(starty & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(20,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(21,LCD_CMD,0x01);
	SET_LCD_CMD_PARAMETER(22,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(23,LCD_DATA,(starty & 0xFF));

	SET_LCD_CMD_PARAMETER(24,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(25,LCD_CMD,0x02);
	SET_LCD_CMD_PARAMETER(26,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(27,LCD_DATA,(endy & 0xFF00)>>8);

	SET_LCD_CMD_PARAMETER(28,LCD_CMD,0x2b);
	SET_LCD_CMD_PARAMETER(29,LCD_CMD,0x03);
	SET_LCD_CMD_PARAMETER(30,LCD_DATA,0x00);

	SET_LCD_CMD_PARAMETER(31,LCD_DATA,(endy & 0xFF));

	SET_LCD_CMD_PARAMETER(32,LCD_CMD,0x2C);
	SET_LCD_CMD_PARAMETER(33,LCD_CMD,0x00);

	SET_LCD_ROI_CTRL_NUMBER_OF_CMD(34);
	if(1)
	{
		ENABLE_LCD_TRANSFER_COMPLETE_INT;
	}
	else
	{
		DISABLE_LCD_TRANSFER_COMPLETE_INT;
	}
	ENABLE_LCD_ROI_CTRL_CMD_FIRST;
	START_LCD_TRANSFER;

	lcd_assert_fail = KAL_FALSE;
	
}

void LCD_BlockWrite_RM68180A(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{

	kal_uint16 h;
	h = endy - starty + 1;
	
	//ASSERT(lcd_assert_fail==KAL_FALSE);
	///lcd_assert_fail = KAL_TRUE;
	while ((LCD_IS_RUNNING)||(LCD_WAIT_TE)||(LCD_BUSY)){}

		//LCD_CLR_SW_TE;

	if(h>480)
	{
		kal_uint16 h1 = 480;
		
		LCD_BlockWrite_Single_RM68180A(startx,starty,endx,starty+h1-1);
		if(1)
		{
			Lcd_set_callback_parameter(startx,starty+h1,endx,endy,LCD_BlockWrite_Single_RM68180A);
		}
		else
		{
			afterlog = KAL_TRUE;
			
			while ((LCD_IS_RUNNING)||(LCD_WAIT_TE)||(LCD_BUSY)){}
			LCD_BlockWrite_Single_RM68180A(startx,starty+h1,endx,endy);
		
		}


	}
	else
	{
		LCD_BlockWrite_Single_RM68180A(startx,starty,endx,endy);

	}

}


//LCD ILI9806
#define SHBS_LCD_IC_ILI9806_ID 0x9806

void LCD_CtrlWrite_ILI9806A(kal_uint16 reg_index)
{
	LCD_CtrlWrite_MAINLCD(reg_index&0xFF);
}


void LCD_DataWrite_ILI9806A(kal_uint16 reg_data)
{
	LCD_DataWrite_MAINLCD(reg_data&0xFF);
}

void LCD_SET_REG_ILI9806A(kal_uint16 reg_index, kal_uint16 reg_data)
{
	LCD_CtrlWrite_ILI9806A(reg_index);
	LCD_DataWrite_ILI9806A(reg_data);
}

kal_uint16 Lcd_Read_ID_ILI9806A(void)
{
	kal_uint16 LCD_temp1,LCD_temp2,LCD_temp3,LCD_temp4;
	kal_uint16 LCD_ID;   
	
	LCD_CtrlWrite_ILI9806A(0xD3);  
	LCD_DataRead_MAINLCD(LCD_temp1);	
	LCD_DataRead_MAINLCD(LCD_temp2);	
	LCD_DataRead_MAINLCD(LCD_temp3);
	LCD_DataRead_MAINLCD(LCD_temp4);

	LCD_ID = ((LCD_temp3<<8)|(LCD_temp4));

	return LCD_ID;
}

void LCD_Init_ILI9806A(void)
{
	LCD_CtrlWrite_ILI9806A(0xFF);  // EXTC Command Set enable register 
	LCD_DataWrite_ILI9806A(0xFF); 
	LCD_DataWrite_ILI9806A(0x98); 
	LCD_DataWrite_ILI9806A(0x06); 
	 
	LCD_CtrlWrite_ILI9806A(0xBA); // SPI Interface Setting 
	LCD_DataWrite_ILI9806A(0x60); 
	 
	LCD_CtrlWrite_ILI9806A(0xBC); // GIP 1 
	LCD_DataWrite_ILI9806A(0x03); 
	LCD_DataWrite_ILI9806A(0x0E); 
	LCD_DataWrite_ILI9806A(0x03); 
	LCD_DataWrite_ILI9806A(0x63); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x1B); 
	LCD_DataWrite_ILI9806A(0x12); 
	LCD_DataWrite_ILI9806A(0x6F); 
	LCD_DataWrite_ILI9806A(0x00); 
	LCD_DataWrite_ILI9806A(0x00); 
	LCD_DataWrite_ILI9806A(0x00); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0xFF); 
	LCD_DataWrite_ILI9806A(0xF2); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x00); 
	LCD_DataWrite_ILI9806A(0x40); 
	 
	LCD_CtrlWrite_ILI9806A(0xBD); // GIP 2 
	LCD_DataWrite_ILI9806A(0x02); 
	LCD_DataWrite_ILI9806A(0x13); 
	LCD_DataWrite_ILI9806A(0x45); 
	LCD_DataWrite_ILI9806A(0x67); 
	LCD_DataWrite_ILI9806A(0x45); 
	LCD_DataWrite_ILI9806A(0x67); 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x23); 
	 
	LCD_CtrlWrite_ILI9806A(0xBE); // GIP 3 
	LCD_DataWrite_ILI9806A(0x01); 
	LCD_DataWrite_ILI9806A(0x22); 
	LCD_DataWrite_ILI9806A(0x22); 
	LCD_DataWrite_ILI9806A(0xDC); 
	LCD_DataWrite_ILI9806A(0xBA); 
	LCD_DataWrite_ILI9806A(0x67); 
	LCD_DataWrite_ILI9806A(0x22); 
	LCD_DataWrite_ILI9806A(0x22); 
	LCD_DataWrite_ILI9806A(0x22); 
	 
	LCD_CtrlWrite_ILI9806A(0xC7); // VCOM Control 
	LCD_DataWrite_ILI9806A(0x84); 
	 
	LCD_CtrlWrite_ILI9806A(0xED); // en_volt_reg measure VGMP 
	LCD_DataWrite_ILI9806A(0x7F); 
	LCD_DataWrite_ILI9806A(0x0F); 
	LCD_DataWrite_ILI9806A(0x00); 
	 
	LCD_CtrlWrite_ILI9806A(0xC0); // Power Control 1 
	LCD_DataWrite_ILI9806A(0x03); 
	LCD_DataWrite_ILI9806A(0x0B); 
	LCD_DataWrite_ILI9806A(0x0A); 
	LCD_CtrlWrite_ILI9806A(0xB4);  // Display Inversion Control 
	LCD_DataWrite_ILI9806A(0x02); 
	LCD_DataWrite_ILI9806A(0x02); 
	LCD_DataWrite_ILI9806A(0x02); 
	 
	LCD_CtrlWrite_ILI9806A(0xF7);  // 480x800 
	LCD_DataWrite_ILI9806A(0x81); 
	 
	LCD_CtrlWrite_ILI9806A(0xB1);  // Frame Rate Control 
	LCD_DataWrite_ILI9806A(0x00); 
	LCD_DataWrite_ILI9806A(0x13); 
	LCD_DataWrite_ILI9806A(0x13); 
	 
	LCD_CtrlWrite_ILI9806A(0xF2);  //Panel Timing Control 
	LCD_DataWrite_ILI9806A(0x00); 
	LCD_DataWrite_ILI9806A(0x58); 
	LCD_DataWrite_ILI9806A(0x40); 
	 
	LCD_CtrlWrite_ILI9806A(0xC1); 
	LCD_DataWrite_ILI9806A(0x17); 
	LCD_DataWrite_ILI9806A(0x82); 
	LCD_DataWrite_ILI9806A(0x58); 
	LCD_DataWrite_ILI9806A(0x20); 
	 
	LCD_CtrlWrite_ILI9806A(0xE0); 
	LCD_DataWrite_ILI9806A(0x00); //P1 
	LCD_DataWrite_ILI9806A(0x04); //P2 
	LCD_DataWrite_ILI9806A(0x11); //P3 
	LCD_DataWrite_ILI9806A(0x11); //P4 
	LCD_DataWrite_ILI9806A(0x13); //P5 
	LCD_DataWrite_ILI9806A(0x1B); //P6 
	LCD_DataWrite_ILI9806A(0xCA); //P7 
	LCD_DataWrite_ILI9806A(0x09); //P8 
	LCD_DataWrite_ILI9806A(0x04); //P9 
	LCD_DataWrite_ILI9806A(0x08); //P10 
	LCD_DataWrite_ILI9806A(0x04); //P11 
	LCD_DataWrite_ILI9806A(0x0F); //P12 
	LCD_DataWrite_ILI9806A(0x0F); //P13 
	LCD_DataWrite_ILI9806A(0x33); //P14 
	LCD_DataWrite_ILI9806A(0x2F); //P15 
	LCD_DataWrite_ILI9806A(0x00); //P16 
	 
	LCD_CtrlWrite_ILI9806A(0xE1); 
	LCD_DataWrite_ILI9806A(0x00); //P1 
	LCD_DataWrite_ILI9806A(0x01); //P2 
	LCD_DataWrite_ILI9806A(0x04); //P3 
	LCD_DataWrite_ILI9806A(0x09); //P4 
	LCD_DataWrite_ILI9806A(0x0D); //P5 
	LCD_DataWrite_ILI9806A(0x10); //P6 
	LCD_DataWrite_ILI9806A(0x7B); //P7 
	LCD_DataWrite_ILI9806A(0x07); //P8 
	LCD_DataWrite_ILI9806A(0x04); //P9 
	LCD_DataWrite_ILI9806A(0x09); //P10 
	LCD_DataWrite_ILI9806A(0x08); //P11 
	LCD_DataWrite_ILI9806A(0x0B); //P12 
	LCD_DataWrite_ILI9806A(0x0A); //P13 
	LCD_DataWrite_ILI9806A(0x2B); //P14 
	LCD_DataWrite_ILI9806A(0x25); //P15 
	LCD_DataWrite_ILI9806A(0x00); //P16 
	 
	LCD_CtrlWrite_ILI9806A(0x36); //Memory Access Control
	LCD_DataWrite_ILI9806A(0xC0);

	LCD_CtrlWrite_ILI9806A(0x3A); //Interface Pixel Format
	LCD_DataWrite_ILI9806A(0x55);
	 
	LCD_CtrlWrite_ILI9806A(0x35); //Tearing Effect Line  
	 
	LCD_CtrlWrite_ILI9806A(0x11); //Exit Sleep 
	Delayms(120); 

	LCD_CtrlWrite_ILI9806A(0x29); // Display On 

}

void LCD_EnterSleep_ILI9806A(void)
{
	LCD_CtrlWrite_ILI9806A(0x28) ;
	Delayms(10); 
	LCD_CtrlWrite_ILI9806A(0x10);     
	Delayms(120); 
}

void LCD_ExitSleep_ILI9806A(void)
{
	//*************Power On sequence ****************//
	LCD_Init_ILI9806A();

}

void LCD_BlockClear_ILI9806A(kal_uint16 x1,kal_uint16 y1,kal_uint16 x2,kal_uint16 y2,kal_uint16 data)
{
	kal_uint16 LCD_x;
	kal_uint16 LCD_y;
  
	LCD_CtrlWrite_ILI9806A(0x2A);
	LCD_DataWrite_ILI9806A((x1&0xFF00)>>8);
	LCD_DataWrite_ILI9806A(x1&0xFF);
	LCD_DataWrite_ILI9806A ((x2&0xFF00)>>8);
	LCD_DataWrite_ILI9806A(x2&0xFF);

	LCD_CtrlWrite_ILI9806A(0x2B);
	LCD_DataWrite_ILI9806A((y1&0xFF00)>>8);
	LCD_DataWrite_ILI9806A(y1&0xFF);
	LCD_DataWrite_ILI9806A ((y2&0xFF00)>>8);
	LCD_DataWrite_ILI9806A(y2&0xFF);

	LCD_CtrlWrite_ILI9806A(0x2C);

    for(LCD_y=y1;LCD_y<=y2;LCD_y++)
    {
      for(LCD_x=x1;LCD_x<=x2;LCD_x++)
      {
        #if (defined(MAIN_LCD_8BIT_MODE))
            *((volatile unsigned char *) MAIN_LCD_DATA_ADDR)=(kal_uint8)((data & 0xFF00)>>8);
            *((volatile unsigned char *) MAIN_LCD_DATA_ADDR)=(kal_uint8)(data & 0xFF);      
        #elif (defined(MAIN_LCD_9BIT_MODE))
            *((volatile unsigned short *) MAIN_LCD_DATA_ADDR)=((r_color<<3)|(g_color>>3));
            *((volatile unsigned short *) MAIN_LCD_DATA_ADDR)=((g_color&0x38)<<6)|b_color ;
        #elif (defined(MAIN_LCD_16BIT_MODE) || defined(MAIN_LCD_18BIT_MODE))
        #endif
      }
    }
	
}

void LCD_BlockWrite_Single_ILI9806A(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{

	lcd_assert_fail = KAL_TRUE;

	LCD_SET_ROI_OFFSET(startx+1024, starty+1024);
	LCD_SET_ROI_SIZE(endx-startx+1,endy-starty+1);


	SET_LCD_CMD_PARAMETER(0, LCD_CMD, 0x2A);
	SET_LCD_CMD_PARAMETER(1, LCD_DATA, ((startx&0xFF00)>>8));
	SET_LCD_CMD_PARAMETER(2, LCD_DATA, ((startx&0xFF)));
	SET_LCD_CMD_PARAMETER(3, LCD_DATA, ((endx&0xFF00)>>8));
	SET_LCD_CMD_PARAMETER(4, LCD_DATA, ((endx&0xFF)));

	SET_LCD_CMD_PARAMETER(5, LCD_CMD, 0x2B);
	SET_LCD_CMD_PARAMETER(6, LCD_DATA, (((starty+54)&0xFF00)>>8));
	SET_LCD_CMD_PARAMETER(7, LCD_DATA, (((starty+54)&0xFF)));
	SET_LCD_CMD_PARAMETER(8, LCD_DATA, (((endy+54)&0xFF00)>>8));
	SET_LCD_CMD_PARAMETER(9, LCD_DATA, (((endy+54)&0xFF)));

	SET_LCD_CMD_PARAMETER(10, LCD_CMD,0x2C);

	SET_LCD_ROI_CTRL_NUMBER_OF_CMD(11);
	
	if(1)
	{
		ENABLE_LCD_TRANSFER_COMPLETE_INT;
	}
	else
	{
		DISABLE_LCD_TRANSFER_COMPLETE_INT;
	}
	ENABLE_LCD_ROI_CTRL_CMD_FIRST;
	START_LCD_TRANSFER;

	lcd_assert_fail = KAL_FALSE;
	
}

void LCD_BlockWrite_ILI9806A(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{

	kal_uint16 h;
	h = endy - starty + 1;
	
	//ASSERT(lcd_assert_fail==KAL_FALSE);
	///lcd_assert_fail = KAL_TRUE;
	while ((LCD_IS_RUNNING)||(LCD_WAIT_TE)||(LCD_BUSY)){}

		//LCD_CLR_SW_TE;

	if(h>480)
	{
		kal_uint16 h1 = 480;
		
		LCD_BlockWrite_Single_ILI9806A(startx,starty,endx,starty+h1-1);
		if(1)
		{
			Lcd_set_callback_parameter(startx,starty+h1,endx,endy,LCD_BlockWrite_Single_ILI9806A);
		}
		else
		{
			afterlog = KAL_TRUE;
			
			while ((LCD_IS_RUNNING)||(LCD_WAIT_TE)||(LCD_BUSY)){}
			LCD_BlockWrite_Single_ILI9806A(startx,starty+h1,endx,endy);
		
		}


	}
	else
	{
		LCD_BlockWrite_Single_ILI9806A(startx,starty,endx,endy);

	}

}


/////////////////////////////////////////////////////////////
kal_uint16 MAINLCD_ID = 0;
kal_uint16 MAINLCD_ID2 = 0;
kal_uint8 lcd_check = 0;
void LCD_ClearAll_MAINLCD(kal_uint16 data);
void LCD_EnterSleep_MAINLCD(void)
{
#if defined(LQT_SUPPORT)/*Do not remove LQT code segment*/
 if(!(lcd_at_mode==LCD_AT_RELEASE_MODE))
 {
  return;
 }	
#endif /*defined(LQT_SUPPORT))*/	

	if (MAINLCD_ID == SHBS_LCD_IC_RM68180_ID)
	{
		LCD_EnterSleep_RM68180A();
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_OTM8009_ID)
	{
	 	LCD_EnterSleep_OTM8009A();
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_ILI9806_ID)
	{
		LCD_EnterSleep_ILI9806A();
	}
	else
	{
		LCD_EnterSleep_ILI9806A();
	}
}

void LCD_ExitSleep_MAINLCD(void)
{
#if defined(LQT_SUPPORT)/*Do not remove LQT code segment*/
 if(!(lcd_at_mode==LCD_AT_RELEASE_MODE))
 {
  return;
 }	
#endif /*defined(LQT_SUPPORT))*/

	if (MAINLCD_ID == SHBS_LCD_IC_RM68180_ID)
	{
		LCD_ExitSleep_RM68180A();
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_OTM8009_ID)
	{
	 	LCD_ExitSleep_OTM8009A();
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_ILI9806_ID)
	{
		LCD_ExitSleep_ILI9806A();
	}
	else
	{
		LCD_ExitSleep_ILI9806A();
	}

	LCD_ClearAll_MAINLCD(0x0000);
	Delayms(10); // Delay 50ms
}

void LCD_Partial_On_MAINLCD(kal_uint16 start_page,kal_uint16 end_page)
{

}

void LCD_Partial_Off_MAINLCD(void)
{

}

kal_uint8 LCD_Partial_line_MAINLCD(void)
{
	return 1;		/* partial display in 1 line alignment */
}

void LCD_Set_Y_Addr_MAINLCD(kal_uint16 start_row, kal_uint16 end_row)
{

}

void LCD_Set_X_Addr_MAINLCD(kal_uint16 start_column, kal_uint16 end_column)
{

}

void LCD_blockClear_MAINLCD(kal_uint16 x1,kal_uint16 y1,kal_uint16 x2,kal_uint16 y2,kal_uint16 data)
{

	if (MAINLCD_ID == SHBS_LCD_IC_RM68180_ID)
	{
		LCD_BlockClear_RM68180A(x1,y1,x2,y2,data);
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_OTM8009_ID)
	{
	 	LCD_BlockClear_OTM8009A(x1,y1,x2,y2,data);
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_ILI9806_ID)
	{
		LCD_BlockClear_ILI9806A(x1,y1,x2,y2,data);
	}
	else
	{
		LCD_BlockClear_ILI9806A(x1,y1,x2,y2,data);
	}

}

void LCD_ClearAll_MAINLCD(kal_uint16 data)
{
	LCD_blockClear_MAINLCD(0,0,LCD_WIDTH-1,LCD_HEIGHT-1,data);
}


void LCD_Init_MAINLCD(kal_uint32 bkground, void **buf_addr)
{
	lcd_check = 0;
	
	// Enhance LCD I/F driving current.
#if defined(MT6238)
	*((volatile unsigned short *) 0x80010700) |= 0xE0E0;
#endif


	//Delayms(10);
	SET_LCD_CTRL_RESET_PIN;
	Delayms(3);
	CLEAR_LCD_CTRL_RESET_PIN;
	Delayms(10); 
	SET_LCD_CTRL_RESET_PIN;
	Delayms(120);

	MAINLCD_ID = Lcd_Read_ID_RM68180A();

	if (MAINLCD_ID == SHBS_LCD_IC_RM68180_ID)
	{
		LCD_Init_RM68180A();
	}
	else
	{
		MAINLCD_ID = Lcd_Read_ID_OTM8009A();

		if (MAINLCD_ID == SHBS_LCD_IC_OTM8009_ID)
		{
			LCD_Init_OTM8009A();
		}
		else
		{
			MAINLCD_ID = Lcd_Read_ID_ILI9806A();
			
			if (MAINLCD_ID == SHBS_LCD_IC_ILI9806_ID)
			{
				LCD_Init_ILI9806A();
			}
			else
			{
				LCD_Init_ILI9806A();
			}
		}
	}
	
	LCD_ClearAll_MAINLCD(0x0000);
	Delayms(20);
	lcd_check = 1;		
}

#ifdef __BANGS_DRV_FOR_GET_ZUJIAN_INFO__   
const kal_char* GET_DEFAULT_LCD_INFO(void)
{
 	kal_char* lcd_info_str;
	   
	if (MAINLCD_ID == SHBS_LCD_IC_RM68180_ID)
	{
		lcd_info_str = "RM68180\n";
		return lcd_info_str;
	}
	
	if (MAINLCD_ID == SHBS_LCD_IC_OTM8009_ID)
	{
		lcd_info_str = "OTM8009\n";
		return lcd_info_str;
	}
	
	if (MAINLCD_ID == SHBS_LCD_IC_ILI9806_ID)
	{
		lcd_info_str = "ILI9806\n";
		return lcd_info_str;
	}
	
	/* default value , don't move, or phone assert */
	{
		lcd_info_str = "NO LCD\n";
		return lcd_info_str;
	}
}
#endif

#ifdef __BANGS_DRV_FOR_ZUJIAN_SUPPORT__   
const kal_char* GET_DEFAULT_LCD_SUPPORT(void)
{
 	kal_char* lcd_info_str;


	{
		lcd_info_str = "RM68180\nOTM8009\nILI9806\n";
		return lcd_info_str;
	}


	/* default value , don't move, or phone assert */
	{
		lcd_info_str = "NO LCD SUPPORT\n";
		return lcd_info_str;
	}
}
#endif

void LCD_PWRON_MAINLCD(kal_bool on)
{
   if(on)
   {
      LCD_ExitSleep_MAINLCD();
   }
   else
   {
      LCD_EnterSleep_MAINLCD();
   }
}

void LCD_SetContrast_MAINLCD(kal_uint8 level)
{
}

void LCD_ON_MAINLCD(kal_bool on)
{
   if (on)
   {
      LCD_ExitSleep_MAINLCD();
   }
   else
   {
      LCD_EnterSleep_MAINLCD();
   }
}

void LCD_BlockWrite_MAINLCD(kal_uint16 startx,kal_uint16 starty,kal_uint16 endx,kal_uint16 endy)
{
	
#ifdef LQT_SUPPORT/*Do not remove LQT code segment*/
    if(!lcd_update_permission&&!(lcd_at_mode==LCD_AT_RELEASE_MODE))
    {
    	lcd_assert_fail = KAL_FALSE;
    	return;//in LQT mode but not update permitted
    }
    if(lcd_update_permission&&!(lcd_at_mode==LCD_AT_RELEASE_MODE))
    {
    	startx = 0;
    	starty = 0;
    	endx = LCD_WIDTH -1;
    	endy = LCD_HEIGHT -1; //in LQT mode and update permitted
    }
#endif /*LQT_SUPPORT*/

#if defined(__RF_DESENSE_TEST__)&& defined(__FM_DESENSE_COPY_NO_LCM_UPDATE_)
   // under this test, LCD will do nothing ==> just return this function call
   return;
#endif

	kal_prompt_trace(MOD_ENG,"MAINLCD_ID_GJ =: %x", MAINLCD_ID);

	if (MAINLCD_ID == SHBS_LCD_IC_RM68180_ID)
	{
		LCD_BlockWrite_RM68180A(startx,starty,endx,endy);
	}
	else if (MAINLCD_ID == SHBS_LCD_IC_OTM8009_ID)
	{
	 	LCD_BlockWrite_OTM8009A(startx,starty,endx,endy);

	}
	else if (MAINLCD_ID == SHBS_LCD_IC_ILI9806_ID)
	{
		LCD_BlockWrite_ILI9806A(startx,starty,endx,endy);

	}
	else
	{
		LCD_BlockWrite_ILI9806A(startx,starty,endx,endy);

	}
	
	//ENABLE_LCD_TRANSFER_COMPLETE_INT;
	//ENABLE_LCD_ROI_CTRL_CMD_FIRST;
	//START_LCD_TRANSFER;

	//lcd_assert_fail = KAL_FALSE;
}

void LCD_Size_MAINLCD(kal_uint16 *out_LCD_width,kal_uint16 *out_LCD_height)
{
   *out_LCD_width = LCD_WIDTH;
   *out_LCD_height = LCD_HEIGHT;
}

/*Engineering mode*/
kal_uint8 LCD_GetParm_MAINLCD(lcd_func_type type)
{
   switch(type)
   {
      case lcd_Bais_func:
         return 1;
      case lcd_Contrast_func:
         return 1;
      case lcd_LineRate_func:
         return 1;
      case lcd_Temperature_Compensation_func:
         return 1;
      default:
         ASSERT(0);
      return 100;
   }
}

void LCD_SetBias_MAINLCD(kal_uint8 *bias)
{
}

void LCD_Contrast_MAINLCD(kal_uint8 *contrast)
{
}

void LCD_LineRate_MAINLCD(kal_uint8 *linerate)
{
}

void LCD_Temp_Compensate_MAINLCD(kal_uint8 *compensate)
{
}

void LCD_Set_Scan_Direction_MAINLCD(kal_uint8 rotate_value)
{
	switch (rotate_value)
	{	
		#if 0
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
/* under construction !*/
		#endif
		break;
	}

}	/* LCD_Set_Scan_Direction_MAINLCD() */
LCM_IOCTRL_STATUS_ENUM
LCD_IOCTRL_MAINLCD(LCM_IOCTRL_ID_ENUM ID, void* Parameters)
{
   switch (ID)
   {
       case LCM_IOCTRL_QUERY__FRAME_RATE:
       {
             
			#if (defined(__MMI_MAINLCD_240X400__)||defined(__MMI_MAINLCD_240X320__))
            *(kal_uint32*) (Parameters) =  55.6;
			#else
            *(kal_uint32*) (Parameters) =  75.8;
			#endif
            return LCM_IOCTRL_OK;
             
       }

       case LCM_IOCTRL_SET__FRAME_RATE:
       {
	   		 return LCM_IOCTRL_NOT_SUPPORT;
            
       }


       case LCM_IOCTRL_QUERY__FRAME_MARKER:
	   	 	return LCM_IOCTRL_NOT_SUPPORT;

       case LCM_IOCTRL_SET__FRAME_MARKER:
            return LCM_IOCTRL_NOT_SUPPORT;


       case LCM_IOCTRL_QUERY__SUPPORT_H_V_SIGNAL_FUNC:
       {
	   		 return LCM_IOCTRL_NOT_SUPPORT;
       }

       case LCM_IOCTRL_QUERY__SUPPORT_V_PULSE_WIDTH:
       {
			 return LCM_IOCTRL_NOT_SUPPORT;
			
       }

       case LCM_IOCTRL_QUERY__SUPPORT_H_PULSE_WIDTH:
                return LCM_IOCTRL_NOT_SUPPORT;


       case LCM_IOCTRL_QUERY__BACK_PORCH:
       {
            *(kal_uint32*) (Parameters) = 2; // for example
            return LCM_IOCTRL_OK;
       }

       case LCM_IOCTRL_QUERY__FRONT_PORCH:
       {
            *(kal_uint32*) (Parameters) = 6; // for example
            return LCM_IOCTRL_OK;
       }

       case LCM_IOCTRL_SET__BACK_PORCH:
       {
	   	 return LCM_IOCTRL_NOT_SUPPORT;
       }

       case LCM_IOCTRL_SET__FRONT_PORCH:
       {
	   	 return LCM_IOCTRL_NOT_SUPPORT;
       }

       case LCM_IOCTRL_QUERY__TE_EDGE_ATTRIB:
       {
	   		return LCM_IOCTRL_NOT_SUPPORT;
       }

       case LCM_IOCTRL_QUERY__SUPPORT_READBACK_FUNC:
            return LCM_IOCTRL_NOT_SUPPORT;

       case LCM_IOCTRL_QUERY__SCANLINE_REG:
            return LCM_IOCTRL_NOT_SUPPORT;

       case LCM_IOCTRL_QUERY__IF_CS_NUMBER:
       {
            *(kal_uint32*) (Parameters) =  LCD_IF_PARALLEL_0;
            return LCM_IOCTRL_OK;
       }

       case LCM_IOCTRL_QUERY__LCM_WIDTH:
       {
            *(kal_uint32*) (Parameters) =  320;
            return LCM_IOCTRL_OK;
       }

       case LCM_IOCTRL_QUERY__LCM_HEIGHT:
       {
            *(kal_uint32*) (Parameters) =  480;
            return LCM_IOCTRL_OK;
       }
       
     case LCM_IOCTRL_QUERY__SYNC_MODE:

          *(kal_uint32*) (Parameters) = LCM_TE_VSYNC_MODE;
          return LCM_IOCTRL_OK;

     case LCM_IOCTRL_QUERY__FLIP_MIRROR:
          return LCM_IOCTRL_NOT_SUPPORT;

     case LCM_IOCTRL_QUERY__ROTATION:
          return LCM_IOCTRL_OK;
          
     case LCM_IOCTRL_QUERY__LCD_PPI: 
     	  *((kal_uint32*)Parameters) = LCM_DPI;
      	  return LCM_IOCTRL_OK;  
#if 1
	 case LCM_IOCTRL_QUERY_TEAR_CONTROL_FOR_ONE_TE_MODE:
	 	  #if (defined(__MMI_MAINLCD_240X400__)||defined(__MMI_MAINLCD_240X320__))
			 return LCM_IOCTRL_OK;
		  #else
		  	return LCM_IOCTRL_NOT_SUPPORT;
		  #endif
#endif		
     default:
          return LCM_IOCTRL_NOT_SUPPORT;
          
   }
}



LCD_Funcs LCD_func_MAINLCD = {
   LCD_Init_MAINLCD,
   LCD_PWRON_MAINLCD,
   LCD_SetContrast_MAINLCD,
   LCD_ON_MAINLCD,
   LCD_BlockWrite_MAINLCD,
   LCD_Size_MAINLCD,
   LCD_EnterSleep_MAINLCD,
   LCD_ExitSleep_MAINLCD,
   LCD_Partial_On_MAINLCD,
   LCD_Partial_Off_MAINLCD,
   LCD_Partial_line_MAINLCD,
   /*Engineering mode*/
   LCD_GetParm_MAINLCD,
   LCD_SetBias_MAINLCD,
   LCD_Contrast_MAINLCD,
   LCD_LineRate_MAINLCD,
   LCD_Temp_Compensate_MAINLCD
#ifdef LCM_ROTATE_SUPPORT
   ,LCD_Set_Scan_Direction_MAINLCD
#endif
#ifdef LQT_SUPPORT/*Do not remove LQT code segment*/
   ,LCD_gamma_test
   ,LCD_flicker_test
#endif
  , NULL
   ,LCD_IOCTRL_MAINLCD
};

void LCD_FunConfig(void)
{

	MainLCD = &LCD_func_MAINLCD;

	#ifdef DUAL_LCD
		//SubLCD = &LCD_func_HD66791;
	#endif
}

