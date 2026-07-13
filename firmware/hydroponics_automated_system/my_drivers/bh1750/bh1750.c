/**
 * @file bh1750.c
 * @brief BH1750 ambient light sensor driver implementation.
 */

/*Includes*/
#include "bh1750.h"

/*Defines / Macros*/

//System parameters
#define BH1750_COMMAND_SIZE_BYTES							1
#define BH1750_MEASUREMENT_SIZE_BYTES						2
#define BH1750_HIGH_RESOLUTION_MEASUREMENT_TIME_MS 			180
#define BH1750_HIGH_RESOLUTION_2_MEASUREMENT_TIME_MS 		180
#define BH1750_LOW_RESOLUTION_MEASUREMENT_TIME_MS 			24
#define BH1750_LUX_CONVERSION_FACTOR						1.2f

//Commands
#define BH1750_CMD_POWER_DOWN 								0x00
#define BH1750_CMD_POWER_ON   								0x01
#define BH1750_CMD_RESET	  								0x07
#define BH1750_CMD_START_CONTINUOUS_HI_RES_MEASUREMENT		0x10
#define BH1750_CMD_START_CONTINUOUS_HI_RES_2_MEASUREMENT	0x11
#define BH1750_CMD_START_CONTINUOUS_LOW_RES_MEASUREMENT		0x13
#define BH1750_CMD_START_ONE_TIME_HI_RES_MEASUREMENT  		0x20
#define BH1750_CMD_START_ONE_TIME_HI_RES_2_MEASUREMENT  	0x21
#define BH1750_CMD_START_ONE_TIME_LOW_RES_MEASUREMENT		0x23


/*Private functions*/
/**
 * @brief Translate an I2C manager status to a BH1750 driver status.
 *
 * @param[in] i2c_mgr_status Status returned by the I2C manager.
 *
 * @return Equivalent BH1750 driver status.
 */
static bh1750_status_t _bh1750_i2c_mgr_status_translate(i2c_manager_status_t i2c_mgr_status){
	switch(i2c_mgr_status){
	case I2C_MGR_STATUS_OK:						return BH1750_STATUS_OK;
	case I2C_MGR_STATUS_ERR_NULL_POINTER:		return BH1750_STATUS_ERR_NULL_POINTER;
	case I2C_MGR_STATUS_BUSY:					return BH1750_STATUS_BUSY;
	case I2C_MGR_STATUS_ERR_TIMEOUT:			return BH1750_STATUS_ERR_TIMEOUT;
	case I2C_MGR_STATUS_ERR_MUTEX:				return BH1750_STATUS_ERR_I2C_MGR;
	default:									return BH1750_STATUS_ERROR;
	}
}

/**
 * @brief Send a single-byte command to the BH1750.
 *
 * @param[in,out] dev Initialized driver instance.
 * @param[in] cmd Command byte to send.
 *
 * @retval BH1750_STATUS_OK Command sent successfully.
 * @retval BH1750_STATUS_ERR_NULL_POINTER @p dev is NULL.
 * @retval BH1750_STATUS_ERR_NOT_INITIALIZED @p dev was not initialized.
 * @retval BH1750_STATUS_BUSY I2C manager is busy.
 * @retval BH1750_STATUS_ERR_TIMEOUT I2C transaction timed out.
 * @retval BH1750_STATUS_ERR_I2C_MGR I2C manager reported an internal error.
 */
static bh1750_status_t _bh1750_send_cmd(bh1750_t *dev, const uint8_t cmd){
	if(dev == NULL) 			return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)	return BH1750_STATUS_ERR_NOT_INITIALIZED;

	i2c_manager_status_t i2c_status = i2c_manager_write(dev->cfg.mgr, dev->cfg.dev_address, &cmd, BH1750_COMMAND_SIZE_BYTES, I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS);
	return _bh1750_i2c_mgr_status_translate(i2c_status);
}

