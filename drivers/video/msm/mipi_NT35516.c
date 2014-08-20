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
#include "mipi_NT35516.h"
#include <mach/rpc_pmapp.h>
#include <mach/socinfo.h>

static struct msm_panel_common_pdata *mipi_nt35516_pdata;
static struct dsi_buf nt35516_tx_buf;
static struct dsi_buf nt35516_rx_buf;

#define NT35516_SLEEP_OFF_DELAY 150
#define NT35516_DISPLAY_ON_DELAY 150

/* common setting */
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char write_ram[2] = {0x2c, 0x00}; /* write ram */

static struct dsi_cmd_desc nt35516_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(enter_sleep), enter_sleep}
};

//----------------------------------------------------------------
//CMD 3
//----------------------------------------------------------------
static char CMD_FF[5] = {0xFF,0xAA,0x55,0x25,0x01};	 	
static char CMD_F3[8] = {0xF3,0x02,0x03,0x07,0x44,0x88,0xD1,0x0D};
static char CMD_F4[6] = {0xF4,0x00,0x48,0x00,0x00,0x00};
                
//Page 0              
static char CMD_F0[6] = {0xF0,0x55,0xAA,0x52,0x08,0x00};
                
static char CMD_BC[2] = {0xBC,0x04};    //Inversion set 04/04dot; 00/line  
static char CMD_CC[2] = {0xCC,0x01};
static char CMD_B0[6] = {0xB0,0x00,0x0C,0x40,0x3C,0x3C};            
static char CMD_B7[3] = {0xB7,0x72,0x72};
static char CMD_BA[2] = {0xBA,0x05};

//Page 1                
static char CMD_F0_ex[6] = {0xF0,0x55,0xAA,0x52,0x08,0x01};
                                                              
static char CMD_B0_ex[2] = {0xB0,0x0A};                  //AVDD:5.5V 
static char CMD_B6[2] = {0xB6,0x54};                  //AVDD:3.0
static char CMD_B1[2] = {0xB1,0x0A};                  //AVEE:-5.5V  
static char CMD_B7_ex[2] = {0xB7,0x24};                  //AVEE:-2.0
static char CMD_B2[2] = {0xB2,0x03};                  //VCL:-4.0V 
static char CMD_B8[2] = {0xB8,0x30};                  //VCL:-2.0
static char CMD_B3[2] = {0xB3,0x0D};                  //VGH:13.5V 
static char CMD_B9[2] = {0xB9,0x24};                  //VGH:AVDD-AVEE+VDDB
static char CMD_B4[2] = {0xB4,0x06};                  //VGLX:0A/-12V;06/-10V;03/-8.5V 
static char CMD_BA_ex[2] = {0xBA,0x24};                  //VGLX:AVEE+VCL-AVDD0     
static char CMD_B5[2] = {0xB5,0x07};
static char CMD_BB[2] = {0xBB,0x44};
static char CMD_BC_ex[4] = {0xBC,0x00,0xA0,0x01};        //VGMP:5.0V,VGSP:0.3V                
static char CMD_BD[4] = {0xBD,0x00,0xA0,0x01};        //VGMN:-5.0V,VGSN:-0.3V    
static char CMD_BE[2] = {0xBE,0x48};                 ;
//static char cabc_set1[2] = {0x51,0xff};
static char cabc_set2[2] = {0x53,0x2c};
                                                                                                                                                                                   
