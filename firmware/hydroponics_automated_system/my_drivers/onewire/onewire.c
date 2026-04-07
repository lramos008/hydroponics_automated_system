#include "onewire/onewire.h"
/*Defines / Macros*/
//System delay times
#define ONEWIRE_RESET_PULL_TIME_US					500
#define ONEWIRE_WAIT_BEFORE_PRESENCE_READ_US		70
#define ONEWIRE_WAIT_FOR_PRESENCE_PULSE_END_US		420
#define ONEWIRE_RECOVERY_TIME_US					2
#define ONEWIRE_SLOT_TIME_US						70
#define ONEWIRE_WRITE_1_PULL_TIME_US				5
#define ONEWIRE_WRITE_0_PULL_TIME_US				70
#define ONEWIRE_READ_PULL_TIME_US					2
#define ONEWIRE_READ_SAMPLE_TIME_US					8


//Commands
#define ONEWIRE_CMD_READ_ROM	0x33
#define ONEWIRE_CMD_MATCH_ROM	0x55
#define ONEWIRE_CMD_SKIP_ROM	0xCC

/*Private functions*/
static void delay_us(TIM_HandleTypeDef *htim, uint16_t us){
	uint16_t start = __HAL_TIM_GET_COUNTER(htim);

	//Wait to reach the counter
	while((uint16_t)(__HAL_TIM_GET_COUNTER(htim) - start) < us);
}

