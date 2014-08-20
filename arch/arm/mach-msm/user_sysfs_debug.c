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
//#include <linux/err.h>
//#include <linux/list.h>
#include <linux/io.h>
#include <mach/msm_iomap.h>
//#include "gpio_hw.h"
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include "mach/proc_comm.h"

#define private_attr(_name) \
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = __stringify(_name),	\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

/*
assemble     A [address]\n \
go           G [=address] [addresses]\n \
input        I port\n \
load         L [address] [drive] [firstsector] [number]\n \
name         N [pathname] [arglist]\n \
output       O port byte\n \
proceed      P [=address] [number]\n \
register     R [register]\n \
trace        T [=address] [value]\n \
unassemble   U [range]\n \
write        W [address] [drive] [firstsector] [number]\n \
*/

static const char *debug_help = " \
compare      C range address\n \
dump         D [range]\n \
enter        E address [list]\n \
fill         F range list\n \
go           G address [list]\n \
hex          H value1 value2\n \
load         L range file_name\n \
move         M range address\n \
move         P processor <0|1>\n \
quit         Q\n \
search       S range list\n \
write        W range file_name\n";

#define MEM_GET(addr) (mem_proc == 0 ? *(volatile unsigned long *)(addr) : user_proc_mem_get(((unsigned *)(addr))))
#define MEM_SET(addr, val) (mem_proc == 0 ? *(volatile unsigned long *)(addr) = val : user_proc_mem_set(((unsigned *)(addr)), val))
static int mem_proc = 0;

unsigned user_proc_mem_get(unsigned *addr)
{
	int res;
	unsigned data1 = (unsigned)addr;
	unsigned data2 = 0;
	
	printk("%s called.\n", __func__);
	res = msm_proc_comm(PCOM_OEM_DEBUG_MEM_GET, &data1, &data2);

	return data2;
}

int user_proc_mem_set(unsigned *addr, unsigned val)
{
	int res;
	unsigned data1 = (unsigned)addr;
	unsigned data2 = val;
	
	printk("%s called.\n", __func__);
	res = msm_proc_comm(PCOM_OEM_DEBUG_MEM_SET, &data1, &data2);
	printk("%s(), res=%d", __func__, res);
	return res;
}

unsigned user_proc_mem_go(unsigned *addr, unsigned val)
{
	int res;
	unsigned data1 = (unsigned)addr;
	unsigned data2 = val;
	
	printk("%s called.\n", __func__);
	res = msm_proc_comm(PCOM_OEM_DEBUG_MEM_GO, &data1, &data2);
	printk("%s(), res=%d", __func__, res);
	return data2;
}

static ssize_t help_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	p += sprintf(p, debug_help);

	return p - buf;
}

static ssize_t help_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	printk("buf=%s, n=%d\n", buf, n);

	return n;
}

int range_parse(const char *buf, unsigned char **addr0, int *size)
{
	unsigned char *addr1;
	int res = 0;

	printk("buf = %s", buf);
	//p += sprintf(p, "%s", buf);
	res = sscanf(buf, "%lx l%x", (unsigned long *)addr0, size);
	printk("res = %d, addr0 = 0x%lx, size = 0x%x\n", res, (unsigned long)*addr0, *size);
	if (res < 2) {
		res = sscanf(buf, "%lx %lx", (unsigned long *)addr0, (unsigned long *)&addr1);
		printk("res = %d, addr0 = 0x%lx, addr1 = 0x%lx\n", res, (unsigned long)*addr0, (unsigned long)addr1);

		if (res < 2 || *addr0 > addr1)
			*size = 0;
		else
			*size = addr1 - *addr0;

		printk("addr0 = 0x%lx, size = 0x%x\n", (unsigned long)*addr0, *size);
	}

	return res;
}

#define C_BUF_LEN 64
static int c_buf_init = 0;
static char c_buf[C_BUF_LEN];

