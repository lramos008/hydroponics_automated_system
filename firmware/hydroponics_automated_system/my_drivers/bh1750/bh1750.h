#pragma once

/**
 * @file bh1750.h
 * @brief Non-blocking driver for the BH1750 ambient light sensor.
 *
 * This driver exposes a small finite-state-machine API for one-time
 * illuminance measurements through the project I2C manager.
 */

/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "i2c_manager/i2c_manager.h"

/*Public enums*/
/**
 * @brief Driver operation status codes.
 */
typedef enum{
	BH1750_STATUS_OK,						/**< Operation completed successfully. */
	//Device status
	BH1750_STATUS_BUSY,						/**< Driver, device, or I2C manager is busy. */
	//Initialization / check
	BH1750_STATUS_ERR_NOT_INITIALIZED,		/**< Driver instance was not initialized. */
	BH1750_STATUS_ERR_NULL_POINTER,			/**< A required pointer argument is NULL. */
	BH1750_STATUS_ERR_INVALID_RESOLUTION,	/**< Configured resolution mode is not valid. */
	//I2C status
	BH1750_STATUS_ERR_TIMEOUT,				/**< I2C transaction timed out. */
	BH1750_STATUS_ERR_I2C_MGR,				/**< I2C manager reported an internal error. */
	//State machine
	BH1750_STATUS_ERR_INVALID_STATE,		/**< State machine reached an invalid state. */
	BH1750_STATUS_ERROR						/**< Generic driver error. */
}bh1750_status_t;

/**
 * @brief Internal driver state machine states.
 */
typedef enum{
	BH1750_STATE_IDLE = 0,					/**< Driver is idle and ready to start a measurement. */
	BH1750_STATE_STARTING_MEASUREMENT,		/**< Driver is sending the measurement command. */
	BH1750_STATE_WAITING_MEASUREMENT,		/**< Driver is waiting for the conversion time to elapse. */
	BH1750_STATE_READING_MEASUREMENT,		/**< Driver is reading the sensor measurement bytes. */
	BH1750_STATE_MEASUREMENT_IS_READY,		/**< A measurement is ready to be consumed. */
	BH1750_STATE_RESETTING,					/**< Driver is sending the sensor reset sequence. */
	BH1750_STATE_ERROR						/**< Driver is latched in error state until reset. */
}bh1750_state_t;

/**
 * @brief BH1750 measurement resolution modes.
 */
typedef enum{
	BH1750_RESOLUTION_LOW,					/**< Low resolution mode, typical conversion time 24 ms. */
	BH1750_RESOLUTION_HIGH,					/**< High resolution mode, typical conversion time 180 ms. */
	BH1750_RESOLUTION_HIGH_2,				/**< High resolution mode 2, typical conversion time 180 ms. */
	BH1750_RESOLUTION_MAX_COUNT				/**< Number of valid resolution modes. */
}bh1750_resolution_mode_t;

/*Public structures*/
/**
 * @brief BH1750 driver configuration.
 */
typedef struct{
	i2c_manager_t *mgr;						/**< Pointer to the I2C manager used by the driver. */
	bh1750_resolution_mode_t res_mode;		/**< Resolution mode used for one-time measurements. */
	uint8_t dev_address;					/**< I2C device address in the format expected by i2c_manager. */
}bh1750_config_t;

/**
 * @brief BH1750 measurement data.
 */
typedef struct{
	uint16_t raw_value;						/**< Raw 16-bit measurement value read from the sensor. */
	float lux;								/**< Converted illuminance value in lux. */
}bh1750_data_t;

/**
 * @brief BH1750 driver instance.
 *
 * The application owns this structure and must keep it alive while the driver
 * is in use. Its fields are exposed for static allocation, but application code
 * should treat them as driver-owned after calling @ref bh1750_init.
 */
