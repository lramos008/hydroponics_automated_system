#include "gravity_tds/gravity_tds.h"
#include <string.h>

/*Defines*/
/*Adapt using cmsis dsp*/

/*Private functions*/
static gravity_tds_err_t _gravity_tds_read_adc(gravity_tds_handle_t *dev, uint32_t *raw_value){
	//Set config structure
	ADC_ChannelConfTypeDef cfg = {0};
	cfg.Channel = dev->channel;
	cfg.Rank    = 1;									//Single mode, always first
	cfg.SamplingTime = ADC_SAMPLETIME_15CYCLES;

	//Apply ADC config
	if(HAL_ADC_ConfigChannel(dev->hadc, &cfg) != HAL_OK) return GRAVITY_ERR_ADC_CONFIG;

	//Init ADC conversion
	HAL_ADC_Start(dev->hadc);
	if(HAL_ADC_PollForConversion(dev->hadc, 10) == HAL_OK){
		*raw_value = HAL_ADC_GetValue(dev->hadc);
		return GRAVITY_OK;
	}
	return GRAVITY_ERR_ADC_READING;													//read error
}

static gravity_tds_err_t _gravity_tds_update(gravity_tds_handle_t *dev){
	//Get a measurement and add it to the internal buffer
	gravity_tds_err_t err;
	uint32_t raw_value;
	err = _gravity_tds_read_adc(dev, &raw_value);
	if(err != GRAVITY_OK) return err;

	//Add latests measurement to the buffer
	dev->adc_buffer[dev->buffer_idx++] = raw_value;
	if(dev->buffer_idx >= GRAVITY_TDS_INTERNAL_BUFFER_SIZE){
		dev->buffer_idx = 0;														//Restart circular buffer index
	}

	//Average calculations
	uint32_t sum = 0;
	for(uint32_t i = 0; i < GRAVITY_TDS_INTERNAL_BUFFER_SIZE; i++){
		sum += dev->adc_buffer[i];
	}

	float avg_adc = (float) sum / GRAVITY_TDS_INTERNAL_BUFFER_SIZE;

	//Save last voltage value
	dev->last_voltage = (avg_adc * ((float) dev->vref)) / ((float)dev->adc_range);

	//Calculate compensated voltage by temperature
	float compensation_coeff = 1.0 + 0.02 * (dev->solution_temp - 25.0);
	float v_25 = dev->last_voltage / compensation_coeff;

	/*Calculate base EC. This polynom was designed to output a ppm value.
	 * In order to get EC you need to do a conversion. DFRobot used the
	 * NaCl scale where an EC of 1.0 mS/cm is equivalent to 500 ppm.
	 * Thus that factor is used to normalize the EC value.
	 */
	float base_ec = (133.42 * v_25 * v_25 * v_25 -
					 255.86 * v_25 * v_25 +
					 857.39 * v_25) / 500.0f;

	//Apply calibration factor (due to sensor imperfections)
	dev->last_ec = base_ec * dev->k_value;
	return GRAVITY_OK;
}

/*Public functions*/
gravity_tds_err_t gravity_tds_init(gravity_tds_handle_t *dev, ADC_HandleTypeDef *hadc, uint32_t channel,
								   uint32_t adc_range, float vref, float k_value, float tds_factor)		{
	//Sanity check
	if(!dev || !hadc) return GRAVITY_ERR_NULL;

	//Init handle
	dev->hadc = hadc;
	dev->channel = channel;
	dev->adc_range = adc_range;
	dev->vref = vref;
	dev->k_value = k_value;
	dev->tds_factor = tds_factor;
	dev->solution_temp = 25.0;									//Initial value, should set temperature after this
	memset(dev->adc_buffer, 0, sizeof(dev->adc_buffer));
	dev->buffer_idx = 0;
	return GRAVITY_OK;
}


gravity_tds_err_t gravity_tds_set_temperature(gravity_tds_handle_t *dev, float temp){
	//Sanity check
	if(!dev || !dev->hadc) return GRAVITY_ERR_NULL;

	//Set temperature
	dev->solution_temp = temp;
	return GRAVITY_OK;
}

gravity_tds_err_t gravity_tds_get_voltage(gravity_tds_handle_t *dev, float *voltage){
	//Sanity check
	if(!dev || !dev->hadc || !voltage) return GRAVITY_ERR_NULL;

	//Update values
	gravity_tds_err_t err = _gravity_tds_update(dev);
	if(err != GRAVITY_OK) return err;

	//Assign voltage value to pointer
	*voltage = dev->last_voltage;
	return GRAVITY_OK;
}

gravity_tds_err_t gravity_tds_get_value_ec(gravity_tds_handle_t *dev, float *ec){
	//Sanity check
	if(!dev || !dev->hadc || !ec) return GRAVITY_ERR_NULL;

	//Update values
	gravity_tds_err_t err = _gravity_tds_update(dev);
	if(err != GRAVITY_OK) return err;

	//Assign ec value to pointer
	*ec = dev->last_ec;
	return GRAVITY_OK;
}

gravity_tds_err_t gravity_tds_get_value_ppm(gravity_tds_handle_t *dev, float *ppm){
	//Sanity check
	if(!dev || !dev->hadc || !ppm) return GRAVITY_ERR_NULL;

	//Update values
	gravity_tds_err_t err = _gravity_tds_update(dev);
	if(err != GRAVITY_OK) return err;

	//Assign tds value to pointer
	*ppm = dev->last_ec * dev->tds_factor;
	return GRAVITY_OK;
}

//Assumes the solution is stabilized, use standard deviation with the buffer to know if it is
gravity_tds_err_t gravity_tds_calibrate(gravity_tds_handle_t *dev, float target_ec){
	//Sanity check
	if(!dev || !dev->hadc) return GRAVITY_ERR_NULL;

	//Get raw EC using k = 1.0
	dev->k_value = 1.0f;
	_gravity_tds_update(dev);
	float raw_ec = dev->last_ec;

	//Guard to avoid dividing by zero
	if(raw_ec < 0.1f) return GRAVITY_ERR_CALIBRATION;

	dev->k_value = target_ec / raw_ec;
	return GRAVITY_OK;
}


bool gravity_tds_is_reading_stable(gravity_tds_handle_t *dev){
	//Calculate standard deviation and decide the desired threshold to know if it is stabilized
	//Use cmsis dsp
}

