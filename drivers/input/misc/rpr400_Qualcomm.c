/*
 *  rpr400.c - Linux kernel modules for ambient light + proximity sensor
 */
#include <linux/debugfs.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>

#define RPR400_DRV_NAME	"rpr400"
#define DRIVER_VERSION		"1.0"

//#define RPR400_INT		81	//don't use because I use client->irq instead

//#define APDS990x_PS_DETECTION_THRESHOLD		600
//#define APDS990x_PS_HSYTERESIS_THRESHOLD	500

//#define APDS990x_ALS_THRESHOLD_HSYTERESIS	20	/* 20 = 20% */

/*************** Definitions ******************/
/* RPR400 REGSTER */
#define REG_SYSTEMCONTROL         (0x40)
#define REG_MODECONTROL           (0x41)
#define REG_ALSPSCONTROL          (0x42)
#define REG_PERSISTENCE           (0x43)
#define REG_PSDATA                (0x44)
#define REG_PSDATA_LSB            (0x44)
#define REG_PSDATA_MBS            (0x45)
#define REG_ALSDATA0              (0x46)
#define REG_ALSDATA0_LSB          (0x46)
#define REG_ALSDATA0_MBS          (0x47)
#define REG_ALSDATA1              (0x48)
#define REG_ALSDATA1_LSB          (0x48)
#define REG_ALSDATA1_MBS          (0x49)
#define REG_INTERRUPT             (0x4A)
#define REG_PSTH                  (0x4B)
#define REG_PSTH_LSB              (0x4B)
#define REG_PSTH_MBS              (0x4C)
#define REG_PSTL                  (0x4D)
#define REG_PSTL_LSB              (0x4D)
#define REG_PSTL_MBS              (0x4E)
#define REG_ALSDATA0TH            (0x4F)
#define REG_ALSDATA0TH_LSB       (0x4F)
#define REG_ALSDATA0TH_MBS       (0x50)
#define REG_ALSDATA0TL            (0x51)
#define REG_ALSDATA0TL_LSB        (0x51)
#define REG_ALSDATA0TL_MBS        (0x52)

/* SETTINGS */
#define CUT_UNIT         (10)		//Andy 2012.6.6, previous is (1000), but I don't think the customer need als data so accurate.
#define CALC_ERROR        (0x80000000)
#define MASK_LONG         (0xFFFFFFFF)
#define MASK_CHAR         (0xFF)
#define BOTH_STANDBY	(0)
#define ALS100MS	(0x5)
#define PS100MS		(0x3)
#define BOTH100MS	(0x6)
#define LEDCURRENT_025MA    (0)
#define LEDCURRENT_050MA    (1)
#define LEDCURRENT_100MA    (2)
#define LEDCURRENT_200MA    (3)
#define ALSGAIN_X1X1        (0x0 << 2)
#define ALSGAIN_X1X2        (0x1 << 2)
#define ALSGAIN_X2X2        (0x5 << 2)
#define ALSGAIN_X64X64      (0xA << 2)
#define ALSGAIN_X128X64     (0xE << 2)
#define ALSGAIN_X128X128    (0xF << 2)
#define NORMAL_MODE         (0 << 4)
#define LOW_NOISE_MODE      (1 << 4)
#define PS_INT_MASK		(1 << 7)
#define ALS_INT_MASK	(1 << 6)
#define PS_THH_ONLY         (0 << 4)
#define PS_THH_BOTH_HYS     (1 << 4)
#define PS_THH_BOTH_OUTSIDE (2 << 4)
#define POLA_ACTIVEL        (0 << 3)
#define POLA_INACTIVEL      (1 << 3)
#define OUTPUT_ANYTIME      (0 << 2)
#define OUTPUT_LATCH        (1 << 2)
#define MODE_NONUSE         (0)
#define MODE_PROXIMITY      (1)
#define MODE_ILLUMINANCE    (2)
#define MODE_BOTH           (3)

/* RANGE */
#define REG_PSTH_MAX     (0xFFF)
#define REG_PSTL_MAX     (0xFFF)
#define PERSISTENCE_MAX     (0x0F)
#define GEN_READ_MAX 	(19)
#define REG_ALSPSCTL_MAX    (0x3F)
#define REG_INTERRUPT_MAX   (0x2F)

/* INIT PARAM */
#define PS_ALS_SET_MODE_CONTROL   (NORMAL_MODE)
#define PS_ALS_SET_ALSPS_CONTROL  (LEDCURRENT_100MA | ALSGAIN_X1X1)	//Default value
#define PS_ALS_SET_INTR_PERSIST   (3)
#define PS_ALS_SET_INTR           (PS_THH_BOTH_OUTSIDE| POLA_ACTIVEL | OUTPUT_LATCH | MODE_PROXIMITY)
#define PS_ALS_SET_PS_TH          (150)//(80)	//(50)
#define PS_ALS_SET_PS_TL          (100)//(10)	//Changed from (0x000) to avoid triggered by background noise. 
#define PS_ALS_SET_ALS_TH         (2000) 	//Compare with ALS_DATA0. ALS_Data equals 0.192*ALS_DATA0 roughly. 
#define PS_ALS_SET_ALS_TL         (0x0000)
#define PS_ALS_SET_MIN_DELAY_TIME (125)

/* OTHER */
#ifdef _ALS_BIG_ENDIAN_
#define CONVERT_TO_BE(value) ((((value) >> 8) & 0xFF) | (((value) << 8) & 0xFF00))
#else
#define CONVERT_TO_BE(value) (value)
#endif

#define _FUNCTION_USED_	(0)
#define RPR400_GPIO_IRQ 17


static void long_long_divider(unsigned long long data, unsigned long base_divier, unsigned long *answer, unsigned long long *overplus);

/*************** Structs ******************/
struct  wake_lock ps_lock;
struct ALS_PS_DATA {
	struct i2c_client *client;
	struct mutex update_lock;
	struct delayed_work	dwork;	/* for PS interrupt */
	struct delayed_work    als_dwork; /* for ALS polling */
	struct input_dev *input_dev_als;
	struct input_dev *input_dev_ps;

