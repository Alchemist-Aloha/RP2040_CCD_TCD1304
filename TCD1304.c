#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
// For ADC input:
#include "hardware/adc.h"
#include "hardware/dma.h"
// For resistor DAC output:
#include "pico/multicore.h"
// TCD1304 pinout
#define PWM_PIN 14
#define SH_PIN 15
#define ICG_PIN 13
#define ADC_PIN 26

// PWM configuration
void setup_pwm(uint slice_num, uint channel)
{
    // Set the wrap value to generate 2 MHz frequency
    uint32_t wrap_value = 114;                              // Based on 230 MHz system clock and 2 MHz frequency. wrap_value = (sys_clock / PWM frequency)-1
    pwm_set_wrap(slice_num, wrap_value);                    // Set the wrap value (16-bit)
    pwm_set_chan_level(slice_num, channel, wrap_value / 2); // Set duty cycle (50%)
    pwm_set_enabled(slice_num, true);                       // Enable PWM output
}

// Channel 0 is GPIO26
#define CAPTURE_CHANNEL 0
#define CAPTURE_DEPTH 65536

int main()
{
    // Set system clock frequency
    set_sys_clock_khz(230000, true);
    stdio_init_all();

    // Initialize PWM on the specified pin
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    uint channel = pwm_gpio_to_channel(PWM_PIN);
    setup_pwm(slice_num, channel);

    // Initialize GPIO for SH and ICG
    gpio_init(SH_PIN);
    gpio_set_dir(SH_PIN, GPIO_OUT);
    // gpio_put(SH_PIN, 1); // with 74hc04
    gpio_put(SH_PIN, 0); // without 74hc04

    gpio_init(ICG_PIN);
    gpio_set_dir(ICG_PIN, GPIO_OUT);
    // gpio_put(ICG_PIN, 0); // with 74hc04
    gpio_put(ICG_PIN, 1); // without 74hc04

    // Init GPIO for analogue use: hi-Z, no pulls, disable digital input buffer.
    adc_gpio_init(ADC_PIN + CAPTURE_CHANNEL);

    adc_init();
    adc_select_input(CAPTURE_CHANNEL);
    adc_fifo_setup(
        true,  // Write each completed conversion to the sample FIFO
        true,  // Enable DMA data request (DREQ)
        1,     // DREQ (and IRQ) asserted when at least 1 sample present
        false, // We won't see the ERR bit because of 8 bit reads; disable.
        true   // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(96);

    printf("Arming DMA\n");
    sleep_ms(1000);
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    uint8_t capture_buf[CAPTURE_DEPTH];

    while (true)
    {

        // Control SH and ICG pins
        // gpio_put(ICG_PIN, 1); // with 74hc04
        gpio_put(ICG_PIN, 0); // without 74hc04
        // delay before exposure for 100 cpu cycles (~430 ns)
        __asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
        // gpio_put(SH_PIN, 0); // with 74hc04
        gpio_put(SH_PIN, 1); // without 74hc04
        // exposure time
        sleep_ms(500);
        // gpio_put(SH_PIN, 1); // with 74hc04
        gpio_put(SH_PIN, 0); // without 74hc04
        // delay after exposure
        busy_wait_us_32(5);
        // gpio_put(ICG_PIN, 0); // with 74hc04
        gpio_put(ICG_PIN, 1); // without 74hc04

        // busy_wait_us_32(60);

        dma_adc_read(dma_chan, capture_buf, cfg);

        // Print samples to stdout so you can display them in pyplot, excel, matlab
        for (int i = 0; i < CAPTURE_DEPTH; ++i)
        {
            printf("%-3d, ", capture_buf[i]);
            if (i % 30 == 29)
                printf("\n");
            // busy_wait_us_32(1);
        }
    }
}