typedef struct{
	bh1750_config_t cfg;					/**< Active driver configuration. */
	bh1750_state_t state;					/**< Current state machine state. */
	bh1750_status_t last_status;				/**< Last status returned by the state machine. */
	bh1750_status_t error_cause;				/**< Original error that caused @ref BH1750_STATE_ERROR. */
	bh1750_data_t data;						/**< Last measurement data stored by the driver. */
	TickType_t measurement_start_tick;		/**< FreeRTOS tick captured when conversion starts. */
	TickType_t measurement_wait_ticks;		/**< Required conversion wait time in FreeRTOS ticks. */
	bool is_initialized;						/**< Indicates whether the instance was initialized. */
	bool start_requested;					/**< Deferred start request consumed by the state machine. */
	bool reset_requested;					/**< Deferred reset request consumed by the state machine. */
	bool data_consumed;						/**< Indicates whether the ready measurement was read. */
}bh1750_t;

/*API functions*/
//State machine API
/**
 * @brief Initialize a BH1750 driver instance.
 *
 * @param[out] dev Pointer to driver instance to initialize.
 * @param[in] cfg Pointer to driver configuration. The contents are copied into @p dev.
 *
 * @retval BH1750_STATUS_OK Driver initialized successfully.
 * @retval BH1750_STATUS_ERR_NULL_POINTER @p dev, @p cfg, or @p cfg->mgr is NULL.
 * @retval BH1750_STATUS_ERR_INVALID_RESOLUTION @p cfg->res_mode is not valid.
 */
bh1750_status_t bh1750_init(bh1750_t *dev, bh1750_config_t *cfg);

/**
 * @brief Request a new one-time measurement.
 *
 * This function only queues the request. Call @ref bh1750_process periodically
 * until @ref bh1750_is_ready returns true.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @retval BH1750_STATUS_OK Measurement request accepted.
 * @retval BH1750_STATUS_BUSY Driver is not idle.
 * @retval BH1750_STATUS_ERR_NULL_POINTER @p dev is NULL.
 * @retval BH1750_STATUS_ERR_NOT_INITIALIZED @p dev was not initialized.
 * @retval BH1750_STATUS_ERROR Driver is in error state.
 */
bh1750_status_t bh1750_start_measurement(bh1750_t *dev);

/**
 * @brief Advance the BH1750 state machine.
 *
 * Call this function periodically from the application task. It performs I2C
 * operations when needed and never blocks beyond the I2C manager transaction
 * timeout.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @return Current driver status after processing one state-machine step.
 */
bh1750_status_t bh1750_process(bh1750_t *dev);

/**
 * @brief Check whether a measurement is ready to be read.
 *
 * @param[in] dev Initialized driver instance.
 *
 * @retval true A measurement is ready.
 * @retval false No measurement is ready, @p dev is NULL, or @p dev is not initialized.
 */
bool		    bh1750_is_ready(bh1750_t *dev);

/**
 * @brief Get the last completed measurement.
 *
 * The raw sensor value is copied to @p data and converted to lux using
 * lux = raw_value / 1.2. After a successful read, the state machine
 * returns to idle on a subsequent @ref bh1750_process call.
 *
 * @param[in,out] dev Initialized driver instance.
 * @param[out] data Destination for raw and converted measurement values.
 *
 * @retval BH1750_STATUS_OK Measurement copied successfully.
 * @retval BH1750_STATUS_BUSY Measurement is not ready yet.
 * @retval BH1750_STATUS_ERR_NULL_POINTER @p dev or @p data is NULL.
 * @retval BH1750_STATUS_ERR_NOT_INITIALIZED @p dev was not initialized.
 */
bh1750_status_t bh1750_get_data(bh1750_t *dev, bh1750_data_t *data);
//Device control API
/**
 * @brief Request a BH1750 device reset.
 *
 * This function queues the reset request. Call @ref bh1750_process periodically
 * to execute it.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @retval BH1750_STATUS_OK Reset request accepted.
 * @retval BH1750_STATUS_ERR_NULL_POINTER @p dev is NULL.
 * @retval BH1750_STATUS_ERR_NOT_INITIALIZED @p dev was not initialized.
 */
bh1750_status_t bh1750_reset(bh1750_t *dev);
