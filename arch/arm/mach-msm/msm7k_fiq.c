/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/nmi.h>
#include <asm/fiq.h>
#include <asm/hardware/gic.h>
#include <asm/cacheflush.h>
#include <mach/irqs-8625.h>
#include <mach/socinfo.h>
#include <asm/unwind.h>

#include "msm_watchdog.h"

#define MODULE_NAME "MSM7K_FIQ"

struct msm_watchdog_dump msm_dump_cpu_ctx;
static int fiq_counter;
void *msm7k_fiq_stack;

/* Called from the FIQ asm handler */
void msm7k_fiq_handler(void)
{
	struct irq_data *d;
	struct irq_chip *c;
	struct pt_regs context_regs;

	pr_info("Fiq is received %s\n", __func__);
	fiq_counter++;
	d = irq_get_irq_data(MSM8625_INT_A9_M2A_2);
	c = irq_data_get_irq_chip(d);
	c->irq_mask(d);
	local_irq_disable();

	/* Clear the IRQ from the ENABLE_SET */
	gic_clear_irq_pending(MSM8625_INT_A9_M2A_2);
	local_irq_enable();
	flush_cache_all();
	outer_flush_all();
 pr_err("%s msm_dump_cpu_ctx usr_r0:0x%x", __func__, msm_dump_cpu_ctx.usr_r0);
	pr_err("%s msm_dump_cpu_ctx usr_r0:0x%x usr_r1:0x%x usr_r2:0x%x usr_r3:0x%x usr_r4:0x%x usr_r5:0x%x usr_r6:0x%x usr_r7:0x%x usr_r8:0x%x usr_r9:0x%x usr_r10:0x%x usr_r11:0x%x usr_r12:0x%x usr_r13:0x%x usr_r14:0x%x irq_spsr:0x%x irq_r13:0x%x irq_r14:0x%x svc_spsr:0x%x svc_r13:0x%x svc_r14:0x%x abt_spsr:0x%x abt_r13:0x%x abt_r14:0x%x und_spsr:0x%x und_r13:0x%x und_r14:0x%x fiq_spsr:0x%x fiq_r8:0x%x fiq_r9:0x%x fiq_r10:0x%x fiq_r11:0x%x fiq_r12:0x%x fiq_r13:0x%x fiq_r14:0x%x\n",__func__, msm_dump_cpu_ctx.usr_r0,msm_dump_cpu_ctx.usr_r1,msm_dump_cpu_ctx.usr_r2,msm_dump_cpu_ctx.usr_r3, msm_dump_cpu_ctx.usr_r4, msm_dump_cpu_ctx.usr_r5, msm_dump_cpu_ctx.usr_r6, msm_dump_cpu_ctx.usr_r7, msm_dump_cpu_ctx.usr_r8, msm_dump_cpu_ctx.usr_r9, msm_dump_cpu_ctx.usr_r10, msm_dump_cpu_ctx.usr_r11, msm_dump_cpu_ctx.usr_r12, msm_dump_cpu_ctx.usr_r13, msm_dump_cpu_ctx.usr_r14, msm_dump_cpu_ctx.irq_spsr, msm_dump_cpu_ctx.irq_r13, msm_dump_cpu_ctx.irq_r14, msm_dump_cpu_ctx.svc_spsr, msm_dump_cpu_ctx.svc_r13, msm_dump_cpu_ctx.svc_r14, msm_dump_cpu_ctx.abt_spsr,msm_dump_cpu_ctx.abt_r13, msm_dump_cpu_ctx.abt_r14, msm_dump_cpu_ctx.und_spsr,msm_dump_cpu_ctx.und_r13, msm_dump_cpu_ctx.und_r14, msm_dump_cpu_ctx.fiq_spsr,msm_dump_cpu_ctx.fiq_r8, msm_dump_cpu_ctx.fiq_r9, msm_dump_cpu_ctx.fiq_r10, msm_dump_cpu_ctx.fiq_r11, msm_dump_cpu_ctx.fiq_r12, msm_dump_cpu_ctx.fiq_r13, msm_dump_cpu_ctx.fiq_r14);
	context_regs.ARM_sp = msm_dump_cpu_ctx.svc_r13;
	context_regs.ARM_lr = msm_dump_cpu_ctx.svc_r14;
	context_regs.ARM_fp = msm_dump_cpu_ctx.usr_r11; //for the svc r11 is the same with usr r11
	context_regs.ARM_pc = msm_dump_cpu_ctx.svc_r14;
	//dump_stack();
	unwind_backtrace(&context_regs, current);
#ifdef CONFIG_SMP
	trigger_all_cpu_backtrace();
#endif

	return;
}

struct fiq_handler msm7k_fh = {
	.name = MODULE_NAME,
};

static int __init msm_setup_fiq_handler(void)
{
	int ret = 0;

	claim_fiq(&msm7k_fh);
	set_fiq_handler(&msm7k_fiq_start, msm7k_fiq_length);
	msm7k_fiq_stack = (void *)__get_free_pages(GFP_KERNEL,
				THREAD_SIZE_ORDER);
	if (msm7k_fiq_stack == NULL) {
		pr_err("FIQ STACK SETUP IS NOT SUCCESSFUL\n");
		return -ENOMEM;
	}

	fiq_set_type(MSM8625_INT_A9_M2A_2, IRQF_TRIGGER_RISING);
	gic_set_irq_secure(MSM8625_INT_A9_M2A_2);
	enable_irq(MSM8625_INT_A9_M2A_2);
	pr_info("%s : msm7k fiq setup--done\n", __func__);
	return ret;
}

static int __init init7k_fiq(void)
{
	if (!cpu_is_msm8625() && !cpu_is_msm8625q())
		return 0;

	if (msm_setup_fiq_handler())
		pr_err("MSM7K FIQ INIT FAILED\n");

	return 0;
}
late_initcall(init7k_fiq);
