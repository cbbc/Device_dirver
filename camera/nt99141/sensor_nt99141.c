/*
 * Filename:	sensor_nt99141.c
 * Revised:		$Date: 2014-07-25 16:22:12 -0800 (Fri, 25 July 2014) $
 * Revision:	$Revision: 00001 $
 * Author:		ChenJianYong		
 *
 * Copyright (c) 2010 GZ Duoyue Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Change Log:
 * Date           Author           Notes
 * 2014-07-25     ChenJianYong     first version
 */

#include "sensor_cfg.h"
#include "sensor_drv_u.h"

/**---------------------------------------------------------------------------*
 ** 						   Macro Define
 **---------------------------------------------------------------------------*/
//#define NT99141_I2C_ADDR_W		0x54
//#define NT99141_I2C_ADDR_R		0x55
#define NT99141_I2C_ADDR_W		0x2A
#define NT99141_I2C_ADDR_R		0x2A

typedef enum {
	FLICKER_50HZ = 0,
	FLICKER_60HZ,
	FLICKER_MAX
} FLICKER_E;

/**---------------------------------------------------------------------------*
 ** 					Local Function Prototypes							  *
 **---------------------------------------------------------------------------*/
LOCAL uint32_t _nt99141_poweron(uint32_t power_on);
LOCAL uint32_t _nt99141_identify(uint32_t param);
LOCAL uint32_t _nt99141_flash(uint32_t param);
LOCAL uint32_t _nt99141_set_anti_flicker(uint32_t param);
LOCAL uint32_t _nt99141_set_image_effect(uint32_t param);
LOCAL uint32_t _nt99141_set_awb(uint32_t param);
LOCAL uint32_t _nt99141_set_exposure_compensation(uint32_t param);
LOCAL uint32_t _nt99141_set_brightness(uint32_t param);
LOCAL uint32_t _nt99141_set_contrast(uint32_t param);

LOCAL uint32_t _nt99141_set_video_mode(uint32_t mode);

/**---------------------------------------------------------------------------*
 ** 						Local Variables 								 *
 **---------------------------------------------------------------------------*/

