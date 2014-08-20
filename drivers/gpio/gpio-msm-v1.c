/* linux/arch/arm/mach-msm/gpio.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/mach/irq.h>
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>
#include <mach/msm_smsm.h>
#include <mach/proc_comm.h>


/* see 80-VA736-2 Rev C pp 695-751
**
** These are actually the *shadow* gpio registers, since the
** real ones (which allow full access) are only available to the
** ARM9 side of the world.
**
** Since the _BASE need to be page-aligned when we're mapping them
** to virtual addresses, adjust for the additional offset in these
** macros.
*/

#if defined(CONFIG_ARCH_MSM7X30)
#define MSM_GPIO1_REG(off) (MSM_GPIO1_BASE + (off))
#define MSM_GPIO2_REG(off) (MSM_GPIO2_BASE + 0x400 + (off))
#else
#define MSM_GPIO1_REG(off) (MSM_GPIO1_BASE + 0x800 + (off))
#define MSM_GPIO2_REG(off) (MSM_GPIO2_BASE + 0xC00 + (off))
#endif

#if defined(CONFIG_ARCH_MSM7X00A) || defined(CONFIG_ARCH_MSM7X25) ||\
    defined(CONFIG_ARCH_MSM7X27)

/* output value */
#define MSM_GPIO_OUT_0         MSM_GPIO1_REG(0x00)  /* gpio  15-0  */
#define MSM_GPIO_OUT_1         MSM_GPIO2_REG(0x00)  /* gpio  42-16 */
#define MSM_GPIO_OUT_2         MSM_GPIO1_REG(0x04)  /* gpio  67-43 */
#define MSM_GPIO_OUT_3         MSM_GPIO1_REG(0x08)  /* gpio  94-68 */
#define MSM_GPIO_OUT_4         MSM_GPIO1_REG(0x0C)  /* gpio 106-95 */
#define MSM_GPIO_OUT_5         MSM_GPIO1_REG(0x50)  /* gpio 107-121 */

/* same pin map as above, output enable */
#define MSM_GPIO_OE_0          MSM_GPIO1_REG(0x10)
#define MSM_GPIO_OE_1          MSM_GPIO2_REG(0x08)
#define MSM_GPIO_OE_2          MSM_GPIO1_REG(0x14)
#define MSM_GPIO_OE_3          MSM_GPIO1_REG(0x18)
#define MSM_GPIO_OE_4          MSM_GPIO1_REG(0x1C)
#define MSM_GPIO_OE_5          MSM_GPIO1_REG(0x54)

/* same pin map as above, input read */
#define MSM_GPIO_IN_0          MSM_GPIO1_REG(0x34)
#define MSM_GPIO_IN_1          MSM_GPIO2_REG(0x20)
#define MSM_GPIO_IN_2          MSM_GPIO1_REG(0x38)
#define MSM_GPIO_IN_3          MSM_GPIO1_REG(0x3C)
#define MSM_GPIO_IN_4          MSM_GPIO1_REG(0x40)
#define MSM_GPIO_IN_5          MSM_GPIO1_REG(0x44)

/* same pin map as above, 1=edge 0=level interrup */
#define MSM_GPIO_INT_EDGE_0    MSM_GPIO1_REG(0x60)
#define MSM_GPIO_INT_EDGE_1    MSM_GPIO2_REG(0x50)
#define MSM_GPIO_INT_EDGE_2    MSM_GPIO1_REG(0x64)
#define MSM_GPIO_INT_EDGE_3    MSM_GPIO1_REG(0x68)
#define MSM_GPIO_INT_EDGE_4    MSM_GPIO1_REG(0x6C)
#define MSM_GPIO_INT_EDGE_5    MSM_GPIO1_REG(0xC0)

/* same pin map as above, 1=positive 0=negative */
#define MSM_GPIO_INT_POS_0     MSM_GPIO1_REG(0x70)
#define MSM_GPIO_INT_POS_1     MSM_GPIO2_REG(0x58)
#define MSM_GPIO_INT_POS_2     MSM_GPIO1_REG(0x74)
#define MSM_GPIO_INT_POS_3     MSM_GPIO1_REG(0x78)
#define MSM_GPIO_INT_POS_4     MSM_GPIO1_REG(0x7C)
#define MSM_GPIO_INT_POS_5     MSM_GPIO1_REG(0xBC)

/* same pin map as above, interrupt enable */
#define MSM_GPIO_INT_EN_0      MSM_GPIO1_REG(0x80)
#define MSM_GPIO_INT_EN_1      MSM_GPIO2_REG(0x60)
#define MSM_GPIO_INT_EN_2      MSM_GPIO1_REG(0x84)
#define MSM_GPIO_INT_EN_3      MSM_GPIO1_REG(0x88)
#define MSM_GPIO_INT_EN_4      MSM_GPIO1_REG(0x8C)
#define MSM_GPIO_INT_EN_5      MSM_GPIO1_REG(0xB8)

/* same pin map as above, write 1 to clear interrupt */
#define MSM_GPIO_INT_CLEAR_0   MSM_GPIO1_REG(0x90)
#define MSM_GPIO_INT_CLEAR_1   MSM_GPIO2_REG(0x68)
#define MSM_GPIO_INT_CLEAR_2   MSM_GPIO1_REG(0x94)
#define MSM_GPIO_INT_CLEAR_3   MSM_GPIO1_REG(0x98)
#define MSM_GPIO_INT_CLEAR_4   MSM_GPIO1_REG(0x9C)
#define MSM_GPIO_INT_CLEAR_5   MSM_GPIO1_REG(0xB4)

/* same pin map as above, 1=interrupt pending */
#define MSM_GPIO_INT_STATUS_0  MSM_GPIO1_REG(0xA0)
#define MSM_GPIO_INT_STATUS_1  MSM_GPIO2_REG(0x70)
#define MSM_GPIO_INT_STATUS_2  MSM_GPIO1_REG(0xA4)
#define MSM_GPIO_INT_STATUS_3  MSM_GPIO1_REG(0xA8)
#define MSM_GPIO_INT_STATUS_4  MSM_GPIO1_REG(0xAC)
#define MSM_GPIO_INT_STATUS_5  MSM_GPIO1_REG(0xB0)

#endif

#if defined(CONFIG_ARCH_MSM7X30)

/* output value */
#define MSM_GPIO_OUT_0         MSM_GPIO1_REG(0x00)   /* gpio  15-0   */
#define MSM_GPIO_OUT_1         MSM_GPIO2_REG(0x00)   /* gpio  43-16  */
#define MSM_GPIO_OUT_2         MSM_GPIO1_REG(0x04)   /* gpio  67-44  */
#define MSM_GPIO_OUT_3         MSM_GPIO1_REG(0x08)   /* gpio  94-68  */
#define MSM_GPIO_OUT_4         MSM_GPIO1_REG(0x0C)   /* gpio 106-95  */
#define MSM_GPIO_OUT_5         MSM_GPIO1_REG(0x50)   /* gpio 133-107 */
#define MSM_GPIO_OUT_6         MSM_GPIO1_REG(0xC4)   /* gpio 150-134 */
#define MSM_GPIO_OUT_7         MSM_GPIO1_REG(0x214)  /* gpio 181-151 */

