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

/* Set clock rate to 100 kHz */
#define SPI_HALF_CYCLE 5

/**
 ** Perform GPIO initialisation for SPI
 **/
int pico_adc_init(void)
{
   /* CS output, set to '1' */
   gpio_init(ADC_CS_GPIO);
   gpio_set_dir(ADC_CS_GPIO, GPIO_OUT);
   gpio_put(ADC_CS_GPIO, 1);

   /* CLK output, set to '0' */
   gpio_init(ADC_CLK_GPIO);
   gpio_set_dir(ADC_CLK_GPIO, GPIO_OUT);
   gpio_put(ADC_CLK_GPIO, 0);

   /* DATA disabled */
   gpio_deinit(ADC_DATA_GPIO);

   return PICO_OK;
}

/**
 ** Write control bits to SPI and select the channel
 **/
int pico_adc_write(int chn)
{
   /* Write bit START (1) */
   gpio_put(ADC_DATA_GPIO, 1);
   sleep_us(SPI_HALF_CYCLE);
   gpio_put(ADC_CLK_GPIO, 1);
   sleep_us(SPI_HALF_CYCLE);
   gpio_put(ADC_CLK_GPIO, 0);

   /* Write bit SGL (1) */
   gpio_put(ADC_DATA_GPIO, 1);
   sleep_us(SPI_HALF_CYCLE);
   gpio_put(ADC_CLK_GPIO, 1);
   sleep_us(SPI_HALF_CYCLE);
   gpio_put(ADC_CLK_GPIO, 0);

   /* Write bit ODD (chn) */
   gpio_put(ADC_DATA_GPIO, (chn==0)?0:1);
   sleep_us(SPI_HALF_CYCLE);
   gpio_put(ADC_CLK_GPIO, 1);
   sleep_us(SPI_HALF_CYCLE);

   return PICO_OK;
}

/**
 ** Read data bits from SPI after conversion is complete
 **/
int pico_adc_read(uint8_t* out, size_t out_len)
{
   unsigned int val;
   unsigned int cnt;

   /* Clear output memory */
   for (int i=0; i < out_len; ++i)
      out[i] = 0;

   /* DATA switch to input */
   gpio_set_dir(ADC_DATA_GPIO, GPIO_IN);

   /* Read the number of expected bits: First is the 'start' bit. 
    * Then we expect MSB-ordered byte data followed by MSL-ordered byte
    * data of the same value. Then the 'stop' bit should follow
    */
   cnt = 18;
   while (cnt > 0)
   {
      gpio_put(ADC_CLK_GPIO, 0);
      sleep_us(SPI_HALF_CYCLE);

      /* Read data bit from ADC and shift into output buffer */
      val = (gpio_get(ADC_DATA_GPIO)?1:0);
      for (int i=0; i < out_len-1; ++i)
      {
         out[i] = (0x7F & out[i])<<1;
         if (out[i+1] > 127) out[i]++;
      }
      out[out_len-1] = (0x7F & out[out_len-1])<<1;
      out[out_len-1] += val;

      /* Prepare bus for next bit */
      gpio_put(ADC_CLK_GPIO, 1);
      sleep_us(SPI_HALF_CYCLE);

      cnt--;
   }

   /* Settle clock to 0 */
   gpio_put(ADC_CLK_GPIO, 0);
   return PICO_OK;
}

/**
 ** Start SPI with GPIO CS enable and DATA used as output
 **/
int pico_adc_start(void)
{
   /* DATA output at first, set to '1' */
   gpio_init(ADC_DATA_GPIO);
   gpio_set_dir(ADC_DATA_GPIO, GPIO_OUT);
   gpio_put(ADC_DATA_GPIO, 1);

   /* CS output, set to '0' */
   gpio_put(ADC_CS_GPIO, 0);
   return PICO_OK;
}

/**
 ** End SPI with GPIO CS disable
 **/
int pico_adc_end(void)
{
   /* CS output, set to '1' */
   gpio_put(ADC_CS_GPIO, 1);

   /* DATA disabled */
   gpio_deinit(ADC_DATA_GPIO);

   return -1;
}

/**
 ** Main function to run
 **/
int main()
{
   int rc;
   uint8_t adc_data[3];
   uint8_t chn0_val;
   float   chn0_volt;

   float VperLSB = 3.3 / 256.0;

   bi_decl(bi_program_description("This is the ADC0832 binary."));
   bi_decl(bi_1pin_with_name(ADC_CS_GPIO, "ADC SPI CS"));
   bi_decl(bi_1pin_with_name(ADC_CLK_GPIO, "ADC SPI Clock"));
   bi_decl(bi_1pin_with_name(ADC_DATA_GPIO, "ADC SPI Data"));

   stdio_init_all();

   rc = pico_adc_init();
   hard_assert(rc == PICO_OK);

   while(true)
   {
      /* Temporary tests */
      rc = pico_adc_start();

      rc = pico_adc_write(0); /* Select first channel */
      rc = pico_adc_read(adc_data, 3); /* Read two 8-bit data */

      rc = pico_adc_end();

      /* Extract MSB bit pattern */
      chn0_val = (0x01 & adc_data[0])?128:0;
      chn0_val += (adc_data[1] >> 1);

      /* Convert to voltage */
      chn0_volt = 0.0;
      if (chn0_val > 0)
      {
         chn0_volt = (VperLSB/2.0) + VperLSB * chn0_val;
      }

      /* Print and sleep */
      printf("Data: %02x -> %g V\n", chn0_val, chn0_volt);
      sleep_ms(1000);
   }
}
