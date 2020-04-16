/*
 * The Real_time_clock driver for Ambiq AM1805 family of i2c rtc chips.
 *
 * Copyright(c) 2018 Geniatech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/workqueue.h>
#include <linux/reset.h>
#include <linux/cpu_version.h>
#include <linux/i2c-qcom.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/string.h>
#include <linux/bcd.h>

#include "rtc-am1805.h"
static int feed_dog=0;

int c_dbg_lvl_am1805 = 0;
#define RTC_DBG(lvl, x...) do { pr_info(x); } while (0)
#define RTC_DBG_VAL (1 << 0)
#define RTC_DBG_WR (1 << 1)

#define TIME_LEN 10

struct rtc_am1805_time {
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int week;
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
};

struct rtc_am1805_priv {
	struct	rtc_device *rtc;
	unsigned int initb;
	unsigned int i2c_bus_index;
	unsigned long base_addr;
	struct i2c_driver	driver;
	struct i2c_adapter *pI2cAdapter;
	struct i2c_client	*pI2cClient;
	struct device *dev;
	struct timer_list timer;
	struct work_struct work;
	struct workqueue_struct *watchdog_workqueue;
	
    	//for watchdog
    	spinlock_t watchdog_lock;	
    	int watchdog_enable;
    	int watchdog_timer;
	
    	//for ioctl
	dev_t am1805_id;
	struct class *am1805_class;
    	struct cdev cdev;
};

enum watchdog_Enable_Type {
	TYPE_DISABLE,
	TYPE_KERNLE,	// auto clean watchdog
	TYPE_USER	//clean watchdog for user
};

static struct i2c_board_info am1805_i2c_boardinfo = {
	I2C_BOARD_INFO("rtc_am1805", 0x69),
};

static struct rtc_am1805_priv *rtc_info = NULL;

static int rtc_am1805_i2c_read(unsigned char *buff, u8 reg, unsigned len)
{
	int res = 0;
	struct i2c_msg msgs[2];
	unsigned char ctrBuf[3] = {(reg&0xff), 0, 0};
	if (rtc_info->pI2cClient == NULL){
        	pr_err("%s :i2c client is null\n",__func__);	
    		return -1;
    	}

	msgs[0].addr	= rtc_info->pI2cClient->addr; 
	msgs[0].flags	= 0;
	msgs[0].len	= sizeof(reg);
	msgs[0].buf	= &reg;
	msgs[1].addr	= rtc_info->pI2cClient->addr; 
	msgs[1].flags	= I2C_M_RD;
	msgs[1].len	= len;
	msgs[1].buf	= buff;

	res = i2c_transfer(rtc_info->pI2cClient->adapter, msgs, 2);
	if (res < 0){
		pr_err("%s: i2c transfer failed(%d), addr:%x\n",
				__func__, res, rtc_info->pI2cClient->addr);
	}else{
		res = 0;
	}
	return res;
}

static int rtc_am1805_i2c_write(unsigned char *buff, __u32 addr, unsigned len)
{
	int res = 0;
	unsigned char *kBuf;
	int i;
	struct i2c_msg msg;

	if (rtc_info->pI2cClient == NULL){
	    	pr_err("%s i2c client is null.\n",__func__);
    		return -1;
    	}

	kBuf = kmalloc(len+1, GFP_KERNEL);
	memcpy(kBuf+1, buff, len);
	kBuf[0] = addr&0xFF;
	msg.addr = rtc_info->pI2cClient->addr;
	msg.flags = 0;
	msg.len = len+1;
	msg.buf = kBuf;

	pr_info("%s:i2c write register\n",__func__);
	for (i=0;i<=len;i++){
	    pr_info("%s i2c write %02x \n",__func__,kBuf[i]);
	}

	res = i2c_transfer(rtc_info->pI2cClient->adapter, &msg, 1);
	if (res < 0)
		pr_err("%s: i2c transfer failed ,ret:%d .\n", __func__,res);
	else
		res = 0;

	kfree(kBuf);
	return res;
}

static int rtc_am1805_i2c_write_byte(u8 reg, u8 value)
{
    u8 buffer[2] = { reg, value };
    struct i2c_msg msg;
    int err;

    msg.addr = rtc_info->pI2cClient->addr;
    msg.flags = 0;
    msg.len = sizeof(buffer);
    msg.buf = buffer;
   
    err = i2c_transfer(rtc_info->pI2cClient->adapter, &msg, 1); 
    if (err < 0){
        return -1;
    }
    return 0;
}

#if 0
static void rtc_am1805_reset_rtc(void){
	unsigned char buf[2];
    unsigned len=1;

    //Configuration key init value
	buf[0] = 0x3c; 
	buf[1] = 0x00; 

	if (rtc_am1805_i2c_write(buf, CONFIG_KEY_REG, len)<0){
        pr_info("%s :reset RTC failed.\n",__func__); 
    }else{
        pr_info("%s :reset RTC success.\n",__func__); 
    }
}
#endif

static int rtc_am1805_get_chip_id(void)
{
	unsigned char buf[2];
	if (rtc_am1805_i2c_read(buf, CHIPID_REG, 0x01) >= 0) {
        	//pr_info("%s :read chip id reg:%x.\n",__func__,buf[0]);
    	}else{
        	return -1; 
    	}
    	return 0;
}

//-------rtc function start----------
unsigned char bcd_to_char_am1805(unsigned char value)
{
	unsigned char result = 0;
	result = ((value >> 4) & 0x0f) * 10;
	result += value & 0x0f;
	return result;
}

unsigned char char_to_bcd_am1805(unsigned char value)
{
	unsigned char result = 0;
	result = (((value / 10) & 0x0f) << 4) | ((value % 10) & 0x0f);
	return result;
}

static int parse_init_date(const char *date)
{
	char local_str[TIME_LEN + 1];
	char *year_s, *month_s, *day_s, *str;
	unsigned int year_d, month_d, day_d;
	unsigned int init_date;
	int ret;
	if (strlen(date) != 10)
		return -1;
	memset(local_str, 0, TIME_LEN + 1);
	strncpy(local_str, date, TIME_LEN);
	str = local_str;
	year_s = strsep(&str, "/");
	if (!year_s)
		return -1;
	month_s = strsep(&str, "/");
	if (!month_s)
		return -1;
	day_s = str;
	pr_info("year: %s\nmonth: %s\nday: %s\n", year_s, month_s, day_s);
	ret = kstrtou32(year_s, 10, &year_d);
	if (ret < 0 || year_d > 2100 || year_d < 1900)
		return -1;
	ret = kstrtou32(month_s, 10, &month_d);
	if (ret < 0 || month_d > 12)
		return -1;
	ret = kstrtou32(day_s, 10, &day_d);
	if (ret < 0 || day_d > 31)
		return -1;
	init_date = mktime(year_d, month_d, day_d, 0, 0, 0);
	return init_date;
}

static int parse_date(const char *date, struct rtc_am1805_time *tm)
{
	char local_str[48];
	char *str, *ptr;
	int ret;
	unsigned int index, value;
	memset(local_str, 0, 48);
	memset(tm, 0, sizeof(struct rtc_am1805_time));
	strncpy(local_str, date, strlen(date));
	str = local_str;
	index = 0;
	while ((ptr = strsep(&str, "-: ")) != NULL) {
		ret = kstrtou32(ptr, 10, &value);
		switch (index) {
		case 0:
			if (ret < 0 || value > 2100 || value < 1900)
				return -1;
			tm->year = value;
		break;
		case 1:
			if (ret < 0 || value > 12 || value < 1)
				return -1;
			tm->month = value;
		break;
		case 2:
			if (ret < 0 || value < 1 ||
				value > rtc_month_days
				(tm->month, tm->year))
				return -1;
			tm->day = value;
		break;
		case 3:
			if (ret < 0 || value >= 24)
				return -1;
			tm->hour = value;
		break;
		case 4:
			if (ret < 0 || value >= 60)
				return -1;
			tm->minute = value;
		break;
		case 5:
			if (ret < 0 || value >= 60)
				return -1;
			tm->second = value;
		break;
		}
		index++;
	}
	return 0;
}

static int read_time(struct rtc_am1805_time *tm)
{
	unsigned char buf[7];
	
	if (rtc_am1805_get_chip_id()<0){
        RTC_DBG(RTC_DBG_VAL, "%s :get chip id failed \n",__func__);
       		return -1;
	}
	
	if (rtc_am1805_i2c_read(buf, TIMES_BEGIN_REG, 0x07) < 0) {
        	pr_err("read time err.\n");
		return -1;
    	}
	tm->second = bcd_to_char_am1805(buf[0]);
	tm->minute = bcd_to_char_am1805(buf[1]);
	if (buf[2] & 0x40) {
		tm->hour   = bcd_to_char_am1805(buf[2] & 0x1f);
		if (buf[2] & 0x30) {
			if (tm->hour < 12)
				tm->hour += 12;
		} else {
			if (tm->hour == 12)
				tm->hour = 0;
		}
	} else
	tm->hour   = bcd_to_char_am1805(buf[2] & 0x3f);
	tm->week   = bcd_to_char_am1805(buf[6] & 0x07);
	tm->day    = bcd_to_char_am1805(buf[3]);
	tm->month  = bcd_to_char_am1805(buf[4] & 0x7f);
	tm->year   = bcd_to_char_am1805(buf[5]) + 1970;
	//RTC_DBG(RTC_DBG_VAL, "read time %04d-%02d-%02d %02d:%02d:%02d\n",
	//	tm->year, tm->month, tm->day, tm->hour, tm->minute, tm->second);
	return 0;
}

static int write_time(struct rtc_am1805_time *tm)
{
	unsigned char buf[7];
	buf[0] = char_to_bcd_am1805(tm->second);
	buf[1] = char_to_bcd_am1805(tm->minute);
	buf[2] = char_to_bcd_am1805(tm->hour);
	buf[3] = char_to_bcd_am1805(tm->day);
	buf[4] = char_to_bcd_am1805(tm->month);
	buf[5] = char_to_bcd_am1805(tm->year-1970);
	buf[6] = char_to_bcd_am1805(tm->week);
	RTC_DBG(RTC_DBG_VAL, "write time %04d-%02d-%02d %02d:%02d:%02d\n",
		tm->year, tm->month, tm->day, tm->hour, tm->minute, tm->second);
	if (rtc_am1805_i2c_write(buf, TIMES_BEGIN_REG, 0x07) < 0)
		return -1;
	return 0;
}

static int get_rtc_am1805_status(void)
{
	struct rtc_am1805_time ptime;

	if (read_time(&ptime) < 0)
		return -1;
	if (ptime.second >= 60)
		return -1;
	if (ptime.minute >= 60)
		return -1;
	if (ptime.hour >= 24)
		return -1;
	if (ptime.month > 12)
		return -1;
	
	if (ptime.day > rtc_month_days(ptime.month-1, ptime.year)){
		RTC_DBG(RTC_DBG_VAL, "rtc_mouth:ptime.month:%d,ptime.year:%d,ptime.day:%d,rtc_month_days:%d\n",ptime.month,ptime.year,ptime.day,rtc_month_days(ptime.month-1, ptime.year));
		return -1;
    	}

	return 0;
}

static int rtc_am1805_init_date(unsigned long init_date)
{
	struct rtc_time tm;
	struct rtc_am1805_time ptime;

	rtc_time_to_tm(init_date, &tm);
	ptime.second = tm.tm_sec;
	ptime.minute = tm.tm_min;
	ptime.hour   = tm.tm_hour;
	ptime.day    = tm.tm_mday;
	ptime.month  = tm.tm_mon;
	ptime.year   = tm.tm_year + 1970;

	ptime.week   = tm.tm_wday;
	printk(KERN_ERR "year:%d,month:%d,week:%d",ptime.year,ptime.month,ptime.week);
	if (write_time(&ptime) < 0)
		return -1;
	return 0;
}
//--------rtc function end----------

int write_alarm_am1805(struct rtc_time *tm)
{
	unsigned char buf[4];
	buf[0] = char_to_bcd_am1805(tm->tm_sec);
	buf[1] = char_to_bcd_am1805(tm->tm_min);
	buf[2] = char_to_bcd_am1805(tm->tm_hour);
	buf[3] = char_to_bcd_am1805(tm->tm_mday);
	RTC_DBG(RTC_DBG_VAL, "write alarm time %02d:%02d:%02d\n",
			tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    	if (rtc_am1805_i2c_write(buf, ALARM_BEGIN_SECREG, 0x04) < 0)
		return -1;
	return 0;
}

/*
 *	am1805_watchdog_feeddog- set up the watchdog timer  
 *
 *	Inputs:
 *	period - timeout period in ms (65 to 124,000)
 *	pin - pin to generate the watchdog signal
 *		0 => disable WDT
 *		1 => generate an interrupt on FOUT/nIRQ
 *		2 => generate an interrupt on PSW/nIRQ2
 *		3 => generate a reset on nRST (AM18xx only)
 *
 */
