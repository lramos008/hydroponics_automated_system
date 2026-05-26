#pragma once

/*Includes*/
#include <stdint.h>
#include <stdbool.h>

/*Status*/
typedef enum{
	MOVING_AVERAGE_OK,
	MOVING_AVERAGE_ERR_NULL,
	MOVING_AVERAGE_ERR_INVALID_SIZE,
	MOVING_AVERAGE_ERR_NOT_INITIALIZED
}moving_average_status_t;


/*Handle*/
typedef struct{
	//Filter buffer parameters
	float *buffer;
	uint16_t size;
	uint16_t index;
	//Filter internal variables
	float sum;
	float moving_avg_value;
	//Filter internal status
	uint16_t count;
	bool is_initialized;
	bool is_ready;
}moving_average_handle_t;

/*API*/
moving_average_status_t moving_average_init(moving_average_handle_t *hfilter, float *buffer, uint16_t size);
moving_average_status_t moving_average_process(moving_average_handle_t *hfilter, float sample);
moving_average_status_t moving_average_get_value(moving_average_handle_t *hfilter, float *average);
moving_average_status_t moving_average_reset_filter(moving_average_handle_t *hfilter);
