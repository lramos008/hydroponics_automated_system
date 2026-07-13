#ifndef DS18B20_H
#define DS18B20_H
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "onewire/onewire.h"

/*Public defines*/
#define DS18B20_ROM_SIZE_BYTES				8
#define DS18B20_9_BITS_RESOLUTION_STEP			0.5f
#define DS18B20_10_BITS_RESOLUTION_STEP			0.25f
#define DS18B20_11_BITS_RESOLUTION_STEP			0.125f
#define DS18B20_12_BITS_RESOLUTION_STEP			0.0625f

/*Public enums*/
typedef enum{
	DS18B20_OK,
	DS18B20_CONVERSION_NOT_READY,
	DS18B20_ERR_NULL,
	DS18B20_ERR_BUS,
	DS18B20_ERR_NO_PRESENCE,
	DS18B20_ERR_INVALID_ROM,
	DS18B20_ERR_ROM_NOT_FOUND,
	DS18B20_ERR_CRC,
	DS18B20_BUSY,
	DS18B20_ERR_NOT_INITIALIZED,
	DS18B20_ERR_INVALID_RESOLUTION,
	DS18B20_ERR_INVALID_STATE,
	DS18B20_ERROR
}ds18b20_err_t;

typedef enum{
	DS18B20_12_BIT_RESOLUTION,						//Default at power on
	DS18B20_11_BIT_RESOLUTION,
	DS18B20_10_BIT_RESOLUTION,
	DS18B20_9_BIT_RESOLUTION,
	DS18B20_UNKNOWN_RESOLUTION
}ds18b20_resolution_t;

typedef enum{
	DS18B20_STATE_IDLE = 0,
	DS18B20_STATE_STARTING_CONVERSION,
	DS18B20_STATE_WAITING_CONVERSION,
	DS18B20_STATE_READING_SCRATCHPAD,
	DS18B20_STATE_MEASUREMENT_IS_READY,
	DS18B20_STATE_ERROR
}ds18b20_state_t;

/*Public structures*/
typedef struct{
	onewire_t *ow_bus;
	ds18b20_resolution_t resolution;
	const uint8_t *rom;
	size_t rom_len;
}ds18b20_config_t;

typedef struct{
	int16_t raw_temperature;
	float temperature;
}ds18b20_data_t;

typedef struct{
	ds18b20_config_t cfg;
	uint8_t rom[DS18B20_ROM_SIZE_BYTES];
	ds18b20_resolution_t resolution;
	ds18b20_state_t state;
	ds18b20_err_t last_status;
	ds18b20_err_t error_cause;
	ds18b20_data_t data;
	TickType_t measurement_start_tick;
	TickType_t measurement_wait_ticks;
	bool is_initialized;
	bool start_requested;
	bool data_consumed;
}ds18b20_t;

/*API functions*/
ds18b20_err_t ds18b20_init(ds18b20_t *dev, ds18b20_config_t *cfg);
ds18b20_err_t ds18b20_start_measurement(ds18b20_t *dev);
ds18b20_err_t ds18b20_process(ds18b20_t *dev);
bool ds18b20_is_ready(ds18b20_t *dev);
ds18b20_err_t ds18b20_get_data(ds18b20_t *dev, ds18b20_data_t *data);
ds18b20_err_t ds18b20_reset(ds18b20_t *dev);
//ds18b20_err_t ds18b20_set_alarm_thresholds(ds18b20_t *dev, uint8_t th, uint8_t tl);						//Not useful with only one sensor
//ds18b20_err_t ds18b20_get_alarm_thresholds(ds18b20_t *dev, uint8_t *th, uint8_t *tl);						//Not useful with only one sensor
//ds18b20_err_t ds18b20_check_alarm_flag(ds18b20_t *dev, bool *alarm_flag);									//Not useful with only one sensor
#endif/*DS18B20_H*/