LOCAL const SENSOR_REG_T s_reg_common[] = {
	// Sensor Control Registers (0x3000)
	{0x3040,0x04}, // Calibration_Control_0
	{0x3041,0x02}, // Calibration_Control_1
	{0x3042,0xFF}, // ABLC_Thr_Top
	{0x3043,0x08}, // ABLC_Thr_Bottom
	{0x3052,0xE0}, // Optical_Black_Weight_0
	{0x3053,0x50}, // Optical_Black_Weight_1
	{0x3054,0x00}, // OB_OFS
	{0x3055,0x40}, // Reserved
	{0x305F,0x11}, // Reserved
	{0x3061,0x01}, // Reserved
	{0x3069,0x03}, // Pad_Config_Pix_Out, Output driving - > 8mA
	{0x306A,0x03}, // Pad_Config_PCLKt_Out, Output driving - > 8mA

	/*{0x3100,0x0f}, // Unknow Reg
	{0x3106,0x03}, // Unknow Reg
	{0x3105,0x01}, // Unknow Reg
	{0x3108,0x05}, // 00 Unknow Reg
	{0x3110,0x22}, // Unknow Reg
	{0x3111,0x57}, // Unknow Reg
	{0x3112,0x22}, // Unknow Reg
	{0x3113,0x55}, // Unknow Reg
	{0x3114,0x05}, // Unknow Reg
	{0x3135,0x00}, // Unknow Reg*/

	// SOC Lens Shading Correction Registers (0x3210)
	{0x3210,0x11}, // R // 18 LSC_R_Gain0
	{0x3211,0x11}, // 1B LSC_R_Gain1
	{0x3212,0x11}, // 1B LSC_R_Gain2
	{0x3213,0x11}, // 1A LSC_R_Gain3
	{0x3214,0x10}, // Gr // 17 LSC_Gr_Gain0
	{0x3215,0x10}, // 18 LSC_Gr_Gain1
	{0x3216,0x10}, // 19 LSC_Gr_Gain2
	{0x3217,0x10}, // 18 LSC_Gr_Gain3
	{0x3218,0x10}, // Gb // 17 LSC_Gb_Gain0
	{0x3219,0x10}, // 18 LSC_Gb_Gain1
	{0x321A,0x10}, // 19 LSC_Gb_Gain2
	{0x321B,0x10}, // 18 LSC_Gb_Gain3
	{0x321C,0x0f}, // B // 16 LSC_B_Gain0
	{0x321D,0x0f}, // 16 LSC_B_Gain1
	{0x321E,0x0f}, // 16 LSC_B_Gain2
	{0x321F,0x0f}, // 15 LSC_B_Gain3
	{0x3231,0x68}, // 74 LSC_EV_Bot, Rst: C0
	{0x3232,0xC4}, // LSC_Shf_Ctr, Rst: C3

	// SOC Statistic Registers (0x3250)
	{0x3250,0x80}, // Color_Acc_Ctrl, Rst: 80
	{0x3251,0x01}, // Reserved
	{0x3252,0x4B}, // Reserved
	{0x3253,0xA5}, // Reserved
	{0x3254,0x00}, // Reserved
	{0x3255,0xEB}, // Reserved
	{0x3256,0x85}, // Reserved
	{0x3257,0x50}, // CA_Y_Top_Thr, Rst: D0

	// SOC Gamma Registers (0x3270)
	{0x3270,0x06}, // 00 Gamma_Tab_0
	{0x3271,0x12}, // 0C Gamma_Tab_1
	{0x3272,0x1E}, // 18 Gamma_Tab_2
	{0x3273,0x34}, // 32 Gamma_Tab_3
	{0x3274,0x48}, // 44 Gamma_Tab_4
	{0x3275,0x59}, // 54 Gamma_Tab_5
	{0x3276,0x74}, // 70 Gamma_Tab_6
	{0x3277,0x87}, // 88 Gamma_Tab_7
	{0x3278,0x98}, // 9D Gamma_Tab_8
	{0x3279,0xA5}, // B0 Gamma_Tab_9
	{0x327A,0xBC}, // CF Gamma_Tab_10
	{0x327B,0xD0}, // E2 Gamma_Tab_11
	{0x327C,0xE4}, // EF Gamma_Tab_12
	{0x327D,0xF5}, // E7 Gamma_Tab_13
	{0x327E,0xFF}, // Gamma_Tab_14

	// SOC Auto-White Balance Registers (0x3290)
	{0x3290,0x01}, // WB_Gain_R_H, Rst: 01
	{0x3291,0x38}, // WB_Gain_R_L, Rst: 00
	{0x3296,0x01}, // WB_Gain_B_H, Rst: 01
	{0x3297,0x68}, // WB_Gain_B_L, Rst: 00
	{0x329B,0x01}, // AWB_Control_0, Rst: 00

	{0x32A1,0x01}, // Unknow Reg
	{0x32A2,0x10}, // Unknow Reg
	{0x32A3,0x01}, // Unknow Reg
	{0x32A4,0xA0}, // Unknow Reg
	{0x32A5,0x01}, // Unknow Reg
	{0x32A6,0x28}, // Unknow Reg
	{0x32A7,0x02}, // Unknow Reg
	{0x32A8,0x20}, // Unknow Reg
	{0x32A9,0x01}, // Unknow Reg

	// SOC Auto-Exposure Registers (0x32B0)
	{0x32B0,0x00}, // 55 LA_Wgt_Ctrl0, Rst: 55
	{0x32B1,0x14}, // 7D LA_Wgt_Ctrl1, Rst: 55
	{0x32B2,0x3C}, // AA LA_Wgt_Ctrl2, Rst: 55
	{0x32B3,0x69}, // 14 LA_Wgt_Ctrl3, Rst: 55
	{0x32B8,0x31}, // 33 AE_DetcRange_Upper, Rst: 48
	{0x32B9,0x25}, // 27 AE_DetcRange_Lower, Rst: 38
	{0x32BB,0xF7}, // AE_Control_0, Rst: 87
	{0x32BC,0x2B}, // 2D AE_Target_Lum, Rst: 40
	{0x32BD,0x2E}, // 30 AE_ConvRange_Upper, Rst: 44
	{0x32BE,0x28}, // 2A AE_ConvRange_Lower, Rst: 3C
	//{0x32C4,0x33}, // 0x33:10X; 0x37:12X; 0x3B:14X AGC_Max_Limit, Rst: 20

	// SOC Output Format and Specical Effects Registers (0x32F0)
	{0x32F0,0x01}, // Output_Format
	{0x32F2,0x84}, // Y_Component
	{0x32F6,0x0F}, // Special_Effect_1
	{0x32F9,0x42}, // Reserved
	{0x32FA,0x24}, // Reserved
	{0x32FC,0x00}, // Brt_Ofs, Brightness offset (-128 ~ 127)

	// Image Quality Control Registers (0x3300)
	//{0x3300,0x00}, // Noise_Enh_Param
	//{0x3301,0x00}, // Noise_Enh_Param
	/*{0x3302,0x00}, // Matrix_RR_H
	{0x3303,0x5C}, // 4D Matrix_RR_L
	{0x3304,0x00}, // Matrix_RG_H
	{0x3305,0x6C}, // 96 Matrix_RG_L
	{0x3306,0x00}, // Matrix_RB_H
	{0x3307,0x39}, // 1D Matrix_RB_L
	{0x3308,0x07}, // Matrix_GR_H
	{0x3309,0xBE}, // E8 Matrix_GR_L
	{0x330A,0x06}, // Matrix_GG_H
	{0x330B,0xF9}, // BD Matrix_GG_L
	{0x330C,0x01}, // Matrix_GB_H
	{0x330D,0x46}, // 5B Matrix_GB_L
	{0x330E,0x01}, // Matrix_BR_H
	{0x330F,0x0D}, // 91 Matrix_BR_L
	{0x3310,0x06}, // Matrix_BG_H
	{0x3311,0xFD}, // 80 Matrix_BG_L
	{0x3312,0x07}, // Matrix_BB_H
	{0x3313,0xFC}, // EF Matrix_BB_L*/
	{0x3302,0x00}, // Matrix_RR_H
	{0x3303,0x5C}, // Matrix_RR_L
	{0x3304,0x00}, // Matrix_RG_H
	{0x3305,0x6C}, // Matrix_RG_L
	{0x3306,0x00}, // Matrix_RB_H
	{0x3307,0x39}, // Matrix_RB_L
	{0x3308,0x07}, // Matrix_GR_H
	{0x3309,0xBE}, // Matrix_GR_L
	{0x330A,0x06}, // Matrix_GG_H
	{0x330B,0xF9}, // Matrix_GG_L
	{0x330C,0x01}, // Matrix_GB_H
	{0x330D,0x46}, // Matrix_GB_L
	{0x330E,0x01}, // Matrix_BR_H
	{0x330F,0x10}, // 91 Matrix_BR_L
	{0x3310,0x06}, // Matrix_BG_H
	{0x3311,0xFD}, // 80 Matrix_BG_L
	{0x3312,0x07}, // Matrix_BB_H
	{0x3313,0xEF}, // EF Matrix_BB_L

	{0x3325,0x5F}, // Unknow Reg
	{0x3326,0x03}, // Unknow Reg
	{0x3327,0x0A}, // Unknow Reg
	{0x3328,0x0A}, // Unknow Reg
	{0x3329,0x06}, // Unknow Reg
	{0x332A,0x06}, // Unknow Reg
	{0x332B,0x1C}, // Unknow Reg
	{0x332C,0x1C}, // Unknow Reg
	{0x332D,0x00}, // Unknow Reg
	{0x332E,0x1D}, // Unknow Reg
	{0x332F,0x1F}, // Unknow Reg

	{0x3330,0x00}, // Unknow Reg
	{0x3331,0x04}, // Unknow Reg
	{0x3332,0x80}, // Unknow Reg
	{0x3338,0x18}, // Unknow Reg
	{0x3339,0x63}, // Unknow Reg
	{0x333A,0x36}, // Unknow Reg
	{0x333F,0x07}, // Unknow Reg

	{0x3360,0x0c}, // 1.825X // 10 Unknow Reg
	{0x3361,0x17}, // 3X // 14 // 18 Unknow Reg
	{0x3362,0x37}, // 6X // 33 // 1F Unknow Reg
	{0x3363,0x37}, // Unknow Reg
	{0x3364,0x98}, // Unknow Reg
	{0x3365,0x8c}, // Unknow Reg
	{0x3366,0x80}, // Unknow Reg
	{0x3367,0x68}, // Unknow Reg
	{0x3368,0x68}, // Unknow Reg
	{0x3369,0x50}, // Unknow Reg
	{0x336A,0x38}, // Unknow Reg
	{0x336B,0x1c}, // Unknow Reg
	{0x336C,0x00}, // Unknow Reg
	{0x336D,0x20}, // Unknow Reg
	{0x336E,0x1C}, // Unknow Reg
	{0x336F,0x18}, // Unknow Reg
	{0x3370,0x10}, // Unknow Reg
	{0x3371,0x30}, // Unknow Reg
	{0x3372,0x34}, // Unknow Reg
	{0x3373,0x3a}, // Unknow Reg
	{0x3374,0x3f}, // Unknow Reg
	{0x3375,0x08}, // Unknow Reg
	{0x3376,0x08}, // Unknow Reg
	{0x3377,0x08}, // Unknow Reg
	{0x3378,0x08}, // Unknow Reg
	{0x338A,0x34}, // Unknow Reg
	{0x338B,0x7F}, // Unknow Reg
	{0x338C,0x10}, // Unknow Reg
	{0x338D,0x23}, // Unknow Reg
	{0x338E,0x7F}, // Unknow Reg
	{0x338F,0x14}, // Unknow Reg*/

	{0x3060,0x01}, // Reg_Activate_Ctrl
};

