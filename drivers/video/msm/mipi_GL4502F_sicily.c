/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/device.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "msm_fb_panel.h"
#include "mipi_GL4502F_sicily.h"

static struct msm_panel_common_pdata *mipi_GL4502F_alaska_pdata;
static struct dsi_buf GL4502F_alaska_tx_buf;
static struct dsi_buf GL4502F_alaska_rx_buf;

extern  int mipi_dsi_panel_power_lenovo(int on);
extern int g_lcd;
int dsi_mdp_busy;
int orise_lcd = 0;
struct msm_fb_data_type *mfd_test;
/************************* Power ON Command **********************/
/***********************BOE**************************************/
static char enable_page1[6]={		//Manufacture command set control
	0xF0,0x55,0xAA,0x52,0x08,0x01
};
static char avdd_set1[4]={			//Power control AVDD
	0xB6,0x34,0x34,0x34
};
static char avdd_set2[4]={			//AVDD Voltage 6.5-(9X0.1)=5.6
	0xB0,0x09,0x09,0x09
};
static char avee_set1[4]={			//Power control AVDD Booster time and booster frequency -2.0*VDDB/(HS/2)  
	0xB7,0x34,0x34,0x34
};
static char avee_set2[4]={			//AVEE Voltage 6.5-(9X0.1)=5.6
	0xB1,0x09,0x09,0x09
};
static char vcl_power_set1[4]={ 	//Power control VCL -2*VDDB,HS/2
     0xB8,0x24,0x24,0x24};
static char vcl_power_set2[4]={ 	//VCL Voltage -3.0V 
  0xB2,0x00,0x00,0x00};
static char vgh_enable1[4]={		//Power control VGH AVDD-AVEE+VDDB,HS/2
	0xB9,0x24,0x24,0x24
};
static char bf_set[2]={				
	0xBF,0x01
};
static char vgh_enable2[4]={		//VGH Voltage 15V 
     0xB3,0x05,0x05,0x05
};
static char vgl_set[4]={			//Power control VGLX AVEE+VCL-AVDD,HS/2
	0xBA,0x34,0x34,0x34
};
static char vgl_reg_set[4]={		//VGL_REG Voltage -10V
	0xB5,0x0B,0x0B,0x0B
};
static char vgmp_set[4]={			//VGMP=3.0+128*0.0125=4.6V,VGSP=0V
	0xBC,0x00,0xA3,0x00
};
static char vgmn_set[4]={			//VGMN=-(3.0+128*0.0125)=-4.6V,VGSN=0V
	0xBD,0x00,0xA3,0x00
};
static char vcom_set[3]={			//VCOM OFFSET Voltage -47*0.0125=-0.5875; 2F
	0xBE,0x00,0x45
};
static char red_set1[53]={
	0xD1,0x00,0x01,0x00,0xD4,0x00,0xF0,0x01,0x04,0x01,0x16,0x01,0x32,0x01,0x4A,0x01,0x72,0x01,0x91,0x01,0xC2,0x01,0xEA,0x02,0x2A,0x02,0x60,0x02,0x62,0x02,0x9F,0x02,0xC8,0x02,0xEA,0x03,0x14,0x03,0x2D,0x03,0x4E,0x03,0x5E,0x03,0x6D,0x03,0x76,0x03,0x7B,0x03,0x7D,0x03,0xFE
};
static char green_set1[53]={
	0xD2,0x00,0x9B,0x00,0xB5,0x00,0xD7,0x00,0xE5,0x01,0x01,0x01,0x24,0x01,0x3F,0x01,0x68,0x01,0x89,0x01,0xBD,0x01,0xE5,0x02,0x28,0x02,0x5D,0x02,0x5F,0x02,0x92,0x02,0xC7,0x02,0xEB,0x03,0x14,0x03,0x32,0x03,0x56,0x03,0x6D,0x03,0x8E,0x03,0x9D,0x03,0xAD,0x03,0xB3,0x03,0xFE
};
static char blue_set1[53]={
	0xD3,0x00,0x01,0x00,0x9F,0x00,0xC8,0x00,0xE7,0x00,0xFA,0x01,0x1E,0x01,0x3A,0x01,0x62,0x01,0x84,0x01,0xB7,0x01,0xDE,0x02,0x22,0x02,0x5A,0x02,0x5C,0x02,0x92,0x02,0xC3,0x02,0xEA,0x03,0x16,0x03,0x30,0x03,0x63,0x03,0x79,0x03,0x86,0x03,0x93,0x03,0xA3,0x03,0xA9,0x03,0xFE
};
static char red_set2[53]={
	0xD4,0x00,0x01,0x00,0xD4,0x00,0xF0,0x01,0x04,0x01,0x16,0x01,0x32,0x01,0x4A,0x01,0x72,0x01,0x91,0x01,0xC2,0x01,0xEA,0x02,0x2A,0x02,0x60,0x02,0x62,0x02,0x9F,0x02,0xC8,0x02,0xEA,0x03,0x14,0x03,0x2D,0x03,0x4E,0x03,0x5E,0x03,0x6D,0x03,0x76,0x03,0x7B,0x03,0x7D,0x03,0xFE
};
static char green_set2[53]={
	0xD5,0x00,0x9B,0x00,0xB5,0x00,0xD7,0x00,0xE5,0x01,0x01,0x01,0x24,0x01,0x3F,0x01,0x68,0x01,0x89,0x01,0xBD,0x01,0xE5,0x02,0x28,0x02,0x5D,0x02,0x5F,0x02,0x92,0x02,0xC7,0x02,0xEB,0x03,0x14,0x03,0x32,0x03,0x56,0x03,0x6D,0x03,0x8E,0x03,0x9D,0x03,0xAD,0x03,0xB3,0x03,0xFE
};
static char blue_set2[53]={
	0xD6,0x00,0x01,0x00,0x9F,0x00,0xC8,0x00,0xE7,0x00,0xFA,0x01,0x1E,0x01,0x3A,0x01,0x62,0x01,0x84,0x01,0xB7,0x01,0xDE,0x02,0x22,0x02,0x5A,0x02,0x5C,0x02,0x92,0x02,0xC3,0x02,0xEA,0x03,0x16,0x03,0x30,0x03,0x63,0x03,0x79,0x03,0x86,0x03,0x93,0x03,0xA3,0x03,0xA9,0x03,0xFE
};