static void onewire_set_pin_mode(GPIO_TypeDef *port, uint16_t pin, uint32_t mode){
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = pin;
	GPIO_InitStruct.Mode = mode;
	GPIO_InitStruct.Pull = GPIO_NOPULL;					//Extern pull up for 1-wire
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void drive_line_low(onewire_t *bus){
	onewire_set_pin_mode(bus->port, bus->pin, GPIO_MODE_OUTPUT_OD);
	HAL_GPIO_WritePin(bus->port, bus->pin, GPIO_PIN_RESET);
}

static void release_line(onewire_t *bus){
	//Release line by configuring as input (Hi-Z) and letting the external pull up do its job
	onewire_set_pin_mode(bus->port, bus->pin, GPIO_MODE_INPUT);
}

static uint8_t read_line_state(onewire_t *bus){
	return HAL_GPIO_ReadPin(bus->port, bus->pin);
}

static void write_bit(onewire_t *bus, uint8_t bit){
	if(bit){
		drive_line_low(bus);
		delay_us(bus->htim, ONEWIRE_WRITE_1_PULL_TIME_US);									//Pull down the line within the first 15 us to write 1
		release_line(bus);
		delay_us(bus->htim, ONEWIRE_SLOT_TIME_US - ONEWIRE_WRITE_1_PULL_TIME_US);			//Wait slot time to end (total 70 us)
	}
	else{
		drive_line_low(bus);
		delay_us(bus->htim, ONEWIRE_WRITE_0_PULL_TIME_US);									//Write 0 occuppies all the time slot (70 us)
		release_line(bus);
	}

	delay_us(bus->htim, ONEWIRE_RECOVERY_TIME_US);											//Wait recovery time
}

static void read_bit(onewire_t *bus, uint8_t *bit){
	//Start read time slot
	drive_line_low(bus);
	delay_us(bus->htim, ONEWIRE_READ_PULL_TIME_US);
	release_line(bus);

	//Wait before sampling
	delay_us(bus->htim, ONEWIRE_READ_SAMPLE_TIME_US);										//Reading occurs at READ_PULL_TIME_US + READ_SAMPLE_TIME_US since the falling edge

	//Read bit
	*bit = (read_line_state(bus) == GPIO_PIN_RESET) ? 0 : 1;

	//Wait for slot to end
	delay_us(bus->htim, ONEWIRE_SLOT_TIME_US - ONEWIRE_READ_PULL_TIME_US - ONEWIRE_READ_SAMPLE_TIME_US);

	//Wait recovery time
	delay_us(bus->htim, ONEWIRE_RECOVERY_TIME_US);
}

/*Public functions*/
onewire_err_t onewire_init(onewire_t *bus, GPIO_TypeDef *port, uint16_t pin, TIM_HandleTypeDef *htim){
	//Sanity check
	if(!bus || !port || !htim){
		return ONEWIRE_ERR_NULL;
	}

	//Initialize handle
	bus->port = port;
	bus->pin = pin;
	bus->htim = htim;

	//Release line just in case
	release_line(bus);
	return ONEWIRE_OK;
}

onewire_err_t onewire_reset(onewire_t *bus){
	//Sanity check
	if(!bus || !bus->port || !bus->htim){
		return ONEWIRE_ERR_NULL;
	}

	drive_line_low(bus);
	delay_us(bus->htim, ONEWIRE_RESET_PULL_TIME_US);
	release_line(bus);
	delay_us(bus->htim, ONEWIRE_WAIT_BEFORE_PRESENCE_READ_US);

	//Check line state
	onewire_err_t err;
	if(read_line_state(bus) == GPIO_PIN_RESET){
		err = ONEWIRE_OK;														//Presence pulse detected
		delay_us(bus->htim, ONEWIRE_WAIT_FOR_PRESENCE_PULSE_END_US);			//Wait for presence pulse to end
	}
	else{
		err = ONEWIRE_ERR_NO_PRESENCE;											//Presence not detected
	}
	return err;
}


onewire_err_t onewire_write_byte(onewire_t *bus, uint8_t data){
	//Sanity check
	if(!bus || !bus->port || !bus->htim){
		return ONEWIRE_ERR_NULL;
	}

	//Send data bit by bit
	uint8_t bit = 0;
	for(uint8_t i = 0; i < 8; i++){
		bit = (data >> i) & 0x01;												//Least significant bit first
		write_bit(bus, bit);
	}
	return ONEWIRE_OK;
}

onewire_err_t onewire_read_byte(onewire_t *bus, uint8_t *data){
	//Sanity check
	if(!bus || !bus->port || !bus->htim || !data){
		return ONEWIRE_ERR_NULL;
	}

	//Set data to 0 just in case
	*data = 0;

	//Read data bit by bit
	uint8_t bit;
	for(uint8_t i = 0; i < 8; i++){
		read_bit(bus, &bit);
		if(bit){
			*data |= (1 << i);
		}
		else{
			*data &= ~(1 << i);
		}
	}
	return ONEWIRE_OK;
}

onewire_err_t onewire_write_multiple_bytes(onewire_t *bus, const uint8_t *data, size_t len){
	//Sanity check
	if(!bus || !bus->port || !bus->htim || !data){
		return ONEWIRE_ERR_NULL;
	}
	//Len check
	if(len == 0){
		return ONEWIRE_OK;
	}

	for(size_t i = 0; i < len; i++){
		onewire_write_byte(bus, data[i]);
	}
	return ONEWIRE_OK;
}

onewire_err_t onewire_read_multiple_bytes(onewire_t *bus, uint8_t *data, size_t len){
	//Sanity check
	if(!bus || !bus->port || !bus->htim || !data){
		return ONEWIRE_ERR_NULL;
	}
	//Len check
	if(len == 0){
		return ONEWIRE_OK;
	}

	for(size_t i = 0; i < len; i++){
		onewire_read_byte(bus, &data[i]);
	}
	return ONEWIRE_OK;
}

onewire_err_t onewire_skip_rom(onewire_t *bus){
	//Sanity check
	if(!bus || !bus->port || !bus->htim){
		return ONEWIRE_ERR_NULL;
	}

	onewire_write_byte(bus, ONEWIRE_CMD_SKIP_ROM);
	return ONEWIRE_OK;
}

onewire_err_t onewire_match_rom(onewire_t *bus, const uint8_t *rom, size_t len){
	//Sanity check
	if(!bus || !bus->port || !bus->htim || !rom){
		return ONEWIRE_ERR_NULL;
	}

	onewire_write_byte(bus, ONEWIRE_CMD_MATCH_ROM);
	onewire_write_multiple_bytes(bus, rom, len);
	return ONEWIRE_OK;
}

onewire_err_t onewire_read_rom(onewire_t *bus, uint8_t *rom, size_t len){
	//Sanity check
	if(!bus || !bus->port || !bus->htim || !rom){
		return ONEWIRE_ERR_NULL;
	}

	onewire_write_byte(bus, ONEWIRE_CMD_READ_ROM);
	onewire_read_multiple_bytes(bus, rom, len);
	return ONEWIRE_OK;
}

uint8_t onewire_crc8(const uint8_t *data, uint8_t len){
    uint8_t crc = 0;

    for(uint8_t i = 0; i < len; i++){
        uint8_t inbyte = data[i];

        for(uint8_t j = 0; j < 8; j++){
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;

            if(mix){
                crc ^= 0x8C;   // polinomio invertido
            }

            inbyte >>= 1;
        }
    }

    return crc;
}
