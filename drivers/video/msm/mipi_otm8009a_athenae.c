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
#include "mipi_otm8009a.h"
#include <mach/pmic.h>
#include <linux/gpio.h>


static struct msm_panel_common_pdata *mipi_otm8009a_pdata;
static struct dsi_buf otm8009a_tx_buf;
static struct dsi_buf otm8009a_rx_buf;

#define OTM8009A_DELAY_TIME		0
#define OTM8009A_SLEEPOUT_TIME	200
#define  OTM8009A_CABC_ON

#if 1
static char otm8009a_set1[] = {0x00, 0x00};
static char otm8009a_set2[] = {0xFF, 0x80, 0x09, 0x01};
static char otm8009a_set3[] = {0x00, 0x80};
static char otm8009a_set4[] = {0xFF, 0x80, 0x09};
static char otm8009a_set5[] = {0x00, 0x90};
static char otm8009a_set6[] = {0xB3, 0x02};
static char otm8009a_set7[] = {0x00, 0x92};
static char otm8009a_set8[] = {0xB3, 0x45};
static char otm8009a_set9[] = {0x00, 0xC0};
static char otm8009a_set10[] = {0xB3, 0x11};
static char otm8009a_set11[] = {0x00, 0xA6};
static char otm8009a_set12[] = {0xB3, 0x20};
static char otm8009a_set13[] = {0x00, 0xA7};
static char otm8009a_set14[] = {0xB3, 0x01};

static char otm8009a_set15[] = {0x00, 0xA3};
static char otm8009a_set16[] = {0xC0, 0x00};

static char otm8009a_set17[] = {0x00, 0xB4};
static char otm8009a_set18[] = {0xC0, 0x50};
//static char otm8009a_set19[] = {0x00, 0x80};
//static char otm8009a_set20[] = {0xC0, 0x00, 0x57, 0x00, 0x15, 0x15};
static char otm8009a_set21[] = {0x00, 0x80};

static char otm8009a_set22[] = {0xC4, 0x00, 0x04};

static char otm8009a_set23[] = {0x00, 0x80};
static char otm8009a_set24[] = {0xC5, 0x03, 0x00, 0x03	};

static char otm8009a_set25[] = {0x00, 0x90};
static char otm8009a_set26[] = {0xC5, 0x96, 0x2b, 0x04, 0x7B, 0x33, 0x33, 0x34};

/*
static char otm8009a_set27[] = {0x00, 0xb1};
static char otm8009a_set28[] = {0xc5, 0xa9};
*/

static char otm8009a_set27[] = {0x00, 0xd0};
static char otm8009a_set28[] = {0xcf, 0x00};

static char otm8009a_set29[] = {0x00, 0x00};

static char otm8009a_set30[] = {0xD8, 0x70, 0x70};

static char otm8009a_set31[] = {0x00, 0x00};

static char otm8009a_set32[] = {0xD9, 0x33};

