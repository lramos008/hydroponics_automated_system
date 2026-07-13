/*Includes*/
#include "analog_manager.h"
#include <string.h>

static analog_manager_status_t analog_manager_validate_config(const analog_manager_config_t *cfg){
	if(cfg == NULL)				 							return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(cfg->hadc == NULL)	 								return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(cfg->vref <= 0.0f)	 								return ANALOG_MANAGER_STATUS_ERR_INVALID_CONFIG;
	if(cfg->adc_resolution == 0U)							return ANALOG_MANAGER_STATUS_ERR_INVALID_CONFIG;
	if(cfg->channel_count == 0U)							return ANALOG_MANAGER_STATUS_ERR_INVALID_CONFIG;
	if(cfg->channel_count > ANALOG_MANAGER_MAX_CHANNELS)	return ANALOG_MANAGER_STATUS_ERR_INVALID_CONFIG;

	for(uint8_t ch = 0; ch < cfg->channel_count; ch++){
		if(cfg->signal_map[ch] >= ANALOG_SIGNAL_COUNT){
			return ANALOG_MANAGER_STATUS_ERR_INVALID_SIGNAL;
		}

		for(uint8_t next_ch = ch + 1U; next_ch < cfg->channel_count; next_ch++){
			if(cfg->signal_map[ch] == cfg->signal_map[next_ch]){
				return ANALOG_MANAGER_STATUS_ERR_INVALID_CONFIG;
			}
		}
	}

	return ANALOG_MANAGER_STATUS_OK;
}

static analog_manager_status_t analog_manager_find_signal_index(analog_manager_t *mgr, analog_signal_id_t signal, uint8_t *index){
	if(mgr == NULL)						return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(index == NULL)					return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(!mgr->is_initialized)			return ANALOG_MANAGER_STATUS_ERR_NOT_INITIALIZED;
	if(signal >= ANALOG_SIGNAL_COUNT)	return ANALOG_MANAGER_STATUS_ERR_INVALID_SIGNAL;

	for(uint8_t ch = 0; ch < mgr->cfg.channel_count; ch++){
		if(mgr->cfg.signal_map[ch] == signal){
			*index = ch;
			return ANALOG_MANAGER_STATUS_OK;
		}
	}

	return ANALOG_MANAGER_STATUS_ERR_SIGNAL_NOT_FOUND;
}

/*Public functions*/
analog_manager_status_t analog_manager_init(analog_manager_t *mgr, const analog_manager_config_t *cfg){
	//Sanity check
	if(mgr == NULL) return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;

	analog_manager_status_t cfg_status = analog_manager_validate_config(cfg);
	if(cfg_status != ANALOG_MANAGER_STATUS_OK) return cfg_status;

	//Initialize internal flags
	mgr->is_initialized = false;
	mgr->is_running = false;
	mgr->cfg = *cfg;

	//Initialize internal filter status and buffers
	for(uint8_t ch = 0; ch < ANALOG_MANAGER_MAX_CHANNELS; ch++){
		//Buffers
		mgr->filtered_voltage[ch] = 0.0f;
		mgr->dma_buffer[ch] = 0U;
		memset(mgr->filter_buffers[ch], 0.0f, sizeof(float) * ANALOG_MOVING_AVG_FILTER_SIZE);
		//Filters
		if(moving_average_init(&mgr->filters[ch], mgr->filter_buffers[ch], ANALOG_MOVING_AVG_FILTER_SIZE) != MOVING_AVERAGE_OK){
			return ANALOG_MANAGER_STATUS_ERR_FILTER_INIT;
		}
	}
	//The handle is now initialized
	mgr->is_initialized = true;

	return ANALOG_MANAGER_STATUS_OK;
}

