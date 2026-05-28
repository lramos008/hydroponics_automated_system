#include "ec_sensor_iface.h"

/*Public functions*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface, analog_manager_t *hman){
	//Sanity check
	if(iface == NULL)		return EC_SENSOR_IFACE_ERR_NULL;
	if(hman  == NULL)		return EC_SENSOR_IFACE_ERR_NULL;

	//Init iface
	iface->hman = hman;
	iface->channel = 1;	//Cambiar
	iface->calibration.slope  = 1.0f;
	iface->calibration.offset = 0.0f;
	iface->ec_value = 0.0f;
	iface->is_ready = false;
	moving_average_status_t filter_status = moving_average_init(&iface->filter, iface->filter_buffer, EC_SENSOR_IFACE_FILTER_SIZE);
	if(filter_status != MOVING_AVERAGE_OK) return EC_SENSOR_IFACE_ERR_FILTER_NOT_INITIALIZED;

	return EC_SENSOR_IFACE_OK;
}


ec_sensor_iface_status_t ec_sensor_iface_update(ec_sensor_iface_t *iface, float compensation_temperature){
	//Sanity check
	if(iface == NULL)		return EC_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return EC_SENSOR_IFACE_ERR_NULL;

	//Read voltage value from sensor
	float filtered_voltage = analog_manager_get_filtered_voltage(iface->hman, iface->channel);

	//Compensate voltage by temperature effects
	float compensation_coeff = 1.0 + 0.02 * (compensation_temperature - 25.0);
	float v_25 = filtered_voltage / compensation_coeff;									//Compensated voltage for 25 °C

	/*Calculate base EC. This polynom was designed to output a ppm value.
	 * In order to get EC you need to do a conversion. DFRobot used the
	 * NaCl scale where an EC of 1.0 mS/cm is equivalent to 500 ppm.
	 * Thus that factor is used to normalize the EC value.
	 */
	float base_ec = (133.42 * v_25 * v_25 * v_25 - 255.86 * v_25 * v_25 + 857.39 * v_25) / 500.0f;

	//Apply calibration factors (due to sensor + system imperfections)
	iface->ec_value = base_ec * iface->calibration.slope + iface->calibration.offset;

	return EC_SENSOR_IFACE_OK;
}

float ec_sensor_iface_get_ec_value(ec_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)	return EC_SENSOR_IFACE_ERR_NULL;

	return iface->ec_value;
}

ec_sensor_iface_status_t ec_sensor_iface_reset(ec_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)		return EC_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return EC_SENSOR_IFACE_ERR_NULL;

	//Reset moving average filter
	analog_manager_reset_filter(iface->hman, iface->channel);

	return EC_SENSOR_IFACE_OK;
}


ec_sensor_iface_status_t ec_sensor_iface_two_point_calibration(ec_sensor_iface_t *iface, float ec_base_low, float ec_base_high){
	//Sanity check
	if(iface == NULL)		return EC_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return EC_SENSOR_IFACE_ERR_NULL;

	//Set standard pattern values
	float ec_real_low  = EC_SENSOR_STANDARD_PATTERN_LOW;
	float ec_real_high = EC_SENSOR_STANDARD_PATTERN_HIGH;

	//Check that denominator is not equal to 0.0
	float denom = ec_base_high - ec_base_low;
	if(denom == 0.0f) return EC_SENSOR_IFACE_ERR_CALIBRATION;

	//Calculate slope
	iface->calibration.slope = (ec_real_high - ec_real_low) / denom;

	//Calculate offset
	iface->calibration.offset = ec_real_low - ec_base_low * iface->calibration.slope;

	return EC_SENSOR_IFACE_OK;
}


bool ec_sensor_iface_is_ready(ec_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)		return EC_SENSOR_IFACE_ERR_NULL;
	if(iface->hman == NULL)	return EC_SENSOR_IFACE_ERR_NULL;

	return analog_manager_is_filter_ready(iface->hman, iface->channel);
}
