/*Includes*/
#include "display_iface.h"

/*Private functions*/
display_iface_status_t display_iface_home(display_iface_t *iface){
	if(iface == NULL)					return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)			return DISPLAY_ERR_NOT_INITIALIZED;

	st7066u_return_home(iface->dev);				//Reset cursor to 0,0
	//Save cursor position
	iface->cursor_pos.row = 0;
	iface->cursor_pos.col = 0;

	return DISPLAY_OK;
}


/*Public functions*/
display_iface_status_t display_iface_init(display_iface_t *iface){
	if(iface == NULL)					return DISPLAY_ERR_NULL;
	if(iface->dev == NULL)				return DISPLAY_ERR_NULL;
	if(!iface->dev->is_initialized)		return DISPLAY_ERR_CONTROLLER_NOT_INITIALIZED;

	//Config display
	st7066u_set_display(iface->dev, true);
	st7066u_set_cursor(iface->dev, 0, 0);
	st7066u_set_cursor_visible(iface->dev, false);
	st7066u_set_cursor_blink(iface->dev, false);
	st7066u_clear_display(iface->dev);
	//Set initial cursor position
	iface->cursor_pos.row = 0;
	iface->cursor_pos.col = 0;
	//Initialize iface
	iface->is_initialized = true;

	return DISPLAY_OK;
}

display_iface_status_t display_iface_clear(display_iface_t *iface){
	if(iface == NULL)					return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)			return DISPLAY_ERR_NOT_INITIALIZED;

	st7066u_clear_display(iface->dev);
	display_iface_home(iface);					//Set cursor to 0,0

	return DISPLAY_OK;
}

display_iface_status_t display_iface_clear_line(display_iface_t *iface, uint8_t num_line){
	if(iface == NULL)						return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)				return DISPLAY_ERR_NOT_INITIALIZED;
	if(num_line >= iface->dev->num_of_rows)	return DISPLAY_ERR_INVALID_LINE;

	display_iface_status_t status;
	status = display_iface_set_cursor(iface, num_line, 0);
	if(status != DISPLAY_OK)	return status;

	//Clear whole line
	for(uint8_t i = 0; i < iface->dev->num_of_cols; i++){
		status = display_iface_write_char(iface, ' ');
		if(status != DISPLAY_OK)	return status;
	}

	return display_iface_set_cursor(iface, num_line, 0);
}

display_iface_status_t display_iface_write_line(display_iface_t *iface, uint8_t num_line, char *str){
	if(iface == NULL)						return DISPLAY_ERR_NULL;
	if(str == NULL)							return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)				return DISPLAY_ERR_NOT_INITIALIZED;
	if(num_line >= iface->dev->num_of_rows)	return DISPLAY_ERR_INVALID_LINE;

	display_iface_status_t status;
	status = display_iface_set_cursor(iface, num_line, 0);
	if(status != DISPLAY_OK)	return status;

	//Write string char by char
	uint8_t char_count = 0;
	while(*str != '\0'){
		//Check if end of line was reached
		if(char_count >= iface->dev->num_of_cols)	return DISPLAY_ERR_TEXT_TRUNCATED;

		status = display_iface_write_char(iface, *str);
		if(status != DISPLAY_OK)	return status;
		str++;
		char_count++;
	}

	return DISPLAY_OK;
}

display_iface_status_t display_iface_set_cursor(display_iface_t *iface, uint8_t row, uint8_t col){
	if(iface == NULL)														return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)												return DISPLAY_ERR_NOT_INITIALIZED;
	if(row >= iface->dev->num_of_rows || col >= iface->dev->num_of_cols)	return DISPLAY_ERR_INVALID_POSITION;

	//Set cursor
	st7066u_set_cursor(iface->dev, row, col);
	//Save cursor pos
	iface->cursor_pos.row = row;
	iface->cursor_pos.col = col;

	return DISPLAY_OK;
}

display_iface_status_t display_iface_write_char(display_iface_t *iface, char ch){
	if(iface == NULL)						return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)				return DISPLAY_ERR_NOT_INITIALIZED;

	st7066u_write_char(iface->dev, ch);
	//Increase cursor pos
	iface->cursor_pos.col++;
	if(iface->cursor_pos.col >= iface->dev->num_of_cols){
		iface->cursor_pos.col = 0;
		iface->cursor_pos.row++;
		display_iface_status_t status = display_iface_set_cursor(iface, iface->cursor_pos.row, iface->cursor_pos.col);
		if(status != DISPLAY_OK)			return status;
	}

	return DISPLAY_OK;
}

display_iface_status_t display_iface_write_string(display_iface_t *iface, char *str){
	if(iface == NULL)				return DISPLAY_ERR_NULL;
	if(str == NULL)					return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)		return DISPLAY_ERR_NOT_INITIALIZED;

	while(*str != '\0'){
		display_iface_status_t status = display_iface_write_char(iface, *str);
		if(status != DISPLAY_OK)	return status;

		str++;
	}

	return DISPLAY_OK;
}