static char enable_page0[6]={			//Page0 set
	0xF0,0x55,0xAA,0x52,0x08,0x00
};
static char res_set[2]={				//Resolution set
	0xB5,0x6B
};
static char sdt_set[2]={				//Source output data hold time 2.5us
	0xB6,0x0A
};
static char gate_eq_set[3]={			//EQ Control for gate signal 0us
	0xB7,0x00,0x00
};
static char src_eq_set[5]={			//EQ Control for source signal 5*0.5=2.5us
	0xB8,0x01,0x05,0x05,0x05
};
static char clkb_set[2]={				//Source control in vertical porch time, off color
	0xBA,0x01
};
static char clm_set[4]={				//Inversion driving control, column inversion
	0xBC,0x00,0x00,0x00
};
static char boe_set[4]={				//Display timing control for VGSW[3:0]
	0xCC,0x03,0x00,0x00
};
static char dis_time_set[6]={			//Display timing control
	0xBD,0x01, 0x6c, 0x1d,0x1d, 0x00
};
static char dis_time_set2[6]={			//Display timing control
	0xBE,0x01,0x84,0x07,0x31,0x00
};
static char dis_time_set3[6]={			//Display timing control
	0xBF,0x01,0x84,0x07,0x31,0x00
};

static char cmd_set3[1]={				//Color invert
    0x21
};
static char cmd_set6[1]={				//Sleep out 
    0x11
};
static char cmd_set7[1]={				//Display on
    0x29
};
static char cmd_last[3]={
	0xB1,0xE8,0x00
};
static char cmd_36_set[2]={				//
	0x36,0x00
};
static char cmd_35_set[2]={				//
    0x35,0x00
};

