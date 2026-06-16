#pragma once

/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Defines*/

/*Enums*/
//Status
typedef enum{
	ST7066U_OK,
	//Initialization
	ST7066U_ERR_NULL,
	ST7066U_ERR_INVALID_PARAMETER,
	ST7066U_ERR_NOT_INITIALIZED,
	//Generic error
	ST7066U_ERR_NOT_SPECIFIED
}st7066u_status_t;

//Data bus lines
typedef enum{
	ST7066U_DATA_BUS_LINE_1 = 0,		//DB4
	ST7066U_DATA_BUS_LINE_2,			//DB5
	ST7066U_DATA_BUS_LINE_3,			//DB6
	ST7066U_DATA_BUS_LINE_4,			//DB7
	ST7066U_DATA_BUS_LINE_COUNT,
}st7066u_data_bus_line_t;

/*Structs*/
//Pin definitions
typedef struct{
	GPIO_TypeDef *port;
	uint16_t pin;
}st7066u_pin_t;

typedef struct st7066u_internal_state_s st7066u_internal_state_t;

//ST7066U
typedef struct{
	st7066u_pin_t db_line[ST7066U_DATA_BUS_LINE_COUNT];
	st7066u_pin_t enable;
	st7066u_pin_t rs;
	st7066u_internal_state_t *state;
	uint8_t num_of_rows;
	uint8_t num_of_cols;
	bool is_initialized;
}st7066u_lcd_controller_t;

//API

/**
 *  @brief Initializes the lcd controller.
 *
 *  Ensures the lcd controller structure is filled with valid
 *  GPIO configurations for every pin and does the initialization
 *  sequence for its use.
 *
 *  @param dev Pointer to the st7066u_lcd_controller structure.
 *
 *  @return
 *  	- ST7066U_OK
 *  	- ST7066U_ERR_NULL
 *
 * 	@note GPIO pins of the struct must be configured before calling this function
 * 		  using the STM32CubeMX configurator.
 *
 */
st7066u_status_t st7066u_init(st7066u_lcd_controller_t *dev);
st7066u_status_t st7066u_write_char(st7066u_lcd_controller_t *dev, char ch);
st7066u_status_t st7066u_write_string(st7066u_lcd_controller_t *dev, char *str);
st7066u_status_t st7066u_set_cursor(st7066u_lcd_controller_t *dev, uint8_t row, uint8_t col);
st7066u_status_t st7066u_clear_display(st7066u_lcd_controller_t *dev);
st7066u_status_t st7066u_return_home(st7066u_lcd_controller_t *dev);
st7066u_status_t st7066u_set_display(st7066u_lcd_controller_t *dev, bool enabled);
st7066u_status_t st7066u_set_cursor_visible(st7066u_lcd_controller_t *dev, bool enabled);
st7066u_status_t st7066u_set_cursor_blink(st7066u_lcd_controller_t *dev, bool enabled);
st7066u_status_t st7066u_create_cgram_char(st7066u_lcd_controller_t *dev, uint8_t slot, const uint8_t bitmap[8]);

