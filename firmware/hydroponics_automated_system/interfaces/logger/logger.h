#pragma once
/*Includes*/
#include <stdint.h>
#include <stdbool.h>

/*Public enums*/
//Status
typedef enum{
	LOGGER_OK,
	LOGGER_BUFFER_FULL,
	LOGGER_BUFFER_EMPTY,
	LOGGER_ERR_NULL,
	LOGGER_ERR_MOUNT,
	LOGGER_ERR_UNMOUNT,
	LOGGER_ERR_OPEN,
	LOGGER_ERR_CLOSE,
	LOGGER_ERR_WRITE,
	LOGGER_ERR_READ,
	LOGGER_ERR_SYNC,
	LOGGER_ERR_NO_MEDIA,
	LOGGER_ERR_INVALID_TIMESTAMP
}logger_err_t;

//Events
typedef enum{
	LOGGER_EVENT_PUMP_ON,
	LOGGER_EVENT_LOW_LEVEL,
	LOGGER_EVENT_SD_ERROR
}logger_event_t;

/*Public structures*/
typedef struct{
	float environment_temp;
	float environment_hum;
	float environment_lux;
	float solution_temp;
	float solution_ph;
	float solution_ec;
	bool is_solution_level_low;
	uint8_t reserved[3];
	uint32_t timestamp;
}logger_data_t;											//This structure occupies 32 bytes

/*Public API*/
logger_err_t logger_start(void);
logger_err_t logger_log_data(logger_data_t *data);
logger_err_t logger_log_event(logger_event_t event, uint32_t timestamp);
logger_err_t logger_flush(void);
logger_err_t logger_read_last_n_logs(uint32_t n, logger_data_t *buffer, uint32_t *read_count);
logger_err_t logger_stop(void);
bool 		 logger_is_running(void);

