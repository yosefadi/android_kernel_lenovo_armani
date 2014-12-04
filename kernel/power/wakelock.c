/* kernel/power/wakelock.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 * Copyright (c) 2012-2013 The Linux Foundation. All Rights Reserved.
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
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/wakelock.h>
#ifdef CONFIG_WAKELOCK_STAT
#include <linux/proc_fs.h>
#endif
#include "power.h"

#ifdef CONFIG_FAST_BOOT
#include <linux/delay.h>
#endif

#ifdef CONFIG_MSM_SM_EVENT
#include <linux/sm_event_log.h>
#include <linux/sm_event.h>
#endif


enum {
	DEBUG_EXIT_SUSPEND = 1U << 0,
	DEBUG_WAKEUP = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_EXPIRE = 1U << 3,
	DEBUG_WAKE_LOCK = 1U << 4,
	DEBUG_WRITELOG = 1U << 5,	//add by zhuangyt.
};
static int debug_mask = DEBUG_EXIT_SUSPEND | DEBUG_WAKEUP | DEBUG_SUSPEND | DEBUG_WRITELOG;	//modify by zhuangyt.
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define WAKE_LOCK_TYPE_MASK              (0x0f)
#define WAKE_LOCK_INITIALIZED            (1U << 8)
#define WAKE_LOCK_ACTIVE                 (1U << 9)
#define WAKE_LOCK_AUTO_EXPIRE            (1U << 10)
#define WAKE_LOCK_PREVENTING_SUSPEND     (1U << 11)

static DEFINE_SPINLOCK(list_lock);
static LIST_HEAD(inactive_locks);
static struct list_head active_wake_locks[WAKE_LOCK_TYPE_COUNT];
static int current_event_num;
static int suspend_sys_sync_count;
static DEFINE_SPINLOCK(suspend_sys_sync_lock);
static struct workqueue_struct *suspend_sys_sync_work_queue;
static DECLARE_COMPLETION(suspend_sys_sync_comp);
struct workqueue_struct *suspend_work_queue;
struct workqueue_struct *sync_work_queue;
struct wake_lock main_wake_lock;
struct wake_lock sync_wake_lock;
suspend_state_t requested_suspend_state = PM_SUSPEND_MEM;
static struct wake_lock unknown_wakeup;
static struct wake_lock suspend_backoff_lock;

#define SUSPEND_BACKOFF_THRESHOLD	10
#define SUSPEND_BACKOFF_INTERVAL	10000

static unsigned suspend_short_count;

#ifdef CONFIG_WAKELOCK_STAT
static struct wake_lock deleted_wake_locks;
static ktime_t last_sleep_time_update;
static int wait_for_wakeup;

#define SLEEP_LOG

#ifdef SLEEP_LOG
#define WRITE_SLEEP_LOG
/* yangjq, 20121127, Add to print sysfs tcxo_last_sleep */
extern void print_tcxo_last_sleep(void);

//add by zhuangty
struct sleep_log
{
  char time[18];
  long timesec;
  unsigned int log;
  unsigned char wr;
  uint32_t maoints;
  uint32_t rpc_prog;
  uint32_t rpc_proc;
  char smd_port_name[20];

//      31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00


//bit1-0=00 :try to sleep; bit 1-0 = 01 : leave from sleep    ;bit1-0=10:fail to sleep
//bit31-bit24 : return value
};

#define TRY_TO_SLEEP  (0)
#define LEAVE_FORM_SLEEP  (1)
#define FAIL_TO_SLEEP  (2)

#define SLEEP_LOG_LENGTH 80
struct sleep_log array_sleep_log[SLEEP_LOG_LENGTH];
int sleep_log_pointer = 0, sleep_log_count = 0;
int enter_times = 0;

#if 0
extern uint32_t mywr;
extern uint32_t mao_ints;
extern uint32_t rpc_prog;
extern uint32_t rpc_proc;
extern char smd_port_name[20];
#else

#define DEM_WAKEUP_REASON_NONE       0x00000000
#define DEM_WAKEUP_REASON_SMD        0x00000001
#define DEM_WAKEUP_REASON_INT        0x00000002
#define DEM_WAKEUP_REASON_GPIO       0x00000004
#define DEM_WAKEUP_REASON_TIMER      0x00000008
#define DEM_WAKEUP_REASON_ALARM      0x00000010
#define DEM_WAKEUP_REASON_RESET      0x00000020
#define DEM_WAKEUP_REASON_OTHER      0x00000040
#define DEM_WAKEUP_REASON_REMOTE     0x00000080

#define DEM_MAX_PORT_NAME_LEN (20)

struct msm_pm_smem_t {
	uint32_t sleep_time;
	uint32_t irq_mask;
	uint32_t resources_used;
	uint32_t reserved1;

	uint32_t wakeup_reason;
	uint32_t pending_irqs;
	uint32_t rpc_prog;
	uint32_t rpc_proc;
	char     smd_port_name[DEM_MAX_PORT_NAME_LEN];
	uint32_t reserved2;
};
extern struct msm_pm_smem_t msm_pm_smem_data_copy;
#endif //0

