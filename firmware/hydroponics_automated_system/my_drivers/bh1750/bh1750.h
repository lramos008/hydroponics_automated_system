#pragma once
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "i2c_manager/i2c_manager.h"

/*Public enums*/
typedef enum{
	BH1750_STATUS_OK,
	//Device status
	BH1750_STATUS_BUSY,
	//Initialization / check
	BH1750_STATUS_ERR_NOT_INITIALIZED,
	BH1750_STATUS_ERR_NULL_POINTER,
	BH1750_STATUS_ERR_INVALID_RESOLUTION,
	//I2C status
	BH1750_STATUS_ERR_TIMEOUT,
	BH1750_STATUS_ERR_I2C_MGR,
	//State machine
	BH1750_STATUS_ERR_INVALID_STATE,
	BH1750_STATUS_ERROR
}bh1750_status_t;


typedef enum{
	BH1750_STATE_IDLE = 0,
	BH1750_STATE_STARTING_MEASUREMENT,
	BH1750_STATE_WAITING_MEASUREMENT,
	BH1750_STATE_READING_MEASUREMENT,
	BH1750_STATE_MEASUREMENT_IS_READY,
	BH1750_STATE_RESETTING,
	BH1750_STATE_ERROR
}bh1750_state_t;

typedef enum{
	BH1750_RESOLUTION_LOW,
	BH1750_RESOLUTION_HIGH,
	BH1750_RESOLUTION_HIGH_2,
	BH1750_RESOLUTION_MAX_COUNT							//Used for resolution type check
}bh1750_resolution_mode_t;

/*Public structures*/
typedef struct{
	i2c_manager_t *mgr;
	bh1750_resolution_mode_t res_mode;
	uint8_t dev_address;
}bh1750_config_t;

typedef struct{
	uint16_t value;
}bh1750_data_t;

typedef struct{
	bh1750_config_t cfg;
	bh1750_state_t state;
	bh1750_status_t last_status;
	bh1750_status_t error_cause;
	bh1750_data_t data;
	TickType_t measurement_start_tick;
	TickType_t measurement_wait_ticks;
	bool is_initialized;
	bool start_requested;
	bool reset_requested;
	bool data_consumed;
}bh1750_t;

/*API functions*/
//State machine API
bh1750_status_t bh1750_init(bh1750_t *dev, bh1750_config_t *cfg);
bh1750_status_t bh1750_start_measurement(bh1750_t *dev);
bh1750_status_t bh1750_process(bh1750_t *dev);
bool		    bh1750_is_ready(bh1750_t *dev);
bh1750_status_t bh1750_get_data(bh1750_t *dev, bh1750_data_t *data);
//Device control API
bh1750_status_t bh1750_reset(bh1750_t *dev);
