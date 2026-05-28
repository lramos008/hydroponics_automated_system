#pragma once

/*Includes*/
#include "analog_manager/analog_manager.h"

/*Defines*/
/*Defines*/
#define PH_SENSOR_IFACE_FILTER_SIZE 	32					//Must be a power of 2
#define PH_SENSOR_STANDARD_PATTERN_LOW  4.0f
#define PH_SENSOR_STANDARD_PATTERN_HIGH 7.0f

/*Status*/
typedef enum{
	//Initialization
	PH_SENSOR_IFACE_OK,
	PH_SENSOR_IFACE_ERR_NULL,
	//Filter status
	PH_SENSOR_IFACE_FILTER_NOT_READY,
	PH_SENSOR_IFACE_ERR_FILTER_NOT_INITIALIZED,
	//Calibration
	PH_SENSOR_IFACE_ERR_CALIBRATION,
	//Measurement
	PH_SENSOR_IFACE_ERR_ADC
}ph_sensor_iface_status_t;

/*Calibration*/
typedef struct{
	float slope_25;
	float offset;
}ph_sensor_calibration_t;

/*Interface*/
typedef struct{
	//ADC
	analog_manager_t *hman;
	uint8_t channel;
	//Calibration
	ph_sensor_calibration_t calibration;
	//Filter
	moving_average_handle_t filter;
	float filter_buffer[PH_SENSOR_IFACE_FILTER_SIZE];
	//Compensated EC value
	float ph_value;
	//Filter is ready for its use
	bool is_ready;
}ph_sensor_iface_t;

/*S(T)=S25​(1+0.003(T−25))*/
/*API*/
ph_sensor_iface_status_t ph_sensor_iface_init(ph_sensor_iface_t *iface, analog_manager_t *hman);
ph_sensor_iface_status_t ph_sensor_iface_update(ph_sensor_iface_t *iface, float compensation_temperature);
float					 ph_sensor_iface_get_ph_value(ph_sensor_iface_t *iface);
ph_sensor_iface_status_t ph_sensor_iface_reset(ph_sensor_iface_t *iface);
ph_sensor_iface_status_t ph_sensor_iface_two_point_calibration(ph_sensor_iface_t *iface, float v_high, float v_low);
ph_sensor_iface_status_t ph_sensor_iface_save_calibration_constants(ph_sensor_iface_t *iface);
ph_sensor_iface_status_t ph_sensor_iface_load_calibration_constants(ph_sensor_iface_t *iface);
bool 					 ph_sensor_iface_is_ready(ph_sensor_iface_t *iface);