//Gamma                      1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 
static char CMD_D1[17] = {0xD1,0x00,0x00,0x00,0x10,0x00,0x30,0x00,0x5F,0x00,0x7D,0x00,0xAA,0x00,0xDF,0x01,0x26};       
static char CMD_D2[17] = {0xD2,0x01,0x53,0x01,0x91,0x01,0xC2,0x02,0x18,0x02,0x58,0x02,0x5C,0x02,0x9E,0x02,0xDC};	
static char CMD_D3[17] = {0xD3,0x02,0xFF,0x03,0x2D,0x03,0x4D,0x03,0x77,0x03,0x93,0x03,0xB7,0x03,0xC7,0x03,0xE5};               
static char CMD_D4[5 ] = {0xD4,0x03,0xF5,0x03,0xFF}; 
static char CMD_D5[17] = {0xD5,0x00,0x00,0x00,0x10,0x00,0x30,0x00,0x5F,0x00,0x7D,0x00,0xAA,0x00,0xDF,0x01,0x26};       
static char CMD_D6[17] = {0xD6,0x01,0x53,0x01,0x91,0x01,0xC2,0x02,0x18,0x02,0x58,0x02,0x5C,0x02,0x9E,0x02,0xDC};	
static char CMD_D7[17] = {0xD7,0x02,0xFF,0x03,0x2D,0x03,0x4D,0x03,0x77,0x03,0x93,0x03,0xB7,0x03,0xC7,0x03,0xE5};               
static char CMD_D8[5]  = {0xD8,0x03,0xF5,0x03,0xFF}; 
static char CMD_D9[17] = {0xD9,0x00,0x00,0x00,0x10,0x00,0x30,0x00,0x5F,0x00,0x7D,0x00,0xAA,0x00,0xDF,0x01,0x26};       
static char CMD_DD[17] = {0xDD,0x01,0x53,0x01,0x91,0x01,0xC2,0x02,0x18,0x02,0x58,0x02,0x5C,0x02,0x9E,0x02,0xDC};	
static char CMD_DE[17] = {0xDE,0x02,0xFF,0x03,0x2D,0x03,0x4D,0x03,0x77,0x03,0x93,0x03,0xB7,0x03,0xC7,0x03,0xE5};               
static char CMD_DF[5]  = {0xDF,0x03,0xF5,0x03,0xFF};
static char CMD_E0[17] = {0xE0,0x00,0x00,0x00,0x10,0x00,0x30,0x00,0x5F,0x00,0x7D,0x00,0xAA,0x00,0xDF,0x01,0x26};       
static char CMD_E1[17] = {0xE1,0x01,0x53,0x01,0x91,0x01,0xC2,0x02,0x18,0x02,0x58,0x02,0x5C,0x02,0x9E,0x02,0xDC};	
static char CMD_E2[17] = {0xE2,0x02,0xFF,0x03,0x2D,0x03,0x4D,0x03,0x77,0x03,0x93,0x03,0xB7,0x03,0xC7,0x03,0xE5};               
static char CMD_E3[5]  = {0xE3,0x03,0xF5,0x03,0xFF};
static char CMD_E4[17] = {0xE4,0x00,0x00,0x00,0x10,0x00,0x30,0x00,0x5F,0x00,0x7D,0x00,0xAA,0x00,0xDF,0x01,0x26};       
static char CMD_E5[17] = {0xE5,0x01,0x53,0x01,0x91,0x01,0xC2,0x02,0x18,0x02,0x58,0x02,0x5C,0x02,0x9E,0x02,0xDC};	
static char CMD_E6[17] = {0xE6,0x02,0xFF,0x03,0x2D,0x03,0x4D,0x03,0x77,0x03,0x93,0x03,0xB7,0x03,0xC7,0x03,0xE5};               
static char CMD_E7[5]  = {0xE7,0x03,0xF5,0x03,0xFF};
static char CMD_E8[17] = {0xE8,0x00,0x00,0x00,0x10,0x00,0x30,0x00,0x5F,0x00,0x7D,0x00,0xAA,0x00,0xDF,0x01,0x26};       
static char CMD_E9[17] = {0xE9,0x01,0x53,0x01,0x91,0x01,0xC2,0x02,0x18,0x02,0x58,0x02,0x5C,0x02,0x9E,0x02,0xDC};	
static char CMD_EA[17] = {0xEA,0x02,0xFF,0x03,0x2D,0x03,0x4D,0x03,0x77,0x03,0x93,0x03,0xB7,0x03,0xC7,0x03,0xE5};               
static char CMD_EB[5]  = {0xEB,0x03,0xF5,0x03,0xFF};

