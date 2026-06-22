#pragma once

/*Include*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

/*Defines*/
#define I2C_MANAGER_QUEUE_CAPACITY 8U

/*Enums*/
typedef enum{
	I2C_MANAGER_STATUS_OK,
	I2C_MANAGER_STATUS_FULL,
	I2C_MANAGER_STATUS_QUEUE_FULL,
	I2C_MANAGER_STATUS_REQUEST_IN_USE,
	I2C_MANAGER_STATUS_INVALID_REQUEST
}i2c_manager_status_t;

typedef enum{
	I2C_MANAGER_RESULT_NONE,
	I2C_MANAGER_RESULT_OK,

	I2C_MANAGER_RESULT_TIMEOUT,
	I2C_MANAGER_RESULT_NACK,
	I2C_MANAGER_RESULT_BUS_ERROR,
	I2C_MANAGER_RESULT_DMA_ERROR,
	I2C_MANAGER_RESULT_ABORTED,

	I2C_MANAGER_RESULT_UNKNOWN_ERROR
}i2c_manager_result_t;

typedef enum{
	I2C_TRANSFER_WRITE,
	I2C_TRANSFER_READ
}i2c_transfer_type_t;

typedef enum{
	I2C_REQUEST_IDLE,
	I2C_REQUEST_QUEUED,
	I2C_REQUEST_ACTIVE,
	I2C_REQUEST_COMPLETE,
	I2C_REQUEST_ERROR
}i2c_request_state_t;

/*Structs*/
typedef struct{
	//I2C info
	uint16_t address;
	uint8_t *buffer;
	uint16_t length;
	i2c_transfer_type_t transfer_type;
	//Request state
	volatile i2c_request_state_t state;
	volatile i2c_manager_result_t result;
}i2c_request_t;

typedef struct{
	I2C_HandleTypeDef *hi2c;								//Handle for using I2C
	i2c_request_t *queue[I2C_MANAGER_QUEUE_CAPACITY];		//Array of pointers to requests
	uint8_t queue_head;
	uint8_t queue_tail;
	uint8_t queue_count;

	i2c_request_t *active_request;							//NULL means I2C bus is free

	volatile bool dma_finished;								//Set from the HAL DMA callbacks
	volatile i2c_manager_result_t dma_result;
	uint32_t active_deadline_ms;
}i2c_manager_t;

i2c_manager_status_t i2c_manager_init(i2c_manager_t *manager);
i2c_manager_status_t i2c_manager_submit(i2c_manager_t *manager, i2c_request_t *request);
void 				 i2c_manager_process(i2c_manager_t *manager, uint32_t now_ms);
void				 i2c_manager_on_complete_from_isr(i2c_manager_t *manager, i2c_manager_result_t result);