static void am1805_watchdog_feeddog(uint32_t period, uint8_t pin)
{
    	uint8_t WDTreg;
	uint8_t wds;
	uint8_t bmb;
	uint8_t wrb;
    	unsigned char temp;

    	//RTC_DBG(RTC_DBG_VAL, "%s ,run \n",__func__);
	
    	// Use the shortest clock interval which will allow the selected period
	if (period < (31000 / 16))
	{
		wrb = 0;						// Use 16 Hz
		bmb = (period * 16) / 1000;
	}
	else if (period < (31000 / 4))
	{
		wrb = 1;						// Use 4 Hz
		bmb = (period * 4) / 1000;
	}
	else if (period < 31000)
    	{
		wrb = 2;						// Use 1 Hz
		bmb = period / 1000;
	}
	else
	{
		wrb = 3;						// Use 1/4 Hz
		bmb = period / 4000;
	}

	switch (pin)
	{
		case WATCHDOG_INT_DISABLE:							 // Disable WDT
			wds = 0;
			bmb = 0;
			break;
		case WATCHDOG_INT_PIN_FOUT_IRQ: 						 // Interrupt on FOUT/nIRQ
			wds = 0;					 // Select interrupt
            
            //Clear the OUT1S field
            if (rtc_am1805_i2c_read(&temp, CONTROL_2_REG, 1) < 0){
                pr_info("%s:read CONTROL_2_REG error",__func__);    
            }
            temp = temp & 0xfc; 
            rtc_am1805_i2c_write(&temp, CONTROL_2_REG, 0x01);
			break;
		case WATCHDOG_INT_PIN_PSW_IRQ2:							 // Interrupt on PSW/nIRQ2
			wds = 0;					 // Select interrupt
            
            //Clear the OUT2S field
            if (rtc_am1805_i2c_read(&temp, CONTROL_2_REG, 1) < 0){
                pr_info("%s:read CONTROL_2_REG error",__func__);    
            }
            temp = temp & 0xe3; 
            rtc_am1805_i2c_write(&temp, CONTROL_2_REG, 0x01);
			break;
		case WATCHDOG_INT_PIN_RST: 						 // Interrupt on nRST
		default:
			wds = 1;					 // Select reset out
			break;
	}
	WDTreg = (wds * 0x80) + (bmb * 0x4) + wrb;	
    	
	//printk(KERN_ERR "feed dog value:WDTreg:%d,wds:%d,bmb:%d,period:%d,pin:%d,wrb:%d,wds:%d\n",WDTreg,wds,bmb,period,pin,wrb,wds);
	rtc_am1805_i2c_write_byte(WDT_REG,WDTreg);
}