	unsigned int enable;	//used to indicate working mode
	unsigned int als_time;	//als measurement time
	unsigned int ps_time;	//ps measurement time
	unsigned int ps_th_l;	//ps threshold low
	unsigned int ps_th_h;	//ps threshold high
	unsigned int als_th_l;	//als threshold low, not used in the program
	unsigned int als_th_h;	//als threshold high, not used in the program
	unsigned int persistence;	//persistence
	unsigned int control;	//als_ps_control

	/* register value */
	unsigned short als_data0_raw;	//register value of data0
	unsigned short als_data1_raw;	//register value of data1
	unsigned short ps_data_raw;	//register value of ps

	/* control flag from HAL */
	unsigned int enable_ps_sensor;
	unsigned int enable_als_sensor;

	/* PS parameters */

	unsigned int ps_direction;		/* 0 = near-to-far; 1 = far-to-near */
	unsigned int ps_data;			/* to store PS data */
	float ps_distance;
	unsigned int ledcurrent;	//led current

	/* ALS parameters */
	unsigned int als_data;			/* to store ALS data */
	unsigned int als_level;
	unsigned int gain0;	//als data0 gain
	unsigned int gain1;	//als data1 gain
	unsigned int als_poll_delay;	// the unit is ms I think. needed for als polling

    char *type;
};

typedef struct {
    unsigned long long data;
    unsigned long long data0;
    unsigned long long data1;
    unsigned char      gain_data0;
    unsigned char      gain_data1;
    unsigned long      dev_unit;
    unsigned char      als_time;
    unsigned short     als_data0;
    unsigned short     als_data1;
} CALC_DATA;

typedef struct {
    unsigned long positive;
    unsigned long decimal;
} CALC_ANS;

/*************** Function Claims ******************/

/*************** Global Data ******************/
/* parameter for als calculation */
#define COEFFICIENT               (4)
const unsigned long data0_coefficient[COEFFICIENT] = {192, 141, 127, 117};
const unsigned long data1_coefficient[COEFFICIENT] = {316, 108,  86,  74};
const unsigned long judge_coefficient[COEFFICIENT] = { 29,  65,  85, 158};

/* mode control table */
#define MODE_CTL_FACTOR (16)
static const struct MCTL_TABLE {
    short ALS;
    short PS;
} mode_table[MODE_CTL_FACTOR] = {
    {  0,   0},   /*  0 */
    {  0,  10},   /*  1 */
    {  0,  40},   /*  2 */
    {  0, 100},   /*  3 */
    {  0, 400},   /*  4 */
    {100,   0},   /*  5 */
    {100, 100},   /*  6 */
    {100, 400},   /*  7 */
    {400,   0},   /*  8 */
    {400, 100},   /*  9 */
    {400,   0},   /* 10 */
    {400, 400},   /* 11 */
    {  0,   0},   /* 12 */
    {  0,   0},   /* 13 */
    {  0,   0},   /* 14 */
    {  0,   0}    /* 15 */
};

/* gain table */
#define GAIN_FACTOR (16)
static const struct GAIN_TABLE {
    char DATA0;
    char DATA1;
} gain_table[GAIN_FACTOR] = {
    {  1,   1},   /*  0 */
    {  2,   1},   /*  1 */
    {  0,   0},   /*  2 */
    {  0,   0},   /*  3 */
    {  0,   0},   /*  4 */
    {  2,   2},   /*  5 */
    {  0,   0},   /*  6 */
    {  0,   0},   /*  7 */
    {  0,   0},   /*  8 */
    {  0,   0},   /*  9 */
    { 64,  64},   /* 10 */
    {  0,   0},   /* 11 */
    {  0,   0},   /* 12 */
    {  0,   0},   /* 13 */
    {128,  64},   /* 14 */
    {128, 128}    /* 15 */
};

/*************** Functions ******************/
/******************************************************************************
 * NAME       : rpr400_set_enable
 * FUNCTION   : set measurement time according to enable
 * REMARKS    : this function will overwrite the work mode. if it is called improperly, 
 *			   you may shutdown some part unexpectedly. please check als_ps->enable first.
 *			   I assume it is run in normal mode. If you want low noise mode, the code should be modified.
 *****************************************************************************/
static int rpr400_set_enable(struct i2c_client *client, int enable)
{
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	int ret;

	if(enable > 0x0B)
	{
		printk(KERN_ERR "%s: RPR400 enable error: invalid measurement time setting.\n", __func__);
		return -EINVAL;
	}
	else
	{
		mutex_lock(&als_ps->update_lock);
		ret = i2c_smbus_write_byte_data(client, REG_MODECONTROL, enable);
		mutex_unlock(&als_ps->update_lock);

		als_ps->enable = enable;
		als_ps->als_time = mode_table[(enable & 0xF)].ALS;
		als_ps->ps_time = mode_table[(enable & 0xF)].PS;

		return ret;
	}
}

//#if _FUNCTION_USED_	//masked because they are claimed but not used, which will cause error when compilling. These functions provides some methods.
static int rpr400_set_ps_threshold_low(struct i2c_client *client, int threshold)
{
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	int ret = 0;
	unsigned short write_data;
	int pilt;

    /* check whether the parameter is valid */
	if(threshold > REG_PSTL_MAX)
	{
		printk(KERN_ERR "RPR400 set ps threshold low error: exceed maximum possible value.\n");
		return -EINVAL;
	}
	if(threshold > als_ps->ps_th_h)
	{
		printk(KERN_ERR "RPR400 set ps threshold low error: higher than threshold high.\n");
		return -EINVAL;
	}
	
    /* write register to RPR400 via i2c */
	write_data = CONVERT_TO_BE(threshold);
	mutex_lock(&als_ps->update_lock);
	pilt = i2c_smbus_read_word_data(client, REG_PSTL);
	if(pilt != 0){
		ret = i2c_smbus_write_i2c_block_data(client, REG_PSTL, sizeof(write_data), (unsigned char *)&write_data);
	}
	mutex_unlock(&als_ps->update_lock);
	if(ret < 0)
	{
		printk(KERN_ERR "RPR400 set ps threshold low error: write i2c fail.\n");
		return ret;
	}
	als_ps->ps_th_l = threshold;	//Update the value after successful i2c write to avoid difference. 
		
	return 0;
}

