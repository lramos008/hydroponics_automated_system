#include "ds18b20/ds18b20.h"
/*Defines / Macros*/
//System delay times
#define DS18B20_T_CONVERT_9_BIT_TIME_MS			100
#define DS18B20_T_CONVERT_10_BIT_TIME_MS		200
#define DS18B20_T_CONVERT_11_BIT_TIME_MS		400
#define DS18B20_T_CONVERT_12_BIT_TIME_MS		800

//Reg sizes
#define DS18B20_SCRATCHPAD_SIZE_BYTES			9
#define DS18B20_TEMP_REG_SIZE_BYTES				2
#define DS18B20_TH_REG_SIZE_BYTES				1
#define DS18B20_TL_REG_SIZE_BYTES				1
#define DS18B20_CONFIG_REG_SIZE_BYTES			1
#define DS18B20_WRITE_SCRATCHPAD_SIZE_BYTES		DS18B20_TH_REG_SIZE_BYTES + DS18B20_TL_REG_SIZE_BYTES + DS18B20_CONFIG_REG_SIZE_BYTES
#define DS18B20_RESOLUTION_BITS_POS				5

//System parameters
#define DS18B20_FAMILY_CODE						0x28

//Commands
#define DS18B20_CMD_CONVERT_TEMP 				0x44
#define DS18B20_CMD_WRITE_SCRATCHPAD 			0x4E
#define DS18B20_CMD_READ_SCRATCHPAD				0xBE
#define DS18B20_CMD_COPY_SCRATCHPAD				0x48
#define DS18B20_CMD_RECALL_E2					0xB8
#define DS18B20_CMD_READ_POWER_SUPPLY			0xB4

/*Private types*/
typedef struct{
	uint8_t temp_lsb;
	uint8_t temp_msb;
	uint8_t th_reg;
	uint8_t tl_reg;
	uint8_t config_reg;
	uint8_t reserved[3];
	uint8_t crc;
}ds18b20_scratchpad_t;

/*Private function prototypes*/
static ds18b20_err_t ds18b20_init_single_drop(ds18b20_t *dev, onewire_t *ow_bus, ds18b20_resolution_t resolution);
static ds18b20_err_t ds18b20_init_multi_drop(ds18b20_t *dev, onewire_t *ow_bus, ds18b20_resolution_t resolution, const uint8_t *rom, size_t len);
static ds18b20_err_t ds18b20_start_temperature_conversion(ds18b20_t *dev);
static ds18b20_err_t ds18b20_is_conversion_ready(ds18b20_t *dev);
static ds18b20_err_t ds18b20_read_raw_temperature(ds18b20_t *dev, int16_t *raw_temp);
static ds18b20_err_t ds18b20_read_scratchpad(ds18b20_t *dev, ds18b20_scratchpad_t *scratchpad);
static ds18b20_err_t ds18b20_set_resolution(ds18b20_t *dev, ds18b20_resolution_t resolution);
static ds18b20_err_t ds18b20_validate_rom_family(const uint8_t *rom, size_t len);

/*Private functions*/
static void set_rom(uint8_t *dev_rom, uint8_t *rom, size_t len){
	for(uint8_t i = 0; i < len; i++){
		dev_rom[i] = rom[i];
	}
}

static onewire_t *get_bus(ds18b20_t *dev){
	return dev->cfg.ow_bus;
}

static ds18b20_err_t onewire_err_translate(onewire_err_t err){
	switch(err){
	case ONEWIRE_OK:					return DS18B20_OK;
	case ONEWIRE_ERR_NULL:				return DS18B20_ERR_NULL;
	case ONEWIRE_ERR_NO_PRESENCE:		return DS18B20_ERR_NO_PRESENCE;
	case ONEWIRE_ERR_BUS:				return DS18B20_ERR_BUS;
	default:							return DS18B20_ERROR;
	}
}

