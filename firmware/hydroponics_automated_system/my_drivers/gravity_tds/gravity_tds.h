#pragma once
/*Public includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Defines*/
#define GRAVITY_TDS_INTERNAL_BUFFER_SIZE 32

/*Enums*/
typedef enum{
	GRAVITY_OK,
	GRAVITY_ERR_NULL,
	GRAVITY_ERR_ADC_CONFIG,
	GRAVITY_ERR_ADC_READING,
	GRAVITY_ERR_CALIBRATION,
	GRAVITY_ERR_OUT_OF_RANGE
}gravity_tds_err_t;


/*Structs*/
typedef struct{
	ADC_HandleTypeDef *hadc;
	uint32_t channel;						//e.g ADC_CHANNEL_0
	uint32_t adc_range;
	float vref;								//e.g. 3V3
	float k_value;							//Calibration factor due to sensor imperfections
	float tds_factor;						//0.5 for America and 0.7 for Europe. Used to translate from physic unit (EC) to mass (ppm).
	float solution_temp;					//Obtained from a water temp sensor

	/*Last measurements*/
	float last_voltage;
	float last_ec;

	/*Internal buffer for filtering*/
	uint32_t adc_buffer[GRAVITY_TDS_INTERNAL_BUFFER_SIZE];
	int buffer_idx;
	bool is_buffer_full;
}gravity_tds_handle_t;


/*Public API*/
gravity_tds_err_t gravity_tds_init(gravity_tds_handle_t *dev, ADC_HandleTypeDef *hadc, uint32_t channel,
								   uint32_t adc_range, float vref, float k_value, float tds_factor);

gravity_tds_err_t gravity_tds_set_temperature(gravity_tds_handle_t *dev, float temp);
gravity_tds_err_t gravity_tds_get_voltage(gravity_tds_handle_t *dev, float *voltage);
gravity_tds_err_t gravity_tds_get_value_ec(gravity_tds_handle_t *dev, float *ec);
gravity_tds_err_t gravity_tds_get_value_ppm(gravity_tds_handle_t *dev, float *ppm);
gravity_tds_err_t gravity_tds_calibrate(gravity_tds_handle_t *dev, float target_ec);
bool 			  gravity_tds_is_reading_stable(gravity_tds_handle_t *dev);



