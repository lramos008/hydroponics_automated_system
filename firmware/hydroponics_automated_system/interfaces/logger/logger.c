#include "logger.h"
#include <stddef.h>
#include "fatfs/ff.h"

/*Defines*/
#define LOGGER_MAX_LOGS 16								//Data occuppies 32 bytes, so it's multiple of 512, then 16 logs fits in a block write
#define LOGGER_DATA_FILE_PATH  "data.bin"
#define LOGGER_EVENT_FILE_PATH "events.bin"

/*Private enums*/
typedef enum{
	LOGGER_OPEN_APPEND,
	LOGGER_OPEN_READ
}logger_open_t;

/*Private structs*/
typedef struct{
	logger_data_t logs[LOGGER_MAX_LOGS];
	size_t logs_idx;
	bool is_data_file_opened;
	bool is_event_file_opened;
}logger_internal_state_t;

/*Private variables*/
//Internal buffer
static logger_internal_state_t logger_buf = {0};

//Fatfs internal state variables
volatile uint8_t FatFsCnt = 0;
volatile uint8_t Timer1, Timer2;
static FATFS fs;
static FIL data_file;
static FIL event_file;
static FRESULT res;
static UINT bw, br;

/*Private functions*/
static void SDTimer_Handler(void)
{
  if(Timer1 > 0)
    Timer1--;

  if(Timer2 > 0)
    Timer2--;
}

static void logger_queue_log_data(logger_data_t *data){
	//Save data on internal buffer
	logger_buf.logs[logger_buf.logs_idx].environment_temp = data->environment_temp;
	logger_buf.logs[logger_buf.logs_idx].environment_hum = data->environment_hum;
	logger_buf.logs[logger_buf.logs_idx].environment_lux = data->environment_lux;
	logger_buf.logs[logger_buf.logs_idx].solution_temp = data->solution_temp;
	logger_buf.logs[logger_buf.logs_idx].solution_ph = data->solution_ph;
	logger_buf.logs[logger_buf.logs_idx].solution_ec = data->solution_ec;
	logger_buf.logs[logger_buf.logs_idx].is_solution_level_low = data->is_solution_level_low;
	logger_buf.logs[logger_buf.logs_idx].is_solution_level_low = data->is_solution_level_low;
	logger_buf.logs[logger_buf.logs_idx].reserved[0] = data->reserved[0];
	logger_buf.logs[logger_buf.logs_idx].reserved[1] = data->reserved[1];
	logger_buf.logs[logger_buf.logs_idx].reserved[2] = data->reserved[2];
	logger_buf.logs[logger_buf.logs_idx].timestamp = data->timestamp;
	logger_buf.logs_idx++;
}

static logger_err_t logger_data_open_read(void){
	res = f_open(&data_file, LOGGER_DATA_FILE_PATH, FA_READ);
	return (res != FR_OK) ? LOGGER_ERR_OPEN : LOGGER_OK;
}

static logger_err_t logger_data_open_append(void){
	res = f_open(&data_file, LOGGER_DATA_FILE_PATH, FA_OPEN_APPEND | FA_WRITE);
	return (res != FR_OK) ? LOGGER_ERR_OPEN : LOGGER_OK;
}

static logger_err_t logger_data_close(void){
	res = f_close(&data_file);
	return (res != FR_OK) ? LOGGER_ERR_CLOSE : LOGGER_OK;
}

static logger_err_t logger_events_open_append(void){
	res = f_open(&event_file, LOGGER_EVENT_FILE_PATH, FA_OPEN_APPEND | FA_WRITE);
	return (res != FR_OK) ? LOGGER_ERR_OPEN : LOGGER_OK;
}

static logger_err_t logger_events_close(void){
	res = f_close(&event_file);
	return (res != FR_OK) ? LOGGER_ERR_CLOSE : LOGGER_OK;
}


static logger_err_t logger_data_write_to_sd(void){
	//Write data into file
	size_t btw = logger_buf.logs_idx * sizeof(logger_data_t);
	res = f_write(&data_file, (uint8_t *) logger_buf.logs, btw, &bw);
	if(res != FR_OK || bw != btw){
		return LOGGER_ERR_WRITE;
	}

	//Sync SD in order to avoid losing data
	res = f_sync(&data_file);
	if(res != FR_OK){
		return LOGGER_ERR_SYNC;
	}

	//Reset logs index from internal buffer
	logger_buf.logs_idx = 0;
	return LOGGER_OK;
}

static logger_err_t logger_data_get_log_count(uint32_t *log_count){
	//Sanity check
	if(!log_count) return LOGGER_ERR_NULL;

	//Get file size
	uint32_t size = f_size(&data_file);
	*log_count    = size / sizeof(logger_data_t);
	return LOGGER_OK;
}

/*Freertos Tick Hook to handle SD card*/
void vApplicationTickHook(void)
{
    static uint8_t FatFsCnt = 0;

    FatFsCnt++;
    if (FatFsCnt >= 10)
    {
        FatFsCnt = 0;
        SDTimer_Handler();
    }
}

DWORD get_fattime(void)
{
    return 0;
}

/*Public functions*/
logger_err_t logger_start(void){
	res = f_mount(&fs, "", 1);
	return (res != FR_OK) ? LOGGER_ERR_MOUNT : LOGGER_OK;
}


logger_err_t logger_stop(void){
	res  = f_mount(NULL, "", 0);
	return (res != FR_OK) ? LOGGER_ERR_UNMOUNT : LOGGER_OK;
}

logger_err_t logger_log_data(logger_data_t *data){
	if(logger_buf.logs_idx < LOGGER_MAX_LOGS){
		logger_queue_log_data(data);
		return LOGGER_OK;
	}
	return LOGGER_BUFFER_FULL;
}

logger_err_t logger_flush(void){
	if(logger_buf.logs_idx == 0) return LOGGER_BUFFER_EMPTY;

	//Write logs into SD card
	logger_err_t err;
	err = logger_data_open_append();
	if(err != LOGGER_OK) return err;

	err = logger_data_write_to_sd();
	if(err != LOGGER_OK) return err;

	err = logger_data_close();
	return err;
}

logger_err_t logger_read_last_n_logs(uint32_t n, logger_data_t *buffer, uint32_t *read_count){
	//Sanity check
	if(!buffer || !read_count){
		return LOGGER_ERR_NULL;
	}

	//Get log count
	logger_err_t err;
	uint32_t log_count;


	err = logger_data_open_read();
	err = logger_data_get_log_count(&log_count);
	*read_count = (n > log_count) ? log_count : n;

	//Get offset
	uint32_t start = log_count - n;
	uint32_t offset = start * sizeof(logger_data_t);
	f_lseek(&data_file, offset);

	//Read logs
	if(f_read(&data_file, (uint8_t *) buffer, (*read_count) * sizeof(logger_data_t), &br) != FR_OK){
		logger_data_close();
		return LOGGER_ERR_READ;
	}

	err = logger_data_close();
	return LOGGER_OK;
}
