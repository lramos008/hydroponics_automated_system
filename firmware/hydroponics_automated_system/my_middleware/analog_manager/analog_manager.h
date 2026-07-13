#pragma once
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "filters/moving_average.h"

/*Defines*/
#define ANALOG_MOVING_AVG_FILTER_SIZE 64U		//Must be a power of 2
#define ANALOG_MANAGER_MAX_CHANNELS 8U

/*Enums*/
typedef enum{
	ANALOG_SIGNAL_EC,
	ANALOG_SIGNAL_PH,
	ANALOG_SIGNAL_CT_CIRCULATION,
	ANALOG_SIGNAL_COUNT
}analog_signal_id_t;

typedef enum{
	ANALOG_MANAGER_STATUS_OK,
	//Initialization
	ANALOG_MANAGER_STATUS_ERR_NULL_POINTER,
	ANALOG_MANAGER_STATUS_ERR_NOT_INITIALIZED,
	ANALOG_MANAGER_STATUS_ERR_INVALID_CONFIG,
	//Status
	ANALOG_MANAGER_STATUS_ERR_NOT_STARTED,
	//Signal
	ANALOG_MANAGER_STATUS_ERR_INVALID_SIGNAL,
	ANALOG_MANAGER_STATUS_ERR_SIGNAL_NOT_FOUND,
	//ADC
	ANALOG_MANAGER_STATUS_ERR_ADC_START,
	ANALOG_MANAGER_STATUS_ERR_ADC_STOP,
	//Filter
	ANALOG_MANAGER_STATUS_ERR_FILTER_INIT,
	ANALOG_MANAGER_STATUS_ERR_FILTER_UPDATE,
	ANALOG_MANAGER_STATUS_ERR_FILTER_RESET
}analog_manager_status_t;

/*Structs*/
typedef struct{
	ADC_HandleTypeDef *hadc;
	float vref;
	uint16_t adc_resolution;

	uint8_t channel_count;
	analog_signal_id_t signal_map[ANALOG_MANAGER_MAX_CHANNELS];
}analog_manager_config_t;

typedef struct{
	analog_manager_config_t cfg;
	//DMA channel samples
	uint16_t dma_buffer[ANALOG_MANAGER_MAX_CHANNELS];
	//Filtered voltages (output ready for being used by interfaces)
	float filtered_voltage[ANALOG_MANAGER_MAX_CHANNELS];
	//Moving average filters
	moving_average_handle_t filters[ANALOG_MANAGER_MAX_CHANNELS];
	float filter_buffers[ANALOG_MANAGER_MAX_CHANNELS][ANALOG_MOVING_AVG_FILTER_SIZE];
	//Internal status
	bool is_initialized;
	bool is_running;
}analog_manager_t;

/*API*/
analog_manager_status_t analog_manager_init(analog_manager_t *mgr, const analog_manager_config_t *cfg);


analog_manager_status_t analog_manager_start(analog_manager_t *mgr);

analog_manager_status_t analog_manager_stop(analog_manager_t *mgr);

analog_manager_status_t analog_manager_process(analog_manager_t *mgr);

analog_manager_status_t analog_manager_reset_filter(analog_manager_t *mgr, analog_signal_id_t signal);

float					analog_manager_get_filtered_voltage(analog_manager_t *mgr, analog_signal_id_t signal);

bool					analog_manager_is_filter_ready(analog_manager_t *mgr, analog_signal_id_t signal);