/* same pin map as above, output enable */
#define MSM_GPIO_OE_0          MSM_GPIO1_REG(0x10)
#define MSM_GPIO_OE_1          MSM_GPIO2_REG(0x08)
#define MSM_GPIO_OE_2          MSM_GPIO1_REG(0x14)
#define MSM_GPIO_OE_3          MSM_GPIO1_REG(0x18)
#define MSM_GPIO_OE_4          MSM_GPIO1_REG(0x1C)
#define MSM_GPIO_OE_5          MSM_GPIO1_REG(0x54)
#define MSM_GPIO_OE_6          MSM_GPIO1_REG(0xC8)
#define MSM_GPIO_OE_7          MSM_GPIO1_REG(0x218)

/* same pin map as above, input read */
#define MSM_GPIO_IN_0          MSM_GPIO1_REG(0x34)
#define MSM_GPIO_IN_1          MSM_GPIO2_REG(0x20)
#define MSM_GPIO_IN_2          MSM_GPIO1_REG(0x38)
#define MSM_GPIO_IN_3          MSM_GPIO1_REG(0x3C)
#define MSM_GPIO_IN_4          MSM_GPIO1_REG(0x40)
#define MSM_GPIO_IN_5          MSM_GPIO1_REG(0x44)
#define MSM_GPIO_IN_6          MSM_GPIO1_REG(0xCC)
#define MSM_GPIO_IN_7          MSM_GPIO1_REG(0x21C)

/* same pin map as above, 1=edge 0=level interrup */
#define MSM_GPIO_INT_EDGE_0    MSM_GPIO1_REG(0x60)
#define MSM_GPIO_INT_EDGE_1    MSM_GPIO2_REG(0x50)
#define MSM_GPIO_INT_EDGE_2    MSM_GPIO1_REG(0x64)
#define MSM_GPIO_INT_EDGE_3    MSM_GPIO1_REG(0x68)
#define MSM_GPIO_INT_EDGE_4    MSM_GPIO1_REG(0x6C)
#define MSM_GPIO_INT_EDGE_5    MSM_GPIO1_REG(0xC0)
#define MSM_GPIO_INT_EDGE_6    MSM_GPIO1_REG(0xD0)
#define MSM_GPIO_INT_EDGE_7    MSM_GPIO1_REG(0x240)

/* same pin map as above, 1=positive 0=negative */
#define MSM_GPIO_INT_POS_0     MSM_GPIO1_REG(0x70)
#define MSM_GPIO_INT_POS_1     MSM_GPIO2_REG(0x58)
#define MSM_GPIO_INT_POS_2     MSM_GPIO1_REG(0x74)
#define MSM_GPIO_INT_POS_3     MSM_GPIO1_REG(0x78)
#define MSM_GPIO_INT_POS_4     MSM_GPIO1_REG(0x7C)
#define MSM_GPIO_INT_POS_5     MSM_GPIO1_REG(0xBC)
#define MSM_GPIO_INT_POS_6     MSM_GPIO1_REG(0xD4)
#define MSM_GPIO_INT_POS_7     MSM_GPIO1_REG(0x228)

/* same pin map as above, interrupt enable */
#define MSM_GPIO_INT_EN_0      MSM_GPIO1_REG(0x80)
#define MSM_GPIO_INT_EN_1      MSM_GPIO2_REG(0x60)
#define MSM_GPIO_INT_EN_2      MSM_GPIO1_REG(0x84)
#define MSM_GPIO_INT_EN_3      MSM_GPIO1_REG(0x88)
#define MSM_GPIO_INT_EN_4      MSM_GPIO1_REG(0x8C)
#define MSM_GPIO_INT_EN_5      MSM_GPIO1_REG(0xB8)
#define MSM_GPIO_INT_EN_6      MSM_GPIO1_REG(0xD8)
#define MSM_GPIO_INT_EN_7      MSM_GPIO1_REG(0x22C)

/* same pin map as above, write 1 to clear interrupt */
#define MSM_GPIO_INT_CLEAR_0   MSM_GPIO1_REG(0x90)
#define MSM_GPIO_INT_CLEAR_1   MSM_GPIO2_REG(0x68)
#define MSM_GPIO_INT_CLEAR_2   MSM_GPIO1_REG(0x94)
#define MSM_GPIO_INT_CLEAR_3   MSM_GPIO1_REG(0x98)
#define MSM_GPIO_INT_CLEAR_4   MSM_GPIO1_REG(0x9C)
#define MSM_GPIO_INT_CLEAR_5   MSM_GPIO1_REG(0xB4)
#define MSM_GPIO_INT_CLEAR_6   MSM_GPIO1_REG(0xDC)
#define MSM_GPIO_INT_CLEAR_7   MSM_GPIO1_REG(0x230)

/* same pin map as above, 1=interrupt pending */
#define MSM_GPIO_INT_STATUS_0  MSM_GPIO1_REG(0xA0)
#define MSM_GPIO_INT_STATUS_1  MSM_GPIO2_REG(0x70)
#define MSM_GPIO_INT_STATUS_2  MSM_GPIO1_REG(0xA4)
#define MSM_GPIO_INT_STATUS_3  MSM_GPIO1_REG(0xA8)
#define MSM_GPIO_INT_STATUS_4  MSM_GPIO1_REG(0xAC)
#define MSM_GPIO_INT_STATUS_5  MSM_GPIO1_REG(0xB0)
#define MSM_GPIO_INT_STATUS_6  MSM_GPIO1_REG(0xE0)
#define MSM_GPIO_INT_STATUS_7  MSM_GPIO1_REG(0x234)

#endif

enum {
	GPIO_DEBUG_SLEEP = 1U << 0,
};
static int msm_gpio_debug_mask;
module_param_named(debug_mask, msm_gpio_debug_mask, int,
		   S_IRUGO | S_IWUSR | S_IWGRP);

#define FIRST_GPIO_IRQ MSM_GPIO_TO_INT(0)

#define MSM_GPIO_BANK(bank, first, last)				\
	{								\
		.regs = {						\
			.out =         MSM_GPIO_OUT_##bank,		\
			.in =          MSM_GPIO_IN_##bank,		\
			.int_status =  MSM_GPIO_INT_STATUS_##bank,	\
			.int_clear =   MSM_GPIO_INT_CLEAR_##bank,	\
			.int_en =      MSM_GPIO_INT_EN_##bank,		\
			.int_edge =    MSM_GPIO_INT_EDGE_##bank,	\
			.int_pos =     MSM_GPIO_INT_POS_##bank,		\
			.oe =          MSM_GPIO_OE_##bank,		\
		},							\
		.chip = {						\
			.base = (first),				\
			.ngpio = (last) - (first) + 1,			\
			.get = msm_gpio_get,				\
			.set = msm_gpio_set,				\
			.direction_input = msm_gpio_direction_input,	\
			.direction_output = msm_gpio_direction_output,	\
			.to_irq = msm_gpio_to_irq,			\
			.request = msm_gpio_request,			\
			.free = msm_gpio_free,				\
		}							\
	}