static TickType_t get_conversion_wait_ticks(ds18b20_resolution_t resolution){
	switch(resolution){
	case DS18B20_9_BIT_RESOLUTION:		return pdMS_TO_TICKS(DS18B20_T_CONVERT_9_BIT_TIME_MS);
	case DS18B20_10_BIT_RESOLUTION:		return pdMS_TO_TICKS(DS18B20_T_CONVERT_10_BIT_TIME_MS);
	case DS18B20_11_BIT_RESOLUTION:		return pdMS_TO_TICKS(DS18B20_T_CONVERT_11_BIT_TIME_MS);
	case DS18B20_12_BIT_RESOLUTION:		return pdMS_TO_TICKS(DS18B20_T_CONVERT_12_BIT_TIME_MS);
	default:							return pdMS_TO_TICKS(DS18B20_T_CONVERT_12_BIT_TIME_MS);
	}
}

static float convert_raw_temperature(int16_t raw_temp){
	return raw_temp * DS18B20_12_BITS_RESOLUTION_STEP;
}

static bool is_resolution_valid(ds18b20_resolution_t resolution){
	return resolution == DS18B20_9_BIT_RESOLUTION ||
		   resolution == DS18B20_10_BIT_RESOLUTION ||
		   resolution == DS18B20_11_BIT_RESOLUTION ||
		   resolution == DS18B20_12_BIT_RESOLUTION;
}

static void ds18b20_fsm_reset(ds18b20_t *dev){
	dev->last_status = DS18B20_OK;
	dev->error_cause = DS18B20_OK;
	dev->state = DS18B20_STATE_IDLE;
	dev->data.raw_temperature = 0;
	dev->data.temperature = 0.0f;
	dev->measurement_start_tick = 0;
	dev->measurement_wait_ticks = 0;
	dev->start_requested = false;
	dev->data_consumed = false;
}

static ds18b20_err_t send_command(ds18b20_t *dev, uint8_t command){
	onewire_t *bus = get_bus(dev);
	onewire_err_t err = onewire_reset(bus);
	if(err == ONEWIRE_ERR_NO_PRESENCE){
		return DS18B20_ERR_NO_PRESENCE;
	}
	if(err != ONEWIRE_OK){
		return onewire_err_translate(err);
	}
	err = onewire_match_rom(bus, dev->rom, DS18B20_ROM_SIZE_BYTES);
	if(err != ONEWIRE_OK){
		return onewire_err_translate(err);
	}
	err = onewire_write_byte(bus, command);
	if(err != ONEWIRE_OK){
		return onewire_err_translate(err);
	}
	return DS18B20_OK;
}

static ds18b20_err_t read_response(ds18b20_t *dev, uint8_t *response, size_t len){
	return onewire_err_translate(onewire_read_multiple_bytes(get_bus(dev), response, len));
}

static ds18b20_err_t write_bytes_to_ds18b20(ds18b20_t *dev, const uint8_t *data, size_t len){
	return onewire_err_translate(onewire_write_multiple_bytes(get_bus(dev), data, len));
}

static uint8_t calculate_crc(uint8_t *buffer, size_t len){
	return onewire_crc8(buffer, len);
}


/*Public functions*/
ds18b20_err_t ds18b20_init(ds18b20_t *dev, ds18b20_config_t *cfg){
	if(!dev || !cfg || !cfg->ow_bus || !cfg->ow_bus->htim || !cfg->ow_bus->port){
		return DS18B20_ERR_NULL;
	}
	if(!is_resolution_valid(cfg->resolution)){
		return DS18B20_ERR_INVALID_RESOLUTION;
	}

	dev->is_initialized = false;
	dev->cfg.ow_bus = cfg->ow_bus;
	dev->cfg.resolution = cfg->resolution;
	dev->cfg.rom = cfg->rom;
	dev->cfg.rom_len = cfg->rom_len;

	ds18b20_err_t err;
	if(cfg->rom == NULL){
		err = ds18b20_init_single_drop(dev, cfg->ow_bus, cfg->resolution);
	}
	else{
		err = ds18b20_init_multi_drop(dev, cfg->ow_bus, cfg->resolution, cfg->rom, cfg->rom_len);
	}
	if(err != DS18B20_OK){
		return err;
	}

	err = ds18b20_set_resolution(dev, cfg->resolution);
	if(err != DS18B20_OK){
		dev->is_initialized = false;
		return err;
	}

	ds18b20_fsm_reset(dev);
	dev->is_initialized = true;
	return DS18B20_OK;
}

