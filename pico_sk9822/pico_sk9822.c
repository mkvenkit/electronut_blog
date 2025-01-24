/*

pico_sk9822.c


Send patterns to a 12 element SK9822 LED ring (found on the Sipeed mic array board).
Uses PIO to send the signals.

electronut.in

*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// generated
#include "sk9822.pio.h"

#define PIN_CLK 14
#define PIN_DATA 15
#define NLEDS 12

// PIO 
PIO pio;
uint sm;
uint offset;

// colors
uint8_t col1[] = {255, 0, 0};
uint8_t col2[] = {255, 0, 255};
uint8_t col3[] = {0, 255, 255};
uint8_t colors[NLEDS][3];
uint32_t colIndex = 0;
uint8_t brightness = 15; // 5 bit value

// create a 32 bit frame 
uint32_t create_frame(uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue) {
    return (0b111 << 29) |               // 3 bits '111' at the MSB
           ((brightness & 0x1F) << 24) | // 5-bit brightness (masked)
           ((blue & 0xFF) << 16) |       // 8-bit blue
           ((green & 0xFF) << 8) |       // 8-bit green
           (red & 0xFF);                 // 8-bit red
}

// send start + 12 x colors + end frame 
void send_frames()
{
    // sent start frame 
    pio_sm_put_blocking(pio, sm, 0);

    // middle frames 
    for (size_t i = 0; i < NLEDS; i++) {
        uint32_t frame = create_frame(brightness, colors[i][0], colors[i][1], colors[i][2]);
        pio_sm_put_blocking(pio, sm, frame);
    }
    
    // send end frame 
    pio_sm_put_blocking(pio, sm, 0xffffffff);
}

// send an LED pattern 
void send_pattern()
{
    // fill with c3
    for (size_t i = 0; i < NLEDS; i++) {
        memcpy(colors[i], col3, 3);
    }
    // copy c1
    memcpy(colors[colIndex], col1, 3);
    // copy c2
    memcpy(colors[(colIndex + 1) % NLEDS], col2, 3);
    memcpy(colors[(colIndex + 2) % NLEDS], col2, 3);
    memcpy(colors[(colIndex - 1) % NLEDS], col2, 3);
    memcpy(colors[(colIndex - 2) % NLEDS], col2, 3);
    // incr 
    colIndex = (colIndex + 1) % NLEDS;

    // update colors
    send_frames();
}

// sk9822 loop 
void run_sk9822()
{
    // add program 
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&sk9822_program, 
        &pio, &sm, &offset, PIN_CLK, 2, true);
    hard_assert(success);

    // init program  
    sk9822_program_init(pio, sm, offset, PIN_CLK, PIN_DATA);

    // loop 
    while (true) {
        // send LED pattern
        send_pattern();
        // wait 
        sleep_ms(100);
    }

    // clean up - never reached
    pio_remove_program_and_unclaim_sm(&sk9822_program, pio, sm, offset);

}

int main()
{
    stdio_init_all();
    run_sk9822();
}
