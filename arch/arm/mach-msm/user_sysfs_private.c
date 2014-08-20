/*
* Copyright (C) 2012 lenovo, Inc.
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

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ctype.h>
/* yangjq, 2011-12-16, Add for vreg, START */
#include <linux/platform_device.h>
#include <mach/vreg.h>
/* yangjq, 2011-12-16, Add for vreg, END */
#include <linux/err.h>
#include <asm/gpio.h>
#include <mach/mpp.h>
#include <mach/pmic.h>
#include <linux/regulator/consumer.h>

//yangjq, Add for acpuclk_get_rate()
#include "acpuclock.h"
#include "mach/proc_comm.h"

#define SCLK_HZ (32768)

#define private_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#if 1
#define TLMM_NUM_GPIO 133

extern int tlmm_dump_info(char* buf);
static ssize_t tlmm_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tlmm_dump_info(buf);
	return p - buf;
}

static ssize_t tlmm_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	char pull_c, dir_c;
	int gpio, func, pull, dir, drvstr, output_val;
	unsigned cfg;
	int res;
	
	res = sscanf(buf, "%d %d %c %c %d %d", &gpio, &func, &pull_c, &dir_c, &drvstr, &output_val);
	printk("res=%d.  %d %d %c %c %d %d\n", res, gpio, func, pull_c, dir_c, drvstr, output_val);
	
	if((res != 5) && (res != 6)) 
		goto tlmm_store_wrong_para;

	if(gpio >= TLMM_NUM_GPIO)
		goto tlmm_store_wrong_para;
		
	if('N' == pull_c)
		pull = 0;
	else if('D' == pull_c)
		pull = 1;
	else if('K' == pull_c)
		pull = 2;
	else if('U' == pull_c)
		pull = 3;
	else 
		goto tlmm_store_wrong_para;

	if('I' == dir_c)
		dir = 0;
	else if('O' == dir_c)
		dir = 1;
	else 
		goto tlmm_store_wrong_para;
	
	drvstr = drvstr/2 - 1; // 2mA -> 0, 4mA -> 1, 6mA -> 2, ...	
	if(drvstr > 7)
		goto tlmm_store_wrong_para;
	
	if(output_val > 1)
		goto tlmm_store_wrong_para;
		
	printk("final set: %d %d %d %d %d %d\n", gpio, func, pull, dir, drvstr, output_val);

	cfg = GPIO_CFG(gpio, func, dir, pull, drvstr);	
	res = gpio_tlmm_config(cfg, GPIO_CFG_ENABLE);
	if(res < 0) {
		printk("Error: Config failed.\n");
		goto tlmm_store_wrong_para;
	}
	
	if((func == 0)  && (dir == 1)) // gpio output
		gpio_set_value(gpio, output_val);
	
	goto tlmm_store_ok;
		
tlmm_store_wrong_para:
	printk("Wrong Input.\n");	
	printk("Format: gpio_num  function  pull  direction  strength [output_value]\n");
	printk("      gpio_num: 0 ~ 132\n");	
	printk("      function: number, where 0 is GPIO\n");	
	printk("      pull: 'N': NO_PULL, 'D':PULL_DOWN, 'K':KEEPER, 'U': PULL_UP\n");	
	printk("      direction: 'I': Input, 'O': Output\n");	
	printk("      strength:  2, 4, 6, 8, 10, 12, 14, 16\n");	
	printk("      output_value:  Optional. 0 or 1. vaild if GPIO output\n");	
	printk(" e.g.  'echo  20 0 D I 2'  ==> set pin 20 as GPIO input \n");	
	printk(" e.g.  'echo  20 0 D O 2 1'  ==> set pin 20 as GPIO output and the output = 1 \n");	

tlmm_store_ok:	
	return n;
}

/* yangjq, 20120106, Set GPIO's sleep config from sysfs. START */
extern int tlmm_sleep_table_dump_info(char* buf);
static ssize_t tlmm_sleep_table_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tlmm_sleep_table_dump_info(buf);
	return p - buf;
}

