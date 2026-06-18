/*Includes*/
#include "st7066u_lcd_controller.h"

#include "../../my_utils/delay_us/delay_us.h"

/*Private defines*/
#define ST7066U_BOOT_TIME_MS								100
#define ST7066U_DEFAULT_WAIT_TIME_MS 						1
#define ST7066U_LONG_WAIT_TIME_MS	 						2			//Used with clear display and return home

//Commands
#define ST7066U_CMD_CLEAR_DISPLAY							0x01
#define ST7066U_CMD_RETURN_HOME								0x02
#define ST7066U_CMD_ENTRY_MODE_SET							0x04
#define ST7066U_CMD_DISPLAY_CONTROL							0x08		//Display ON/OFF
#define ST7066U_CMD_CURSOR_AND_DISPLAY_SHIFT				0x10
#define ST7066U_CMD_FUNCTION_SET							0x20
#define ST7066U_CMD_SET_CGRAM_ADDRESS						0x40		//Use address as mask
#define ST7066U_CMD_SET_DDRAM_ADDRESS						0x80		//Use address as mask

//Masks
#define ST7066U_MASK_DIR_INCREMENT_DDRAM_ADDRESS			0x02		//Related to ENTRY_MODE_SET cmd, set moving direction
#define ST7066U_MASK_DIR_DECREMENT_DDRAM_ADDRESS			0x00		//Related to ENTRY_MODE_SET cmd, set moving direction
#define ST7066U_MASK_DIR_SHIFT_ENTIRE_DISPLAY_TO_THE_LEFT	0x03		//Related to ENTRY_MODE_SET cmd, set moving direction
#define ST7066U_MASK_DIR_SHIFT_ENTIRE_DISPLAY_TO_THE_RIGHT	0x01		//Related to ENTRY_MODE_SET cmd, set moving direction

#define ST7066U_MASK_SHIFT_CURSOR_TO_THE_LEFT				0x00		//Related to CURSOR_AND_DISPLAY_SHIFT cmd
#define ST7066U_MASK_SHIFT_CURSOR_TO_THE_RIGHT				0x04		//Related to CURSOR_AND_DISPLAY_SHIFT cmd
#define ST7066U_MASK_SHIFT_DISPLAY_TO_THE_LEFT				0x08		//Related to CURSOR_AND_DISPLAY_SHIFT cmd
#define ST7066U_MASK_SHIFT_DISPLAY_TO_THE_RIGHT				0x0C		//Related to CURSOR_AND_DISPLAY_SHIFT cmd

#define ST7066U_MASK_DISPLAY_ON								0x04		//Related to DISPLAY_CONTROL cmd
#define ST7066U_MASK_DISPLAY_OFF							0x00		//Related to DISPLAY_CONTROL cmd
#define ST7066U_MASK_CURSOR_ON								0x02		//Related to DISPLAY_CONTROL cmd
#define ST7066U_MASK_CURSOR_OFF								0x00		//Related to DISPLAY_CONTROL cmd
#define ST7066U_MASK_CURSOR_BLINK_ON						0x01		//Related to DISPLAY_CONTROL cmd
#define ST7066U_MASK_CURSOR_BLINK_OFF						0x00		//Related to DISPLAY_CONTROL cmd

#define	ST7066U_MASK_FUNCTION_8BIT							0x10		//Related to FUNCTION_SET cmd
#define ST7066U_MASK_FUNCTION_4BIT							0x00		//Related to FUNCTION_SET cmd
#define ST7066U_MASK_1_LINE_DISPLAY_MODE					0x00		//Related to FUNCTION_SET cmd
#define ST7066U_MASK_2_LINE_DISPLAY_MODE					0x08		//Related to FUNCTION_SET cmd
#define ST7066U_MASK_DISPLAY_5X8_DOT_FORMAT					0x00		//Related to FUNCTION_SET cmd
#define ST7066U_MASK_DISPLAY_5X11_DOT_FOMAT					0x04		//Related to FUNCTION_SET cmd

//Fonts
#define ST7066U_FONT_5X8									1
#define ST7066U_FONT_5X11									0

