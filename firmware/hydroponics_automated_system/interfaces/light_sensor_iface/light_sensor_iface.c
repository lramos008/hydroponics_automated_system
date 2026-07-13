#include "light_sensor_iface.h"

/*Private functions*/
static light_sensor_iface_status_t light_sensor_iface_translate_driver_status(bh1750_status_t status){
	switch(status){
	case BH1750_STATUS_OK:						return LIGHT_SENSOR_IFACE_STATUS_OK;
	case BH1750_STATUS_BUSY:					return LIGHT_SENSOR_IFACE_STATUS_BUSY;
	case BH1750_STATUS_ERR_NULL_POINTER:		return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	default:									return LIGHT_SENSOR_IFACE_STATUS_ERR_DRIVER;
	}
}

/*API functions*/
light_sensor_iface_status_t light_sensor_iface_init(light_sensor_iface_t *iface, bh1750_t *dev){
	if(iface == NULL)	return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	iface->is_initialized = false;
	if(dev == NULL)		return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;

	iface->dev = dev;
	iface->data.lux = 0.0f;
	iface->is_initialized = true;

	return LIGHT_SENSOR_IFACE_STATUS_OK;
}

light_sensor_iface_status_t light_sensor_iface_start_measurement(light_sensor_iface_t *iface){
	if(iface == NULL)			return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return LIGHT_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	return light_sensor_iface_translate_driver_status(bh1750_start_measurement(iface->dev));
}

light_sensor_iface_status_t light_sensor_iface_process(light_sensor_iface_t *iface){
	if(iface == NULL)			return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return LIGHT_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	return light_sensor_iface_translate_driver_status(bh1750_process(iface->dev));
}

bool light_sensor_iface_is_ready(light_sensor_iface_t *iface){
	if(iface == NULL)			return false;
	if(!iface->is_initialized)	return false;

	return bh1750_is_ready(iface->dev);
}

light_sensor_iface_status_t light_sensor_iface_get_data(light_sensor_iface_t *iface, light_sensor_iface_data_t *data){
	if(iface == NULL)			return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(data == NULL)			return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return LIGHT_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	bh1750_data_t driver_data;
	light_sensor_iface_status_t status = light_sensor_iface_translate_driver_status(bh1750_get_data(iface->dev, &driver_data));
	if(status == LIGHT_SENSOR_IFACE_STATUS_BUSY){
		return LIGHT_SENSOR_IFACE_STATUS_NOT_READY;
	}
	if(status != LIGHT_SENSOR_IFACE_STATUS_OK){
		return status;
	}

	iface->data.lux = driver_data.lux;
	data->lux = iface->data.lux;

	return LIGHT_SENSOR_IFACE_STATUS_OK;
}

light_sensor_iface_status_t light_sensor_iface_reset(light_sensor_iface_t *iface){
	if(iface == NULL)			return LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)	return LIGHT_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	light_sensor_iface_status_t status = light_sensor_iface_translate_driver_status(bh1750_reset(iface->dev));
	if(status != LIGHT_SENSOR_IFACE_STATUS_OK){
		return status;
	}

	iface->data.lux = 0.0f;

	return LIGHT_SENSOR_IFACE_STATUS_OK;
}