static int rpr400_set_ps_threshold_high(struct i2c_client *client, int threshold)
{
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	int ret = 0;
	unsigned short write_data;
	int piht;

    /* check whether the parameter is valid */
	if(threshold > REG_PSTH_MAX)
	{
		printk(KERN_ERR "RPR400 set ps threshold high error: exceed maximum possible value.\n");
		return -EINVAL;
	}
	if(threshold < als_ps->ps_th_l)
	{
		printk(KERN_ERR "RPR400 set ps threshold high error: lower than threshold low.\n");
		return -EINVAL;
	}
	
    /* write register to RPR400 via i2c */
	write_data = CONVERT_TO_BE(threshold);
	mutex_lock(&als_ps->update_lock);
	piht = i2c_smbus_read_word_data(client, REG_PSTH);
	if(piht != 4095){
		ret = i2c_smbus_write_i2c_block_data(client, REG_PSTH, sizeof(write_data), (unsigned char *)&write_data);
	}
	mutex_unlock(&als_ps->update_lock);
	if(ret < 0)
	{
		printk(KERN_ERR "RPR400 set ps threshold high error: write i2c fail.\n");
		return ret;
	}
	als_ps->ps_th_h = threshold;	//Update the value after successful i2c write to avoid difference. 
		
	return 0;
}

#if _FUNCTION_USED_	//masked because they are claimed but not used, which will cause error when compilling. These functions provides some methods.
static int rpr400_set_persist(struct i2c_client *client, int persist)
{
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	int ret;
	
    /* check whether the parameter is valid */
	if(persist > PERSISTENCE_MAX)
	{
		printk(KERN_ERR "RPR400 set persistence error: exceed maximum possible value.\n");
		return -EINVAL;
	}
	
    /* write register to RPR400 via i2c */
	mutex_lock(&als_ps->update_lock);
	ret = i2c_smbus_write_byte_data(client, REG_PERSISTENCE, persist);
	mutex_unlock(&als_ps->update_lock);
	if(ret < 0)
	{
		printk(KERN_ERR "RPR400 set persistence error: write i2c fail.\n");
		return ret;
	}
	als_ps->persistence = persist;	//Update the value after successful i2c write to avoid difference. 

	return 0;
}
#endif

static int rpr400_set_control(struct i2c_client *client, int control)
{
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	int ret;
	unsigned char gain, led_current;
	
	if(control > REG_ALSPSCTL_MAX)
	{
		printk(KERN_ERR "RPR400 set control error: exceed maximum possible value.\n");
		return -EINVAL;
	}
	
	gain = (control & 0x3C) >> 2;	//gain setting values
	led_current = control & 0x03;		//led current setting value

	if(!((gain == ALSGAIN_X1X1) || (gain == ALSGAIN_X1X2) || (gain == ALSGAIN_X2X2) || (gain == ALSGAIN_X64X64)
		|| (gain == ALSGAIN_X128X64) || (gain == ALSGAIN_X128X128)))
	{
		printk(KERN_ERR "RPR400 set control error: invalid gain setting. \n");
		return -EINVAL;
	}
	
	mutex_lock(&als_ps->update_lock);
	ret = i2c_smbus_write_byte_data(client, REG_ALSPSCONTROL, control);
	mutex_unlock(&als_ps->update_lock);
	if(ret < 0)
	{
		printk(KERN_ERR "RPR400 set control error: write i2c fail.\n");
		return ret;
	}
	als_ps->control = control;
	als_ps->gain0 = gain_table[gain].DATA0;
	als_ps->gain1 = gain_table[gain].DATA1;
	als_ps->ledcurrent = led_current;

	return ret;
}
//#endif

/******************************************************************************
 * NAME       : calc_rohm_als_data
 * FUNCTION   : calculate illuminance data for RPR400
 * REMARKS    : final_data is 1000 times, which is defined as CUT_UNIT, of the actual lux value
 *****************************************************************************/
static int calc_rohm_als_data(unsigned short data0, unsigned short data1, unsigned char gain0, unsigned char gain1, unsigned char time)
{
#define DECIMAL_BIT      (15)
#define JUDGE_FIXED_COEF (100)
#define MAX_OUTRANGE     (11357)
#define MAXRANGE_NMODE   (0xFFFF)
#define MAXSET_CASE      (4)

	int                final_data;
	CALC_DATA          calc_data;
	CALC_ANS           calc_ans;
	unsigned long      calc_judge;
	unsigned char      set_case;
	unsigned long      div_answer;
	unsigned long long div_overplus;
	unsigned long long overplus;
	unsigned long      max_range;

	/* set the value of measured als data */
	calc_data.als_data0  = data0;
	calc_data.als_data1  = data1;
	calc_data.gain_data0 = gain0;

	/* set max range */
	if (calc_data.gain_data0 == 0) 
	{
		/* issue error value when gain is 0 */
		return (CALC_ERROR);
	}
	else
	{
		max_range = MAX_OUTRANGE / calc_data.gain_data0;
	}
	
	/* calculate data */
	if (calc_data.als_data0 == MAXRANGE_NMODE) 
	{
		calc_ans.positive = max_range;
		calc_ans.decimal  = 0;
	} 
	else 
	{
		/* get the value which is measured from power table */
		calc_data.als_time = 100;

		calc_judge = calc_data.als_data1 * JUDGE_FIXED_COEF;
		if (calc_judge < (calc_data.als_data0 * judge_coefficient[0])) 
		{
			set_case = 0;
		} 
		else if (calc_judge < (calc_data.als_data0 * judge_coefficient[1]))
		{
			set_case = 1;
		} 
		else if (calc_judge < (calc_data.als_data0 * judge_coefficient[2])) 
		{
			set_case = 2;
		}
		else if (calc_judge < (calc_data.als_data0 * judge_coefficient[3])) 
		{
			 set_case = 3;
		} 
		else
		{
			set_case = MAXSET_CASE;
		}
		calc_ans.positive = 0;
		if (set_case >= MAXSET_CASE) 
		{
			calc_ans.decimal = 0;	//which means that lux output is 0
		}
		else
		{
			calc_data.gain_data1 = gain1;
			calc_data.data0      = (unsigned long long )(data0_coefficient[set_case] * calc_data.als_data0) * calc_data.gain_data1;
			calc_data.data1      = (unsigned long long )(data1_coefficient[set_case] * calc_data.als_data1) * calc_data.gain_data0;
			calc_data.data       = (calc_data.data0 - calc_data.data1);
			calc_data.dev_unit   = calc_data.gain_data0 * calc_data.gain_data1 * calc_data.als_time * 10;

			/* calculate a positive number */
			div_answer   = 0;
			div_overplus = 0;
			long_long_divider(calc_data.data, calc_data.dev_unit, &div_answer, &div_overplus);
			calc_ans.positive = div_answer;
			/* calculate a decimal number */
			calc_ans.decimal = 0;
			overplus         = div_overplus;
			if (calc_ans.positive < max_range)
			{
				if (overplus != 0)
				{
					overplus     = overplus << DECIMAL_BIT;
					div_answer   = 0;
					div_overplus = 0;
					long_long_divider(overplus, calc_data.dev_unit, &div_answer, &div_overplus);
					calc_ans.decimal = div_answer;
				}
			}

			else
			{
				calc_ans.positive = max_range;
			}
		}
	}
	
	final_data = calc_ans.positive * CUT_UNIT + ((calc_ans.decimal * CUT_UNIT) >> DECIMAL_BIT);
					
	return (final_data);

#undef DECIMAL_BIT
#undef JUDGE_FIXED_COEF
#undef MAX_OUTRANGE
#undef MAXRANGE_NMODE
#undef MAXSET_CASE
}