//Line number
#define ST7066U_DISPLAY_1_LINE								1
#define ST7066U_DISPLAY_2_LINE								0


/*Display configuration*/
#define ST7066U_CFG_FONT									ST7066U_FONT_5X8
#define ST7066U_CFG_DISPLAY_LINE							ST7066U_DISPLAY_2_LINE

#if ((ST7066U_CFG_FONT != ST7066U_FONT_5X8) && (ST7066U_CFG_FONT != ST7066U_FONT_5X11))									//Font validation
#error "Invalid ST7066U font config."
#endif

#if ((ST7066U_CFG_DISPLAY_LINE != ST7066U_DISPLAY_1_LINE) && (ST7066U_CFG_DISPLAY_LINE != ST7066U_DISPLAY_2_LINE))		//Display lines validation
#error "Invalid ST7066U display line config."
#endif

#if (ST7066U_CFG_FONT == ST7066U_FONT_5X8)
#define ST7066U_FUNCTION_SET_F_FIELD ST7066U_MASK_DISPLAY_5X8_DOT_FORMAT
#else
#define ST7066U_FUNCTION_SET_F_FIELD ST7066U_MASK_DISPLAY_5X11_DOT_FORMAT;
#endif

#if (ST7066U_CFG_DISPLAY_LINE == ST7066U_DISPLAY_2_LINE)
#define ST7066U_FUNCTION_SET_N_FIELD ST7066U_MASK_2_LINE_DISPLAY_MODE
#define ST7066U_NUM_OF_ROWS 		 4
#define ST7066U_NUM_OF_COLS			 20
#else
#define ST7066U_FUNCTION_SET_N_FIELD ST7066U_MASK_1_LINE_DISPLAY_MODE
#define ST7066U_NUM_OF_ROWS 		 1
#define ST7066U_NUM_OF_COLS			 16
#endif

#define ST7066U_FUNCTION_SET_DL_FIELD ST7066U_MASK_FUNCTION_4BIT

//Generate bitfields
#define ST7066U_FUNCTION_SET_BITFIELDS (ST7066U_CMD_FUNCTION_SET | ST7066U_FUNCTION_SET_DL_FIELD | ST7066U_FUNCTION_SET_F_FIELD | ST7066U_FUNCTION_SET_N_FIELD)

/*Private structs*/
struct st7066u_internal_state_s{
	uint8_t display_control_reg;
};

/*Private variable*/
struct st7066u_internal_state_s s_state = {0};

/*Private functions*/
static void _st7066u_select_instruction_register(st7066u_lcd_controller_t *dev){
	HAL_GPIO_WritePin(dev->rs.port, dev->rs.pin, GPIO_PIN_RESET);						//Set to 0 to send a command
}

static void _st7066u_select_data_register(st7066u_lcd_controller_t *dev){
	HAL_GPIO_WritePin(dev->rs.port, dev->rs.pin, GPIO_PIN_SET);							//Set to 1 to send data
}

static void _st7066u_pulse_enable(st7066u_lcd_controller_t *dev){
	HAL_GPIO_WritePin(dev->enable.port, dev->enable.pin, GPIO_PIN_SET);
	//HAL_Delay(1);
	delay_us(2);
	HAL_GPIO_WritePin(dev->enable.port, dev->enable.pin, GPIO_PIN_RESET);
}

static void _st7066u_set_data_bus_lines(st7066u_lcd_controller_t *dev, uint8_t nibble){
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_1].port, dev->db_line[ST7066U_DATA_BUS_LINE_1].pin, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_2].port, dev->db_line[ST7066U_DATA_BUS_LINE_2].pin, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_3].port, dev->db_line[ST7066U_DATA_BUS_LINE_3].pin, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_4].port, dev->db_line[ST7066U_DATA_BUS_LINE_4].pin, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void _st7066u_write_4bits(st7066u_lcd_controller_t *dev, uint8_t nibble){
	_st7066u_set_data_bus_lines(dev, nibble);
	_st7066u_pulse_enable(dev);
	HAL_Delay(ST7066U_DEFAULT_WAIT_TIME_MS);
}

