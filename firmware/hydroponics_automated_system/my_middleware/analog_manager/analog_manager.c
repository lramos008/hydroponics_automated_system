/*Includes*/
#include "analog_manager.h"
#include <string.h>

analog_manager_status_t analog_manager_init(analog_manager_t *analog){
	//Sanity check
	if(analog == NULL) 		 return ANALOG_MANAGER_ERR_NULL;
	if(analog->hadc == NULL) return ANALOG_MANAGER_ERR_NULL;

	//Initialize internal flags
	analog->is_initialized = false;

	//Initialize internal filter status and buffers
	for(uint8_t ch = 0; ch < ANALOG_CHANNEL_COUNT; ch++){
		//Buffers
		analog->filtered_voltage[ch] = 0.0f;
		analog->dma_buffer[ch] = 0.0f;
		memset(analog->filter_buffers[ch], 0.0f, sizeof(float) * ANALOG_MOVING_AVG_FILTER_SIZE);
		//Filters
		if(moving_average_init(&analog->filters[ch], analog->filter_buffers[ch], ANALOG_MOVING_AVG_FILTER_SIZE) != MOVING_AVERAGE_OK){
			return ANALOG_MANAGER_ERR_FILTER_INIT;
		}
	}
	//The handle is now initialized
	analog->is_initialized = true;

	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_start(analog_manager_t *analog){
	if(analog == NULL)			return ANALOG_MANAGER_ERR_NULL;
	if(!analog->is_initialized) return ANALOG_MANAGER_ERR_NOT_INITIALIZED;

	//Start ADC scanning with DMA
	HAL_StatusTypeDef status = HAL_ADC_Start_DMA(analog->hadc, (uint32_t *)analog->dma_buffer, ANALOG_CHANNEL_COUNT);
	if(status != HAL_OK)		return ANALOG_MANAGER_ERR_ADC_START;

	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_stop(analog_manager_t *analog){
	if(analog == NULL)			return ANALOG_MANAGER_ERR_NULL;
	if(!analog->is_initialized) return ANALOG_MANAGER_ERR_NOT_INITIALIZED;

	//Stop ADC scanning with DMA
	HAL_StatusTypeDef status = HAL_ADC_Stop_DMA(analog->hadc);
	if(status != HAL_OK)		return ANALOG_MANAGER_ERR_ADC_STOP;

	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_update(analog_manager_t *analog){
	if(analog == NULL)			return ANALOG_MANAGER_ERR_NULL;
	if(!analog->is_initialized) return ANALOG_MANAGER_ERR_NOT_INITIALIZED;

	//Process channels
	for(uint8_t ch; ch < ANALOG_CHANNEL_COUNT; ch++){
		//Get raw measurement
		uint16_t raw_value = analog->dma_buffer[ch];
		//Convert to voltage
		float voltage = ((float) raw_value * analog->vref) / ((float)analog->adc_resolution);
		//Process sample with moving average filter
		if(moving_average_process(&analog->filters[ch], voltage) != MOVING_AVERAGE_OK){
			return ANALOG_MANAGER_ERR_FILTER_UPDATE;
		}
		//Set filtered value on the filtered voltage buffer
		analog->filtered_voltage[ch] = moving_average_get_value(&analog->filters[ch]);
	}

	return ANALOG_MANAGER_OK;
}

analog_manager_status_t analog_manager_reset_filter(analog_manager_t *analog, analog_channel_id_t channel){
	if(analog == NULL)					return ANALOG_MANAGER_ERR_NULL;
	if(!analog->is_initialized) 		return ANALOG_MANAGER_ERR_NOT_INITIALIZED;
	if(channel >= ANALOG_CHANNEL_COUNT) return ANALOG_MANAGER_ERR_INVALID_CHANNEL;

	//Reset moving average filter
	if(moving_average_reset_filter(&analog->filters[channel]) != MOVING_AVERAGE_OK){
		return ANALOG_MANAGER_ERR_FILTER_RESET;
	}

	//Reset filtered value at channel position
	analog->filtered_voltage[channel] = 0.0f;

	return ANALOG_MANAGER_OK;
}

float analog_manager_get_filtered_voltage(analog_manager_t *analog, analog_channel_id_t channel){
	if(analog == NULL)					return -1.0f;
	if(!analog->is_initialized)			return -1.0f;
	if(channel >= ANALOG_CHANNEL_COUNT) return -1.0f;

	return analog->filtered_voltage[channel];
}

bool analog_manager_is_filter_ready(analog_manager_t *analog, analog_channel_id_t channel){
	if(analog == NULL)					return false;
	if(!analog->is_initialized)			return false;
	if(channel >= ANALOG_CHANNEL_COUNT) return false;

	return analog->filters[channel].is_ready;
}

