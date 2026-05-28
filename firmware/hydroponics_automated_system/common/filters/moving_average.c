/*Includes*/
#include "filters/moving_average.h"
#include <string.h>

/*Private functions*/
static bool _moving_average_is_power_of_two(uint16_t value){
	return (value != 0) && ((value & (value - 1)) == 0);
}

static bool _moving_average_is_filter_ready(moving_average_handle_t *hfilter){
	return (hfilter->count >= hfilter->size);
}

/*Public functions*/
moving_average_status_t moving_average_init(moving_average_handle_t *hfilter, float *buffer, uint16_t size){
	//Sanity check
	if(!hfilter || !buffer) 							return MOVING_AVERAGE_ERR_NULL;
	if(!_moving_average_is_power_of_two(size))			return MOVING_AVERAGE_ERR_INVALID_SIZE;			//It is 0 or not a power of 2

	//Initialize handle with arguments
	hfilter->buffer = buffer;
	hfilter->size   = size;

	//Set initialized flag
	hfilter->is_initialized = true;

	//Reset filter
	moving_average_status_t status = moving_average_reset_filter(hfilter);
	return status;
}

moving_average_status_t moving_average_reset_filter(moving_average_handle_t *hfilter){
	//Sanity check
	if(!hfilter->is_initialized)			return MOVING_AVERAGE_ERR_NOT_INITIALIZED;
	if(!hfilter || !hfilter->buffer){
		hfilter->is_initialized = false;																//Deinitialize filter
		return MOVING_AVERAGE_ERR_NULL;
	}

	//Reset internal status
	hfilter->is_ready = false;
	hfilter->index = 0;
	hfilter->sum   = 0;
	hfilter->count = 0;
	memset(hfilter->buffer, 0.0f, hfilter->size * sizeof(float));
	return MOVING_AVERAGE_OK;
}

moving_average_status_t moving_average_process(moving_average_handle_t *hfilter, float sample){
	//Sanity check
	if(!hfilter->is_initialized)			return MOVING_AVERAGE_ERR_NOT_INITIALIZED;
	if(!hfilter || !hfilter->buffer){
		hfilter->is_initialized = false;																//Deinitialize filter
		return MOVING_AVERAGE_ERR_NULL;
	}

	//Check if filter is ready for its use
	hfilter->is_ready = _moving_average_is_filter_ready(hfilter);

	//Substract element at index position from the sum
	hfilter->sum -= hfilter->buffer[hfilter->index];

	//Add new sample to the buffer
	hfilter->buffer[hfilter->index] = sample;

	//Add sample to the sum
	hfilter->sum += sample;

	//Update index
	hfilter->index = (hfilter->index + 1) & (hfilter->size - 1);

	//Calculate moving average
	hfilter->moving_avg_value = hfilter->sum / (float) hfilter->size;
	return MOVING_AVERAGE_OK;
}

float moving_average_get_value(moving_average_handle_t *hfilter){
	//Sanity check
	if(!hfilter->is_initialized)			return MOVING_AVERAGE_ERR_NOT_INITIALIZED;
	if(!hfilter || !hfilter->buffer){
		hfilter->is_initialized = false;																//Deinitialize filter
		return MOVING_AVERAGE_ERR_NULL;
	}

	//Get value from internal state structure
	return hfilter->moving_avg_value;
}