static int
sleeplog_read_proc (char *page, char **start, off_t off,
		    int count, int *eof, void *data)
{
  unsigned long irqflags;
  int len = 0;
  char *p = page;
  int pos, i;

  spin_lock_irqsave (&list_lock, irqflags);


  for (i = 0; (i < sleep_log_count) && (i < SLEEP_LOG_LENGTH); i++)
    {
      if (sleep_log_count >= SLEEP_LOG_LENGTH)
	pos = (sleep_log_pointer + i) % SLEEP_LOG_LENGTH;
      else
	pos = i;
      switch (array_sleep_log[pos].log & 0xF)
	{
	case TRY_TO_SLEEP:
	  p += sprintf (p, ">%s\n", array_sleep_log[pos].time);
	  break;
	case LEAVE_FORM_SLEEP:
	  if (array_sleep_log[pos].wr & 0x1)
	    p +=
	      sprintf (p, "<%s(0x%x,0x%x,0x%x,%d,%s)\n",
		       array_sleep_log[pos].time, array_sleep_log[pos].wr,
		       array_sleep_log[pos].maoints,
		       array_sleep_log[pos].rpc_prog,
		       array_sleep_log[pos].rpc_proc,
		       array_sleep_log[pos].smd_port_name);
	  else
	    p +=
	      sprintf (p, "<%s(0x%x,0x%x)\n", array_sleep_log[pos].time,
		       array_sleep_log[pos].wr, array_sleep_log[pos].maoints);
	  break;
	case FAIL_TO_SLEEP:
	  p +=
	    sprintf (p, "^%s(%d)\n", array_sleep_log[pos].time,
		     (char) (array_sleep_log[pos].log >> 24));
	  break;
	}
    }


  spin_unlock_irqrestore (&list_lock, irqflags);

  *start = page + off;

  len = p - page;
  if (len > off)
    len -= off;
  else
    len = 0;

  return len < count ? len : count;

}

static int
sleeplog2_read_proc (char *page, char **start, off_t off,
		     int count, int *eof, void *data)
{
  unsigned long irqflags;
  int len = 0;
  char *p = page;
  int pos, i;

  spin_lock_irqsave (&list_lock, irqflags);


  for (i = 0; (i < sleep_log_count) && (i < SLEEP_LOG_LENGTH); i++)
    {
      if (sleep_log_count >= SLEEP_LOG_LENGTH)
	pos = (sleep_log_pointer + i) % SLEEP_LOG_LENGTH;
      else
	pos = i;
      switch (array_sleep_log[pos].log & 0xF)
	{
	case TRY_TO_SLEEP:
	  p += sprintf (p, ">[%ld]\n", array_sleep_log[pos].timesec);
	  break;
	case LEAVE_FORM_SLEEP:
	  p += sprintf (p, "<[%ld]\n", array_sleep_log[pos].timesec);
	  break;
	case FAIL_TO_SLEEP:
	  p += sprintf (p, "^[%ld]\n", array_sleep_log[pos].timesec);
	  break;
	}
    }


  spin_unlock_irqrestore (&list_lock, irqflags);

  *start = page + off;

  len = p - page;
  if (len > off)
    len -= off;
  else
    len = 0;

  return len < count ? len : count;

}

////////////////////////////////////////add by zhuangyt.

#endif
int get_expired_time(struct wake_lock *lock, ktime_t *expire_time)
{
	struct timespec ts;
	struct timespec kt;
	struct timespec tomono;
	struct timespec delta;
	struct timespec sleep;
	long timeout;

	if (!(lock->flags & WAKE_LOCK_AUTO_EXPIRE))
		return 0;
	get_xtime_and_monotonic_and_sleep_offset(&kt, &tomono, &sleep);
	timeout = lock->expires - jiffies;
	if (timeout > 0)
		return 0;
	jiffies_to_timespec(-timeout, &delta);
	set_normalized_timespec(&ts, kt.tv_sec + tomono.tv_sec - delta.tv_sec,
				kt.tv_nsec + tomono.tv_nsec - delta.tv_nsec);
	*expire_time = timespec_to_ktime(ts);
	return 1;
}


static int print_lock_stat(struct seq_file *m, struct wake_lock *lock)
{
	int lock_count = lock->stat.count;
	int expire_count = lock->stat.expire_count;
	ktime_t active_time = ktime_set(0, 0);
	ktime_t total_time = lock->stat.total_time;
	ktime_t max_time = lock->stat.max_time;

	ktime_t prevent_suspend_time = lock->stat.prevent_suspend_time;
	if (lock->flags & WAKE_LOCK_ACTIVE) {
		ktime_t now, add_time;
		int expired = get_expired_time(lock, &now);
		if (!expired)
			now = ktime_get();
		add_time = ktime_sub(now, lock->stat.last_time);
		lock_count++;
		if (!expired)
			active_time = add_time;
		else
			expire_count++;
		total_time = ktime_add(total_time, add_time);
		if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND)
			prevent_suspend_time = ktime_add(prevent_suspend_time,
					ktime_sub(now, last_sleep_time_update));
		if (add_time.tv64 > max_time.tv64)
			max_time = add_time;
	}

	return seq_printf(m,
		     "\"%s\"\t%d\t%d\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\n",
		     lock->name, lock_count, expire_count,
		     lock->stat.wakeup_count, ktime_to_ns(active_time),
		     ktime_to_ns(total_time),
		     ktime_to_ns(prevent_suspend_time), ktime_to_ns(max_time),
		     ktime_to_ns(lock->stat.last_time));
}