/******************************************************************************
 * NAME       : long_long_divider
 * FUNCTION   : calc divider of unsigned long long int or unsgined long
 * REMARKS    :
 *****************************************************************************/
static void long_long_divider(unsigned long long data, unsigned long base_divier, unsigned long *answer, unsigned long long *overplus)
{
    volatile unsigned long long divier;
    volatile unsigned long      unit_sft;

    *answer = 0;
    divier = base_divier;
    if (data > MASK_LONG) {
        unit_sft = 0;
        while ((data >> 1) > divier) {
            unit_sft++;
            divier = divier << 1;
        }
        while (data > base_divier) {
            if (data > divier) {
                *answer += 1L << unit_sft;
                data    -= divier;
            }
            unit_sft--;
            divier = divier >> 1;
        }
        *overplus = data;
    } else {
        *answer = (unsigned long)(data & MASK_LONG) / base_divier;
        /* calculate over plus and shift 16bit */
        *overplus = (unsigned long long)(data - (*answer * base_divier));
    }
}

/******************************************************************************
 * NAME       : calculate_ps_data
 * FUNCTION   : calculate proximity data for RPR400
 * REMARKS    : 12 bit output
 *****************************************************************************/
static int calc_rohm_ps_data(unsigned short ps_reg_data)
{
    return (ps_reg_data & 0xFFF);
}

static unsigned int rpr400_als_data_to_level(unsigned int als_data)
{
#if 0
#define ALS_LEVEL_NUM 15
	int als_level[ALS_LEVEL_NUM]  = { 0, 50, 100, 150,  200,  250,  300, 350, 400,  450,  550, 650, 750, 900, 1100};
	int als_value[ALS_LEVEL_NUM]  = { 0, 50, 100, 150,  200,  250,  300, 350, 400,  450,  550, 650, 750, 900, 1100};
    	unsigned char idx;

	for(idx = 0; idx < ALS_LEVEL_NUM; idx ++)
	{
		if(als_data < als_value[idx])
		{
			break;
		}
	}
	if(idx >= ALS_LEVEL_NUM)
	{
		printk(KERN_ERR "RPR400 als data to level: exceed range.\n");
		idx = ALS_LEVEL_NUM - 1;
	}
	
	return als_level[idx];
#undef ALS_LEVEL_NUM
#endif
	return als_data;
}

static void rpr400_reschedule_work(struct ALS_PS_DATA *als_ps,
					  unsigned long delay)
{
	unsigned long flags;

	spin_lock_irqsave(&als_ps->update_lock.wait_lock, flags);

	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	__cancel_delayed_work(&als_ps->dwork);
	schedule_delayed_work(&als_ps->dwork, delay);

	spin_unlock_irqrestore(&als_ps->update_lock.wait_lock, flags);
}

/* ALS polling routine */
static void rpr400_als_polling_work_handler(struct work_struct *work)
{
	struct ALS_PS_DATA *als_ps = container_of(work, struct ALS_PS_DATA, als_dwork.work);
	struct i2c_client *client=als_ps->client;
	int tmp = 0;
	
	tmp = i2c_smbus_read_word_data(client, REG_ALSDATA0);
	if(tmp < 0)
	{
		printk(KERN_ERR "RPR400 als polling handler error: i2c read data0 fail. \n");
		//return tmp;
	}
	als_ps->als_data0_raw = (unsigned short)tmp;
	tmp = i2c_smbus_read_word_data(client, REG_ALSDATA1);
	if(tmp < 0)
	{
		printk(KERN_ERR "RPR400 als polling handler error: i2c read data1 fail. \n");
		//return tmp;
	}
	als_ps->als_data1_raw = (unsigned short)tmp;

// Theorically it is not necesssary to do so, but I just want to avoid any potential error.  -- Andy 2012.6.6
	tmp = i2c_smbus_read_byte_data(client, REG_ALSPSCONTROL);
	if(tmp < 0)
	{
		printk(KERN_ERR "RPR400 als polling handler error: i2c read gain fail. \n");
		//return tmp;
	}
	tmp = (tmp & 0x3C) >> 2;
	als_ps->gain0 = gain_table[tmp].DATA0;
	als_ps->gain1 = gain_table[tmp].DATA1;	
	
	tmp = i2c_smbus_read_byte_data(client, REG_MODECONTROL);
	if(tmp < 0)
	{
		printk(KERN_ERR "RPR400 als polling handler error: i2c read time fail. \n");
		//return tmp;
	}
	tmp = tmp & 0x7;
	als_ps->als_time = mode_table[tmp].ALS;	
	if(als_ps->als_time != 0){
	    als_ps->als_data = calc_rohm_als_data(als_ps->als_data0_raw, als_ps->als_data1_raw, als_ps->gain0, als_ps->gain1, als_ps->als_time);

	    als_ps->als_level = rpr400_als_data_to_level(als_ps->als_data);
	    //printk(KERN_INFO "RPR400 als report: data0 = %d, data1 = %d, gain0 = %d, gain1 = %d, time = %d, lux = %d, level = %d.\n", als_ps->als_data0_raw, 
		    //als_ps->als_data1_raw, als_ps->gain0, als_ps->gain1, als_ps->als_time, als_ps->als_data, als_ps->als_level);
	    if(als_ps->als_data != CALC_ERROR){
	        input_report_abs(als_ps->input_dev_als, ABS_MISC, als_ps->als_level); // report als data. maybe necessary to convert to lux level
	        input_sync(als_ps->input_dev_als);
	        als_ps->input_dev_als->absinfo[ABS_MISC].value = -1;
	    }
	}
	i2c_smbus_read_word_data(client, REG_PSDATA); //clear psensor interrupt by reading pdata.
	schedule_delayed_work(&als_ps->als_dwork, msecs_to_jiffies(als_ps->als_poll_delay));	// restart timer
}


