#pragma once

/*Include*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"

/*Defines*/
#define I2C_MGR_DEFAULT_MUTEX_TIMEOUT_MS 10U

/*Enums*/
typedef enum{
	I2C_MGR_STATUS_OK,
	I2C_MGR_STATUS_BUSY,
	I2C_MGR_STATUS_ERR_TIMEOUT,
	I2C_MGR_STATUS_ERR_NOT_INITIALIZED,
	I2C_MGR_STATUS_ERROR,
	I2C_MGR_STATUS_ERR_INVALID_ARG,
	I2C_MGR_STATUS_ERR_NULL_POINTER,
	I2C_MGR_STATUS_ERR_MUTEX
}i2c_manager_status_t;

/*Structs*/
typedef struct{
	I2C_HandleTypeDef *hi2c;
	SemaphoreHandle_t i2c_mutex;
	uint32_t hal_timeout_ms;
	bool is_initialized;
}i2c_manager_t;

/*API*/
i2c_manager_status_t i2c_manager_init(i2c_manager_t *mgr, I2C_HandleTypeDef *hi2c, uint32_t hal_timeout_ms);
i2c_manager_status_t i2c_manager_write(i2c_manager_t *mgr, uint16_t dev_addr, const uint8_t *data, uint16_t len, TickType_t mutex_timeout);
i2c_manager_status_t i2c_manager_read(i2c_manager_t *mgr, uint16_t dev_addr, uint8_t *data, uint16_t len, TickType_t mutex_timeout);
i2c_manager_status_t i2c_manager_mem_write(i2c_manager_t *mgr, uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size, const uint8_t *data, uint16_t len, TickType_t mutex_timeout);
i2c_manager_status_t i2c_manager_mem_read(i2c_manager_t *mgr, uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size, uint8_t *data, uint16_t len, TickType_t mutex_timeout);
i2c_manager_status_t i2c_manager_is_device_ready(i2c_manager_t *mgr, uint16_t dev_addr, uint32_t trials);
i2c_manager_status_t i2c_manager_recover(i2c_manager_t *mgr);
