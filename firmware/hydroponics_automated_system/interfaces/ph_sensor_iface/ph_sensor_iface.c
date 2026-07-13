#include "ph_sensor_iface.h"

/*Public functions*/
ph_sensor_iface_status_t ph_sensor_iface_init(ph_sensor_iface_t *iface, analog_manager_t *mgr, analog_signal_id_t signal){
	if(iface == NULL)					return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	iface->is_initialized 	  		= false;
	if(mgr == NULL) 					return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(signal >= ANALOG_SIGNAL_COUNT) 	return PH_SENSOR_IFACE_STATUS_ERR_INVALID_SIGNAL;

	iface->mgr 				  		= mgr;
	iface->signal 			  		= signal;
	iface->calibration.slope_25		= 1.0f;			//Default slope and offset, calibration is required
	iface->calibration.offset 		= 0.0f;
	iface->data.compensated_voltage = 0.0f;
	iface->data.ph_value      		= 0.0f;
	iface->is_initialized 	  		= true;

	return PH_SENSOR_IFACE_STATUS_OK;
}

ph_sensor_iface_status_t ph_sensor_iface_process(ph_sensor_iface_t *iface, float temperature){
	if(iface == NULL)							return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return PH_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	if(!analog_manager_is_filter_ready(iface->mgr, iface->signal)){
		return PH_SENSOR_IFACE_STATUS_NOT_READY;
	}

	if(temperature < 0.0f || temperature > 60.0f){
		return PH_SENSOR_IFACE_STATUS_ERR_CORRUPTED_TEMPERATURE;
	}

	if(iface->calibration.slope_25 <= 0.0f){
		return PH_SENSOR_IFACE_STATUS_ERR_CALIBRATION;
	}

	float filtered_voltage = analog_manager_get_filtered_voltage(iface->mgr, iface->signal);

	//Compensate slope from temperature effects
	float compensated_slope = iface->calibration.slope_25 * (1.0f + 0.003f * (temperature - 25.0f));

	iface->data.compensated_voltage = filtered_voltage;
	iface->data.ph_value = filtered_voltage * compensated_slope + iface->calibration.offset;

	return PH_SENSOR_IFACE_STATUS_OK;
}

bool ph_sensor_iface_is_ready(ph_sensor_iface_t *iface){
	if(iface == NULL)			return false;
	if(!iface->is_initialized)	return false;

	return analog_manager_is_filter_ready(iface->mgr, iface->signal);
}

ph_sensor_iface_status_t ph_sensor_iface_get_data(ph_sensor_iface_t *iface, ph_sensor_iface_data_t *data){
	if(iface == NULL)							return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(data  == NULL)							return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return PH_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	data->compensated_voltage = iface->data.compensated_voltage;
	data->ph_value = iface->data.ph_value;

	return PH_SENSOR_IFACE_STATUS_OK;
}

ph_sensor_iface_status_t ph_sensor_iface_set_calibration(ph_sensor_iface_t *iface, ph_sensor_iface_calibration_t calibration){
	if(iface == NULL)							return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return PH_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;
	if(calibration.slope_25 <= 0.0f)			return PH_SENSOR_IFACE_STATUS_ERR_CALIBRATION;

	iface->calibration = calibration;

	return PH_SENSOR_IFACE_STATUS_OK;
}

ph_sensor_iface_status_t ph_sensor_iface_reset(ph_sensor_iface_t *iface){
	if(iface == NULL)							return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return PH_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	if(analog_manager_reset_filter(iface->mgr, iface->signal) != ANALOG_MANAGER_STATUS_OK){
		return PH_SENSOR_IFACE_STATUS_ERR_RESET;
	}

	iface->data.compensated_voltage = 0.0f;
	iface->data.ph_value			= 0.0f;

	return PH_SENSOR_IFACE_STATUS_OK;
}

ph_sensor_iface_status_t ph_sensor_iface_two_point_calibration(ph_sensor_iface_t *iface, float v_high, float v_low){
	if(iface == NULL)							return PH_SENSOR_IFACE_STATUS_ERR_NULL_POINTER;
	if(!iface->is_initialized)					return PH_SENSOR_IFACE_STATUS_ERR_NOT_INITIALIZED;

	float denom = v_high - v_low;
	if(denom == 0.0f) return PH_SENSOR_IFACE_STATUS_ERR_CALIBRATION;

	iface->calibration.slope_25 = (PH_SENSOR_STANDARD_PATTERN_HIGH - PH_SENSOR_STANDARD_PATTERN_LOW) / denom;
	iface->calibration.offset   = PH_SENSOR_STANDARD_PATTERN_LOW - v_low * iface->calibration.slope_25;

	return PH_SENSOR_IFACE_STATUS_OK;
}
