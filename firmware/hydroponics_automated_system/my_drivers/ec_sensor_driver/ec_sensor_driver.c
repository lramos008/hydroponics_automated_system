#include <ec_sensor_driver/ec_sensor_driver.h>

/*Defines*/
#define EC_SENSOR_DRIVER_ADC_POLL_TIMEOUT 10

/*Private functions*/
static ec_sensor_driver_status_t _ec_sensor_driver_read_raw_value(ec_sensor_driver_t *dev, uint16_t *raw_value){
	//Set config structure
	ADC_ChannelConfTypeDef cfg = {0};
	cfg.Channel = dev->adc_channel;
	cfg.Rank    = 1;									//Single mode, always first
	cfg.SamplingTime = ADC_SAMPLETIME_15CYCLES;

	//Apply ADC config
	if(HAL_ADC_ConfigChannel(dev->hadc, &cfg) != HAL_OK) return EC_SENSOR_DRIVER_ERR_ADC_CONFIG;

	//Init ADC conversion
	HAL_ADC_Start(dev->hadc);
	if(HAL_ADC_PollForConversion(dev->hadc, EC_SENSOR_DRIVER_ADC_POLL_TIMEOUT) == HAL_OK){
		*raw_value = (uint16_t) HAL_ADC_GetValue(dev->hadc);
		return EC_SENSOR_DRIVER_OK;
	}
	return EC_SENSOR_DRIVER_ERR_ADC_READING;
}
//static gravity_tds_err_t _gravity_tds_read_adc(gravity_tds_handle_t *dev, uint32_t *raw_value){
//	//Set config structure
//	ADC_ChannelConfTypeDef cfg = {0};
//	cfg.Channel = dev->channel;
//	cfg.Rank    = 1;									//Single mode, always first
//	cfg.SamplingTime = ADC_SAMPLETIME_15CYCLES;
//
//	//Apply ADC config
//	if(HAL_ADC_ConfigChannel(dev->hadc, &cfg) != HAL_OK) return GRAVITY_ERR_ADC_CONFIG;
//
//	//Init ADC conversion
//	HAL_ADC_Start(dev->hadc);
//	if(HAL_ADC_PollForConversion(dev->hadc, 10) == HAL_OK){
//		*raw_value = HAL_ADC_GetValue(dev->hadc);
//		return GRAVITY_OK;
//	}
//	return GRAVITY_ERR_ADC_READING;													//read error
//}

//static gravity_tds_err_t _gravity_tds_update(gravity_tds_handle_t *dev){
//	//Get a measurement and add it to the internal buffer
//	gravity_tds_err_t err;
//	uint32_t raw_value;
//	err = _gravity_tds_read_adc(dev, &raw_value);
//	if(err != GRAVITY_OK) return err;
//
//	//Add latests measurement to the buffer
//	dev->adc_buffer[dev->buffer_idx++] = raw_value;
//	if(dev->buffer_idx >= GRAVITY_TDS_INTERNAL_BUFFER_SIZE){
//		dev->buffer_idx = 0;														//Restart circular buffer index
//	}
//
//	//Average calculations
//	uint32_t sum = 0;
//	for(uint32_t i = 0; i < GRAVITY_TDS_INTERNAL_BUFFER_SIZE; i++){
//		sum += dev->adc_buffer[i];
//	}
//
//	float avg_adc = (float) sum / GRAVITY_TDS_INTERNAL_BUFFER_SIZE;
//
//	//Save last voltage value
//	dev->last_voltage = (avg_adc * ((float) dev->vref)) / ((float)dev->adc_range);
//
//	//Calculate compensated voltage by temperature
//	float compensation_coeff = 1.0 + 0.02 * (dev->solution_temp - 25.0);
//	float v_25 = dev->last_voltage / compensation_coeff;
//
//	/*Calculate base EC. This polynom was designed to output a ppm value.
//	 * In order to get EC you need to do a conversion. DFRobot used the
//	 * NaCl scale where an EC of 1.0 mS/cm is equivalent to 500 ppm.
//	 * Thus that factor is used to normalize the EC value.
//	 */
//	float base_ec = (133.42 * v_25 * v_25 * v_25 -
//					 255.86 * v_25 * v_25 +
//					 857.39 * v_25) / 500.0f;
//
//	//Apply calibration factor (due to sensor imperfections)
//	dev->last_ec = base_ec * dev->k_value;
//	return GRAVITY_OK;
//}

/*Public functions*/
ec_sensor_driver_status_t ec_sensor_driver_init(ec_sensor_driver_t *dev, ADC_HandleTypeDef *hadc, uint32_t adc_channel, uint32_t adc_range, float vref){
	//Sanity check
	if(!dev || !hadc) return EC_SENSOR_DRIVER_ERR_NULL;

	//Init handle
	dev->hadc 		 = hadc;
	dev->adc_channel = adc_channel;
	dev->adc_range 	 = adc_range;
	dev->vref      	 = vref;
	return EC_SENSOR_DRIVER_OK;
}

ec_sensor_driver_status_t ec_sensor_driver_read_voltage(ec_sensor_driver_t *dev, float *voltage){
	//Sanity check
	if(!dev || !dev->hadc)	return EC_SENSOR_DRIVER_ERR_NULL;

	//Read raw value
	uint16_t raw_value;
	ec_sensor_driver_status_t status = _ec_sensor_driver_read_raw_value(dev, &raw_value);
	if(status != EC_SENSOR_DRIVER_OK) return status;

	//Convert to voltage
	*voltage = (float) raw_value * dev->vref / (float) dev->adc_range;
	return EC_SENSOR_DRIVER_OK;
}
/*ec_sensor_driver_status_t ec_sensor_driver_update(ec_sensor_driver_handle_t *handle){
	//Sanity check
	if(!handle || !handle->hadc)	return EC_SENSOR_DRIVER_ERR_NULL;

	//Read raw value
	uint16_t raw_value;
	ec_sensor_driver_status_t status = _ec_sensor_driver_read_raw_value(handle, &raw_value);
	if(status != EC_SENSOR_DRIVER_OK) return status;

	//Convert raw value to voltage
	float read_voltage = (float) raw_value * handle->vref / (float) handle->adc_range;

	//Update moving average filter
	moving_average_status_t filter_status = moving_average_process(&handle->hfilter, read_voltage);
	if(filter_status != MOVING_AVERAGE_OK) return EC_SENSOR_DRIVER_ERR_FILTER;

	//Wait until filter is ready
	if(!handle->hfilter.is_ready) return EC_SENSOR_DRIVER_OK;

	//Get filtered voltage
	filter_status = moving_average_get_value(&handle->hfilter, &handle->voltage);
	if(filter_status != MOVING_AVERAGE_OK) return
	return EC_SENSOR_DRIVER_OK;
}*/



