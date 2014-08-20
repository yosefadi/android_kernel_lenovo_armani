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
#include "mipi_himax.h"

static struct msm_panel_common_pdata *mipi_himax_pdata;
static struct dsi_buf himax_tx_buf;
static struct dsi_buf himax_rx_buf;

#define HIMAX_CMD_DELAY		0
#define HIMAX_SLEEP_OFF_DELAY	150
#define HIMAX_DISPLAY_ON_DELAY	150
#define GPIO_HIMAX_LCD_RESET	129

static char password_set[4]={
    0xB9,0xFF,0x83,0x69
};

static char power_set[20]={
    0xB1,0x01,0x00,0x44,
    0x0A,0x00,0x0f,0x0f,
    0x2c,0x2c,0x3F,0x3F,
    0x01,0x3A,0x01,0xE6,
    0xE6,0xE6,0xE6,0xE6
};

static char display_set[16]={
    0xB2,0x00,0x10,0x05,
    0x0A,0x70,0x00,0xFF,
    0x05,0x00,0x00,0x00,
    0x03,0x03,0x00,0x01
};

static char columm_set[6]={
    0xB4,0x00,0x1D,0x80,
    0x06,0x02
};

static char HZ_set[3]={
	0xb0,0x01,0x0b
};

static char cmd_0x36_set[2]={
    0x36,0x00
};

static char cmd_0xCC_set[2]={
    0xCC,0x22
};

static char vcom_set[3]={
    0xB6,0x4D,0x4D
};

static char cmd_set1[27]={
    0xD5,0x00,0x01,0x03,
    0x28,0x01,0x04,0x00,
    0x70,0x11,0x13,0x00,
    0x00,0x60,0x04,0x71,
    0x05,0x00,0x00,0x71,
    0x05,0x60,0x04,0x07,
    0x0F,0x04,0x04
};

static char gamma_set[35]={
    0xE0,0x00,0x20,0x26,
    0x34,0x38,0x3f,0x33,
    0x4b,0x09,0x13,0x0e,
    0x15,0x16,0x14,0x15,
    0x11,0x17,0x00,0x20,
    0x26,0x34,0x38,0x3f,
    0x33,0x4b,0x09,0x13,
    0x0e,0x15,0x16,0x14,
    0x15,0x11,0x17
};

#if 0
static char dgc_set[128]={
    0xC1,0x01,0x02,0x08,
    0x12,0x1A,0x22,0x2A,
    0x31,0x36,0x3F,0x48,
    0x51,0x58,0x60,0x68,
    0x70,0x78,0x80,0x88,
    0x90,0x98,0xA0,0xA7,
    0xAF,0xB6,0xBE,0xC7,
    0xCE,0xD6,0xDE,0xE6,
    0xEF,0xF5,0xFB,0xFC,
    0xFE,0x8C,0xA4,0x19,
    0xEC,0x1B,0x4C,0x40,
    0x02,0x08,0x12,0x1A,
    0x22,0x2A,0x31,0x36,
    0x3F,0x48,0x51,0x58,
    0x60,0x68,0x70,0x78,
    0x80,0x88,0x90,0x98,
    0xA0,0xA7,0xAF,0xB6,
    0xBE,0xC7,0xCE,0xD6,
    0xDE,0xE6,0xEF,0xF5,
    0xFB,0xFC,0xFE,0x8C,
    0xA4,0x19,0xEC,0x1B,
    0x4C,0x40,0x02,0x08,
    0x12,0x1A,0x22,0x2A,
    0x31,0x36,0x3F,0x48,
    0x51,0x58,0x60,0x68,
    0x70,0x78,0x80,0x88,
    0x90,0x98,0xA0,0xA7,
    0xAF,0xB6,0xBE,0xC7,
    0xCE,0xD6,0xDE,0xE6,
    0xEF,0xF5,0xFB,0xFC,
    0xFE,0x8C,0xA4,0x19,
    0xEC,0x1B,0x4C,0x40
};

static char cmd_set2[2]={
    0x3A,0x77
};
#endif

static char mipi_set[14]={
    0xBA,0x00,0xA0,0xC6,
    0x00,0x0A,0x00,0x10,
    0x30,0x6F,0x02,0x11,
    0x18,0x40
};

static char cmd_set6[1]={
    0x11
};

static char cmd_set7[1]={
    0x29
};

static struct dsi_cmd_desc himax_cmd_on_cmds_video_mode[] = {
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(password_set), password_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_set), power_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(display_set), display_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(columm_set), columm_set},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(HZ_set), HZ_set},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(cmd_0x36_set), cmd_0x36_set},
    {DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(cmd_0xCC_set), cmd_0xCC_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(vcom_set), vcom_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(cmd_set1), cmd_set1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(gamma_set), gamma_set},
    //{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(dgc_set), dgc_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(mipi_set), mipi_set},
    //{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd_set2), cmd_set2},
    {DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(cmd_set6), cmd_set6},
    {DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(cmd_set7), cmd_set7},
};

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc himax_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static int mipi_himax_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    static int first_time_on_avoid_lcd_flush = 1;
    printk("##MIPI::%s:  , first_time=%d#######\n", __func__,first_time_on_avoid_lcd_flush);
    if(first_time_on_avoid_lcd_flush)
    {
    	first_time_on_avoid_lcd_flush = 0;
    	return 0;
    }
	pr_info("%s enter\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	msleep(20);
	mipi_dsi_cmds_tx(&himax_tx_buf, himax_cmd_on_cmds_video_mode,
			ARRAY_SIZE(himax_cmd_on_cmds_video_mode));
	pr_info("%s done\n", __func__);

	return 0;
}

static int mipi_himax_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_info("%s\n", __func__);

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&himax_tx_buf, himax_display_off_cmds,
			ARRAY_SIZE(himax_display_off_cmds));

	return 0;
}

static void mipi_himax_set_backlight(struct msm_fb_data_type *mfd)
{
	int32 level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;

	if (mipi_himax_pdata && mipi_himax_pdata->backlight_level) {
		level = mipi_himax_pdata->backlight_level(mfd->bl_level, max, min);

		if (level < 0) {
			pr_info("%s: backlight level control failed\n", __func__);
		}
	} else {
		pr_info("%s: missing baclight control function\n", __func__);
	}

	return;
}

static int __devinit mipi_himax_lcd_probe(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_himax_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_himax_lcd_probe,
	.driver = {
		.name   = "mipi_himax",
	},
};

static struct msm_fb_panel_data himax_panel_data = {
	.on		= mipi_himax_lcd_on,
	.off		= mipi_himax_lcd_off,
	.set_backlight	= mipi_himax_set_backlight,
};

static int ch_used[3];

int mipi_himax_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_himax", (panel << 8)|channel);

	if (!pdev)
		return -ENOMEM;

	himax_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &himax_panel_data,
				sizeof(himax_panel_data));
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

static int __init mipi_himax_lcd_init(void)
{
	pr_info("###%s#####\n",__func__);
	mipi_dsi_buf_alloc(&himax_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&himax_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

module_init(mipi_himax_lcd_init);
