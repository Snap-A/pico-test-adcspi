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
 ** Define the GPIO pin numbers used for accessing the Texas Instrument ADC via SPI
 **/

#define ADC_CS_GPIO   9
#define ADC_CLK_GPIO  10
#define ADC_MISO_GPIO 8
#define ADC_MOSI_GPIO 11

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

   /* CLK output, set to '1' */
   gpio_init(ADC_CLK_GPIO);
   gpio_set_dir(ADC_CLK_GPIO, GPIO_OUT);
   gpio_put(ADC_CLK_GPIO, 1);

   /* MOSI output, set to '0' */
   gpio_init(ADC_MOSI_GPIO);
   gpio_set_dir(ADC_MOSI_GPIO, GPIO_OUT);
   gpio_put(ADC_MOSI_GPIO, 0);

   /* MISO input */
   gpio_init(ADC_MISO_GPIO);
   gpio_set_dir(ADC_MISO_GPIO, GPIO_IN);

   return PICO_OK;
}

/**
 ** Write 16 control bits to chip (MOSI) to select the channel
 **/
int pico_adc_write(int chn)
{
   int cnt = 16;
   while (cnt > 0)
   {
      /* Start with falling edge */
      gpio_put(ADC_CLK_GPIO, 0);

      /* Set MOSI line state */
      if (cnt == 12)
      {
         /* Write bit channel (1) */
        gpio_put(ADC_MOSI_GPIO, (chn)?1:0);
      }
      else
      {
         /* Always '0' */
         gpio_put(ADC_MOSI_GPIO, 0);
      }
      sleep_us(SPI_HALF_CYCLE);

      /* Clock data to chip with rising edge */
      gpio_put(ADC_CLK_GPIO, 1);
      sleep_us(SPI_HALF_CYCLE);

      /* Loop counter */
      --cnt;
   }

   return PICO_OK;
}

/**
 ** Read data bits from chip (MISO) after conversion is complete
 **/
int pico_adc_read(uint8_t* out, size_t out_len)
{
   unsigned int val;
   unsigned int cnt;

   /* Clear output memory */
   for (int i=0; i < out_len; ++i)
      out[i] = 0;

   /* Read the number of expected bits: 16 for each frame. */
   cnt = 16;
   while (cnt > 0)
   {
      /* Tell chip to set MISO to next bit value */
      gpio_put(ADC_CLK_GPIO, 0);
      sleep_us(SPI_HALF_CYCLE);

      /* Read data bit from ADC and shift into output buffer */
      gpio_put(ADC_CLK_GPIO, 1);
      val = (gpio_get(ADC_MISO_GPIO)?1:0);
      for (int i=0; i < out_len-1; ++i)
      {
         out[i] = (0x7F & out[i])<<1;
         if (out[i+1] > 127) out[i]++;
      }
      out[out_len-1] = (0x7F & out[out_len-1])<<1;
      out[out_len-1] += val;

      /* Loop counter */
      --cnt;

      /* Wait for cycle */
      sleep_us(SPI_HALF_CYCLE);
   }

   return PICO_OK;
}

/**
 ** Start SPI with GPIO CS enable
 **/
int pico_adc_start(void)
{
   /* MOSI output, set to '1' */
   gpio_put(ADC_MOSI_GPIO, 1);

   /* CS output, set to '0' */
   gpio_put(ADC_CS_GPIO, 0);
   return PICO_OK;
}

/**
 ** End SPI with GPIO CS disable and CLK at '1'
 **/
int pico_adc_end(void)
{
  /* CLK output, set to '1' */
  gpio_put(ADC_CLK_GPIO, 1);

  /* CS output, set to '1' */
  gpio_put(ADC_CS_GPIO, 1);

  /* MOSI output, set to '1' */
  gpio_put(ADC_MOSI_GPIO, 1);

  return PICO_OK;
}

/**
 ** Main function to run
 **/
int main()
{
   int      rc;
   uint8_t  adc_data[3];
   uint16_t chn0_val;
   float    chn0_volt;
   uint16_t chn1_val;
   float    chn1_volt;

   float VperLSB = 3.3 / 256.0;

   bi_decl(bi_program_description("This is the ADCT082S201 binary."));
   bi_decl(bi_1pin_with_name(ADC_CS_GPIO, "ADC SPI CS"));
   bi_decl(bi_1pin_with_name(ADC_CLK_GPIO, "SPI Clock"));
   bi_decl(bi_1pin_with_name(ADC_MISO_GPIO, "SPI MISO"));
   bi_decl(bi_1pin_with_name(ADC_MOSI_GPIO, "SPI MOSI"));

   stdio_init_all();

   rc = pico_adc_init();
   hard_assert(rc == PICO_OK);

   while(true)
   {
      /* Temporary tests */
      rc = pico_adc_start();

      rc = pico_adc_write(0); /* Select first channel */
      rc = pico_adc_read(adc_data, 3); /* Read two 8-bit data */
      /* Extract MSB bit pattern */
      chn0_val = adc_data[0] & 0xF0;
      chn0_val += (adc_data[0] & 0x0F) * 256;

      rc = pico_adc_write(1); /* Select second channel */
      rc = pico_adc_read(adc_data, 3); /* Read two 8-bit data */
      /* Extract MSB bit pattern */
      chn1_val = adc_data[0] & 0xF0;
      chn1_val += (adc_data[0] & 0x0F) * 256;
      
      rc = pico_adc_end();

      /* Shift out to correct resolution (max 12 bit -> real 8 bit) */
      chn0_val >>= 4;
      chn1_val >>= 4;

      /* Convert to voltage */
      chn0_volt = 0.0;
      if (chn0_val > 0)
      {
         chn0_volt = (VperLSB/2.0) + VperLSB * chn0_val;
      }
      chn1_volt = 0.0;
      if (chn1_val > 0)
      {
         chn1_volt = (VperLSB/2.0) + VperLSB * chn0_val;
      }

      /* Print and sleep */
      printf("Data[0]: %02x -> %g V\n", chn0_val, chn0_volt);
      printf("Data[1]: %02x -> %g V\n", chn1_val, chn1_volt);
      sleep_ms(1000);
   }
}