extern int tlmm_sleep_table_set_cfg(unsigned gpio, unsigned cfg);
static ssize_t tlmm_sleep_table_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	char pull_c, dir_c;
	int gpio, func = 0, pull = 0, dir = 0, drvstr = 0, rmt = 0, output_val = 0;
	int ignore;
	unsigned cfg;
	int res;
	
	res = sscanf(buf, "%d %d %c %c %d %d", &gpio, &func, &pull_c, &dir_c, &drvstr, &output_val);
	printk("res=%d.  %d %d %c %c %d %d\n", res, gpio, func, pull_c, dir_c, drvstr, output_val);
	
	if(1 == res) { // if only gpio, means ingore(disable) the gpio's sleep config 
		ignore = 1;
		printk("final set: to disable gpio %d sleep config\n", gpio);
	}	
	else {
		ignore = 0;
	
		if((res != 5) && (res != 6)) 
			goto tlmm_sleep_table_store_wrong_para;

		if(gpio >= TLMM_NUM_GPIO)
			goto tlmm_sleep_table_store_wrong_para;
			
		if('N' == pull_c)
			pull = 0;
		else if('D' == pull_c)
			pull = 1;
		else if('K' == pull_c)
			pull = 2;
		else if('U' == pull_c)
			pull = 3;
		else 
			goto tlmm_sleep_table_store_wrong_para;

		if('I' == dir_c)
			dir = 0;
		else if('O' == dir_c)
			dir = 1;
		else 
			goto tlmm_sleep_table_store_wrong_para;
		
		drvstr = drvstr/2 - 1; // 2mA -> 0, 4mA -> 1, 6mA -> 2, ...	
		if(drvstr > 7)
			goto tlmm_sleep_table_store_wrong_para;
		
		rmt = 2; // HAL_TLMM_OUTPUT_LOW in AMSS
		if(1 == dir) // output
			if(1 == output_val)
				rmt = 3; // HAL_TLMM_OUTPUT_HIGH in AMSS
				
		printk("final set: %d %d %d %d %d %d(rmt)\n", gpio, func, pull, dir, drvstr, rmt);
	}
		
	// GPIO_CFG_AMSS is same as GPIO_CFG in AMSS side	
	#define GPIO_CFG_AMSS(gpio, func, dir, pull, drvstr, rmt) \
         (((gpio)&0xFF)<<8|((rmt)&0xF)<<4|((func)&0xF)|((dir)&0x1)<<16| \
         ((pull)&0x3)<<17|((drvstr)&0x7)<<19)
		 
	cfg = GPIO_CFG_AMSS(ignore ? 0xff : gpio, func, dir, pull, drvstr, rmt);	
	res = tlmm_sleep_table_set_cfg(gpio, cfg);
	if(res < 0) {
		printk("Error: Config failed.\n");
		goto tlmm_sleep_table_store_wrong_para;
	}
	
	goto tlmm_sleep_table_store_ok;
		
tlmm_sleep_table_store_wrong_para:
	printk("Wrong Input.\n");	
	printk("Format: refer to tlmm's format except  'echo gpio_num > xxx' to disable the gpio's setting\n");	

tlmm_sleep_table_store_ok:	
	return n;
}
/* yangjq, 20120106, Set GPIO's sleep config from sysfs. END */
extern int tlmm_sleep_table_before_dump_info(char* buf);
static ssize_t tlmm_sleep_table_before_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tlmm_sleep_table_before_dump_info(buf);
	return p - buf;
}

extern int tlmm_sleep_table_before_set_cfg(unsigned gpio, unsigned cfg);
static ssize_t tlmm_sleep_table_before_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	char pull_c, dir_c;
	int gpio, func = 0, pull = 0, dir = 0, drvstr = 0, output_val = 0;
	int ignore;
	unsigned cfg;
	int res;
	
	res = sscanf(buf, "%d %d %c %c %d %d", &gpio, &func, &pull_c, &dir_c, &drvstr, &output_val);
	printk("res=%d.  %d %d %c %c %d %d\n", res, gpio, func, pull_c, dir_c, drvstr, output_val);
	
	if(1 == res) { // if only gpio, means ingore(disable) the gpio's sleep config 
		ignore = 1;
		printk("final set: to disable gpio %d sleep config\n", gpio);
	}	
	else {
		ignore = 0;
	
		if((res != 5) && (res != 6)) 
			goto tlmm_sleep_table_before_store_wrong_para;

		if(gpio >= TLMM_NUM_GPIO)
			goto tlmm_sleep_table_before_store_wrong_para;
			
		if('N' == pull_c)
			pull = 0;
		else if('D' == pull_c)
			pull = 1;
		else if('K' == pull_c)
			pull = 2;
		else if('U' == pull_c)
			pull = 3;
		else 
			goto tlmm_sleep_table_before_store_wrong_para;

		if('I' == dir_c)
			dir = 0;
		else if('O' == dir_c)
			dir = 1;
		else 
			goto tlmm_sleep_table_before_store_wrong_para;
		
		drvstr = drvstr/2 - 1; // 2mA -> 0, 4mA -> 1, 6mA -> 2, ...	
		if(drvstr > 7)
			goto tlmm_sleep_table_before_store_wrong_para;
				
		printk("final set: %d %d %d %d %d\n", gpio, func, pull, dir, drvstr);
	}
		 
	cfg = GPIO_CFG(ignore ? 0xff : gpio, func, dir, pull, drvstr);
	res = tlmm_sleep_table_before_set_cfg(gpio, cfg | (output_val << 30));
	if(res < 0) {
		printk("Error: Config failed.\n");
		goto tlmm_sleep_table_before_store_wrong_para;
	}
	
	goto tlmm_sleep_table_before_store_ok;
		