/* PS interrupt routine */
static void rpr400_ps_int_work_handler(struct work_struct *work)
{
	struct ALS_PS_DATA *als_ps = container_of((struct delayed_work *)work, struct ALS_PS_DATA, dwork);
	struct i2c_client *client=als_ps->client;
	int tmp;

	tmp =  i2c_smbus_read_byte_data(client, REG_INTERRUPT);
	if(tmp < 0)
	{
		printk(KERN_ERR "RPR400 ps int handler error: i2c read interrupt status fail. \n");
		//return tmp;
	}
	if(tmp & PS_INT_MASK)	//Interrupt is caused by PS
	{
		tmp = i2c_smbus_read_byte_data(client, REG_ALSPSCONTROL);
		if(tmp < 0)
		{
			printk(KERN_ERR "RPR400 ps int handler error: i2c read led current fail. \n");
			//return tmp;
		}
		als_ps->ledcurrent = tmp & 0x3;
		
		tmp = i2c_smbus_read_word_data(client, REG_PSDATA);
		if(tmp < 0)
		{
			printk(KERN_ERR "RPR400 ps int handler error: i2c read ps data fail. \n");
			//return tmp;
		}
		als_ps->ps_data_raw = (unsigned short)tmp;
		if(als_ps->ps_time != 0) {
		    als_ps->ps_data = calc_rohm_ps_data(als_ps->ps_data_raw);

		    if(als_ps->ps_data > als_ps->ps_th_h)
		    {
			    als_ps->ps_direction = 0;
		    }
		    else if(als_ps->ps_data < als_ps->ps_th_l)
		    {
			    als_ps->ps_direction = 1;
		    }

		    printk(KERN_INFO "RPR400 ps report: raw_data = %d, data = %d, direction = %d. \n",
				    als_ps->ps_data_raw, als_ps->ps_data, als_ps->ps_direction);

		    input_report_abs(als_ps->input_dev_ps, ABS_DISTANCE, als_ps->ps_direction);
		    input_sync(als_ps->input_dev_ps);
            //modify threshold
            if(als_ps->ps_direction == 0) {
                i2c_smbus_write_word_data(client, REG_PSTL, als_ps->ps_th_l);
                i2c_smbus_write_word_data(client, REG_PSTH, 4095);
	        }
            else if(als_ps->ps_direction == 1) {
                i2c_smbus_write_word_data(client, REG_PSTL, 0);
                i2c_smbus_write_word_data(client, REG_PSTH, als_ps->ps_th_h);
	        }
	    }
	}
	else
	{
		printk(KERN_ERR "RPR400 ps int handler error: unknown interrupt source.\n");
	}
	
	enable_irq(client->irq);
}

/* assume this is ISR */
static irqreturn_t rpr400_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	disable_irq_nosync(client->irq);
	printk("RPR400_interrupt\n");
	wake_lock_timeout(&ps_lock, HZ / 2);
	rpr400_reschedule_work(als_ps, 0);

	return IRQ_HANDLED;
}

/*************** SysFS Support ******************/
static ssize_t rpr400_show_enable_ps_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	
	return sprintf(buf, "%d\n", als_ps->enable_ps_sensor);
}

static ssize_t rpr400_store_enable_ps_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
// 	unsigned long flags;

	printk(KERN_INFO "RPR400 enable PS sensor -> %ld \n", val);
		
	if ((val != 0) && (val != 1)) 
	{
		printk("%s:store unvalid value=%ld\n", __func__, val);
		return count;
	}
	
	if(val == 1) 
	{
		//turn on p sensor
		////wake_lock(&ps_lock);
		if (als_ps->enable_ps_sensor == 0) 
		{
			als_ps->enable_ps_sensor = 1;
		
			if (als_ps->enable_als_sensor == 0) 
			{
				rpr400_set_enable(client, PS100MS);	//PS on and ALS off
			}
			else
			{
				rpr400_set_enable(client, BOTH100MS);	//PS on and ALS on
			}
		}
	} 
	else 
	{
		if(als_ps->enable_ps_sensor == 1)
		{
			als_ps->enable_ps_sensor = 0;
			if (als_ps->enable_als_sensor == 0) 
			{
				rpr400_set_enable(client, BOTH_STANDBY);	//PS off and ALS off
			}
			else 
			{
				rpr400_set_enable(client, ALS100MS);	//PS off and ALS on
			}
			////wake_unlock(&ps_lock);
		}
	}
		
	return count;
}

static ssize_t rpr400_show_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	
	return sprintf(buf, "%d\n", als_ps->enable_als_sensor);
}

