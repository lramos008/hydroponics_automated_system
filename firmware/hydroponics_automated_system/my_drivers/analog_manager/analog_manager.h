#pragma once

/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "filters/moving_average.h"

/*Enums*/
typedef enum{
	//Initialization
	ANALOG_MANAGER_OK,
	ANALOG_MANAGER_ERR_NULL,
	//Hardware
	ANALOG_MANAGER_ERR_INVALID_CHANNEL,
	ANALOG_MANAGER_ERR_ADC_START,
	ANALOG_MANAGER_ERR_ADC_STOP,
	ANALOG_MANAGER_ERR_DMA,
	//Filter
	ANALOG_MANAGER_ERR_FILTER
}analog_manager_status_t;

/*Structs*/
typedef struct{
	uint8_t dma_index;
	uint16_t adc_resolution;
	float vref;
	moving_average_handle_t *filter;
}analog_channel_t;

typedef struct{
	ADC_HandleTypeDef *hadc;
	analog_channel_t *channels;
	uint8_t channels_count;
	uint16_t *dma_buffer;
	float *filtered_voltage;
}analog_manager_t;


/*API*/
analog_manager_status_t analog_manager_init(analog_manager_t *hman);
analog_manager_status_t analog_manager_start(analog_manager_t *hman);
analog_manager_status_t analog_manager_stop(analog_manager_t *hman);
analog_manager_status_t analog_manager_update(analog_manager_t *hman);
analog_manager_status_t analog_manager_reset_filter(analog_manager_t *hman, uint8_t channel);
float analog_manager_get_filtered_voltage(analog_manager_t *hman, uint8_t channel);
bool analog_manager_is_filter_ready(analog_manager_t *hman, uint8_t channel);