#define MSM_GPIO_BROKEN_INT_CLEAR 1

struct msm_gpio_regs {
	void __iomem *out;
	void __iomem *in;
	void __iomem *int_status;
	void __iomem *int_clear;
	void __iomem *int_en;
	void __iomem *int_edge;
	void __iomem *int_pos;
	void __iomem *oe;
};

struct msm_gpio_chip {
	spinlock_t		lock;
	struct gpio_chip	chip;
	struct msm_gpio_regs	regs;
#if MSM_GPIO_BROKEN_INT_CLEAR
	unsigned                int_status_copy;
#endif
	unsigned int            both_edge_detect;
	unsigned int            int_enable[2]; /* 0: awake, 1: sleep */
};

static int msm_gpio_write(struct msm_gpio_chip *msm_chip,
			  unsigned offset, unsigned on)
{
	unsigned mask = BIT(offset);
	unsigned val;

	val = __raw_readl(msm_chip->regs.out);
	if (on)
		__raw_writel(val | mask, msm_chip->regs.out);
	else
		__raw_writel(val & ~mask, msm_chip->regs.out);
	return 0;
}

static void msm_gpio_update_both_edge_detect(struct msm_gpio_chip *msm_chip)
{
	int loop_limit = 100;
	unsigned pol, val, val2, intstat;
	do {
		val = __raw_readl(msm_chip->regs.in);
		pol = __raw_readl(msm_chip->regs.int_pos);
		pol = (pol & ~msm_chip->both_edge_detect) |
		      (~val & msm_chip->both_edge_detect);
		__raw_writel(pol, msm_chip->regs.int_pos);
		intstat = __raw_readl(msm_chip->regs.int_status);
		val2 = __raw_readl(msm_chip->regs.in);
		if (((val ^ val2) & msm_chip->both_edge_detect & ~intstat) == 0)
			return;
	} while (loop_limit-- > 0);
	printk(KERN_ERR "msm_gpio_update_both_edge_detect, "
	       "failed to reach stable state %x != %x\n", val, val2);
}

static int msm_gpio_clear_detect_status(struct msm_gpio_chip *msm_chip,
					unsigned offset)
{
	unsigned bit = BIT(offset);

#if MSM_GPIO_BROKEN_INT_CLEAR
	/* Save interrupts that already triggered before we loose them. */
	/* Any interrupt that triggers between the read of int_status */
	/* and the write to int_clear will still be lost though. */
	msm_chip->int_status_copy |= __raw_readl(msm_chip->regs.int_status);
	msm_chip->int_status_copy &= ~bit;
#endif
	__raw_writel(bit, msm_chip->regs.int_clear);
	msm_gpio_update_both_edge_detect(msm_chip);
	return 0;
}

static int msm_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_chip *msm_chip;
	unsigned long irq_flags;

	msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	__raw_writel(__raw_readl(msm_chip->regs.oe) & ~BIT(offset),
			msm_chip->regs.oe);
	mb();
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
	return 0;
}

static int
msm_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct msm_gpio_chip *msm_chip;
	unsigned long irq_flags;

	msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	msm_gpio_write(msm_chip, offset, value);
	__raw_writel(__raw_readl(msm_chip->regs.oe) | BIT(offset),
			msm_chip->regs.oe);
	mb();
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
	return 0;
}

static int msm_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_chip *msm_chip;
	int rc;

	msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	rc = (__raw_readl(msm_chip->regs.in) & (1U << offset)) ? 1 : 0;
	mb();
	return rc;
}

static void msm_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct msm_gpio_chip *msm_chip;
	unsigned long irq_flags;

	msm_chip = container_of(chip, struct msm_gpio_chip, chip);
	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	msm_gpio_write(msm_chip, offset, value);
	mb();
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
}

static int msm_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	return MSM_GPIO_TO_INT(chip->base + offset);
}

#ifdef CONFIG_MSM_GPIOMUX
static int msm_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return msm_gpiomux_get(chip->base + offset);
}

static void msm_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	msm_gpiomux_put(chip->base + offset);
}
#else
#define msm_gpio_request NULL
#define msm_gpio_free NULL
#endif

struct msm_gpio_chip msm_gpio_chips[] = {
#if defined(CONFIG_ARCH_MSM7X00A)
	MSM_GPIO_BANK(0,   0,  15),
	MSM_GPIO_BANK(1,  16,  42),
	MSM_GPIO_BANK(2,  43,  67),
	MSM_GPIO_BANK(3,  68,  94),
	MSM_GPIO_BANK(4,  95, 106),
	MSM_GPIO_BANK(5, 107, 121),
#elif defined(CONFIG_ARCH_MSM7X25) || defined(CONFIG_ARCH_MSM7X27)
	MSM_GPIO_BANK(0,   0,  15),
	MSM_GPIO_BANK(1,  16,  42),
	MSM_GPIO_BANK(2,  43,  67),
	MSM_GPIO_BANK(3,  68,  94),
	MSM_GPIO_BANK(4,  95, 106),
	MSM_GPIO_BANK(5, 107, 132),
#elif defined(CONFIG_ARCH_MSM7X30)
	MSM_GPIO_BANK(0,   0,  15),
	MSM_GPIO_BANK(1,  16,  43),
	MSM_GPIO_BANK(2,  44,  67),
	MSM_GPIO_BANK(3,  68,  94),
	MSM_GPIO_BANK(4,  95, 106),
	MSM_GPIO_BANK(5, 107, 133),
	MSM_GPIO_BANK(6, 134, 150),
	MSM_GPIO_BANK(7, 151, 181),
#elif defined(CONFIG_ARCH_QSD8X50)
	MSM_GPIO_BANK(0,   0,  15),
	MSM_GPIO_BANK(1,  16,  42),
	MSM_GPIO_BANK(2,  43,  67),
	MSM_GPIO_BANK(3,  68,  94),
	MSM_GPIO_BANK(4,  95, 103),
	MSM_GPIO_BANK(5, 104, 121),
	MSM_GPIO_BANK(6, 122, 152),
	MSM_GPIO_BANK(7, 153, 164),
#endif
};

static void msm_gpio_irq_ack(struct irq_data *d)
{
	unsigned long irq_flags;
	struct msm_gpio_chip *msm_chip = irq_get_chip_data(d->irq);
	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	msm_gpio_clear_detect_status(msm_chip,
			     d->irq - gpio_to_irq(msm_chip->chip.base));
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
}

static void msm_gpio_irq_mask(struct irq_data *d)
{
	unsigned long irq_flags;
	struct msm_gpio_chip *msm_chip = irq_get_chip_data(d->irq);
	unsigned offset = d->irq - gpio_to_irq(msm_chip->chip.base);

	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	/* level triggered interrupts are also latched */
	if (!(__raw_readl(msm_chip->regs.int_edge) & BIT(offset)))
		msm_gpio_clear_detect_status(msm_chip, offset);
	msm_chip->int_enable[0] &= ~BIT(offset);
	__raw_writel(msm_chip->int_enable[0], msm_chip->regs.int_en);
	mb();
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
}

