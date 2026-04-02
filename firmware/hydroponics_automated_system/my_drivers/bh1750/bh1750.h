#ifndef BH1750_H
#define BH1750_H
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Public enums*/
typedef enum{
	BH1750_OK,
	BH1750_ERR_NULL,
	BH1750_ERR_TIMEOUT,
	BH1750_ERR_BUS
}bh1750_err_t;

typedef enum{
	BH1750_LOW_RES_MODE,
	BH1750_HI_RES_MODE,
	BH1750_HI_RES_MODE_2
}bh1750_res_mode_t;

/*Public structures*/
typedef struct{
	I2C_HandleTypeDef *hi2c;
	uint8_t dev_address;
}bh1750_t;

/*API functions*/
bh1750_err_t bh1750_init(bh1750_t *dev, I2C_HandleTypeDef *hi2c, uint8_t dev_address);
bh1750_err_t bh1750_power_on(bh1750_t *dev);
bh1750_err_t bh1750_power_down(bh1750_t *dev);
bh1750_err_t bh1750_reset(bh1750_t *dev);
bh1750_err_t bh1750_start_measurement(bh1750_t *dev, bh1750_res_mode_t res_mode);
bh1750_err_t bh1750_read_raw_measurement(bh1750_t *dev, uint16_t *raw);
float bh1750_convert_raw_to_lux(uint16_t raw);
#endif/*BH1750_H*/
