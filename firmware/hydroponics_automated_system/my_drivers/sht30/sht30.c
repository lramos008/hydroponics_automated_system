#include <sht30/sht30.h>

/*Defines / Macros*/
#define SHT30_CLK_STRETCH_OPTIONS 				SHT30_CLK_STRETCHING_MAX_COUNT
#define SHT30_REP_OPTIONS						SHT30_REPEATABILITY_MAX_COUNT
#define SHT30_REG_SIZE_BYTES					2

//System parameters
#define SHT30_POWER_UP_TIME_MS					1
#define SHT30_SOFT_RESET_TIME_MS				2
#define SHT30_CMD_WAITING_TIME_MS				1
#define SHT30_HI_REP_MEASUREMENT_DURATION_MS	16
#define SHT30_MED_REP_MEASUREMENT_DURATION_MS	7
#define SHT30_LOW_REP_MEASUREMENT_DURATION_MS	5
#define SHT30_TEMP_MEASUREMENT_BYTES			2
#define SHT30_HR_MEASUREMENT_BYTES				2
#define SHT30_CRC_BYTES							2
#define SHT30_MEASUREMENT_SIZE_BYTES			(SHT30_TEMP_MEASUREMENT_BYTES + SHT30_HR_MEASUREMENT_BYTES + SHT30_CRC_BYTES)

//Commands
#define SHT30_CMD_READ_STATUS_REG				0xF32D
#define SHT30_CMD_CLEAR_STATUS_REG				0x3041
#define SHT30_CMD_SOFT_RESET					0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET			0x0006

//Addresses
#define SHT30_GENERAL_CALL_ADDR					0x0000

/*Private global variables*/
static const uint16_t sht30_measure_cmd_table[SHT30_CLK_STRETCH_OPTIONS][SHT30_REP_OPTIONS] =	{
																									//Clock stretching enabled
																									{
																											0x2C06,					//High repeatability
																											0x2C0D,					//Medium repeatability
																											0x2C10					//Low repeatability
																									},
																									//Clock stretching disabled
																									{
																											0x2400,					//High repeatability
																											0x240B,					//Medium repeatability
																											0x2416					//Low repeatability
																									}
																								};

/*Private functions*/
static sht30_status_t _sht30_i2c_mgr_status_translate(i2c_manager_status_t i2c_mgr_status){
	switch(i2c_mgr_status){
	case I2C_MGR_STATUS_OK:						return SHT30_STATUS_OK;
	case I2C_MGR_STATUS_ERR_NULL_POINTER:		return SHT30_STATUS_ERR_NULL_POINTER;
	case I2C_MGR_STATUS_BUSY:					return SHT30_STATUS_BUSY;
	case I2C_MGR_STATUS_ERR_TIMEOUT:			return SHT30_STATUS_ERR_TIMEOUT;
	case I2C_MGR_STATUS_ERR_MUTEX:				return SHT30_STATUS_ERR_I2C_MGR;
	default:									return SHT30_STATUS_ERROR;
	}
}

static float _sht30_convert_raw_to_temperature(uint16_t temp_raw){
	return -45.0f + 175.0f * ((float) temp_raw / 65535.0f);
}

static float _sht30_convert_raw_to_hr(uint16_t hr_raw){
	return 100.0f * ((float) hr_raw / 65535.0f);
}

static uint8_t _sht30_crc8_calculation(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;

    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static void _sht30_u16_to_buffer(uint16_t value, uint8_t *buffer){
	buffer[0] = (value >> 8) & 0xFF;
	buffer[1] = value & 0xFF;
}

static TickType_t _sht30_get_measurement_wait_ticks(sht30_repeatability_t repeatability){
	switch(repeatability){
	case SHT30_HIGH_REPEATABILITY:		return pdMS_TO_TICKS(SHT30_HI_REP_MEASUREMENT_DURATION_MS);
	case SHT30_MEDIUM_REPEATABILITY:		return pdMS_TO_TICKS(SHT30_MED_REP_MEASUREMENT_DURATION_MS);
	case SHT30_LOW_REPEATABILITY:		return pdMS_TO_TICKS(SHT30_LOW_REP_MEASUREMENT_DURATION_MS);
	default:							return pdMS_TO_TICKS(SHT30_HI_REP_MEASUREMENT_DURATION_MS);
	}
}

static sht30_status_t _sht30_send_cmd(sht30_t *dev, uint16_t cmd){
	if(dev == NULL) 			return SHT30_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)	return SHT30_STATUS_ERR_NOT_INITIALIZED;

	uint8_t cmd_buf[SHT30_REG_SIZE_BYTES];
	_sht30_u16_to_buffer(cmd, cmd_buf);

	i2c_manager_status_t i2c_status = i2c_manager_write(dev->cfg.mgr, dev->cfg.dev_address, cmd_buf, SHT30_REG_SIZE_BYTES, I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS);
	return _sht30_i2c_mgr_status_translate(i2c_status);
}