static void msm_gpio_irq_unmask(struct irq_data *d)
{
	unsigned long irq_flags;
	struct msm_gpio_chip *msm_chip = irq_get_chip_data(d->irq);
	unsigned offset = d->irq - gpio_to_irq(msm_chip->chip.base);

	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	/* level triggered interrupts are also latched */
	if (!(__raw_readl(msm_chip->regs.int_edge) & BIT(offset)))
		msm_gpio_clear_detect_status(msm_chip, offset);
	msm_chip->int_enable[0] |= BIT(offset);
	__raw_writel(msm_chip->int_enable[0], msm_chip->regs.int_en);
	mb();
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
}

static int msm_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	unsigned long irq_flags;
	struct msm_gpio_chip *msm_chip = irq_get_chip_data(d->irq);
	unsigned offset = d->irq - gpio_to_irq(msm_chip->chip.base);

	spin_lock_irqsave(&msm_chip->lock, irq_flags);

	if (on)
		msm_chip->int_enable[1] |= BIT(offset);
	else
		msm_chip->int_enable[1] &= ~BIT(offset);

	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
	return 0;
}

static int msm_gpio_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	unsigned long irq_flags;
	struct msm_gpio_chip *msm_chip = irq_get_chip_data(d->irq);
	unsigned offset = d->irq - gpio_to_irq(msm_chip->chip.base);
	unsigned val, mask = BIT(offset);

	spin_lock_irqsave(&msm_chip->lock, irq_flags);
	val = __raw_readl(msm_chip->regs.int_edge);
	if (flow_type & IRQ_TYPE_EDGE_BOTH) {
		__raw_writel(val | mask, msm_chip->regs.int_edge);
		__irq_set_handler_locked(d->irq, handle_edge_irq);
	} else {
		__raw_writel(val & ~mask, msm_chip->regs.int_edge);
		__irq_set_handler_locked(d->irq, handle_level_irq);
	}
	if ((flow_type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		msm_chip->both_edge_detect |= mask;
		msm_gpio_update_both_edge_detect(msm_chip);
	} else {
		msm_chip->both_edge_detect &= ~mask;
		val = __raw_readl(msm_chip->regs.int_pos);
		if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_HIGH))
			__raw_writel(val | mask, msm_chip->regs.int_pos);
		else
			__raw_writel(val & ~mask, msm_chip->regs.int_pos);
	}
	mb();
	spin_unlock_irqrestore(&msm_chip->lock, irq_flags);
	return 0;
}

static void msm_gpio_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	int i, j, mask;
	unsigned val;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	chained_irq_enter(chip, desc);

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		struct msm_gpio_chip *msm_chip = &msm_gpio_chips[i];
		val = __raw_readl(msm_chip->regs.int_status);
		val &= msm_chip->int_enable[0];
		while (val) {
			mask = val & -val;
			j = fls(mask) - 1;
			/* printk("%s %08x %08x bit %d gpio %d irq %d\n",
				__func__, v, m, j, msm_chip->chip.start + j,
				FIRST_GPIO_IRQ + msm_chip->chip.start + j); */
			val &= ~mask;
			generic_handle_irq(FIRST_GPIO_IRQ +
					   msm_chip->chip.base + j);
		}
	}

	chained_irq_exit(chip, desc);
}

static struct irq_chip msm_gpio_irq_chip = {
	.name      = "msmgpio",
	.irq_ack	= msm_gpio_irq_ack,
	.irq_mask	= msm_gpio_irq_mask,
	.irq_unmask	= msm_gpio_irq_unmask,
	.irq_set_wake	= msm_gpio_irq_set_wake,
	.irq_set_type	= msm_gpio_irq_set_type,
};

#define NUM_GPIO_SMEM_BANKS 6
#define GPIO_SMEM_NUM_GROUPS 2
#define GPIO_SMEM_MAX_PC_INTERRUPTS 8
struct tramp_gpio_smem {
	uint16_t num_fired[GPIO_SMEM_NUM_GROUPS];
	uint16_t fired[GPIO_SMEM_NUM_GROUPS][GPIO_SMEM_MAX_PC_INTERRUPTS];
	uint32_t enabled[NUM_GPIO_SMEM_BANKS];
	uint32_t detection[NUM_GPIO_SMEM_BANKS];
	uint32_t polarity[NUM_GPIO_SMEM_BANKS];
};

static void msm_gpio_sleep_int(unsigned long arg)
{
	int i, j;
	struct tramp_gpio_smem *smem_gpio;

	BUILD_BUG_ON(NR_GPIO_IRQS > NUM_GPIO_SMEM_BANKS * 32);

	smem_gpio = smem_alloc(SMEM_GPIO_INT, sizeof(*smem_gpio));
	if (smem_gpio == NULL)
		return;

	local_irq_disable();
	for (i = 0; i < GPIO_SMEM_NUM_GROUPS; i++) {
		int count = smem_gpio->num_fired[i];
		for (j = 0; j < count; j++) {
			/* TODO: Check mask */
			generic_handle_irq(
				MSM_GPIO_TO_INT(smem_gpio->fired[i][j]));
		}
	}
	local_irq_enable();
}

static DECLARE_TASKLET(msm_gpio_sleep_int_tasklet, msm_gpio_sleep_int, 0);

void msm_gpio_enter_sleep(int from_idle)
{
	int i;
	struct tramp_gpio_smem *smem_gpio;

	smem_gpio = smem_alloc(SMEM_GPIO_INT, sizeof(*smem_gpio));

	if (smem_gpio) {
		for (i = 0; i < ARRAY_SIZE(smem_gpio->enabled); i++) {
			smem_gpio->enabled[i] = 0;
			smem_gpio->detection[i] = 0;
			smem_gpio->polarity[i] = 0;
		}
	}

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		__raw_writel(msm_gpio_chips[i].int_enable[!from_idle],
		       msm_gpio_chips[i].regs.int_en);
		if (smem_gpio) {
			uint32_t tmp;
			int start, index, shiftl, shiftr;
			start = msm_gpio_chips[i].chip.base;
			index = start / 32;
			shiftl = start % 32;
			shiftr = 32 - shiftl;
			tmp = msm_gpio_chips[i].int_enable[!from_idle];
			smem_gpio->enabled[index] |= tmp << shiftl;
			smem_gpio->enabled[index+1] |= tmp >> shiftr;
			smem_gpio->detection[index] |=
				__raw_readl(msm_gpio_chips[i].regs.int_edge) <<
				shiftl;
			smem_gpio->detection[index+1] |=
				__raw_readl(msm_gpio_chips[i].regs.int_edge) >>
				shiftr;
			smem_gpio->polarity[index] |=
				__raw_readl(msm_gpio_chips[i].regs.int_pos) <<
				shiftl;
			smem_gpio->polarity[index+1] |=
				__raw_readl(msm_gpio_chips[i].regs.int_pos) >>
				shiftr;
		}
	}
	mb();

	if (smem_gpio) {
		if (msm_gpio_debug_mask & GPIO_DEBUG_SLEEP)
			for (i = 0; i < ARRAY_SIZE(smem_gpio->enabled); i++) {
				printk("msm_gpio_enter_sleep gpio %d-%d: enable"
				       " %08x, edge %08x, polarity %08x\n",
				       i * 32, i * 32 + 31,
				       smem_gpio->enabled[i],
				       smem_gpio->detection[i],
				       smem_gpio->polarity[i]);
			}
		for (i = 0; i < GPIO_SMEM_NUM_GROUPS; i++)
			smem_gpio->num_fired[i] = 0;
	}
}