static void am1805_watchdog_work(struct work_struct *work)
{
	struct rtc_am1805_priv *plat = (struct rtc_am1805_priv *)container_of(work,struct rtc_am1805_priv , work);

    	//RTC_DBG(RTC_DBG_VAL, "%s ,feed_dog:%d,run \n",__func__,feed_dog);
	if(plat->watchdog_enable == TYPE_KERNLE /*&& feed_dog < 30*/) {
		am1805_watchdog_feeddog(rtc_info->watchdog_timer,WATCHDOG_INT_PIN_RST);
		mod_timer(&rtc_info->timer,  jiffies +  msecs_to_jiffies(WATCHDOG_TIMER_FOR_FEED_DOG));
		feed_dog++;
	}
}

static void am1805_watchdog_timer(unsigned long data)
{
	struct rtc_am1805_priv *plat = (struct rtc_am1805_priv *)data;
	unsigned long flags = 0;
	
    	//RTC_DBG(RTC_DBG_VAL, "%s ,run \n",__func__);
	spin_lock_irqsave(&plat->watchdog_lock, flags);
	queue_work(plat->watchdog_workqueue, &plat->work);
	spin_unlock_irqrestore(&plat->watchdog_lock, flags);
}
//--------watchdog end---------------

//--------calibration start---------------
// mode: 0->XT;1->RC
static void am_set_calibration(uint8_t mode, int32_t adjust) 
{
    int32_t adjint;
    uint8_t adjreg;
    uint8_t adjregu;
    uint8_t xtcal;
    unsigned char temp;

    if (adjust < 0 ){
		adjint = ((adjust)*1000 - 953);
    }else{
		adjint = ((adjust)*1000 + 953);
    }

    adjint = adjint/1907;
    adjint = adjint - CALIBRATION_POSITIVELY_BIASED_VALUE;

    if (mode == 0)
    {
        // XT adjust
        if (adjint > 63 )
        {
            // 64 to 127
            xtcal = 0;
            adjreg = ((adjint >> 1) & 0x3F) | 0x80; // CMDX = 1
        }
        else if (adjint > -65)
        {
            // -64 to 63
            xtcal = 0;
            adjreg = (adjint & 0x7F);               // CMDX = 0
        }
        else if (adjint > -129)
        {
            // -128 to -65
            xtcal = 1;
            adjreg = ((adjint + 64) & 0x7F);        // CMDX = 0
        }
        else if (adjint > -193)
        {
            // -192 to -129
            xtcal = 2;
            adjreg = ((adjint + 128) & 0x7F);       // CMDX = 0
        }
        else if (adjint > -257)
        {
            // -256 to -193
            xtcal = 3;
            adjreg = ((adjint + 192) & 0x7F);       // CMDX = 0
        }
        else
        {
            // -320 to -257
            xtcal = 3;
            adjreg = ((adjint + 192) >> 1) & 0xFF;  // CMDX = 1
        }
    
	rtc_am1805_i2c_write(&adjreg, CAL_XT_REG, 0x01);
        RTC_DBG(RTC_DBG_VAL, "%s write CAL_XT_REG Reg__ :0x%02x\n",__func__,adjreg);
	if (rtc_am1805_i2c_read(&temp, OSC_STATUS_REG, 1) < 0){
		pr_info("%s:read OSC_STATUS_REG error",__func__);    
	}
	temp &= 0x3F; 
        temp = temp | (xtcal << 6);             // Add XTCAL field
	rtc_am1805_i2c_write(&temp, OSC_STATUS_REG, 0x01);
        RTC_DBG(RTC_DBG_VAL, "%s write OSC_STATUS_REG Reg__ :0x%02x\n",__func__,temp);
    }
    else
    {
        // RC adjust
        if (adjint > 32767 )
        {
            // 32768 to 65535
            adjreg = ((adjint >> 3) & 0xFF);        // Lower 8 bits
            adjregu = ((adjint >> 11) | 0xC0);      // CMDR = 3
        }
        else if (adjint > 16383 )
        {
            // 16384 to 32767
            adjreg = ((adjint >> 2) & 0xFF);        // Lower 8 bits
            adjregu = ((adjint >> 10) | 0x80);      // CMDR = 2
        }
        else if (adjint > 8191 )
        {
            // 8192 to 16383
            adjreg = ((adjint >> 1) & 0xFF);        // Lower 8 bits
            adjregu = ((adjint >> 9) | 0x40);       // CMDR = 2
        }
        else if (adjint >= 0 )
        {
            // 0 to 1023
            adjreg = ((adjint) & 0xFF);             // Lower 8 bits
            adjregu = (adjint >> 8);                // CMDR = 0
        }
        else if (adjint > -8193 )
        {
            // -8192 to -1
            adjreg = ((adjint) & 0xFF);             // Lower 8 bits
            adjregu = (adjint >> 8) & 0x3F;         // CMDR = 0
        }
        else if (adjint > -16385 )
        {
            // -16384 to -8193
            adjreg = ((adjint >> 1) & 0xFF);        // Lower 8 bits
            adjregu = (adjint >> 9) & 0x7F;         // CMDR = 1
        }
        else if (adjint > -32769 )
        {
            // -32768 to -16385
            adjreg = ((adjint >> 2) & 0xFF);        // Lower 8 bits
            adjregu = (adjint >> 10) & 0xBF;        // CMDR = 2
        }
        else
        {
            // -65536 to -32769
            adjreg = ((adjint >> 3) & 0xFF);        // Lower 8 bits
            adjregu = (adjint >> 11) & 0xFF;        // CMDR = 3
        }

	rtc_am1805_i2c_write(&adjregu, CAL_RC_HI_REG, 0x01);// Load the CALRU register
	rtc_am1805_i2c_write(&adjreg, CAL_RC_LOW_REG, 0x01);// Load the CALRL register
    }
}
//--------calibration end---------------