static ds18b20_err_t ds18b20_init_single_drop(ds18b20_t *dev, onewire_t *ow_bus, ds18b20_resolution_t resolution){
	//Sanity check
	if(!dev || !ow_bus || !ow_bus->htim || !ow_bus->port){
		return DS18B20_ERR_NULL;
	}
	if(!is_resolution_valid(resolution)){
		return DS18B20_ERR_INVALID_RESOLUTION;
	}

	//Read ds18b20 serial
	dev->is_initialized = false;
	uint8_t rom[DS18B20_ROM_SIZE_BYTES] = {0};
	onewire_err_t ow_err = onewire_reset(ow_bus);
	if(ow_err == ONEWIRE_ERR_NO_PRESENCE){
		return DS18B20_ERR_NO_PRESENCE;
	}
	if(ow_err != ONEWIRE_OK){
		return onewire_err_translate(ow_err);
	}
	ow_err = onewire_read_rom(ow_bus, rom, DS18B20_ROM_SIZE_BYTES);
	if(ow_err != ONEWIRE_OK){
		return onewire_err_translate(ow_err);
	}

	//Check if rom is from the DS18B20 family
	if(ds18b20_validate_rom_family(rom, DS18B20_ROM_SIZE_BYTES) != DS18B20_OK){
		return DS18B20_ERR_INVALID_ROM;
	}

	//Init handle
	set_rom(dev->rom, rom, DS18B20_ROM_SIZE_BYTES);
	dev->resolution = resolution;
	return DS18B20_OK;
}

static ds18b20_err_t ds18b20_init_multi_drop(ds18b20_t *dev, onewire_t *ow_bus, ds18b20_resolution_t resolution, const uint8_t *rom, size_t len){
	//Sanity check
	if(!dev || !ow_bus || !ow_bus->htim || !ow_bus->port || !rom){
		return DS18B20_ERR_NULL;
	}
	if(!is_resolution_valid(resolution)){
		return DS18B20_ERR_INVALID_RESOLUTION;
	}

	//Check if rom is from the DS18B20 family
	dev->is_initialized = false;
	if(ds18b20_validate_rom_family(rom, len) != DS18B20_OK){
		return DS18B20_ERR_INVALID_ROM;
	}

	//Init handle
	set_rom(dev->rom, (uint8_t *) rom, len);
	dev->resolution = resolution;
	return DS18B20_OK;
}

ds18b20_err_t ds18b20_start_measurement(ds18b20_t *dev){
	if(!dev){
		return DS18B20_ERR_NULL;
	}
	if(!dev->is_initialized){
		return DS18B20_ERR_NOT_INITIALIZED;
	}
	if(dev->state == DS18B20_STATE_ERROR){
		return DS18B20_ERROR;
	}
	if(dev->state != DS18B20_STATE_IDLE){
		return DS18B20_BUSY;
	}

	dev->start_requested = true;
	return DS18B20_OK;
}

ds18b20_err_t ds18b20_process(ds18b20_t *dev){
	if(!dev){
		return DS18B20_ERR_NULL;
	}
	if(!dev->is_initialized){
		return DS18B20_ERR_NOT_INITIALIZED;
	}

	switch(dev->state){
	case DS18B20_STATE_IDLE:
		if(dev->start_requested){
			dev->state = DS18B20_STATE_STARTING_CONVERSION;
			dev->start_requested = false;
		}
		dev->last_status = DS18B20_OK;
		break;
	case DS18B20_STATE_STARTING_CONVERSION:
		dev->last_status = ds18b20_start_temperature_conversion(dev);
		if(dev->last_status == DS18B20_OK){
			dev->measurement_start_tick = xTaskGetTickCount();
			dev->measurement_wait_ticks = get_conversion_wait_ticks(dev->resolution);
			dev->state = DS18B20_STATE_WAITING_CONVERSION;
		}
		else{
			dev->state = DS18B20_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = DS18B20_ERROR;
		}
		break;
	case DS18B20_STATE_WAITING_CONVERSION:
	{
		TickType_t elapsed_time_tick = xTaskGetTickCount();
		bool was_conversion_time_reached = elapsed_time_tick - dev->measurement_start_tick >= dev->measurement_wait_ticks;
		if(was_conversion_time_reached){
			ds18b20_err_t conversion_status = ds18b20_is_conversion_ready(dev);
			if(conversion_status == DS18B20_OK){
				dev->state = DS18B20_STATE_READING_SCRATCHPAD;
			}
			else if(conversion_status != DS18B20_CONVERSION_NOT_READY){
				dev->state = DS18B20_STATE_ERROR;
				dev->error_cause = conversion_status;
				dev->last_status = DS18B20_ERROR;
				break;
			}
		}
		dev->last_status = DS18B20_BUSY;
	}
		break;
	case DS18B20_STATE_READING_SCRATCHPAD:
		dev->last_status = ds18b20_read_raw_temperature(dev, &dev->data.raw_temperature);
		if(dev->last_status == DS18B20_OK){
			dev->data.temperature = convert_raw_temperature(dev->data.raw_temperature);
			dev->state = DS18B20_STATE_MEASUREMENT_IS_READY;
		}
		else{
			dev->state = DS18B20_STATE_ERROR;
			dev->error_cause = dev->last_status;
			dev->last_status = DS18B20_ERROR;
		}
		break;
	case DS18B20_STATE_MEASUREMENT_IS_READY:
		if(dev->data_consumed){
			ds18b20_fsm_reset(dev);
		}
		else{
			dev->last_status = DS18B20_OK;
		}
		break;
	case DS18B20_STATE_ERROR:
		dev->last_status = DS18B20_ERROR;
		break;
	default:
		dev->state = DS18B20_STATE_ERROR;
		dev->error_cause = DS18B20_ERR_INVALID_STATE;
		dev->last_status = DS18B20_ERROR;
	}

	return dev->last_status;
}

