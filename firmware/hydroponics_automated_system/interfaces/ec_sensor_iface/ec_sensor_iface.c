#include "ec_sensor_iface.h"

/*Public functions*/
ec_sensor_iface_status_t ec_sensor_iface_init(ec_sensor_iface_t *iface, analog_manager_t *mgr, analog_signal_id_t signal){
	if(iface == NULL)					return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	iface->is_initialized 	  		= false;
	if(mgr == NULL) 					return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(signal >= ANALOG_SIGNAL_COUNT) 	return EC_SENSOR_IFACE_STATUS_ERR_INVALID_SIGNAL;

	iface->mgr 				  		= mgr;
	iface->signal 			  		= signal;
	iface->calibration.slope  		= 1.0f;			//Default slope and offset, calibration is required
	iface->calibration.offset 		= 0.0f;
	iface->data.compensated_voltage = 0.0f;
	iface->data.ec_value      		= 0.0f;
	iface->is_initialized 	  		= true;

	return EC_SENSOR_IFACE_STATUS_OK;
}

ec_sensor_iface_status_t ec_sensor_iface_process(ec_sensor_iface_t *iface, float temperature){
	if(iface == NULL)							return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return EC_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	if(!analog_manager_is_filter_ready(iface->mgr, iface->signal)){
		return EC_SENSOR_IFACE_STATUS_NOT_READY;
	}

	if(temperature < 0.0f || temperature > 60.0f){
		return EC_SENSOR_IFACE_STATUS_ERR_CORRUPTED_TEMPERATURE;
	}

	if(iface->calibration.slope <= 0.0f){
		return EC_SENSOR_IFACE_STATUS_ERR_CALIBRATION;
	}

	float filtered_voltage = analog_manager_get_filtered_voltage(iface->mgr, iface->signal);

	//Compensate voltage from temperature effects
	float compensation_coeff = 1.0f + 0.02f * (temperature - 25.0f);
	float compensated_voltage = filtered_voltage / compensation_coeff;

	iface->data.compensated_voltage = compensated_voltage;
	iface->data.ec_value = compensated_voltage * iface->calibration.slope + iface->calibration.offset;

	return EC_SENSOR_IFACE_STATUS_OK;
}

bool ec_sensor_iface_is_ready(ec_sensor_iface_t *iface){
	if(iface == NULL)			return false;
	if(!iface->is_initialized)	return false;

	return analog_manager_is_filter_ready(iface->mgr, iface->signal);
}

ec_sensor_iface_status_t ec_sensor_iface_get_data(ec_sensor_iface_t *iface, ec_sensor_iface_data_t *data){
	if(iface == NULL)							return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(data  == NULL)							return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return EC_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	data->compensated_voltage = iface->data.compensated_voltage;
	data->ec_value = iface->data.ec_value;

	return EC_SENSOR_IFACE_STATUS_OK;
}


ec_sensor_iface_status_t ec_sensor_iface_set_calibration(ec_sensor_iface_t *iface, ec_sensor_iface_calibration_t calibration){
	if(iface == NULL)							return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return EC_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;
	if(calibration.slope <= 0.0f)				return EC_SENSOR_IFACE_STATUS_ERR_CALIBRATION;

	iface->calibration = calibration;

	return EC_SENSOR_IFACE_STATUS_OK;
}

ec_sensor_iface_status_t ec_sensor_iface_reset(ec_sensor_iface_t *iface){
	if(iface == NULL)							return EC_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return EC_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	if(analog_manager_reset_filter(iface->mgr, iface->signal) != ANALOG_MANAGER_STATUS_OK){
		return EC_SENSOR_IFACE_STATUS_ERR_RESET;
	}

	iface->data.compensated_voltage = 0.0f;
	iface->data.ec_value			= 0.0f;

	return EC_SENSOR_IFACE_STATUS_OK;
}
