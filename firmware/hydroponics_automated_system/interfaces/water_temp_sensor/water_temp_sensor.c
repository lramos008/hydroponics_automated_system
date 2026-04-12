/*Includes*/
#include "water_temp_sensor.h"

/*API functions*/
water_temp_err_t water_temp_sensor_init(water_temp_sensor_t *sensor, ds18b20_t *dev){
	//Sanity check
	if(!sensor || !dev || !dev->ow_bus){
		return WATER_TEMP_ERR_NULL;
	}

	//Init handle
	sensor->dev = dev;

	//Set resolution
	ds18b20_err_t err = ds18b20_set_resolution(sensor->dev, sensor->dev->resolution);
	if(err != DS18B20_OK){
		return WATER_TEMP_ERR_INIT;
	}
	return WATER_TEMP_OK;
}

water_temp_err_t water_temp_sensor_request(water_temp_sensor_t *sensor){
	if(!sensor || !sensor->dev || !sensor->dev->ow_bus){
		return WATER_TEMP_ERR_NULL;
	}

	//Start temp conversion
	ds18b20_err_t err = ds18b20_start_temperature_conversion(sensor->dev);
	if(err != DS18B20_OK){
		if(err == DS18B20_ERR_NO_PRESENCE) return WATER_TEMP_ERR_SENSOR_DISCONNECTED;
		else 							   return WATER_TEMP_ERR_COMM;
	}
	return WATER_TEMP_OK;
}

water_temp_err_t water_temp_is_sensor_ready(water_temp_sensor_t *sensor){
	if(!sensor || !sensor->dev || !sensor->dev->ow_bus){
		return WATER_TEMP_ERR_NULL;
	}

	//Check if conversion ended
	ds18b20_err_t err = ds18b20_is_conversion_ready(sensor->dev);
	return (err == DS18B20_CONVERSION_NOT_READY) ? WATER_TEMP_NOT_READY : WATER_TEMP_OK;
}

water_temp_err_t water_temp_sensor_read(water_temp_sensor_t *sensor, float *temp){
	if(!sensor || !sensor->dev || !sensor->dev->ow_bus || !temp){
		return WATER_TEMP_ERR_NULL;
	}

	int16_t raw_temp;
	ds18b20_err_t err = ds18b20_read_raw_temperature(sensor->dev, &raw_temp);
	if(err != DS18B20_OK){
		if(err == DS18B20_ERR_NO_PRESENCE) return WATER_TEMP_ERR_SENSOR_DISCONNECTED;
		else 							   return WATER_TEMP_ERR_INVALID_DATA;
	}

	if(sensor->dev->resolution == DS18B20_12_BIT_RESOLUTION){
		*temp = raw_temp * DS18B20_12_BITS_RESOLUTION_STEP;
	}
	else if(sensor->dev->resolution == DS18B20_11_BIT_RESOLUTION){
		*temp = raw_temp * DS18B20_11_BITS_RESOLUTION_STEP;
	}
	else if(sensor->dev->resolution == DS18B20_10_BIT_RESOLUTION){
		*temp = raw_temp * DS18B20_10_BITS_RESOLUTION_STEP;
	}
	else if(sensor->dev->resolution == DS18B20_9_BIT_RESOLUTION){
		*temp = raw_temp * DS18B20_9_BITS_RESOLUTION_STEP;
	}
	return WATER_TEMP_OK;
}


