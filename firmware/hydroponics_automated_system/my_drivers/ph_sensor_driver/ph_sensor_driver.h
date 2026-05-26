#pragma once

/*Includes*/
#include <stdint.h>
#include "stm32f4xx_hal.h"

/*Enums*/
typedef enum{
	//Initialization
	PH_SENSOR_DRIVER_OK,
	PH_SENSOR_DRIVER_ERR_NULL,
	//Hardware
	PH_SENSOR_DRIVER_ERR_ADC_CONFIG,
	PH_SENSOR_DRIVER_ERR_ADC_READING
}ph_sensor_driver_status_t;

/*Structs*/
