#include "light_sensor.h"

/*Defines*/

/*API functions*/
light_err_t light_sensor_init(light_sensor_t *light_sensor, bh1750_t *dev, bh1750_res_mode_t res_mode){
	//Sanity check
	if(!light_sensor || !dev || !dev->hi2c){
		return LIGHT_ERR_NULL;
	}

	//Init handle
	light_sensor->dev = dev;
	light_sensor->res_mode = res_mode;

	//Set sensor into power on mode
	bh1750_err_t err = bh1750_power_on(light_sensor->dev);
	if(err != BH1750_OK){
		if(err == BH1750_ERR_NULL)    		return LIGHT_ERR_NULL;
		else if(err == BH1750_ERR_TIMEOUT) 	return LIGHT_ERR_TIMEOUT;
		else						  		return LIGHT_ERR_BUS;
	}

	//Reset lux data register
	err = bh1750_reset(light_sensor->dev);
	if(err != BH1750_OK){
		if(err == BH1750_ERR_NULL)    		return LIGHT_ERR_NULL;
		else if(err == BH1750_ERR_TIMEOUT) 	return LIGHT_ERR_TIMEOUT;
		else						  		return LIGHT_ERR_BUS;
	}

	//Set to power down mode
	err = bh1750_power_down(light_sensor->dev);
	if(err != BH1750_OK){
		if(err == BH1750_ERR_NULL)    		return LIGHT_ERR_NULL;
		else if(err == BH1750_ERR_TIMEOUT) 	return LIGHT_ERR_TIMEOUT;
		else						  		return LIGHT_ERR_BUS;
	}
	return LIGHT_OK;
}

light_err_t light_sensor_read(light_sensor_t *light_sensor, float *lux){
	//Sanity check
	if(!light_sensor || !lux){
		return LIGHT_ERR_NULL;
	}

	//Set light sensor into waiting for measurement command state
	bh1750_err_t err = bh1750_power_on(light_sensor->dev);
	if(err != BH1750_OK){
		if(err == BH1750_ERR_NULL)    		return LIGHT_ERR_NULL;
		else if(err == BH1750_ERR_TIMEOUT) 	return LIGHT_ERR_TIMEOUT;
		else						  		return LIGHT_ERR_BUS;
	}

	//Start a measurement
	err = bh1750_start_measurement(light_sensor->dev, light_sensor->res_mode);
	if(err != BH1750_OK){
		if(err == BH1750_ERR_NULL)    		return LIGHT_ERR_NULL;
		else if(err == BH1750_ERR_TIMEOUT) 	return LIGHT_ERR_TIMEOUT;
		else						  		return LIGHT_ERR_BUS;
	}

	//Read raw value
	uint16_t raw_value;
	err = bh1750_read_raw_measurement(light_sensor->dev, &raw_value);		//It goes to power down mode after a oneshot measurement
	if(err != BH1750_OK){
		if(err == BH1750_ERR_NULL)    		return LIGHT_ERR_NULL;
		else if(err == BH1750_ERR_TIMEOUT) 	return LIGHT_ERR_TIMEOUT;
		else						  		return LIGHT_ERR_BUS;
	}

	//Convert to lux
	*lux = bh1750_convert_raw_to_lux(raw_value);
	return LIGHT_OK;
}