static struct dsi_cmd_desc GL4502F_alaska_cmd_on_cmds_video_mode[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enable_page1), enable_page1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avdd_set1), avdd_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avdd_set2), avdd_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avee_set1), avee_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(avee_set2), avee_set2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(vcl_power_set1), vcl_power_set1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(vcl_power_set2), vcl_power_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgh_enable1), vgh_enable1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(bf_set), bf_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgh_enable2), vgh_enable2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgl_set), vgl_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgl_reg_set), vgl_reg_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgmp_set), vgmp_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vgmn_set), vgmn_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(vcom_set), vcom_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(red_set1), red_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(green_set1), green_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(blue_set1), blue_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(red_set2), red_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(green_set2), green_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(blue_set2), blue_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enable_page0), enable_page0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(res_set), res_set},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(sdt_set), sdt_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(gate_eq_set), gate_eq_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(src_eq_set), src_eq_set},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(clkb_set), clkb_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(clm_set), clm_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dis_time_set), dis_time_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dis_time_set2), dis_time_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(dis_time_set3), dis_time_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_set), boe_set},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmd_last), cmd_last}, 	
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd_36_set), cmd_36_set},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd_set3), cmd_set3},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cmd_35_set), cmd_35_set},
	{DTYPE_GEN_WRITE, 1, 0, 0, 120, sizeof(cmd_set6), cmd_set6},
	{DTYPE_GEN_WRITE, 1, 0, 0, 0, sizeof(cmd_set7), cmd_set7},
};
static char CMI1_set[1] = {0x11};  
  static char CMI2_set[2] = {0x00,0x00};
  static char CMI3_set[4] = {0xff,0x80,0x09,0x01};
  static char CMI4_set[2] = {0x00,0x80};
  static char CMI5_set[3] = {0xff,0x80,0x09};
  static char CMI6_set[1] = {0x11};
 static char CMI7_set[2] = {0x00,0x80};
  static char CMI8_set[13] = {0xf5,0x01,0x18,0x02,0x18,0x10,0x18,0x02,0x18,0x0e,0x18,0x0f,0x20};
static char CMI9_set[2] = {0x00,0x90};
  static char CMI10_set[11] = {0xf5,0x02,0x18,0x08,0x18,0x06,0x18,0x0d,0x18,0x0b,0x18};
static char CMI11_set[2] = {0x00,0xa0};
  static char CMI12_set[9] = {0xf5,0x10,0x18,0x01,0x18,0x14,0x18,0x14,0x18};
static char CMI13_set[2] = {0x00,0xb0};
  static char CMI14_set[13] = {0xf5,0x14,0x18,0x12,0x18,0x13,0x18,0x11,0x18,0x13,0x18,0x00,0x00};
static char CMI15_set[2] = {0x00,0xb4};
  static char CMI16_set[2] = {0xc0,0x55};
  static char CMI17_set[2] = {0x00,0x82};
  static char CMI18_set[2] = {0xc5,0xa3};
  static char CMI19_set[2] = {0x00,0x90};
  static char CMI20_set[3] = {0xc5,0xd6,0x76};
  static char CMI21_set[2] = {0x00,0x00};
  static char CMI22_set[3] = {0xd8,0xa7,0xa7}; //5.2V
  static char CMI23_set[2] = {0x00,0x00};
  static char CMI24_set[2] = {0xd9,0x74}; //-1.7375
  static char CMI25_set[2] = {0x00,0x81};
  static char CMI26_set[2] = {0xc1,0x66};
static char CMI27_set[2] = {0x00,0xa1};
  static char CMI28_set[2] = {0xc1,0x08};
static char CMI29_set[2] = {0x00,0xa3};
  static char CMI30_set[2] = {0xc0,0x1b};// pre charge time
static char CMI31_set[2] = {0x00,0x81};
  static char CMI32_set[2] = {0xc4,0x83};//Source output current
static char CMI33_set[2] = {0x00,0x92};
  static char CMI34_set[2] = {0xc5,0x01};
static char CMI35_set[2] = {0x00,0xb1};
  static char CMI36_set[2] = {0xc5,0xa9};
static char CMI37_set[2] = {0x00,0x80};
  static char CMI38_set[7] = {0xce,0x84,0x03,0x00,0x83,0x03,0x00};
  static char CMI39_set[2] = {0x00,0x90};
  static char CMI40_set[2] = {0xb3,0x02};
static char CMI41_set[2] = {0x00,0x92};
  static char CMI42_set[2] = {0xb3,0x45};
