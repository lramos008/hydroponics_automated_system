#ifndef WATER_TEMP_SENSOR_H
#define WATER_TEMP_SENSOR_H
/*Includes*/
#include "ds18b20/ds18b20.h"

/*Public enums*/
typedef enum{
	WATER_TEMP_OK,
	WATER_TEMP_NOT_READY,
	WATER_TEMP_ERR_NULL,
	WATER_TEMP_ERR_INIT,
	WATER_TEMP_ERR_INVALID_DATA,
	WATER_TEMP_ERR_SENSOR_DISCONNECTED,
	WATER_TEMP_ERR_COMM
}water_temp_err_t;

/*Public structures*/
typedef struct{
	ds18b20_t *dev;
}water_temp_sensor_t;

/*API functions*/
water_temp_err_t water_temp_sensor_init(water_temp_sensor_t *sensor, ds18b20_t *dev);
water_temp_err_t water_temp_sensor_request(water_temp_sensor_t *sensor);
water_temp_err_t water_temp_is_sensor_ready(water_temp_sensor_t *sensor);
water_temp_err_t water_temp_sensor_read(water_temp_sensor_t *sensor, float *temp);
#endif/*WATER_TEMP_SENSOR_H*/