static void _st7066u_write_byte(st7066u_lcd_controller_t *dev, uint8_t data)
{
	_st7066u_write_4bits(dev, (data >> 4) & 0x0F);
	_st7066u_write_4bits(dev, data & 0x0F);
}

static void _st7066u_send_command(st7066u_lcd_controller_t *dev, uint8_t cmd){
	_st7066u_select_instruction_register(dev);
	_st7066u_write_byte(dev, cmd);
}

static void _st7066u_send_data(st7066u_lcd_controller_t *dev, uint8_t data){
	_st7066u_select_data_register(dev);
	_st7066u_write_byte(dev, data);
}

static void _st7066u_set_display_4bits_mode(st7066u_lcd_controller_t *dev){
	HAL_Delay(ST7066U_BOOT_TIME_MS);
	_st7066u_select_instruction_register(dev);
	_st7066u_write_4bits(dev, 0x3);					//Wake up. Set 8-bit interface
	_st7066u_write_4bits(dev, 0x3);
	_st7066u_write_4bits(dev, 0x3);
	_st7066u_write_4bits(dev, 0x2);					//Set 4-bit interface
}


/*Public functions*/
st7066u_status_t st7066u_init(st7066u_lcd_controller_t *dev){
	//Sanity check
	if(dev == NULL)	return ST7066U_ERR_NULL;
	//Check if every GPIO is defined
	for(uint8_t current_line = 0; current_line < ST7066U_DATA_BUS_LINE_COUNT; current_line++){

		if(dev->db_line[current_line].port == NULL)	return ST7066U_ERR_NULL;
	}
	if(dev->rs.port == NULL) 		return ST7066U_ERR_NULL;
	if(dev->enable.port == NULL)	return ST7066U_ERR_NULL;
	//LCD controller initialization
	dev->state = &s_state;
	dev->num_of_rows = ST7066U_NUM_OF_ROWS;
	dev->num_of_cols = ST7066U_NUM_OF_COLS;
	dev->is_initialized = true;

	_st7066u_set_display_4bits_mode(dev);

	//Config display mode, font and number of lines
	_st7066u_send_command(dev, ST7066U_FUNCTION_SET_BITFIELDS);


	//Turn on display and set cursor and DDRAM config
	dev->state->display_control_reg = ST7066U_CMD_DISPLAY_CONTROL | ST7066U_MASK_DISPLAY_ON | ST7066U_MASK_CURSOR_OFF | ST7066U_MASK_CURSOR_BLINK_OFF;
	_st7066u_send_command(dev, ST7066U_CMD_DISPLAY_CONTROL | ST7066U_MASK_DISPLAY_ON | ST7066U_MASK_CURSOR_OFF | ST7066U_MASK_CURSOR_BLINK_OFF);
	_st7066u_send_command(dev, ST7066U_CMD_ENTRY_MODE_SET | ST7066U_MASK_DIR_INCREMENT_DDRAM_ADDRESS);
	_st7066u_send_command(dev, ST7066U_CMD_CLEAR_DISPLAY);
	HAL_Delay(2);

	return ST7066U_OK;
}

st7066u_status_t st7066u_write_char(st7066u_lcd_controller_t *dev, char ch){
	if(dev == NULL)		return ST7066U_ERR_NULL;
	if(!dev->is_initialized)	return ST7066U_ERR_NOT_INITIALIZED;

	_st7066u_send_data(dev, ch);

	return ST7066U_OK;
}

st7066u_status_t st7066u_write_string(st7066u_lcd_controller_t *dev, char *str){
	if(dev == NULL)				return ST7066U_ERR_NULL;
	if(str == NULL)				return ST7066U_ERR_NULL;
	if(!dev->is_initialized)	return ST7066U_ERR_NOT_INITIALIZED;

	while(*str != '\0'){
		st7066u_write_char(dev, *str);
		str++;
	}

	return ST7066U_OK;
}