//--------abstracts class start------
static ssize_t show_all_reg(struct class *class,
			struct class_attribute *attr,	char *buf)
{
    	int i;
    	unsigned char tbuf[2];
	
	RTC_DBG(RTC_DBG_VAL, "enter function: %s\n", __func__);

	//get all registers
	for (i = 0; i < 0x3f; i++) {
        if (rtc_am1805_i2c_read(tbuf, i, 0x01) >= 0) {
            RTC_DBG(RTC_DBG_VAL, "read chip  Reg__[0x%02x] :0x%02x\n",i,tbuf[0]);
        }else{
            pr_err("%s :get chip Reg failed \n",__func__);
            break;
		}
    	}
    return 0;
}

static ssize_t show_time(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	int ret;
	struct rtc_am1805_time tm;
	if (read_time(&tm) < 0)
		return -1;
	ret = sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d\n",
		tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
	return ret;
}

static ssize_t store_time(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count)
{
	struct rtc_am1805_time tm;
	parse_date(buf, &tm);
	RTC_DBG(RTC_DBG_VAL, "set time %04d-%02d-%02d %02d:%02d:%02d\n",
		tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
	if (write_time(&tm) < 0)
		return -1;
    return count;
}

static ssize_t store_reg(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count)
{
    unsigned char reg_value[3];	
    char str[] = "geniatech zkh";
    char *p,*buff;
    int i,temp_val;
    
    i=0; 
    strcpy(str,buf);
    buff=str;
    p = strsep(&buff, " ");//split with empty space
    while(p)
    {
        sscanf(p,"%i",&temp_val);
        reg_value[i]=temp_val;
        i=i+1;
        p = strsep(&buff, " ");
    }

    for(i=0;i<2;i++){
        RTC_DBG(RTC_DBG_VAL, "%s:reg_value[%d] is :0x%02x\n",__func__,i, reg_value[i]);
    }
    rtc_am1805_i2c_write(&reg_value[1],reg_value[0], 1);
    return count;
}

static ssize_t show_reg(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count)
{
    unsigned char tbuf[2];
    int reg;	
    
    sscanf(buf,"%i",&reg);
    if (rtc_am1805_i2c_read(tbuf, reg, 0x01) >= 0) {
        RTC_DBG(RTC_DBG_VAL, "%s read chip Reg__ :0x%02x\n",__func__,tbuf[0]);
    }else{
        pr_err("%s :get chip Reg failed \n",__func__);
    }
    return count;
}

static ssize_t am1805_show_feeddog(struct class *class,
			struct class_attribute *attr,	char *buf)
{
	int ret;

	if (rtc_info->watchdog_enable==TYPE_DISABLE)
	{
		ret = sprintf(buf, "0 -- disable\n");
	}
	else if(rtc_info->watchdog_enable==TYPE_KERNLE)
	{
		ret = sprintf(buf, "1 -- kernel feed\n");
	}
	else{
		ret = sprintf(buf, "2 -- user feed\n");
	}

	return ret;
}

static ssize_t am1805_store_feeddog(struct class *class,
	struct class_attribute *attr, const char *buf, size_t count)
{
	int type;

  	RTC_DBG(RTC_DBG_VAL, "%s ,run \n",__func__);

  	sscanf(buf,"%i",&type);

  	if(type==0){
		rtc_info->watchdog_enable = TYPE_DISABLE;
	 	am1805_watchdog_feeddog(rtc_info->watchdog_timer,WATCHDOG_INT_DISABLE);
  	}
  	else if(type==1){
	 	rtc_info->watchdog_enable = TYPE_KERNLE;
	 	am1805_watchdog_feeddog(rtc_info->watchdog_timer,WATCHDOG_INT_PIN_RST);
  	}
  	else{
	 	rtc_info->watchdog_enable = TYPE_USER;
	 	am1805_watchdog_feeddog(rtc_info->watchdog_timer,WATCHDOG_INT_PIN_RST);
  	}

  	return count;
}

static ssize_t aml1805_show_timer(struct class* class,
	struct class_attribute *att, char* buf)
{
  	RTC_DBG(RTC_DBG_VAL, "%s ,run \n",__func__);

  	return sprintf(buf, "%d\n", rtc_info->watchdog_timer);
}

static ssize_t am1805_store_timer(struct class* class,
	struct class_attribute* att, const char* buf, size_t count)
{
	int value;

  	RTC_DBG(RTC_DBG_VAL, "%s ,run \n",__func__);

  	sscanf(buf,"%i",&value);
  	RTC_DBG(RTC_DBG_VAL, "value = %d \n", value);

  	rtc_info->watchdog_timer = value;
	am1805_watchdog_feeddog(rtc_info->watchdog_timer,WATCHDOG_INT_PIN_RST);

  	return count;
}

static ssize_t am1805_set_alarm(struct class *class,
			struct class_attribute *attr,	char *buf)
{
    	unsigned char reg_tmp_value;
	unsigned char tbuf[4];
	struct rtc_am1805_time tm;
	int ret;

    	if (read_time(&tm) < 0)
		return -1;
	ret = sprintf(tbuf, "%04d-%02d-%02d %02d:%02d:%02d\n",
		tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
    	RTC_DBG(RTC_DBG_VAL, "%s ,run \n",__func__);
    
	buf[0] = char_to_bcd_am1805(tm.minute+2);//add two minutes for test
	buf[1] = char_to_bcd_am1805(tm.hour);
	buf[2] = char_to_bcd_am1805(tm.day);
	buf[3] = char_to_bcd_am1805(tm.month);
	RTC_DBG(RTC_DBG_VAL, "write alarm time %02d %02d:%02d\n",
			tm.day, tm.hour, tm.minute);
    
    	if (rtc_am1805_i2c_write(tbuf, ALARM_BEGIN_REG, 0x04) < 0)
		return -1;
   
   
    	if (rtc_am1805_i2c_read(&reg_tmp_value, STATUS_REG, 1) < 0){
        	pr_info("%s:read error",__func__);    
    	}
    	reg_tmp_value = reg_tmp_value & 0xfb; 
    
    	pr_info("%s:reg_tmp_value is :%02x",__func__,reg_tmp_value);    
    	return ret;
}

/* lihong remove */
/*static struct class_attribute rtc_class_attrs[] = {
	__ATTR(show_all_reg, S_IRUGO | S_IWUSR, show_all_reg, NULL),
	__ATTR(time, S_IRUGO | S_IWUSR, show_time, store_time),
	__ATTR(regwrite, S_IRUGO | S_IWUSR, NULL, store_reg),
	__ATTR(regread, S_IRUGO | S_IWUSR, NULL,show_reg),
	__ATTR(feeddog, S_IRUGO | S_IWUSR, am1805_show_feeddog,am1805_store_feeddog),
	__ATTR(timer, S_IRUGO | S_IWUSR, aml1805_show_timer, am1805_store_timer),
	__ATTR(setalarm, S_IRUGO | S_IWUSR, am1805_set_alarm,NULL),
	__ATTR_NULL
};

static struct class rtc_am1805_class = {
	.name = "rtc_am1805",
	.class_attrs = rtc_class_attrs,
};*/
//--------abstracts class end------

//--------platform rtc dev start---
static int rtc_am1805_open(struct device *dev)
{
	RTC_DBG(RTC_DBG_VAL, "%s :open success!\n",__func__);

	return 0;
}

static int rtc_am1805_read_time(struct device *dev, struct rtc_time *tm)
{
	struct rtc_am1805_time ptime;
	unsigned long time_t;

	//RTC_DBG(RTC_DBG_VAL,"%s \n",__func__); 
	if (read_time(&ptime) < 0)
		return -1;

	time_t = mktime(ptime.year, ptime.month, ptime.day,
				ptime.hour, ptime.minute, ptime.second);
	//RTC_DBG(RTC_DBG_VAL,"rtc_am1805: have read the rtc time, time is %ld\n",time_t);
	//RTC_DBG(RTC_DBG_VAL, "rtc_am1805 read time %04d-%02d-%02d %02d:%02d:%02d\n",
	//	ptime.year, ptime.month, ptime.day, ptime.hour, ptime.minute, ptime.second);
	if (time_t < 0) {
		RTC_DBG(RTC_DBG_VAL,
			"rtc_am1805: time(%ld) < 0, reset to 0", time_t);
		time_t = 0;
	}
	rtc_time_to_tm(time_t, tm);

	return 0;
}


static int rtc_am1805_write_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time_t;
	struct rtc_am1805_time ptime;
	
	RTC_DBG(RTC_DBG_VAL,"%s \n",__func__); 
	ptime.second = tm->tm_sec;
	ptime.minute = tm->tm_min;
	ptime.hour   = tm->tm_hour;
	ptime.day    = tm->tm_mday;
	ptime.month  = tm->tm_mon + 1;
	ptime.year   = tm->tm_year + 1900;
	ptime.week   = tm->tm_wday;
	rtc_tm_to_time(tm, &time_t);
	
	RTC_DBG(RTC_DBG_VAL,
		"rtc_am1805 : write the rtc time, time is %ld\n",time_t);
	RTC_DBG(RTC_DBG_VAL, "%04d-%02d-%02d %02d:%02d:%02d\n",
		ptime.year, ptime.month, ptime.day,
		ptime.hour, ptime.minute, ptime.second);

	if (write_time(&ptime) < 0)
		return -1;
	RTC_DBG(RTC_DBG_VAL, "rtc_am1805 : the time has been written\n");

	return 0;
}

static int rtc_am1805_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	unsigned char buf[7];

	RTC_DBG(RTC_DBG_VAL,"%s \n",__func__); 
	if (rtc_am1805_i2c_read(buf, ALARM_BEGIN_SECREG, 0x07) < 0) {
        pr_err("%s:read time err.\n",__func__);
		return -1;
    	}

	/* some of these fields may be wildcard/"match all" */
	alarm->time.tm_sec = buf[0];
	alarm->time.tm_min = buf[1];
	alarm->time.tm_hour = buf[2];
	alarm->time.tm_mday = buf[3];
	alarm->time.tm_mon = buf[4];

	/*
	  here is a problem,because am1805's alarm not have years register.
	  I think that maybe we can return a year's data from the register
	  of the rtc.
	 */
	if (rtc_am1805_i2c_read(buf,YEARS_REG,0x01)<0) {
        pr_err("%s:read rtc year register err.\n",__func__);
		return -1;
	}
	alarm->time.tm_year = buf[0]+1970;
													
	RTC_DBG(RTC_DBG_VAL, "alarm read RTC date/time %4d-%02d-%02d(%d) %02d:%02d:%02d\n",
	1970 + alarm->time.tm_year, alarm->time.tm_mon,
	alarm->time.tm_mday, alarm->time.tm_wday, alarm->time.tm_hour,
	alarm->time.tm_min, alarm->time.tm_sec);

	if (rtc_am1805_i2c_read(buf,STATUS_REG,0x01)<0) {
        pr_err("%s:read rtc status register err.\n",__func__);
		return -1;
	}
	if (buf[0] & Status_ALM)
		alarm->enabled = 1;
	else
		alarm->enabled = 0;

	return 0;
}