static ssize_t rpr400_store_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
 	unsigned long flags;
	
	printk(KERN_INFO "%s: RPR400 enable ALS sensor -> %ld\n", __func__, val);

	if ((val != 0) && (val != 1))
	{
		return count;
	}
	
	if(val == 1)
	{
        //turn on light  sensor
		if (als_ps->enable_als_sensor==0)
		{
			als_ps->enable_als_sensor = 1;
			
			if(als_ps->enable_ps_sensor == 1)
			{
				rpr400_set_enable(client, BOTH100MS);
			}
			else
			{
				rpr400_set_enable(client, ALS100MS);
			}
		}
		spin_lock_irqsave(&als_ps->update_lock.wait_lock, flags); 
		__cancel_delayed_work(&als_ps->als_dwork);
		schedule_delayed_work(&als_ps->als_dwork, msecs_to_jiffies(als_ps->als_poll_delay));	// 125ms
		spin_unlock_irqrestore(&als_ps->update_lock.wait_lock, flags);	
	}
	else
	{
		if(als_ps->enable_als_sensor == 1)
		{
			als_ps->enable_als_sensor = 0;

			if(als_ps->enable_ps_sensor == 1)
			{
				rpr400_set_enable(client, PS100MS);
			}
			else
			{
				rpr400_set_enable(client, BOTH_STANDBY);
			}
		}
		
		spin_lock_irqsave(&als_ps->update_lock.wait_lock, flags); 
		__cancel_delayed_work(&als_ps->als_dwork);
		spin_unlock_irqrestore(&als_ps->update_lock.wait_lock, flags);	
	}
	
	return count;
}

static ssize_t rpr400_show_als_poll_delay(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	
	return sprintf(buf, "%d\n", als_ps->als_poll_delay*1000);	// return in micro-second
}

static ssize_t rpr400_store_als_poll_delay(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
//	int ret;
//	int poll_delay = 0;
 	unsigned long flags;
	
	if (val < PS_ALS_SET_MIN_DELAY_TIME * 1000)
		val = PS_ALS_SET_MIN_DELAY_TIME * 1000;	// previously, it is 5ms. I don't know why. 
												//Considering that our ALS measurement time is 100ms at least, I don't think 5ms makes sense.
	
	als_ps->als_poll_delay = val /1000;	// convert us => ms
	
	/* we need this polling timer routine for sunlight canellation */
	spin_lock_irqsave(&als_ps->update_lock.wait_lock, flags); 
		
	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	__cancel_delayed_work(&als_ps->als_dwork);
	schedule_delayed_work(&als_ps->als_dwork, msecs_to_jiffies(als_ps->als_poll_delay));	// 125ms
			
	spin_unlock_irqrestore(&als_ps->update_lock.wait_lock, flags);	
	
	return count;
}

static ssize_t rpr400_show_pdata(struct device *dev,
                struct device_attribute *attr, char *buf){
    struct i2c_client *client = to_i2c_client(dev);
    int pdata;
    
    pdata = i2c_smbus_read_word_data(client, REG_PSDATA);

    return sprintf(buf, "%d\n", pdata);
}

static ssize_t rpr400_show_piht(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct ALS_PS_DATA *data = i2c_get_clientdata(client);

        return sprintf(buf, "%d\n", data->ps_th_h);
}

static ssize_t rpr400_store_piht(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        unsigned long val = simple_strtoul(buf, NULL, 10);
        int ret;
        
        ret = rpr400_set_ps_threshold_high(client, val);
        if (ret < 0)
            return ret;

        return count;
}

static ssize_t rpr400_show_pilt(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct ALS_PS_DATA *data = i2c_get_clientdata(client);

        return sprintf(buf, "%d\n", data->ps_th_l);
}

static ssize_t rpr400_store_pilt(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        unsigned long val = simple_strtoul(buf, NULL, 10);
        int ret;
        
        ret = rpr400_set_ps_threshold_low(client, val);
        if (ret < 0)
            return ret;

        return count;
}

static ssize_t rpr400_show_control(struct device *dev,
                            struct device_attribute *attr, char *buf)
{
        struct i2c_client *client = to_i2c_client(dev);
        struct ALS_PS_DATA *data = i2c_get_clientdata(client);

        return sprintf(buf, "%d\n", data->control);
}

static ssize_t rpr400_store_control(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
        struct i2c_client *client = to_i2c_client(dev);
        unsigned long val = simple_strtoul(buf, NULL, 10);
        int ret;
        
        ret = rpr400_set_control(client, val);
        if (ret < 0)
            return ret;

        return count;
}

static ssize_t rpr400_show_type(struct device *dev,
                struct device_attribute *attr, char *buf){
    struct i2c_client *client = to_i2c_client(dev);
    struct ALS_PS_DATA *data = i2c_get_clientdata(client);

    return sprintf(buf, "%s\n", data->type);
}

static DEVICE_ATTR(als_poll_delay,  S_IRUGO | S_IWUGO,
				    rpr400_show_als_poll_delay, rpr400_store_als_poll_delay);

static DEVICE_ATTR(enable_als_sensor,  S_IRUGO | S_IWUGO ,
				  rpr400_show_enable_als_sensor, rpr400_store_enable_als_sensor);

static DEVICE_ATTR(enable_ps_sensor,  S_IRUGO | S_IWUGO ,
				   rpr400_show_enable_ps_sensor, rpr400_store_enable_ps_sensor);

static DEVICE_ATTR(pdata,  S_IRUGO, rpr400_show_pdata, NULL);

static DEVICE_ATTR(piht, S_IWUSR | S_IRUGO, 
                    rpr400_show_piht, rpr400_store_piht);

static DEVICE_ATTR(pilt, S_IWUSR | S_IRUGO,
                    rpr400_show_pilt, rpr400_store_pilt);

static DEVICE_ATTR(control, S_IWUSR | S_IRUGO,
                    rpr400_show_control, rpr400_store_control);

static DEVICE_ATTR(type,  S_IRUGO, rpr400_show_type, NULL);

static struct attribute *rpr400_attributes[] = {
	&dev_attr_enable_ps_sensor.attr,
	&dev_attr_enable_als_sensor.attr,
	&dev_attr_als_poll_delay.attr,
    &dev_attr_pdata.attr,
    &dev_attr_piht.attr,
    &dev_attr_pilt.attr,
    &dev_attr_control.attr,
    &dev_attr_type.attr,
	NULL
};

static const struct attribute_group rpr400_attr_group = {
	.attrs = rpr400_attributes,
};