static ssize_t c_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	unsigned char *addr0 = 0;
	unsigned char *addr1 = 0;
	int size = 0;
	int res;
	int i;
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");
	if (c_buf_init) {
		res = sscanf(c_buf, "%lx l%x %lx",  (unsigned long *)&addr0, &size, (unsigned long *)&addr1);
		if (res < 3)
			p += sprintf(p, "Syntax: c(ompare) source_address(%%x) [l]size|address(%%x) destination_address(%%x)\n");
		else {
			for (i = 0; i < size; i += 4) {
				if (MEM_GET(addr1 + i) != MEM_GET(addr0 + i)) {
					p += sprintf(p, "[%08lx]:", (unsigned long)(addr0 + i));
					p += sprintf(p, " %08lx", MEM_GET(addr0 + i));
					p += sprintf(p, "\t[%08lx]:", (unsigned long)(addr1 + i));
					p += sprintf(p, " %08lx\n", MEM_GET(addr1 + i));
				}
			}
		}
	}

	return p - buf;
}

static ssize_t c_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char *addr0 = 0;
	unsigned char *addr1 = 0;
	unsigned long skip = 0;
	int size = 0;
	int res;

	c_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto c_store_wrong_para;

	res = sscanf(buf, "%lx l%lx %lx", &skip, &skip, (unsigned long *)&addr1);
	if(res < 2)
		res = sscanf(buf, "%lx %lx %lx", &skip, &skip, (unsigned long *)&addr1);
	printk("val = 0x%lx\n", (unsigned long)addr1);

	size = size == 0 ? 4 : size;

	sprintf(c_buf, "%lx l%x %lx", (unsigned long)addr0, size, (unsigned long)addr1);
	c_buf_init = 1;
	goto c_store_exit;

c_store_wrong_para:
	printk(" Syntax: c(ompare) source_address(%%x) [l]size|address(%%x) destination_address(%%x)\n");
	printk("Example: echo 'fa001000 l10 fa001100' > c\n");
	printk("Example: echo 'fa001000 fa001010 fa001100' > c\n");

c_store_exit:
	return n;
}

#define D_BUF_LEN 64
static int d_buf_init = 0;
static char d_buf[D_BUF_LEN];
static char *d_parse(const char *buf, char *p);

static ssize_t d_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");

	if (d_buf_init) {
		p = d_parse(d_buf, p);
	}

	return p - buf;
}

static ssize_t d_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
#if 0
	printk("buf=%s, n=%d\n", buf, n);
	printk("*(buf + %d)=0x%02x\n",  n - 1, *(buf + n - 1));
	if (d_buf_init == 0) {
		d_buf_init = 1;
		//memset(d_buf, 0, D_BUF_LEN);
	}
	memcpy(d_buf, buf, n < D_BUF_LEN ? n : D_BUF_LEN - 1);
	//strncpy(d_buf, buf, n < D_BUF_LEN ? n : D_BUF_LEN - 1);
	*(d_buf + n - 1) = 0;
	//d_parse(buf, NULL);

	return n;
#else //0
	unsigned char *addr0 = 0;
	int size = 0;
	int res;

	d_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto d_store_wrong_para;

	size = size == 0 ? 4 : size;
	sprintf(d_buf, "%lx l%x", (unsigned long)addr0, size);
	d_buf_init = 1;
	goto d_store_exit;

d_store_wrong_para:
	printk(" Syntax: d(ump) address(%%x) [l]size|address(%%x)\n");
	printk("Example: echo 'fa001000 l100' > d\n");
	printk("Example: echo 'fa001000 fa001100' > d\n");

d_store_exit:
	return n;
#endif //0
}

#define E_BUF_LEN 128
static int e_buf_init = 0;
static char e_buf[E_BUF_LEN];

static ssize_t e_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");

	if (e_buf_init) {
		p = d_parse(e_buf, p);
	}

	return p - buf;
}

static ssize_t e_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char *addr0 = 0;
	unsigned long val;
	const char *p;
	int res;
	int i;

	e_buf_init = 0;
	printk("buf=%s, n=%d\n", buf, n);

	res = sscanf(buf, "%lx", (unsigned long *)&addr0);
	printk("res = %d, addr0 = 0x%lx\n", res, (unsigned long)addr0);
	if (res < 1)
		goto e_store_wrong_para;

	/* To ensure "0x_address 1 2 3" and " 0x_address 1 2 3" both work */
	p = skip_spaces(buf);
	p = strnchr(p, n - (p - buf), ' ');
	printk("p = %s\n", p);
	for (i = 0; p;i += 4) {
		p = skip_spaces(p);
		res = sscanf(p, "%lx", &val);
		printk("res = %d, p = %s, val = 0x%lx\n", res, p, val);
		if (res < 1)
			break;
		MEM_SET(addr0 + i, val);
		p = strnchr(p, n - (p - buf), ' ');
	}
	sprintf(e_buf, "%lx l%x", (unsigned long)addr0, i);
	e_buf_init = 1;
	goto e_store_exit;

