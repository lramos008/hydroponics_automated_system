#pragma once

/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Defines*/
//Commands
#define ST7066U_CMD_INIT_STATE				0x00
#define ST7066U_CMD_CLEAR_DISPLAY			0x01
#define ST7066U_CMD_RETURN_HOME				0x02
#define ST7066U_CMD_ENTRY_MODE_SET			0x04
#define ST7066U_CMD_DISPLAY_CONTROL			0x08
#define ST7066U_CMD_CURSOR_SHIFT			0x10
#define ST7066U_CMD_FUNCTION_SET			0x20
#define ST7066U_CMD_SET_DDRAM_ADDRESS		0x80			//Use address as mask

//Masks
#define ST7066U_SHIFT_CURSOR_TO_THE_LEFT	0x00
#define ST7066U_SHIFT_CURSOR_TO_THE_RIGHT	0x02
#define ST7066U_SHIFT_DISPLAY_TO_THE_LEFT	0x03
#define ST7066U_SHIFT_DISPLAY_TO_THE_RIGHT	0x01
#define ST7066U_DISPLAY_ON					0x04
#define ST7066U_DISPLAY_OFF					0x00
#define ST7066U_CURSOR_ON					0x02
#define ST7066U_CURSOR_OFF					0x00
#define ST7066U_CURSOR_BLINK_ON				0x01
#define ST7066U_CURSOR_BLINK_OFF			0x00
#define	ST7066U_FUNCTION_8BIT				0x10
#define ST7066U_FUNCTION_4BIT				0x00
#define ST7066U_1_LINE_DISPLAY_MODE			0x00
#define ST7066U_2_LINE_DISPLAY_MODE			0x08
#define ST7066U_DISPLAY_5X8_DOT_FORMAT		0x00
#define ST7066U_DISPLAY_5X11_DOT_FOMAT		0x04

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

//ST7066U
typedef struct{
	st7066u_pin_t db_line[ST7066U_DATA_BUS_LINE_COUNT];
	st7066u_pin_t enable;
	st7066u_pin_t rs;
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
st7066u_status_t st7066u_send_command(st7066u_lcd_controller_t *dev, uint8_t cmd);
st7066u_status_t st7066u_write_char(st7066u_lcd_controller_t *dev, char ch);
st7066u_status_t st7066u_write_string(st7066u_lcd_controller_t *dev, char *ch);


//lcd_init()
//lcd_clear()
//lcd_home()
//lcd_set_cursor()
//lcd_write_char()
//
//lcd_display_on()
//lcd_display_off()
//
//lcd_cursor_on()
//lcd_cursor_off()
//
//lcd_blink_on()
//lcd_blink_off()
//
//lcd_shift_left()
//lcd_shift_right()
//
//lcd_create_char()