static int wakelock_stats_show(struct seq_file *m, void *unused)
{
	unsigned long irqflags;
	struct wake_lock *lock;
	int ret;
	int type;

	spin_lock_irqsave(&list_lock, irqflags);

	ret = seq_puts(m, "name\tcount\texpire_count\twake_count\tactive_since"
			"\ttotal_time\tsleep_time\tmax_time\tlast_change\n");
	list_for_each_entry(lock, &inactive_locks, link)
		ret = print_lock_stat(m, lock);
	for (type = 0; type < WAKE_LOCK_TYPE_COUNT; type++) {
		list_for_each_entry(lock, &active_wake_locks[type], link)
			ret = print_lock_stat(m, lock);
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
	return 0;
}

static void wake_unlock_stat_locked(struct wake_lock *lock, int expired)
{
	ktime_t duration;
	ktime_t now;
	if (!(lock->flags & WAKE_LOCK_ACTIVE))
		return;
	if (get_expired_time(lock, &now))
		expired = 1;
	else
		now = ktime_get();
	lock->stat.count++;
	if (expired)
		lock->stat.expire_count++;
	duration = ktime_sub(now, lock->stat.last_time);
	lock->stat.total_time = ktime_add(lock->stat.total_time, duration);
	if (ktime_to_ns(duration) > ktime_to_ns(lock->stat.max_time))
		lock->stat.max_time = duration;
	lock->stat.last_time = ktime_get();
	if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND) {
		duration = ktime_sub(now, last_sleep_time_update);
		lock->stat.prevent_suspend_time = ktime_add(
			lock->stat.prevent_suspend_time, duration);
		lock->flags &= ~WAKE_LOCK_PREVENTING_SUSPEND;
	}
}

static void update_sleep_wait_stats_locked(int done)
{
	struct wake_lock *lock;
	ktime_t now, etime, elapsed, add;
	int expired;

	now = ktime_get();
	elapsed = ktime_sub(now, last_sleep_time_update);
	list_for_each_entry(lock, &active_wake_locks[WAKE_LOCK_SUSPEND], link) {
		expired = get_expired_time(lock, &etime);
		if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND) {
			if (expired)
				add = ktime_sub(etime, last_sleep_time_update);
			else
				add = elapsed;
			lock->stat.prevent_suspend_time = ktime_add(
				lock->stat.prevent_suspend_time, add);
		}
		if (done || expired)
			lock->flags &= ~WAKE_LOCK_PREVENTING_SUSPEND;
		else
			lock->flags |= WAKE_LOCK_PREVENTING_SUSPEND;
	}
	last_sleep_time_update = now;
}
#endif

#ifdef CONFIG_MSM_SM_EVENT
void add_active_wakelock_event(void)
{
	struct wake_lock *lock, *n;
	unsigned long irqflags;
	int type;

	spin_lock_irqsave(&list_lock, irqflags);
	for (type = 0; type < WAKE_LOCK_TYPE_COUNT; type++) {
		list_for_each_entry_safe(lock, n, &active_wake_locks[type], link) {
			sm_add_event(SM_WAKELOCK_EVENT | WAKELOCK_EVENT_ON, (uint32_t)(lock->expires - jiffies), 0, (void *)lock->name, strlen(lock->name)+1);
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(add_active_wakelock_event);
#endif

static void expire_wake_lock(struct wake_lock *lock)
{
#ifdef CONFIG_WAKELOCK_STAT
	wake_unlock_stat_locked(lock, 1);
#endif
	lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
	list_del(&lock->link);
	list_add(&lock->link, &inactive_locks);
	if (debug_mask & (DEBUG_WAKE_LOCK | DEBUG_EXPIRE))
		pr_info("expired wake lock %s\n", lock->name);
}

/* Caller must acquire the list_lock spinlock */
static void print_active_locks(int type)
{
	struct wake_lock *lock;
	bool print_expired = true;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	list_for_each_entry(lock, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout > 0)
				pr_info("active wake lock %s, time left %ld\n",
					lock->name, timeout);
			else if (print_expired)
				pr_info("wake lock %s, expired\n", lock->name);
		} else {
			pr_info("active wake lock %s\n", lock->name);
			if (!(debug_mask & DEBUG_EXPIRE))
				print_expired = false;
		}
	}
}

// Copy from print_active_locks
int wakelock_dump_info(char* buf)
{
	char* p = buf;

	struct wake_lock *lock, *n;
	const char* type_txt[] = { "WAKE_LOCK_SUSPEND:", "WAKE_LOCK_IDLE:"};
	int type;
	unsigned long irqflags;
	
	p += sprintf(p, "\nWake Locks:\n");
	for (type = 0; type < WAKE_LOCK_TYPE_COUNT; type++) {
		p += sprintf(p, " %s\n", type_txt[type]);
		spin_lock_irqsave(&list_lock, irqflags);
		list_for_each_entry_safe(lock, n, &active_wake_locks[type], link) {
			if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
				long timeout = lock->expires - jiffies;
#if 0
				if (timeout <= 0)
					p += sprintf(p, "      (inactive)[%s], expired\n", lock->name);
				else
#else // yangjq, 20121012, Only show active wakelocks
				if (timeout > 0)
#endif //0
					p += sprintf(p, "   (active)[%s], time left %ld (jiffies)\n",
						lock->name, timeout);
			} else // active
				p += sprintf(p, "   (active)[%s]\n", lock->name);
		}
		spin_unlock_irqrestore(&list_lock, irqflags);
	}
	
	return p - buf;
}