static struct dsi_cmd_desc nt35516_cmd_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_FF), CMD_FF},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_F3), CMD_F3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_F4), CMD_F4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_F0), CMD_F0},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_BC), CMD_BC},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_CC), CMD_CC},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_B0), CMD_B0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_B7), CMD_B7},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_BA), CMD_BA},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_F0_ex), CMD_F0_ex},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B0_ex), CMD_B0_ex},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B6), CMD_B6},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B1), CMD_B1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B7_ex), CMD_B7_ex},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B2), CMD_B2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B8), CMD_B8},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B3), CMD_B3},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B9), CMD_B9},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B4), CMD_B4},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_BA_ex), CMD_BA_ex},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_B5), CMD_B5},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_BB), CMD_BB},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_BC_ex), CMD_BC_ex},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_BD), CMD_BD},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(CMD_BE), CMD_BE},


	//{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cabc_set1), cabc_set1},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(cabc_set2), cabc_set2},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D1), CMD_D1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D2), CMD_D2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D3), CMD_D3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D4), CMD_D4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D5), CMD_D5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D6), CMD_D6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D7), CMD_D7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D8), CMD_D8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_D9), CMD_D9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_DD), CMD_DD},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_DE), CMD_DE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_DF), CMD_DF},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E0), CMD_E0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E1), CMD_E1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E2), CMD_E2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E3), CMD_E3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E4), CMD_E4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E5), CMD_E5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E6), CMD_E6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E7), CMD_E7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E8), CMD_E8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_E9), CMD_E9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_EA), CMD_EA},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CMD_EB), CMD_EB},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120,	sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(display_on), display_on},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0,	sizeof(write_ram), write_ram},
};


static struct dsi_cmd_desc nt35516_video_display_on_cmds[] = {

};

static int mipi_nt35516_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	static int first_time_on_avoid_lcd_flush =0;

	pr_err("####%s E########%d  \n", __func__, first_time_on_avoid_lcd_flush);
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

   if (!mfd->cont_splash_done) {
        mfd->cont_splash_done = 1;
        return 0;
    }
	
    if(first_time_on_avoid_lcd_flush)
    {
    	first_time_on_avoid_lcd_flush = 0;
    	return 0;
    }

	/* EVB uses different LCM with SKU5, so the display needs not reverse */
	//if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()) {
	//	cmd19[2] = 0x00;
	//	video19[2] = 0x00;
	//}

	if (mfd->panel_info.mipi.mode == DSI_VIDEO_MODE) {
        pr_debug("%s video mode\n", __func__);
		mipi_dsi_cmds_tx(&nt35516_tx_buf,
			nt35516_video_display_on_cmds,
			ARRAY_SIZE(nt35516_video_display_on_cmds));
	} else if (mfd->panel_info.mipi.mode == DSI_CMD_MODE) {
	    pr_debug("%s cmd mode\n", __func__);
		mipi_dsi_cmds_tx(&nt35516_tx_buf,
			nt35516_cmd_display_on_cmds,
			ARRAY_SIZE(nt35516_cmd_display_on_cmds));
	}

    pr_err("%s X\n", __func__);
	return 0;
}

static int mipi_nt35516_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&nt35516_tx_buf, nt35516_display_off_cmds,
			ARRAY_SIZE(nt35516_display_off_cmds));

	return 0;
}

static int __devinit mipi_nt35516_lcd_probe(struct platform_device *pdev)
{
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_nt35516_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35516_lcd_probe,
	.driver = {
		.name   = "mipi_NT35516",
	},
};
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
	mipi_dsi_cmds_tx(&nt35516_tx_buf, novatek_cmd_backlight_cmds2,
			ARRAY_SIZE(novatek_cmd_backlight_cmds2));
	mutex_unlock(&mfd->dma->ov_mutex);
	return;

}
static void mipi_nt35516_set_backlight(struct msm_fb_data_type *mfd)
{

	int32 level;
	int max = mfd->panel_info.bl_max;
	int min = mfd->panel_info.bl_min;

	if (mipi_nt35516_pdata && mipi_nt35516_pdata->backlight_level) {
		level = mipi_nt35516_pdata->backlight_level(mfd->bl_level, max, min);
		set_pmic_backlight(mfd);
		if (level < 0) {
			printk("%s: backlight level control failed\n", __func__);
		}
	} else {
		printk("%s: missing baclight control function\n", __func__);
	}

	return;
}

static struct msm_fb_panel_data nt35516_panel_data = {
	.on	= mipi_nt35516_lcd_on,
	.off = mipi_nt35516_lcd_off,
	.set_backlight = mipi_nt35516_set_backlight,
};

static int ch_used[3];

static int mipi_nt35516_lcd_init(void)
{
	mipi_dsi_buf_alloc(&nt35516_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&nt35516_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_nt35516_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_nt35516_lcd_init();
	if (ret) {
		pr_err("mipi_nt35516_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_NT35516", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	nt35516_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &nt35516_panel_data,
				sizeof(nt35516_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
