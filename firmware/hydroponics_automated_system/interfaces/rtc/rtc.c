/*Includes*/
#include "rtc.h"

/*Defines / Macros*/
#define DEC_MAX_VALUE 99
#define BCD_MAX_VALUE 9

#define RTC_EPOCH_YEAR 2000
#define RTC_EPOCH_DAY 6								//Day of the week of the first day of 2000 year

/*Private global variables*/
static const uint8_t days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

/*Private functions*/
static uint8_t dec_to_bcd_conversion(uint8_t value){
	//Sanity check
	if(value > DEC_MAX_VALUE){
		return 0xFF;								//Return max value supported by uint8_t
	}

	//BCD conversion for 2 digit decimal number
	uint8_t bcd_value;
	bcd_value = ((value / 10) << 4);
	bcd_value |= (value % 10);
	return bcd_value;
}

static uint8_t bcd_to_dec_conversion(uint8_t value){
	//Sanity check
	uint8_t bcd_digit1 = ((value & 0xF0) >> 4);
	uint8_t bcd_digit2 = (value & 0x0F);
	if(bcd_digit1 > BCD_MAX_VALUE || bcd_digit2 > BCD_MAX_VALUE){
		return 0xFF;
	}

	//Decimal conversion for 2 digit bcd number
	return bcd_digit1 * 10 + bcd_digit2;
}

