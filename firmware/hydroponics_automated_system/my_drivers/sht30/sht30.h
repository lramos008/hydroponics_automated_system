#pragma once
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "i2c_manager/i2c_manager.h"

/*Public enums*/
typedef enum{
	SHT30_STATUS_OK = 0,
	//Device status
	SHT30_STATUS_BUSY,
	//Initialization / check
	SHT30_STATUS_ERR_NOT_INITIALIZED,
	SHT30_STATUS_ERR_NULL_POINTER,
	SHT30_STATUS_ERR_INVALID_REPEATABILITY,
	SHT30_STATUS_ERR_INVALID_CLK_STRETCHING,
	//I2C status
	SHT30_STATUS_ERR_TIMEOUT,
	SHT30_STATUS_ERR_CRC,
	SHT30_STATUS_ERR_I2C_MGR,
	//State machine
	SHT30_STATUS_ERR_INVALID_STATE,
	SHT30_STATUS_ERROR
}sht30_status_t;

typedef enum{
	SHT30_STATE_POWER_UP_WAIT = 0,
	SHT30_STATE_IDLE,
	SHT30_STATE_STARTING_MEASUREMENT,
	SHT30_STATE_WAITING_MEASUREMENT,
	SHT30_STATE_READING_MEASUREMENT,
	SHT30_STATE_MEASUREMENT_IS_READY,
	SHT30_STATE_RESETTING,
	SHT30_STATE_WAITING_RESET,
	SHT30_STATE_ERROR
}sht30_state_t;

typedef enum{
	SHT30_HIGH_REPEATABILITY = 0,
	SHT30_MEDIUM_REPEATABILITY,
	SHT30_LOW_REPEATABILITY,
	SHT30_REPEATABILITY_MAX_COUNT
}sht30_repeatability_t;

typedef enum{
	SHT30_CLK_STRETCHING_ENABLED = 0,
	SHT30_CLK_STRETCHING_DISABLED,
	SHT30_CLK_STRETCHING_MAX_COUNT
}sht30_clk_stretching_t;

/*Public structures*/
typedef struct{
	i2c_manager_t *mgr;
	uint8_t dev_address;
	sht30_repeatability_t repeatability;
	sht30_clk_stretching_t clk_stretching;
}sht30_config_t;

typedef struct{
	uint16_t temp_raw;
	uint16_t hr_raw;
	float temperature;
	float humidity;
}sht30_data_t;

typedef struct{
	sht30_config_t cfg;
	sht30_state_t state;
	sht30_status_t last_status;
	sht30_status_t error_cause;
	sht30_data_t data;
	TickType_t power_up_start_tick;
	TickType_t measurement_start_tick;
	TickType_t measurement_wait_ticks;
	TickType_t reset_start_tick;
	bool is_initialized;
	bool start_requested;
	bool reset_requested;
	bool data_consumed;
}sht30_t;

/*API functions*/
sht30_status_t sht30_init(sht30_t *dev, sht30_config_t *cfg);
sht30_status_t sht30_start_measurement(sht30_t *dev);
sht30_status_t sht30_process(sht30_t *dev);
bool		  sht30_is_ready(sht30_t *dev);
sht30_status_t sht30_get_data(sht30_t *dev, sht30_data_t *data);
sht30_status_t sht30_reset(sht30_t *dev);
