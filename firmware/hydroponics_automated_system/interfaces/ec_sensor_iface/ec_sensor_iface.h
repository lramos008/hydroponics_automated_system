#pragma once

/*Includes*/
#include "analog_manager/analog_manager.h"

/*Defines*/
#define EC_SENSOR_STANDARD_PATTERN_LOW  1.413f
#define EC_SENSOR_STANDARD_PATTERN_HIGH 2.770f

/*Status*/
typedef enum{
	EC_SENSOR_IFACE_STATUS_OK,
	EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER,
	EC_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED,
	EC_SENSOR_IFACE_STATUS_ERR_INVALID_SIGNAL,
	EC_SENSOR_IFACE_STATUS_ERR_RESET,
	EC_SENSOR_IFACE_STATUS_ERR_CALIBRATION,
	EC_SENSOR_IFACE_STATUS_ERR_CORRUPTED_TEMPERATURE,
	EC_SENSOR_IFACE_STATUS_NOT_READY
}ec_sensor_iface_status_t;

/*Calibration*/
typedef struct{
	float slope;
	float offset;
}ec_sensor_iface_calibration_t;

/*Interface*/
typedef struct{
	float compensated_voltage;
	float ec_value;
}ec_sensor_iface_data_t;

typedef struct{
	analog_manager_t *mgr;
	analog_signal_id_t signal;
	ec_sensor_iface_calibration_t calibration;
	ec_sensor_iface_data_t data;
	bool is_initialized;
}ec_sensor_iface_t;

/*API*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface, analog_manager_t *mgr, analog_signal_id_t signal);
ec_sensor_iface_status_t ec_sensor_iface_process(ec_sensor_iface_t *iface, float temperature);
bool					 ec_sensor_iface_is_ready(ec_sensor_iface_t *iface);
ec_sensor_iface_status_t ec_sensor_iface_get_data(ec_sensor_iface_t *iface, ec_sensor_iface_data_t *data);
ec_sensor_iface_status_t ec_sensor_iface_set_calibration(ec_sensor_iface_t *iface, ec_sensor_iface_calibration_t calibration);
ec_sensor_iface_status_t ec_sensor_iface_reset(ec_sensor_iface_t *iface);

