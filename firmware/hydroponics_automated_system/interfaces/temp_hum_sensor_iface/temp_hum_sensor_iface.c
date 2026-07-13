#include "temp_hum_sensor_iface.h"

/*Private functions*/
static temp_hum_sensor_iface_status_t temp_hum_sensor_iface_translate_driver_status(sht30_status_t status){
	switch(status){
	case SHT30_STATUS_OK:					return TEMP_HUM_SENSOR_IFACE_STATUS_OK;
	case SHT30_STATUS_BUSY:					return TEMP_HUM_SENSOR_IFACE_STATUS_BUSY;
	case SHT30_STATUS_ERR_NULL_POINTER:		return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	case SHT30_STATUS_ERR_NOT_INITIALIZED:	return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;
	default:								return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_DRIVER;
	}
}

/*API functions*/
temp_hum_sensor_iface_status_t temp_hum_sensor_iface_init(temp_hum_sensor_iface_t *iface, sht30_t *dev){
	if(iface == NULL)	return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	iface->is_initialized = false;
	if(dev == NULL)		return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;

	iface->dev = dev;
	iface->data.temperature = 0.0f;
	iface->data.humidity = 0.0f;
	iface->is_initialized = true;

	return TEMP_HUM_SENSOR_IFACE_STATUS_OK;
}

temp_hum_sensor_iface_status_t temp_hum_sensor_iface_start_measurement(temp_hum_sensor_iface_t *iface){
	if(iface == NULL)			return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	return temp_hum_sensor_iface_translate_driver_status(sht30_start_measurement(iface->dev));
}

temp_hum_sensor_iface_status_t temp_hum_sensor_iface_process(temp_hum_sensor_iface_t *iface){
	if(iface == NULL)			return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	return temp_hum_sensor_iface_translate_driver_status(sht30_process(iface->dev));
}

bool temp_hum_sensor_iface_is_ready(temp_hum_sensor_iface_t *iface){
	if(iface == NULL)			return false;
	if(!iface->is_initialized)	return false;

	return sht30_is_ready(iface->dev);
}

temp_hum_sensor_iface_status_t temp_hum_sensor_iface_get_data(temp_hum_sensor_iface_t *iface, temp_hum_sensor_iface_data_t *data){
	if(iface == NULL)			return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(data == NULL)			return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	sht30_data_t driver_data;
	temp_hum_sensor_iface_status_t status = temp_hum_sensor_iface_translate_driver_status(sht30_get_data(iface->dev, &driver_data));
	if(status == TEMP_HUM_SENSOR_IFACE_STATUS_BUSY){
		return TEMP_HUM_SENSOR_IFACE_STATUS_NOT_READY;
	}
	if(status != TEMP_HUM_SENSOR_IFACE_STATUS_OK){
		return status;
	}

	iface->data.temperature = driver_data.temperature;
	iface->data.humidity = driver_data.humidity;
	data->temperature = iface->data.temperature;
	data->humidity = iface->data.humidity;

	return TEMP_HUM_SENSOR_IFACE_STATUS_OK;
}

temp_hum_sensor_iface_status_t temp_hum_sensor_iface_reset(temp_hum_sensor_iface_t *iface){
	if(iface == NULL)			return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	temp_hum_sensor_iface_status_t status = temp_hum_sensor_iface_translate_driver_status(sht30_reset(iface->dev));
	if(status != TEMP_HUM_SENSOR_IFACE_STATUS_OK){
		return status;
	}

	iface->data.temperature = 0.0f;
	iface->data.humidity = 0.0f;

	return TEMP_HUM_SENSOR_IFACE_STATUS_OK;
}