static int rtc_am1805_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct rtc_time *tm=&alarm->time;
	struct rtc_am1805_priv *priv;
    unsigned char temp;

	RTC_DBG(RTC_DBG_VAL,"%s .\n",__func__); 
	priv = dev_get_drvdata(dev);
	
	//clear ALM bit
    if (rtc_am1805_i2c_read(&temp, STATUS_REG, 1) < 0){
        pr_info("%s:read STATUS_REG error",__func__);    
    }
    temp &= ~Status_ALM; 
    rtc_am1805_i2c_write(&temp, STATUS_REG, 0x01);
    
    //set CDT RPT bit,set repat
    if (rtc_am1805_i2c_read(&temp, TIMER_CTRL_REG, 1) < 0){
        pr_info("%s:read TIMER_CTRL_REG error",__func__);    
    }
    temp &= ~RPT_FUNC_BIT;//Clear RPT bit 
    temp |= ALARM_REPATI_ONCE_PER_DAY<<2;
    rtc_am1805_i2c_write(&temp, TIMER_CTRL_REG, 0x01);

    //set OUT2S bit for PWS/NIRQ2
    if (rtc_am1805_i2c_read(&temp, CONTROL_2_REG, 1) < 0){
        pr_info("%s:read CONTROL_2_REG error",__func__);    
    }
    temp &= ~OUT2S_FUNC_BIT;//Clear OUT2S bit 
    temp |= ALARM_USE_PIN_AND_FUNC<<2;//set OUT2S for AIRQ
    rtc_am1805_i2c_write(&temp, CONTROL_2_REG, 0x01);

    //set time to alarm register
	if (write_alarm_am1805(tm)<0){	
		return -1;
    }

    //set AIE bit for enable ararm 
    if (rtc_am1805_i2c_read(&temp, INT_MASK_REG, 1) < 0){
        pr_info("%s:read INT_MASK_REG error",__func__);    
    }
    temp = (temp & 0x9b) | ALARM_IM_FUNC<<5;//set for generate irq or plus
    temp |= INT_MASK_AIE;//set AIE
    rtc_am1805_i2c_write(&temp, INT_MASK_REG, 0x01);

	RTC_DBG(RTC_DBG_VAL,"%s set alarm success\n",__func__); 
    return 0;
}