analog_manager_status_t analog_manager_start(analog_manager_t *mgr){
	if(mgr == NULL)				return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(!mgr->is_initialized)	return ANALOG_MANAGER_STATUS_ERR_NOT_INITIALIZED;
	if(mgr->is_running)			return ANALOG_MANAGER_STATUS_OK;

	//Start ADC scanning with DMA
	HAL_StatusTypeDef status = HAL_ADC_Start_DMA(mgr->cfg.hadc, (uint32_t *)mgr->dma_buffer, mgr->cfg.channel_count);
	if(status != HAL_OK)		return ANALOG_MANAGER_STATUS_ERR_ADC_START;

	mgr->is_running = true;
	return ANALOG_MANAGER_STATUS_OK;
}

analog_manager_status_t analog_manager_stop(analog_manager_t *mgr){
	if(mgr == NULL)				return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(!mgr->is_initialized)	return ANALOG_MANAGER_STATUS_ERR_NOT_INITIALIZED;
	if(!mgr->is_running)		return ANALOG_MANAGER_STATUS_OK;

	//Stop ADC scanning with DMA
	HAL_StatusTypeDef status = HAL_ADC_Stop_DMA(mgr->cfg.hadc);
	if(status != HAL_OK)		return ANALOG_MANAGER_STATUS_ERR_ADC_STOP;

	mgr->is_running = false;
	return ANALOG_MANAGER_STATUS_OK;
}

analog_manager_status_t analog_manager_process(analog_manager_t *mgr){
	if(mgr == NULL)				return ANALOG_MANAGER_STATUS_ERR_NULL_POINTER;
	if(!mgr->is_initialized)	return ANALOG_MANAGER_STATUS_ERR_NOT_INITIALIZED;
	if(!mgr->is_running)		return ANALOG_MANAGER_STATUS_ERR_NOT_STARTED;

	uint16_t samples[ANALOG_MANAGER_MAX_CHANNELS];
	for(uint8_t ch = 0; ch < mgr->cfg.channel_count; ch++){
		samples[ch] = mgr->dma_buffer[ch];
	}

	//Process channels
	for(uint8_t ch = 0; ch < mgr->cfg.channel_count; ch++){
		//Get raw measurement
		uint16_t raw_value = samples[ch];
		//Convert to voltage
		float voltage = ((float) raw_value * mgr->cfg.vref) / ((float)mgr->cfg.adc_resolution);
		//Process sample with moving average filter
		if(moving_average_process(&mgr->filters[ch], voltage) != MOVING_AVERAGE_OK){
			return ANALOG_MANAGER_STATUS_ERR_FILTER_UPDATE;
		}
		//Set filtered value on the filtered voltage buffer
		mgr->filtered_voltage[ch] = moving_average_get_value(&mgr->filters[ch]);
	}

	return ANALOG_MANAGER_STATUS_OK;
}

analog_manager_status_t analog_manager_reset_filter(analog_manager_t *mgr, analog_signal_id_t signal){
	uint8_t channel = 0U;
	analog_manager_status_t status = analog_manager_find_signal_index(mgr, signal, &channel);
	if(status != ANALOG_MANAGER_STATUS_OK) return status;

	//Reset moving average filter
	if(moving_average_reset_filter(&mgr->filters[channel]) != MOVING_AVERAGE_OK){
		return ANALOG_MANAGER_STATUS_ERR_FILTER_RESET;
	}

	//Reset filtered value at channel position
	mgr->filtered_voltage[channel] = 0.0f;

	return ANALOG_MANAGER_STATUS_OK;
}

float analog_manager_get_filtered_voltage(analog_manager_t *mgr, analog_signal_id_t signal){
	uint8_t channel = 0U;
	if(analog_manager_find_signal_index(mgr, signal, &channel) != ANALOG_MANAGER_STATUS_OK){
		return -1.0f;
	}

	return mgr->filtered_voltage[channel];
}

bool analog_manager_is_filter_ready(analog_manager_t *mgr, analog_signal_id_t signal){
	uint8_t channel = 0U;
	if(analog_manager_find_signal_index(mgr, signal, &channel) != ANALOG_MANAGER_STATUS_OK){
		return false;
	}

	return mgr->filters[channel].is_ready;
}

