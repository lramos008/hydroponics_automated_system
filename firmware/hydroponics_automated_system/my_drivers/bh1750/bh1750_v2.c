/*Includes*/
#include "bh1750.h"

/*Defines / Macros*/

//System parameters
#define BH1750_COMMAND_SIZE_BYTES							1
#define BH1750_MEASUREMENT_SIZE_BYTES						2
#define BH1750_HIGH_RESOLUTION_MEASUREMENT_TIME_MS 			180
#define BH1750_HIGH_RESOLUTION_2_MEASUREMENT_TIME_MS 		180
#define BH1750_LOW_RESOLUTION_MEASUREMENT_TIME_MS 			24

//Commands
#define BH1750_CMD_POWER_ON   								0x00
#define BH1750_CMD_POWER_DOWN 								0x01
#define BH1750_CMD_RESET	  								0x07
#define BH1750_CMD_START_CONTINUOUS_HI_RES_MEASUREMENT		0x10
#define BH1750_CMD_START_CONTINUOUS_HI_RES_2_MEASUREMENT	0x11
#define BH1750_CMD_START_CONTINUOUS_LOW_RES_MEASUREMENT		0x13
#define BH1750_CMD_START_ONE_TIME_HI_RES_MEASUREMENT  		0x20
#define BH1750_CMD_START_ONE_TIME_HI_RES_2_MEASUREMENT  	0x21
#define BH1750_CMD_START_ONE_TIME_LOW_RES_MEASUREMENT		0x23


/*Private functions*/
static bh1750_status_t _bh1750_i2c_mgr_status_translate(i2c_manager_status_t i2c_mgr_status){
	switch(i2c_mgr_status){
	case I2C_MGR_STATUS_OK:						return BH1750_STATUS_OK;
	case I2C_MGR_STATUS_ERR_NULL_POINTER:		return BH1750_STATUS_ERR_NULL_POINTER;
	case I2C_MGR_STATUS_BUSY:					return BH1750_STATUS_BUSY;
	case I2C_MGR_STATUS_ERR_TIMEOUT:			return BH1750_STATUS_ERR_TIMEOUT;
	default:									return BH1750_STATUS_ERROR;
	}
}

static bh1750_status_t _bh1750_send_cmd(bh1750_t *dev, const uint8_t cmd){
	i2c_manager_status_t i2c_status = i2c_manager_write(dev->cfg.mgr, dev->cfg.dev_address, &cmd, BH1750_CMD_BYTES_SIZE, I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS);
	return _bh1750_i2c_mgr_status_translate(i2c_status);
}

static bh1750_status_t _bh1750_read_measurement(bh1750_t *dev){
	uint8_t measurement[BH1750_MEASUREMENT_SIZE_BYTES] = {0};
	i2c_manager_status_t i2c_status = i2c_manager_read(dev->cfg.mgr, dev->cfg.dev_address, measurement, BH1750_MEASUREMENT_SIZE_BYTES, I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS);
	if(i2c_status != I2C_MGR_STATUS_OK){
		return _bh1750_i2c_mgr_status_translate(i2c_status);
	}

	//Save measurement
	dev->data.value =  (((uint16_t) measurement[0]) << 8) | measurement[1];
	return BH1750_STATUS_OK;
}

static bh1750_status_t _bh1750_power_on(bh1750_t *dev){
	uint8_t cmd = BH1750_CMD_POWER_ON;
	return _bh1750_send_cmd(dev, cmd);
}

static bh1750_status_t _bh1750_power_down(bh1750_t *dev){
	uint8_t cmd = BH1750_CMD_POWER_DOWN;
	return _bh1750_send_cmd(dev, cmd);
}