/*
 *	prams:
 *  value - the period of the countdown timer
 *	repeat - configure the interrupt output type
 *		0 => generate a single level interrupt
 *		1 => generate a repeated pulsed interrupt, 1/64 s (range must be 1)
 *		2 => generate a single pulsed interrupt, 1/64 s (range must be 1)
 */
static int rtc_am1805_set_countdown_timer(int value,int repeat)
{
    unsigned char temp;
	uint8_t te_bit,tm_bit,trpt_bit,tfs_bit;

	RTC_DBG(RTC_DBG_VAL,"%s .\n",__func__); 

	/* 
	 * Sometimes maybe we will recover the old timer, in the case,
	 * we should clean the old timer,and fill the new value into  
	 * timer register
	 */

	//clear TE/TM/TRPT/TFS bit
    if (rtc_am1805_i2c_read(&temp, TIMER_CTRL_REG, 1) < 0){
        pr_info("%s:read TIMER_CTRL_REG error",__func__);    
    }
    temp &= RPT_FUNC_BIT; 
    rtc_am1805_i2c_write(&temp, TIMER_CTRL_REG, 0x01);
    
    //set OUT2S bit for NIRQ
    if (rtc_am1805_i2c_read(&temp, CONTROL_2_REG, 1) < 0){
        pr_info("%s:read CONTROL_2_REG error",__func__);    
    }
    temp &= ~OUT2S_FUNC_BIT;//Clear OUT2S bit 
    temp |= TIMER_USE_PIN_AND_FUNC<<2;//set OUT2S for nIRQ
    rtc_am1805_i2c_write(&temp, CONTROL_2_REG, 0x01);
 
 	//clear TIM bit
    if (rtc_am1805_i2c_read(&temp, STATUS_REG, 1) < 0){
        pr_info("%s:read STATUS_REG error",__func__);    
    }
    temp &= ~Status_TIM; 
    rtc_am1805_i2c_write(&temp, STATUS_REG, 0x01);
    
	//set TIE bit for enable timer 
    if (rtc_am1805_i2c_read(&temp, INT_MASK_REG, 1) < 0){
        pr_info("%s:read INT_MASK_REG error",__func__);    
    }
    temp |= INT_MASK_TIE;//set TIE
    rtc_am1805_i2c_write(&temp, INT_MASK_REG, 0x01);

	te_bit=1; //Timer Enable
	temp = value;
	if (repeat == 0) {//no repeat
		// Level interrupt
		tm_bit = 1;// Level
		trpt_bit = 0;// No repeat
	}else if (repeat == 1){
		//Repeat pluse interrupt	
		tm_bit = 0;// disable Level
		trpt_bit = 1;// repeat
	}else {//if (repeat == 2)
		//Single pluse interrupt	
		tm_bit = 0;// disable Level
		trpt_bit = 0;// repeat
	}//if (repeat ==0) 
		
	if (temp<=256){
		// Use 1 Hz
		tfs_bit = 2;
	}else{
		// Use 1/60 Hz
		tfs_bit = 3;

		temp = temp/60;
		temp = temp -1;
	}	
	//set timer
	rtc_am1805_i2c_write(&temp, TIMER_REG, 0x01);

	if (trpt_bit == 1){
		rtc_am1805_i2c_write(&temp, TIMER_INITIAL_REG, 0x01);
	}

	//Enable CDT 
   	if (rtc_am1805_i2c_read(&temp, TIMER_CTRL_REG, 1) < 0){
        	pr_info("%s:read TIMER_CTRL_REG error",__func__);    
    	}
    	temp &= RPT_FUNC_BIT; //keep alarm RPT bit
		temp = temp | (te_bit * 0x80) | (tm_bit * 0x40) | (trpt_bit * 0x20) | tfs_bit;	// Merge the fields;
    	rtc_am1805_i2c_write(&temp, TIMER_CTRL_REG, 0x01);
	
	RTC_DBG(RTC_DBG_VAL,"%s set alarm success\n",__func__); 
    	return 0;
}

