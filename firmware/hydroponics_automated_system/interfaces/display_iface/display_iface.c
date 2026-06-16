/*Includes*/
#include "display_iface.h"

/*Private functions*/
static bool _display_iface_is_word_separator(char ch){
	return (ch == ' ' || ch == '\t');
}

static display_iface_status_t _display_iface_new_line(display_iface_t *iface){
	iface->cursor_pos.col = 0;
	iface->cursor_pos.row++;

	if(iface->cursor_pos.row >= iface->dev->num_of_rows)	return DISPLAY_ERR_TEXT_TRUNCATED;

	return display_iface_set_cursor(iface, iface->cursor_pos.row, iface->cursor_pos.col);
}

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

	//Init iface
	iface->cursor_pos.row = 0;
	iface->cursor_pos.col = 0;
	iface->is_initialized = true;

	return DISPLAY_OK;
}

display_iface_status_t display_iface_clear(display_iface_t *iface){
	if(iface == NULL)					return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)			return DISPLAY_ERR_NOT_INITIALIZED;

	st7066u_clear_display(iface->dev);
	st7066u_return_home(iface->dev);				//Reset cursor to 0,0

	return DISPLAY_OK;
}

display_iface_status_t display_iface_set_cursor(display_iface_t *iface, uint8_t row, uint8_t col){
	if(iface == NULL)														return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)												return DISPLAY_ERR_NOT_INITIALIZED;
	if(row >= iface->dev->num_of_rows || col >= iface->dev->num_of_cols)	return DISPLAY_ERR_INVALID_POSITION;

	//Save cursor pos
	iface->cursor_pos.row = row;
	iface->cursor_pos.col = col;
	//Set cursor
	st7066u_set_cursor(iface->dev, iface->cursor_pos.row, iface->cursor_pos.col);

	return DISPLAY_OK;
}

display_iface_status_t display_iface_write_char(display_iface_t *iface, char ch){
	if(iface == NULL)														return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)												return DISPLAY_ERR_NOT_INITIALIZED;
	if(iface->cursor_pos.row >= iface->dev->num_of_rows)					return DISPLAY_ERR_TEXT_TRUNCATED;

	//Check if wrap is needed
	if(iface->cursor_pos.col >= iface->dev->num_of_cols){
		//Write at the next row
		iface->cursor_pos.col = 0;
		iface->cursor_pos.row++;
		//Check if row is a valid one
		if(iface->cursor_pos.row >= iface->dev->num_of_rows)				return DISPLAY_ERR_TEXT_TRUNCATED;
		display_iface_set_cursor(iface, iface->cursor_pos.row, iface->cursor_pos.col);
	}
	//Write char
	st7066u_write_char(iface->dev, ch);
	iface->cursor_pos.col++;

	return DISPLAY_OK;
}

display_iface_status_t display_iface_write_string(display_iface_t *iface, char *str){
	if(iface == NULL)														return DISPLAY_ERR_NULL;
	if(str == NULL)															return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)												return DISPLAY_ERR_NOT_INITIALIZED;
	if(iface->cursor_pos.row >= iface->dev->num_of_rows)					return DISPLAY_ERR_TEXT_TRUNCATED;

	display_iface_status_t status;
	while(*str != '\0'){
		status = display_iface_write_char(iface, *str);
		if(status != DISPLAY_OK)	return status;
		str++;
	}

	return DISPLAY_OK;
}

display_iface_status_t display_iface_write_at_pos(display_iface_t *iface, uint8_t row, uint8_t col, char *str){
	if(iface == NULL)														return DISPLAY_ERR_NULL;
	if(str == NULL)															return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)												return DISPLAY_ERR_NOT_INITIALIZED;
	if(row >= iface->dev->num_of_rows || col >= iface->dev->num_of_cols)	return DISPLAY_ERR_INVALID_POSITION;

	display_iface_status_t status = display_iface_set_cursor(iface, row, col);
	status = display_iface_write_string(iface, str);

	return status;
}

display_iface_status_t display_iface_write_wrapped(display_iface_t *iface, char *str){
	if(iface == NULL)														return DISPLAY_ERR_NULL;
	if(str == NULL)															return DISPLAY_ERR_NULL;
	if(!iface->is_initialized)												return DISPLAY_ERR_NOT_INITIALIZED;
	if(iface->cursor_pos.row >= iface->dev->num_of_rows)					return DISPLAY_ERR_TEXT_TRUNCATED;

	display_iface_status_t status;

	while(*str != '\0'){
		if(*str == '\n'){
			status = _display_iface_new_line(iface);
			if(status != DISPLAY_OK)	return status;
			str++;
			continue;
		}

		if(_display_iface_is_word_separator(*str)){
			char *space_start = str;
			uint8_t space_count = 0;

			while(_display_iface_is_word_separator(*str)){
				space_count++;
				str++;
			}

			if(*str == '\0' || *str == '\n' || iface->cursor_pos.col == 0)	continue;

			uint8_t word_len = 0;
			while(str[word_len] != '\0' && str[word_len] != '\n' && !_display_iface_is_word_separator(str[word_len])){
				word_len++;
			}

			if(word_len <= iface->dev->num_of_cols && (iface->cursor_pos.col + space_count + word_len) > iface->dev->num_of_cols){
				status = _display_iface_new_line(iface);
				if(status != DISPLAY_OK)	return status;
				continue;
			}

			while(space_start < str){
				status = display_iface_write_char(iface, ' ');
				if(status != DISPLAY_OK)	return status;
				space_start++;
			}

			continue;
		}

		uint8_t word_len = 0;
		while(str[word_len] != '\0' && str[word_len] != '\n' && !_display_iface_is_word_separator(str[word_len])){
			word_len++;
		}

		if(iface->cursor_pos.col != 0 && (iface->cursor_pos.col + word_len) > iface->dev->num_of_cols){
			status = _display_iface_new_line(iface);
			if(status != DISPLAY_OK)	return status;
		}

		while(word_len > 0){
			status = display_iface_write_char(iface, *str);
			if(status != DISPLAY_OK)	return status;
			str++;
			word_len--;
		}
	}

	return DISPLAY_OK;
}