bool ds18b20_is_ready(ds18b20_t *dev){
	if(!dev)					return false;
	if(!dev->is_initialized)	return false;
	return dev->state == DS18B20_STATE_MEASUREMENT_IS_READY;
}

ds18b20_err_t ds18b20_get_data(ds18b20_t *dev, ds18b20_data_t *data){
	if(!dev || !data){
		return DS18B20_ERR_NULL;
	}
	if(!dev->is_initialized){
		return DS18B20_ERR_NOT_INITIALIZED;
	}
	if(dev->state != DS18B20_STATE_MEASUREMENT_IS_READY){
		return DS18B20_BUSY;
	}

	data->raw_temperature = dev->data.raw_temperature;
	data->temperature = dev->data.temperature;
	dev->data_consumed = true;
	return DS18B20_OK;
}

ds18b20_err_t ds18b20_reset(ds18b20_t *dev){
	if(!dev || !dev->cfg.ow_bus){
		return DS18B20_ERR_NULL;
	}
	if(!dev->is_initialized){
		return DS18B20_ERR_NOT_INITIALIZED;
	}

	onewire_err_t err = onewire_reset(get_bus(dev));
	if(err != ONEWIRE_OK){
		return onewire_err_translate(err);
	}

	ds18b20_fsm_reset(dev);
	return DS18B20_OK;
}

static ds18b20_err_t ds18b20_start_temperature_conversion(ds18b20_t *dev){
	//Sanity check
	if(!dev || !dev->cfg.ow_bus){
		return DS18B20_ERR_NULL;
	}

	//Send T conversion command
	ds18b20_err_t err = send_command(dev, DS18B20_CMD_CONVERT_TEMP);
	return err;
}

static ds18b20_err_t ds18b20_is_conversion_ready(ds18b20_t *dev){
	//Sanity check
	if(!dev || !dev->cfg.ow_bus){
		return DS18B20_ERR_NULL;
	}

	uint8_t state;
	onewire_err_t err = onewire_read_bit(get_bus(dev), &state);
	if(err != ONEWIRE_OK){
		return onewire_err_translate(err);
	}
	return (state == 1) ? DS18B20_OK : DS18B20_CONVERSION_NOT_READY;
}

static ds18b20_err_t ds18b20_read_raw_temperature(ds18b20_t *dev, int16_t *raw_temp){
	//Sanity check
	if(!dev || !dev->cfg.ow_bus || !raw_temp){
		return DS18B20_ERR_NULL;
	}

	//Read scratchpad
	ds18b20_scratchpad_t scratchpad;
	ds18b20_err_t err = ds18b20_read_scratchpad(dev, &scratchpad);
	if(err != DS18B20_OK){
		return err;
	}

	//Extract temperature from scratchpad
	*raw_temp = ((int16_t)scratchpad.temp_msb << 8) | scratchpad.temp_lsb;
	return DS18B20_OK;
}

