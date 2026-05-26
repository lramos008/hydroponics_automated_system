#pragma once

/*Includes*/
#include "ec_sensor_driver/ec_sensor_driver.h"
#include "filters/moving_average.h"

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

/*Handle*/
typedef struct{
	//Sensor internal status
	ec_sensor_driver_t *dev;
	ec_sensor_calibration_t calibration;
	//Filter internal status
	moving_average_handle_t hfilter;
	float filter_buffer[EC_SENSOR_IFACE_FILTER_SIZE];
	//Filtered measurement
	float ec_value;
}ec_sensor_iface_t;

ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface, ec_sensor_driver_t *dev);
ec_sensor_iface_status_t ec_sensor_iface_update(ec_sensor_iface_t *iface, float compensation_temperature);
ec_sensor_iface_status_t ec_sensor_iface_get_ec_value(ec_sensor_iface_t *iface, float *ec_value);
ec_sensor_iface_status_t ec_sensor_iface_two_point_calibration(ec_sensor_iface_t *iface, float ec_base_1, float ec_base_2);
ec_sensor_iface_status_t ec_sensor_iface_save_calibration_constants(ec_sensor_iface_t *iface);
ec_sensor_iface_status_t ec_sensor_iface_load_calibration_constants(ec_sensor_iface_t *iface);