e_store_wrong_para:
	printk(" Syntax: e(nter) address(%%x) value_list(%%x)\n");
	printk("Example: echo 'fa001000 100' > e\n");
	printk("Example: echo 'fa001000 100 200' > e\n");

e_store_exit:
	return n;
}

#define F_BUF_LEN 64
static int f_buf_init = 0;
static char f_buf[F_BUF_LEN];

static ssize_t f_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");
	if (f_buf_init) {
		p = d_parse(f_buf, p);
	}

	return p - buf;
}

static ssize_t f_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char *addr0 = 0;
	unsigned long val = 0;
	unsigned long skip = 0;
	int size = 0;
	int res;
	int i;

	f_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto f_store_wrong_para;

	res = sscanf(buf, "%lx l%lx %lx", &skip, &skip, &val);
	if(res < 2)
		res = sscanf(buf, "%lx %lx %lx", &skip, &skip, &val);
	printk("val = 0x%lx\n", val);

	size = size == 0 ? 4 : size;
	for (i = 0; i < size; i += 4) {
		MEM_SET(addr0 + i, val);
	}
	sprintf(f_buf, "%lx l%x", (unsigned long)addr0, size);
	f_buf_init = 1;
	goto f_store_exit;

f_store_wrong_para:
	printk(" Syntax: f(ill) address(%%x) [l]size|address(%%x) value(%%x)\n");
	printk("Example: echo 'fa001000 l10 0' > f\n");
	printk("Example: echo 'fa001000 fa001010 ffffffff' > f\n");

f_store_exit:
	return n;
}

#define G_ARG_LEN 16
static int g_argc = 0;
unsigned long g_argv[G_ARG_LEN];

static ssize_t g_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	int result = 0;
	int i;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");

	if (g_argc) {
		p += sprintf(p, "0x%08lx(", g_argv[0]);
		for (i = 1; i < g_argc && i < G_ARG_LEN; i++) {
			if ( i > 1)
				p += sprintf(p, ", ");
			p += sprintf(p, "0x%08lx", g_argv[i]);
		}
		p += sprintf(p, ")");

		if (mem_proc == 0) {
			switch (g_argc) {
			case 1:
				result = ((int (*)(void))g_argv[0]) ();
				break;
			case 2:
				result = ((int (*)(int))g_argv[0]) (g_argv[1]);
				break;
			case 3:
				result = ((int (*)(int, int))g_argv[0]) (g_argv[1], g_argv[2]);
				break;
			case 4:
				result = ((int (*)(int, int, int))g_argv[0]) (g_argv[1], g_argv[2], g_argv[3]);
				break;
			case 5:
				result = ((int (*)(int, int, int, int))g_argv[0]) (g_argv[1], g_argv[2], g_argv[3], g_argv[4]);
				break;
			case 6:
				result = ((int (*)(int, int, int, int, int))g_argv[0]) (g_argv[1], g_argv[2], g_argv[3], g_argv[4], g_argv[5]);
				break;
			case 7:
				result = ((int (*)(int, int, int, int, int, int))g_argv[0]) (g_argv[1], g_argv[2], g_argv[3], g_argv[4], g_argv[5], g_argv[6]);
				break;
			case 8:
				result = ((int (*)(int, int, int, int, int, int, int))g_argv[0]) (g_argv[1], g_argv[2], g_argv[3], g_argv[4], g_argv[5], g_argv[6], g_argv[7]);
				break;
			case 9:
				result = ((int (*)(int, int, int, int, int, int, int, int))g_argv[0]) (g_argv[1], g_argv[2], g_argv[3], g_argv[4], g_argv[5], g_argv[6], g_argv[7], g_argv[8]);
				break;
			default:
				p += sprintf(p, "\nToo many arguments, NOT supported yet!\n");
				return p - buf;
				break;
			}
		} else {
			for (i = 0; i < g_argc && i < G_ARG_LEN; i++) {
				result = user_proc_mem_go(NULL, g_argv[i]);
			}
			result = user_proc_mem_go((unsigned *)g_argv[0], (unsigned)g_argc);
		}
		p += sprintf(p, " = 0x%08x = #%d\n", result, result);
	}

	return p - buf;
}

