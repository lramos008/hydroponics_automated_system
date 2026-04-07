#ifndef DS18B20_H
#define DS18B20_H
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Public enums*/
typedef enum{
	DS18B20_OK,
	DS18B20_ERR_NULL,
	DS18B20_ERR_BUS,
	DS18B20_ERR_NO_PRESENCE,
	DS18B20_ERR_INVALID_ROM,
	DS18B20_ERR_ROM_NOT_FOUND,
	DS18B20_ERR_CONVERSION_NOT_READY,
	DS18B20_ERR_CRC
}ds18b20_err_t;

typedef enum{
	DS18B20_12_BIT_RESOLUTION,						//Default at power on
	DS18B20_11_BIT_RESOLUTION,
	DS18B20_10_BIT_RESOLUTION,
	DS18B20_9_BIT_RESOLUTION
}ds18b20_resolution_t;

/*Public structures*/
typedef struct{
	onewire_t *ow_bus;
	uint8_t rom[8];
}ds18b20_t;

/*API functions*/
ds18b20_err_t ds18b20_init_single_drop(ds18b20_t *dev, onewire_t *ow_bus);
ds18b20_err_t ds18b20_init_multi_drop(ds18b20_t *dev, onewire_t *ow_bus, const uint8_t rom[8]);
ds18b20_err_t ds18b20_start_temperature_conversion(ds18b20_t *dev);
ds18b20_err_t ds18b20_is_conversion_ready(ds18b20_t *dev);
ds18b20_err_t ds18b20_read_temperature(ds18b20_t *dev, float *temp);
ds18b20_err_t ds18b20_read_scratchpad(ds18b20_t *dev, uint8_t scratchpad[9]);								//Useful for debugging
ds18b20_err_t ds18b20_set_resolution(ds18b20_t *dev, ds18b20_resolution_t resolution);
ds18b20_err_t ds18b20_get_resolution(ds18b20_t *dev, ds18b20_resolution_t *resolution);
ds18b20_err_t ds18b20_validate_rom_family(const uint8_t rom[8]);
//ds18b20_err_t ds18b20_set_alarm_thresholds(ds18b20_t *dev, uint8_t th, uint8_t tl);						//Not useful with only one sensor
//ds18b20_err_t ds18b20_get_alarm_thresholds(ds18b20_t *dev, uint8_t *th, uint8_t *tl);						//Not useful with only one sensor
//ds18b20_err_t ds18b20_check_alarm_flag(ds18b20_t *dev, bool *alarm_flag);									//Not useful with only one sensor
#endif/*DS18B20_H*/