tlmm_sleep_table_before_store_wrong_para:
	printk("Wrong Input.\n");	
	printk("Format: refer to tlmm's format except  'echo gpio_num > xxx' to disable the gpio's setting\n");	

tlmm_sleep_table_before_store_ok:	
	return n;
}

extern int tlmm_last_sleep_dump_info(char* buf);
static ssize_t tlmm_last_sleep_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tlmm_last_sleep_dump_info(buf);
	return p - buf;
}

static ssize_t tlmm_last_sleep_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}

extern int tlmm_before_sleep_dump_info(char* buf);
static ssize_t tlmm_before_sleep_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tlmm_before_sleep_dump_info(buf);
	return p - buf;
}

static ssize_t tlmm_before_sleep_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}

//command for query gpio owner
extern int tlmm_gpio_owner_dump_info(char* buf);
static ssize_t gpio_owner_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tlmm_gpio_owner_dump_info(buf);
	return p - buf;
}

static ssize_t gpio_owner_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}
//command for query gpio owner

extern int vreg_dump_info(char* buf);
static ssize_t vreg_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += vreg_dump_info(buf);
	return p - buf;
}

//extern void vreg_config(struct vreg *vreg, unsigned on, unsigned mv);
extern void regulator_config(struct regulator *reg, unsigned on, unsigned mv);
static ssize_t vreg_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	unsigned on, mv = 0;
	char on_str[10];
	char vreg_name[20];
	//struct vreg *vreg;
	struct regulator *reg;
	int res;

	res = sscanf(buf, "%s %s %u", vreg_name, on_str, &mv);
	printk("res=%d.  %s %s %u\n", res, vreg_name, on_str, mv);

	if(!strcmp(on_str, "on"))
		on = 1;
	else if(!strcmp(on_str, "off"))
		on = 0;
	else 
		goto vreg_store_wrong_para;

	//vreg = vreg_get(NULL, vreg_name);
	reg = regulator_get(NULL, vreg_name);
	if(((2 != res) && (3 != res))  || (on > 1) || IS_ERR_OR_NULL(reg))
		goto vreg_store_wrong_para;

	regulator_config(reg, on, mv);
	goto vreg_store_store_ok;

vreg_store_wrong_para:
	printk("Wrong Input.\n");
	printk("Format:  'echo msma 0' to off.  'echo msma 1' or 'echo msma 1 1500' to on with 1500 mV \n");

vreg_store_store_ok:
	return n;
}

extern int vreg_last_sleep_dump_info(char* buf);
static ssize_t vreg_last_sleep_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += vreg_last_sleep_dump_info(buf);
	return p - buf;
}

static ssize_t vreg_last_sleep_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}

/* sub commands of PCOM_OEM_PM_GET_LOG */
#define PM_GET_LOG_TCXO_OKTS	1
#define PM_GET_LOG_CLK_OKTS	2
#define PM_GET_LOG_TCXO_OKTS_A	3
#define PM_GET_LOG_CLK_OKTS_A	4

int get_tcxo_okts(int subcommand, unsigned id, unsigned *para1, unsigned* para2)
{
	int res;

	if(PM_GET_LOG_TCXO_OKTS != subcommand && PM_GET_LOG_CLK_OKTS != subcommand  &&
		PM_GET_LOG_TCXO_OKTS_A != subcommand && PM_GET_LOG_CLK_OKTS_A != subcommand)
		return 0;
	
	*para1 = (subcommand << 16) | id;
	res = msm_proc_comm(PCOM_OEM_PM_GET_LOG, para1, para2);
	if(res >= 0)
		return 1;
	else // fail
		return 0;
}