/**
 * @brief Read the two-byte raw measurement from the BH1750.
 *
 * The raw value is stored in the driver instance. Conversion to lux is performed
 * by @ref bh1750_get_data.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @retval BH1750_STATUS_OK Measurement read successfully.
 * @retval BH1750_STATUS_BUSY Driver is not in the read state or the I2C manager is busy.
 * @retval BH1750_STATUS_ERR_NULL_POINTER @p dev is NULL.
 * @retval BH1750_STATUS_ERR_NOT_INITIALIZED @p dev was not initialized.
 * @retval BH1750_STATUS_ERR_TIMEOUT I2C transaction timed out.
 * @retval BH1750_STATUS_ERR_I2C_MGR I2C manager reported an internal error.
 */
static bh1750_status_t _bh1750_read_measurement(bh1750_t *dev){
	if(dev == NULL) 			return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)	return BH1750_STATUS_ERR_NOT_INITIALIZED;

	if(dev->state != BH1750_STATE_READING_MEASUREMENT){
		return BH1750_STATUS_BUSY;
	}

	uint8_t measurement[BH1750_MEASUREMENT_SIZE_BYTES] = {0};
	i2c_manager_status_t i2c_status = i2c_manager_read(dev->cfg.mgr, dev->cfg.dev_address, measurement, BH1750_MEASUREMENT_SIZE_BYTES, I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS);
	if(i2c_status != I2C_MGR_STATUS_OK){
		return _bh1750_i2c_mgr_status_translate(i2c_status);
	}

	//Save measurement
	dev->data.raw_value =  (((uint16_t) measurement[0]) << 8) | measurement[1];
	return BH1750_STATUS_OK;
}

/**
 * @brief Send the BH1750 power-on command.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @return Status returned by the command transaction.
 */
static bh1750_status_t _bh1750_power_on(bh1750_t *dev){
	uint8_t cmd = BH1750_CMD_POWER_ON;
	return _bh1750_send_cmd(dev, cmd);
}

/**
 * @brief Send the BH1750 power-down command.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @return Status returned by the command transaction.
 */
static bh1750_status_t _bh1750_power_down(bh1750_t *dev){
	uint8_t cmd = BH1750_CMD_POWER_DOWN;
	return _bh1750_send_cmd(dev, cmd);
}

/**
 * @brief Send the BH1750 reset command.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @return Status returned by the command transaction.
 */
static bh1750_status_t _bh1750_reset(bh1750_t *dev){
	uint8_t cmd = BH1750_CMD_RESET;
	return _bh1750_send_cmd(dev, cmd);
}

/**
 * @brief Start a one-time measurement using the configured resolution.
 *
 * The function powers on the sensor, selects the command that matches the
 * configured resolution, and stores the corresponding conversion wait time.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @return Status returned by the power-on or measurement command transaction.
 */
static bh1750_status_t _bh1750_request_measurement(bh1750_t *dev){
	//Power on BH1750 in order to start a measurement
	bh1750_status_t status;
	status = _bh1750_power_on(dev);
	if(status != BH1750_STATUS_OK){
		return status;
	}

	//Select start measurement cmd and wait time based on resolution
	uint8_t cmd;
	switch(dev->cfg.res_mode){
	case BH1750_RESOLUTION_LOW:
		cmd = BH1750_CMD_START_ONE_TIME_LOW_RES_MEASUREMENT;
		dev->measurement_wait_ticks = pdMS_TO_TICKS(BH1750_LOW_RESOLUTION_MEASUREMENT_TIME_MS);
		break;
	case BH1750_RESOLUTION_HIGH:
		cmd = BH1750_CMD_START_ONE_TIME_HI_RES_MEASUREMENT;
		dev->measurement_wait_ticks = pdMS_TO_TICKS(BH1750_HIGH_RESOLUTION_MEASUREMENT_TIME_MS);
		break;
	case BH1750_RESOLUTION_HIGH_2:
		cmd = BH1750_CMD_START_ONE_TIME_HI_RES_2_MEASUREMENT;
		dev->measurement_wait_ticks = pdMS_TO_TICKS(BH1750_HIGH_RESOLUTION_2_MEASUREMENT_TIME_MS);
		break;
	default:
		cmd = BH1750_CMD_START_ONE_TIME_HI_RES_MEASUREMENT;
		dev->measurement_wait_ticks = pdMS_TO_TICKS(BH1750_HIGH_RESOLUTION_MEASUREMENT_TIME_MS);
		break;
	}

	status = _bh1750_send_cmd(dev, cmd);

	return status;
}