void msm_gpio_exit_sleep(void)
{
	int i;
	struct tramp_gpio_smem *smem_gpio;

	smem_gpio = smem_alloc(SMEM_GPIO_INT, sizeof(*smem_gpio));

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		__raw_writel(msm_gpio_chips[i].int_enable[0],
		       msm_gpio_chips[i].regs.int_en);
	}
	mb();

	if (smem_gpio && (smem_gpio->num_fired[0] || smem_gpio->num_fired[1])) {
		if (msm_gpio_debug_mask & GPIO_DEBUG_SLEEP)
			printk(KERN_INFO "gpio: fired %x %x\n",
			      smem_gpio->num_fired[0], smem_gpio->num_fired[1]);
		tasklet_schedule(&msm_gpio_sleep_int_tasklet);
	}
}


int gpio_tlmm_config(unsigned config, unsigned disable)
{
	return msm_proc_comm(PCOM_RPC_GPIO_TLMM_CONFIG_EX, &config, &disable);
}
EXPORT_SYMBOL(gpio_tlmm_config);

int msm_gpios_request_enable(const struct msm_gpio *table, int size)
{
	int rc = msm_gpios_request(table, size);
	if (rc)
		return rc;
	rc = msm_gpios_enable(table, size);
	if (rc)
		msm_gpios_free(table, size);
	return rc;
}
EXPORT_SYMBOL(msm_gpios_request_enable);

void msm_gpios_disable_free(const struct msm_gpio *table, int size)
{
	msm_gpios_disable(table, size);
	msm_gpios_free(table, size);
}
EXPORT_SYMBOL(msm_gpios_disable_free);

int msm_gpios_request(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = 0; i < size; i++) {
		g = table + i;
		rc = gpio_request(GPIO_PIN(g->gpio_cfg), g->label);
		if (rc) {
			pr_err("gpio_request(%d) <%s> failed: %d\n",
			       GPIO_PIN(g->gpio_cfg), g->label ?: "?", rc);
			goto err;
		}
	}
	return 0;
err:
	msm_gpios_free(table, i);
	return rc;
}
EXPORT_SYMBOL(msm_gpios_request);

void msm_gpios_free(const struct msm_gpio *table, int size)
{
	int i;
	const struct msm_gpio *g;
	for (i = size-1; i >= 0; i--) {
		g = table + i;
		gpio_free(GPIO_PIN(g->gpio_cfg));
	}
}
EXPORT_SYMBOL(msm_gpios_free);

int msm_gpios_enable(const struct msm_gpio *table, int size)
{
	int rc;
	int i;
	const struct msm_gpio *g;
	for (i = 0; i < size; i++) {
		g = table + i;
		rc = gpio_tlmm_config(g->gpio_cfg, GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("gpio_tlmm_config(0x%08x, GPIO_CFG_ENABLE)"
			       " <%s> failed: %d\n",
			       g->gpio_cfg, g->label ?: "?", rc);
			pr_err("pin %d func %d dir %d pull %d drvstr %d\n",
			       GPIO_PIN(g->gpio_cfg), GPIO_FUNC(g->gpio_cfg),
			       GPIO_DIR(g->gpio_cfg), GPIO_PULL(g->gpio_cfg),
			       GPIO_DRVSTR(g->gpio_cfg));
			goto err;
		}
	}
	return 0;
err:
	msm_gpios_disable(table, i);
	return rc;
}
EXPORT_SYMBOL(msm_gpios_enable);

int msm_gpios_disable(const struct msm_gpio *table, int size)
{
	int rc = 0;
	int i;
	const struct msm_gpio *g;
	for (i = size-1; i >= 0; i--) {
		int tmp;
		g = table + i;
		tmp = gpio_tlmm_config(g->gpio_cfg, GPIO_CFG_DISABLE);
		if (tmp) {
			pr_err("gpio_tlmm_config(0x%08x, GPIO_CFG_DISABLE)"
			       " <%s> failed: %d\n",
			       g->gpio_cfg, g->label ?: "?", rc);
			pr_err("pin %d func %d dir %d pull %d drvstr %d\n",
			       GPIO_PIN(g->gpio_cfg), GPIO_FUNC(g->gpio_cfg),
			       GPIO_DIR(g->gpio_cfg), GPIO_PULL(g->gpio_cfg),
			       GPIO_DRVSTR(g->gpio_cfg));
			if (!rc)
				rc = tmp;
		}
	}

	return rc;
}
EXPORT_SYMBOL(msm_gpios_disable);

/* Locate the GPIO_OUT register for the given GPIO and return its address
 * and the bit position of the gpio's bit within the register.
 *
 * This function is used by gpiomux-v1 in order to support output transitions.
 */
void msm_gpio_find_out(const unsigned gpio, void __iomem **out,
	unsigned *offset)
{
	struct msm_gpio_chip *msm_chip = msm_gpio_chips;

	while (gpio >= msm_chip->chip.base + msm_chip->chip.ngpio)
		++msm_chip;

	*out = msm_chip->regs.out;
	*offset = gpio - msm_chip->chip.base;
}

static int __devinit msm_gpio_probe(struct platform_device *dev)
{
	int i, j = 0;
	int grp_irq;

	for (i = FIRST_GPIO_IRQ; i < FIRST_GPIO_IRQ + NR_GPIO_IRQS; i++) {
		if (i - FIRST_GPIO_IRQ >=
			msm_gpio_chips[j].chip.base +
			msm_gpio_chips[j].chip.ngpio)
			j++;
		irq_set_chip_data(i, &msm_gpio_chips[j]);
		irq_set_chip_and_handler(i, &msm_gpio_irq_chip,
					 handle_edge_irq);
		set_irq_flags(i, IRQF_VALID);
	}

	for (i = 0; i < dev->num_resources; i++) {
		grp_irq = platform_get_irq(dev, i);
		if (grp_irq < 0)
			return -ENXIO;

		irq_set_chained_handler(grp_irq, msm_gpio_irq_handler);
		irq_set_irq_wake(grp_irq, (i + 1));
	}

	for (i = 0; i < ARRAY_SIZE(msm_gpio_chips); i++) {
		spin_lock_init(&msm_gpio_chips[i].lock);
		__raw_writel(0, msm_gpio_chips[i].regs.int_en);
		gpiochip_add(&msm_gpio_chips[i].chip);
	}

	mb();
	return 0;
}

static struct platform_driver msm_gpio_driver = {
	.probe = msm_gpio_probe,
	.driver = {
		.name = "msmgpio",
		.owner = THIS_MODULE,
	},
};