static sht30_status_t _sht30_request_measurement(sht30_t *dev){
	uint16_t cmd = sht30_measure_cmd_table[dev->cfg.clk_stretching][dev->cfg.repeatability];
	dev->measurement_wait_ticks = _sht30_get_measurement_wait_ticks(dev->cfg.repeatability);
	return _sht30_send_cmd(dev, cmd);
}

static sht30_status_t _sht30_read_measurement(sht30_t *dev){
	if(dev == NULL) 			return SHT30_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)	return SHT30_STATUS_ERR_NOT_INITIALIZED;

	if(dev->state != SHT30_STATE_READING_MEASUREMENT){
		return SHT30_STATUS_BUSY;
	}

	uint8_t buffer[SHT30_MEASUREMENT_SIZE_BYTES] = {0};
	i2c_manager_status_t i2c_status = i2c_manager_read(dev->cfg.mgr, dev->cfg.dev_address, buffer, SHT30_MEASUREMENT_SIZE_BYTES, I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS);
	if(i2c_status != I2C_MGR_STATUS_OK){
		return _sht30_i2c_mgr_status_translate(i2c_status);
	}

	uint8_t temp_crc = _sht30_crc8_calculation(buffer, SHT30_TEMP_MEASUREMENT_BYTES);
	if(temp_crc != buffer[2]){
		return SHT30_STATUS_ERR_CRC;
	}

	uint8_t hr_crc = _sht30_crc8_calculation(&buffer[3], SHT30_HR_MEASUREMENT_BYTES);
	if(hr_crc != buffer[5]){
		return SHT30_STATUS_ERR_CRC;
	}

	dev->data.temp_raw = ((uint16_t)buffer[0] << 8) | buffer[1];
	dev->data.hr_raw = ((uint16_t)buffer[3] << 8) | buffer[4];
	dev->data.temperature = _sht30_convert_raw_to_temperature(dev->data.temp_raw);
	dev->data.humidity = _sht30_convert_raw_to_hr(dev->data.hr_raw);

	return SHT30_STATUS_OK;
}

static void _sht30_fsm_reset(sht30_t *dev){
	dev->last_status 	 		 = SHT30_STATUS_OK;
	dev->error_cause 			 = SHT30_STATUS_OK;
	dev->state					 = SHT30_STATE_IDLE;
	dev->data.temp_raw	 		 = 0;
	dev->data.hr_raw	 		 = 0;
	dev->data.temperature 		 = 0.0f;
	dev->data.humidity	 		 = 0.0f;
	dev->power_up_start_tick	 = 0;
	dev->measurement_start_tick	 = 0;
	dev->measurement_wait_ticks  = 0;
	dev->reset_start_tick		 = 0;
	dev->start_requested		 = false;
	dev->reset_requested		 = false;
	dev->data_consumed			 = false;
}

/*Public API*/
sht30_status_t sht30_init(sht30_t *dev, sht30_config_t *cfg){
	if(dev == NULL)											return SHT30_STATUS_ERR_NULL_POINTER;
	if(cfg == NULL)											return SHT30_STATUS_ERR_NULL_POINTER;
	if(cfg->mgr == NULL)									return SHT30_STATUS_ERR_NULL_POINTER;
	if(cfg->repeatability >= SHT30_REPEATABILITY_MAX_COUNT)	return SHT30_STATUS_ERR_INVALID_REPEATABILITY;
	if(cfg->clk_stretching >= SHT30_CLK_STRETCHING_MAX_COUNT)	return SHT30_STATUS_ERR_INVALID_CLK_STRETCHING;

	dev->is_initialized	 		 = false;
	dev->cfg.mgr 		 		 = cfg->mgr;
	dev->cfg.dev_address 		 = cfg->dev_address;
	dev->cfg.repeatability 		 = cfg->repeatability;
	dev->cfg.clk_stretching 	 = cfg->clk_stretching;

	_sht30_fsm_reset(dev);
	dev->state				 = SHT30_STATE_POWER_UP_WAIT;
	dev->power_up_start_tick = xTaskGetTickCount();
	dev->is_initialized	 	 = true;
	return SHT30_STATUS_OK;
}

sht30_status_t sht30_start_measurement(sht30_t *dev){
	if(dev == NULL)						return SHT30_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return SHT30_STATUS_ERR_NOT_INITIALIZED;

	if(dev->state == SHT30_STATE_ERROR){
		return SHT30_STATUS_ERROR;
	}

	if(dev->state != SHT30_STATE_IDLE){
		return SHT30_STATUS_BUSY;
	}

	dev->start_requested = true;
	return SHT30_STATUS_OK;
}

