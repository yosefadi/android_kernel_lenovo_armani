/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define DEBUG
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_yassy.h"

#define TE_TIME (100 / 1000)
#ifdef CONFIG_HW_ANDORRA
//#define ESD_FOR_YASSY
//#define ESD_USE_TIMER
#ifdef ESD_FOR_YASSY
extern int dsi_mdp_busy;
extern void yassy_lcd_reset(struct fb_info *info);
#ifdef ESD_USE_TIMER
struct timer_list te_timer;
int yassy_te_check = 0;
#else
struct mutex te_lock;
struct delayed_work lcd_delayed_work;
#endif
#endif
#endif
struct msm_fb_data_type *mfd_priv;
static struct msm_panel_common_pdata *mipi_yassy_pdata;
static struct dsi_buf yassy_tx_buf;
static struct dsi_buf yassy_rx_buf;
#define YASSY_DELAY_TIME		0
#define YASSY_SLEEPOUT_TIME	200
#define  YASSY_CABC_ON
static char yassy_set1[] = {0x00,0x00};
static char yassy_set2[] = {0xff,0x96,0x08,0x01};
static char yassy_set3[] = {0x00,0x80};
static char yassy_set4[] = {0xff,0x96,0x08};
static char yassy_set5[] = {0x00,0x00};
static char yassy_set6[] = {0xa0,0x00};
static char yassy_set7[] = {0x00,0x80};
static char yassy_set8[] = {0xb3,0x00,0x00,0x20,0x00,0x00};
static char yassy_set9[] = {0x00,0xc0};
static char yassy_set10[] = {0xb3,0x09};
static char yassy_set11[] = {0x00,0x80};
static char yassy_set12[] = {0xc0,0x00,0x46,0x00,0x08,0x08,0x00,0x45,0x08,0x08};
static char yassy_set13[] = {0x00,0x92};
static char yassy_set14[] = {0xc0,0x00,0x10,0x00,0x13};
static char yassy_set15[] = {0x00,0xa2};
static char yassy_set16[] = {0xc0,0x0c,0x05,0x02};
static char yassy_set17[] = {0x00,0xb3};
static char yassy_set18[] = {0xc0,0x00,0x50};
static char yassy_set19[] = {0x00,0x81};
static char yassy_set20[] = {0xc1,0x55};
static char yassy_set21[] = {0x00,0x80};
static char yassy_set22[] = {0xc4,0x00,0x84,0xfc};
static char yassy_set23[] = {0x00,0xa0};
static char yassy_set24[] = {0xb3,0x10,0x00};
static char yassy_set25[] = {0x00,0xa0};
static char yassy_set26[] = {0xc0,0x00};
static char yassy_set27[] = {0x00,0xa0};
static char yassy_set28[] = {0xc4,0x33,0x09,0x90,0x2b,0x33,0x09,0x90,0x54};
static char yassy_set29[] = {0x00,0x80};
static char yassy_set30[] = {0xc5,0x08,0x00,0xa0,0x11};
static char yassy_set31[] = {0x00,0x90};
static char yassy_set32[] = {0xc5,0xd6,0x57,0x00,0x57,0x33,0x33,0x34};
static char yassy_set33[] = {0x00,0xd0};
static char yassy_set34[] = {0xc5,0xd6,0x57,0x00,0x57,0x33,0x33,0x34};
static char yassy_set35[] = {0x00,0xb0};
static char yassy_set36[] = {0xc5,0x04,0xac,0x01,0x00,0x71,0xb1,0x83};
static char yassy_set37[] = {0x00,0x00};
static char yassy_set38[] = {0xd9,0x5f};
static char yassy_set39[] = {0x00,0xb4};
static char yassy_set40[] = {0xc6,0x50};
static char yassy_set41[] = {0x00,0xb0};
static char yassy_set42[] = {0xc6,0x03,0x10,0x00,0x1f,0x12};
static char yassy_set43[] = {0x00,0xe1};
static char yassy_set44[] = {0xc0,0x9f};
static char yassy_set45[] = {0x00,0xb7};
static char yassy_set46[] = {0xb0,0x10};
static char yassy_set47[] = {0x00,0xc0};
static char yassy_set48[] = {0xb0,0x55};
static char yassy_set49[] = {0x00,0xb1};
static char yassy_set50[] = {0xb0,0x03};
static char yassy_set51[] = {0x00,0x81};
static char yassy_set52[] = {0xd6,0x00};
static char yassy_set53[] = {0x00,0x00};
static char yassy_set54[] = {0x00,0x00};
static char yassy_set55[] = {0xe1,0x01,0x10,0x12,0x0f,0x08,0x2c,0x0b,0x0a,0x03,0x07,0x12,0x08,0x0d,0x0e,0x09,0x01};
static char yassy_set56[] = {0xe1,0x01,0x10,0x12,0x0f,0x07,0x2c,0x0b,0x0a,0x03,0x07,0x12,0x08,0x0d,0x0e,0x09,0x01};
static char yassy_set57[] = {0x00,0x80};
static char yassy_set58[] = {0xcb,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set59[] = {0x00,0x90};
static char yassy_set60[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set61[] = {0x00,0xa0};
static char yassy_set62[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set63[] = {0x00,0xb0};
static char yassy_set64[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set65[] = {0x00,0xc0};
static char yassy_set66[] = {0xcb,0x00,0x00,0x00,0x04,0x00,0x00,0x04,0x04,0x00,0x00,0x04,0x04,0x04,0x00,0x00};
static char yassy_set67[] = {0x00,0xd0};
static char yassy_set68[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x00,0x00,0x04,0x04};
static char yassy_set69[] = {0x00,0xe0};
static char yassy_set70[] = {0xcb,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00};
static char yassy_set71[] = {0x00,0xf0};
static char yassy_set72[] = {0xcb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static char yassy_set73[] = {0x00,0x80};
static char yassy_set74[] = {0xcc,0x00,0x00,0x00,0x02,0x00,0x00,0x0a,0x0e,0x00,0x00};
static char yassy_set75[] = {0x00,0x90};
static char yassy_set76[] = {0xcc,0x0c,0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x09};
static char yassy_set77[] = {0x00,0xa0};
static char yassy_set78[] = {0xcc,0x0d,0x00,0x00,0x0b,0x0f,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00};
static char yassy_set79[] = {0x00,0xb0};
static char yassy_set80[] = {0xcc,0x00,0x00,0x00,0x02,0x00,0x00,0x0a,0x0e,0x00,0x00};
static char yassy_set81[] = {0x00,0xc0};
static char yassy_set82[] = {0xcc,0x0c,0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set83[] = {0x00,0xd0};
static char yassy_set84[] = {0xcc,0x05,0x00,0x00,0x00,0x00,0x0f,0x0b,0x00,0x00,0x0d,0x09,0x01,0x00,0x00,0x00};
static char yassy_set85[] = {0x00,0x80};
static char yassy_set86[] = {0xce,0x84,0x03,0x18,0x83,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set87[] = {0x00,0x90};
static char yassy_set88[] = {0xce,0x33,0xbf,0x18,0x33,0xc0,0x18,0x10,0x0f,0x18,0x10,0x10,0x18,0x00,0x00};
static char yassy_set89[] = {0x00,0xa0};
static char yassy_set90[] = {0xce,0x38,0x02,0x03,0xc1,0x00,0x18,0x00,0x38,0x01,0x03,0xc2,0x00,0x18,0x00};
static char yassy_set91[] = {0x00,0xb0};
static char yassy_set92[] = {0xce,0x38,0x00,0x03,0xc3,0x00,0x18,0x00,0x30,0x00,0x03,0xc4,0x00,0x18,0x00};
static char yassy_set93[] = {0x00,0xc0};
static char yassy_set94[] = {0xce,0x30,0x01,0x03,0xc5,0x00,0x18,0x00,0x30,0x02,0x03,0xc6,0x00,0x18,0x00};
static char yassy_set95[] = {0x00,0xd0};
static char yassy_set96[] = {0xce,0x30,0x03,0x03,0xc7,0x00,0x18,0x00,0x30,0x04,0x03,0xc8,0x00,0x18,0x00};
static char yassy_set97[] = {0x00,0x80};
static char yassy_set98[] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set99[] = {0x00,0x90};
static char yassy_set100[] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set101[] = {0x00,0xa0};
static char yassy_set102[] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set103[] = {0x00,0xb0};
static char yassy_set104[] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char yassy_set105[] = {0x00,0xc0};
static char yassy_set106[] = {0xcf,0x01,0x01,0x20,0x20,0x00,0x00,0x02,0x04,0x00,0x00};
static char yassy_set107[] = {0x00,0x00};
static char yassy_set108[] = {0xd8,0x97,0x97};
#if 1
//cabc open
#ifdef YASSY_CABC_ON
//static char yassy_cabc_set1[] = {0x51,0xff};
static char yassy_cabc_set2[] = {0x53,0x2c};
static char yassy_cabc_set3[] = {0x55,0x01};
#endif
//cabc freq control
static char yassy_freq_set1[] = {0x00,0xb1};
static char yassy_freq_set2[] = {0xc6,0xff};
//TE control
#ifdef YASSY_TE_ON
static char yassy_freq_set3[] = {0x35,0x00};
#endif

static char yassy_freq_set4[] = {0x00,0x00};
static char yassy_freq_set5[] = {0xff,0xff,0xff,0xff};
static char yassy_freq_set6[] = {0x11};
static char yassy_freq_set7[] = {0x29};
static char yassy_freq_set8[] = {0x2c};
#endif



static struct dsi_cmd_desc yassy_cmd_on_cmds_video_mode[] = {
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set1), yassy_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set2), yassy_set2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set3), yassy_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set4), yassy_set4},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set5), yassy_set5},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set6), yassy_set6},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set7), yassy_set7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set8), yassy_set8},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set9), yassy_set9},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set10), yassy_set10},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set11), yassy_set11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set12), yassy_set12},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set13), yassy_set13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set14), yassy_set14},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set15), yassy_set15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set16), yassy_set16},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set17), yassy_set17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set18), yassy_set18},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set19), yassy_set19},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set20), yassy_set20},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set21), yassy_set21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set22), yassy_set22},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set23), yassy_set23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set24), yassy_set24},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set25), yassy_set25},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set26), yassy_set26},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set27), yassy_set27},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set28), yassy_set28},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set29), yassy_set29},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set30), yassy_set30},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set31), yassy_set31},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set32), yassy_set32},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set33), yassy_set33},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set34), yassy_set34},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set35), yassy_set35},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set36), yassy_set36},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set37), yassy_set37},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set38), yassy_set38},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set39), yassy_set39},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set40), yassy_set40},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set41), yassy_set41},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set42), yassy_set42},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set43), yassy_set43},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set44), yassy_set44},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set45), yassy_set45},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set46), yassy_set46},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set47), yassy_set47},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set48), yassy_set48},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set49), yassy_set49},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set50), yassy_set50},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set51), yassy_set51},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set52), yassy_set52},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set53), yassy_set53},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set54), yassy_set54},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set55), yassy_set55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set56), yassy_set56},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set57), yassy_set57},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set58), yassy_set58},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set59), yassy_set59},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set60), yassy_set60},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set61), yassy_set61},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set62), yassy_set62},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set63), yassy_set63},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set64), yassy_set64},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set65), yassy_set65},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set66), yassy_set66},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set67), yassy_set67},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set68), yassy_set68},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set69), yassy_set69},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set70), yassy_set70},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set71), yassy_set71},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set72), yassy_set72},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set73), yassy_set73},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set74), yassy_set74},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set75), yassy_set75},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set76), yassy_set76},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set77), yassy_set77},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set78), yassy_set78},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set79), yassy_set79},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set80), yassy_set80},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set81), yassy_set81},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set82), yassy_set82},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set83), yassy_set83},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set84), yassy_set84},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set85), yassy_set85},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set86), yassy_set86},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set87), yassy_set87},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set88), yassy_set88},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set89), yassy_set89},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set90), yassy_set90},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set91), yassy_set91},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set92), yassy_set92},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set93), yassy_set93},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set94), yassy_set94},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set95), yassy_set95},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set96), yassy_set96},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set97), yassy_set97},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set98), yassy_set98},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set99), yassy_set99},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set100), yassy_set100},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set101), yassy_set101},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set102), yassy_set102},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set103), yassy_set103},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set104), yassy_set104},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set105), yassy_set105},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set106), yassy_set106},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set107), yassy_set107},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_set108), yassy_set108},

