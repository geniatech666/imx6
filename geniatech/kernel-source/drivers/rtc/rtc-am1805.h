/*
 * drivers/amlogic/rtc/rtc_am1805.h
 *
 */

//Register
#define HUNDREDTHS_REG      0x00
#define TIMES_BEGIN_REG     0x01
#define YEARS_REG			0x06
#define ALARM_HUNDRS_REG	0x08
#define ALARM_BEGIN_SECREG	    0x09
#define ALARM_BEGIN_REG	    0x0A
#define STATUS_REG          0x0F
#define CONTROL_1_REG       0x10
#define CONTROL_2_REG		0x11
#define INT_MASK_REG		0x12
#define SQW_REG				0x13
#define CAL_XT_REG			0x14
#define CAL_RC_HI_REG		0x15
#define CAL_RC_LOW_REG		0x16
#define SLEEP_CTRL_REG		0x17
#define TIMER_CTRL_REG		0x18
#define TIMER_REG			0x19
#define TIMER_INITIAL_REG	0x1A
#define WDT_REG				0x1B
#define OSC_CONTROL_REG     0x1C
#define OSC_STATUS_REG		0x1D
#define CONFIG_KEY_REG      0x1F
#define ACAL_FLT_REG     	0x26
#define CHIPID_REG          0x28
#define EXTENDED_ADDR_REG   0x3F
#define RTC_CDT_SET_TIME 0x20

//Alarm IM bit
#define IM_INT_MOD          0x00
#define IM_PULSE_1_8192_MOD 0x01
#define IM_PULSE_1_64_MOD   0x02
#define IM_PULSE_1_4_MOD    0x03

//INT MASK reg
#define INT_MASK_TIE        0x08
#define INT_MASK_AIE        0x04

//Status register bits
#define Status_CB		0x80
#define Status_BAT		0x40
#define Status_WDT		0x20
#define Status_BL		0x10
#define Status_TIM		0x08
#define Status_ALM		0x04
#define Status_EX2		0x02
#define Status_EX1		0x01

//OUT2S bit
#define OUT2S_FUNC_BIT		0x1c

//OUT2S bit function define
/*
 *	pin - select the pin to generate a countdown interrupt
 *		0 => disable the countdown timer
 *		1 => generate an interrupt on nTIRQ only, asserted low
 *		2 => generate an interrupt on FOUT/nIRQ and nTIRQ, both asserted low
 *		3 => generate an interrupt on PSW/nIRQ2 and nTIRQ, both asserted low
 *		4 => generate an interrupt on CLKOUT/nIRQ3 and nTIRQ, both asserted low
 *		5 => generate an interrupt on CLKOUT/nIRQ3 (asserted high) and nTIRQ (asserted low)
 */
#define OUT2S_FUNCTION_NIRQ     0x00
#define OUT2S_FUNCTION_SQW      0x01
#define OUT2S_FUNCTION_RESERVED 0x02
#define OUT2S_FUNCTION_NAIRQ    0x03
#define OUT2S_FUNCTION_TIRQ     0x04
#define OUT2S_FUNCTION_NTIRQ    0x05
#define OUT2S_FUNCTION_SLEEP    0x06
#define OUT2S_FUNCTION_OUTB     0x07

//Alarm repation function
#define ALARM_REPATI_ONCE_PER_SECOND	0x07//match hundredth
#define ALARM_REPATI_ONCE_PER_MINUTE	0x06//match hundredth and second
#define ALARM_REPATI_ONCE_PER_HOUR		0x05//match hundredth`second`minute
#define ALARM_REPATI_ONCE_PER_DAY		0x04//match hundredth`second`minute`hours
#define ALARM_REPATI_ONCE_PER_WEEK		0x03//match hundredth`second`minute`hours,weekday
#define ALARM_REPATI_ONCE_PER_MONTH		0x02//match hundredth`second`minute`hours,date
#define ALARM_REPATI_ONCE_PER_YEAR		0x01//match hundredth`second`minute`hours,date and month 

//define some interrupt pin 
#define WATCHDOG_INT_DISABLE 0
#define WATCHDOG_INT_PIN_FOUT_IRQ 1
#define WATCHDOG_INT_PIN_PSW_IRQ2 2
#define WATCHDOG_INT_PIN_RST 3

//Alarm repeat bit
#define RPT_FUNC_BIT		0x1c

//Calibration mode
#define MODE_XT 0
#define MODE_RC 1
/*******************************************
 *
 *  watchdog control
 *
 ******************************************/
//watchdog configuration
#define WATCHDOG_USR_PIN WATCHDOG_INT_PIN_RST
#define WATCHDOG_TIMER_TIMER 4000//millisecond
#define WATCHDOG_TIMER_FOR_FEED_DOG 2000//millisecond

/*******************************************
 *
 *  alarm control
 *
 ******************************************/
#define ALARM_USE_PIN_AND_FUNC OUT2S_FUNCTION_NAIRQ //PSW/IRQ2
#define ALARM_IM_FUNC IM_INT_MOD //use interrupt
//#define ALARM_IM_FUNC IM_PULSE_1_4_MOD   //use pluse 

/* alarms may be up to 24 hours in the future.
 * Rather than expecting every RTC to implement "don't care"
 * for day/month/year fields, just force the alarm to have
 * the right values for those fields.
 *
 * RTC_WKALM_SET should be used instead.  Not only does it
 * eliminate the need for a separate RTC_AIE_ON call, it
 * doesn't have the "alarm 23:59:59 in the future" race.
 *
 * NOTE:  some legacy code may have used invalid fields as
 * wildcards, exposing hardware "periodic alarm" capabilities.
 * Not supported here.
 */
#define ALARM_REPAT ALARM_REPATI_ONCE_PER_DAY

/*******************************************
 *
 *  timer control
 *
 ******************************************/
#define TIMER_USE_PIN_AND_FUNC OUT2S_FUNCTION_NIRQ //interrupt

/*******************************************
 *
 *  calibration control
 *
 ******************************************/
/* This value is first measured with an oscilloscope and 
 * then calculated based on the correction formula
 */
#define CALIBRATION_VALUE -244

/* The value is fine-tuned according to the actual situation. 
 * Under normal circumstances, the value is 0, but sometimes 
 * the value calculated by the formula is not accurate,
 * so this value is slightly adjusted with the device deviation.
 */
#define CALIBRATION_POSITIVELY_BIASED_VALUE 5 

