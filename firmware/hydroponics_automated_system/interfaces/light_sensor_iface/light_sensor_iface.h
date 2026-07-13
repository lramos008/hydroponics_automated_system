#pragma once

/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "bh1750/bh1750.h"

/*Public enums*/
typedef enum{
	LIGHT_SENSOR_IFACE_STATUS_OK,
	LIGHT_SENSOR_IFACE_STATUS_BUSY,
	LIGHT_SENSOR_IFACE_STATUS_NOT_READY,
	LIGHT_SENSOR_IFACE_STATUS_ERR_NULL_POINTER,
	LIGHT_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED,
	LIGHT_SENSOR_IFACE_STATUS_ERR_DRIVER
}light_sensor_iface_status_t;

/*Public structures*/
typedef struct{
	float lux;
}light_sensor_iface_data_t;

typedef struct{
	bh1750_t *dev;
	light_sensor_iface_data_t data;
	bool is_initialized;
}light_sensor_iface_t;

/*API functions*/
light_sensor_iface_status_t light_sensor_iface_init(light_sensor_iface_t *iface, bh1750_t *dev);
light_sensor_iface_status_t light_sensor_iface_start_measurement(light_sensor_iface_t *iface);
light_sensor_iface_status_t light_sensor_iface_process(light_sensor_iface_t *iface);
bool						light_sensor_iface_is_ready(light_sensor_iface_t *iface);
light_sensor_iface_status_t light_sensor_iface_get_data(light_sensor_iface_t *iface, light_sensor_iface_data_t *data);
light_sensor_iface_status_t light_sensor_iface_reset(light_sensor_iface_t *iface);
