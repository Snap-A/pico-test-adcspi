/********************************************************************
 **
 **
 ********************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

/**
 ** Pico W devices use a GPIO on the WIFI chip for the LED,
 ** so when building for Pico W, CYW43_WL_GPIO_LED_PIN will be defined
 **/
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/**
 ** Define the GPIO pin numbers used for accessing the Adeept ADC via SPI
 **
 ** NOTE: The ADC places the DI and DO pins of the chip on the SAME connector pin!
 **       This means a single data line is present and the MOSI/MISO feature of the
 **       standard SPI driver logic cannot be used.
 **
 **       Instead, the single line has to switch from OUT/Z-state to IN in the middle of
 **       the transaction.
 **/

#define ADC_CS_GPIO   17
#define ADC_CLK_GPIO  18
#define ADC_DATA_GPIO 19

/**
 ** Perform GPIO initialisation for SPI
 **/
int pico_adc_init(void)
{
   return -1;
}

/**
 ** Start SPI with GPIO CS enable
 **/
int pico_adc_start(void)
{
   return -1;
}

/**
 ** End SPI with GPIO CS disable
 **/
int pico_adc_end(void)
{
   return -1;
}

/**
 ** Write control bits to SPI and select the channel
 **/
int pico_adc_write(int chn)
{
   return -1;
}

/**
 ** Read data bits from SPI after conversion
 **/
int pico_adc_read(uint8_t* out, size_t out_len)
{
   return -1;
}

int main()
{
   int rc;
   uint8_t adc_data[16];

   bi_decl(bi_program_description("This is the ADC0832 binary."));
   bi_decl(bi_1pin_with_name(ADC_CS_GPIO, "ADC SPI CS"));
   bi_decl(bi_1pin_with_name(ADC_CLK_GPIO, "ADC SPI Clock"));
   bi_decl(bi_1pin_with_name(ADC_DATA_GPIO, "ADC SPI Data"));

   stdio_init_all();

   rc = pico_adc_init();
   hard_assert(rc == PICO_OK);

   while(true)
   {
      rc = pico_adc_start();
      rc = pico_adc_end();
      printf("Data: %d\n", adc_data);
      sleep_ms(1000);
   }
}
