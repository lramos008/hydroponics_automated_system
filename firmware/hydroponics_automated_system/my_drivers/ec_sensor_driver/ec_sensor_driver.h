#pragma once
/*Public includes*/
#include <stdint.h>
#include "stm32f4xx_hal.h"

/*Enums*/
typedef enum{
	//Initialization
	EC_SENSOR_DRIVER_OK,
	EC_SENSOR_DRIVER_ERR_NULL,
	//Hardware
	EC_SENSOR_DRIVER_ERR_ADC_CONFIG,
	EC_SENSOR_DRIVER_ERR_ADC_READING
}ec_sensor_driver_status_t;


/*Structs*/
typedef struct{
	//EC sensor ADC params
	ADC_HandleTypeDef *hadc;
	uint32_t adc_channel;					//e.g ADC_CHANNEL_0
	uint32_t adc_range;
	float vref;								//e.g. 3V3
}ec_sensor_driver_t;


/*Public API*/
ec_sensor_driver_status_t ec_sensor_driver_init(ec_sensor_driver_t *dev, ADC_HandleTypeDef *hadc, uint32_t adc_channel, uint32_t adc_range, float vref);
ec_sensor_driver_status_t ec_sensor_driver_read_voltage(ec_sensor_driver_t *dev, float *voltage);



