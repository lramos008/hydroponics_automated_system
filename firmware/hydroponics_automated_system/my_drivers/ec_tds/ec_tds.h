#ifndef EC_TDS_GRAVITY_H
#define EC_TDS_GRAVITY_H
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Public enums*/
typedef enum{
	EC_TDS_OK,
	EC_TDS_ERR_NULL,
	EC_TDS_ERR_OUT_OF_RANGE
}ec_tds_err_t;

/*Public structures*/
typedef struct{
	//ADC
	ADC_HandleTypeDef *hadc;
	uint32_t channel;
	//EC / TDS sensor
	float vref;
	uint16_t adc_max_count;
	float k;							//Calibration factor
	float tds_factor;
}ec_tds_handle_t;

typedef struct{
	float voltage;
	float ec;
	float tds;
}ec_tds_measurement_t;

/*Public API*/
ec_tds_err_t ec_tds_init(ec_tds_handle_t *dev, ADC_HandleTypeDef *hadc, uint32_t channel, float vref, float k, float tds_factor, uint16_t adc_max_count);
ec_tds_err_t ec_tds_read(ec_tds_handle_t *dev, ec_tds_measurement_t *measurement);
#endif/*EC_TDS_GRAVITY_H*/