static int tcxo_last_sleep_dump_info(char* buf)
{
	char* p = buf;
	unsigned para1, para2;
	int fetched;

	fetched = 0;
	if (get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 0, &para1, &para2))
		fetched = para1;

	p += sprintf(p, "tcxo_last_sleep:\n");
	if (!fetched) {
		if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 2, &para1, &para2)) {
			p += sprintf(p, "%16s%10d, %6d.%03d(s)\n", "tcxo_try:",
				para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
		}

		if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 3, &para1, &para2)) {
			p += sprintf(p, "%16s%10d, %6d.%03d(s)\n", "tcxo_off:",
				para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
		}

		if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 4, &para1, &para2)) {
			p += sprintf(p, "%16s%10d, %6d.%03d(s)\n", "tcxo_on(halt):",
				para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
		}

		if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 5, &para1, &para2)) {
			p += sprintf(p, "%16s%10d, %6d.%03d(s)\n", "apps pc:",
				para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
		}

		if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 1, &para1, &para2)) {
			p += sprintf(p,  "%16s0x%08x 0x%08x\n", "client not_okts:", para1, para2);
		}

		if(get_tcxo_okts(PM_GET_LOG_CLK_OKTS, 1, &para1, &para2)) {
			p += sprintf(p,  "%16s0x%08x 0x%08x", "clock not_okts:", para1, para2);
		}

		if(get_tcxo_okts(PM_GET_LOG_CLK_OKTS, 2, &para1, &para2)) {
			p += sprintf(p, " 0x%08x 0x%08x\n", para1, para2);
		}

		//p += sprintf(p, "\n");
	}
	return p - buf;
}

static ssize_t tcxo_last_sleep_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tcxo_last_sleep_dump_info(buf);
	return p - buf;
}

static ssize_t tcxo_last_sleep_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}

void print_tcxo_last_sleep(void)
{
	unsigned para1, para2;

	if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 2, &para1, &para2)) {
		printk("%16s%10d, %6d.%03d(s)\n", "tcxo_try:",
			para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
	}

	if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 3, &para1, &para2)) {
		printk("%16s%10d, %6d.%03d(s)\n", "tcxo_off:",
			para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
	}

	if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 4, &para1, &para2)) {
		printk("%16s%10d, %6d.%03d(s)\n", "tcxo_on(halt):",
			para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
	}

	if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 5, &para1, &para2)) {
		printk("%16s%10d, %6d.%03d(s)\n", "apps pc:",
			para1, (para2 / SCLK_HZ), (para2 % SCLK_HZ * 1000 / SCLK_HZ));
	}

	if(get_tcxo_okts(PM_GET_LOG_TCXO_OKTS, 1, &para1, &para2)) {
		printk( "%16s0x%08x 0x%08x\n", "client not_okts:", para1, para2);
	}

	if(get_tcxo_okts(PM_GET_LOG_CLK_OKTS, 1, &para1, &para2)) {
		printk( "%16s0x%08x 0x%08x", "clock not_okts:", para1, para2);
	}

	if(get_tcxo_okts(PM_GET_LOG_CLK_OKTS, 2, &para1, &para2)) {
		printk(" 0x%08x 0x%08x\n", para1, para2);
	}

	//printk("\n");
}
EXPORT_SYMBOL(print_tcxo_last_sleep);

static int tcxo_okts_detail_dump_info(char* buf)
{
	char* p = buf;
	unsigned para1, para2;
	int i;
	unsigned short not_okts[64];
	unsigned not_okts_total;
	int fetched;

	fetched = not_okts_total = 0;
	if (get_tcxo_okts(PM_GET_LOG_TCXO_OKTS_A, 0, &para1, &para2)) {
		fetched = para1;
		not_okts_total = para2;
	}

	p += sprintf(p, "tcxo_okts_detail:\n");
	if (!fetched) {
		for (i = 0; i < 16; i++) {
			if (get_tcxo_okts(PM_GET_LOG_TCXO_OKTS_A, i + 1, &para1, &para2)) {
				not_okts[i * 4 + 0] = para1;
				not_okts[i * 4 + 1] = para1 >> 16;
				not_okts[i * 4 + 2] = para2;
				not_okts[i * 4 + 3] = para2 >> 16;
			}
		}

		for (i = 0; i < 64; i++) {
			if (not_okts[i] > 0)
				p += sprintf(p, "not_okts[%2d]=%d\n", i, not_okts[i]);
		}

		p += sprintf(p, "not_okts_total=%d\n", not_okts_total);
	}
	return p - buf;
}

static ssize_t tcxo_okts_detail_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += tcxo_okts_detail_dump_info(buf);
	return p - buf;
}

