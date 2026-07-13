#pragma once

/*Includes*/
#include <stdbool.h>
#include "sht30/sht30.h"

/*Public enums*/
typedef enum{
	TEMP_HUM_SENSOR_IFACE_STATUS_OK,
	TEMP_HUM_SENSOR_IFACE_STATUS_BUSY,
	TEMP_HUM_SENSOR_IFACE_STATUS_NOT_READY,
	TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NULL_POINTER,
	TEMP_HUM_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED,
	TEMP_HUM_SENSOR_IFACE_STATUS_ERR_DRIVER
}temp_hum_sensor_iface_status_t;

/*Public structures*/
typedef struct{
	float temperature;
	float humidity;
}temp_hum_sensor_iface_data_t;

typedef struct{
	sht30_t *dev;
	temp_hum_sensor_iface_data_t data;
	bool is_initialized;
}temp_hum_sensor_iface_t;

/*API functions*/
temp_hum_sensor_iface_status_t temp_hum_sensor_iface_init(temp_hum_sensor_iface_t *iface, sht30_t *dev);
temp_hum_sensor_iface_status_t temp_hum_sensor_iface_start_measurement(temp_hum_sensor_iface_t *iface);
temp_hum_sensor_iface_status_t temp_hum_sensor_iface_process(temp_hum_sensor_iface_t *iface);
bool							temp_hum_sensor_iface_is_ready(temp_hum_sensor_iface_t *iface);
temp_hum_sensor_iface_status_t temp_hum_sensor_iface_get_data(temp_hum_sensor_iface_t *iface, temp_hum_sensor_iface_data_t *data);
temp_hum_sensor_iface_status_t temp_hum_sensor_iface_reset(temp_hum_sensor_iface_t *iface);
