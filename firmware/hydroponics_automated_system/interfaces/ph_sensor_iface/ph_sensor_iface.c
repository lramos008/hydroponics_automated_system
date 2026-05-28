/*Includes*/
#include "ph_sensor_iface.h"

/*Public functions*/
ph_sensor_iface_status_t ph_sensor_iface_init(ph_sensor_iface_t *iface, analog_manager_t *hman){
	//Sanity check
	if(iface == NULL)		return PH_SENSOR_IFACE_ERR_NULL;
	if(hman  == NULL)		return PH_SENSOR_IFACE_ERR_NULL;

	//Init iface
	iface->hman = hman;
	iface->channel = 2;	//Cambiar
	iface->calibration.slope_25 = 1.0f;
	iface->calibration.offset   = 0.0f;
	iface->ph_value 			= 0.0f;
	iface->is_ready				= false;
	moving_average_status_t filter_status = moving_average_init(&iface->filter, iface->filter_buffer, PH_SENSOR_IFACE_FILTER_SIZE);
	if(filter_status != MOVING_AVERAGE_OK) return PH_SENSOR_IFACE_ERR_FILTER_NOT_INITIALIZED;

	return PH_SENSOR_IFACE_OK;
}


ph_sensor_iface_status_t ph_sensor_iface_update(ph_sensor_iface_t *iface, float compensation_temperature){
	//Sanity check
	if(iface == NULL)		return PH_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return PH_SENSOR_IFACE_ERR_NULL;

	//Read voltage value from sensor
	float filtered_voltage = analog_manager_get_filtered_voltage(iface->hman, iface->channel);

	//Compensate slope
	float compensated_slope = iface->calibration.slope_25 * (1.0 + 0.003 * (compensation_temperature - 25.0));

	//Calculate pH by using calibration factors (due to sensor + system imperfections)
	iface->ph_value = filtered_voltage * compensated_slope + iface->calibration.offset;

	return PH_SENSOR_IFACE_OK;
}

float ph_sensor_iface_get_ph_value(ph_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)	return PH_SENSOR_IFACE_ERR_NULL;

	return iface->ph_value;
}

ph_sensor_iface_status_t ph_sensor_iface_reset(ph_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)		return PH_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return PH_SENSOR_IFACE_ERR_NULL;

	//Reset moving average filter
	analog_manager_reset_filter(iface->hman, iface->channel);

	return PH_SENSOR_IFACE_OK;
}

ph_sensor_iface_status_t ph_sensor_iface_two_point_calibration(ph_sensor_iface_t *iface, float v_high, float v_low){
	//Sanity check
	if(iface == NULL)		return PH_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return PH_SENSOR_IFACE_ERR_NULL;

	//Set standard pattern values
	float ph_real_low  = PH_SENSOR_STANDARD_PATTERN_LOW;
	float ph_real_high = PH_SENSOR_STANDARD_PATTERN_HIGH;

	//Check that denominator is not equal to 0.0
	float denom = v_high - v_low;
	if(denom == 0.0f) return PH_SENSOR_IFACE_ERR_CALIBRATION;

	//Calculate slope
	iface->calibration.slope_25 = (ph_real_high - ph_real_low) / denom;

	//Calculate offset
	iface->calibration.offset   = ph_real_low - v_low * iface->calibration.slope_25;

	return PH_SENSOR_IFACE_OK;
}

bool ph_sensor_iface_is_ready(ph_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)		return PH_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return PH_SENSOR_IFACE_ERR_NULL;

	return analog_manager_is_filter_ready(iface->hman, iface->channel);
}
