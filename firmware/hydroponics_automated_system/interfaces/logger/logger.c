#include "logger.h"
#include "fatfs/ff.h"

/*Defines*/

/*Private variables*/
static volatile uint8_t FatFsCnt = 0;
static volatile uint8_t Timer1, Timer2;
static FATFS fs;
static FIL fil;
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
logger_err_t logger_init(void){
	res = f_mount(&fs, "", 1);
	return (res != FR_OK) ? LOGGER_ERR_NOT_INITIALIZED : LOGGER_OK;
}

logger_err_t logger_save_log(logger_data_t *data){

}