static ds18b20_err_t ds18b20_read_scratchpad(ds18b20_t *dev, ds18b20_scratchpad_t *scratchpad){
	//Sanity check
	if(!dev || !dev->cfg.ow_bus || !scratchpad){
		return DS18B20_ERR_NULL;
	}

	//Read scratchpad
	uint8_t buffer[DS18B20_SCRATCHPAD_SIZE_BYTES] = {0};
	ds18b20_err_t err = send_command(dev, DS18B20_CMD_READ_SCRATCHPAD);
	if(err != DS18B20_OK){
		return err;
	}

	err = read_response(dev, buffer, DS18B20_SCRATCHPAD_SIZE_BYTES);
	if(err != DS18B20_OK){
		return err;
	}

	//Calculate CRC
	uint8_t calculated_crc = calculate_crc(buffer, DS18B20_SCRATCHPAD_SIZE_BYTES - 1);				//It does not include the CRC
	if(calculated_crc != buffer[8]){
		return DS18B20_ERR_CRC;
	}

	//Unpack scratchpad
	scratchpad->temp_lsb 	 = buffer[0];
	scratchpad->temp_msb 	 = buffer[1];
	scratchpad->th_reg       = buffer[2];
	scratchpad->tl_reg       = buffer[3];
	scratchpad->config_reg   = buffer[4];
	scratchpad->reserved[0]  = buffer[5];
	scratchpad->reserved[1]  = buffer[6];
	scratchpad->reserved[2]  = buffer[7];
	scratchpad->crc          = buffer[8];
	return DS18B20_OK;
}

static ds18b20_err_t ds18b20_set_resolution(ds18b20_t *dev, ds18b20_resolution_t resolution){
	//Sanity check
	if(!dev || !dev->cfg.ow_bus){
		return DS18B20_ERR_NULL;
	}

	ds18b20_scratchpad_t scratchpad;
	ds18b20_err_t err = ds18b20_read_scratchpad(dev, &scratchpad);
	if(err != DS18B20_OK){
		return err;
	}

	//Modify config reg
	scratchpad.config_reg &= ~(0x03 << DS18B20_RESOLUTION_BITS_POS);															//Clean resolution bits
	if(resolution == DS18B20_9_BIT_RESOLUTION) 	 	 scratchpad.config_reg |= (0x00 << DS18B20_RESOLUTION_BITS_POS);
	else if(resolution == DS18B20_10_BIT_RESOLUTION) scratchpad.config_reg |= (0x01 << DS18B20_RESOLUTION_BITS_POS);
	else if(resolution == DS18B20_11_BIT_RESOLUTION) scratchpad.config_reg |= (0x02 << DS18B20_RESOLUTION_BITS_POS);
	else if(resolution == DS18B20_12_BIT_RESOLUTION) scratchpad.config_reg |= (0x03 << DS18B20_RESOLUTION_BITS_POS);
	else 											 scratchpad.config_reg |= (0x03 << DS18B20_RESOLUTION_BITS_POS);			//Default 12 bits resolution

	//Serialize data
	uint8_t write_buffer[DS18B20_WRITE_SCRATCHPAD_SIZE_BYTES];
	write_buffer[0] = scratchpad.th_reg;
	write_buffer[1] = scratchpad.tl_reg;
	write_buffer[2] = scratchpad.config_reg;

	err = send_command(dev, DS18B20_CMD_WRITE_SCRATCHPAD);
	if(err != DS18B20_OK){
		return err;
	}

	err = write_bytes_to_ds18b20(dev, write_buffer, DS18B20_WRITE_SCRATCHPAD_SIZE_BYTES);
	if(err != DS18B20_OK){
		return err;
	}
	return DS18B20_OK;
}

static ds18b20_err_t ds18b20_validate_rom_family(const uint8_t *rom, size_t len){
	//Sanity check
	if(!rom) return DS18B20_ERR_NULL;

	//Check rom length
	if(len != DS18B20_ROM_SIZE_BYTES) return DS18B20_ERR_INVALID_ROM;

	//Check if rom belongs to the DS18B20 family
	if(rom[0] != DS18B20_FAMILY_CODE) return DS18B20_ERR_INVALID_ROM;
	return DS18B20_OK;
}
