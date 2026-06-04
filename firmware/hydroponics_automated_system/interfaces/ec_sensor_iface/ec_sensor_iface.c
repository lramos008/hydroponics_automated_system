#include "ec_sensor_iface.h"

/*Public functions*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface){
	if(iface == NULL)							return EC_SENSOR_IFACE_ERR_NULL;
	if(iface->analog == NULL) 					return EC_SENSOR_IFACE_ERR_NULL;
	if(iface->channel >= ANALOG_CHANNEL_COUNT) 	return EC_SENSOR_IFACE_ERR_INVALID_CHANNEL;

	//Init internal variables from iface
	iface->ec_value = 0.0f;
	iface->is_initialized = true;

	return EC_SENSOR_IFACE_OK;
}

float ec_sensor_iface_get_ec_value(ec_sensor_iface_t *iface, float temperature){
	if(iface == NULL)			return -1.0f;
	if(!iface->is_initialized) 	return -1.0f;

	//Read filtered voltage
	float filtered_voltage = analog_manager_get_filtered_voltage(iface->analog, iface->channel);
	if(filtered_voltage < 0.0f) return filtered_voltage;

	//Compensate voltage from temperature effects
	float compensation_coeff = 1.0 + 0.02 * (temperature - 25.0);
	float compensated_voltage = filtered_voltage / compensation_coeff;							//Compensated voltage for 25
	//Calculate EC value by applying calib factors due to system imperfections
	iface->ec_value = compensated_voltage * iface->calibration.slope + iface->calibration.offset;

	return iface->ec_value;
}

ec_sensor_iface_status_t ec_sensor_iface_reset(ec_sensor_iface_t *iface){
	//Sanity check
	if(iface == NULL)			return EC_SENSOR_IFACE_ERR_NULL;
	if(!iface->is_initialized)	return EC_SENSOR_IFACE_ERR_NOT_INITIALIZED;

	//Reset moving average filter
	if(analog_manager_reset_filter(iface->analog, iface->channel) != ANALOG_MANAGER_OK){
		return EC_SENSOR_IFACE_ERR_RESET;
	}
	//Reset internal variables
	iface->ec_value = 0.0f;

	return EC_SENSOR_IFACE_OK;
}

ec_sensor_iface_status_t ec_sensor_iface_set_calibration(ec_sensor_iface_t *iface, ec_sensor_calibration_t cal){
	if(iface == NULL)			return EC_SENSOR_IFACE_ERR_NULL;
	if(!iface->is_initialized)	return EC_SENSOR_IFACE_ERR_NOT_INITIALIZED;

	//Set calib constants
	iface->calibration.slope  = cal.slope;
	iface->calibration.offset = cal.offset;

	return EC_SENSOR_IFACE_OK;
}

ec_sensor_iface_status_t ec_sensor_iface_set_default_calibration(ec_sensor_iface_t *iface){
	if(iface == NULL)			return EC_SENSOR_IFACE_ERR_NULL;
	if(!iface->is_initialized)	return EC_SENSOR_IFACE_ERR_NOT_INITIALIZED;

	//Set default calib constants
	iface->calibration.slope  = 1.0f;
	iface->calibration.offset = 0.0f;

	return EC_SENSOR_IFACE_OK;
}

bool ec_sensor_iface_is_ready(ec_sensor_iface_t *iface){
	if(iface == NULL)			return false;
	if(!iface->is_initialized)	return false;

	return iface->analog->filters[iface->channel].is_ready;
}
