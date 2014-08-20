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
#include <mach/pmic.h>
#include <mach/rpc_pmapp.h>

/* TPS61161 timing params */
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

struct  lm3528_work {
  struct work_struct work;
};
static struct  lm3528_work * lm3528_sensorw;
static struct i2c_client    * lm3528_client;
spinlock_t tps61161_spin_lock;

static int prev_bl = 0;
static int bl_ctl_num = 0;
static int backlight_lm = 0;

static struct msm_panel_common_pdata *tps61161_backlight_pdata;
char vbrightless[32] = {
	0x80,0xB2,0xC0,0xD1,
	0xD7,0xDB,0xDF,0xE2,
	0xE4,0xE6,0xE9,0xEB,
	0xED,0xEE,0xEF,0xF1,
	0xF2,0xF3,0xF4,0xF5,
	0xF6,0xF7,0xF8,0xF9,
	0xFA,0xFB,0xFC,0xFC,
	0xFD,0xFE,0xFE,0xFF
};


//######################################################
static const struct i2c_device_id lm3528_i2c_id[] = {
  {"lm3528", 0},{}
};


static int lm3528_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("%s\n",__func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		printk("i2c_check_functionality failed\n");
		return -ENOMEM;
	}

	lm3528_sensorw = kzalloc(sizeof(struct lm3528_work), GFP_KERNEL);
	if (!lm3528_sensorw)
	{
		printk("kzalloc failed\n");
		return -ENOMEM;
	}
	i2c_set_clientdata(client, lm3528_sensorw);
	lm3528_client = client;
	lm3528_client->addr = client->addr;
	printk("%s: lm3528_client->addr = 0x%02x\n",__func__,lm3528_client->addr);
	return 0;
}

static int lm3528_i2c_remove(struct i2c_client *client)
{
	if(NULL != lm3528_sensorw)
	{
	kfree(lm3528_sensorw);
	lm3528_sensorw = NULL;
	}
	lm3528_client = NULL;
	return 0;
}

static struct i2c_driver lm3528_i2c_driver = {
    .id_table = lm3528_i2c_id,
    .probe  = lm3528_i2c_probe,
    .remove = lm3528_i2c_remove,
    .driver = {
        .name = "lm3528",
    },
};

#if 0
static int lm3528_read(struct i2c_client *client, int reg, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	return 0;
}

static int lm3528_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,
		 .len = readlen,
		 .buf = readbuf,
		 },
	};
	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0){
		dev_err(&client->dev, "%s: i2c read error,lm3528.\n",
			__func__);
	}
	return ret;
}

static int lm3528_write(struct i2c_client *client, u8 reg, u8 val)
{
	return i2c_smbus_write_byte_data(client, reg, val);
}
#endif

static int lm3528_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret < 0){
		dev_err(&client->dev, "%s: i2c write error, lm3528.\n", __func__);
	}  
	return ret;
}
//======================================================

static int lm3528_set_bl(int level, int max, int min)
{
	//unsigned long flags;
	unsigned char txbuf[2];
	if(backlight_lm < 0)	 
	    return 0;
	if(prev_bl == level)
	    return 0;
	
	memset(txbuf, 0, sizeof(txbuf));

	printk("###%s: set backlight to %d, prev=%d, min = %d, max = %d#########\n", __func__, level, prev_bl, min, max);

	//spin_lock_irqsave(&tps61161_spin_lock, flags); //disable local irq and preemption

	if(lm3528_client == NULL)
	{
		//spin_unlock_irqrestore(&tps61161_spin_lock, flags);
		return 0;
	}

	gpio_set_value(96, 1);
	txbuf[0] = 0xa0;
	txbuf[1] = vbrightless[level];
	backlight_lm = lm3528_i2c_write(lm3528_client, txbuf, sizeof(txbuf));
       if(backlight_lm < 0)	   	
	     return 0;
	txbuf[0] = 0xb0;
	txbuf[1] = vbrightless[level];
	lm3528_i2c_write(lm3528_client, txbuf, sizeof(txbuf));

	if(prev_bl == 0 && level!=0)
	{
		printk("###%s #### turn on backlight, gpio96 = %d##########\n",__func__, gpio_get_value(96));
		txbuf[0] = 0x10;
		txbuf[1] = 0x03;
		lm3528_i2c_write(lm3528_client, txbuf, sizeof(txbuf));
	}else if(prev_bl != 0 && level==0)
	{
		printk("###%s #### turn off backlight##########\n",__func__);
		txbuf[0] = 0x10;
		txbuf[1] = 0x00;
		lm3528_i2c_write(lm3528_client, txbuf, sizeof(txbuf));
		gpio_set_value(96, 0);
		printk("###%s #### turn off backlight,  gpio96 is %d##########\n",__func__, gpio_get_value(96));
	}
		
	prev_bl = level;

	//spin_unlock_irqrestore(&tps61161_spin_lock, flags);

	return 0;
}
static int KTD2500_set_bl(int level, int max, int min)
{
	int ret;
       if(level > 0)
           lm3528_set_bl(15,31,0);
       else if(level == 0)
           lm3528_set_bl( 0,31,0);
	   
	ret = pmapp_disp_backlight_set_brightness(level);

	if (ret)
		pr_err("%s: can't set lcd backlight!\n", __func__);

	return ret;
}

