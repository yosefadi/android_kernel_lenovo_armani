/* include/asm/mach-msm/htc_pwrsink.h
 *
 * Copyright (C) 2008 HTC Corporation.
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2012-2013, The Linux Foundation. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define DEBUG  0

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/module.h>
#include "pmic.h"
#include "timed_output.h"
#include "msm_vibrator.h"
#include <mach/msm_rpcrouter.h>

#ifdef CONFIG_MSM_RPC_VIBRATOR
#define PM_LIBPROG      0x30000061
//change rpc ver number 
#define PM_LIBVERS      0x00060001
#define HTC_PROCEDURE_SET_VIB_ON_OFF	22
#define VIB_OFF_DELAY  50 //ms

#define PMIC_VIBRATOR_LEVEL	(3000)
#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
static int volt_value = 3000;
#endif
#if DEBUG
#define debug_print(x...) do {pr_info(x); } while (0)
#else
#define debug_print(x...) do {} while (0)
#endif

static struct work_struct work_vibrator_on;
static struct work_struct work_vibrator_off;
static struct hrtimer vibe_timer;

static volatile int g_vibrator_status=0;//represent the vibrator's real on off status; 1:on  0:off
#if DEBUG
struct timespec volatile g_ts_start;
struct timespec volatile g_ts_end;
#endif
static int vibrator_on_delay;			
uint get_vibrator_voltage(void)
{
    
        return 3000;

}
static void set_pmic_vibrator(int on)
{
    static struct msm_rpc_endpoint *vib_endpoint;
    struct set_vib_on_off_req
    {
        struct rpc_request_hdr hdr;
        uint32_t data;
    }
    req;

    if (!vib_endpoint)
    {
        vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
        if (IS_ERR(vib_endpoint))
        {
            printk(KERN_ERR "init vib rpc failed!\n");
            vib_endpoint = 0;
            return;
        }
    }

    if (on)
        req.data = cpu_to_be32(volt_value);
    else
        req.data = cpu_to_be32(0);

    msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
                 sizeof(req), 5 * HZ);
	
    if(on)
    {
        g_vibrator_status=1;
#if DEBUG
        g_ts_start = current_kernel_time();
#endif
    }
    else
    {
        if(g_vibrator_status==1)
        {
        	   g_vibrator_status=0;	//chenchongbao.20111218
#if DEBUG
            g_ts_end = current_kernel_time();
            pr_info("vibrator vibrated %ld ms.\n",
                    (g_ts_end.tv_sec-g_ts_start.tv_sec)*1000+g_ts_end.tv_nsec/1000000-g_ts_start.tv_nsec/1000000 );
#endif
        }
    }
}


static void pmic_vibrator_on(struct work_struct *work)
{
    if( g_vibrator_status==0)//if vibrator is on now
    {
        debug_print("Q:pmic_vibrator_on,start");	//chenchongbao.20111218
        set_pmic_vibrator(1);
        debug_print("Q:pmic_vibrator_on,done\n");
    }
    else
    {
        debug_print("Q:pmic_vibrator_on, already on, do nothing.\n");
    }
	vibrator_on_delay = (vibrator_on_delay > 15000 ? 15000 : vibrator_on_delay);	//moved from enable fun & modified chenchongbao.20111218
	hrtimer_start(&vibe_timer,	/* chenchongbao.20111218 moved from enable fun & modified */
                      ktime_set(vibrator_on_delay / 1000, (vibrator_on_delay % 1000 ) * 1000000),
                      HRTIMER_MODE_REL);
}

static void pmic_vibrator_off(struct work_struct *work)
{
    if( g_vibrator_status==1)//if vibrator is on now
    {
        debug_print("Q:pmic_vibrator_off,start");	//chenchongbao.20111218
        set_pmic_vibrator(0);
        debug_print("Q:pmic_vibrator_off,done\n"); 
    }
    else
    {
        debug_print("Q:pmic_vibrator_off, already off, do nothing.\n");
    }
}