static ssize_t g_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char *addr0 = 0;
	unsigned long val;
	const char *p;
	int res;
	int i;

	g_argc = 0;
	printk("buf=%s, n=%d\n", buf, n);

	res = sscanf(buf, "%lx", (unsigned long *)&addr0);
	printk("res = %d, addr0 = 0x%lx\n", res, (unsigned long)addr0);
	if (res < 1)
		goto g_store_wrong_para;

	g_argv[0] = (unsigned long)addr0;
	/* To ensure "0x_address 1 2 3" and " 0x_address 1 2 3" both work */
	p = skip_spaces(buf);
	p = strnchr(p, n - (p - buf), ' ');
	printk("p = %s\n", p);
	for (i = 1; p && i < G_ARG_LEN; i++) {
		p = skip_spaces(p);
		res = sscanf(p, "%lx", &val);
		printk("res = %d, p = %s, val = 0x%lx\n", res, p, val);
		if (res < 1)
			break;
		g_argv[i] = val;
		p = strnchr(p, n - (p - buf), ' ');
	}
	g_argc = i;
	goto g_store_exit;

g_store_wrong_para:
	printk(" Syntax: g(o) address(%%x) arg_list(%%x)\n");
	printk("Example: echo 'fa001000 100' > g\n");
	printk("Example: echo 'fa001000 100 200' > g\n");

g_store_exit:
	return n;
}

#define H_BUF_LEN 64
static int h_buf_init = 0;
static char h_buf[H_BUF_LEN];

static ssize_t h_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	unsigned long val0;
	unsigned long val1;
	int res;
	char *p = buf;

	if (h_buf_init) {
		res = sscanf(h_buf, "%lx %lx", &val0, &val1);
		if (res == 1) {
			p += sprintf(p, "%08lx = #%lu\n", val0, val0);
		} else if (res == 2) {
			p += sprintf(p, "%08lx, %08lx\n", val0, val1);
			p += sprintf(p, "%08lx, %08lx\n", val0 + val1, val0 - val1);
		}
	}

	return p - buf;
}

static ssize_t h_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	printk("buf=%s, n=%d\n", buf, n);

	if (h_buf_init == 0) {
		h_buf_init = 1;
	}
	memcpy(h_buf, buf, n < H_BUF_LEN ? n : H_BUF_LEN - 1);
	*(h_buf + n - 1) = 0;

	return n;
}

#define L_BUF_LEN 256
static int l_buf_init = 0;
static char l_buf[L_BUF_LEN];

static int read_from_file(const char *buf, int size, const char *file_name)
{
	struct file *file;
	mm_segment_t old_fs;
	unsigned long val;
	int res;
	int i;

	file = filp_open(file_name, O_RDWR, 0644);
	if (IS_ERR(file)) {
		printk ("File %s open error, file = 0x%08lx!\n", file_name, (unsigned long)file);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	//file->f_op->write(file, buf, size, &file->f_pos);
	for (i = 0; size == 0 || i < size; i += 4) {
		res = file->f_op->read(file, (char *)&val, 4, &file->f_pos);
		printk ("res = %d, 0x%08lx\n", res, val);
		if (res < 4)
			break;
		MEM_SET(buf + i, val);
	}
	set_fs(old_fs);

	filp_close(file, NULL);

	return i;
}

static ssize_t l_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	unsigned char file_name[L_BUF_LEN];
	unsigned char *addr0 = 0;
	int size = 0;
	int res;
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");
	if (l_buf_init) {
		res = sscanf(l_buf, "%lx l%x %s",  (unsigned long *)&addr0, &size, file_name);
		//p += sprintf(p, "w %08lx l%x %s\n", (unsigned long)addr0, size, file_name);
		if (res < 3)
			p += sprintf(p, "Syntax: (load) address(%%x) [l]size|address(%%x) file_name(%%x)\n");
		else {
			res = read_from_file(addr0, size, file_name);
			if (res < 0)
				p += sprintf(p, "File %s open error!\n", file_name);
			else
				p += sprintf(p, "Read %d bytes to 0x%08lx from file %s\n", res, (unsigned long)addr0, file_name);
		}
	}

	return p - buf;
}