static int __init msm_gpio_init(void)
{
	return platform_driver_register(&msm_gpio_driver);
}
postcore_initcall(msm_gpio_init);
/* yangjq, 20121127, Add sysfs for gpio's debug, START */
#define TLMM_NUM_GPIO 133

#define TLMM_GPIO_CFG(gpio, func, dir, pull, drive) \
         (((gpio)&0xFF)<< 8 |  \
          ((func)&0xF)|        \
          ((dir)&0x1) << 16|   \
          ((pull)&0x3) << 17|  \
          ((drive)&0x7)<< 19)

#define HAL_OWNER_VAL(config)    \
         (((config)&0x80000000)>>31)

#define HAL_OUTPUT_VAL(config)    \
         (((config)&0x40000000)>>30)

#define HAL_GPIO_NUMBER(config) \
         ((0x20000000&(config))?(((config)&0x3FF0)>>4):(((config)&0xFF00)>>8))

#define HAL_RMT_VAL(config)     \
         ((0x20000000&(config))?(((config)&0x1E00000)>>0x15):(((config)&0xF0)>>4))

#define HAL_DRVSTR_VAL(config) \
         ((0x20000000&(config))?(((config)&0x1E0000)>>17):(((config)&0x380000)>>19))

#define HAL_PULL_VAL(config)   \
         ((0x20000000&(config))?(((config)&0x18000)>>15):(((config)&0x60000)>>17))

#define HAL_DIR_VAL(config)    \
         ((0x20000000&(config))?(((config)&0x4000)>>14):(((config)&0x10000)>>16))

#define HAL_FUNC_VAL(config)  ((config)&0xF)

#define HAL_GPIO_OUTVAL HAL_RMT_VAL

/* yangjq, 20121113, Get gpio's owner */
static int tlmm_get_gpio_owner(unsigned gpio, unsigned *owner)
{
	int res;
	BUG_ON(gpio >= TLMM_NUM_GPIO); // gpio > TLMM_NUM_GPIO is valid here(to get another info)
	
	*owner = (u32)0x02;
	res = msm_proc_comm(PCOM_OEM_GPIO_TLMM_GET_OWNER, &gpio, owner); // get anCurrentOwner[] in AMSS
	if(res < 0) 
		printk("Warning: msm_proc_comm(PCOM_GPIO_TLMM_GET_OWNER) of gpio %d = 0x%x\n", gpio, res);
	
	return res;
}

/* yangjq, 20121113, Get tlmm/gpio info */
static int tlmm_get_cfg(unsigned gpio, unsigned* cfg)
{
	int res;
	BUG_ON(gpio >= TLMM_NUM_GPIO); // gpio > TLMM_NUM_GPIO is valid here(to get another info)
	
	*cfg = (u32)0xffff;
	res = msm_proc_comm(PCOM_OEM_GPIO_TLMM_GET_CFG, &gpio, cfg); // get anImmediateConfig[] in AMSS
	if(res < 0) 
		printk("Warning: msm_proc_comm(PCOM_GPIO_TLMM_GET_CFG) of gpio %d = 0x%x\n", gpio, res);
	return res;
}

/* yangjq, 20120106, Set GPIO's sleep config from sysfs */
static int tlmm_sleep_table_get_cfg(unsigned gpio, unsigned* cfg)
{
	int res;
	BUG_ON(gpio >= TLMM_NUM_GPIO);
	
	*cfg = (u32)0xffff;
	res = msm_proc_comm(PCOM_OEM_GPIO_TLMM_SLEEP_TABLE_GET_CFG, &gpio, cfg); // get SLEEP_CONFIGS[] in AMSS
	if(res < 0) 
		printk("Warning: msm_proc_comm(PCOM_GPIO_TLMM_SLEEP_TABLE_GET_CFG) of gpio %d = 0x%x\n", gpio, res);
	return res;
}

int tlmm_sleep_table_set_cfg(unsigned gpio, unsigned cfg)
{
	int res;
	//BUG_ON(gpio >= TLMM_NUM_GPIO && HAL_GPIO_NUMBER(cfg) != 0xff);
	if (gpio >= TLMM_NUM_GPIO && gpio != 255 && gpio != 256) {
		printk("gpio >= TLMM_NUM_GPIO && gpio != 255 && gpio != 256!\n");
		return -1;
	}

	res = msm_proc_comm(PCOM_OEM_GPIO_TLMM_SLEEP_TABLE_SET_CFG, &gpio, &cfg); // set SLEEP_CONFIGS[] in AMSS
	if(res < 0) 
		printk("Warning: msm_proc_comm(PCOM_GPIO_TLMM_SLEEP_TABLE__SET_CFG) of gpio %d = 0x%x\n", gpio, res);
	return res;
}

/* yangjq, 20120106, Get GPIO's sleep config from sysfs */
int tlmm_last_sleep_get_cfg(unsigned gpio, unsigned* cfg)
{
	int res;
	BUG_ON(gpio >= TLMM_NUM_GPIO && gpio != 256);
	
	*cfg = (u32)0xffff;
	res = msm_proc_comm(PCOM_OEM_GPIO_TLMM_LAST_SLEEP_GET_CFG, &gpio, cfg); 
	if(res < 0)
		printk("Warning: msm_proc_comm(PCOM_GPIO_TLMM_LAST_SLEEP_GET_CFG) of gpio %d = 0x%x\n", gpio, res);
	return res;
}

static int tlmm_is_gpio(unsigned cfg)
{
	return (cfg & 0xf) == 0; 
}

#if 0
static int tlmm_is_gpio_output(unsigned cfg)
{
	return tlmm_is_gpio(cfg) && (((cfg >> 16) & 0x1) == 1); 
}
#endif //0

static int tlmm_dump_cfg(char* buf,unsigned gpio, unsigned cfg, int output_val)
{
	static char* drvstr_str[] = { "2", "4", "6", "8", "10", "12", "14", "16" }; // mA
	static char*   pull_str[] = { "N", "D", "K", "U" };	 // "NO_PULL", "PULL_DOWN", "KEEPER", "PULL_UP"
	static char*    dir_str[] = { "I", "O" }; // "Input", "Output"	 
	static char*    rmt_str[] = { "N", "A", "L", "H"  }; // "NO_RMT", "RMT_TO_ALL", HAL_TLMM_OUTPUT_LOW, HAL_TLMM_OUTPUT_HIGH

	char func_str[20];
	
	char* p = buf;

#if 0
	// refer to AMSS GPIO_CFG definition. Note: Linux has a different definition for GPIO_CFG
	int drvstr   = (cfg >> 19) & 0x7;
	int pull     = (cfg >> 17) & 0x3;
	int dir      = (cfg >> 16) & 0x1;
	int gpio_num = (cfg >>  8) & 0xff; // shoud be same as parameter gpio
	int rmt      = (cfg >>  4) & 0xf & 0x1;  // it has 4 bits, but the value only 0 or 1
	int func     =  cfg        & 0xf;
#else
	int drvstr   = HAL_DRVSTR_VAL(cfg);
	int pull     = HAL_PULL_VAL(cfg);
	int dir      = HAL_DIR_VAL(cfg);
	int gpio_num = HAL_GPIO_NUMBER(cfg); // shoud be same as parameter gpio
	int rmt      = HAL_RMT_VAL(cfg);  // it has 4 bits, but the value only 0 or 1
	int func     = HAL_FUNC_VAL(cfg);
#endif //0

	if(gpio_num != gpio) { // gpio_num can be 0xff in sleep config
		p += sprintf(p, "[%d]:0x%x gpio=%d!\n", gpio, cfg, gpio_num);
		return p - buf;
	}

	if(tlmm_is_gpio(cfg))
		sprintf(func_str, "0");
	else
		sprintf(func_str, "%d", func);

	//used_in_linux(gpio)
	p += sprintf(p, "%s%d:0x%x %s%s%s%s%s", HAL_OWNER_VAL(cfg) ? "*" : " ", gpio, cfg,
			func_str, pull_str[pull], dir_str[dir], drvstr_str[drvstr], rmt_str[rmt]);

	//if(tlmm_is_gpio_output(cfg))
		p += sprintf(p, " = %d", output_val);

	p += sprintf(p, "\n");	
			
	return p - buf;		
}

