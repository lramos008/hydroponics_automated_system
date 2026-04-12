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

/*Private functions*/
static void set_rom(uint8_t *dev_rom, uint8_t *rom, size_t len){
	for(uint8_t i = 0; i < len; i++){
		dev_rom[i] = rom[i];
	}
}

static ds18b20_err_t send_command(ds18b20_t *dev, uint8_t command){
	onewire_err_t err = onewire_reset(dev->ow_bus);
	if(err == ONEWIRE_ERR_NO_PRESENCE){
		return DS18B20_ERR_NO_PRESENCE;
	}
	onewire_match_rom(dev->ow_bus, dev->rom, DS18B20_ROM_SIZE_BYTES);
	onewire_write_byte(dev->ow_bus, command);
	return DS18B20_OK;
}

static ds18b20_err_t read_response(ds18b20_t *dev, uint8_t *response, size_t len){
	return onewire_read_multiple_bytes(dev->ow_bus, response, len);
}

static ds18b20_err_t write_bytes_to_ds18b20(ds18b20_t *dev, const uint8_t *data, size_t len){
	return onewire_write_multiple_bytes(dev->ow_bus, data, len);
}

static uint8_t calculate_crc(uint8_t *buffer, size_t len){
	return onewire_crc8(buffer, len);
}


/*Public functions*/
ds18b20_err_t ds18b20_init_single_drop(ds18b20_t *dev, onewire_t *ow_bus, ds18b20_resolution_t resolution){
	//Sanity check
	if(!dev || !ow_bus || !ow_bus->htim || !ow_bus->port){
		return DS18B20_ERR_NULL;
	}

	//Read ds18b20 serial
	uint8_t rom[DS18B20_ROM_SIZE_BYTES] = {0};
	onewire_err_t ow_err = onewire_reset(ow_bus);
	if(ow_err == ONEWIRE_ERR_NO_PRESENCE){
		return DS18B20_ERR_NO_PRESENCE;
	}
	onewire_read_rom(ow_bus, rom, DS18B20_ROM_SIZE_BYTES);

	//Check if rom is from the DS18B20 family
	if(ds18b20_validate_rom_family(rom, DS18B20_ROM_SIZE_BYTES) != DS18B20_OK){
		return DS18B20_ERR_INVALID_ROM;
	}

	//Init handle
	dev->ow_bus = ow_bus;
	set_rom(dev->rom, rom, DS18B20_ROM_SIZE_BYTES);
	dev->resolution = resolution;
	return DS18B20_OK;
}

ds18b20_err_t ds18b20_init_multi_drop(ds18b20_t *dev, onewire_t *ow_bus, ds18b20_resolution_t resolution, const uint8_t *rom, size_t len){
	//Sanity check
	if(!dev || !ow_bus || !ow_bus->htim || !ow_bus->port || !rom){
		return DS18B20_ERR_NULL;
	}

	//Check if rom is from the DS18B20 family
	if(ds18b20_validate_rom_family(rom, len) != DS18B20_OK){
		return DS18B20_ERR_INVALID_ROM;
	}

	//Init handle
	dev->ow_bus = ow_bus;
	set_rom(dev->rom, (uint8_t *) rom, len);
	dev->resolution = resolution;
	return DS18B20_OK;
}

ds18b20_err_t ds18b20_start_temperature_conversion(ds18b20_t *dev){
	//Sanity check
	if(!dev || !dev->ow_bus){
		return DS18B20_ERR_NULL;
	}

	//Send T conversion command
	ds18b20_err_t err = send_command(dev, DS18B20_CMD_CONVERT_TEMP);
	return err;
}

ds18b20_err_t ds18b20_is_conversion_ready(ds18b20_t *dev){
	//Sanity check
	if(!dev || !dev->ow_bus){
		return DS18B20_ERR_NULL;
	}

	uint8_t state;
	onewire_read_bit(dev->ow_bus, &state);
	return (state == 0) ? DS18B20_OK : DS18B20_CONVERSION_NOT_READY;
}

ds18b20_err_t ds18b20_read_raw_temperature(ds18b20_t *dev, int16_t *raw_temp){
	//Sanity check
	if(!dev || !dev->ow_bus || !raw_temp){
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

ds18b20_err_t ds18b20_read_scratchpad(ds18b20_t *dev, ds18b20_scratchpad_t *scratchpad){
	//Sanity check
	if(!dev || !dev->ow_bus || !scratchpad){
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

ds18b20_err_t ds18b20_set_resolution(ds18b20_t *dev, ds18b20_resolution_t resolution){
	//Sanity check
	if(!dev || !dev->ow_bus){
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

ds18b20_err_t ds18b20_get_resolution(ds18b20_t *dev, ds18b20_resolution_t *resolution){
	//Sanity check
	if(!dev || !dev->ow_bus || !resolution){
		return DS18B20_ERR_NULL;
	}

	ds18b20_scratchpad_t scratchpad;
	ds18b20_err_t err = ds18b20_read_scratchpad(dev, &scratchpad);
	if(err != DS18B20_OK){
		return err;
	}

	uint8_t resolution_bits = (scratchpad.config_reg & (0x3 << DS18B20_RESOLUTION_BITS_POS)) >> DS18B20_RESOLUTION_BITS_POS;
	if(resolution_bits == 0x00) 	 *resolution = DS18B20_9_BIT_RESOLUTION;
	else if(resolution_bits == 0x01) *resolution = DS18B20_10_BIT_RESOLUTION;
	else if(resolution_bits == 0x02) *resolution = DS18B20_11_BIT_RESOLUTION;
	else if(resolution_bits == 0x03) *resolution = DS18B20_12_BIT_RESOLUTION;
	else 							 *resolution = DS18B20_UNKNOWN_RESOLUTION;
	return DS18B20_OK;
}

ds18b20_err_t ds18b20_validate_rom_family(const uint8_t *rom, size_t len){
	//Sanity check
	if(!rom) return DS18B20_ERR_NULL;

	//Check rom length
	if(len != DS18B20_ROM_SIZE_BYTES) return DS18B20_ERR_INVALID_ROM;

	//Check if rom belongs to the DS18B20 family
	if(rom[0] != DS18B20_FAMILY_CODE) return DS18B20_ERR_INVALID_ROM;
	return DS18B20_OK;
}