static ssize_t l_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char file_name[L_BUF_LEN];
	unsigned char *addr0 = 0;
	unsigned long skip = 0;
	int size = 0;
	int res;

	l_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto l_store_wrong_para;

	res = sscanf(buf, "%lx l%lx %s", &skip, &skip, file_name);
	if(res < 2)
		res = sscanf(buf, "%lx %lx %s", &skip, &skip, file_name);
	printk("file_name = %s\n", file_name);

	//size = size == 0 ? 4 : size;

	sprintf(l_buf, "%lx l%x %s", (unsigned long)addr0, size, file_name);
	l_buf_init = 1;
	goto l_store_exit;

l_store_wrong_para:
	printk(" Syntax: (load) address(%%x) [l]size|address(%%x) file_name(%%x)\n");
	printk("Example: echo 'fa001000 l10 /data/1.bin' > l\n");
	printk("Example: echo 'fa001000 fa001010 /data/1.bin' > l\n");

l_store_exit:
	return n;
}

#define M_BUF_LEN 64
static int m_buf_init = 0;
static char m_buf[M_BUF_LEN];

static ssize_t m_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");
	if (m_buf_init) {
		p = d_parse(m_buf, p);
	}

	return p - buf;
}

static ssize_t m_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char *addr0 = 0;
	unsigned char *addr1 = 0;
	unsigned long skip = 0;
	int size = 0;
	int res;
	int i;

	m_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto m_store_wrong_para;

	res = sscanf(buf, "%lx l%lx %lx", &skip, &skip, (unsigned long *)&addr1);
	if(res < 2)
		res = sscanf(buf, "%lx %lx %lx", &skip, &skip, (unsigned long *)&addr1);
	printk("val = 0x%lx\n", (unsigned long)addr1);

	size = size == 0 ? 4 : size;
	for (i = 0; i < size; i += 4) {
		MEM_SET(addr1 + i, MEM_GET(addr0 + i));
	}
	sprintf(m_buf, "%lx l%x", (unsigned long)addr1, size);
	m_buf_init = 1;
	goto m_store_exit;

m_store_wrong_para:
	printk(" Syntax: m(ove) source_address(%%x) [l]size|address(%%x) destination_address(%%x)\n");
	printk("Example: echo 'fa001000 l10 fa001100' > m\n");
	printk("Example: echo 'fa001000 fa001010 fa001100' > m\n");

m_store_exit:
	return n;
}

static ssize_t p_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	char *p = buf;
	p += sprintf(p, "%d\n", mem_proc);

	return p - buf;
}

static ssize_t p_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	int res;

	printk("buf=%s, n=%d\n", buf, n);

	res = sscanf(buf, "%d", &mem_proc);
	if (res != 1 || mem_proc > 1) {
		printk(" Syntax: p(rocessor) processor_id(%%d)\n");
		printk("Example: echo '0' > p\n");
		printk("Example: echo '1' > p\n");

		mem_proc = 0;
	}

	return n;
}

#define S_BUF_LEN 64
static int s_buf_init = 0;
static char s_buf[S_BUF_LEN];

static ssize_t s_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	unsigned char *addr0 = 0;
	unsigned long val = 0;
	int size = 0;
	int res;
	int i;
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");
	if (s_buf_init) {
		res = sscanf(s_buf, "%lx l%x %lx",  (unsigned long *)&addr0, &size, &val);
		//p += sprintf(p, "%08lx l%x %08lx\n", (unsigned long)addr0, size, val);
		if (res < 3)
			p += sprintf(p, "Syntax: s(earch) source_address(%%x) [l]size|address(%%x) value(%%x)\n");
		else {
			for (i = 0; i < size; i += 4) {
				if (MEM_GET(addr0 + i) == val) {
					p += sprintf(p, "[%08lx]\n", (unsigned long)(addr0 + i));
				}
			}
		}
	}

	return p - buf;
}