static long has_wake_lock_locked(int type)
{
	struct wake_lock *lock, *n;
	long max_timeout = 0;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	list_for_each_entry_safe(lock, n, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout <= 0)
				expire_wake_lock(lock);
			else if (timeout > max_timeout)
				max_timeout = timeout;
		} else
			return -1;
	}
	return max_timeout;
}

#ifdef CONFIG_FAST_BOOT
extern bool fake_shut_down;
#endif

long has_wake_lock(int type)
{
	long ret;
	unsigned long irqflags;
#ifdef CONFIG_FAST_BOOT
	if (fake_shut_down)
		return 0;
#endif	
	spin_lock_irqsave(&list_lock, irqflags);
	ret = has_wake_lock_locked(type);
	if (ret && (debug_mask & DEBUG_WAKEUP) && type == WAKE_LOCK_SUSPEND)
		print_active_locks(type);
	spin_unlock_irqrestore(&list_lock, irqflags);
	return ret;
}

#ifdef SLEEP_LOG

char logfilename[60];
struct file *file = NULL;

static int
writelog (void)
{
#ifdef WRITE_SLEEP_LOG

  char buf[256];
  char *p, *p0;
  int i, pos;
  mm_segment_t old_fs;
  p = buf;
  p0 = p;

  
  if (file == NULL)
    file = filp_open (logfilename, O_RDWR | O_APPEND | O_CREAT, 0644);
  if (IS_ERR (file))
    {
      printk ("error occured while opening file %s, exiting...\n",
	      logfilename);
      return 0;
    }

//        sprintf(buf,"%s", "The Messages.");


  if (sleep_log_count > 1)
    {
      for (i = 0; i < 2; i++)
	{
	  if (sleep_log_pointer == 0)
	    pos = SLEEP_LOG_LENGTH - 2 + i;
	  else
	    pos = sleep_log_pointer - 2 + i;
	  switch (array_sleep_log[pos].log & 0xF)
	    {
	    case TRY_TO_SLEEP:
	      p +=
		sprintf (p, ">[%ld]%s\n", array_sleep_log[pos].timesec,
			 array_sleep_log[pos].time);
	      break;
	    case LEAVE_FORM_SLEEP:
	      if (array_sleep_log[pos].wr & 0x1)
		p +=
		  sprintf (p, "<[%ld]%s(0x%x,0x%x,0x%x,%d,%s)\n",
			   array_sleep_log[pos].timesec,
			   array_sleep_log[pos].time, array_sleep_log[pos].wr,
			   array_sleep_log[pos].maoints,
			   array_sleep_log[pos].rpc_prog,
			   array_sleep_log[pos].rpc_proc,
			   array_sleep_log[pos].smd_port_name);
	      else
		p +=
		  sprintf (p, "<[%ld]%s(0x%x,0x%x)\n",
			   array_sleep_log[pos].timesec,
			   array_sleep_log[pos].time, array_sleep_log[pos].wr,
			   array_sleep_log[pos].maoints);
	      break;
	    case FAIL_TO_SLEEP:
	      p +=
		sprintf (p, "^[%ld]%s(%d)\n", array_sleep_log[pos].timesec,
			 array_sleep_log[pos].time,
			 (char) (array_sleep_log[pos].log >> 24));
	      break;
	    }
	}
    }



//       printk("=========log  ::: %s  len=%d\n",buf,p-p0);
  old_fs = get_fs ();
  set_fs (KERNEL_DS);
  file->f_op->write (file, p0, p - p0, &file->f_pos);
  set_fs (old_fs);

  if (file != NULL)
    {
      filp_close (file, NULL);
      file = NULL;
    }
#endif
  return 0;
}
#endif
static void suspend_sys_sync(struct work_struct *work)
{
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("PM: Syncing filesystems...\n");

	sys_sync();

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("sync done.\n");

	spin_lock(&suspend_sys_sync_lock);
	suspend_sys_sync_count--;
	spin_unlock(&suspend_sys_sync_lock);
}
static DECLARE_WORK(suspend_sys_sync_work, suspend_sys_sync);

void suspend_sys_sync_queue(void)
{
	int ret;

	spin_lock(&suspend_sys_sync_lock);
	ret = queue_work(suspend_sys_sync_work_queue, &suspend_sys_sync_work);
	if (ret)
		suspend_sys_sync_count++;
	spin_unlock(&suspend_sys_sync_lock);
}