sht30_status_t sht30_process(sht30_t *dev){
	if(dev == NULL)						return SHT30_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return SHT30_STATUS_ERR_NOT_INITIALIZED;

	if(dev->reset_requested && dev->state != SHT30_STATE_RESETTING && dev->state != SHT30_STATE_WAITING_RESET){
		dev->state = SHT30_STATE_RESETTING;
		dev->reset_requested = false;
	}

	switch(dev->state){
	case SHT30_STATE_POWER_UP_WAIT:
	{
		TickType_t elapsed_time_tick = xTaskGetTickCount();
		bool was_power_up_time_reached = elapsed_time_tick - dev->power_up_start_tick >= pdMS_TO_TICKS(SHT30_POWER_UP_TIME_MS);
		if(was_power_up_time_reached){
			dev->state = SHT30_STATE_IDLE;
			dev->last_status = SHT30_STATUS_OK;
		}
		else{
			dev->last_status = SHT30_STATUS_BUSY;
		}
	}
		break;
	case SHT30_STATE_IDLE:
		if(dev->start_requested){
			dev->state = SHT30_STATE_STARTING_MEASUREMENT;
			dev->start_requested = false;
		}
		dev->last_status = SHT30_STATUS_OK;
		break;
	case SHT30_STATE_STARTING_MEASUREMENT:
		dev->last_status = _sht30_request_measurement(dev);
		if(dev->last_status == SHT30_STATUS_OK){
			dev->state = SHT30_STATE_WAITING_MEASUREMENT;
			dev->measurement_start_tick = xTaskGetTickCount();
		}
		else if(dev->last_status == SHT30_STATUS_BUSY){
			//Do nothing, wait for the next cycle and try again
		}
		else{
			dev->state 		 = SHT30_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = SHT30_STATUS_ERROR;
		}
		break;
	case SHT30_STATE_WAITING_MEASUREMENT:
	{
		TickType_t elapsed_time_tick = xTaskGetTickCount();
		bool was_measurement_read_time_reached = elapsed_time_tick - dev->measurement_start_tick >= dev->measurement_wait_ticks;
		if(was_measurement_read_time_reached){
			dev->state = SHT30_STATE_READING_MEASUREMENT;
		}
		dev->last_status = SHT30_STATUS_BUSY;
	}
		break;
	case SHT30_STATE_READING_MEASUREMENT:
		dev->last_status = _sht30_read_measurement(dev);
		if(dev->last_status == SHT30_STATUS_OK){
			dev->state = SHT30_STATE_MEASUREMENT_IS_READY;
		}
		else if(dev->last_status == SHT30_STATUS_BUSY){
			//Wait for I2C bus to be freed
		}
		else{
			dev->state 		 = SHT30_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = SHT30_STATUS_ERROR;
		}
		break;
	case SHT30_STATE_MEASUREMENT_IS_READY:
		if(dev->data_consumed){
			_sht30_fsm_reset(dev);
		}
		else{
			dev->last_status = SHT30_STATUS_OK;
		}
		break;
	case SHT30_STATE_RESETTING:
		dev->last_status = _sht30_send_cmd(dev, SHT30_CMD_SOFT_RESET);
		if(dev->last_status == SHT30_STATUS_OK){
			dev->state = SHT30_STATE_WAITING_RESET;
			dev->reset_start_tick = xTaskGetTickCount();
		}
		else if(dev->last_status == SHT30_STATUS_BUSY){
			//Do nothing, wait for the next cycle
		}
		else{
			dev->state 		 = SHT30_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = SHT30_STATUS_ERROR;
		}
		break;
	case SHT30_STATE_WAITING_RESET:
	{
		TickType_t elapsed_time_tick = xTaskGetTickCount();
		bool was_reset_time_reached = elapsed_time_tick - dev->reset_start_tick >= pdMS_TO_TICKS(SHT30_SOFT_RESET_TIME_MS + SHT30_CMD_WAITING_TIME_MS);
		if(was_reset_time_reached){
			_sht30_fsm_reset(dev);
			dev->last_status = SHT30_STATUS_OK;
		}
		else{
			dev->last_status = SHT30_STATUS_BUSY;
		}
	}
		break;
	case SHT30_STATE_ERROR:
		dev->last_status = SHT30_STATUS_ERROR;
		break;
	default:
		dev->state		 = SHT30_STATE_ERROR;
		dev->error_cause = SHT30_STATUS_ERR_INVALID_STATE;
		dev->last_status = SHT30_STATUS_ERROR;
	}

	return dev->last_status;
}

bool sht30_is_ready(sht30_t *dev){
	if(dev == NULL)				return false;
	if(!dev->is_initialized)	return false;
	return (dev->state == SHT30_STATE_MEASUREMENT_IS_READY);
}

sht30_status_t sht30_get_data(sht30_t *dev, sht30_data_t *data){
	if(dev == NULL)						return SHT30_STATUS_ERR_NULL_POINTER;
	if(data == NULL)					return SHT30_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized)			return SHT30_STATUS_ERR_NOT_INITIALIZED;

	if(dev->state != SHT30_STATE_MEASUREMENT_IS_READY){
		return SHT30_STATUS_BUSY;
	}

	data->temp_raw 		= dev->data.temp_raw;
	data->hr_raw 		= dev->data.hr_raw;
	data->temperature 	= dev->data.temperature;
	data->humidity 		= dev->data.humidity;
	dev->data_consumed  = true;
	return SHT30_STATUS_OK;
}

sht30_status_t sht30_reset(sht30_t *dev){
	if(dev == NULL) 			return SHT30_STATUS_ERR_NULL_POINTER;
	if(!dev->is_initialized) 	return SHT30_STATUS_ERR_NOT_INITIALIZED;

	dev->reset_requested = true;
	return SHT30_STATUS_OK;
}
