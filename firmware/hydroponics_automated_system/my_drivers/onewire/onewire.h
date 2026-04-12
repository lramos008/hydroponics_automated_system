#ifndef ONEWIRE_H
#define ONEWIRE_H
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

/*Public enums*/
typedef enum{
	ONEWIRE_OK,
	ONEWIRE_ERR_NULL,
	ONEWIRE_ERR_NO_PRESENCE,
	ONEWIRE_ERR_BUS,
	ONEWIRE_ERR_NO_MORE_DEVICES_FOUND
}onewire_err_t;


/*Public structures*/
typedef struct{
	GPIO_TypeDef *port;
	uint16_t pin;
	TIM_HandleTypeDef *htim;
}onewire_t;

/*API functions*/
//One-wire low level functions
onewire_err_t onewire_init(onewire_t *bus, GPIO_TypeDef *port, uint16_t pin, TIM_HandleTypeDef *htim);
onewire_err_t onewire_reset(onewire_t *bus);
onewire_err_t onewire_read_bit(onewire_t *bus, uint8_t *bit);
onewire_err_t onewire_write_byte(onewire_t *bus, uint8_t data);
onewire_err_t onewire_read_byte(onewire_t *bus, uint8_t *data);
onewire_err_t onewire_write_multiple_bytes(onewire_t *bus, const uint8_t *data, size_t len);
onewire_err_t onewire_read_multiple_bytes(onewire_t *bus, uint8_t *data, size_t len);
uint8_t onewire_crc8(const uint8_t *data, uint8_t len);

//ROM commands
onewire_err_t onewire_skip_rom(onewire_t *bus);
onewire_err_t onewire_match_rom(onewire_t *bus, const uint8_t *rom, size_t len);
onewire_err_t onewire_read_rom(onewire_t *bus, uint8_t *rom, size_t len);
//Search rom will be implemented in the future
#endif/*ONEWIRE_H*/