static char CMI43_set[2] = {0x00,0x80};
  static char CMI44_set[6] = {0xc0,0x00,0x57,0x00,0x15,0x15};
static char CMI45_set[2] = {0x00,0x90};
  static char CMI46_set[7] = {0xce,0x33,0x5d,0x00,0x33,0x5e,0x00};
static char CMI47_set[2] = {0x00,0xa0};
  static char CMI48_set[15] = {0xce,0x38,0x02,0x03,0x57,0x00,0x00,0x00,0x38,0x01,0x03,0x58,0x00,0x00,0x00};
static char CMI49_set[2] = {0x00,0xb0};
  static char CMI50_set[15] = {0xce,0x38,0x00,0x03,0x59,0x00,0x00,0x00,0x30,0x00,0x03,0x5a,0x00,0x00,0x00};
static char CMI51_set[2] = {0x00,0xc0};
  static char CMI52_set[15] = {0xce,0x30,0x01,0x03,0x5b,0x00,0x00,0x00,0x30,0x02,0x03,0x5c,0x00,0x00,0x00};
static char CMI53_set[2] = {0x00,0xd0};
  static char CMI54_set[15] = {0xce,0x30,0x03,0x03,0x5d,0x00,0x00,0x00,0x30,0x04,0x03,0x5e,0x00,0x00,0x00};
 static char CMI55_set[2] = {0x00,0xc7};
  static char CMI56_set[2] = {0xcf,0x00};
  static char CMI57_set[2] = {0x00,0xc0};
  static char CMI58_set[16] = {0xcb,0x00,0x00,0x00,0x00,0x54,0x54,0x54,0x54,0x00,0x54,0x00,0x54,0x00,0x00,0x00};
static char CMI59_set[2] = {0x00,0xd0};
  static char CMI60_set[16] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x54,0x54,0x54,0x54,0x00,0x54};