static char otm8009a_set33[] = {0x00, 0x81};
static char otm8009a_set34[] = {0xC1, 0x66};
static char otm8009a_set35[] = {0x00, 0x00};
static char otm8009a_set36[] = {0xE1, 0x08, 0x11, 0x07, 0x0D, 0x06, 0x0d, 0x0A, 0x08, 0x05, 0x08, 0x0e, 0x09, 0x0f, 0x0E, 0x0A, 0x03};
static char otm8009a_set37[] = {0x00, 0x00};
static char otm8009a_set38[] = {0xE2, 0x08, 0x11, 0x07, 0x0D, 0x06, 0x0d, 0x0A, 0x08, 0x05, 0x08, 0x0e, 0x09, 0x0f, 0x0E, 0x0A, 0x03};
static char otm8009a_set39[] = {0x00, 0x80};
static char otm8009a_set40[] = {0xCE, 0x85, 0x01, 0x00, 0x84, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set41[] = {0x00, 0xA0};
static char otm8009a_set42[] = {0xCE, 0x18, 0x02, 0x03, 0x5B, 0x00, 0x00, 0x00, 0x18, 0x01, 0x03, 0x5C, 0x00, 0x00, 0x00};
static char otm8009a_set43[] = {0x00, 0xB0};
static char otm8009a_set44[] = {0xCE, 0x18, 0x04, 0x03, 0x5D, 0x00, 0x00, 0x00, 0x18, 0x03, 0x03, 0x5E, 0x00, 0x00, 0x00};
/*
static char otm8009a_set45[] = {0x00, 0xc0};
static char otm8009a_set46[] = {0xCE, 0x30, 0x01, 0x03, 0x5b, 0x00, 0x00, 0x00, 0x30, 0x02, 0x03, 0x5c, 0x00, 0x00, 0x00};
static char otm8009a_set47[] = {0x00, 0xd0};
static char otm8009a_set48[] = {0xCE, 0x30, 0x03, 0x03, 0x5D, 0x00, 0x00, 0x00, 0x30, 0x04, 0x03, 0x5E, 0x00, 0x00, 0x00};
*/
static char otm8009a_set49[] = {0x00, 0xC0};
static char otm8009a_set50[] = {0xCF, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static char otm8009a_set51[] = {0x00, 0x80};
static char otm8009a_set52[] = {0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set53[] = {0x00, 0x90};
static char otm8009a_set54[] = {0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set55[] = {0x00, 0xA0};
static char otm8009a_set56[] = {0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set57[] = {0x00, 0xB0};
static char otm8009a_set58[] = {0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set59[] = {0x00, 0xC0};
static char otm8009a_set60[] = {0xCB, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set61[] = {0x00, 0xD0};
static char otm8009a_set62[] = {0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set63[] = {0x00, 0xE0};
static char otm8009a_set64[] = {0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set65[] = {0x00, 0xF0};
static char otm8009a_set66[] = {0xCB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static char otm8009a_set67[] = {0x00, 0x80};
static char otm8009a_set68[] = {0xCC, 0x00, 0x26, 0x09, 0x0B, 0x01, 0x25, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set69[] = {0x00, 0x90};
static char otm8009a_set70[] = {0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02};
static char otm8009a_set71[] = {0x00, 0xA0};
static char otm8009a_set72[] = {0xCC, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set73[] = {0x00, 0xB0};
static char otm8009a_set74[] = {0xCC, 0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26, 0x00, 0x00, 0x00, 0x00};
static char otm8009a_set75[] = {0x00, 0xC0};
static char otm8009a_set76[] = {0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01};
static char otm8009a_set77[] = {0x00, 0xD0};
static char otm8009a_set78[] = {0xCC, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*
static char otm8009a_set79[] = {0x00,0x80};
static char otm8009a_set80[] = {0xf5,0x01,0x18,0x02,0x18,0x10,0x18,0x02,0x18,0x0e,0x18,0x0f,0x20};
static char otm8009a_set81[] = {0x00,0x90};
static char otm8009a_set82[] = {0xf5,0x02,0x18,0x08,0x18,0x06,0x18,0x0d,0x18,0x0b,0x18};
static char otm8009a_set83[] = {0x00,0xa0};
static char otm8009a_set84[] = {0xf5,0x10,0x18,0x01,0x18,0x14,0x18,0x14,0x18};
static char otm8009a_set85[] = {0x00,0xb0};
static char otm8009a_set86[] = {0xf5,0x14,0x18,0x12,0x18,0x13,0x18,0x11,0x18,0x13,0x18,0x00,0x00};
*/

static char otm8009a_set87[] = {0x11};
static char otm8009a_set88[] = {0x29};
static char otm8009a_set89[] = {0x2C};
#endif
#if 1
static struct dsi_cmd_desc otm8009a_cmd_on_cmds_video_mode[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set1), otm8009a_set1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set2), otm8009a_set2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set3), otm8009a_set3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set4), otm8009a_set4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set5), otm8009a_set5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set6), otm8009a_set6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set7), otm8009a_set7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set8), otm8009a_set8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set9), otm8009a_set9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set10), otm8009a_set10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set11), otm8009a_set11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set12), otm8009a_set12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set13), otm8009a_set13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set14), otm8009a_set14},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set15), otm8009a_set15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set16), otm8009a_set16},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set17), otm8009a_set17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set18), otm8009a_set18},
	//{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set19), otm8009a_set19},
	//{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set20), otm8009a_set20},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set21), otm8009a_set21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set22), otm8009a_set22},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set23), otm8009a_set23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set24), otm8009a_set24},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set25), otm8009a_set25},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set26), otm8009a_set26},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set27), otm8009a_set27},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set28), otm8009a_set28},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set29), otm8009a_set29},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set30), otm8009a_set30},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set31), otm8009a_set31},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set32), otm8009a_set32},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set33), otm8009a_set33},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set34), otm8009a_set34},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set35), otm8009a_set35},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set36), otm8009a_set36},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set37), otm8009a_set37},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set38), otm8009a_set38},
	{DTYPE_GEN_WRITE1, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set39), otm8009a_set39},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set40), otm8009a_set40},
	{DTYPE_GEN_WRITE1, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set41), otm8009a_set41},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set42), otm8009a_set42},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set43), otm8009a_set43},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set44), otm8009a_set44},
	/*
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set45), otm8009a_set45},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set46), otm8009a_set46},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set47), otm8009a_set47},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set48), otm8009a_set48},
	*/
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set49), otm8009a_set49},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set50), otm8009a_set50},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set51), otm8009a_set51},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set52), otm8009a_set52},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set53), otm8009a_set53},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set54), otm8009a_set54},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set55), otm8009a_set55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set56), otm8009a_set56},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set57), otm8009a_set57},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set58), otm8009a_set58},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set59), otm8009a_set59},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set60), otm8009a_set60},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set61), otm8009a_set61},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set62), otm8009a_set62},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set63), otm8009a_set63},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set64), otm8009a_set64},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set65), otm8009a_set65},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set66), otm8009a_set66},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set67), otm8009a_set67},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set68), otm8009a_set68},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set69), otm8009a_set69},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set70), otm8009a_set70},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set71), otm8009a_set71},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set72), otm8009a_set72},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set73), otm8009a_set73},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set74), otm8009a_set74},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set75), otm8009a_set75},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set76), otm8009a_set76},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set77), otm8009a_set77},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set78), otm8009a_set78},
	/*
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set79), otm8009a_set79},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set80), otm8009a_set80},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set81), otm8009a_set81},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set82), otm8009a_set82},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set83), otm8009a_set83},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set84), otm8009a_set84},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set85), otm8009a_set85},
	{DTYPE_GEN_LWRITE, 1, 0, 0, OTM8009A_DELAY_TIME, sizeof(otm8009a_set86), otm8009a_set86},
	*/
	{DTYPE_GEN_WRITE, 1, 0, 0, 200, sizeof(otm8009a_set87), otm8009a_set87},
	{DTYPE_GEN_WRITE, 1, 0, 0, 20, sizeof(otm8009a_set88), otm8009a_set88},
	{DTYPE_GEN_WRITE, 1, 0, 0, 20, sizeof(otm8009a_set89), otm8009a_set89},

};
#endif

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc otm8009a_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};
static int mipi_otm8009a_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	static int first_time_on_avoid_lcd_flush = 0;
	printk("##MIPI::%s:  , first_time=%d#######\n", __func__,first_time_on_avoid_lcd_flush);

	mfd = platform_get_drvdata(pdev);
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
	/*if (!mfd->cont_splash_done) {
	  mfd->cont_splash_done = 1;
	  pr_info("%s enter cont_splash_done = %d\n exit", __func__, mfd->cont_splash_done);
	  return 0;
	  }*/
	
	mipi_dsi_cmds_tx(&otm8009a_tx_buf, otm8009a_cmd_on_cmds_video_mode,
			ARRAY_SIZE(otm8009a_cmd_on_cmds_video_mode));
	pr_info("%s done\n", __func__);

	return 0;
}