/*Public functions*/
bh1750_status_t bh1750_init(bh1750_t *dev, bh1750_config_t *cfg){
	if(dev == NULL)											return BH1750_STATUS_ERR_NULL_POINTER;
	if(cfg == NULL)											return BH1750_STATUS_ERR_NULL_POINTER;
	if(cfg->mgr == NULL)									return BH1750_STATUS_ERR_NULL_POINTER;
	if(cfg->res_mode >= BH1750_RESOLUTION_MAX_COUNT)		return BH1750_STATUS_ERR_INVALID_RESOLUTION;

	//Set bh1750 struct values
	dev->cfg.mgr 		 		= cfg->mgr;
	dev->cfg.dev_address 		= cfg->dev_address;
	dev->cfg.res_mode 	 		= cfg->res_mode;
	dev->last_status 	 		= BH1750_STATUS_OK;
	dev->data.value		 		= 0;
	dev->measurement_start_tick	= 0;
	dev->measurement_wait_ticks = 0;
	dev->state			 		= BH1750_STATE_IDLE;
	dev->is_initialized	 		= true;

	return BH1750_STATUS_OK;
}

bh1750_status_t bh1750_start_measurement(bh1750_t *dev){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	//Power on BH1750 in order to start a measurement
	bh1750_status_t status = _bh1750_power_on(dev);
	if(status != BH1750_STATUS_OK){
		dev->state = BH1750_STATE_ERROR;
		dev->last_status = status;
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
	if(status != BH1750_STATUS_OK){
		dev->state = BH1750_STATE_ERROR;
		dev->last_status = status;
		return status;
	}

	//Advance state machine state
	dev->measurement_start_tick = xTaskGetTickCount();									//Tick reference to know when measurement is ready
	dev->state = BH1750_STATE_WAITING_MEASUREMENT;
	dev->last_status = BH1750_STATUS_BUSY;												//From now on the BH1750 is busy

	return BH1750_STATUS_OK;
}

bh1750_status_t bh1750_process(bh1750_t *dev){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	bh1750_status_t status;
	switch(dev->state){
	case BH1750_STATE_NOT_INITIALIZED:
		//Do nothing
		break;
	case BH1750_STATE_IDLE:
		//Do nothing, start measurement function advances state
		break;
	case BH1750_STATE_WAITING_MEASUREMENT:
		TickType_t elapsed_time_tick = xTaskGetTickCount();
		if(elapsed_time_tick - dev->measurement_start_tick >= dev->measurement_wait_ticks){
			dev->measurement_start_tick = 0;
			dev->state = BH1750_STATE_READING_MEASUREMENT;
		}
		break;
	case BH1750_STATE_READING_MEASUREMENT:
		status = _bh1750_read_measurement(dev);
		if(status != BH1750_STATUS_OK){
			dev->last_status = status;
			if(status != BH1750_STATUS_BUSY){
				dev->state = BH1750_STATE_ERROR;
			}
		}
		else{
			dev->state = BH1750_STATE_MEASUREMENT_READY;
		}
		break;
	case BH1750_STATE_MEASUREMENT_READY:
		//Do nothing, the public read function advances state
		break;
	case BH1750_STATE_ERROR:

		break;
	default:
		dev->state = BH1750_STATE_IDLE;
		break;
	}

	return BH1750_STATUS_OK;
}

bool bh1750_is_ready(bh1750_t *dev){
	if(dev == NULL)				return false;
	if(!dev->is_initialized)	return false;
	return (dev->state == BH1750_STATE_MEASUREMENT_READY);
}

bh1750_status_t bh1750_get_data(bh1750_t *dev, bh1750_data_t *data){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	data->value = dev->data.value;
	return BH1750_STATUS_OK;
}

bh1750_state_t  bh1750_get_state(bh1750_t *dev){
	if(dev == NULL)				return BH1750_STATE_NOT_INITIALIZED;
	if(!dev->is_initialized) 	return BH1750_STATE_NOT_INITIALIZED;

	return dev->state;
}

bh1750_status_t bh1750_get_last_status(bh1750_t *dev){
	if(dev == NULL)						return BH1750_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return BH1750_STATUS_ERR_NOT_INITIALIZED;

	return dev->last_status;
}
