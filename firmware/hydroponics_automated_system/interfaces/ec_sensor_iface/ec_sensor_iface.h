#pragma once

/*Includes*/
#include "analog_manager/analog_manager.h"

/*Defines*/
#define EC_SENSOR_STANDARD_PATTERN_LOW  1.413f
#define EC_SENSOR_STANDARD_PATTERN_HIGH 2.770f

/*Status*/
typedef enum{
	EC_SENSOR_IFACE_OK,
	//Initialization
	EC_SENSOR_IFACE_ERR_NULL,
	EC_SENSOR_IFACE_ERR_NOT_INITIALIZED,
	EC_SENSOR_IFACE_ERR_INVALID_CHANNEL,
	EC_SENSOR_IFACE_ERR_RESET,
	//Calibration
	EC_SENSOR_IFACE_ERR_CALIBRATION,
	//ADC
	EC_SENSOR_IFACE_ERR_ANALOG_MANAGER
}ec_sensor_iface_status_t;

/*Calibration*/
typedef struct{
	float slope;
	float offset;
}ec_sensor_calibration_t;

/*Interface*/
typedef struct{
	//ADC
	analog_manager_t *analog;
	analog_channel_id_t channel;
	//Calibration
	ec_sensor_calibration_t calibration;
	//Internal status
	float ec_value;
	bool is_initialized;
}ec_sensor_iface_t;

/*API*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface);
float 					 ec_sensor_iface_get_ec_value(ec_sensor_iface_t *iface, float temperature);
ec_sensor_iface_status_t ec_sensor_iface_reset(ec_sensor_iface_t *iface);
ec_sensor_iface_status_t ec_sensor_iface_set_calibration(ec_sensor_iface_t *iface, ec_sensor_calibration_t cal);
ec_sensor_iface_status_t ec_sensor_iface_set_default_calibration(ec_sensor_iface_t *iface);
bool 					 ec_sensor_iface_is_ready(ec_sensor_iface_t *iface);
