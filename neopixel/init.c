#include <stdio.h>

/*
 *
 * Copyright 2018 Curt Brune <curt@brune.net>
 * All rights reserved.
 *
 * Most of this was lifted from the Freedom-E-SDK init code.
 * Copyright 2016 SiFive, Inc.
 *
 */

#include "platform.h"

static unsigned long mtime_lo(void)
{
  return *(volatile unsigned long *)(CLINT_CTRL_ADDR + CLINT_MTIME);
}

static void use_hfrosc(int div, int trim)
{
    /* Make sure the HFROSC is running at its default setting */
    PRCI_REG(PRCI_HFROSCCFG) = (ROSC_DIV(div) | ROSC_TRIM(trim) | ROSC_EN(1));
    while ((PRCI_REG(PRCI_HFROSCCFG) & ROSC_RDY(1)) == 0) ;
    PRCI_REG(PRCI_PLLCFG) &= ~PLL_SEL(1);
}

static void use_pll(int refsel, int bypass, int r, int f, int q)
{
    /* Ensure that we aren't running off the PLL before we mess with it. */
    if (PRCI_REG(PRCI_PLLCFG) & PLL_SEL(1)) {
        /* Make sure the HFROSC is running at its default setting */
        use_hfrosc(4, 16);
    }

    /* Set PLL Source to be HFXOSC if available. */
    uint32_t config_value = 0;

    config_value |= PLL_REFSEL(refsel);

    if (bypass) {
        // Bypass
        config_value |= PLL_BYPASS(1);

        PRCI_REG(PRCI_PLLCFG) = config_value;

        // If we don't have an HFXTAL, this doesn't really matter.
        // Set our Final output divide to divide-by-1:
        PRCI_REG(PRCI_PLLDIV) = (PLL_FINAL_DIV_BY_1(1) | PLL_FINAL_DIV(0));
    } else {
        // In case we are executing from QSPI,
        // (which is quite likely) we need to
        // set the QSPI clock divider appropriately
        // before boosting the clock frequency.

        // Div = f_sck/2
        SPI0_REG(SPI_REG_SCKDIV) = 8;

        // Set DIV Settings for PLL
        // Both HFROSC and HFXOSC are modeled as ideal
        // 16MHz sources (assuming dividers are set properly for
        // HFROSC).
        // (Legal values of f_REF are 6-48MHz)

        // Set DIVR to divide-by-2 to get 8MHz frequency
        // (legal values of f_R are 6-12 MHz)

        config_value |= PLL_BYPASS(1);
        config_value |= PLL_R(r);

        // Set DIVF to get 512Mhz frequncy
        // There is an implied multiply-by-2, 16Mhz.
        // So need to write 32-1
        // (legal values of f_F are 384-768 MHz)
        config_value |= PLL_F(f);

        // Set DIVQ to divide-by-2 to get 256 MHz frequency
        // (legal values of f_Q are 50-400Mhz)
        config_value |= PLL_Q(q);

        // Set our Final output divide to divide-by-1:
        PRCI_REG(PRCI_PLLDIV) = (PLL_FINAL_DIV_BY_1(1) | PLL_FINAL_DIV(0));

        PRCI_REG(PRCI_PLLCFG) = config_value;

        // Un-Bypass the PLL.
        PRCI_REG(PRCI_PLLCFG) &= ~PLL_BYPASS(1);

        // Wait for PLL Lock
        // Note that the Lock signal can be glitchy.
        // Need to wait 100 us
        // RTC is running at 32kHz.
        // So wait 4 ticks of RTC.
        uint32_t now = mtime_lo();
        while (mtime_lo() - now < 4) ;

        // Now it is safe to check for PLL Lock
        while ((PRCI_REG(PRCI_PLLCFG) & PLL_LOCK(1)) == 0) ;
    }

    // Switch over to PLL Clock source
    PRCI_REG(PRCI_PLLCFG) |= PLL_SEL(1);
}

static void use_default_clocks()
{
    // Turn off the LFROSC
    AON_REG(AON_LFROSC) &= ~ROSC_EN(1);

    // Use HFROSC
    use_hfrosc(4, 16);
}

static void uart_init(size_t baud_rate)
{
    GPIO_REG(GPIO_IOF_SEL) &= ~IOF0_UART0_MASK;
    GPIO_REG(GPIO_IOF_EN) |= IOF0_UART0_MASK;
    UART0_REG(UART_REG_DIV) = get_cpu_freq() / baud_rate - 1;
    UART0_REG(UART_REG_TXCTRL) = UART_TXEN;
    UART0_REG(UART_REG_RXCTRL) = UART_RXEN;

    // Wait a bit to avoid corruption on the UART.
    // (In some cases, switching to the IOF can lead
    // to output glitches, so need to let the UART
    // reciever time out and resynchronize to the real
    // start of the stream.
    volatile int i=0;
    while(i < 10000){i++;}
}

int _getc(char * c) {
    uint32_t val = (uint32_t) UART0_REG(UART_REG_RXFIFO);
    if (val & 0x80000000) {
        return 0;
    } else {
        *c =  val & 0xFF;
        return 1;
    }
}

void board_init()
{
    use_default_clocks();
    use_pll(1, 0, 1, 31, 1);
    uart_init(115200);
    printf("Starting program...\n");
    printf("Core CPU freq: %ld Hz\n", get_cpu_freq());
    printf("Timer freq   : %ld Hz\n", get_timer_freq());
}
