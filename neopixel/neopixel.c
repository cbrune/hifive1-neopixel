/*
 * Copyright 2018 Curt Brune <curt@brune.net>
 * All rights reserved
 *
 * Portions of this code and inspiration comes from the SiFive
 * Freedom-e-SDK.
 * Copyright 2016 SiFive, Inc.
 */

/* This program demonstrates driving Neopixel LEDs using a HiFive1
 * board from SiFive.  This board contains a SiFive Freedom E310
 * (FE310) microcontroller.
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "platform.h"
#include "utility.h"

#define NEO_GPIO (PIN_8_OFFSET)
#define NEO_GPIO_MASK (1 << NEO_GPIO)
static inline volatile uint64_t get_timer() {
    return *((volatile uint64_t*)(CLINT_CTRL_ADDR + CLINT_MTIME));
}

/**
 * 1 timer tick is worth (1 / timer_freq) seconds or (1000000 /
 * timer_freq) useconds.
 */
static void inline _nsleep(uint32_t nsec) __attribute__ ((unused));
static void inline _nsleep(uint32_t nsec) {
    volatile uint32_t time = nsec / 40;
    if (time == 0)
        time = 1;
    while(time-- > 0) {}
}

static void _sleep(uint32_t sec) __attribute__ ((unused));
static void _sleep(uint32_t sec) {
    while(sec-- > 0)
        _nsleep(1000000000);
}

static uint32_t gpio_on;
static uint32_t gpio_off;

/*
 * These are the timing constraints taken mostly from the WS2812
 * datasheets. These are chosen to be conservative and avoid problems
 * rather than for maximum throughput.
 */
#define T1H  700    // Width of a 1 bit in ns
#define T1L  600    // Width of a 1 bit in ns
#define T0H  400    // Width of a 0 bit in ns
#define T0L  900    // Width of a 0 bit in ns
#define RES 7000    // Width of the low gap between bits to cause a frame to latch

static int dcount = 100;

static inline void send_bit(uint8_t bit) {
    if (bit) {
        GPIO_REG(GPIO_OUTPUT_VAL) = gpio_on;
        _nsleep(T1H - 70);
        GPIO_REG(GPIO_OUTPUT_VAL) = gpio_off;
        _nsleep(T1L - 150);
    } else {
        GPIO_REG(GPIO_OUTPUT_VAL) = gpio_on;
        _nsleep(T0H - 100);
        GPIO_REG(GPIO_OUTPUT_VAL) = gpio_off;
        _nsleep(T0L - 100);
    }
}

static inline void send_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++)
        send_bit((byte & (0x1 << (7 - i))) ? 1 : 0);
}

static inline void send_pixel(uint8_t r, uint8_t g, uint8_t b) {
    send_byte(g);
    send_byte(r);
    send_byte(b);
}

static inline void send_pixel_scale(uint16_t r, uint16_t g, uint16_t b, uint8_t scale) __attribute__ ((unused));
static inline void send_pixel_scale(uint16_t r, uint16_t g, uint16_t b, uint8_t scale) {
    uint16_t rs, gs, bs;
    if (scale == 0) {
        send_pixel(0, 0, 0);
    } else {
        rs = (r * scale)/255;
        if ((rs == 0) && (r > 0))
            rs = 1;
        gs = (g * scale)/255;
        if ((gs == 0) && (g > 0))
            gs = 1;
        bs = (b * scale)/255;
        if ((bs == 0) && (b > 0))
            bs = 1;
        send_pixel(rs, gs, bs);
    }
}

static inline void latch() {
    GPIO_REG(GPIO_OUTPUT_VAL) = gpio_off;
    _nsleep(RES);
}

#define N_LEDS  (60)

/**
 * set_all() -- set all the LEDs to the specified color
 * @r, @g, @b -- red, green, blue color values (0-255)
 * @show -- if true, latch the colors to the LEDs
 */
static void set_all(uint8_t r, uint8_t g, uint8_t b, bool show) __attribute__ ((unused));
static void set_all(uint8_t r, uint8_t g, uint8_t b, bool show) {
    for (int i = 0 ; i < N_LEDS; i++)
        send_pixel(r,g,b);
    if (show)
        latch();
}