static ssize_t s_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char *addr0 = 0;
	unsigned long val = 0;
	unsigned long skip = 0;
	int size = 0;
	int res;

	s_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto s_store_wrong_para;

	res = sscanf(buf, "%lx l%lx %lx", &skip, &skip, &val);
	if(res < 2)
		res = sscanf(buf, "%lx %lx %lx", &skip, &skip, &val);
	printk("val = 0x%lx\n", val);

	size = size == 0 ? 4 : size;

	sprintf(s_buf, "%lx l%x %lx", (unsigned long)addr0, size, val);
	s_buf_init = 1;
	goto s_store_exit;

s_store_wrong_para:
	printk(" Syntax: s(earch) source_address(%%x) [l]size|address(%%x) value(%%x)\n");
	printk("Example: echo 'fa001000 l10 12345678' > s\n");
	printk("Example: echo 'fa001000 fa001010 12345678' > s\n");

s_store_exit:
	return n;
}

#define W_BUF_LEN 256
static int w_buf_init = 0;
static char w_buf[W_BUF_LEN];

static int write_to_file(const char *buf, int size, const char *file_name)
{
	struct file *file;
	mm_segment_t old_fs;
	unsigned long val;
	int i;

	/* Remove O_APPEND to overwrite the old file */
	file = filp_open(file_name, O_RDWR| O_CREAT, 0644);
	if (IS_ERR(file)) {
		printk ("File %s open error, file = 0x%08lx!\n", file_name, (unsigned long)file);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	//file->f_op->write(file, buf, size, &file->f_pos);
	for (i = 0; i < size; i += 4) {
		val = MEM_GET(buf + i);
		file->f_op->write(file, (char *)&val, 4, &file->f_pos);
	}
	set_fs(old_fs);

	filp_close(file, NULL);

	return i;
}

static ssize_t w_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	unsigned char file_name[W_BUF_LEN];
	unsigned char *addr0 = 0;
	int size = 0;
	int res;
	char *p = buf;
	//p += sprintf(p, "d(ump) address(%%x) [l]size|address(%%x)\n");
	if (w_buf_init) {
		res = sscanf(w_buf, "%lx l%x %s",  (unsigned long *)&addr0, &size, file_name);
		//p += sprintf(p, "w %08lx l%x %s\n", (unsigned long)addr0, size, file_name);
		if (res < 3)
			p += sprintf(p, "Syntax: w(rite) address(%%x) [l]size|address(%%x) file_name(%%x)\n");
		else {
			res = write_to_file(addr0, size, file_name);
			if (res < 0)
				p += sprintf(p, "File %s open error!\n", file_name);
			else
				p += sprintf(p, "Writed %d bytes from 0x%08lx to file %s\n", res, (unsigned long)addr0, file_name);
		}
	}

	return p - buf;
}

static ssize_t w_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t n)
{
	unsigned char file_name[W_BUF_LEN];
	unsigned char *addr0 = 0;
	unsigned long skip = 0;
	int size = 0;
	int res;

	w_buf_init = 0;
	res = range_parse(buf, (unsigned char **)&addr0, &size);

	if(res < 1 || size < 0)
		goto w_store_wrong_para;

	res = sscanf(buf, "%lx l%lx %s", &skip, &skip, file_name);
	if(res < 2)
		res = sscanf(buf, "%lx %lx %s", &skip, &skip, file_name);
	printk("file_name = %s\n", file_name);

	size = size == 0 ? 4 : size;

	sprintf(w_buf, "%lx l%x %s", (unsigned long)addr0, size, file_name);
	w_buf_init = 1;
	goto w_store_exit;

w_store_wrong_para:
	printk(" Syntax: w(rite) address(%%x) [l]size|address(%%x) file_name(%%x)\n");
	printk("Example: echo 'fa001000 l10 /data/1.bin' > w\n");
	printk("Example: echo 'fa001000 fa001010 /data/1.bin' > w\n");

w_store_exit:
	return n;
}

