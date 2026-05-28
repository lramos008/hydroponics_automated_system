/*Includes*/
#include "analog_manager.h"

/*Public functions*/
analog_manager_status_t analog_manager_init(analog_manager_t *hman){
	//Sanity check
	if(hman == NULL) 				return ANALOG_MANAGER_ERR_NULL;
	if(hman->channels == NULL) 		return ANALOG_MANAGER_ERR_NULL;
	if(hman->dma_buffer == NULL) 	return ANALOG_MANAGER_ERR_NULL;
	for(uint8_t i = 0; i < hman->channels_count; i++){
		if(hman->channels[i].filter == NULL) return ANALOG_MANAGER_ERR_NULL;
		hman->filtered_voltage[i] = 0.0f;
		moving_average_reset_filter(hman->channels[i].filter);
	}
	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_start(analog_manager_t *hman){
	//Sanity check
	if(hman == NULL) 				return ANALOG_MANAGER_ERR_NULL;
	if(hman->hadc == NULL) 			return ANALOG_MANAGER_ERR_NULL;
	if(hman->dma_buffer == NULL) 	return ANALOG_MANAGER_ERR_NULL;

	HAL_StatusTypeDef status;
	status = HAL_ADC_Start_DMA(hman->hadc, (uint32_t *) hman->dma_buffer, hman->channels_count);
	if(status != HAL_OK) return ANALOG_MANAGER_ERR_ADC_START;

	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_stop(analog_manager_t *hman){
	//Sanity check
	if(hman == NULL) 				return ANALOG_MANAGER_ERR_NULL;
	if(hman->hadc == NULL) 			return ANALOG_MANAGER_ERR_NULL;

	HAL_StatusTypeDef status;
	status = HAL_ADC_Stop_DMA(hman->hadc);
	if(status != HAL_OK) return ANALOG_MANAGER_ERR_ADC_STOP;

	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_update(analog_manager_t *hman){
	//Sanity check
	if(hman == NULL) 					return ANALOG_MANAGER_ERR_NULL;
	if(hman->channels == NULL) 			return ANALOG_MANAGER_ERR_NULL;
	if(hman->dma_buffer == NULL) 		return ANALOG_MANAGER_ERR_NULL;
	if(hman->filtered_voltage == NULL) 	return ANALOG_MANAGER_ERR_NULL;

	//Process channels
	for(uint8_t i = 0; i < hman->channels_count; i++){
		//Select channel
		analog_channel_t *channel = &hman->channels[i];

		//Get raw measurement
		uint16_t raw_value = hman->dma_buffer[channel->dma_index];

		//Convert to voltage
		float voltage = ((float) raw_value * channel->vref) / channel->adc_resolution;

		//Process sample with the moving average filter
		moving_average_process(channel->filter, voltage);

		//Set filtered value on filtered voltage buffer
		hman->filtered_voltage[i] = moving_average_get_value(channel->filter);
	}
	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_reset_filter(analog_manager_t *hman, uint8_t channel){
	//Sanity check
	if(hman == NULL)					return ANALOG_MANAGER_ERR_NULL;
	if(hman->channels == NULL)			return ANALOG_MANAGER_ERR_NULL;
	if(channel >= hman->channels_count) return ANALOG_MANAGER_ERR_INVALID_CHANNEL;

	//Select filter to reset
	moving_average_handle_t *filter = hman->channels[channel].filter;
	if(filter == NULL) 					return ANALOG_MANAGER_ERR_NULL;

	//Reset
	moving_average_reset_filter(filter);
	hman->filtered_voltage[channel] = 0.0f;

	return ANALOG_MANAGER_OK;
}

float analog_manager_get_filtered_voltage(analog_manager_t *hman, uint8_t channel){
	//Sanity check
	if(hman == NULL) 					return ANALOG_MANAGER_ERR_NULL;
	if(hman->filtered_voltage == NULL) 	return ANALOG_MANAGER_ERR_NULL;
	if(channel >= hman->channels_count)	return ANALOG_MANAGER_ERR_INVALID_CHANNEL;

	return hman->filtered_voltage[channel];
}

bool analog_manager_is_filter_ready(analog_manager_t *hman, uint8_t channel){
	if(hman == NULL) 					return ANALOG_MANAGER_ERR_NULL;
	if(hman->channels == NULL)			return ANALOG_MANAGER_ERR_NULL;
	if(channel >= hman->channels_count) return ANALOG_MANAGER_ERR_INVALID_CHANNEL;

	moving_average_handle_t *filter = hman->channels[channel].filter;
	return filter->is_ready;
}