/*************** Initialze Functions ******************/
static int rpr400_init_client(struct i2c_client *client)
{
    struct init_func_write_data {
        unsigned char mode_ctl;
        unsigned char psals_ctl;
        unsigned char persist;
        unsigned char reserved0;
        unsigned char reserved1;
        unsigned char reserved2;
        unsigned char reserved3;
        unsigned char reserved4;
        unsigned char reserved5;
        unsigned char intr;
        unsigned char psth_hl;
        unsigned char psth_hh;
        unsigned char psth_ll;
        unsigned char psth_lh;
        unsigned char alsth_hl;
        unsigned char alsth_hh;
        unsigned char alsth_ll;
        unsigned char alsth_lh;
    } write_data;
    int result;
    unsigned char gain;

    unsigned char mode_ctl    = PS_ALS_SET_MODE_CONTROL;
    unsigned char psals_ctl   = PS_ALS_SET_ALSPS_CONTROL;
    unsigned char persist     = PS_ALS_SET_INTR_PERSIST;
    unsigned char intr        = PS_ALS_SET_INTR;
    unsigned short psth_upper  = PS_ALS_SET_PS_TH;
    unsigned short psth_low    = PS_ALS_SET_PS_TL;
    unsigned short alsth_upper = PS_ALS_SET_ALS_TH;
    unsigned short alsth_low   = PS_ALS_SET_ALS_TL;

    struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	
    /* execute software reset */
    result =  i2c_smbus_write_byte_data(client, REG_SYSTEMCONTROL, 0xC0);	//soft-reset
    if (result != 0) {
        return (result);
    }

    /* not check parameters are psth_upper, psth_low, alsth_upper, alsth_low */
    /* check the PS orerating mode */
    if ((NORMAL_MODE != mode_ctl) && (LOW_NOISE_MODE != mode_ctl)) {
	printk(KERN_ERR "RPR400 init client: invalid parameter.\n");
        return (-EINVAL);
    }
    /* check the parameter of ps and als control */
    if (psals_ctl > REG_ALSPSCTL_MAX) {
	printk(KERN_ERR "RPR400 init client: invalid parameter.\n");
        return (-EINVAL);
    }
    /* check the parameter of ps interrupt persistence */
    if (persist > PERSISTENCE_MAX) {
	printk(KERN_ERR "RPR400 init client: invalid parameter.\n");
        return (-EINVAL);
    }
    /* check the parameter of interrupt */
    if (intr > REG_INTERRUPT_MAX) {
	printk(KERN_ERR "RPR400 init client: invalid parameter.\n");
        return (-EINVAL);
    }
    /* check the parameter of proximity sensor threshold high */
    if (psth_upper > REG_PSTH_MAX) {
	printk(KERN_ERR "RPR400 init client: invalid parameter.\n");
        return (-EINVAL);
    }
    /* check the parameter of proximity sensor threshold low */
    if (psth_low > REG_PSTL_MAX) {
	printk(KERN_ERR "RPR400 init client: invalid parameter.\n");
        return (-EINVAL);
    }
    write_data.mode_ctl  = mode_ctl;
    write_data.psals_ctl = psals_ctl;
    write_data.persist   = persist;
    write_data.reserved0 = 0;
    write_data.reserved1 = 0;
    write_data.reserved2 = 0;
    write_data.reserved3 = 0;
    write_data.reserved4 = 0;
    write_data.reserved5 = 0;
    write_data.intr      = intr;
    write_data.psth_hl   = CONVERT_TO_BE(psth_upper) & MASK_CHAR;
    write_data.psth_hh   = CONVERT_TO_BE(psth_upper) >> 8;
    write_data.psth_ll   = CONVERT_TO_BE(psth_low) & MASK_CHAR;
    write_data.psth_lh   = CONVERT_TO_BE(psth_low) >> 8;
    write_data.alsth_hl  = CONVERT_TO_BE(alsth_upper) & MASK_CHAR;
    write_data.alsth_hh  = CONVERT_TO_BE(alsth_upper) >> 8;
    write_data.alsth_ll  = CONVERT_TO_BE(alsth_low) & MASK_CHAR;
    write_data.alsth_lh  = CONVERT_TO_BE(alsth_low) >> 8;
    result               = i2c_smbus_write_i2c_block_data(client, REG_MODECONTROL, sizeof(write_data), (unsigned char *)&write_data);

	if(result < 0)
	{
		printk(KERN_ERR "RPR400 init client error: i2c write fail. \n");
		return result;
	}

	gain = (psals_ctl & 0x3C) >> 2;	//gain setting values
	
	als_ps->enable = mode_ctl;
	als_ps->als_time = mode_table[(mode_ctl & 0xF)].ALS;
	als_ps->ps_time = mode_table[(mode_ctl & 0xF)].PS;
	als_ps->persistence = persist;
	als_ps->ps_th_l = psth_low;
	als_ps->ps_th_h = psth_upper;
	als_ps->als_th_l = alsth_low;
	als_ps->als_th_h = alsth_upper;
	als_ps->control = psals_ctl;
	als_ps->gain0 = gain_table[gain].DATA0;
	als_ps->gain1 = gain_table[gain].DATA1;
	als_ps->ledcurrent = psals_ctl & 0x03;
    i2c_smbus_write_word_data(client, REG_PSTL, 0);
    i2c_smbus_write_word_data(client, REG_PSTH, als_ps->ps_th_h);
	
    return (result);
}

/*********** I2C init/probing/exit functions ****************/

static struct i2c_driver rpr400_driver;
static int __devinit rpr400_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
#define ROHM_PSALS_ALSMAX (65535)
#define ROHM_PSALS_PSMAX  (4095)

	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ALS_PS_DATA *als_ps;
	
	int err = 0;
    int dev_id;
	printk("%s E\n",__func__);

	wake_lock_init(&ps_lock,WAKE_LOCK_SUSPEND,"ps wakelock");

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	als_ps = kzalloc(sizeof(struct ALS_PS_DATA), GFP_KERNEL);
	if (!als_ps) {
		err = -ENOMEM;
		goto exit;
	}
	als_ps->client = client;
	i2c_set_clientdata(client, als_ps);

    //check whether is rpr400 or apds9930
    dev_id = i2c_smbus_read_byte_data(client, REG_SYSTEMCONTROL); 
    if(dev_id != 0x09){
        kfree(als_ps);
        return -1;
    }
    als_ps->type = "1";
    printk("%s: id(0x%x), this is rpr400!\n", __func__, dev_id & 0x3f);
    if(client->irq<=0)
	    client->irq = MSM_GPIO_TO_INT(RPR400_GPIO_IRQ);
	mutex_init(&als_ps->update_lock);

	INIT_DELAYED_WORK(&als_ps->dwork, rpr400_ps_int_work_handler);
	INIT_DELAYED_WORK(&als_ps->als_dwork, rpr400_als_polling_work_handler); 
	
	printk("%s interrupt is hooked\n", __func__);

	/* Initialize the RPR400 chip */
	err = rpr400_init_client(client);
	if (err)
		goto exit_kfree;


	als_ps->als_poll_delay = PS_ALS_SET_MIN_DELAY_TIME;	