static char char_buf[64];

int sprintf_u32(char *p_char, unsigned long val)
{
	int i;
	char c;
	int pos;

	pos = 0;
	for (i = 0; i < 4; i++) {
		c = val >> (i * 8);
		//printk("i=%d, val=%lx, c=%02x, pos=%d\n", i, val, c, pos);
		pos += sprintf(p_char + pos, "%c", isprint(c) ? c : ' ');
	}

	return pos;
}

static char *d_parse(const char *buf, char *p)
{
	unsigned char *addr0 = 0;
	unsigned long val = 0;
	int size = 0;
	int res;
	int i;
	char *p_char;
	char *p_from = p;

	printk("buf = %s\n", buf);
	res = sscanf(buf, "%lx l%x", (unsigned long *)&addr0, &size);
	printk("res = %d, addr0 = 0x%lx, size = 0x%x\n", res, (unsigned long)addr0, size);
	if (res < 2)
		p += sprintf(p, "Syntax: d(ump) address(%%x) [l]size|address(%%x)\n");
	else {
		p_char = char_buf;
		for (i = 0; i < size && (p - p_from) < PAGE_SIZE; i += 4) {
			if (i % 32 == 0) {
				//printk("\n[%08lx]:", (unsigned long)(addr0 + i));
				if (p_char > char_buf) {
					*p_char = 0;
					p += sprintf(p, " ; %s\n", char_buf);
					p_char = char_buf;
				}

				p += sprintf(p, "[%08lx]:", (unsigned long)(addr0 + i));
			}
			//printk(" %08lx", (unsigned long)*(unsigned long *)(addr0 + i));
#if 0
			val = __raw_readl(addr0 + i);
			printk(" %08lx(__raw_readl)", val);
			val = __raw_readl(IOMEM(addr0) + i);
			printk(" %08lx(__raw_readl)(IOMEM)", val);
			val = *(volatile unsigned long *)(IOMEM(addr0) + i);
			printk(" %08lx(IOMEM)", val);
#endif //0
			val = MEM_GET(addr0 + i);
			//printk(" %08lx", val);
			p += sprintf(p, " %08lx", val);
			//p_char += sprintf(p_char, "%c%c%c%c", (char)val, (char)(val >> 8), (char)(val >> 16), (char)(val >> 24));
			p_char += sprintf_u32(p_char, val);
		}
		if (size > 0 && size <= 4)
			p += sprintf(p, " = #%lu", val);
		//printk("\n");
		if (p_char > char_buf) {
			*p_char = 0;
			p += sprintf(p, " ; %s\n", char_buf);
		}
	}

	return p;
}

private_attr(help);
private_attr(c);
private_attr(d);
private_attr(e);
private_attr(f);
private_attr(g);
private_attr(h);
private_attr(l);
private_attr(m);
private_attr(p);
private_attr(s);
private_attr(w);

static struct attribute *g_debug_attr[] = {
	&help_attr.attr,
	&c_attr.attr,
	&d_attr.attr,
	&e_attr.attr,
	&f_attr.attr,
	&g_attr.attr,
	&h_attr.attr,
	&l_attr.attr,
	&m_attr.attr,
	&p_attr.attr,
	&s_attr.attr,
	&w_attr.attr,
	NULL,
};

static struct attribute_group debug_attr_group = {
	.attrs = g_debug_attr,
};

static struct kobject *sysfs_debug_kobj;

static int __init sysfs_debug_init(void)
{
	int result;

	printk("%s(), %d\n", __func__, __LINE__);

	sysfs_debug_kobj = kobject_create_and_add("debug", NULL);
	if (!sysfs_debug_kobj)
		return -ENOMEM;

	result = sysfs_create_group(sysfs_debug_kobj, &debug_attr_group);
	printk("%s(), %d, result=%d\n", __func__, __LINE__, result);

	return result;
}

static void __exit sysfs_debug_exit(void)
{
	printk("%s(), %d\n", __func__, __LINE__);
	sysfs_remove_group(sysfs_debug_kobj, &debug_attr_group);

	kobject_put(sysfs_debug_kobj);
}

module_init(sysfs_debug_init);
module_exit(sysfs_debug_exit);