static bool suspend_sys_sync_abort;
static void suspend_sys_sync_handler(unsigned long);
static DEFINE_TIMER(suspend_sys_sync_timer, suspend_sys_sync_handler, 0, 0);
/* value should be less then half of input event wake lock timeout value
 * which is currently set to 5*HZ (see drivers/input/evdev.c)
 */
#define SUSPEND_SYS_SYNC_TIMEOUT (HZ/4)
static void suspend_sys_sync_handler(unsigned long arg)
{
	if (suspend_sys_sync_count == 0) {
		complete(&suspend_sys_sync_comp);
	} else if (has_wake_lock(WAKE_LOCK_SUSPEND)) {
		suspend_sys_sync_abort = true;
		complete(&suspend_sys_sync_comp);
	} else {
		mod_timer(&suspend_sys_sync_timer, jiffies +
				SUSPEND_SYS_SYNC_TIMEOUT);
	}
}

int suspend_sys_sync_wait(void)
{
	suspend_sys_sync_abort = false;

	if (suspend_sys_sync_count != 0) {
		mod_timer(&suspend_sys_sync_timer, jiffies +
				SUSPEND_SYS_SYNC_TIMEOUT);
		wait_for_completion(&suspend_sys_sync_comp);
	}
	if (suspend_sys_sync_abort) {
		pr_info("suspend aborted....while waiting for sys_sync\n");
		return -EAGAIN;
	}

	return 0;
}

static void suspend_backoff(void)
{
	pr_info("suspend: too many immediate wakeups, back off\n");
	wake_lock_timeout(&suspend_backoff_lock,
			  msecs_to_jiffies(SUSPEND_BACKOFF_INTERVAL));
}

static void suspend(struct work_struct *work)
{
	int ret;
	int entry_event_num;
	struct timespec ts_entry, ts_exit;
#ifdef SLEEP_LOG
	struct timespec ts_;
	struct rtc_time tm_;
#endif	//SLEEP_LOG

	if (has_wake_lock(WAKE_LOCK_SUSPEND)) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("suspend: abort suspend\n");
		return;
	}

	entry_event_num = current_event_num;
	suspend_sys_sync_queue();
	
#ifdef SLEEP_LOG
	/*-------------Simon add --------------*/
	printk("ARM11 try to ENTER sleep mode>>>\n ");
	/*-------------Simon add --------------*/

//add by zhuangyt.
////////////////////////////////////////////////////////////////////////
  getnstimeofday (&ts_);
  rtc_time_to_tm (ts_.tv_sec + 8 * 3600, &tm_);
  sprintf (array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].time,
	   "%d-%02d-%02d %02d:%02d:%02d", tm_.tm_year + 1900,
	   tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour, tm_.tm_min, tm_.tm_sec);
//      (tm_.tm_hour+8)%24, tm_.tm_min, tm_.tm_sec);
	if (strlen (logfilename) < 1)
    {
      sprintf (logfilename,
	       "/data/local/log/aplog/sleeplog%d%02d%02d_%02d%02d%02d.txt",
	       tm_.tm_year + 1900, tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour,
	       tm_.tm_min, tm_.tm_sec);
      printk ("=================Log File name = %s \n", logfilename);
    }

  array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].timesec = ts_.tv_sec;
  array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].log = TRY_TO_SLEEP;
  sleep_log_pointer++;
  sleep_log_count++;
  if (sleep_log_pointer == SLEEP_LOG_LENGTH)
    sleep_log_pointer = 0;
#endif
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("suspend: enter suspend\n");
	getnstimeofday(&ts_entry);
	
if (debug_mask & DEBUG_EXIT_SUSPEND) {
		struct rtc_time tm;
		rtc_time_to_tm(ts_entry.tv_sec, &tm);
		pr_info("suspend: enter suspend, "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts_entry.tv_nsec);
	}	
	
	ret = pm_suspend(requested_suspend_state);
#ifdef SLEEP_LOG
///////////////////////////////////////////////////////////////////add by zhuangyt.

  getnstimeofday (&ts_);
  rtc_time_to_tm (ts_.tv_sec + 8 * 3600, &tm_);
  sprintf (array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].time,
	   "%d-%02d-%02d %02d:%02d:%02d", tm_.tm_year + 1900,
	   tm_.tm_mon + 1, tm_.tm_mday, tm_.tm_hour, tm_.tm_min, tm_.tm_sec);

  array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].timesec = ts_.tv_sec;


  if (ret == 0)
    {
      printk
	("ARM11 Exit from sleep<<<: wr=0x%x mi=0x%x sp=%s rpc_prog=0x%x rpc_proc=%d reserved2=0x%x\n ",
	 msm_pm_smem_data_copy.wakeup_reason, msm_pm_smem_data_copy.pending_irqs, msm_pm_smem_data_copy.smd_port_name, msm_pm_smem_data_copy.rpc_prog, msm_pm_smem_data_copy.rpc_proc, msm_pm_smem_data_copy.reserved2);

      array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].log =
	LEAVE_FORM_SLEEP;
      array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].wr = msm_pm_smem_data_copy.wakeup_reason;

      if ((msm_pm_smem_data_copy.wakeup_reason)&(DEM_WAKEUP_REASON_GPIO))
        array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].maoints =
          msm_pm_smem_data_copy.reserved2;
      else
        array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].maoints =
	  msm_pm_smem_data_copy.pending_irqs;

      array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].rpc_prog =
	msm_pm_smem_data_copy.rpc_prog;
      array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].rpc_proc =
	msm_pm_smem_data_copy.rpc_proc;
      strcpy (array_sleep_log
	      [sleep_log_pointer % SLEEP_LOG_LENGTH].smd_port_name,
	      msm_pm_smem_data_copy.smd_port_name);

      /* yangjq, 20121127, Add to print sysfs tcxo_last_sleep */
      print_tcxo_last_sleep();
    }
  else
    {
      printk
	("ARM11 FAIL to enter sleep^^^\n");

      array_sleep_log[sleep_log_pointer % SLEEP_LOG_LENGTH].log =
	FAIL_TO_SLEEP | (ret << 24);

    }

  sleep_log_pointer++;
  sleep_log_count++;
  if (sleep_log_pointer == SLEEP_LOG_LENGTH)
    sleep_log_pointer = 0;

  if (debug_mask & DEBUG_WRITELOG)
    {
      enter_times++;
      if (enter_times < 5000)
	writelog ();
    }