static char CMI61_set[2] = {0x00,0xe0};
  static char CMI62_set[10] = {0xcb,0x00,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char CMI63_set[2] = {0x00,0x80};
  static char CMI64_set[11] = {0xcc,0x00,0x00,0x00,0x00,0x0c,0x0a,0x10,0x0e,0x00,0x02};
static char CMI65_set[2] = {0x00,0x90};
  static char CMI66_set[16] = {0xcc,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0B}; 
  static char CMI67_set[2] = {0x00,0xa0};
static char CMI68_set[16] = {0xcc,0x09,0x0f,0x0d,0x00,0x01,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  static char CMI69_set[2] = {0x00,0xb0};
static char CMI70_set[11] = {0xcc,0x00,0x00,0x00,0x00,0x0d,0x0f,0x09,0x0b,0x00,0x05};
static char CMI71_set[2] = {0x00,0xc0};
  static char CMI72_set[16] = {0xcc,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e};
static char CMI73_set[2] = {0x00,0xd0};
  static char CMI74_set[16] = {0xcc,0x10,0x0a,0x0c,0x00,0x06,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char CMI75_set[2] = {0x00,0x00};
static char CMI76_set[17] = {0xe1,0x00,0x0d,0x13,0x0f,0x07,0x0f,0x0b,0x0a,0x04,0x08,0x0e,0x09,0x11,0x10,0x08,0x03};
static char CMI77_set[2] = {0x00,0x00};
static char CMI78_set[17] = {0xe2,0x00,0x0d,0x13,0x0f,0x07,0x0f,0x0b,0x0a,0x04,0x08,0x0e,0x09,0x11,0x10,0x08,0x03};
//digital Gamma //
static char CMI85_set[2] = {0x00,0x00};
static char CMI86_set[34] = {0xec,0x40,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x34,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x03};
static char CMI87_set[2] = {0x00,0x00};
static char CMI88_set[34] = {0xed,0x40,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x04};
static char CMI89_set[2] = {0x00,0x00};
static char CMI90_set[34] = {0xee,0x40,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x04};
static char CMI79_set[2] = {0x00,0x00};
static char CMI80_set[2] = {0x26,0x00};
static char CMI81_set[2] = {0x00,0x00};
  static char CMI82_set[5] = {0x2b,0x00,0x00,0x03,0x56};
  
  static char CMI91_set[2] = {0x51,0xFF};
  static char CMI92_set[2] = {0x53,0x0C};
  static char CMI93_set[2] = {0x55,0x03};

  static char CMI83_set[1] = {0x35};
  static char CMI84_set[1] = {0x29};

static struct dsi_cmd_desc CMI_cmd_on_cmds_video_mode[] = {
    {DTYPE_DCS_WRITE, 1, 0, 0, 3, sizeof(CMI1_set), CMI1_set},
    {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI2_set), CMI2_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI3_set), CMI3_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI4_set), CMI4_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 2, sizeof(CMI5_set), CMI5_set},
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(CMI6_set), CMI6_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI7_set), CMI7_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI8_set), CMI8_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI9_set), CMI9_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI10_set), CMI10_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI11_set), CMI11_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI12_set), CMI12_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI13_set), CMI13_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI14_set), CMI14_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI15_set), CMI15_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI16_set), CMI16_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI17_set), CMI17_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI18_set), CMI18_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI19_set), CMI19_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI20_set), CMI20_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI21_set), CMI21_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI22_set), CMI22_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI23_set), CMI23_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI24_set), CMI24_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI25_set), CMI25_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI26_set), CMI26_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI27_set), CMI27_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI28_set), CMI28_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI29_set), CMI29_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI30_set), CMI30_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI31_set), CMI31_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI32_set), CMI32_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI33_set), CMI33_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI34_set), CMI34_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI35_set), CMI35_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI36_set), CMI36_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI37_set), CMI37_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI38_set), CMI38_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI39_set), CMI39_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI40_set), CMI40_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI41_set), CMI41_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI42_set), CMI42_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI43_set), CMI43_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI44_set), CMI44_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI45_set), CMI45_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI46_set), CMI46_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI47_set), CMI47_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI48_set), CMI48_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI49_set), CMI49_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI50_set), CMI50_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI51_set), CMI51_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI52_set), CMI52_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI53_set), CMI53_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI54_set), CMI54_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI55_set), CMI55_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI56_set), CMI56_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI57_set), CMI57_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI58_set), CMI58_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI59_set), CMI59_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI60_set), CMI60_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI61_set), CMI61_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI62_set), CMI62_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI63_set), CMI63_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI64_set), CMI64_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI65_set), CMI65_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI66_set), CMI66_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI67_set), CMI67_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI68_set), CMI68_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI69_set), CMI69_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI70_set), CMI70_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI71_set), CMI71_set},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI72_set), CMI72_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI73_set), CMI73_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI74_set), CMI74_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI75_set), CMI75_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI76_set), CMI76_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI77_set), CMI77_set},
			{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI78_set), CMI78_set},
		        {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI85_set), CMI85_set},
		        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI86_set), CMI86_set},
		        {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI87_set), CMI87_set},
		        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI88_set), CMI88_set},
		        {DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI89_set), CMI89_set},
		        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI90_set), CMI90_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI79_set), CMI79_set},
			{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI80_set), CMI80_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI81_set), CMI81_set},
	        {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMI82_set), CMI82_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI91_set), CMI91_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI92_set), CMI92_set},
		{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMI93_set), CMI93_set},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(CMI83_set), CMI83_set},
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(CMI84_set), CMI84_set},		
};
  
/************************* Power OFF Command **********************/
static char stabdby_off_1[2] = {0x28, 0x00};

static char stabdby_off_2[2] = {0x10, 0x00};

static struct dsi_cmd_desc GL4502F_alaska_display_off_cmds[] = {
    {DTYPE_GEN_WRITE, 1, 0, 0, 0, sizeof(stabdby_off_1), stabdby_off_1},
    {DTYPE_GEN_WRITE, 1, 0, 0, 120, sizeof(stabdby_off_2), stabdby_off_2},
};
#if 0
static struct dsi_cmd_desc nt35510_alaska_password[] = {
       {DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(enable_page1), enable_page1}
};
static char lcd_id[2] = {0xc5, 0x00};
struct dsi_cmd_desc cmd_read_id = {
		DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(lcd_id), lcd_id};
