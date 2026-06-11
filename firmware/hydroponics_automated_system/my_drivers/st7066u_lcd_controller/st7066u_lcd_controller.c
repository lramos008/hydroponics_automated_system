/*Includes*/
#include "st7066u_lcd_controller.h"

/*Defines*/
#define ST7066U_BOOT_TIME_MS			40
#define ST7066U_DEFAULT_WAIT_TIME_MS 	1
#define ST7066U_LONG_WAIT_TIME_MS	 	2				//Used with clear display and return home

/*Private functions*/
static void _st7066u_pulse_enable(st7066u_lcd_controller_t *dev){
	HAL_GPIO_WritePin(dev->enable.port, dev->enable.pin, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(dev->enable.port, dev->enable.pin, GPIO_PIN_RESET);
	HAL_Delay(1);
}

static void _st7066u_set_data_bus_lines(st7066u_lcd_controller_t *dev, uint8_t nibble){
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_1].port, dev->db_line[ST7066U_DATA_BUS_LINE_1].pin, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_2].port, dev->db_line[ST7066U_DATA_BUS_LINE_2].pin, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_3].port, dev->db_line[ST7066U_DATA_BUS_LINE_3].pin, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(dev->db_line[ST7066U_DATA_BUS_LINE_4].port, dev->db_line[ST7066U_DATA_BUS_LINE_4].pin, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void _st7066u_write_4bits(st7066u_lcd_controller_t *dev, uint8_t nibble){
	_st7066u_set_data_bus_lines(dev, nibble);
	HAL_Delay(1);
	_st7066u_pulse_enable(dev);
}

static void _st7066u_set_command_mode(st7066u_lcd_controller_t *dev){
	HAL_GPIO_WritePin(dev->rs.port, dev->rs.pin, GPIO_PIN_RESET);						//Set to 0 to send a command
}

static void _st7066u_set_data_mode(st7066u_lcd_controller_t *dev){
	HAL_GPIO_WritePin(dev->rs.port, dev->rs.pin, GPIO_PIN_SET);							//Set to 1 to send data
}


static void _st7066u_write_byte(st7066u_lcd_controller_t *dev, uint8_t data)
{
    _st7066u_write_4bits(dev, (data >> 4) & 0x0F);
    _st7066u_write_4bits(dev, data & 0x0F);
}


/*Public functions*/
st7066u_status_t st7066u_init(st7066u_lcd_controller_t *dev){
	//Sanity check
	dev->is_initialized = false;
	if(dev == NULL)	return ST7066U_ERR_NULL;
	//Check if every GPIO is defined
	for(uint8_t current_line = 0; current_line < ST7066U_DATA_BUS_LINE_COUNT; current_line++){

		if(dev->db_line[current_line].port == NULL)	return ST7066U_ERR_NULL;
	}
	if(dev->rs.port == NULL) 		return ST7066U_ERR_NULL;
	if(dev->enable.port == NULL)	return ST7066U_ERR_NULL;
	//LCD controller initialization
	dev->is_initialized = true;
	HAL_Delay(100);

	_st7066u_set_command_mode(dev);
	_st7066u_set_data_bus_lines(dev, 0x3);
	HAL_Delay(30);
	_st7066u_pulse_enable(dev);
	HAL_Delay(10);
	_st7066u_pulse_enable(dev);
	HAL_Delay(10);
	_st7066u_pulse_enable(dev);
	HAL_Delay(10);
	_st7066u_set_data_bus_lines(dev, 0x2);
	_st7066u_pulse_enable(dev);

	st7066u_send_command(dev, 0x28);
	HAL_Delay(ST7066U_DEFAULT_WAIT_TIME_MS);
	st7066u_send_command(dev, 0x10);
	HAL_Delay(ST7066U_DEFAULT_WAIT_TIME_MS);
	st7066u_send_command(dev, 0x0F);
	HAL_Delay(ST7066U_DEFAULT_WAIT_TIME_MS);
	st7066u_send_command(dev, 0x06);
	HAL_Delay(ST7066U_LONG_WAIT_TIME_MS);
	st7066u_send_command(dev, 0x80);
	HAL_Delay(ST7066U_DEFAULT_WAIT_TIME_MS);																						//Initialization complete
	return ST7066U_OK;
}


st7066u_status_t st7066u_send_command(st7066u_lcd_controller_t *dev, uint8_t cmd){
	//Sanity check
	if(dev == NULL)				return ST7066U_ERR_NULL;
	if(!dev->is_initialized)	return ST7066U_ERR_NOT_INITIALIZED;
	//Send command in 4 bits mode
	_st7066u_set_command_mode(dev);
	_st7066u_write_4bits(dev, cmd >> 4);
	_st7066u_write_4bits(dev, cmd);

	return ST7066U_OK;
}

st7066u_status_t st7066u_write_char(st7066u_lcd_controller_t *dev, char ch){
	//Sanity check
	if(dev == NULL)				return ST7066U_ERR_NULL;
	if(!dev->is_initialized)	return ST7066U_ERR_NOT_INITIALIZED;
	//Send command in 4 bits mode
	_st7066u_set_data_mode(dev);
	_st7066u_write_4bits(dev, (uint8_t) (ch >> 4));
	_st7066u_write_4bits(dev, (uint8_t) ch);

	return ST7066U_OK;
}
