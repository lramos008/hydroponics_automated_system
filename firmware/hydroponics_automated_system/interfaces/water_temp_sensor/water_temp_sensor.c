/*Includes*/
#include "water_temp_sensor.h"

/*Private functions*/
static water_temp_err_t water_temp_sensor_translate_ds18b20_error(ds18b20_err_t err){
	switch(err){
	case DS18B20_OK:					return WATER_TEMP_OK;
	case DS18B20_BUSY:
	case DS18B20_CONVERSION_NOT_READY:	return WATER_TEMP_NOT_READY;
	case DS18B20_ERR_NULL:				return WATER_TEMP_ERR_NULL;
	case DS18B20_ERR_NO_PRESENCE:		return WATER_TEMP_ERR_SENSOR_DISCONNECTED;
	case DS18B20_ERR_CRC:				return WATER_TEMP_ERR_INVALID_DATA;
	case DS18B20_ERR_INVALID_ROM:
	case DS18B20_ERR_INVALID_RESOLUTION:
	case DS18B20_ERR_NOT_INITIALIZED:	return WATER_TEMP_ERR_INIT;
	default:							return WATER_TEMP_ERR_COMM;
	}
}

/*API functions*/
water_temp_err_t water_temp_sensor_init(water_temp_sensor_t *sensor, ds18b20_t *dev){
	//Sanity check
	if(!sensor || !dev || !dev->cfg.ow_bus || !dev->is_initialized){
		return WATER_TEMP_ERR_NULL;
	}

	//Init handle
	sensor->dev = dev;
	return WATER_TEMP_OK;
}

water_temp_err_t water_temp_sensor_request(water_temp_sensor_t *sensor){
	if(!sensor || !sensor->dev || !sensor->dev->cfg.ow_bus){
		return WATER_TEMP_ERR_NULL;
	}

	//Start temp conversion
	ds18b20_err_t err = ds18b20_start_measurement(sensor->dev);
	if(err != DS18B20_OK){
		return water_temp_sensor_translate_ds18b20_error(err);
	}
	return WATER_TEMP_OK;
}

water_temp_err_t water_temp_is_sensor_ready(water_temp_sensor_t *sensor){
	if(!sensor || !sensor->dev || !sensor->dev->cfg.ow_bus){
		return WATER_TEMP_ERR_NULL;
	}

	ds18b20_err_t err = ds18b20_process(sensor->dev);
	if(err == DS18B20_ERROR){
		return water_temp_sensor_translate_ds18b20_error(sensor->dev->error_cause);
	}

	return ds18b20_is_ready(sensor->dev) ? WATER_TEMP_OK : WATER_TEMP_NOT_READY;
}

water_temp_err_t water_temp_sensor_read(water_temp_sensor_t *sensor, float *temp){
	if(!sensor || !sensor->dev || !sensor->dev->cfg.ow_bus || !temp){
		return WATER_TEMP_ERR_NULL;
	}

	ds18b20_data_t data;
	ds18b20_err_t err = ds18b20_get_data(sensor->dev, &data);
	if(err != DS18B20_OK){
		return water_temp_sensor_translate_ds18b20_error(err);
	}

	*temp = data.temperature;
	return WATER_TEMP_OK;
}