/*
difference between current tlmm and tlmm_sleep_table
1. tlmm_sleep_table's gpio can be 0xff (mean don't set the tlmm when sleep)
2. tlmm_sleep_table's output is got from rmt, current tlmm output is got from current optput status

*/
static int tlmm_sleep_table_dump_cfg(char* buf,unsigned gpio, unsigned cfg)
{
	static char* drvstr_str[] = { "2", "4", "6", "8", "10", "12", "14", "16" }; // mA
	static char*   pull_str[] = { "N", "D", "K", "U" };	 // "NO_PULL", "PULL_DOWN", "KEEPER", "PULL_UP"
	static char*    dir_str[] = { "I", "O" }; // "Input", "Output"	 

	// for current tlmm config, rmt can be NO_RMT or RMT_TO_ALL
	// for sleep   tlmm config, rmt will be HAL_TLMM_OUTPUT_HIGH or HAL_TLMM_OUTPUT_LOW (valid when gpio output)
	static char*    rmt_str[] = { "N", "A", "L", "H" }; // "NO_RMT", "RMT_TO_ALL", HAL_TLMM_OUTPUT_LOW, HAL_TLMM_OUTPUT_HIGH

	char func_str[20];
	
	char* p = buf;

#if 0
	// refer to AMSS GPIO_CFG definition. Note: Linux has a different definition for GPIO_CFG
	int drvstr   = (cfg >> 19) & 0x7;
	int pull     = (cfg >> 17) & 0x3;
	int dir      = (cfg >> 16) & 0x1;
	int gpio_num = (cfg >>  8) & 0xff; // shoud be same as parameter gpio
	int rmt      = (cfg >>  4) & 0xf & 0x3;  // it has 4 bits, but possible value is 2 or 3
	int func     =  cfg        & 0xf;
#else
	int drvstr   = HAL_DRVSTR_VAL(cfg);
	int pull     = HAL_PULL_VAL(cfg);
	int dir      = HAL_DIR_VAL(cfg);
	int gpio_num = HAL_GPIO_NUMBER(cfg); // shoud be same as parameter gpio
	int rmt      = HAL_RMT_VAL(cfg);  // it has 4 bits, but the value only 0 or 1
	int func     = HAL_FUNC_VAL(cfg);
	int val      = HAL_OUTPUT_VAL(cfg);
#endif //0

	if((gpio_num != gpio) && (gpio_num != 0xff)) { // gpio_num can be 0xff in sleep config
		p += sprintf(p, "[%d]:0x%x gpio=%d!\n", gpio, cfg, gpio_num);
		return p - buf;
	}

	if(tlmm_is_gpio(cfg))
		sprintf(func_str, "0");
	else
		sprintf(func_str, "%d", func);

	if(gpio_num == 0xff)
		p += sprintf(p, "%d: No Setting.", gpio);
	else {
		p += sprintf(p, "%s%d:0x%x %s%s%s%s%s", HAL_OWNER_VAL(cfg) ? "*" : " ", gpio, cfg, 
			func_str, pull_str[pull], dir_str[dir], drvstr_str[drvstr], rmt_str[rmt]);
		//if(tlmm_is_gpio_output(cfg))
			p += sprintf(p, " = %d", val);
	}

	p += sprintf(p, "\n");
	
			
	return p - buf;		
}

static int tlmm_dump_header(char* buf)
{
	char* p = buf;
	p += sprintf(p, "bit  0~ 3: function. (0 is GPIO)\n");
	p += sprintf(p, "bit  4~ 7: RMT: 00:NO_RMT, 01:RMT_TO_ALL, 10: output low, 11: output high\n");
	p += sprintf(p, "bit  8~15: gpio number\n");
	p += sprintf(p, "bit 16     : 0: input, 1: output\n");
	p += sprintf(p, "bit 17~18: pull: NO_PULL, PULL_DOWN, KEEPER, PULL_UP\n");
	p += sprintf(p, "bit 19~21: driver strength. \n");
	p += sprintf(p, "0:GPIO\n");
	p += sprintf(p, "N:NO_PULL  D:PULL_DOWN  K:KEEPER  U:PULL_UP\n");
	p += sprintf(p, "I:Input  O:Output\n");
	p += sprintf(p, "2, 4, 6, 8, 10, 12, 14, 16 mA (driver strength) \n");
	p += sprintf(p, "N:NO_RMT  A:RMT_TO_ALL  L:LOW  H:HIGH\n\n");
	return p - buf;	
}

int tlmm_dump_info(char* buf)
{
	unsigned i;
	char* p = buf;

	p += tlmm_dump_header(p);
	p += sprintf(p, "Format: gpio_num  function  pull  direction  strength [output_value]\n");
	p += sprintf(p, " e.g.  'echo  20 0 D O 2 1'  ==> set pin 20 as GPIO output and the output = 1 \n");
	
	for(i = 0; i < TLMM_NUM_GPIO; ++i) {
		unsigned cfg;
		int output_val = 0;

        // version 1
		tlmm_get_cfg(i, &cfg);
		//if(tlmm_is_gpio_output(cfg))
			//output_val = gpio_get_value(i);
			output_val = HAL_OWNER_VAL(cfg) ? gpio_get_value(i) : HAL_OUTPUT_VAL(cfg);
			
		p += tlmm_dump_cfg(p, i, cfg, output_val);
	}
	p+= sprintf(p, "(%d)\n", p - buf); // only for debug reference	
	return p - buf;	
}

