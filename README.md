# Neopixel Demo on HiFive1 Eval Board

This little project is just for fun.  I had 3 goals:

* Spend time with the kids
* Learn about RISC-V and the E310 SoC
* Learn about Neopixels

My daughter inspired all the color streams in the project.  I am but
an implementer of her vision.

Learning about RISC-V and Neopixels had been on the TODO list for too
long.

Checkout this movie of the project in action:

[![Neopixel
Movie](https://github.com/cbrune/hifive1-neopixel/raw/docs/images/neopixel2.png)](https://www.youtube.com/watch?v=rKaM95WB7-A)

All in all, totally fun project.  Enjoy!

# Setup

This is a demo of driving a meter long Neopixel LED strip using the
GPIO controller on a [HiFive1 evaluation
board](https://www.sifive.com/products/hifive1/).  The SoC on this
board is a [SiFive Freedom
E310](https://www.sifive.com/documentation/chips/freedom-e310-g000-manual/).

## SDK

First you need to install the Freedom-E-SDK as described
[here](https://github.com/sifive/freedom-e-sdk).  Build it from source
or install pre-built binaries.

Next edit `config.mk` and set the `SDKDIR` variable to where you
installed the SDK.

## Hardware

This demo uses a meter long Neopixel LED strip, like [this
one](https://www.adafruit.com/product/1138) from
[Adafruit](https://www.adafruit.com/).

I followed [this
guide](https://learn.adafruit.com/adafruit-neopixel-uberguide/basic-connections)
to connect the LED strip to a power supply.

Connect the positive lead of the LED strip to PIN-8 on the HiFive1 and
the negative lead to a ground.  This is defined in the code
`neopixel.c` and the constant `NEO_GPIO`.

# Compile the code

Just type `make software` to compile the code.

# Upload and run the code

Connect your HiFive1 to the host computer in the usual way and type
`make upload`.  That should upload the code.

# Start the Show

Press the reset button on the HiFive1.  You will see some debug
printouts on the serial console and the LED strip should start
dancing.

# Trouble Shooting

You might need to adjust the timing constants slightly for your LED
strip.  See these constants in `neopixel.c`:

```C
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
```