#ifdef YASSY_CABC_ON
	//{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_cabc_set1), yassy_cabc_set1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_cabc_set2), yassy_cabc_set2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_cabc_set3), yassy_cabc_set3},
#endif

	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_freq_set1), yassy_freq_set1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_freq_set2), yassy_freq_set2},
#ifdef  YASSY_TE_ON
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_freq_set3), yassy_freq_set3},
#endif
	{DTYPE_GEN_WRITE1, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_freq_set4), yassy_freq_set4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, YASSY_DELAY_TIME, sizeof(yassy_freq_set5), yassy_freq_set5},
	{DTYPE_GEN_WRITE, 1, 0, 0, YASSY_SLEEPOUT_TIME, sizeof(yassy_freq_set6), yassy_freq_set6},
	{DTYPE_GEN_WRITE, 1, 0, 0, 20, sizeof(yassy_freq_set7), yassy_freq_set7},
	{DTYPE_GEN_WRITE, 1, 0, 0, 20, sizeof(yassy_freq_set8), yassy_freq_set8},
};

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc yassy_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};
static int mipi_yassy_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    static int first_time_on_avoid_lcd_flush = 0;
    printk("##MIPI::%s:  , first_time=%d#######\n", __func__,first_time_on_avoid_lcd_flush);

	mfd = platform_get_drvdata(pdev);
	mfd_priv = mfd;
    if(first_time_on_avoid_lcd_flush)
    {
    	first_time_on_avoid_lcd_flush = 0;
    	return 0;
    }

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pr_info("%s enter cont_splash_done = %d\n", __func__, mfd->cont_splash_done);
	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	msleep(50);
	mipi_dsi_cmds_tx(&yassy_tx_buf, yassy_cmd_on_cmds_video_mode,
			ARRAY_SIZE(yassy_cmd_on_cmds_video_mode));
