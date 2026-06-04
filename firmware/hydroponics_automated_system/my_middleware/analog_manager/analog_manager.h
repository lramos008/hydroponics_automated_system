#pragma once
/*Includes*/
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "filters/moving_average.h"

/*Defines*/
#define ANALOG_MOVING_AVG_FILTER_SIZE 64U		//Must be a power of 2

/*Enums*/
typedef enum{
	ANALOG_CHANNEL_EC,
	ANALOG_CHANNEL_PH,
	ANALOG_CHANNEL_CT_CIRCULATION,

	ANALOG_CHANNEL_COUNT									//Used to automatically know the number of channels
}analog_channel_id_t;

typedef enum{
	ANALOG_MANAGER_OK,
	//Initialization
	ANALOG_MANAGER_ERR_NULL,
	ANALOG_MANAGER_ERR_NOT_INITIALIZED,
	//Channel
	ANALOG_MANAGER_ERR_INVALID_CHANNEL,
	//ADC
	ANALOG_MANAGER_ERR_ADC_START,
	ANALOG_MANAGER_ERR_ADC_STOP,
	//Filter
	ANALOG_MANAGER_ERR_FILTER_INIT,
	ANALOG_MANAGER_ERR_FILTER_UPDATE,
	ANALOG_MANAGER_ERR_FILTER_RESET
}analog_manager_status_t;

/*Structs*/
typedef struct{
	//Hardware
	ADC_HandleTypeDef *hadc;
	float vref;
	uint16_t adc_resolution;
	//DMA channel samples
	uint16_t dma_buffer[ANALOG_CHANNEL_COUNT];
	//Filtered voltages (output ready for being used by interfaces)
	float filtered_voltage[ANALOG_CHANNEL_COUNT];
	//Moving average filters
	moving_average_handle_t filters[ANALOG_CHANNEL_COUNT];
	float filter_buffers[ANALOG_CHANNEL_COUNT][ANALOG_MOVING_AVG_FILTER_SIZE];
	//Internal status
	bool is_initialized;
}analog_manager_t;

/*API*/

/**
 *  @brief Initializes the analog manager service.
 *
 *  Configures all the internal resources that are used by the analog
 *  manager, including filters and buffers. It also checks for a valid
 *  ADC_HandleTypeDef.
 *
 *  @param analog Pointer to the analog manager handle.
 *
 *  @return
 *  	- ANALOG_MANAGER_OK
 *  	- ANALOG_MANAGER_ERR_NULL
 *  	- ANALOG_MANAGER_ERR_FILTER_INIT
 *
 * 	@note The ADC peripheral must be configured before calling this function.
 * 		  Be sure to set DMA and configure the ADC ranks following the order
 * 		  of the analog_channel_id_t enum.
 *
 * 	@warning Calling any function from this library before analog_manager_init
 * 			 results in undefined behavior.
 *
 */
analog_manager_status_t analog_manager_init(analog_manager_t *analog);

/**
 *  @brief Starts ADC channel sampling.
 *
 *  Starts ADC channel sampling (DMA). The fresh samples are stored
 *  at the dma_buffer of the analog manager handle. It is possible to access
 *  the buffer to see the raw data by using the corresponding analog_channel_id_t
 *  enum value as the element position.
 *
 *  @param analog Pointer to the analog manager handle.
 *
 *  @return
 *  	- ANALOG_MANAGER_OK
 *  	- ANALOG_MANAGER_ERR_NULL
 *  	- ANALOG_MANAGER_ERR_NOT_INITIALIZED
 *  	- ANALOG_MANAGER_ERR_ADC_START
 *
 */
analog_manager_status_t analog_manager_start(analog_manager_t *analog);

/**
 *  @brief Stops ADC channel sampling.
 *
 *	Stops ADC channel sampling (DMA stopped).
 *
 *  @param analog Pointer to the analog manager handle.
 *
 *  @return
 *  	- ANALOG_MANAGER_OK
 *  	- ANALOG_MANAGER_ERR_NULL
 *  	- ANALOG_MANAGER_ERR_NOT_INITIALIZED
 *  	- ANALOG_MANAGER_ERR_ADC_STOP
 *
 *  @note By stopping the ADC, the filter will not be able to get new samples.
 *  	  Thus, functions like analog_manager_update and analog_manager_get_
 *  	  filtered_voltage may not work properly, as they will keep displaying
 *  	  the same output value because there are no new values for processing.
 *
 */
analog_manager_status_t analog_manager_stop(analog_manager_t *analog);

/**
 *  @brief Updates all analog channels.
 *
 *	Reads ADC samples from the DMA buffer, converts them to voltage and updates
 *	the corresponding moving average filters.
 *
 *  @param analog Pointer to the analog manager handle.
 *
 *  @return
 *  	- ANALOG_MANAGER_OK
 *  	- ANALOG_MANAGER_ERR_NULL
 *  	- ANALOG_MANAGER_ERR_NOT_INITIALIZED
 *  	- ANALOG_MANAGER_ERR_FILTER_UPDATE
 *
 */
analog_manager_status_t analog_manager_update(analog_manager_t *analog);

/**
 *  @brief Resets a desired channel filter.
 *
 *	Resets the internal status and the buffer of a moving average filter
 *	associated to a specific analog channel.
 *
 *  @param analog Pointer to the analog manager handle.
 *  @param channel Desired channel for whose filter needs a reset.
 *
 *  @return
 *  	- ANALOG_MANAGER_OK
 *  	- ANALOG_MANAGER_ERR_NULL
 *  	- ANALOG_MANAGER_ERR_NOT_INITIALIZED
 *  	- ANALOG_MANAGER_ERR_INVALID_CHANNEL
 *  	- ANALOG_MANAGER_ERR_FILTER_RESET
 *
 */
analog_manager_status_t analog_manager_reset_filter(analog_manager_t *analog, analog_channel_id_t channel);

/**
 *  @brief Gets filtered voltage from channel.
 *
 *	Gets filtered voltage from desired channel by reading the filtered_
 *	voltage vector at the channel position. An update might be needed
 *	before reading the filtered value of the voltage to ensure that the
 *	value is current.
 *
 *  @param analog Pointer to the analog manager handle.
 *  @param channel Desired channel for reading filtered voltage.
 *
 *  @return
 *  	-1.0f if there was an error.
 *  	Filtered voltage in volts otherwise.
 *
 */
float					analog_manager_get_filtered_voltage(analog_manager_t *analog, analog_channel_id_t channel);

/**
 *  @brief Checks if a channel filter is ready for its use
 *
 *	Checks if a desired channel filter buffer is full with fresh samples
 *	to ensure the filter is ready for its use.
 *
 *  @param analog Pointer to the analog manager handle.
 *  @param channel Desired channel for checking if filter is ready.
 *
 *  @return true  if filter is ready.
 *  		false if filter is not ready or an error ocurred.
 *
 */
bool					analog_manager_is_filter_ready(analog_manager_t *analog, analog_channel_id_t channel);