/**
 * set_all_scale() -- set all the LEDs to the specified color with scaling
 * @r, @g, @b -- red, green, blue color values (0-255)
 * @show -- if true, latch the colors to the LEDs
 * @scale -- scale factor (0-255) apply to each RGB value
 *
 * @scale is used to tone down the brightness.  A value of 0 results
 * in no intensity (completely off) and a value of 255 results in no
 * itensity change.
 */
static void set_all_scale(uint8_t r, uint8_t g, uint8_t b, uint8_t scale) __attribute__ ((unused));
static void set_all_scale(uint8_t r, uint8_t g, uint8_t b, uint8_t scale) {
    for (int i = 0 ; i < N_LEDS; i++)
        send_pixel_scale(r, g, b, scale);
}

/**
 * clear_all() -- turn off all the LEDs
 */
static void clear_all(bool show) __attribute__ ((unused));
static void clear_all(bool show) {
    set_all(0, 0, 0, show);
}

/**
 * pulse_all() -- set all LEDs to RGB color and vary intensity
 *
 * Set all the LEDs to a specific color and vary the intensity over
 * time.  This creates a pulsing effect.  Larger values of @delay
 * result in a slow pulse.
 */
static void pulse_all(uint8_t r, uint8_t g, uint8_t b, uint32_t delay) __attribute__ ((unused));
static void pulse_all(uint8_t r, uint8_t g, uint8_t b, uint32_t delay) {
    for (int i = 30 ; i < 226; i++) {
        set_all_scale(r, g, b, i);
        _nsleep(delay);
    }
    for (int i = 1 ; i < 196; i++) {
        set_all_scale(r, g, b, 226 - i);
        _nsleep(delay);
    }
}

/**
 * send_dot() -- bounce a dot of color down the LED strip
 * @dot_r, @dot_g, @dot_b -- RGB of dot to send
 * @bg_r, @bg_g, @bg_b -- RGB all LEDs, except the dot
 * @delay - control speed of dot traversal
 *
 * Send a dot of color down the LED strip and back.
 */
static void send_dot(uint8_t dot_r, uint8_t dot_g, uint8_t dot_b,
                     uint8_t bg_r, uint8_t bg_g, uint8_t bg_b,
                     uint32_t delay) __attribute__ ((unused));