static int rtc_am1805_set_timer(struct device *dev, struct rtc_time *tm)
{
	int time,repeat;

	time=tm->tm_sec;
	repeat=tm->tm_min;

	RTC_DBG(RTC_DBG_VAL,"%s ,time:%d,repeat:%d.\n",__func__,time,repeat); 
	rtc_am1805_set_countdown_timer(time,repeat);

	return 0;
}

static int rtc_am1805_read_timer(struct device *dev, struct rtc_time *tm)
{
	struct rtc_am1805_time ptime;
    	unsigned char temp;
		
	RTC_DBG(RTC_DBG_VAL,"%s .\n",__func__); 
	
	if (rtc_am1805_i2c_read(&temp, TIMER_REG, 1) < 0){
        	pr_info("%s:read TIMER_CTRL_REG error",__func__);    
    	}
    	pr_info("%s:%d",__func__,temp);    
	
	if (read_time(&ptime) < 0)
		return -1;

	tm->tm_sec=temp;
	return 0;
}

static int rtc_am1805_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	struct rtc_time wtime;
	
	/*
	 * Drivers *SHOULD NOT* provide ioctl implementations
	 * for these requests.  Instead, provide methods to
	 * support the following code, so that the RTC's main
	 * features are accessible without using ioctls.
	 *
	 * RTC and alarm times will be in UTC, by preference,
	 * but dual-booting with MS-Windows implies RTCs must
	 * use the local wall clock time.
	 */
	
	RTC_DBG(RTC_DBG_VAL,"%s cmd is :%d\n",__func__,cmd); 
	switch (cmd) {
		case RTC_CDT_SET_TIME:
			pr_info("%s:RTC_CDT_SET_TIME \n",__func__);    
			break;
		default:
			return -ENOIOCTLCMD;
	}

    	return copy_to_user((void __user *)arg,
	    &wtime, sizeof wtime) ? -EFAULT : 0; 
}

static const struct rtc_class_ops rtc_am1805_ops = {
	//lihong remove
	//.open       = rtc_am1805_open,
	.read_time = rtc_am1805_read_time,
	.set_time = rtc_am1805_write_time,
	//.read_alarm = rtc_am1805_read_alarm,
	//.set_alarm = rtc_am1805_set_alarm,
	//.set_cdt_time = rtc_am1805_set_timer,
	//.read_cdt_time = rtc_am1805_read_timer,
	.ioctl      = rtc_am1805_ioctl,
};