static ssize_t tcxo_okts_detail_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}

static int clk_okts_detail_dump_info(char* buf)
{
	char* p = buf;
	unsigned para1, para2;
	int i;
	unsigned short not_okts[128];
	unsigned not_okts_total;
	int fetched;

	fetched = not_okts_total = 0;
	if (get_tcxo_okts(PM_GET_LOG_CLK_OKTS_A, 0, &para1, &para2)) {
		fetched = para1;
		not_okts_total = para2;
	}

	p += sprintf(p, "clk_okts_detail:\n");
	if (!fetched) {
		for (i = 0; i < 32; i++) {
			if (get_tcxo_okts(PM_GET_LOG_CLK_OKTS_A, i + 1, &para1, &para2)) {
				not_okts[i * 4 + 0] = para1;
				not_okts[i * 4 + 1] = para1 >> 16;
				not_okts[i * 4 + 2] = para2;
				not_okts[i * 4 + 3] = para2 >> 16;
			}
		}

		for (i = 0; i < 128; i++) {
			if (not_okts[i] > 0)
				p += sprintf(p, "not_okts[%3d]=%d\n", i, not_okts[i]);
		}

		p += sprintf(p, "not_okts_total=%d\n", not_okts_total);
	}
	return p - buf;
}

static ssize_t clk_okts_detail_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	char* p = buf;
	p += clk_okts_detail_dump_info(buf);
	return p - buf;
}

static ssize_t clk_okts_detail_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);
	return n;
}
#endif

static ssize_t clk_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	extern int clk_dump_info(char* buf, int* count, int enabled);
	int count;
	char *s = buf;

	// show all enabled clocks
	s += sprintf(s, "\nEnabled Clocks:\n");
	s += clk_dump_info(s, &count, 1);
	
	return (s - buf);
}	

static ssize_t clk_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support.\n", __func__);

	return -EPERM;
}

extern int wakelock_dump_info(char* buf);

static ssize_t pm_status_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *s = buf;
	unsigned long rate; // khz
	int i;

	// show CPU clocks
	for (i = 0; i < nr_cpu_ids; i++) {
		rate = acpuclk_get_rate(i); // khz
		s += sprintf(s, "APPS[%d]:(%3lu MHz); \n", i, rate / 1000);
	}

	s += wakelock_dump_info(s);
	
	return (s - buf);
}

static ssize_t pm_status_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	printk(KERN_ERR "%s: no support yet.\n", __func__);

	return -EPERM;
}

private_attr(tlmm);
private_attr(tlmm_sleep_table);
private_attr(tlmm_last_sleep);
private_attr(tlmm_sleep_table_before);
private_attr(tlmm_before_sleep);
private_attr(vreg);
private_attr(vreg_last_sleep);
private_attr(clk);
private_attr(tcxo_last_sleep);
private_attr(tcxo_okts_detail);
private_attr(clk_okts_detail);
//private_attr(batt_control);
private_attr(gpio_owner);
private_attr(pm_status);

static struct attribute *g_private_attr[] = {
	&tlmm_attr.attr,
	&tlmm_sleep_table_attr.attr,
	&tlmm_last_sleep_attr.attr,
	&tlmm_sleep_table_before_attr.attr,
	&tlmm_before_sleep_attr.attr,
	&vreg_attr.attr,
	&vreg_last_sleep_attr.attr,
	&clk_attr.attr,
	&tcxo_last_sleep_attr.attr,
	&tcxo_okts_detail_attr.attr,
	&clk_okts_detail_attr.attr,
	//&batt_control_attr.attr,
	&gpio_owner_attr.attr,
	&pm_status_attr.attr,
	NULL,
};

static struct attribute_group private_attr_group = {
	.attrs = g_private_attr,
};

static struct kobject *sysfs_private_kobj;

static int __init sysfs_private_init(void)
{
	int result;

	printk("%s(), %d\n", __func__, __LINE__);

	sysfs_private_kobj = kobject_create_and_add("private", NULL);
	if (!sysfs_private_kobj)
		return -ENOMEM;

	result = sysfs_create_group(sysfs_private_kobj, &private_attr_group);
	printk("%s(), %d, result=%d\n", __func__, __LINE__, result);

	return result;
}

static void __exit sysfs_private_exit(void)
{
	printk("%s(), %d\n", __func__, __LINE__);
	sysfs_remove_group(sysfs_private_kobj, &private_attr_group);

	kobject_put(sysfs_private_kobj);
}

module_init(sysfs_private_init);
module_exit(sysfs_private_exit);