static void send_dot(uint8_t dot_r, uint8_t dot_g, uint8_t dot_b,
                     uint8_t bg_r, uint8_t bg_g, uint8_t bg_b,
                     uint32_t delay) {
    for (int j = 0 ; j < N_LEDS ; j++) {
        for (int k = 0 ; k < j ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        send_pixel_scale(dot_r, dot_g, dot_b, 255);
        for (int k = j + 1 ; k < N_LEDS ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        _nsleep(delay);
    }
    // then reverse directions
    for (int j = 0 ; j < N_LEDS ; j++) {
        for (int k = j + 1 ; k < N_LEDS ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        send_pixel_scale(dot_r, dot_g, dot_b, 255);
        for (int k = 0 ; k < j ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        _nsleep(delay);
    }
}

/**
 * send_dot2() -- bounce two dots of color in opposite directions
 * @dot_r, @dot_g, @dot_b -- RGB of the dots to send
 * @bg_r, @bg_g, @bg_b -- RGB all LEDs, except the dot
 * @delay - control speed of dot traversal
 *
 * Sends two dots of color down the LED strip from oppositing
 * directions, crossing in the middle.
 */
static void send_dot2(uint8_t dot_r, uint8_t dot_g, uint8_t dot_b,
                     uint8_t bg_r, uint8_t bg_g, uint8_t bg_b,
                     uint32_t delay) __attribute__ ((unused));
static void send_dot2(uint8_t dot_r, uint8_t dot_g, uint8_t dot_b,
                     uint8_t bg_r, uint8_t bg_g, uint8_t bg_b,
                     uint32_t delay) {
    for (int j = 0 ; j < N_LEDS / 2 ; j++) {
        for (int k = 0 ; k < j ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        send_pixel_scale(dot_r, dot_g, dot_b, 255);
        for (int k = j + 1 ; k < N_LEDS - j ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        send_pixel_scale(dot_r, dot_g, dot_b, 255);
        for (int k = N_LEDS - j; k < N_LEDS; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        _nsleep(delay);
    }
    for (int j = (N_LEDS / 2) - 1; j >= 0; j--) {
        for (int k = 0 ; k < j ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        send_pixel_scale(dot_r, dot_g, dot_b, 255);
        for (int k = j + 1 ; k < N_LEDS - j ; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        send_pixel_scale(dot_r, dot_g, dot_b, 255);
        for (int k = N_LEDS - j; k < N_LEDS; k++) {
            send_pixel_scale(bg_r, bg_g, bg_b, 64);
        }
        _nsleep(delay);
    }
}

#define CSTEP (28)
static void send_rainbow_pixel(int idx) {
    int r, g, b;

    idx = idx % N_LEDS;
    if (idx < 10) {
        r = 255;
        b = 0;
        g = (idx * CSTEP) > 255 ? 255 : idx * CSTEP;
    }
    else if (idx < 20) {
        g = 255;
        b = 0;
        r = ((255 - (idx-10)*CSTEP) < 0) ? 0 : (255 - (idx-10)*CSTEP);
    }
    else if (idx < 30) {
        r = 0;
        g = 255;
        b = ((idx-20) * CSTEP) > 255 ? 255 : (idx-20) * CSTEP;
    }
    else if (idx < 40) {
        r = 0;
        b = 255;
        g = ((255 - (idx-30)*CSTEP) < 0) ? 0 : (255 - (idx-30)*CSTEP);
    }
    else if (idx < 50) {
        b = 255;
        g = 0;
        r = ((idx-40) * CSTEP) > 255 ? 255 : (idx-40) * CSTEP;
    }
    else if (idx < 60) {
        g = 0;
        r = 255;
        b = ((255 - (idx-50)*CSTEP) < 0) ? 0 : (255 - (idx-50)*CSTEP);
    }
    send_pixel_scale(r, g, b, 32);
}

/**
 * send_rainbow() -- annimate a rainbow of color
 *
 * Creates a rainbow of color that cycles across the LED strip.
 */
static void send_rainbow() __attribute__ ((unused));
static void send_rainbow() {
    int delay = 15000000;

    for (int j = 0; j < 2; j++)
        for (int i = 0 ; i < N_LEDS; i++) {
            for (int k = 0 ; k < N_LEDS; k++)
                send_rainbow_pixel(i + k);
            _nsleep(delay);
        }
    // then reverse it
    for (int j = 0; j < 2; j++)
        for (int i = 0 ; i < N_LEDS; i++) {
            for (int k = N_LEDS - 1; k >= 0; k--)
                send_rainbow_pixel(i + k);
            _nsleep(delay);
        }
}

int main (void) {

    unsigned char r, g, b;

    board_init();

    gpio_on = GPIO_REG(GPIO_OUTPUT_VAL) | NEO_GPIO_MASK;
    gpio_off = gpio_on & ~NEO_GPIO_MASK;

    // Configure output GPIO driver
    GPIO_REG(GPIO_OUTPUT_VAL) |= NEO_GPIO_MASK;
    GPIO_REG(GPIO_OUTPUT_EN)  |= NEO_GPIO_MASK;

    printf("Neopixel HiFive1 Demo\n");
    printf("Copyright 2018 Curt Brune <curt@brune.net>\n");

    printf("%-20s: 0x%08lx\n", "PRCI_HFROSCCFG", PRCI_REG(PRCI_HFROSCCFG));
    printf("%-20s: 0x%08lx\n", "PRCI_HFXOSCCFG", PRCI_REG(PRCI_HFXOSCCFG));
    printf("%-20s: 0x%08lx\n", "PRCI_PLLCFG", PRCI_REG(PRCI_PLLCFG));
    printf("%-20s: 0x%08lx\n", "PRCI_PLLDIV", PRCI_REG(PRCI_PLLDIV));
    printf("%-20s: 0x%08lx\n", "PRCI_PROCMONCFG", PRCI_REG(PRCI_PROCMONCFG));

    dcount = 0;

    r = 128;
    g = 128;
    b = 128;

    while (1) {
        set_all(255, 255, 255, true);
        set_all(0, 0, 0, true);
        for (int i = 0; i < 4; i++) {
            printf("Pulse: %d\n", i);
            pulse_all(r + 64, g - 128, b - 64, 100000);
        }

        for (int i = 0; i < 2; i++)
            send_dot2(r - 64, g, b + 64, r + 64, g - 128, b - 64, 25000000);

        r += 8;
        g -= 8;
        b += 16;

        for (int h = 0; h < 2; h++)
            send_rainbow();
    }
}