void lm3528_set_keyboardbl(int key_blan)
{
    printk("%s: keyboard backlight key_blan = %d \n",__func__,key_blan);

#ifdef CONFIG_HW_ARUBA
    if(key_blan == 1)
    {
         pmic_secure_mpp_config_i_sink(6,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_ENA);
    }
    else if(key_blan == 0)
    {
         pmic_secure_mpp_config_i_sink(6,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_DIS);
    }
#endif

#ifdef CONFIG_HW_SICILY
    if(key_blan == 1)
    {
        gpio_set_value(6, 1);
        gpio_set_value(98, 1);
    }
    else if(key_blan == 0)
	{
	    gpio_set_value(6, 0);
	    gpio_set_value(98, 0);
    }
#endif

#if defined(CONFIG_HW_ARMANI)||defined(CONFIG_HW_AUDI)
printk("susan1++++++++++++++++++++++++++\n");
/*if(key_blan == 1)
{
	 pmic_secure_mpp_config_i_sink(7,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_ENA);
}
else*/
if(key_blan == 0)
{
	 pmic_secure_mpp_config_i_sink(7,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_DIS);
}
#endif
#ifdef CONFIG_HW_ANDORRA
printk("CONFIG_HW_ANDORRA++++++++++++++++++++++++++\n");

if(key_blan == 1)
{
	pmic_gpio_set_value(9,1);
}
else if(key_blan == 0)
{
	pmic_gpio_set_value(9,0);
}
#endif
}
static int __devinit tps61161_backlight_probe(struct platform_device *pdev)
{
	int rc;
	printk("%s\n", __func__);

	rc = i2c_add_driver(&lm3528_i2c_driver);
	if ((rc < 0 ) || (lm3528_client == NULL))
	{
		printk("i2c_add_driver failed\n");
		if(NULL != lm3528_sensorw)
		{
			kfree(lm3528_sensorw);
			lm3528_sensorw = NULL;
		}
		lm3528_client = NULL;
		return -ENOTSUPP;
	}

	rc = gpio_request(96, "lm3528");
	if (0 == rc) {
		rc = gpio_tlmm_config(GPIO_CFG(96, 0,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc < 0) {
			gpio_free(96);
			printk("@@@@@@@@@@@@@@@@lm3528 gpio_tlmm_config(96) fail@@@@@@@@@@@");
			return rc;
		}
	}else{
		printk("@@@@@@@@@@@@@@@@@lm3528 gpio_request(96) fail@@@@@@@@@@@@@");
		return rc;
	}
	gpio_direction_output(96, 1);
	gpio_set_value(96, 1);
	printk("@@@@@@@@@@@@@@@@@lm3528 gpio(96) is %d@@@@@@@@@@@@@", gpio_get_value(96));

#ifdef CONFIG_HW_SICILY
	gpio_free(6);
    rc = gpio_request(6, "6_key_blen");
    if (0 == rc) {
	    rc = gpio_tlmm_config(GPIO_CFG(6, 0,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	    if (rc < 0) {
		    gpio_free(6);
			printk("@@@@@@@@@@@@@@@@lm3528 gpio_tlmm_config(6) fail@@@@@@@@@@@\n");
	    }
    }else{
		printk("@@@@@@@@@@@@@@@@@lm3528 gpio_request(6) fail@@@@@@@@@@@@@\n");
		//return rc;
	}
    gpio_direction_output(6, 1);
	gpio_set_value(6, 1);
	printk("@@@@@@@@@@@@@@@@@lm3528 gpio(6) is %d@@@@@@@@@@@@@\n", gpio_get_value(6));

    rc = gpio_request(98, "98_key_blen");
    if (0 == rc) {
	    rc = gpio_tlmm_config(GPIO_CFG(98, 0,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	    if (rc < 0) {
		    gpio_free(98);
	    }
    }
    gpio_direction_output(98, 1);
	gpio_set_value(98, 1);
#endif

#ifdef CONFIG_HW_ARUBA
    pmic_secure_mpp_config_i_sink(6,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_ENA);
#endif
#if defined(CONFIG_HW_ARMANI)||defined(CONFIG_HW_AUDI)
    pmic_secure_mpp_config_i_sink(7,PM_MPP__I_SINK__LEVEL_5mA,PM_MPP__I_SINK__SWITCH_DIS);
#endif

	if (pdev->id == 0) {
		tps61161_backlight_pdata = (struct msm_panel_common_pdata *)pdev->dev.platform_data;
		if (tps61161_backlight_pdata) {
                    tps61161_backlight_pdata->backlight_level = KTD2500_set_bl;
			bl_ctl_num = tps61161_backlight_pdata->gpio;
			printk("%s: bl_ctl_num = %d\n", __func__, bl_ctl_num);
			//lm3528_set_bl(6,31,0);
		}
		return 0;
	}

    return -1;
}

static struct platform_driver this_driver = {
    .probe  = tps61161_backlight_probe,
    .driver = {
        .name   = "tps6116_backlight",
    },
};

static int __init tps61161_backlight_init(void)
{
    printk("%s\n", __func__);

	spin_lock_init(&tps61161_spin_lock);

    return platform_driver_register(&this_driver);
}

module_init(tps61161_backlight_init);

