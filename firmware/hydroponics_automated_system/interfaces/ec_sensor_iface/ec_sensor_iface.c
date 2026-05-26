#include "ec_sensor_iface.h"

/*Public functions*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface, ec_sensor_driver_t *dev){
	//Sanity check
	if(!iface || !dev || !dev->hadc) return EC_SENSOR_IFACE_ERR_NULL;

	//Init iface
	iface->dev = dev;
	iface->calibration.slope  = 1.0f;
	iface->calibration.offset = 0.0f;
	iface->ec_value = 0.0f;
	moving_average_status_t filter_status = moving_average_init(&iface->hfilter, iface->filter_buffer, EC_SENSOR_IFACE_FILTER_SIZE);
	if(filter_status != MOVING_AVERAGE_OK) return EC_SENSOR_IFACE_ERR_FILTER_NOT_INITIALIZED;

	return EC_SENSOR_IFACE_OK;
}


ec_sensor_iface_status_t ec_sensor_iface_update(ec_sensor_iface_t *iface, float compensation_temperature){
	//Sanity check
	if(!iface || !iface->dev) return EC_SENSOR_IFACE_ERR_NULL;

	//Read voltage value from sensor
	float voltage;
	ec_sensor_driver_status_t dev_status = ec_sensor_driver_read_voltage(iface->dev, &voltage);
	if(dev_status != EC_SENSOR_DRIVER_OK) return EC_SENSOR_IFACE_ERR_ADC;

	//Update moving average filter
	moving_average_status_t filter_status = moving_average_process(&iface->hfilter, voltage);
	if(filter_status != MOVING_AVERAGE_OK) return EC_SENSOR_IFACE_FILTER_NOT_READY;

	//Wait until filter is ready
	if(!iface->hfilter.is_ready) return EC_SENSOR_DRIVER_OK;

	//Get filtered voltage
	float filtered_voltage;
	filter_status = moving_average_get_value(&iface->hfilter, &filtered_voltage);
	if(filter_status != MOVING_AVERAGE_OK) return EC_SENSOR_IFACE_FILTER_NOT_READY;

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

	return EC_SENSOR_DRIVER_OK;
}


ec_sensor_iface_status_t ec_sensor_iface_get_ec_value(ec_sensor_iface_t *iface, float *ec_value){
	//Sanity check
	if(!iface || !iface->dev || !ec_value) return EC_SENSOR_IFACE_ERR_NULL;

	*ec_value = iface->ec_value;
	return EC_SENSOR_IFACE_OK;
}

ec_sensor_iface_status_t ec_sensor_iface_two_point_calibration(ec_sensor_iface_t *iface, float ec_base_1, float ec_base_2){
	//Sanity check
	if(!iface || !iface->dev) return EC_SENSOR_IFACE_ERR_NULL;

	//Set standard pattern values
	float ec_real_1 = EC_SENSOR_STANDARD_PATTERN_LOW;
	float ec_real_2 = EC_SENSOR_STANDARD_PATTERN_HIGH;

	//Check that denominator is not equal to 0.0
	float denom = ec_base_2 - ec_base_1;
	if(denom == 0.0f) return EC_SENSOR_IFACE_ERR_CALIBRATION;

	//Calculate slope
	iface->calibration.slope = (ec_real_2 - ec_real_1) / denom;

	//Calculate offset
	iface->calibration.offset = ec_real_1 - ec_base_1*iface->calibration.slope;

	return EC_SENSOR_IFACE_OK;
}


ec_sensor_iface_status_t ec_sensor_iface_save_calibration_constants(ec_sensor_iface_t *iface){

}

ec_sensor_iface_status_t ec_sensor_iface_load_calibration_constants(ec_sensor_iface_t *iface){

}