static int timed_vibrator_on(struct timed_output_dev *sdev)
{
    if(!schedule_work(&work_vibrator_on))
    {
        pr_info("vibrator schedule on work failed !\n");
        return 0;
    }
    return 1;
}

static int timed_vibrator_off(struct timed_output_dev *sdev)
{
    if(!schedule_work(&work_vibrator_off))
    {
        pr_info("vibrator schedule off work failed !\n");
        return 0;
    }
    return 1;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
    hrtimer_cancel(&vibe_timer);
    cancel_work_sync(&work_vibrator_on);
    cancel_work_sync(&work_vibrator_off);

    //pr_info("vibrator_enable,%d ms,vibrator:%s now.\n",value,g_vibrator_status?"on":"off");		//chenchongbao.20111218
    //pr_debug("PM_DEBUG_MXP:vibrator_enable,%d ms,vibrator:%s now.\n",value,g_vibrator_status?"on":"off");	
    if (value == 0)
    {
        if(!timed_vibrator_off(dev))//if queue failed, delay 10ms try again by timer
        {
        	   pr_info("vibrator_enable, queue failed!\n");	//chenchongbao.20111218
            value=10;
            hrtimer_start(&vibe_timer,
                          ktime_set(value / 1000, (value % 1000) * 1000000),
                          HRTIMER_MODE_REL);
            value=0;
        }
    }
    else
    {
        //value = (value > 15000 ? 15000 : value);	//we put it in work fun. 20111218
	vibrator_on_delay = value;	//chenchongbao.20111218
        timed_vibrator_on(dev);

        //hrtimer_start(&vibe_timer,		//chenchongbao.20111218 : we put it to workqueue fun
        //              ktime_set(value / 1000, (value % 1000 + VIB_OFF_DELAY) * 1000000),
        //              HRTIMER_MODE_REL);
    }
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
    if (hrtimer_active(&vibe_timer))
    {
        ktime_t r = hrtimer_get_remaining(&vibe_timer);
	struct timeval t = ktime_to_timeval(r);		
	return t.tv_sec * 1000 + t.tv_usec / 1000;
    }
    else
        return 0;
}

#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
static ssize_t intensity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", volt_value);
}

static ssize_t intensity_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	if (sscanf(buf, "%d", &value) != 1)
		return -EINVAL;

	volt_value = value;

	return size;
}

static DEVICE_ATTR(intensity, 0666, intensity_show, intensity_store);
#endif

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
    int value=0;
    debug_print("timer:vibrator timeout!\n");
    if(!timed_vibrator_off(NULL))
    {
        pr_info("timer:vibrator timeout queue failed!\n");	
        value=500;
        hrtimer_start(&vibe_timer,
                      ktime_set(value / 1000, (value % 1000) * 1000000),
                      HRTIMER_MODE_REL);
        value=0;
    }
    return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator =
    {
        .name = "vibrator",
                .get_time = vibrator_get_time,
                            .enable = vibrator_enable,
                                  };

void __init msm_init_pmic_vibrator(void)
{
#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
	int err;
#endif

    pr_info("msm_init_pmic_vibrator\n");
    INIT_WORK(&work_vibrator_on, pmic_vibrator_on);
    INIT_WORK(&work_vibrator_off, pmic_vibrator_off);

    hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    vibe_timer.function = vibrator_timer_func;

    timed_output_dev_register(&pmic_vibrator);
#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
	volt_value = get_vibrator_voltage();

	err = device_create_file(pmic_vibrator.dev, &dev_attr_intensity);
	if (err < 0)
	{
		pr_err("%s: failed to create sysfs\n", __func__);
	}
#endif
}
MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");