/////////////////////////////////////////////////////////////////////////////////
#endif
	getnstimeofday(&ts_exit);

	if (debug_mask & DEBUG_EXIT_SUSPEND) {
		struct rtc_time tm;
		rtc_time_to_tm(ts_exit.tv_sec, &tm);
		pr_info("suspend: exit suspend, ret = %d "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", ret,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts_exit.tv_nsec);
	}

	if (ts_exit.tv_sec - ts_entry.tv_sec <= 1) {
		++suspend_short_count;

		if (suspend_short_count == SUSPEND_BACKOFF_THRESHOLD) {
			suspend_backoff();
			suspend_short_count = 0;
		}
	} else {
		suspend_short_count = 0;
	}

	if (current_event_num == entry_event_num) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("suspend: pm_suspend returned with no event\n");
		wake_lock_timeout(&unknown_wakeup, HZ / 2);
	}
}
static DECLARE_WORK(suspend_work, suspend);

static void expire_wake_locks(unsigned long data)
{
	long has_lock;
	unsigned long irqflags;
	if (debug_mask & DEBUG_EXPIRE)
		pr_info("expire_wake_locks: start\n");
	spin_lock_irqsave(&list_lock, irqflags);
	if (debug_mask & DEBUG_SUSPEND)
		print_active_locks(WAKE_LOCK_SUSPEND);
	has_lock = has_wake_lock_locked(WAKE_LOCK_SUSPEND);
	if (debug_mask & DEBUG_EXPIRE)
		pr_info("expire_wake_locks: done, has_lock %ld\n", has_lock);
	if (has_lock == 0)
		queue_work(suspend_work_queue, &suspend_work);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
static DEFINE_TIMER(expire_timer, expire_wake_locks, 0, 0);

static int power_suspend_late(struct device *dev)
{
	int ret = has_wake_lock(WAKE_LOCK_SUSPEND) ? -EAGAIN : 0;
#ifdef CONFIG_WAKELOCK_STAT
	wait_for_wakeup = !ret;
#endif
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("power_suspend_late return %d\n", ret);
	return ret;
}

static struct dev_pm_ops power_driver_pm_ops = {
	.suspend_noirq = power_suspend_late,
};

static struct platform_driver power_driver = {
	.driver.name = "power",
	.driver.pm = &power_driver_pm_ops,
};
static struct platform_device power_device = {
	.name = "power",
};

void wake_lock_init(struct wake_lock *lock, int type, const char *name)
{
	unsigned long irqflags = 0;

	if (name)
		lock->name = name;
	BUG_ON(!lock->name);

	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_lock_init name=%s\n", lock->name);
#ifdef CONFIG_WAKELOCK_STAT
	lock->stat.count = 0;
	lock->stat.expire_count = 0;
	lock->stat.wakeup_count = 0;
	lock->stat.total_time = ktime_set(0, 0);
	lock->stat.prevent_suspend_time = ktime_set(0, 0);
	lock->stat.max_time = ktime_set(0, 0);
	lock->stat.last_time = ktime_set(0, 0);
#endif
	lock->flags = (type & WAKE_LOCK_TYPE_MASK) | WAKE_LOCK_INITIALIZED;

	INIT_LIST_HEAD(&lock->link);
	spin_lock_irqsave(&list_lock, irqflags);
	list_add(&lock->link, &inactive_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_lock_init);

void wake_lock_destroy(struct wake_lock *lock)
{
	unsigned long irqflags;
	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_lock_destroy name=%s\n", lock->name);
	spin_lock_irqsave(&list_lock, irqflags);
	lock->flags &= ~WAKE_LOCK_INITIALIZED;
#ifdef CONFIG_WAKELOCK_STAT
	if (lock->stat.count) {
		deleted_wake_locks.stat.count += lock->stat.count;
		deleted_wake_locks.stat.expire_count += lock->stat.expire_count;
		deleted_wake_locks.stat.total_time =
			ktime_add(deleted_wake_locks.stat.total_time,
				  lock->stat.total_time);
		deleted_wake_locks.stat.prevent_suspend_time =
			ktime_add(deleted_wake_locks.stat.prevent_suspend_time,
				  lock->stat.prevent_suspend_time);
		deleted_wake_locks.stat.max_time =
			ktime_add(deleted_wake_locks.stat.max_time,
				  lock->stat.max_time);
	}
#endif
	list_del(&lock->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_lock_destroy);

static void wake_lock_internal(
	struct wake_lock *lock, long timeout, int has_timeout)
{
	int type;
	unsigned long irqflags;
	long expire_in;

	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & WAKE_LOCK_TYPE_MASK;
	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	BUG_ON(!(lock->flags & WAKE_LOCK_INITIALIZED));
#ifdef CONFIG_WAKELOCK_STAT
	if (type == WAKE_LOCK_SUSPEND && wait_for_wakeup) {
		if (debug_mask & DEBUG_WAKEUP)
			pr_info("wakeup wake lock: %s\n", lock->name);
		wait_for_wakeup = 0;
		lock->stat.wakeup_count++;
	}
	if ((lock->flags & WAKE_LOCK_AUTO_EXPIRE) &&
	    (long)(lock->expires - jiffies) <= 0) {
		wake_unlock_stat_locked(lock, 0);
		lock->stat.last_time = ktime_get();
	}
#endif
	if (!(lock->flags & WAKE_LOCK_ACTIVE)) {
		lock->flags |= WAKE_LOCK_ACTIVE;
#ifdef CONFIG_WAKELOCK_STAT
		lock->stat.last_time = ktime_get();
#endif
	}
	list_del(&lock->link);
	if (has_timeout) {
		if (debug_mask & DEBUG_WAKE_LOCK)
			pr_info("wake_lock: %s, type %d, timeout %ld.%03lu\n",
				lock->name, type, timeout / HZ,
				(timeout % HZ) * MSEC_PER_SEC / HZ);
		lock->expires = jiffies + timeout;
		lock->flags |= WAKE_LOCK_AUTO_EXPIRE;
		list_add_tail(&lock->link, &active_wake_locks[type]);
	} else {
		if (debug_mask & DEBUG_WAKE_LOCK)
			pr_info("wake_lock: %s, type %d\n", lock->name, type);
		lock->expires = LONG_MAX;
		lock->flags &= ~WAKE_LOCK_AUTO_EXPIRE;
		list_add(&lock->link, &active_wake_locks[type]);
	}
	if (type == WAKE_LOCK_SUSPEND) {
		current_event_num++;
#ifdef CONFIG_WAKELOCK_STAT
		if (lock == &main_wake_lock)
			update_sleep_wait_stats_locked(1);
		else if (!wake_lock_active(&main_wake_lock))
			update_sleep_wait_stats_locked(0);
#endif
		if (has_timeout)
			expire_in = has_wake_lock_locked(type);
		else
			expire_in = -1;
		if (expire_in > 0) {
			if (debug_mask & DEBUG_EXPIRE)
				pr_info("wake_lock: %s, start expire timer, "
					"%ld\n", lock->name, expire_in);
			mod_timer(&expire_timer, jiffies + expire_in);
		} else {
			if (del_timer(&expire_timer))
				if (debug_mask & DEBUG_EXPIRE)
					pr_info("wake_lock: %s, stop expire timer\n",
						lock->name);
			if (expire_in == 0)
				queue_work(suspend_work_queue, &suspend_work);
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void wake_lock(struct wake_lock *lock)
{
	wake_lock_internal(lock, 0, 0);
#ifdef CONFIG_MSM_SM_EVENT
	sm_add_event(SM_WAKELOCK_EVENT | WAKELOCK_EVENT_ON, (uint32_t)(lock->expires - jiffies), 0, (void *)lock->name, strlen(lock->name)+1);
#endif
}
EXPORT_SYMBOL(wake_lock);

void wake_lock_timeout(struct wake_lock *lock, long timeout)
{
	wake_lock_internal(lock, timeout, 1);
#ifdef CONFIG_MSM_SM_EVENT
	sm_add_event(SM_WAKELOCK_EVENT | WAKELOCK_EVENT_ON, (uint32_t)(lock->expires - jiffies), 0, (void *)lock->name, strlen(lock->name)+1);
#endif
}
EXPORT_SYMBOL(wake_lock_timeout);

void wake_unlock(struct wake_lock *lock)
{
	int type;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & WAKE_LOCK_TYPE_MASK;
#ifdef CONFIG_WAKELOCK_STAT
	wake_unlock_stat_locked(lock, 0);
#endif
	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_unlock: %s\n", lock->name);
	lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
	list_del(&lock->link);
#ifdef CONFIG_MSM_SM_EVENT
	sm_add_event(SM_WAKELOCK_EVENT | WAKELOCK_EVENT_OFF, (uint32_t)(lock->expires - jiffies), 0, (void *)lock->name, strlen(lock->name)+1);
#endif
	list_add(&lock->link, &inactive_locks);
	if (type == WAKE_LOCK_SUSPEND) {
		long has_lock = has_wake_lock_locked(type);
		if (has_lock > 0) {
			if (debug_mask & DEBUG_EXPIRE)
				pr_info("wake_unlock: %s, start expire timer, "
					"%ld\n", lock->name, has_lock);
			mod_timer(&expire_timer, jiffies + has_lock);
		} else {
			if (del_timer(&expire_timer))
				if (debug_mask & DEBUG_EXPIRE)
					pr_info("wake_unlock: %s, stop expire "
						"timer\n", lock->name);
			if (has_lock == 0)
				queue_work(suspend_work_queue, &suspend_work);
		}
		if (lock == &main_wake_lock) {
			if (debug_mask & DEBUG_SUSPEND)
				print_active_locks(WAKE_LOCK_SUSPEND);
#ifdef CONFIG_WAKELOCK_STAT
			update_sleep_wait_stats_locked(0);
#endif
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_unlock);

int wake_lock_active(struct wake_lock *lock)
{
	return !!(lock->flags & WAKE_LOCK_ACTIVE);
}
EXPORT_SYMBOL(wake_lock_active);

#ifdef CONFIG_FAST_BOOT
void wakelock_force_suspend(void)
{
	static int cnt;

	if (cnt > 0) {
		pr_info("%s: duplicated\n", __func__);
		return;
	}
	cnt++;

	msleep(3000);
	pr_info("%s: fake shut down\n", __func__);
	queue_work(suspend_work_queue, &suspend_work);

	cnt = 0;
}
#endif

static int wakelock_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, wakelock_stats_show, NULL);
}

static const struct file_operations wakelock_stats_fops = {
	.owner = THIS_MODULE,
	.open = wakelock_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init wakelocks_init(void)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(active_wake_locks); i++)
		INIT_LIST_HEAD(&active_wake_locks[i]);

#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_init(&deleted_wake_locks, WAKE_LOCK_SUSPEND,
			"deleted_wake_locks");
#endif
	wake_lock_init(&main_wake_lock, WAKE_LOCK_SUSPEND, "main");
	wake_lock_init(&sync_wake_lock, WAKE_LOCK_SUSPEND, "sync_system");
	wake_lock(&main_wake_lock);
	wake_lock_init(&unknown_wakeup, WAKE_LOCK_SUSPEND, "unknown_wakeups");
	wake_lock_init(&suspend_backoff_lock, WAKE_LOCK_SUSPEND,
		       "suspend_backoff");

	ret = platform_device_register(&power_device);
	if (ret) {
		pr_err("wakelocks_init: platform_device_register failed\n");
		goto err_platform_device_register;
	}
	ret = platform_driver_register(&power_driver);
	if (ret) {
		pr_err("wakelocks_init: platform_driver_register failed\n");
		goto err_platform_driver_register;
	}

	INIT_COMPLETION(suspend_sys_sync_comp);
	suspend_sys_sync_work_queue =
		create_singlethread_workqueue("suspend_sys_sync");
	if (suspend_sys_sync_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_suspend_sys_sync_work_queue;
	}

	suspend_work_queue = create_singlethread_workqueue("suspend");
	if (suspend_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_suspend_work_queue;
	}

sync_work_queue = create_singlethread_workqueue("sync_system_work");
	if (sync_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_sync_work_queue;
	}	
	
#ifdef CONFIG_WAKELOCK_STAT
	proc_create("wakelocks", S_IRUGO, NULL, &wakelock_stats_fops);
#endif
#ifdef SLEEP_LOG
//add by zhuangyt.
  strcpy (logfilename, "");
  create_proc_read_entry ("sleeplog", S_IRUGO, NULL,
			  sleeplog_read_proc, NULL);
  create_proc_read_entry ("sleeplog2", S_IRUGO, NULL,
			  sleeplog2_read_proc, NULL);
#endif
	return 0;

err_sync_work_queue:
	destroy_workqueue(suspend_work_queue);	
	
err_suspend_work_queue:
err_suspend_sys_sync_work_queue:
	platform_driver_unregister(&power_driver);
err_platform_driver_register:
	platform_device_unregister(&power_device);
err_platform_device_register:
	wake_lock_destroy(&suspend_backoff_lock);
	wake_lock_destroy(&unknown_wakeup);
	wake_lock_destroy(&sync_wake_lock);
	wake_lock_destroy(&main_wake_lock);
#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_destroy(&deleted_wake_locks);
#endif
	return ret;
}

static void  __exit wakelocks_exit(void)
{
#ifdef CONFIG_WAKELOCK_STAT
	remove_proc_entry("wakelocks", NULL);
#endif
	destroy_workqueue(suspend_work_queue);
	destroy_workqueue(sync_work_queue);
	destroy_workqueue(suspend_sys_sync_work_queue);
	platform_driver_unregister(&power_driver);
	platform_device_unregister(&power_device);
	wake_lock_destroy(&suspend_backoff_lock);
	wake_lock_destroy(&unknown_wakeup);
	wake_lock_destroy(&sync_wake_lock);
	wake_lock_destroy(&main_wake_lock);
#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_destroy(&deleted_wake_locks);
#endif
}

core_initcall(wakelocks_init);
module_exit(wakelocks_exit);