st7066u_status_t st7066u_set_cursor(st7066u_lcd_controller_t *dev, uint8_t row, uint8_t col){
	if(dev == NULL)						return ST7066U_ERR_NULL;
	if(!dev->is_initialized)			return ST7066U_ERR_NOT_INITIALIZED;
	if(row >= dev->num_of_rows)			return ST7066U_ERR_INVALID_PARAMETER;
	if(col >= dev->num_of_cols)			return ST7066U_ERR_INVALID_PARAMETER;

	static const uint8_t row_offset[] = {0x00, 0x40, 0x14, 0x54};
	uint8_t ddram_address = row_offset[row] + col;
	_st7066u_send_command(dev, ST7066U_CMD_SET_DDRAM_ADDRESS | ddram_address);

	return ST7066U_OK;
}

st7066u_status_t st7066u_clear_display(st7066u_lcd_controller_t *dev){
	if(dev == NULL)						return ST7066U_ERR_NULL;
	if(!dev->is_initialized)			return ST7066U_ERR_NOT_INITIALIZED;

	_st7066u_send_command(dev, ST7066U_CMD_CLEAR_DISPLAY);

	return ST7066U_OK;
}

st7066u_status_t st7066u_return_home(st7066u_lcd_controller_t *dev){
	if(dev == NULL)						return ST7066U_ERR_NULL;
	if(!dev->is_initialized)			return ST7066U_ERR_NOT_INITIALIZED;

	_st7066u_send_command(dev, ST7066U_CMD_RETURN_HOME);

	return ST7066U_OK;
}

st7066u_status_t st7066u_set_display(st7066u_lcd_controller_t *dev, bool enabled){
	if(dev == NULL)						return ST7066U_ERR_NULL;
	if(!dev->is_initialized)			return ST7066U_ERR_NOT_INITIALIZED;

	bool is_display_on = (bool)(dev->state->display_control_reg & ST7066U_MASK_DISPLAY_ON);

	if(enabled && !is_display_on){
		dev->state->display_control_reg |= ST7066U_MASK_DISPLAY_ON;
		_st7066u_send_command(dev, dev->state->display_control_reg);
	}
	else if(!enabled && is_display_on){
		dev->state->display_control_reg &= ~ST7066U_MASK_DISPLAY_ON;
		_st7066u_send_command(dev, dev->state->display_control_reg);
	}
	else{

	}

	return ST7066U_OK;
}

st7066u_status_t st7066u_set_cursor_visible(st7066u_lcd_controller_t *dev, bool enabled){
	if(dev == NULL)						return ST7066U_ERR_NULL;
	if(!dev->is_initialized)			return ST7066U_ERR_NOT_INITIALIZED;

	bool is_cursor_visible = (bool)(dev->state->display_control_reg & ST7066U_MASK_CURSOR_ON);

	if(enabled && !is_cursor_visible){
		dev->state->display_control_reg |= ST7066U_MASK_CURSOR_ON;
		_st7066u_send_command(dev, dev->state->display_control_reg);
	}
	else if(!enabled && is_cursor_visible){
		dev->state->display_control_reg &= ~ST7066U_MASK_CURSOR_ON;
		_st7066u_send_command(dev, dev->state->display_control_reg);
	}
	else{

	}

	return ST7066U_OK;
}

st7066u_status_t st7066u_set_cursor_blink(st7066u_lcd_controller_t *dev, bool enabled){
	if(dev == NULL)						return ST7066U_ERR_NULL;
	if(!dev->is_initialized)			return ST7066U_ERR_NOT_INITIALIZED;

	bool is_cursor_blinking = (bool)(dev->state->display_control_reg & ST7066U_MASK_CURSOR_BLINK_ON);

	if(enabled && !is_cursor_blinking){
		dev->state->display_control_reg |= ST7066U_MASK_CURSOR_BLINK_ON;
		_st7066u_send_command(dev, dev->state->display_control_reg);
	}
	else if(!enabled && is_cursor_blinking){
		dev->state->display_control_reg &= ~ST7066U_MASK_CURSOR_BLINK_ON;
		_st7066u_send_command(dev, dev->state->display_control_reg);
	}
	else{

	}

	return ST7066U_OK;
}