/* yangjq, 20120106, Show GPIO's sleep config */
int tlmm_sleep_table_dump_info(char* buf)
{
	unsigned i;
	char* p = buf;

	p += tlmm_dump_header(p);
	p += sprintf(p, "Format: gpio_num  function  pull  direction  strength [output_value]\n");
	p += sprintf(p, " e.g.  'echo  20 0 D O 2 1'  ==> set pin 20 as GPIO output and the output = 1 \n");
	p += sprintf(p, " e.g.  'echo  20'  ==> disable pin 20's setting \n");
	p += sprintf(p, " e.g.  'echo  255'  ==> enable sleep table's setting \n");
	p += sprintf(p, " e.g.  'echo  256'  ==> disable sleep table's setting \n");

	for(i = 0; i < TLMM_NUM_GPIO; ++i) {
		unsigned cfg;
		tlmm_sleep_table_get_cfg(i, &cfg);
		p += tlmm_sleep_table_dump_cfg(p, i, cfg);
	}
	p+= sprintf(p, "(%d)\n", p - buf); // only for debug reference	
	return p - buf;	
}

int tlmm_last_sleep_dump_info(char* buf)
{
	unsigned i;
	unsigned cfg;
	int fetched;
	char* p = buf;

	fetched = 0;
	if(tlmm_last_sleep_get_cfg(256, &cfg) == 0)
		fetched = cfg;

	p += sprintf(p, "tlmm_last_sleep:\n");
	if (!fetched) {
		p += tlmm_dump_header(p);
		
		for(i = 0; i < TLMM_NUM_GPIO; ++i) {
			tlmm_last_sleep_get_cfg(i, &cfg);
			p += tlmm_sleep_table_dump_cfg(p, i, cfg);
		}
		p+= sprintf(p, "(%d)\n", p - buf); // only for debug reference
	}
	return p - buf;	
}

int tlmm_gpio_owner_dump_info(char* buf)
{
	unsigned i;
	char* p = buf;
	
	for(i = 0; i < TLMM_NUM_GPIO; ++i) {
		unsigned owner;
		tlmm_get_gpio_owner(i, &owner);
		if(owner==2){
			p += sprintf(p, "%03d : Unknown Owner \n", i);
		}else{
			p += sprintf(p, "%03d : %s \n", i, owner?"APP":"MODEM");
		}
	}
	
	return p - buf;	
}

/* yangjq, 20111216, save tlmm config before sleep */
static unsigned before_sleep_fetched;
static unsigned before_sleep_configs[TLMM_NUM_GPIO];
void tlmm_before_sleep_save_configs(void)
{
	unsigned i;

	//only save tlmm configs when it has been fetched
	if (!before_sleep_fetched)
		return;

	printk("%s(), before_sleep_fetched=%d\n", __func__, before_sleep_fetched);
	before_sleep_fetched = false;
	for(i = 0; i < TLMM_NUM_GPIO; ++i) {
		unsigned cfg;
		int output_val = 0;

        // version 1
		tlmm_get_cfg(i, &cfg);
		//if(tlmm_is_gpio_output(cfg))
			//output_val = gpio_get_value(i);
			output_val = HAL_OWNER_VAL(cfg) ? gpio_get_value(i) : HAL_OUTPUT_VAL(cfg);

		before_sleep_configs[i] = cfg | (output_val << 30);
	}
}

int tlmm_before_sleep_dump_info(char* buf)
{
	unsigned i;
	char* p = buf;

	p += sprintf(p, "tlmm_before_sleep:\n");
	if (!before_sleep_fetched) {
		before_sleep_fetched = true;

		p += tlmm_dump_header(p);
		
		for(i = 0; i < TLMM_NUM_GPIO; ++i) {
			unsigned cfg;
			int output_val = 0;

			cfg = before_sleep_configs[i];
			output_val = HAL_OUTPUT_VAL(cfg);
			//cfg &= 0x3fffffff;
			p += tlmm_dump_cfg(p, i, cfg, output_val);
		}
		p+= sprintf(p, "(%d)\n", p - buf); // only for debug reference
	}
	return p - buf;	
}
/* yangjq, 20130128, set tlmm config before sleep */
static unsigned sleep_table_before_enabled;
static unsigned sleep_table_before_configs[TLMM_NUM_GPIO];

void tlmm_before_sleep_set_configs(void)
{
	int res;
	unsigned i;

	//only save tlmm configs when it has been fetched
	if (!sleep_table_before_enabled)
		return;

	printk("%s(), sleep_table_before_enabled=%d\n", __func__, sleep_table_before_enabled);
	for(i = 0; i < TLMM_NUM_GPIO; ++i) {
		unsigned cfg;
		int gpio;
		int dir;
		int func;
		int output_val = 0;

		cfg = sleep_table_before_configs[i];

		gpio = HAL_GPIO_NUMBER(cfg);
		if((cfg & ~0x20000000) == 0 || gpio != i)
			continue;

		output_val = HAL_OUTPUT_VAL(cfg);
		dir = HAL_DIR_VAL(cfg);
		func = HAL_FUNC_VAL(cfg);

		printk("%s(), [%d]: 0x%x\n", __func__, i, cfg);
		res = gpio_tlmm_config(cfg, GPIO_CFG_ENABLE);
		if(res < 0) {
			printk("Error: Config failed.\n");
		}
		
		if((func == 0) && (dir == 1)) // gpio output
			gpio_set_value(i, output_val);
	}
}

int tlmm_sleep_table_before_set_cfg(unsigned gpio, unsigned cfg)
{
	//BUG_ON(gpio >= TLMM_NUM_GPIO && HAL_GPIO_NUMBER(cfg) != 0xff);
	if (gpio >= TLMM_NUM_GPIO && gpio != 255 && gpio != 256) {
		printk("gpio >= TLMM_NUM_GPIO && gpio != 255 && gpio != 256!\n");
		return -1;
	}

	if(gpio < TLMM_NUM_GPIO)
	{
		sleep_table_before_configs[gpio] = cfg | 0x20000000;
		sleep_table_before_enabled = true;
	}
	else if(gpio == 255)
		sleep_table_before_enabled = true;
	else if(gpio == 256)
		sleep_table_before_enabled = false;

	return 0;
}

int tlmm_sleep_table_before_dump_info(char* buf)
{
	unsigned i;
	char* p = buf;

	p += tlmm_dump_header(p);
	p += sprintf(p, "Format: gpio_num  function  pull  direction  strength [output_value]\n");
	p += sprintf(p, " e.g.  'echo  20 0 D O 2 1'  ==> set pin 20 as GPIO output and the output = 1 \n");
	p += sprintf(p, " e.g.  'echo  20'  ==> disable pin 20's setting \n");
	p += sprintf(p, " e.g.  'echo  255'  ==> enable sleep table's setting \n");
	p += sprintf(p, " e.g.  'echo  256'  ==> disable sleep table's setting \n");

	for(i = 0; i < TLMM_NUM_GPIO; ++i) {
		unsigned cfg;
		//int output_val = 0;

		cfg = sleep_table_before_configs[i];
		//output_val = HAL_OUTPUT_VAL(cfg);
		//cfg &= 0x3fffffff;
		//Complied with GPIO_CFG's version
		//cfg |= 0x20000000;
		//p += tlmm_dump_cfg(p, i, cfg, output_val);
		p += tlmm_sleep_table_dump_cfg(p, i, cfg);
	}
	p+= sprintf(p, "(%d)\n", p - buf); // only for debug reference
	return p - buf;
}
/* yangjq, 20121127, Add sysfs for gpio's debug, END */