#ifdef ESD_FOR_YASSY
#ifdef ESD_USE_TIMER
	mod_timer(&te_timer, TE_TIME  * HZ + jiffies);
#else
	schedule_delayed_work(&lcd_delayed_work, HZ * 5);
#endif
#endif
	pr_info("%s done\n", __func__);
	
	return 0;
}

static int mipi_yassy_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_info("%s\n", __func__);

#ifdef ESD_FOR_YASSY
#ifdef ESD_USE_TIMER
	del_timer(&te_timer);
	yassy_te_check = 0;
#else
	cancel_delayed_work(&lcd_delayed_work);
#endif
#endif
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&yassy_tx_buf, yassy_display_off_cmds,
			ARRAY_SIZE(yassy_display_off_cmds));

	return 0;
}
static void set_pmic_backlight(struct msm_fb_data_type *mfd)
{

	char led_pwm2[2] = {0x51, 0x1E}; /* DTYPE_DCS_WRITE1 */
	struct dsi_cmd_desc novatek_cmd_backlight_cmds2[] = {
		{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm2), led_pwm2},
	};
	led_pwm2[1] = (unsigned char)(mfd->bl_level);
	//printk("%s bl_level = %d\n", __func__, mfd->bl_level);

	mutex_lock(&mfd->dma->ov_mutex);
	
	/* mdp4_dsi_cmd_busy_wait: will turn on dsi clock also */
	mdp4_dsi_cmd_dma_busy_wait(mfd);
	
	mdp4_dsi_blt_dmap_busy_wait(mfd);

	mipi_dsi_mdp_busy_wait(mfd);
	
	//printk("@@@@@@@@@@@@@@@@@@@@led_pwm2[1] =%x\n",led_pwm2[1] );
	mipi_dsi_cmds_tx(&yassy_tx_buf, novatek_cmd_backlight_cmds2,
			ARRAY_SIZE(novatek_cmd_backlight_cmds2));
	mutex_unlock(&mfd->dma->ov_mutex);
	return;

}
static void mipi_yassy_set_backlight(struct msm_fb_data_type *mfd)
{
	int32 level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;

	//if (mipi_yassy_pdata->backlight_level)
	//	printk("************** mipi_yassy_pdata->backlight_level ***********************\n");
	if (mipi_yassy_pdata && mipi_yassy_pdata->backlight_level) {
		level = mipi_yassy_pdata->backlight_level(mfd->bl_level, max, min);
		printk("%s bl_level = %d\n", __func__, mfd->bl_level);
		set_pmic_backlight(mfd);
		if (level < 0) {
			pr_info("%s: backlight level control failed\n", __func__);
		}
	} else {
		pr_info("%s: missing baclight control function\n", __func__);
	}

	return;
}
#ifdef ESD_FOR_YASSY
static char te_reg[] = {0x0a, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc yassy_te_cmd = {
	    DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(te_reg), te_reg};
unsigned int mipi_yassy_lcd_detect_cb(void)
{
	struct dsi_buf * rp, *tp; 
	struct dsi_cmd_desc * cmd; 
	uint32 id = 0; 
	int i;
	tp = &yassy_tx_buf;
	rp = &yassy_rx_buf;

	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	cmd = &yassy_te_cmd;

	if(dsi_mdp_busy == false)
	{
		mutex_lock(&mfd_priv->dma->ov_mutex);
		for (i = 0; i < 4; i++) {
			te_reg[0] = 0x0a + i;
			mipi_dsi_cmds_rx(mfd_priv, tp, rp, cmd, 4);
			id |= (rp->data[0] << (i * 8));
		}
		mutex_unlock(&mfd_priv->dma->ov_mutex);
	}else
		pr_info("@@@@@@@@@@dsi_mdp_busy = false.\n");
	return id;
}
#ifdef ESD_USE_TIMER
void te_timer_handle(unsigned long data)
{
	yassy_te_check = 1;
}
#else
static void lcd_work_handle(struct work_struct *work)
{
	uint32 id;

	mutex_lock(&te_lock);
	id = mipi_yassy_lcd_detect_cb();
	mutex_unlock(&te_lock);
	if(id != 0x9c0000 && id != 0x9c0007) {
		printk("************** %s reset ****************\n", __func__);
		yassy_lcd_reset(mfd_priv->fbi);
		mipi_yassy_set_backlight(mfd_priv);
	} else {
		schedule_delayed_work(&lcd_delayed_work, HZ);
	}
}
#endif
#endif
#define DELAY_TIME 10
static int __devinit mipi_yassy_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_yassy_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
#ifdef ESD_FOR_YASSY
#ifdef ESD_USE_TIMER
	init_timer(&te_timer);
	te_timer.function = te_timer_handle;
	mod_timer(&te_timer, HZ * 10 + jiffies);
#else
	mutex_init(&te_lock);
	INIT_DELAYED_WORK(&lcd_delayed_work, lcd_work_handle);
	schedule_delayed_work(&lcd_delayed_work, DELAY_TIME * HZ);
#endif
#endif

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_yassy_lcd_probe,
	.driver = {
		.name   = "mipi_yassy",
	},
};

static struct msm_fb_panel_data yassy_panel_data = {
	.on		= mipi_yassy_lcd_on,
	.off		= mipi_yassy_lcd_off,
	.set_backlight	= mipi_yassy_set_backlight,
};

static int ch_used[3];

int mipi_yassy_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk("############## %s ################\n", __func__);
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_yassy", (panel << 8)|channel);

	if (!pdev)
		return -ENOMEM;

	yassy_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &yassy_panel_data,
				sizeof(yassy_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);

	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	pr_err("####%s: platform_device_register exit!#####\n", __func__);

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_yassy_lcd_init(void)
{
	pr_info("###%s#####\n",__func__);
	mipi_dsi_buf_alloc(&yassy_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&yassy_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
module_init(mipi_yassy_lcd_init);