#else//disable qcom default code.
#define PM_LIBPROG      0x30000061
#if (CONFIG_MSM_AMSS_VERSION == 6220) || (CONFIG_MSM_AMSS_VERSION == 6225)
#define PM_LIBVERS      0xfb837d0b
#else
#define PM_LIBVERS      0x10001
#endif
#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
static int volt_value = 3000;
#endif
#define HTC_PROCEDURE_SET_VIB_ON_OFF	21
#define PMIC_VIBRATOR_LEVEL	(3000)

static struct work_struct vibrator_work;
static struct hrtimer vibrator_timer;
static DEFINE_MUTEX(vibrator_mtx);

#ifdef CONFIG_PM8XXX_RPC_VIBRATOR
static void set_pmic_vibrator(int on)
{
	int rc;

	rc = pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__MANUAL);
	if (rc) {
		pr_err("%s: Vibrator set mode failed", __func__);
		return;
	}

	if (on)
		rc = pmic_vib_mot_set_volt(get_vibrator_voltage());
	else
		rc = pmic_vib_mot_set_volt(0);

	if (rc)
		pr_err("%s: Vibrator set voltage level failed", __func__);
}
#else
static void set_pmic_vibrator(int on)
{
	static struct msm_rpc_endpoint *vib_endpoint;
	struct set_vib_on_off_req {
		struct rpc_request_hdr hdr;
		uint32_t data;
	} req;
	if(mutex_lock_interruptible(&vibe_mtx))
               return;

	if (!vib_endpoint) {
		vib_endpoint = msm_rpc_connect(PM_LIBPROG, PM_LIBVERS, 0);
		if (IS_ERR(vib_endpoint)) {
			printk(KERN_ERR "init vib rpc failed!\n");
			vib_endpoint = 0;
			mutex_unlock(&vibe_mtx);
			return;
		}
	}


	if (on)
		req.data = cpu_to_be32(volt_value);
	else
		req.data = cpu_to_be32(0);

	msm_rpc_call(vib_endpoint, HTC_PROCEDURE_SET_VIB_ON_OFF, &req,
		sizeof(req), 5 * HZ);

	mutex_unlock(&vibe_mtx);
}
#endif

static void update_vibrator(struct work_struct *work)
{
	pr_debug("%s\n", __func__);
	set_pmic_vibrator(0);
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	mutex_lock(&vibrator_mtx);
	hrtimer_cancel(&vibrator_timer);
	cancel_work_sync(&vibrator_work);

	if (value == 0) {
		set_pmic_vibrator(0);
	} else {
		value = (value > 15000 ? 15000 : value);
		set_pmic_vibrator(value);
		hrtimer_start(&vibrator_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
	mutex_unlock(&vibrator_mtx);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibrator_timer)) {
		ktime_t r = hrtimer_get_remaining(&vibrator_timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return 0;
}

#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
static ssize_t intensity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", volt_value);
}

static ssize_t intensity_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	if (sscanf(buf, "%d", &value) != 1)
		return -EINVAL;

	volt_value = value;

	return size;
}

static DEVICE_ATTR(intensity, 0666, intensity_show, intensity_store);
#endif

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	pr_debug("%s\n", __func__);
	schedule_work(&vibrator_work);
	return HRTIMER_NORESTART;
}

static struct timed_output_dev pmic_vibrator = {
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

void __init msm_init_pmic_vibrator(void)
{
#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
	int err;
#endif
	INIT_WORK(&vibrator_work, update_vibrator);
	hrtimer_init(&vibrator_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibrator_timer.function = vibrator_timer_func;

	timed_output_dev_register(&pmic_vibrator);

#ifdef CONFIG_LENOVO_VIBRATOR_INTENSITY_SYSFS
	volt_value = get_vibrator_voltage();

	err = device_create_file(pmic_vibrator.dev, &dev_attr_intensity);
	if (err < 0)
	{
		pr_err("%s: failed to create sysfs\n", __func__);
	}
#endif
}
MODULE_DESCRIPTION("timed output pmic vibrator device");
MODULE_LICENSE("GPL");
#endif