/**
 * @brief Execute the BH1750 reset sequence.
 *
 * The sensor must be powered on before accepting the reset command.
 *
 * @param[in,out] dev Initialized driver instance.
 *
 * @return Status returned by the power-on or reset command transaction.
 */
static bh1750_status_t _bh1750_request_reset(bh1750_t *dev){
	//Power on BH1750 in order to reset the device
	bh1750_status_t status;
	status = _bh1750_power_on(dev);
	if(status != BH1750_STATUS_OK){
		return status;
	}

	status = _bh1750_reset(dev);

	return status;
}

/**
 * @brief Reset the driver finite-state-machine context.
 *
 * This function clears pending requests, stored data, status fields, and timing
 * data without changing the active configuration or initialization flag.
 *
 * @param[in,out] dev Driver instance to reset.
 */
static void _bh1750_fsm_reset(bh1750_t *dev){
	dev->last_status 	 		 = BH1750_STATUS_OK;
	dev->error_cause 			 = BH1750_STATUS_OK;
	dev->state					 = BH1750_STATE_IDLE;
	dev->data.raw_value		 	 = 0;
	dev->data.lux			 	 = 0.0f;
	dev->measurement_start_tick	 = 0;
	dev->measurement_wait_ticks  = 0;
	dev->start_requested		 = false;
	dev->reset_requested		 = false;
	dev->data_consumed			 = false;
}



/*Public functions*/
/**
 * @copydoc bh1750_init
 */
bh1750_status_t bh1750_init(bh1750_t *dev, bh1750_config_t *cfg){
	if(dev == NULL)											return BH1750_STATUS_ERR_NULL_POINTER;
	if(cfg == NULL)											return BH1750_STATUS_ERR_NULL_POINTER;
	if(cfg->mgr == NULL)									return BH1750_STATUS_ERR_NULL_POINTER;
	if(cfg->res_mode >= BH1750_RESOLUTION_MAX_COUNT)		return BH1750_STATUS_ERR_INVALID_RESOLUTION;

	//Fill bh1750 with current config and set initial values
	dev->is_initialized	 		 = false;
	dev->cfg.mgr 		 		 = cfg->mgr;
	dev->cfg.dev_address 		 = cfg->dev_address;
	dev->cfg.res_mode 	 		 = cfg->res_mode;
	//Reset fsm internal state
	_bh1750_fsm_reset(dev);
	dev->is_initialized	 		 = true;
	return BH1750_STATUS_OK;
}

/**
 * @copydoc bh1750_start_measurement
 */
bh1750_status_t bh1750_start_measurement(bh1750_t *dev){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	//Check if an error ocurred
	if(dev->state == BH1750_STATE_ERROR){
		return BH1750_STATUS_ERROR;
	}

	//Check if current state is idle, otherwise tell the user to wait
	if(dev->state != BH1750_STATE_IDLE){
		return BH1750_STATUS_BUSY;
	}

	//Signal state machine to request
	dev->start_requested = true;
	return BH1750_STATUS_OK;
}

/**
 * @copydoc bh1750_process
 */
