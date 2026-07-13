#pragma once

/*Includes*/
#include <stdbool.h>
#include "analog_manager/analog_manager.h"

/*Defines*/
#define PH_SENSOR_STANDARD_PATTERN_LOW  4.0f
#define PH_SENSOR_STANDARD_PATTERN_HIGH 7.0f

/*Status*/
typedef enum{
	PH_SENSOR_IFACE_STATUS_OK,
	PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER,
	PH_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED,
	PH_SENSOR_IFACE_STATUS_ERR_INVALID_SIGNAL,
	PH_SENSOR_IFACE_STATUS_ERR_RESET,
	PH_SENSOR_IFACE_STATUS_ERR_CALIBRATION,
	PH_SENSOR_IFACE_STATUS_ERR_CORRUPTED_TEMPERATURE,
	PH_SENSOR_IFACE_STATUS_NOT_READY
}ph_sensor_iface_status_t;

/*Calibration*/
typedef struct{
	float slope_25;
	float offset;
}ph_sensor_iface_calibration_t;

/*Interface*/
typedef struct{
	float compensated_voltage;
	float ph_value;
}ph_sensor_iface_data_t;

typedef struct{
	analog_manager_t *mgr;
	analog_signal_id_t signal;
	ph_sensor_iface_calibration_t calibration;
	ph_sensor_iface_data_t data;
	bool is_initialized;
}ph_sensor_iface_t;

/*API*/
ph_sensor_iface_status_t ph_sensor_iface_init(ph_sensor_iface_t *iface, analog_manager_t *mgr, analog_signal_id_t signal);
ph_sensor_iface_status_t ph_sensor_iface_process(ph_sensor_iface_t *iface, float temperature);
bool					 ph_sensor_iface_is_ready(ph_sensor_iface_t *iface);
ph_sensor_iface_status_t ph_sensor_iface_get_data(ph_sensor_iface_t *iface, ph_sensor_iface_data_t *data);
ph_sensor_iface_status_t ph_sensor_iface_set_calibration(ph_sensor_iface_t *iface, ph_sensor_iface_calibration_t calibration);
ph_sensor_iface_status_t ph_sensor_iface_reset(ph_sensor_iface_t *iface);
ph_sensor_iface_status_t ph_sensor_iface_two_point_calibration(ph_sensor_iface_t *iface, float v_high, float v_low);