static int mipi_otm8009a_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_info("%s\n", __func__);

	//pmic_gpio_set_value(0,0);
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&otm8009a_tx_buf, otm8009a_display_off_cmds,
			ARRAY_SIZE(otm8009a_display_off_cmds));

	return 0;
}
#if 0
static void set_pmic_backlight(struct msm_fb_data_type *mfd)
{

	char led_pwm2[2] = {0x51, 0x1E}; /* DTYPE_DCS_WRITE1 */
	struct dsi_cmd_desc novatek_cmd_backlight_cmds2[] = {
		{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm2), led_pwm2},
	};
	led_pwm2[1] = (unsigned char)(mfd->bl_level);
	printk("%s bl_level = %d\n", __func__, mfd->bl_level);

	mutex_lock(&mfd->dma->ov_mutex);

	/* mdp4_dsi_cmd_busy_wait: will turn on dsi clock also */
	mdp4_dsi_cmd_dma_busy_wait(mfd);

	mdp4_dsi_blt_dmap_busy_wait(mfd);

	mipi_dsi_mdp_busy_wait(mfd);

	printk("@@@@@@@@@@@@@@@@@@@@led_pwm2[1] =%x\n",led_pwm2[1] );
	mipi_dsi_cmds_tx(&otm8009a_tx_buf, novatek_cmd_backlight_cmds2,
			ARRAY_SIZE(novatek_cmd_backlight_cmds2));
	mutex_unlock(&mfd->dma->ov_mutex);
	return;

}
static void mipi_otm8009a_set_backlight(struct msm_fb_data_type *mfd)
{
	int32 level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;

	if (mipi_otm8009a_pdata->backlight_level)
		printk("************** mipi_otm8009a_pdata->backlight_level ***********************\n");
	if (mipi_otm8009a_pdata && mipi_otm8009a_pdata->backlight_level) {
		level = mipi_otm8009a_pdata->backlight_level(mfd->bl_level, max, min);
		set_pmic_backlight(mfd);
		printk("%s bl_level = %d\n", __func__, mfd->bl_level);
		if (level < 0) {
			pr_info("%s: backlight level control failed\n", __func__);
		}
	} else {
		pr_info("%s: missing baclight control function\n", __func__);
	}

	return;
}
#endif
void mipi_otm8009a_set_backlight(struct msm_fb_data_type *mfd)
{

	int32 level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;

	printk("%s\n", __func__);
	if (mipi_otm8009a_pdata && mipi_otm8009a_pdata->backlight_level) {
		level = mipi_otm8009a_pdata->backlight_level(mfd->bl_level, max, min);

		if (level < 0) {
			printk("%s: backlight level control failed\n", __func__);
		}   
	} else {
		pmic_gpio_set_value(0, 0);
		printk("%s: missing baclight control function\n", __func__);
	}   

	return;
}
static int __devinit mipi_otm8009a_lcd_probe(struct platform_device *pdev)
{
	pr_info("********************** %s enter ******************************\n", __func__);

	if (pdev->id == 0) {
		mipi_otm8009a_pdata = pdev->dev.platform_data;
		return 0;
	}

	pr_info("********************** %s finished ******************************\n", __func__);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_otm8009a_lcd_probe,
	.driver = {
		.name   = "mipi_otm8009a",
	},
};

static struct msm_fb_panel_data otm8009a_panel_data = {
	.on		= mipi_otm8009a_lcd_on,
	.off		= mipi_otm8009a_lcd_off,
	.set_backlight	= mipi_otm8009a_set_backlight,
};

static int ch_used[3];

int mipi_otm8009a_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	printk("############## %s ################\n", __func__);
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_otm8009a", (panel << 8)|channel);

	if (!pdev)
		return -ENOMEM;

	otm8009a_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &otm8009a_panel_data,
			sizeof(otm8009a_panel_data));
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

static int __init mipi_otm8009a_lcd_init(void)
{
	pr_info("###%s#####\n",__func__);
	mipi_dsi_buf_alloc(&otm8009a_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&otm8009a_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_otm8009a_lcd_init);