//	err = rpr400_set_enable(client, BOTH100MS);
	/* Arrange als work after 125ms */
/*	
	als_ps->als_poll_delay = PS_ALS_SET_MIN_DELAY_TIME;
	spin_lock_irqsave(&als_ps->update_lock.wait_lock, flags); 
	__cancel_delayed_work(&als_ps->als_dwork);
	schedule_delayed_work(&als_ps->als_dwork, msecs_to_jiffies(als_ps->als_poll_delay));	// 125ms
	spin_unlock_irqrestore(&als_ps->update_lock.wait_lock, flags);
*/

	/* Register to Input Device */
	als_ps->input_dev_als = input_allocate_device();
	if (!als_ps->input_dev_als) {
		err = -ENOMEM;
		printk("%s: Failed to allocate input device als\n", __func__);
		goto exit_free_irq;
	}

	als_ps->input_dev_ps = input_allocate_device();
	if (!als_ps->input_dev_ps) {
		err = -ENOMEM;
		printk("%s: Failed to allocate input device ps\n", __func__);
		goto exit_free_dev_als;
	}
	
	set_bit(EV_ABS, als_ps->input_dev_als->evbit);
	set_bit(EV_ABS, als_ps->input_dev_ps->evbit);

	input_set_abs_params(als_ps->input_dev_als, ABS_MISC, 0, ROHM_PSALS_ALSMAX, 0, 0);
	input_set_abs_params(als_ps->input_dev_ps, ABS_DISTANCE, 0, ROHM_PSALS_PSMAX, 0, 0);

	als_ps->input_dev_als->name = "light";
	als_ps->input_dev_ps->name = "proximity";
    als_ps->input_dev_ps->absinfo[ABS_DISTANCE].value = -1;

	err = input_register_device(als_ps->input_dev_als);
	if (err) {
		err = -ENOMEM;
		printk("%s: Unable to register input device als: %s\n",
		       __func__, als_ps->input_dev_als->name);
		goto exit_free_dev_ps;
	}

	err = input_register_device(als_ps->input_dev_ps);
	if (err) {
		err = -ENOMEM;
		printk("%s: Unable to register input device ps: %s\n",
		       __func__, als_ps->input_dev_ps->name);
		goto exit_unregister_dev_als;
	}

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &rpr400_attr_group);
	if (err)
	{
		printk("%s sysfs_create_groupX\n", __func__);
		goto exit_unregister_dev_ps;
	}

	printk("%s support ver. %s enabled\n", __func__, DRIVER_VERSION);
	if (request_irq(client->irq, rpr400_interrupt, IRQF_DISABLED|IRQ_TYPE_EDGE_FALLING,
		RPR400_DRV_NAME, (void *)client)) {
		printk("%s Could not allocate RPR400_INT !\n", __func__);
	
		goto exit_kfree;
	}

	irq_set_irq_wake(client->irq, 1);

	printk(KERN_INFO "%s: RPR400 probe: INT No. %d", __func__, client->irq);
	return 0;


exit_unregister_dev_ps:
	input_unregister_device(als_ps->input_dev_ps);	
exit_unregister_dev_als:
	input_unregister_device(als_ps->input_dev_als);
exit_free_dev_ps:
	input_free_device(als_ps->input_dev_ps);
exit_free_dev_als:
	input_free_device(als_ps->input_dev_als);
exit_free_irq:
	free_irq(client->irq, client);	
exit_kfree:
	kfree(als_ps);
exit:
	return err;

#undef ROHM_PSALS_ALSMAX
#undef ROHM_PSALS_PSMAX
}

static int __devexit rpr400_remove(struct i2c_client *client)
{
	struct ALS_PS_DATA *als_ps = i2c_get_clientdata(client);
	
	input_unregister_device(als_ps->input_dev_als);
	input_unregister_device(als_ps->input_dev_ps);
	
	input_free_device(als_ps->input_dev_als);
	input_free_device(als_ps->input_dev_ps);

	free_irq(client->irq, client);

	sysfs_remove_group(&client->dev.kobj, &rpr400_attr_group);

	/* Power down the device */
	rpr400_set_enable(client, 0);

	kfree(als_ps);

	return 0;
}

static int rpr400_suspend(struct i2c_client *client, pm_message_t mesg)
{
	printk("%s\n", __func__);

	disable_irq(client->irq);
	//wake_unlock(&ps_lock);
	////return rpr400_set_enable(client, 0);
	return 0;
}

static int rpr400_resume(struct i2c_client *client)
{
	printk("%s \n", __func__);
	//wake_lock(&ps_lock);
	enable_irq(client->irq);
	////return rpr400_set_enable(client, PS100MS);
	return 0;
}


MODULE_DEVICE_TABLE(i2c, rpr400_id);

static const struct i2c_device_id rpr400_id[] = {
	{ "als_ps", 0 },
	{ }
};
 
static struct i2c_driver rpr400_driver = {
	.driver = {
		.name	= RPR400_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = rpr400_suspend,
	.resume	= rpr400_resume,
	.probe	= rpr400_probe,
	.remove	= __devexit_p(rpr400_remove),
	.id_table = rpr400_id,
};

static int __init rpr400_init(void)
{
	return i2c_add_driver(&rpr400_driver);
}

static void __exit rpr400_exit(void)
{
	i2c_del_driver(&rpr400_driver);
}

MODULE_AUTHOR("Andy");
MODULE_DESCRIPTION("RPR400 ambient light + proximity sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(rpr400_init);
module_exit(rpr400_exit);
