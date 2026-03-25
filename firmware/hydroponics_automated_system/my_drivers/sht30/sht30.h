#ifndef SHT30_H
#define SHT30_H
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Public enums*/
typedef enum{
	SHT30_OK = 0,
	SHT30_ERR_NULL,
	SHT30_ERR_TIMEOUT,
	SHT30_ERR_BUS,
	SHT30_ERR_CRC
}sht30_err_t;

typedef enum{
	SHT30_HIGH_REPEATABILITY = 0,
	SHT30_MEDIUM_REPEATABILITY,
	SHT30_LOW_REPEATABILITY
}sht30_repeatability_t;

typedef enum{
	SHT30_CLK_STRETCHING_ENABLED = 0,
	SHT30_CLK_STRETCHING_DISABLED
}sht30_clk_stretching_t;


/*Public structures*/
typedef struct{
	I2C_HandleTypeDef *hi2c;
	uint8_t dev_address;
}sht30_t;

/*API functions*/
sht30_err_t sht30_init(sht30_t *dev, I2C_HandleTypeDef *hi2c, uint8_t dev_address);
sht30_err_t sht30_start_measurement(sht30_t *dev, sht30_repeatability_t rep, sht30_clk_stretching_t clk_stretch);
sht30_err_t sht30_get_raw_measurement(sht30_t *dev, uint16_t *temp_raw, uint16_t *hr_raw);
float sht30_convert_raw_to_temperature(uint16_t temp_raw);
float sht30_convert_raw_to_hr(uint16_t hr_raw);
#endif/*SHT30_H*/
