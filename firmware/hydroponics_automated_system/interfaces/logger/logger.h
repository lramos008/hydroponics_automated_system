#pragma once
/*Includes*/
#include <stdint.h>
#include <stdbool.h>

/*Public enums*/
typedef enum{
	LOGGER_OK,
	LOGGER_ERR_MOUNT,
	LOGGER_ERR_OPEN,
	LOGGER_ERR_WRITE,
	LOGGER_ERR_SYNC,
	LOGGER_ERR_NO_MEDIA,
	LOGGER_ERR_INVALID_TIMESTAMP,
	LOGGER_ERR_NOT_INITIALIZED,
	LOGGER_ERR_BUFFER_FULL
}logger_err_t;

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
}logger_data_t;

/*Public API*/
logger_err_t logger_init(void);
logger_err_t logger_save_log(logger_data_t *data);
logger_err_t logger_flush(void);