LOCAL const SENSOR_REG_T s_reg_640_480_30fps[] = {
	// [YUYV_640x480_30.00_30.00_Fps]
	// SOC Auto-Exposure Registers (0x32B0)
	{0x32BF, 0x60}, // AEC_Min_ExpLine, Rst: 40
	{0x32C0, 0x5A}, // AEC_Max_ExpLine, Rst: 80
	{0x32C1, 0x5A}, // Rserved
	{0x32C2, 0x5A}, // Rserved
	{0x32C3, 0x00}, // AGC_Min_Limit, Rst: 00
	//{0x32C4, 0x18}, // AGC_Max_Limit, Rst: 20
	{0x32C5, 0x17}, // Reserved
	{0x32C6, 0x21}, // Reserved
	{0x32C7, 0x00}, // AE_Control_1, Rst: 00
	{0x32C8, 0xE0}, // AEC_Flicker_Base_L, Rst: 3C
	{0x32C9, 0x5A}, // AE_EV_A_L, Rst: 58
	{0x32CA, 0x71}, // AE_EV_B_L, Rst: 68
	{0x32CB, 0x71}, // AE_EV_C_L, Rst: 7C
	{0x32CC, 0x7B}, // AE_EV_D_L, Rst: 84
	{0x32CD, 0x7B}, // AE_EV_E_L, Rst: 98
	//{0x32DB, 0x7B}, // Unknow Reg

	// SOC Scaler Registers (0x32E0)
	{0x32E0, 0x02}, // Scaler_Out_Size_X_H, 0x280 -> 640
	{0x32E1, 0x80}, // Scaler_Out_Size_X_L
	{0x32E2, 0x01}, // Scaler_Out_Size_Y_H, 0x1E0 -> 480
	{0x32E3, 0xE0}, // Scaler_Out_Size_Y_L
	{0x32E4, 0x00}, // HSC_SCF_I
	{0x32E5, 0x80}, // HSC_SCF
	{0x32E6, 0x00}, // VSC_SCF_I
	{0x32E7, 0x80}, // VSC_SCF

	{0x3200, 0x3E}, // Mode_Control_0
	//{0x3201, 0x0F}, // Mode_Control_1
	{0x3201, 0x7F}, // Mode_Control_1
	{0x3028, 0x0B}, // PLL_CTRL1
	{0x3029, 0x00}, // PLL_CTRL2
	{0x302A, 0x04}, // PLL_CTRL3

	//{0x3022, 0x24},
	{0x3022, 0x27}, // Read_Mode_0
	{0x3023, 0x24}, // Read_Mode_1

	{0x3002, 0x00}, // X_Addr_Start_H
	{0x3003, 0x04}, // X_Addr_Start_L
	{0x3004, 0x00}, // Y_Addr_Start_H
	{0x3005, 0x04}, // Y_Addr_Start_L
	{0x3006, 0x02}, // X_Addr_End_H
	{0x3007, 0x83}, // X_Addr_End_L
	{0x3008, 0x01}, // Y_Addr_End_H
	{0x3009, 0xE3}, // Y_Addr_End_L
	{0x300A, 0x06}, // Line_Length_Pck_H
	{0x300B, 0x7C}, // Line_Length_Pck_L
	{0x300C, 0x02}, // Frame_Length_Line_H
	{0x300D, 0xE9}, // Frame_Length_Line_L
	{0x300E, 0x05}, // X_Output_Size_H, 0x500 -> 1280
	{0x300F, 0x00}, // X_Output_Size_L
	{0x3010, 0x02}, // Y_Output_Size_H, 0x2D0 -> 720
	{0x3011, 0xD0}, // Y_Output_Size_L
	{0x3021, 0x06}, // Reset_Register 
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_1280_720_30fps[] = {
	// [YUYV_1280x720_30.00_30.01_Fps]
	// SOC Auto-Exposure Registers (0x32B0)
	{0x32BF, 0x60}, // AEC_Min_ExpLine, Rst: 40
	{0x32C0, 0x5A}, // AEC_Max_ExpLine, Rst: 80
	{0x32C1, 0x5A}, // Rserved
	{0x32C2, 0x5A}, // Reserved
	{0x32C3, 0x00}, // AGC_Min_Limit, Rst: 00
	//{0x32C4, 0x18}, // AGC_Max_Limit, Rst: 20
	{0x32C5, 0x17}, // Reserved
	{0x32C6, 0x21}, // Reserved
	{0x32C7, 0x00}, // AE_Control_1, Rst: 00
	{0x32C8, 0xE0}, // AEC_Flicker_Base_L, Rst: 3C
	{0x32C9, 0x5A}, // AE_EV_A_L, Rst: 58
	{0x32CA, 0x71}, // AE_EV_B_L, Rst: 68
	{0x32CB, 0x71}, // AE_EV_C_L, Rst: 7C
	{0x32CC, 0x7B}, // AE_EV_D_L, Rst: 84
	{0x32CD, 0x7B}, // AE_EV_E_L, Rst: 98

	//{0x32DB, 0x7B}, // Unknow Reg
	// SOC Scaler Registers (0x32E0)
	{0x32E0, 0x05}, // Scaler_Out_Size_X_H, 0x500 -> 1280
	{0x32E1, 0x00}, // Scaler_Out_Size_X_L
	{0x32E2, 0x02}, // Scaler_Out_Size_Y_H, 0x2D0 -> 720
	{0x32E3, 0xD0}, // Scaler_Out_Size_Y_L
	{0x32E4, 0x00}, // HSC_SCF_I
	{0x32E5, 0x00}, // HSC_SCF
	{0x32E6, 0x00}, // VSC_SCF_I
	{0x32E7, 0x00}, // VSC_SCF

	{0x3200, 0x3E}, // Mode_Control_0
	//{0x3201, 0x0F}, // Mode_Control_1
	{0x3201, 0x7F}, // Mode_Control_1

	{0x3028, 0x0B}, // PLL_CTRL1
	{0x3029, 0x00}, // PLL_CTRL2
	{0x302A, 0x04}, // PLL_CTRL3

	//{0x3022, 0x24},
	{0x3022, 0x27}, // Read_Mode_0
	{0x3023, 0x24}, // Read_Mode_1

	{0x3002, 0x00}, // X_Addr_Start_H
	{0x3003, 0x04}, // X_Addr_Start_L
	{0x3004, 0x00}, // Y_Addr_Start_H
	{0x3005, 0x04}, // Y_Addr_Start_L
	{0x3006, 0x05}, // X_Addr_End_H
	{0x3007, 0x03}, // X_Addr_End_L
	{0x3008, 0x02}, // Y_Addr_End_H
	{0x3009, 0xD3}, // Y_Addr_End_L
	{0x300A, 0x06}, // Line_Length_Pck_H
	{0x300B, 0x7C}, // Line_Length_Pck_L
	{0x300C, 0x02}, // Frame_Length_Line_H
	{0x300D, 0xE9}, // Frame_Length_Line_L
	{0x300E, 0x05}, // X_Output_Size_H, 0x500 -> 1280
	{0x300F, 0x00}, // X_Output_Size_L
	{0x3010, 0x02}, // Y_Output_Size_H, 0x2D0 -> 720
	{0x3011, 0xD0}, // Y_Output_Size_L
	{0x3021, 0x06}, // Reset_Register 
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_band_50hz[] = {
	// SOC Auto-Exposure Registers (0x32B0)
	{0x32BF, 0x60},	// AEC_Min_ExpLine
	{0x32C0, 0x5A},	// AEC_Max_ExpLine 
	{0x32C1, 0x5A},	// Reserved
	{0x32C2, 0x5A},	// Reserved
	{0x32C3, 0x00},	// AGC_Min_Limit
	{0x32C4, 0x18},	// AGC_Max_Limit
    {0x32C5, 0x17},	// Reserved
    {0x32C6, 0x21},	// Reserved
	{0x32C7, 0x00},	// AE_Control_1
	{0x32C8, 0xE0},	// AEC_Flicker_Base_L
	{0x32C9, 0x5A},	// AE_EV_A_L
	{0x32CA, 0x71},	// AE_EV_B_L
	{0x32CB, 0x71},	// AE_EV_C_L
	{0x32CC, 0x7B},	// AE_EV_D_L
	{0x32CD, 0x7B},	// AE_EV_E_L
	{0x32DB, 0x7B}	// ??? Unknow Reg*/
	/*{0x32BF, 0x60},
    {0x32C0, 0x5A},
    {0x32C1, 0x5A},
    {0x32C2, 0x5A},
    {0x32C3, 0x00},
    {0x32C4, 0x2B},
    {0x32C5, 0x27},
    {0x32C6, 0x27},
    {0x32C7, 0x00},
    {0x32C8, 0xF1},
    {0x32C9, 0x5A},
    {0x32CA, 0x81},
    {0x32CB, 0x81},
    {0x32CC, 0x81},
    {0x32CD, 0x81},
    {0x32DB, 0x7E}*/
};

LOCAL const SENSOR_REG_T s_reg_band_60hz[] = {
	// SOC Auto-Exposure Registers (0x32B0)
	{0x32BF, 0x60},	// AEC_Min_ExpLine
	{0x32C0, 0x5A},	// AEC_Max_ExpLine 
	{0x32C1, 0x5F},	// Reserved
	{0x32C2, 0x5F},	// Reserved
	{0x32C3, 0x00},	// AGC_Min_Limit
	{0x32C4, 0x18},	// AGC_Max_Limit
	{0x32C5, 0x17},	// Reserved
	{0x32C6, 0x21},	// Reserved
	{0x32C7, 0x00},	// AE_Control_1
	{0x32C8, 0xBA},	// AEC_Flicker_Base_L
	{0x32C9, 0x5F},	// AE_EV_A_L
	{0x32CA, 0x76},	// AE_EV_B_L
	{0x32CB, 0x76},	// AE_EV_C_L
	{0x32CC, 0x80},	// AE_EV_D_L
	{0x32CD, 0x81},	// AE_EV_E_L
	{0x32DB, 0x77}	// ??? Unknow Reg
};

LOCAL const SENSOR_REG_T s_reg_awb_auto[] = {
	{0x3290, 0x01},	// WB_Gain_R_H
	{0x3291, 0x38},	// WB_Gain_R_L
	{0x3296, 0x01},	// WB_Gain_B_H
	{0x3297, 0x68},	// WB_Gain_B_L
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_awb_daylight[] = { // 6500k
	// WB sunny
	{0x3290, 0x01},	// WB_Gain_R_H
	{0x3291, 0x38},	// WB_Gain_R_L
	{0x3296, 0x01},	// WB_Gain_B_H
	{0x3297, 0x68},	// WB_Gain_B_L
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_awb_fluorescent[] = { // 5000k
	// WB Office
	{0x3290, 0x01},	// WB_Gain_R_H
	{0x3291, 0x70},	// WB_Gain_R_L
	{0x3296, 0x01},	// WB_Gain_B_H
	{0x3297, 0xFF},	// WB_Gain_B_L
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_awb_cloudy[] = { // 7500k
	// WB Cloudy
	{0x3290, 0x01},	// WB_Gain_R_H
	{0x3291, 0x51},	// WB_Gain_R_L
	{0x3296, 0x01},	// WB_Gain_B_H
	{0x3297, 0x00},	// WB_Gain_B_L
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_awb_tungsten[] = { //
	// WB HOME
	{0x3290, 0x01},	// WB_Gain_R_H
	{0x3291, 0x30},	// WB_Gain_R_L
	{0x3296, 0x01},	// WB_Gain_B_H
	{0x3297, 0xCB},	// WB_Gain_B_L
	{0x3060, 0x01}
};

LOCAL const SENSOR_REG_T s_reg_ae_n3[] = {
	{0x32F2, 0x54}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_ae_n2[] = {
	{0x32F2, 0x64}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_ae_n1[] = {
	{0x32F2, 0x74}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_ae_0[] = {
	{0x32F2, 0x84}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_ae_p1[] = {
	{0x32F2, 0x94}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_ae_p2[] = {
	{0x32F2, 0xA4}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_ae_p3[] = {
	{0x32F2, 0xB4}	// FIXME: Y_Component
};

LOCAL const SENSOR_REG_T s_reg_bright_n3[] = { // -3
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_bright_n2[] = { // -2
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_bright_n1[] = { // -1
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_bright_0[] = { // 0
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_bright_p1[] = { // 1
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_bright_p2[] = { // 2
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_bright_p3[] = { // 3
	{0x32F1, 0x00}	// FIXME: Special_Effect_0
};

LOCAL const SENSOR_REG_T s_reg_contrast_n3[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL const SENSOR_REG_T s_reg_contrast_n2[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL const SENSOR_REG_T s_reg_contrast_n1[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL const SENSOR_REG_T s_reg_contrast_0[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL const SENSOR_REG_T s_reg_contrast_p1[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL const SENSOR_REG_T s_reg_contrast_p2[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL const SENSOR_REG_T s_reg_contrast_p3[] = {
	{0x32F2, 0x84},	// FIXME: Y_Component
    {0x32F3, 0x80}	// FIXME: Chroma_Component
};

LOCAL SENSOR_REG_TAB_INFO_T s_nt99141_resolution_tab_yuv[] = {
	{ADDR_AND_LEN_OF_ARRAY(s_reg_common), 0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(s_reg_1280_720_30fps), 1280, 720, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{ADDR_AND_LEN_OF_ARRAY(s_reg_640_480_30fps), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

#if 0
LOCAL SENSOR_TRIM_T s_nt99141_resolution_trim_tab[] = {
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},

	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0}
};

LOCAL const SENSOR_REG_T s_nt99141_1280x720_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL SENSOR_VIDEO_INFO_T s_nt99141_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};
#endif

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_nt99141_ioctl_func_tab = {
	/* Internal IOCTL function */
	PNULL,						// reset
	_nt99141_poweron,			// power
	PNULL,						// enter_sleep
	_nt99141_identify,			// identify

	PNULL,						// write_reg
	PNULL,						// read_reg

	/* Custom function */
	PNULL,						// cus_func_1
	PNULL,						// _nt99141_get_resolution_trim_tab,

	/* External IOCTL function */
	PNULL,						// ae_enable
	PNULL,						// hmirror_enable
	PNULL,						// vmirror_enable

	_nt99141_set_brightness,	// _nt99141_set_brightness,
	_nt99141_set_contrast,		// _nt99141_set_contrast,
	PNULL,
	PNULL,						// _nt99141_set_saturation,

	PNULL,						// _nt99141_set_work_mode, // hdr: param0/6, sce
	_nt99141_set_image_effect,	// set_image_effect,

	PNULL,//_nt99141_before_snapshot,	// before_snapshot
	PNULL,//_nt99141_after_snapshot,	// after_snapshot
	_nt99141_flash,				// flash
	PNULL,						// read_ae_value
	PNULL,						// write_ae_value
	PNULL,						// read_gain_value
	PNULL,						// write_gain_value
	PNULL,						// read_gain_scale
	PNULL,						// set_frame_rate
	PNULL,						// af_enable
	PNULL,						// af_get_status
	_nt99141_set_awb,			// set_wb_mode
	PNULL,						// get_skip_frame
	PNULL,						// set_iso
	_nt99141_set_exposure_compensation,		// set_exposure_compensation
	PNULL,						// check_image_format_support
	PNULL,						// change_image_format
	PNULL,						// set_zoom

	/* Customer function */
	PNULL,						// _nt99141_GetExifInfo,
	PNULL,						// set_focus
	_nt99141_set_anti_flicker,	// set_anti_banding_flicker
	PNULL,//_nt99141_set_video_mode,	// set_video_mode
	PNULL,						// pick_jpeg_stream
	PNULL,						// meter_mode
	PNULL,						// get_status
	PNULL,						// stream_on
	PNULL,						// stream_off
	PNULL						// cfg_otp
};

SENSOR_INFO_T g_nt99141_yuv_info = {
	NT99141_I2C_ADDR_W,			// salve i2c write address
	NT99141_I2C_ADDR_R,			// salve i2c read address

	SENSOR_I2C_VAL_8BIT | SENSOR_I2C_REG_16BIT | SENSOR_I2C_FREQ_200,
	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr is 8 bit, 1: i2c register addr is 16 bit
	// other bit: reseved

	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,
	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
 
	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE |
		SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	SENSOR_WB_MODE_AUTO|
        SENSOR_WB_MODE_INCANDESCENCE|
        SENSOR_WB_MODE_U30|
        SENSOR_WB_MODE_CWF|
        SENSOR_WB_MODE_FLUORESCENT|
        SENSOR_WB_MODE_SUN|
        SENSOR_WB_MODE_CLOUD,

	7,
	// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	10,						// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	2,						// count of identify code
	{{0x3000, 0x14},		// supply two code to identify sensor.
	 {0x3001, 0x10}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_3000MV,		// voltage of avdd

	1280,					// max width of source image
	720,					// max height of source image
	"NT99141",				// name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,
	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image from sensor;

	s_nt99141_resolution_tab_yuv,		// point to resolution table information structure
	&s_nt99141_ioctl_func_tab,			// point to ioctl function table
	PNULL,					// information and table about Rawrgb sensor
	PNULL,					// extend information about sensor
	SENSOR_AVDD_1800MV,		// iovdd
	SENSOR_AVDD_1500MV,		// dvdd
	4,						// skip frame num before preview
	3,						// skip frame num before capture
	0,						// deci frame num during preview
	0,						// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	PNULL,
	3,						// skip frame num while change setting
};

LOCAL void _nt99141_write_reg(uint16_t subaddr, uint8_t data)
{	
	Sensor_WriteReg(subaddr, data);

	SENSOR_PRINT("SENSOR: _nt99141_write_reg reg/value(0x%04x, 0x%02X) !!\n", subaddr, data);
}

LOCAL uint8_t _nt99141_read_reg(uint16_t subaddr)
{
	uint8_t value = 0;

	value = Sensor_ReadReg(subaddr);

	SENSOR_PRINT("SENSOR: nt99141_read_reg reg/value(0x%04X, 0x%02X) !!\n", subaddr, value);

	return value;
}

LOCAL uint32_t _nt99141_poweron(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_nt99141_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_nt99141_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_nt99141_yuv_info.iovdd_val;
	BOOLEAN power_down = g_nt99141_yuv_info.power_down_level;
	BOOLEAN reset_level = g_nt99141_yuv_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_3300MV);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		Sensor_Reset(reset_level);
		usleep(10*1000);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT_HIGH("SENSOR_nt99141: _nt99141_Power_On(1:on, 0:off): %d\n", power_on);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _nt99141_identify(uint32_t param)
{
	#define NT99141_PID_VALUE		0x14
	#define NT99141_PID_ADDR		0x3000
	#define NT99141_VER_VALUE		0x10
	#define NT99141_VER_ADDR		0x3001

	uint32_t i;
	uint32_t loop;
	uint8_t ret;
	uint32_t err_cnt = 0;
	uint16_t reg[2] = {NT99141_PID_ADDR, NT99141_VER_ADDR};
	uint8_t value[2] = {NT99141_PID_VALUE, NT99141_VER_VALUE};

	SENSOR_PRINT_HIGH("NT99141_Identify");

	for (i=0; i<2; ) {
		loop = 1000;
		ret = _nt99141_read_reg(reg[i]);
		if(ret != value[i]) {
			err_cnt++;
			if (err_cnt > 3) {
				SENSOR_PRINT_HIGH("It is not NT99141\n");
				return SENSOR_FAIL;
			} else {
				while (loop--);
				continue;
			}
		}

		err_cnt = 0;
		i++;
	}

	SENSOR_PRINT_HIGH("_nt99141_identify: it is NT99141\n");
	return (uint32_t)SENSOR_SUCCESS;
}

// 闪光灯
LOCAL uint32_t _nt99141_flash(uint32_t param)
{
	Sensor_SetFlash(param);
	return SENSOR_SUCCESS;
}

// 闪烁频率
LOCAL uint32_t _nt99141_set_anti_flicker(uint32_t param)
{
	const SENSOR_REG_T *reg_table_ptr = NULL;
	uint32_t reg_table_len = 0, i = 0;

	switch (param) {
	case FLICKER_50HZ:
		reg_table_ptr = s_reg_band_50hz;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_band_50hz);
		break;

	case FLICKER_60HZ:
		reg_table_ptr = s_reg_band_60hz;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_band_60hz);
		break;

	default:
		break;
	}

	for (i=0; i<reg_table_len; i++) {
		_nt99141_write_reg(reg_table_ptr[i].reg_addr, (uint8_t)reg_table_ptr[i].reg_value);
	}

	return SENSOR_SUCCESS;
}

// 图像效果
LOCAL uint32_t _nt99141_set_image_effect(uint32_t param)
{
	_nt99141_write_reg(0x32f1, 0x00);	// effect normal

	return SENSOR_SUCCESS;
}

// 白平衡
LOCAL uint32_t _nt99141_set_awb(uint32_t param)
{
	const SENSOR_REG_T *reg_table_ptr = NULL;
	uint32_t reg_table_len = 0, i = 0;

	SENSOR_PRINT_HIGH("_nt99141_set_awb: %d\n", param);

	switch (param) {
	case 0:		// 自动
		reg_table_ptr = s_reg_awb_auto;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_awb_auto);
		break;

	case 1:		// 白炽光
		reg_table_ptr = s_reg_awb_tungsten;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_awb_tungsten);
		break;

	case 4:		// 荧光
		reg_table_ptr = s_reg_awb_fluorescent;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_awb_fluorescent);
		break;

	case 5:		// 日光
		reg_table_ptr = s_reg_awb_daylight;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_awb_daylight);
		break;

	case 6:		// 阴天
		reg_table_ptr = s_reg_awb_cloudy;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_awb_cloudy);
		break;

	default:
		reg_table_ptr = s_reg_awb_auto;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_awb_auto);
		break;
	}


	for (i=0; i<reg_table_len; i++) {
		_nt99141_write_reg(reg_table_ptr[i].reg_addr, (uint8_t)reg_table_ptr[i].reg_value);
	}

	return SENSOR_SUCCESS;
}

// 曝光
LOCAL uint32_t _nt99141_set_exposure_compensation(uint32_t param)
{
	const SENSOR_REG_T *reg_table_ptr = NULL;
	uint32_t reg_table_len = 0, i = 0;

	SENSOR_PRINT_HIGH("_nt99141_set_exposure_compensation: %d\n", param);

	switch (param) {
	case 0:
		reg_table_ptr = s_reg_ae_n3;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_n3);
		break;

	case 1:
		reg_table_ptr = s_reg_ae_n2;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_n2);
		break;

	case 2:
		reg_table_ptr = s_reg_ae_n1;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_n1);
		break;

	case 3:
		reg_table_ptr = s_reg_ae_0;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_0);
		break;

	case 4:
		reg_table_ptr = s_reg_ae_p1;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_p1);
		break;

	case 5:
		reg_table_ptr = s_reg_ae_p2;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_p2);
		break;

	case 6:
		reg_table_ptr = s_reg_ae_p3;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_p3);
		break;

	default:
		reg_table_ptr = s_reg_ae_0;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_ae_0);
		break;
	}

	for (i=0; i<reg_table_len; i++) {
		_nt99141_write_reg(reg_table_ptr[i].reg_addr, (uint8_t)reg_table_ptr[i].reg_value);
	}

	return SENSOR_SUCCESS;
}

// 亮度
LOCAL uint32_t _nt99141_set_brightness(uint32_t param)
{
	const SENSOR_REG_T *reg_table_ptr = NULL;
	uint32_t reg_table_len = 0, i = 0;

	SENSOR_PRINT_HIGH("_nt99141_set_brightness: %d\n", param);

	switch (param) {
	case 0:
		reg_table_ptr = s_reg_bright_n3;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_n3);
		break;

	case 1:
		reg_table_ptr = s_reg_bright_n2;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_n2);
		break;

	case 2:
		reg_table_ptr = s_reg_bright_n1;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_n1);
		break;

	case 3:
		reg_table_ptr = s_reg_bright_0;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_0);
		break;

	case 4:
		reg_table_ptr = s_reg_bright_p1;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_p1);
		break;

	case 5:
		reg_table_ptr = s_reg_bright_p2;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_p2);
		break;

	case 6:
		reg_table_ptr = s_reg_bright_p3;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_p3);
		break;

	default:
		reg_table_ptr = s_reg_bright_0;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_bright_0);
		break;
	}

	for (i=0; i<reg_table_len; i++) {
		_nt99141_write_reg(reg_table_ptr[i].reg_addr, (uint8_t)reg_table_ptr[i].reg_value);
	}

	return SENSOR_SUCCESS;
}

// 对比度
LOCAL uint32_t _nt99141_set_contrast(uint32_t param)
{
	const SENSOR_REG_T *reg_table_ptr = NULL;
	uint32_t reg_table_len = 0, i = 0;

	SENSOR_PRINT_HIGH("_nt99141_set_contrast: %d\n", param);

	switch (param) {
	case 0:
		reg_table_ptr = s_reg_contrast_n3;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_n3);
		break;

	case 1:
		reg_table_ptr = s_reg_contrast_n2;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_n2);
		break;

	case 2:
		reg_table_ptr = s_reg_contrast_n1;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_n1);
		break;

	case 3:
		reg_table_ptr = s_reg_contrast_0;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_0);
		break;

	case 4:
		reg_table_ptr = s_reg_contrast_p1;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_p1);
		break;

	case 5:
		reg_table_ptr = s_reg_contrast_p2;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_p2);
		break;

	case 6:
		reg_table_ptr = s_reg_contrast_p3;
		reg_table_len = NUMBER_OF_ARRAY(s_reg_contrast_p3);
		break;

	default:
		break;
	}

	for (i=0; i<reg_table_len; i++) {
		_nt99141_write_reg(reg_table_ptr[i].reg_addr, (uint8_t)reg_table_ptr[i].reg_value);
	}

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _nt99141_set_video_mode(uint32_t param)
{
	/*SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX) {
		return 0;
	}

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_gc2235_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_gc2235_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%02x", param);*/

	return SENSOR_SUCCESS;
}

/*LOCAL uint32_t _nt99141_get_resolution_trim_tab(uint32 param)
{
	return (uint32_t)s_nt99141_resolution_trim_tab;
}*/