static u32 mipi_lcd_id(struct msm_fb_data_type * mfd)
{
	struct dsi_buf * rp, *tp;
	struct dsi_cmd_desc * cmd;
	uint32 return_len;
	uint32 id0 = 0, id1 = 0;
		
	printk("@@@@@@@@@@@@@@@@@@##::%s: ###\n", __func__);
	tp = &GL4502F_alaska_tx_buf;
	rp = &GL4502F_alaska_rx_buf;
		
       cmd = &cmd_read_id;
	return_len = mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 4);
	id0 = rp->data[0];
	id1 = rp->data[1];
	pr_info("%s: @data0=0x%08x.\n", __func__, id0);
	pr_info("%s: @data1=0x%08x.\n", __func__, id1);
       if(id0 == 0x55 && id1 == 0x10)
	 {
			pr_info("%s: LCD is BOE\n", __func__);
			return 1;
	 }
	else
	{
			pr_info("%s: LCD is baolongda\n", __func__);
			return 0;
		}
 }
#endif
#if 0
int mipi_novatek_lcd_detect_cb(void)
{
   struct dsi_buf * rp, *tp;
   struct dsi_cmd_desc * cmd;
   //uint32 id = 0;
   tp = &GL4502F_alaska_tx_buf;
   rp = &GL4502F_alaska_rx_buf;

   mipi_dsi_buf_init(rp);
   mipi_dsi_buf_init(tp);
   cmd = &cmd_read_id2;

#if 1
   if(dsi_mdp_busy == false)
   {
       //pr_info("@@@@@@@@@@readid = 111.\n");
       mutex_lock(&mfd_test->dma->ov_mutex);
       mipi_dsi_cmds_rx(mfd_test, tp, rp, cmd, 4);
       mutex_unlock(&mfd_test->dma->ov_mutex);
	   id = rp->data[0];
	   if(id == 0x9c || id == 0x00)
           //pr_info("%s: @@@@@@@@@@data0=0x%08x.\n", __func__, id);
	       return 1;
   }else
       pr_info("@@@@@@@@@@dsi_mdp_busy = false.\n");
   pr_info("%s: esd error data0=0x%08x.\n", __func__, id);
#endif
   return 0;

}
#endif
void mipi_GL4502F_alaska_panel_set_backlight(struct msm_fb_data_type *mfd)
{
    int32 level;
    int max = mfd->panel_info.bl_max;
    int min = mfd->panel_info.bl_min;
	
    if (mipi_GL4502F_alaska_pdata && mipi_GL4502F_alaska_pdata->backlight_level) {
        level = mipi_GL4502F_alaska_pdata->backlight_level(mfd->bl_level, max, min);

        if (level < 0) {
            printk("%s: backlight level control failed\n", __func__);
        }
    } else {
        printk("%s: missing baclight control function\n", __func__);
    }

    return;
}

static int mipi_GL4502F_alaska_lcd_on(struct platform_device *pdev)
{
    struct msm_fb_data_type *mfd;
    static int first_time_on_avoid_lcd_flush =0;

#if 0
    static int first_time_delect_boe_lcd=1;
#endif
    printk("##MIPI::%s:  , first_time=%d#######\n", __func__,first_time_on_avoid_lcd_flush);
   orise_lcd = 0;

    mfd = platform_get_drvdata(pdev);
	mfd_test = mfd;
    if (!mfd)
        return -ENODEV;
    if (mfd->key != MFD_KEY)
        return -EINVAL;
    if(first_time_on_avoid_lcd_flush)
    {
    	 first_time_on_avoid_lcd_flush = 0;
    	return 0;
    }

    if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}
    printk("%s: Enter\n", __func__);
#if 0
    if(first_time_delect_boe_lcd)
    {
    		mipi_dsi_cmds_tx(&GL4502F_alaska_tx_buf,
			nt35510_alaska_password,ARRAY_SIZE(nt35510_alaska_password));		
		mipi_dsi_cmd_bta_sw_trigger();
		g_lcd = mipi_lcd_id(mfd);
		mipi_dsi_panel_power_lenovo(1);
		first_time_delect_boe_lcd = 0;
    }