bh1750_status_t bh1750_process(bh1750_t *dev){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	//Check if reset was requested
	if(dev->reset_requested && dev->state != BH1750_STATE_RESETTING){
		dev->state = BH1750_STATE_RESETTING;
		dev->reset_requested = false;
	}

	switch(dev->state){
	case BH1750_STATE_IDLE:
		if(dev->start_requested){
			dev->state = BH1750_STATE_STARTING_MEASUREMENT;
			dev->start_requested = false;
		}
		dev->last_status = BH1750_STATUS_OK;
		break;
	case BH1750_STATE_STARTING_MEASUREMENT:
		dev->last_status = _bh1750_request_measurement(dev);
		if(dev->last_status == BH1750_STATUS_OK){
			dev->state = BH1750_STATE_WAITING_MEASUREMENT;
			dev->measurement_start_tick = xTaskGetTickCount();
		}
		else if(dev->last_status == BH1750_STATUS_BUSY){
			//Do nothing, wait for the next cycle and try again
		}
		else{
			dev->state 		 = BH1750_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = BH1750_STATUS_ERROR;
		}
		break;
	case BH1750_STATE_WAITING_MEASUREMENT:
	{
		//Wait until the read time is reached
		TickType_t elapsed_time_tick = xTaskGetTickCount();
		bool was_measurement_read_time_reached = elapsed_time_tick - dev->measurement_start_tick >= dev->measurement_wait_ticks;
		if(was_measurement_read_time_reached){
			dev->state = BH1750_STATE_READING_MEASUREMENT;
		}
		dev->last_status = BH1750_STATUS_BUSY;

	}
		break;
	case BH1750_STATE_READING_MEASUREMENT:
		dev->last_status = _bh1750_read_measurement(dev);
		if(dev->last_status == BH1750_STATUS_OK){
			dev->state = BH1750_STATE_MEASUREMENT_IS_READY;
		}
		else if(dev->last_status == BH1750_STATUS_BUSY){
			//Wait for I2C bus to be freed
		}
		else{
			dev->state = BH1750_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = BH1750_STATUS_ERROR;
		}
		break;
	case BH1750_STATE_MEASUREMENT_IS_READY:
		if(dev->data_consumed){
			_bh1750_fsm_reset(dev);								//Reset fsm to allow a new measurement
		}
		else{
			dev->last_status = BH1750_STATUS_OK;
		}
		break;
	case BH1750_STATE_RESETTING:
		dev->last_status = _bh1750_request_reset(dev);
		if(dev->last_status == BH1750_STATUS_OK){
			_bh1750_fsm_reset(dev);								//Reset fsm to allow a new measurement
		}
		else if(dev->last_status == BH1750_STATUS_BUSY){
			//Do nothing, wait for the next cycle
		}
		else{
			dev->state = BH1750_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = BH1750_STATUS_ERROR;
		}
		break;
	case BH1750_STATE_ERROR:
		//Wait for a reset request
		dev->last_status = BH1750_STATUS_ERROR;
		break;
	default:
		dev->state		 = BH1750_STATE_ERROR;					//There is an unknown state, so handle it as an error
		dev->error_cause = BH1750_STATUS_ERR_INVALID_STATE;
		dev->last_status = BH1750_STATUS_ERROR;
	}

	return dev->last_status;
}

/**
 * @copydoc bh1750_is_ready
 */
bool bh1750_is_ready(bh1750_t *dev){
	if(dev == NULL)				return false;
	if(!dev->is_initialized)	return false;
	return (dev->state == BH1750_STATE_MEASUREMENT_IS_READY);
}

/**
 * @copydoc bh1750_get_data
 */
bh1750_status_t bh1750_get_data(bh1750_t *dev, bh1750_data_t *data){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(data == NULL)					return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	if(dev->state != BH1750_STATE_MEASUREMENT_IS_READY){
		return BH1750_STATUS_BUSY;
	}

	dev->data.lux		= (float)dev->data.raw_value / BH1750_LUX_CONVERSION_FACTOR;
	data->raw_value 	= dev->data.raw_value;
	data->lux			= dev->data.lux;
	dev->data_consumed  = true;
	return BH1750_STATUS_OK;
}

/**
 * @copydoc bh1750_reset
 */
bh1750_status_t bh1750_reset(bh1750_t *dev){
	if(dev == NULL) 			return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized) 	return BH1750_STATUS_ERR_NOT_INITIALIZED;

	dev->reset_requested = true;
	return BH1750_STATUS_OK;
}
