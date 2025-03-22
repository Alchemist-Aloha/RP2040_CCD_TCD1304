#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
// For ADC input:
#include "hardware/adc.h"
#include "hardware/dma.h"
// For resistor DAC output:
#include "pico/multicore.h"
// TCD1304 pinout
#define PWM_PIN 14
#define PWM_TEST_PIN 7
#define SH_PIN 9
#define ICG_PIN 13
#define ADC_PIN 26

#define PRINT_INTERVAL 30   // Print a line with 30 pixels
#define CPU_FREQ_KHZ 200000 // CPU 200000 kHz
#define CAPTURE_CHANNEL 0   // Channel 0 is GPIO26
#define CAPTURE_DEPTH 3694  // 3694 samples including the dummy pixel by datasheet
#define SH_PULSE_ON 20      // pulse on and off time (us)
#define SH_PULSE_OFF 20     // pulse on and off time (us)
#define SIGNAL_AVERAGE 10   // average the signal

const int SH_PULSE_COUNT = CAPTURE_DEPTH * 2 / (SH_PULSE_ON + SH_PULSE_OFF); // SH pulse cycle matches the readout time. The single cycle duration defines the exposure time.
// PWM configuration
void setup_pwm_mc(uint slice_num, uint channel)
{
    // Set the wrap value to generate 2 MHz frequency
    uint32_t wrap_value = CPU_FREQ_KHZ / 2000;              // 2 MHz PWM frequency. wrap_value = (sys_clock / PWM frequency)-1
    pwm_set_wrap(slice_num, wrap_value);                    // Set the wrap value (16-bit)
    pwm_set_chan_level(slice_num, channel, wrap_value / 2); // Set duty cycle (50%)
    pwm_set_enabled(slice_num, true);                       // Enable PWM output
}

// PWM configuration of SH. Unused in this version
void setup_pwm_sh(uint slice_num, uint channel)
{
    // Set the wrap value to generate 100 kHz frequency
    uint32_t wrap_value = CPU_FREQ_KHZ / 100;                 // 2 MHz SH frequency. wrap_value = (sys_clock / PWM frequency)-1
    pwm_set_wrap(slice_num, wrap_value);                      // Set the wrap value (16-bit)
    pwm_set_chan_level(slice_num, channel, wrap_value * 0.4); // Set duty cycle (50%)
    pwm_set_enabled(slice_num, true);                         // Enable PWM output
}
// PWM test pin configuration. Unused in this version
void setup_pwm2(uint slice_num, uint channel)
{
    // Set the wrap value to generate 100 kHz frequency
    uint32_t wrap_value = CPU_FREQ_KHZ / 5;                 // 5 kHz frequency for diagnose use. wrap_value = (sys_clock / PWM frequency)-1
    pwm_set_wrap(slice_num, wrap_value);                    // Set the wrap value (16-bit)
    pwm_set_chan_level(slice_num, channel, wrap_value / 2); // Set duty cycle (50%)
    pwm_set_enabled(slice_num, true);                       // Enable PWM output
}

// dma read from adc fifo
void dma_adc_read(uint dma_chan, uint16_t *capture_buf, dma_channel_config cfg)
{
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_channel_configure(dma_chan, &cfg,
                          capture_buf,   // dst
                          &adc_hw->fifo, // src
                          CAPTURE_DEPTH, // transfer count
                          true           // start immediately
    );

    // printf("Starting capture\n");
    adc_run(true);

    // Once DMA finishes, stop any new conversions from starting, and clean up
    // the FIFO in case the ADC was still mid-conversion.
    dma_channel_wait_for_finish_blocking(dma_chan);
    printf("\n\rCapture finished\n\r");
    adc_run(false);
    adc_fifo_drain();
}

// initiate CCD readout. Unused in this version
void start_ccd_readout()
{
    // Control SH and ICG pins
    gpio_put(ICG_PIN, 0); // without 74hc04
    // delay before exposure for 100 cpu cycles (~430 ns)
    __asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
    gpio_put(SH_PIN, 1); // without 74hc04
    // SH pulse width
    busy_wait_us_32(4);
    gpio_put(SH_PIN, 0); // without 74hc04
    // delay after exposure
    busy_wait_us_32(6);
    gpio_put(ICG_PIN, 1); // without 74hc04
}