static int rtc_am1805_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct rtc_am1805_priv *priv;
    	struct device *dev = &client->dev;
    	struct device_node *node = client->dev.of_node;
	int ret,gpio;
	const char *str;
	unsigned long init_data = 0;
	
	RTC_DBG(RTC_DBG_VAL, "rtc_am1805 --rtc_am1805_probe\n");
	printk(KERN_ERR "+++++++++++++++++++ %s\n", __func__);
	
	priv = devm_kzalloc(dev, sizeof(struct rtc_am1805_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

    	printk(KERN_ERR "am1805_i2c_probe begin !\n");
	if (!dev->of_node)
        {
		printk(KERN_ERR "dev->node is null\n");
		return -EINVAL;
	}
	if (client == NULL) {
		printk(KERN_ERR "rtc_am1805_i2c_probe: client is null\n");
		return -EINVAL;
	}
    
	priv->i2c_bus_index = 0xff;

#if 0
    	/* for alarm irq */
	gpio = of_get_named_gpio_flags(node, "gpio_intb", 0, NULL);
	printk(KERN_ERR "gpio_intb:%d",gpio);

	ret = gpio_to_irq(gpio);
	if (ret < 0)
		pr_info("[%s]: failed to map gpio_intr!\n", __func__);
	else {
		priv->initb = ret;
		/*gpio_request(devinfo->config.gpio_intr, DRIVER_NAME);
		gpio_direction_input(devinfo->config.gpio_intr); */
	}
#endif

    	/* init dts date */
	ret = of_property_read_string(node, "init_date", &str);
	if (!ret) {
		RTC_DBG(RTC_DBG_VAL, "init_date: %s\n", str);
		ret = parse_init_date(str);
		if (ret > 0)
			init_data = ret;
	}

    	priv->dev = &client->dev;
    	priv->pI2cClient = client;
    	rtc_info = priv;
    
    	i2c_set_clientdata(client, priv);
    
    	//reset ic
    	//rtc_am1805_reset_rtc();
    	//check chip id
    	if (rtc_am1805_get_chip_id()<0){
        	RTC_DBG(RTC_DBG_VAL, "%s :get chip id failed \n",__func__);
       	// goto out;    
    	}
    	//Disable nEXTR to generate nRST
    	if (rtc_am1805_i2c_write_byte(CONTROL_2_REG,0x1c)<0){
        	RTC_DBG(RTC_DBG_VAL, "%s :disable nEXTR pin failed \n",__func__);
	//        goto out;    
    	}
    	
	//if chip's time is err,init date	
    	if (get_rtc_am1805_status() < 0)
			rtc_am1805_init_date(init_data);

    	//enable watchdog flags,for am1805_watchdog_work or class control
    	rtc_info->watchdog_enable=TYPE_KERNLE;
    	rtc_info->watchdog_timer = WATCHDOG_TIMER_TIMER;

	//Timer for feed dog
	setup_timer(&rtc_info->timer, am1805_watchdog_timer, 100000000);
	rtc_info->timer.data = (unsigned long)rtc_info;
	add_timer(&rtc_info->timer);

    	//Workqueue for set watchdog timer
	INIT_WORK(&rtc_info->work, am1805_watchdog_work);
	rtc_info->watchdog_workqueue = create_singlethread_workqueue("am1805wd");
	if (rtc_info->watchdog_workqueue == NULL) {
        pr_err("%s :Create workqueue failed. \n",__func__);
		goto out;
	}

    	/* platform setup code should have handled this; sigh */
	if (!device_can_wakeup(&client->dev))
		device_init_wakeup(&client->dev, 1);

	priv->rtc = rtc_device_register("rtc_am1805", &client->dev,
				&rtc_am1805_ops, THIS_MODULE);
                
	if (IS_ERR(priv->rtc)) {
		ret = PTR_ERR(priv->rtc);
		goto out;
	}

    //create chr ioctl
#if 0
	if(am1805_creat_chr_fs(rtc_info)){
	    pr_err("Create am1805 chrdev fail\n");	
	}
#endif
	/* lihong remove */
	/*ret = class_register(&rtc_am1805_class);
	if (ret){
		pr_info(" class register rtc_class fail!\n");
		return -1;
	}*/

	//platform_set_drvdata(pdev, priv);
    	dev_set_drvdata(&client->dev,priv);
	RTC_DBG(RTC_DBG_VAL, "rtc_am1805 --rtc_am1805_probe ok\n");

	//calibrate xt 
	am_set_calibration(MODE_XT,CALIBRATION_VALUE);

	return 0;
out:
	kfree(priv);
	return ret;
}

static int rtc_am1805_remove(struct i2c_client *i2c)
{
	struct rtc_am1805_priv *priv = dev_get_drvdata(&i2c->dev);
	rtc_device_unregister(priv->rtc);
	/* lihong remove */
    	/*class_unregister(&rtc_am1805_class);*/
	kfree(priv);
	return 0;
}

static int rtc_am1805_suspend(struct i2c_client *client)
{
	return 0;
}

static int rtc_am1805_resume(struct i2c_client *client)
{
	return 0;
}

static void rtc_am1805_shutdown(struct i2c_client *client)
{
	unsigned char val=0;
	//disable watchdog
	rtc_am1805_i2c_write(&val, WDT_REG, 0x01);
}


static const struct i2c_device_id am1805_i2c_id[] = {
	{ "qcom,rtc_am1805", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, am1805_i2c_id);

static const struct of_device_id meson_rtc_dt_match[] = {
	{ .compatible = "qcom,rtc_am1805",.data = NULL},
	{},
};

MODULE_DEVICE_TABLE(of, meson_rtc_dt_match);

struct i2c_driver rtc_am1805_driver = {
	.driver = {
		.name = "rtc_am1805",
		.of_match_table = meson_rtc_dt_match,
	},
	.probe = rtc_am1805_probe,
	.remove = rtc_am1805_remove,
	//.suspend = rtc_am1805_suspend,
	//.resume = rtc_am1805_resume,
	.shutdown = rtc_am1805_shutdown,
	.id_table = am1805_i2c_id,
};

static int  __init rtc_am1805_init(void)
{
    RTC_DBG(RTC_DBG_VAL, "rtc_am1805 --rtc_am1805_init\n");
    return i2c_add_driver(&rtc_am1805_driver);
}

static void __init rtc_am1805_exit(void)
{
    return i2c_del_driver(&rtc_am1805_driver);
}

module_init(rtc_am1805_init);
module_exit(rtc_am1805_exit);

MODULE_DESCRIPTION("Geniatech am1805 ic driver");
MODULE_LICENSE("GPL");


