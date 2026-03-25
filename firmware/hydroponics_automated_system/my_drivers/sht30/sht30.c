#include <sht30/sht30.h>

/*Defines / Macros*/
#define SHT30_CLK_STRETCH_OPTIONS 				2
#define SHT30_REP_OPTIONS						3
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
static uint8_t sht30_crc8_calculation(const uint8_t *data, uint8_t len)
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

    return crc; // no final XOR
}

/*Public API*/
sht30_err_t sht30_init(sht30_t *dev, I2C_HandleTypeDef *hi2c, uint8_t dev_address){
	//Sanity check
	if(!dev || !hi2c){
		return SHT30_ERR_NULL;
	}

	//Init handle
	dev->hi2c = hi2c;
	dev->dev_address = dev_address;

	//Wait power up time
	HAL_Delay(SHT30_POWER_UP_TIME_MS);
	return SHT30_OK;
}


sht30_err_t sht30_start_measurement(sht30_t *dev, sht30_repeatability_t rep, sht30_clk_stretching_t clk_stretch){
	//Sanity check
	if(!dev || !dev->hi2c){
		return SHT30_ERR_NULL;
	}

	uint16_t cmd = sht30_measure_cmd_table[clk_stretch][rep];
	uint8_t cmd_buf[2];

	cmd_buf[0] = (cmd >> 8) & 0xFF; // MSB
	cmd_buf[1] = cmd & 0xFF;        // LSB

	//Send measurement command
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(dev->hi2c, dev->dev_address, (uint8_t *) cmd_buf, SHT30_REG_SIZE_BYTES, 100);
	if(status != HAL_OK){
		if(status == HAL_TIMEOUT) return SHT30_ERR_TIMEOUT;
		else return SHT30_ERR_BUS;
	}

	//Wait for measurement to be ready
	uint32_t waiting_time;
	if(rep == SHT30_HIGH_REPEATABILITY)		   waiting_time = SHT30_HI_REP_MEASUREMENT_DURATION_MS;
	else if(rep == SHT30_MEDIUM_REPEATABILITY) waiting_time = SHT30_MED_REP_MEASUREMENT_DURATION_MS;
	else 									   waiting_time = SHT30_LOW_REP_MEASUREMENT_DURATION_MS;
	HAL_Delay(waiting_time);
	return SHT30_OK;
}


sht30_err_t sht30_get_raw_measurement(sht30_t *dev, uint16_t *temp_raw, uint16_t *hr_raw){
	//Sanity check
	if(!dev || !dev->hi2c || !temp_raw || !hr_raw){
		return SHT30_ERR_NULL;
	}

	//Read measurement
	uint8_t buffer[SHT30_TEMP_MEASUREMENT_BYTES + SHT30_HR_MEASUREMENT_BYTES + SHT30_CRC_BYTES] = {0};
	HAL_StatusTypeDef status = HAL_I2C_Master_Receive(dev->hi2c, dev->dev_address, buffer, SHT30_TEMP_MEASUREMENT_BYTES + SHT30_HR_MEASUREMENT_BYTES + SHT30_CRC_BYTES, 100);
	if(status != HAL_OK){
		if(status == HAL_TIMEOUT) return SHT30_ERR_TIMEOUT;
		else return SHT30_ERR_BUS;
	}

	//Check temp CRC
	uint8_t temp_crc;
	temp_crc = sht30_crc8_calculation(buffer, SHT30_TEMP_MEASUREMENT_BYTES);
	if(temp_crc != buffer[2]){
		return SHT30_ERR_CRC;
	}

	//Save temp measurement
	*temp_raw = ((uint16_t)buffer[0] << 8) | buffer[1];

	//Check HR CRC
	uint8_t hr_crc;
	hr_crc = sht30_crc8_calculation(&buffer[3], SHT30_HR_MEASUREMENT_BYTES);
	if(hr_crc != buffer[5]){
		return SHT30_ERR_CRC;
	}

	//Save HR measurement
	*hr_raw = ((uint16_t)buffer[3] << 8) | buffer[4];

	//Wait before sending another command
	HAL_Delay(SHT30_CMD_WAITING_TIME_MS);
	return SHT30_OK;
}

float sht30_convert_raw_to_temperature(uint16_t temp_raw){
	return -45.0f + 175.0f * ((float) temp_raw / 65535.0f);
}

float sht30_convert_raw_to_hr(uint16_t hr_raw){
	return 100.0f * ((float) hr_raw / 65535.0f);
}

