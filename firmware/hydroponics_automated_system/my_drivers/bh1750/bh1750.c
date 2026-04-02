#include "bh1750/bh1750.h"

/*Defines / Macros*/
//System parameters
#define BH1750_CMD_BYTES_SIZE				1
#define BH1750_MEASUREMENT_BYTES_SIZE		2
#define BH1750_LOW_RES_MEASUREMENT_TIME_MS 	24
#define BH1750_HI_RES_MEASUREMENT_TIME_MS 	180
#define BH1750_HI_RES_2_MEASUREMENT_TIME_MS 180
#define BH1750_LUX_CONVERSION_FACTOR		1.2f

//Commands
#define BH1750_CMD_POWER_ON   				0x00
#define BH1750_CMD_POWER_DOWN 				0x01
#define BH1750_CMD_RESET	  				0x07
#define BH1750_CMD_CONT_HI_RES_MODE 		0x10
#define BH1750_CMD_CONT_HI_RES_MODE_2 		0x11
#define BH1750_CMD_CONT_LOW_RES_MODE		0x13
#define BH1750_CMD_ONESHOT_HI_RES_MODE  	0x20
#define BH1750_CMD_ONESHOT_HI_RES_MODE_2  	0x21
#define BH1750_CMD_ONESHOT_LOW_RES_MODE		0x23


/*Private functions*/
static bh1750_err_t bh1750_send_cmd(bh1750_t *dev, uint8_t *cmd){
	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(dev->hi2c, dev->dev_address, cmd, BH1750_CMD_BYTES_SIZE, 100);
	if(status != HAL_OK){
		if(status == HAL_TIMEOUT) return BH1750_ERR_TIMEOUT;
		else return BH1750_ERR_BUS;
	}
	return BH1750_OK;
}

/*Public API*/
bh1750_err_t bh1750_init(bh1750_t *dev, I2C_HandleTypeDef *hi2c, uint8_t dev_address){
	//Sanity check
	if(!dev || !hi2c){
		return BH1750_ERR_NULL;
	}

	//Init handle
	dev->hi2c = hi2c;
	dev->dev_address = dev_address;
	return BH1750_OK;
}

bh1750_err_t bh1750_power_on(bh1750_t *dev){
	//Sanity check
	if(!dev || !dev->hi2c){
		return BH1750_ERR_NULL;
	}

	//Send cmd
	uint8_t cmd = BH1750_CMD_POWER_ON;
	return bh1750_send_cmd(dev, &cmd);
}

bh1750_err_t bh1750_power_down(bh1750_t *dev){
	//Sanity check
	if(!dev || !dev->hi2c){
		return BH1750_ERR_NULL;
	}

	//Send cmd
	uint8_t cmd = BH1750_CMD_POWER_DOWN;
	return bh1750_send_cmd(dev, &cmd);
}

bh1750_err_t bh1750_reset(bh1750_t *dev){
	//Sanity check
	if(!dev || !dev->hi2c){
		return BH1750_ERR_NULL;
	}

	//Send cmd
	uint8_t cmd = BH1750_CMD_RESET;
	return bh1750_send_cmd(dev, &cmd);
}


bh1750_err_t bh1750_start_measurement(bh1750_t *dev, bh1750_res_mode_t res_mode){
	//Sanity check
	if(!dev || !dev->hi2c){
		return BH1750_ERR_NULL;
	}

	//Send cmd
	uint8_t cmd;
	uint32_t wait_ms;
	switch(res_mode){
	case BH1750_HI_RES_MODE:
		cmd = BH1750_CMD_ONESHOT_HI_RES_MODE;
		wait_ms = BH1750_HI_RES_MEASUREMENT_TIME_MS;
		break;

	case BH1750_HI_RES_MODE_2:
		cmd = BH1750_CMD_ONESHOT_HI_RES_MODE_2;
		wait_ms = BH1750_HI_RES_2_MEASUREMENT_TIME_MS;
		break;

	case BH1750_LOW_RES_MODE:
		cmd = BH1750_CMD_ONESHOT_LOW_RES_MODE;
		wait_ms = BH1750_LOW_RES_MEASUREMENT_TIME_MS;
		break;

	default:
		cmd = BH1750_CMD_ONESHOT_HI_RES_MODE;
		wait_ms = BH1750_HI_RES_MEASUREMENT_TIME_MS;
		break;
	}

	bh1750_err_t err = bh1750_send_cmd(dev, &cmd);
	if(err == BH1750_OK) HAL_Delay(wait_ms);					//Wait for measurement to finish
	return err;
}

bh1750_err_t bh1750_read_raw_measurement(bh1750_t *dev, uint16_t *raw){
	//Sanity check
	if(!dev || !dev->hi2c){
		return BH1750_ERR_NULL;
	}

	//Read measurement
	uint8_t buffer[BH1750_MEASUREMENT_BYTES_SIZE] = {0};
	HAL_StatusTypeDef status = HAL_I2C_Master_Receive(dev->hi2c, dev->dev_address, buffer, BH1750_MEASUREMENT_BYTES_SIZE, 100);
	if(status != HAL_OK){
		if(status == HAL_TIMEOUT) return BH1750_ERR_TIMEOUT;
		else return BH1750_ERR_BUS;
	}

	//Unpack the measurement
	*raw =  ((uint16_t) buffer[0] << 8) | (uint16_t) buffer[1];
	return BH1750_OK;
}

float bh1750_convert_raw_to_lux(uint16_t raw){
	return (float) raw / BH1750_LUX_CONVERSION_FACTOR;
}