// Control SH pin with a given count and delays. Can be used to control the exposure time.
void control_sh_pin(int count, int high_delay, int low_delay)
{
    for (int i = 0; i < count; ++i)
    {
        gpio_put(SH_PIN, 1);
        busy_wait_us_32(high_delay);
        gpio_put(SH_PIN, 0);
        busy_wait_us_32(low_delay);
    }
}

// Print the capture buffer. This function prints the captured data in lines with depth number of int8 pixel readings.
void print_capture_buffer(uint16_t *buffer, int depth)
{
    for (int i = 0; i < depth; ++i)
    {
        printf("%-3d, ", buffer[i]);
        if (i % PRINT_INTERVAL == PRINT_INTERVAL - 1)
            printf("\n\r");
    }
}

int main()
{
    // Set system clock frequency
    set_sys_clock_khz(CPU_FREQ_KHZ, true);
    stdio_init_all();

    // Initialize PWM on the specified pin
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    uint channel = pwm_gpio_to_channel(PWM_PIN);
    setup_pwm_mc(slice_num, channel);

    // // Initialize PWM test on the specified pin
    gpio_set_function(PWM_TEST_PIN, GPIO_FUNC_PWM);
    uint slice_num_test = pwm_gpio_to_slice_num(PWM_TEST_PIN);
    uint channel_test = pwm_gpio_to_channel(PWM_TEST_PIN);
    setup_pwm2(slice_num_test, channel_test);

    // // Initialize GPIO for SH
    gpio_init(SH_PIN);
    gpio_set_dir(SH_PIN, GPIO_OUT);
    gpio_put(SH_PIN, 0); // without 74hc04

    // Initialize GPIO for ICG
    gpio_init(ICG_PIN);
    gpio_set_dir(ICG_PIN, GPIO_OUT);
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
        false   // Shift each sample to 8 bits when pushing to FIFO
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
    adc_set_clkdiv(0);

    printf("Arming DMA\n");
    sleep_ms(1000);
    // Set up the DMA to start transferring data as soon as it appears in FIFO
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);

    uint16_t capture_buf[CAPTURE_DEPTH];
    uint16_t capture_buf_sum[CAPTURE_DEPTH];
    while (true)
    {
        // Clear the capture buffer
        memset(capture_buf_sum, 0, sizeof(capture_buf));
        for (int i = 0; i < SIGNAL_AVERAGE; ++i)
        {
            memset(capture_buf, 0, sizeof(capture_buf));
            control_sh_pin(5, SH_PULSE_ON, SH_PULSE_OFF);
            adc_run(true);
            // Control SH and ICG pins
            gpio_put(ICG_PIN, 0); // without 74hc04
            // delay before exposure for 60 cpu cycles (~300 ns)
            __asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
            __asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
            gpio_put(SH_PIN, 1);
            busy_wait_us_32(SH_PULSE_ON);
            gpio_put(SH_PIN, 0);
            busy_wait_us_32(5); // ICG pulse delay t1
            gpio_put(ICG_PIN, 1); // without 74hc04
            busy_wait_us_32(SH_PULSE_OFF - 5); 
            // control_sh_pin(1000, 2, 8);
            // adc_run(true);
            dma_channel_configure(dma_chan, &cfg,
                                  capture_buf,   // dst
                                  &adc_hw->fifo, // src
                                  CAPTURE_DEPTH, // transfer count
                                  true           // start immediately
            );

            control_sh_pin(SH_PULSE_COUNT, SH_PULSE_ON, SH_PULSE_OFF);

            // finish adc run and dma read
            dma_channel_wait_for_finish_blocking(dma_chan);
            // printf("\n\rCapture finished\n\r");
            adc_run(false);
            adc_fifo_drain();
            // Add the captured data to the sum buffer
            for (int j = 0; j < CAPTURE_DEPTH; ++j)
            {
                capture_buf_sum[j] = (int16_t)((int32_t)capture_buf[j] + (int16_t)capture_buf_sum[j]) / 2;
            }
        }
        printf("\n\rCapture finished\n\r");
        print_capture_buffer(capture_buf_sum, CAPTURE_DEPTH);
        sleep_ms(30);
    }
}