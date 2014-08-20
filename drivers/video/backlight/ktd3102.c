/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/socinfo.h>
#include <mach/rpc_pmapp.h>
#include <mach/pmic.h>

/* ktd3102 timing params */
#define DELAY_TSTART udelay(4)
#define DELAY_TEOS udelay(4)
#define DELAY_THLB udelay(33)
#define DELAY_TLLB udelay(67)
#define DELAY_THHB udelay(67)
#define DELAY_TLHB udelay(33)
#define DELAY_TESDET udelay(286)
#define DELAY_TESDELAY udelay(190)
#define DELAY_TESWIN mdelay(1)
#define DELAY_TOFF mdelay(4)
#define BL_CTRL	0

//spinlock_t ktd3102_spin_lock;

static int bl_ctl_num = 0;
static int pulse_num = 6;
static int volatile flag = 0;

static struct msm_panel_common_pdata *ktd3102_backlight_pdata;
#if 0
static void ktd3102_send_bit(int bit_data)
{
	if (bit_data == 0) {
		pmic_gpio_set_value(bl_ctl_num, 0);;
		DELAY_TLLB;
		pmic_gpio_set_value(bl_ctl_num, 1);;
		DELAY_THLB;
	} else {
		pmic_gpio_set_value(bl_ctl_num, 0);;
		DELAY_TLHB;
		pmic_gpio_set_value(bl_ctl_num, 1);;
		DELAY_THHB;
	}
}

static void ktd3102_send_byte(unsigned char byte_data)
{
	int n;

	for (n = 0; n < 8; n++) {
		ktd3102_send_bit((byte_data & 0x80) ? 1 : 0);
		byte_data = byte_data << 1;
	}
}
#endif

static int ktd3102_set_bl(int level, int max, int min)
{
	int i;

	printk("****************** %s enter flag = %d ******************\n", __func__, flag);
	if (level == 0) {
		flag = 0;
		pmic_gpio_set_value(bl_ctl_num, 0);
		printk("****************** %s backlight (off) ******************\n", __func__);
		printk("****************** %s done flag = %d ******************\n", __func__, flag);
		return 0;
	} else {
		if (!flag) {
			flag = 1;
			printk("****************** %s backlight (on); pulse_num = %d ******************\n", __func__, pulse_num);
			for (i = 0; i < pulse_num; i++) {
				pmic_gpio_set_value(bl_ctl_num, 1);
				udelay(250);
				pmic_gpio_set_value(bl_ctl_num, 0);
				udelay(100);
				pmic_gpio_set_value(bl_ctl_num, 1);
			}
		}
	}

	printk("****************** %s done flag = %d******************\n", __func__, flag);

	return 0;
}
#ifdef KTD3102_DEBUG
static int ktd3102_set_pulse(const char *val, struct kernel_param *kp)
{
	int i, ret, level;

	ret = param_set_int(val, kp);

	if(ret < 0)
	{   
		printk(KERN_ERR"Errored pulse value");
		return -EINVAL;
	}   
	level = *((int*)kp->arg); 

	printk("%s level = %d\n", __func__, level);
	if (!level) {
		flag = 0;
		pmic_gpio_set_value(bl_ctl_num, 0);
	} else {
		if (!flag)
			flag = 1;
		if (level > pulse_num) {
			for (i = 0; i < level - pulse_num; i++) {
				pmic_gpio_set_value(bl_ctl_num, 1);
				udelay(250);
				pmic_gpio_set_value(bl_ctl_num, 0);
				udelay(100);
				pmic_gpio_set_value(bl_ctl_num, 1);
			}

		} else {
			for (i = 0; i < 32 + level - pulse_num; i++) {
				pmic_gpio_set_value(bl_ctl_num, 1);
				udelay(250);
				pmic_gpio_set_value(bl_ctl_num, 0);
				udelay(100);
				pmic_gpio_set_value(bl_ctl_num, 1);
			}
		}
	}
	pulse_num = level;
	printk("%s pulse_num = %d\n", __func__, pulse_num);
	return 0;
}
#endif

static int __devinit ktd3102_backlight_probe(struct platform_device *pdev)
{
	printk("[DISP]%s\n", __func__);

	if (pdev->id == 0) {
		ktd3102_backlight_pdata = (struct msm_panel_common_pdata *)pdev->dev.platform_data;

		if (ktd3102_backlight_pdata) {
			ktd3102_backlight_pdata->backlight_level = ktd3102_set_bl;
			//bl_ctl_num = ktd3102_backlight_pdata->gpio;
			bl_ctl_num = BL_CTRL;
			printk("[DISP]%s: bl_ctl_num = %d\n", __func__, bl_ctl_num);

			/* FIXME: if continuous splash is enabled, set prev_bl to
			 * a non-zero value to avoid entering easyscale.
			 */
			//if (ktd3102_backlight_pdata->cont_splash_enabled)
			//   prev_bl = 1;
		}

		pmic_gpio_direction_output(bl_ctl_num);
		pmic_gpio_set_value(bl_ctl_num, 1);
		return 0;
	}

	printk("[DISP]%s: Wrong ID\n", __func__);
	return -1;
}

static struct platform_driver this_driver = {
	.probe  = ktd3102_backlight_probe,
	.driver = {
		.name   = "ktd3102_backlight",
	},
};

static int __init ktd3102_backlight_init(void)
{
	printk("[DISP]%s\n", __func__);

	//spin_lock_init(&ktd3102_spin_lock);

	return platform_driver_register(&this_driver);
}

module_init(ktd3102_backlight_init);
#ifdef KTD3102_DEBUG
module_param_call(set_pulse, ktd3102_set_pulse, param_get_int, &pulse_num, S_IRUSR | S_IWUSR);
#endif
