/*Includes*/
#include "temp_hum_sensor.h"

/*Defines*/

/*API functions*/
temp_hum_err_t temp_hum_sensor_init(temp_hum_sensor_t *sensor, sht30_t *dev, sht30_repeatability_t rep){
	//Sanity check
	if(!sensor || !dev || !dev->hi2c){
		return TEMP_HUM_ERR_NULL;
	}

	//Init handle
	sensor->dev = dev;
	sensor->repeatability = rep;
	return TEMP_HUM_OK;
}

temp_hum_err_t temp_hum_sensor_read(temp_hum_sensor_t *sensor, float *temp, float *hr){
	//Sanity check
	if(!sensor || !temp || !hr){
		return TEMP_HUM_ERR_NULL;
	}

	//Start measurement
	sht30_err_t err = sht30_start_measurement(sensor->dev, sensor->repeatability, SHT30_CLK_STRETCHING_DISABLED);
	if(err != SHT30_OK){
		if(err == SHT30_ERR_NULL) 			return TEMP_HUM_ERR_NULL;
		else if(err == SHT30_ERR_TIMEOUT) 	return TEMP_HUM_ERR_TIMEOUT;
		else						 		return TEMP_HUM_ERR_BUS;
	}

	//Read measurement
	uint16_t temp_raw, hr_raw;
	err = sht30_get_raw_measurement(sensor->dev, &temp_raw, &hr_raw);
	if(err != SHT30_OK){
		if(err == SHT30_ERR_NULL) 			return TEMP_HUM_ERR_NULL;
		else if(err == SHT30_ERR_TIMEOUT) 	return TEMP_HUM_ERR_TIMEOUT;
		else						 		return TEMP_HUM_ERR_BUS;
	}

	//Conversion to readable format
	*temp = sht30_convert_raw_to_temperature(temp_raw);
	*hr   = sht30_convert_raw_to_hr(hr_raw);
	return TEMP_HUM_OK;
}
