#pragma once

/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "st7066u_lcd_controller/st7066u_lcd_controller.h"

/*Enums*/
typedef enum{
	DISPLAY_OK = 0,
	//Initialization
	DISPLAY_ERR_NULL,
	DISPLAY_ERR_NOT_INITIALIZED,
	//Params
	DISPLAY_ERR_INVALID_PARAM,
	//Controller
	DISPLAY_ERR_CONTROLLER_NOT_INITIALIZED,
	//Cursor
	DISPLAY_ERR_INVALID_POSITION,
	DISPLAY_ERR_INVALID_LINE,
	//Text
	DISPLAY_ERR_TEXT_TRUNCATED
}display_iface_status_t;

/*Structs*/
typedef struct{
	uint8_t row;
	uint8_t col;
}display_iface_cursor_pos_t;

typedef struct{
	st7066u_lcd_controller_t *dev;
	display_iface_cursor_pos_t cursor_pos;
	bool is_initialized;
}display_iface_t;

/*API*/
//Basic
display_iface_status_t display_iface_init(display_iface_t *iface);
display_iface_status_t display_iface_clear(display_iface_t *iface);
display_iface_status_t display_iface_clear_line(display_iface_t *iface, uint8_t num_line);
display_iface_status_t display_iface_write_line(display_iface_t *iface, uint8_t num_line, char *str);
//Helpers
display_iface_status_t display_iface_set_cursor(display_iface_t *iface, uint8_t row, uint8_t col);
display_iface_status_t display_iface_write_char(display_iface_t *iface, char ch);
display_iface_status_t display_iface_write_string(display_iface_t *iface, char *str);
