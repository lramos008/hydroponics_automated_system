#include "i2c_manager.h"

/*Private defines*/
#define I2C_MANAGER_IS_DEVICE_READY_TIMEOUT_MS 1

/*Private functions*/
static i2c_manager_status_t i2c_manager_hal_status_translate(HAL_StatusTypeDef hal_status){
	switch(hal_status){
	case HAL_OK: 			return I2C_MGR_STATUS_OK;
	case HAL_BUSY:			return I2C_MGR_STATUS_BUSY;
	case HAL_TIMEOUT:		return I2C_MGR_STATUS_ERR_TIMEOUT;
	default:				return I2C_MGR_STATUS_ERROR;
	}
}

static i2c_manager_status_t i2c_manager_lock(i2c_manager_t *mgr, TickType_t timeout_ms){
	if(xSemaphoreTake(mgr->i2c_mutex, pdMS_TO_TICKS(timeout_ms)) != pdTRUE)	return I2C_MGR_STATUS_BUSY;

	return I2C_MGR_STATUS_OK;
}

static i2c_manager_status_t i2c_manager_unlock(i2c_manager_t *mgr){
	if(xSemaphoreGive(mgr->i2c_mutex) != pdTRUE)	return I2C_MGR_STATUS_ERR_MUTEX;

	return I2C_MGR_STATUS_OK;
}

/*Public functions*/
i2c_manager_status_t i2c_manager_init(i2c_manager_t *mgr, I2C_HandleTypeDef *hi2c, uint32_t hal_timeout_ms){
	if(mgr == NULL)				return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(hi2c == NULL)			return I2C_MGR_STATUS_ERR_NULL_POINTER;

	mgr->hi2c = hi2c;
	mgr->i2c_mutex = xSemaphoreCreateMutex();		//Mutex to ensure I2C bus ownership
	mgr->hal_timeout_ms = hal_timeout_ms;
	mgr->is_initialized = true;

	return I2C_MGR_STATUS_OK;
}

i2c_manager_status_t i2c_manager_write(i2c_manager_t *mgr, uint16_t dev_addr, const uint8_t *data, uint16_t len, TickType_t mutex_timeout_ms){
	if(mgr == NULL)				return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(data == NULL)			return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(len == 0)				return I2C_MGR_STATUS_OK;
	if(!mgr->is_initialized)	return I2C_MGR_STATUS_ERR_NOT_INITIALIZED;

	i2c_manager_status_t status;
	status = i2c_manager_lock(mgr, mutex_timeout_ms);					//Take ownership of I2C bus
	if(status != I2C_MGR_STATUS_OK)	return status;

	HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(mgr->hi2c, dev_addr, (uint8_t *) data, len, mgr->hal_timeout_ms);
	if(hal_status != HAL_OK)		return i2c_manager_hal_status_translate(hal_status);

	status = i2c_manager_unlock(mgr);								//Release I2C bus

	return status;
}

i2c_manager_status_t i2c_manager_read(i2c_manager_t *mgr, uint16_t dev_addr, uint8_t *data, uint16_t len, TickType_t mutex_timeout_ms){
	if(mgr == NULL)				return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(data == NULL)			return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(len == 0)				return I2C_MGR_STATUS_OK;
	if(!mgr->is_initialized)	return I2C_MGR_STATUS_ERR_NOT_INITIALIZED;

	i2c_manager_status_t status;
	status = i2c_manager_lock(mgr, mutex_timeout_ms);
	if(status != I2C_MGR_STATUS_OK)	return status;

	HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive(mgr->hi2c, dev_addr, data, len, mgr->hal_timeout_ms);
	if(hal_status != HAL_OK)		return i2c_manager_hal_status_translate(hal_status);

	status = i2c_manager_unlock(mgr);

	return status;
}

i2c_manager_status_t i2c_manager_mem_write(i2c_manager_t *mgr, uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size, const uint8_t *data, uint16_t len, TickType_t mutex_timeout_ms){
	if(mgr == NULL)				return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(data == NULL)			return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(len == 0)				return I2C_MGR_STATUS_OK;
	if(!mgr->is_initialized)	return I2C_MGR_STATUS_ERR_NOT_INITIALIZED;

	i2c_manager_status_t status;
	status = i2c_manager_lock(mgr, mutex_timeout_ms);
	if(status != I2C_MGR_STATUS_OK)	return status;

	HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Write(mgr->hi2c, dev_addr, mem_addr, mem_addr_size, (uint8_t *)data, len, mgr->hal_timeout_ms);
	if(hal_status != HAL_OK)		return i2c_manager_hal_status_translate(hal_status);

	status = i2c_manager_unlock(mgr);

	return status;
}

i2c_manager_status_t i2c_manager_mem_read(i2c_manager_t *mgr, uint16_t dev_addr, uint16_t mem_addr, uint16_t mem_addr_size, uint8_t *data, uint16_t len, TickType_t mutex_timeout_ms){
	if(mgr == NULL)				return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(data == NULL)			return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(len == 0)				return I2C_MGR_STATUS_OK;
	if(!mgr->is_initialized)	return I2C_MGR_STATUS_ERR_NOT_INITIALIZED;

	i2c_manager_status_t status;
	status = i2c_manager_lock(mgr, mutex_timeout_ms);
	if(status != I2C_MGR_STATUS_OK)	return status;

	HAL_StatusTypeDef hal_status = HAL_I2C_Mem_Read(mgr->hi2c, dev_addr, mem_addr, mem_addr_size, data, len, mgr->hal_timeout_ms);
	if(hal_status != HAL_OK)		return i2c_manager_hal_status_translate(hal_status);

	status = i2c_manager_unlock(mgr);

	return status;
}

i2c_manager_status_t i2c_manager_is_device_ready(i2c_manager_t *mgr, uint16_t dev_addr, uint32_t trials){
	if(mgr == NULL)				return I2C_MGR_STATUS_ERR_NULL_POINTER;
	if(!mgr->is_initialized)	return I2C_MGR_STATUS_ERR_NOT_INITIALIZED;

	i2c_manager_status_t status;
	status = i2c_manager_lock(mgr, pdMS_TO_TICKS(I2C_MANAGER_IS_DEVICE_READY_TIMEOUT_MS));
	if(status != I2C_MGR_STATUS_OK)	return status;

	HAL_StatusTypeDef hal_status = HAL_I2C_IsDeviceReady(mgr->hi2c, dev_addr, trials, mgr->hal_timeout_ms);
	if(hal_status != HAL_OK)		return i2c_manager_hal_status_translate(hal_status);

	status = i2c_manager_unlock(mgr);

	return status;
}