#endif

    if(g_lcd){
		pr_info("%s: LCD is BOE111\n", __func__);		
		mipi_dsi_cmds_tx(&GL4502F_alaska_tx_buf,
				GL4502F_alaska_cmd_on_cmds_video_mode,	  ARRAY_SIZE(GL4502F_alaska_cmd_on_cmds_video_mode));
    }else{   
		pr_info("%s: LCD is CMI\n", __func__);
                mipi_dsi_cmds_tx(&GL4502F_alaska_tx_buf,
                CMI_cmd_on_cmds_video_mode,    ARRAY_SIZE(CMI_cmd_on_cmds_video_mode));
    }    
#if 0
    if(mfd->panel_info.type == MIPI_VIDEO_PANEL)
    {
        printk("[LCM]in %s. video panel\n", __func__);
        mipi_dsi_cmds_tx(mfd, &GL4502F_alaska_tx_buf,
            GL4502F_alaska_cmd_on_cmds_video_mode,    ARRAY_SIZE(GL4502F_alaska_cmd_on_cmds_video_mode));
    }
    else//if(mfd->panel_info.type = MIPI_CMD_PANEL)
    {
        printk("[LCM]in %s. cmd panel\n", __func__);
        mipi_dsi_cmds_tx(mfd, &GL4502F_alaska_tx_buf,
            GL4502F_alaska_cmd_on_cmds_video_mode, ARRAY_SIZE(GL4502F_alaska_cmd_on_cmds_video_mode));
    }
#endif
    printk("%s: Done\n", __func__);
    return 0;
}

static int mipi_GL4502F_alaska_lcd_off(struct platform_device *pdev)
{
    struct msm_fb_data_type *mfd;

    printk("%s: Enter\n", __func__);
    mfd = platform_get_drvdata(pdev);

    if (!mfd)
        return -ENODEV;
    if (mfd->key != MFD_KEY)
        return -EINVAL;
    mipi_dsi_cmds_tx(&GL4502F_alaska_tx_buf, GL4502F_alaska_display_off_cmds, ARRAY_SIZE(GL4502F_alaska_display_off_cmds));

    printk("%s: Done\n", __func__);
    return 0;
}

static int __devinit mipi_GL4502F_alaska_lcd_probe(struct platform_device *pdev)
{
    printk("%s: Enter\n", __func__);
    if (pdev->id == 0) {
        mipi_GL4502F_alaska_pdata = pdev->dev.platform_data;
        return 0;
    }
    msm_fb_add_device(pdev);

    printk("%s: Done\n", __func__);
    return 0;
}

static struct platform_driver this_driver = {
    .probe  = mipi_GL4502F_alaska_lcd_probe,
    .driver = {
        .name   = "mipi_GL4502F_alaska",
    },
};

static struct msm_fb_panel_data GL4502F_alaska_panel_data = {
    .on    = mipi_GL4502F_alaska_lcd_on,
    .off    = mipi_GL4502F_alaska_lcd_off,
    .set_backlight    = mipi_GL4502F_alaska_panel_set_backlight,
};

static int ch_used[3];

int mipi_GL4502F_alaska_device_register(struct msm_panel_info *pinfo,
                    u32 channel, u32 panel)
{
    struct platform_device *pdev = NULL;
    int ret;

    printk("%s\n", __func__);

    if ((channel >= 3) || ch_used[channel])
        return -ENODEV;

    ch_used[channel] = TRUE;

    pdev = platform_device_alloc("mipi_GL4502F_alaska", (panel << 8)|channel);
    if (!pdev)
        return -ENOMEM;

    GL4502F_alaska_panel_data.panel_info = *pinfo;

    ret = platform_device_add_data(pdev, &GL4502F_alaska_panel_data, sizeof(GL4502F_alaska_panel_data));
    if (ret) {
        printk(KERN_ERR"%s: platform_device_add_data failed!\n", __func__);
        goto err_device_put;
    }

    ret = platform_device_add(pdev);
    if (ret) {
        printk(KERN_ERR"%s: platform_device_register failed!\n", __func__);
        goto err_device_put;
    }

    return 0;

err_device_put:
    platform_device_put(pdev);
    return ret;
}

static int __init mipi_GL4502F_alaska_lcd_init(void)
{
    printk("%s\n", __func__);

    mipi_dsi_buf_alloc(&GL4502F_alaska_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&GL4502F_alaska_rx_buf, DSI_BUF_SIZE);

    return platform_driver_register(&this_driver);
}

module_init(mipi_GL4502F_alaska_lcd_init);

