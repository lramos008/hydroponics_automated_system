#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H
/*Includes*/
#include "bh1750/bh1750.h"

/*Public enums*/
typedef enum{
	LIGHT_OK,
	LIGHT_ERR_NULL,
	LIGHT_ERR_TIMEOUT,
	LIGHT_ERR_BUS
}light_err_t;

/*Public structures*/
typedef struct{
	bh1750_t *dev;
	bh1750_res_mode_t res_mode;
}light_sensor_t;

/*API functions*/
light_err_t light_sensor_init(light_sensor_t *light_sensor, bh1750_t *dev, bh1750_res_mode_t res_mode);
light_err_t light_sensor_read(light_sensor_t *light_sensor, float *lux);
#endif/*LIGHT_SENSOR_H*/
