#ifndef TEMP_HUM_SENSOR_H
#define TEMP_HUM_SENSOR_H
/*Includes*/
#include "sht30/sht30.h"

/*Public enums*/
typedef enum{
	TEMP_HUM_OK = 0,
	TEMP_HUM_ERR_NULL,
	TEMP_HUM_ERR_TIMEOUT,
	TEMP_HUM_ERR_BUS
}temp_hum_err_t;

/*Public structures*/
typedef struct{
	sht30_t *dev;
	sht30_repeatability_t repeatability;
}temp_hum_sensor_t;

/*API functions*/
temp_hum_err_t temp_hum_sensor_init(temp_hum_sensor_t *sensor, sht30_t *dev, sht30_repeatability_t rep);
temp_hum_err_t temp_hum_sensor_read(temp_hum_sensor_t *sensor, float *temp, float *hr);
#endif/*TEMP_HUM_SENSOR_H*/