static bool is_leap_year(uint16_t year){
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

static ds3231_alarm1_mode_t alarm1_mode_map(rtc_alarm1_mode_t rtc_alarm_mode){
	switch(rtc_alarm_mode){
		case RTC_ALARM1_MATCH_EVERY_SEC: 		return DS3231_ALARM1_MATCH_EVERY_SEC;
		case RTC_ALARM1_MATCH_SEC:				return DS3231_ALARM1_MATCH_SEC;
		case RTC_ALARM1_MATCH_MIN_SEC:			return DS3231_ALARM1_MATCH_MIN_SEC;
		case RTC_ALARM1_MATCH_HR_MIN_SEC:		return DS3231_ALARM1_MATCH_HR_MIN_SEC;
		case RTC_ALARM1_MATCH_DT_HR_MIN_SEC:	return DS3231_ALARM1_MATCH_DT_HR_MIN_SEC;
		case RTC_ALARM1_MATCH_DY_HR_MIN_SEC:	return DS3231_ALARM1_MATCH_DY_HR_MIN_SEC;
		default:								return DS3231_ALARM1_MATCH_UNKNOWN;
	}
}

static ds3231_alarm2_mode_t alarm2_mode_map(rtc_alarm2_mode_t rtc_alarm_mode){
	switch(rtc_alarm_mode){
		case RTC_ALARM2_MATCH_EVERY_MIN: 		return DS3231_ALARM2_MATCH_EVERY_MIN;
		case RTC_ALARM2_MATCH_MIN:				return DS3231_ALARM2_MATCH_MIN;
		case RTC_ALARM2_MATCH_HR_MIN:			return DS3231_ALARM2_MATCH_HR_MIN;
		case RTC_ALARM2_MATCH_DT_HR_MIN:		return DS3231_ALARM2_MATCH_DT_HR_MIN;
		case RTC_ALARM2_MATCH_DY_HR_MIN:		return DS3231_ALARM2_MATCH_DY_HR_MIN;
		default:								return DS3231_ALARM2_MATCH_UNKNOWN;
	}
}

static uint32_t rtc_to_timestamp(const rtc_datetime_t *dt){
	uint32_t days = 0;

	//Years
	for(uint16_t y = RTC_EPOCH_YEAR; y < dt->year; y++){
		days += is_leap_year(y) ? 366 : 365;
	}

	//Month
	for(uint8_t m = 1; m < dt->month; m++){
		days += days_in_month[m - 1];
		if(m == 2 && is_leap_year(dt->year)){
			days += 1;
		}
	}

	//Days
	days += dt->date - 1;

	//Convert to seconds
	uint32_t seconds = days * 86400;
	seconds += dt->hours * 3600;
	seconds += dt->minutes * 60;
	seconds += dt->seconds;

	return seconds;
}

static void timestamp_to_rtc(uint32_t timestamp, rtc_datetime_t *dt){
	//Days
	uint32_t days = timestamp / 86400;
	uint32_t rem  = timestamp % 86400;

	//Hours, minutes and seconds
	dt->hours = rem / 3600;
	rem %= 3600;

	dt->minutes = rem / 60;
	dt->seconds = rem % 60;

	//Year
	dt->year = RTC_EPOCH_YEAR;
	uint16_t days_in_year;
	while(1){
		days_in_year = is_leap_year(dt->year) ? 366 : 365;
		if(days < days_in_year) break;
		days -= days_in_year;
		dt->year++;
	}

	//Month
	dt->month = 1;
	uint8_t dim;
	while(1){
		dim = days_in_month[dt->month - 1];
		if(dt->month == 2 && is_leap_year(dt->year)){
			dim += 1;
		}

		if(days < dim) break;
		days -= dim;
		dt->month++;
	}

	//Date
	dt->date = days + 1;

	//Day
	dt->day = ((RTC_EPOCH_DAY - 1 + (timestamp / 86400)) % 7) + 1;
}

/*API functions*/
rtc_err_t rtc_init(rtc_t *rtc, ds3231_t *dev){
	//Sanity check
	if(!rtc || !dev || !dev->hi2c){
		return RTC_ERR_NULL;
	}

	//Assign ds3231 to this RTC instance
	rtc->dev = dev;

	//Give some time to the RTC to turn on
	HAL_Delay(1000);

	//Disable 32 kHz output
	ds3231_err_t err;
	err = ds3231_disable_en32kHz(dev);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	err = ds3231_enable_alarm_interrupt_output(dev);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	//Disable interrupts
	err = ds3231_disable_alarm(dev, DS3231_ALARM1);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	err = ds3231_disable_alarm(dev, DS3231_ALARM2);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	//Clear interrupt flags
	err = ds3231_clear_alarm_flag(dev, DS3231_ALARM1);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	err = ds3231_clear_alarm_flag(dev, DS3231_ALARM2);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}

rtc_err_t rtc_set_datetime(rtc_t *rtc, const rtc_datetime_t *dt){
	//Sanity check
	if(!rtc || !rtc->dev || !dt){
		return RTC_ERR_NULL;
	}

	//Range check
	if(dt->seconds > 59   ||
	   dt->minutes > 59   ||
	   dt->hours   > 23   ||
	   dt->day     < 1    || dt->day  > 7   ||
	   dt->date    < 1    || dt->date > 31  ||
	   dt->month   < 1    || dt->month > 12 ||
	   dt->year    < 2000 || dt->year > 2099   )
	{
		return RTC_ERR_INVALID_DATETIME;
	}

	//Do decimal to BCD conversion
	ds3231_datetime_t ds3231_dt;
	ds3231_dt.seconds = dec_to_bcd_conversion(dt->seconds);
	ds3231_dt.minutes = dec_to_bcd_conversion(dt->minutes);
	ds3231_dt.hours   = dec_to_bcd_conversion(dt->hours);
	ds3231_dt.day     = dec_to_bcd_conversion(dt->day);
	ds3231_dt.date    = dec_to_bcd_conversion(dt->date);
	ds3231_dt.month   = dec_to_bcd_conversion(dt->month);
	ds3231_dt.year    = dec_to_bcd_conversion(dt->year - 2000);

	ds3231_err_t err = ds3231_set_datetime(rtc->dev, &ds3231_dt);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}


rtc_err_t rtc_get_datetime(rtc_t *rtc, rtc_datetime_t *dt){
	//Sanity check
	if(!rtc || !rtc->dev || !dt){
		return RTC_ERR_NULL;
	}

	//Read DS3231M datetime
	ds3231_datetime_t ds3231_dt;
	ds3231_err_t err = ds3231_get_datetime(rtc->dev, &ds3231_dt);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	//Do BCD to decimal conversion
	dt->seconds = bcd_to_dec_conversion(ds3231_dt.seconds);
	dt->minutes = bcd_to_dec_conversion(ds3231_dt.minutes);
	dt->hours   = bcd_to_dec_conversion(ds3231_dt.hours);
	dt->day     = bcd_to_dec_conversion(ds3231_dt.day);
	dt->date    = bcd_to_dec_conversion(ds3231_dt.date);
	dt->month   = bcd_to_dec_conversion(ds3231_dt.month);
	dt->year    = bcd_to_dec_conversion(ds3231_dt.year) + 2000;

	return RTC_OK;
}

rtc_err_t rtc_set_alarm1(rtc_t *rtc, rtc_alarm1_config_t *cfg){
	//Sanity check
	if(!rtc || !rtc->dev || !cfg){
		return RTC_ERR_NULL;
	}

	if(cfg->seconds > 59 ||
	   cfg->minutes > 59 ||
	   cfg->hours   > 23 ||
	   (cfg->mode == RTC_ALARM1_MATCH_DT_HR_MIN_SEC && (cfg->dydt < 1  || cfg->dydt > 31)) ||
	   (cfg->mode == RTC_ALARM1_MATCH_DY_HR_MIN_SEC && (cfg->dydt < 1  || cfg->dydt > 7 ))		)
	{
		return RTC_ERR_INVALID_DATETIME;
	}

	//Decimal to BCD conversion
	ds3231_alarm1_config_t ds3231_cfg;
	ds3231_cfg.seconds = dec_to_bcd_conversion(cfg->seconds);
	ds3231_cfg.minutes = dec_to_bcd_conversion(cfg->minutes);
	ds3231_cfg.hours   = dec_to_bcd_conversion(cfg->hours);
	ds3231_cfg.dydt    = dec_to_bcd_conversion(cfg->dydt);
	ds3231_cfg.mode    = alarm1_mode_map(cfg->mode);

	//Set alarm1 on ds3231M
	ds3231_err_t err = ds3231_set_alarm1(rtc->dev, &ds3231_cfg);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}

rtc_err_t rtc_set_alarm2(rtc_t *rtc, rtc_alarm2_config_t *cfg){
	//Sanity check
	if(!rtc || !rtc->dev || !cfg){
		return RTC_ERR_NULL;
	}

	if(cfg->minutes > 59 ||
	   cfg->hours   > 23 ||
	   (cfg->mode == RTC_ALARM2_MATCH_DT_HR_MIN && (cfg->dydt < 1  || cfg->dydt > 31)) ||
	   (cfg->mode == RTC_ALARM2_MATCH_DY_HR_MIN && (cfg->dydt < 1  || cfg->dydt > 7 ))		)
	{
		return RTC_ERR_INVALID_DATETIME;
	}

	//Decimal to BCD conversion
	ds3231_alarm2_config_t ds3231_cfg;
	ds3231_cfg.minutes = dec_to_bcd_conversion(cfg->minutes);
	ds3231_cfg.hours   = dec_to_bcd_conversion(cfg->hours);
	ds3231_cfg.dydt    = dec_to_bcd_conversion(cfg->dydt);
	ds3231_cfg.mode    = alarm2_mode_map(cfg->mode);

	//Set alarm1 on ds3231M
	ds3231_err_t err = ds3231_set_alarm2(rtc->dev, &ds3231_cfg);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}

rtc_err_t rtc_enable_alarm(rtc_t *rtc, rtc_alarm_id_t alarm_id){
	//Sanity check
	if(!rtc || !rtc->dev){
		return RTC_ERR_NULL;
	}

	if(alarm_id != RTC_ALARM1 && alarm_id != RTC_ALARM2){
		return RTC_ERR_INVALID_ALARM;
	}

	//Enable alarm
	ds3231_err_t err;
	if(alarm_id == RTC_ALARM1){
		err = ds3231_enable_alarm(rtc->dev, DS3231_ALARM1);
	}
	else{
		err = ds3231_enable_alarm(rtc->dev, DS3231_ALARM2);
	}

	//Handle error
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}

rtc_err_t rtc_disable_alarm(rtc_t *rtc, rtc_alarm_id_t alarm_id){
	//Sanity check
	if(!rtc || !rtc->dev){
		return RTC_ERR_NULL;
	}

	if(alarm_id != RTC_ALARM1 && alarm_id != RTC_ALARM2){
		return RTC_ERR_INVALID_ALARM;
	}

	//Disable alarm
	ds3231_err_t err;
	if(alarm_id == RTC_ALARM1){
		err = ds3231_disable_alarm(rtc->dev, DS3231_ALARM1);
	}
	else{
		err = ds3231_disable_alarm(rtc->dev, DS3231_ALARM2);
	}

	//Handle error
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}

rtc_err_t rtc_get_alarm_flags(rtc_t *rtc, rtc_alarm_flag_t *flag){
	//Sanity check
	if(!rtc || !rtc->dev || !flag){
		return RTC_ERR_NULL;
	}

	//Read alarm flags
	bool alarm1_en_flag, alarm2_en_flag;
	ds3231_err_t err;

	err = ds3231_check_alarm_triggered(rtc->dev, DS3231_ALARM1, &alarm1_en_flag);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	err = ds3231_check_alarm_triggered(rtc->dev, DS3231_ALARM2, &alarm2_en_flag);
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	//Check how many alarms are activated
	if(alarm1_en_flag && alarm2_en_flag)       *flag = RTC_ALARM_FLAG_BOTH_UP;
	else if(alarm1_en_flag && !alarm2_en_flag) *flag = RTC_ALARM_FLAG_1_UP;
	else if(!alarm1_en_flag && alarm2_en_flag) *flag = RTC_ALARM_FLAG_2_UP;
	else                                       *flag = RTC_ALARM_FLAG_NONE;

	return RTC_OK;
}

rtc_err_t rtc_clear_alarm_flag(rtc_t *rtc, rtc_alarm_flag_t flag){
	//Sanity check
	if(!rtc || !rtc->dev){
		return RTC_ERR_NULL;
	}

	//Clear selected flag
	ds3231_err_t err;

	switch(flag){
	case RTC_ALARM_FLAG_1_UP:
		err = ds3231_clear_alarm_flag(rtc->dev, DS3231_ALARM1);
		break;

	case RTC_ALARM_FLAG_2_UP:
		err = ds3231_clear_alarm_flag(rtc->dev, DS3231_ALARM2);
		break;

	case RTC_ALARM_FLAG_BOTH_UP:
		err = ds3231_clear_alarm_flag(rtc->dev, DS3231_ALARM1);
		err = ds3231_clear_alarm_flag(rtc->dev, DS3231_ALARM2);
		break;

	case RTC_ALARM_FLAG_NONE:
		//Do nothing
		break;

	default:
		//CLear both alarms
		err = ds3231_clear_alarm_flag(rtc->dev, DS3231_ALARM1);
		err = ds3231_clear_alarm_flag(rtc->dev, DS3231_ALARM2);
		break;
	}

	//Handle errors
	if(err != DS3231_OK){
		if(err == DS3231_ERR_NULL) return RTC_ERR_NULL;
		else if(err == DS3231_ERR_TIMEOUT) return RTC_ERR_TIMEOUT;
		else return RTC_ERR_BUS;
	}

	return RTC_OK;
}

void rtc_add_time(rtc_datetime_t *dt, uint32_t seconds){
	uint32_t timestamp;
	timestamp = rtc_to_timestamp(dt);

	//Add seconds

    timestamp += seconds;
	timestamp_to_rtc(timestamp, dt);
}

void rtc_substract_time(rtc_datetime_t *dt, uint32_t seconds){
	uint32_t timestamp;
	timestamp = rtc_to_timestamp(dt);

	//Add seconds
	timestamp -= seconds;

	timestamp_to_rtc(timestamp, dt);
}




