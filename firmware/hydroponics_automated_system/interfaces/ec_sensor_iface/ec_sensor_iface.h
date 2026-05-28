#pragma once

/*Includes*/
#include "analog_manager/analog_manager.h"

/*Defines*/
#define EC_SENSOR_IFACE_FILTER_SIZE 	32					//Must be a power of 2
#define EC_SENSOR_STANDARD_PATTERN_LOW  1413.0f
#define EC_SENSOR_STANDARD_PATTERN_HIGH 12880.0f

/*Status*/
typedef enum{
	//Initialization
	EC_SENSOR_IFACE_OK,
	EC_SENSOR_IFACE_ERR_NULL,
	//Filter status
	EC_SENSOR_IFACE_FILTER_NOT_READY,
	EC_SENSOR_IFACE_ERR_FILTER_NOT_INITIALIZED,
	//Calibration
	EC_SENSOR_IFACE_ERR_CALIBRATION,
	//Measurement
	EC_SENSOR_IFACE_ERR_ADC
}ec_sensor_iface_status_t;

/*Calibration*/
typedef struct{
	float slope;
	float offset;
}ec_sensor_calibration_t;

/*Interface*/
typedef struct{
	//ADC
	analog_manager_t *hman;
	uint8_t channel;
	//Calibration
	ec_sensor_calibration_t calibration;
	//Filter
	moving_average_handle_t filter;
	float filter_buffer[EC_SENSOR_IFACE_FILTER_SIZE];
	//Compensated EC value
	float ec_value;
	//Filter is ready for its use
	bool is_ready;
}ec_sensor_iface_t;

/*API*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface, analog_manager_t *hman);
ec_sensor_iface_status_t ec_sensor_iface_update(ec_sensor_iface_t *iface, float compensation_temperature);
float 					 ec_sensor_iface_get_ec_value(ec_sensor_iface_t *iface);
ec_sensor_iface_status_t ec_sensor_iface_reset(ec_sensor_iface_t *iface);
ec_sensor_iface_status_t ec_sensor_iface_two_point_calibration(ec_sensor_iface_t *iface, float ec_base_low, float ec_base_high);
ec_sensor_iface_status_t ec_sensor_iface_save_calibration_constants(ec_sensor_iface_t *iface);
ec_sensor_iface_status_t ec_sensor_iface_load_calibration_constants(ec_sensor_iface_t *iface);
bool 					 ec_sensor_iface_is_ready(ec_sensor_iface_t *iface);
