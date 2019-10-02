#include "common.h"

void debug_reset(void) {
    while (RTC->MODE0.SYNCBUSY.bit.COUNT) {}
    uint32_t rtc_count = RTC->MODE0.COUNT.reg;

    switch (RSTC->RCAUSE.reg) {
        case RSTC_RCAUSE_POR:
            puts ("Power on.");
            break;
        case RSTC_RCAUSE_EXT:
            puts ("Reset.");
            break;
        case RSTC_RCAUSE_BACKUP:
            switch (RSTC->BKUPEXIT.reg) {
                case RSTC_BKUPEXIT_RTC:
                    printf ("Awakened by RTC. Wakeup time %lu ms.\n", rtc_count);
                    break;
                case RSTC_BKUPEXIT_EXTWAKE:
                    printf ("Awakened by pin %d. Total sleep time %lu ms.\n", RSTC->WKCAUSE.reg, rtc_count);
                    break;
                case RSTC_BKUPEXIT_BBPS:
                default:
                    printf ("Awakened by battery backup power switch. Total sleep time %lu ms.\n", rtc_count);
                    break;
            }
            break;
        default:
            printf("RSTC->RCAUSE: %02x\n", RSTC->RCAUSE.reg);
            break;
    }
}

void debug_peripherals(void) {
    puts("Oscillators:");
    if (OSCCTRL->XOSCCTRL.reg & 2) { puts(" OSCCTRL->XOSCMCTRL.ENABLE = 1"); }
    if (OSCCTRL->OSC16MCTRL.reg & 2) {
        puts(" OSCCTRL->OSC16MCTRL.ENABLE = 1");
        printf(" OSCCTRL->OSC16MCTRL.FSEL   = %d\n", (OSCCTRL->OSC16MCTRL.reg >> 2) & 0x03);
    }
    if (OSCCTRL->DFLLCTRL.reg & 2) { puts(" OSCCTRL->DFLLMCTRL.ENABLE = 1"); }
    if (OSCCTRL->DPLLCTRLA.reg  & 2 ) { puts(" OSCCTRL->DPLLCTRLA.ENABLE = 1"); }
    if (OSC32KCTRL->XOSC32K.reg & 2) { puts(" OSC32KCTRL->XOSC32K.ENABLE = 1"); }
    if (OSC32KCTRL->OSC32K.reg & 2) { puts(" OSC32KCTRL->OSC32K.ENABLE  = 1"); }
    printf(" OSC32KCTRL->RTCCTRL.RTCSEL = %d\n", (uint8_t)(OSC32KCTRL->RTCCTRL.reg  & 0x07));

    puts("Clock generators:");
    for(int i=0; i<9; i++) {
        if (GCLK->GENCTRL[i].reg & GCLK_GENCTRL_GENEN) {
            printf(" GCLK->GENCTRL[%02d].SRC = %d\n", i, (uint8_t)(GCLK->GENCTRL[i].reg & 0x0F));
        }
    }
    for(int i=0; i<35; i++) {
        if (GCLK->PCHCTRL[i].reg & GCLK_PCHCTRL_CHEN) {
            printf(" GCLK->PCHCTRL[%02d].SRC = %d\n", i, (uint8_t)(GCLK->PCHCTRL[i].reg & 0x07));
        }
    }

    puts("Main clock:");
    printf(" MCLK->AHBMASK  = 0x%08lx\n", MCLK->AHBMASK.reg);
    printf(" MCLK->APBAMASK = 0x%08lx\n", MCLK->APBAMASK.reg);
    printf(" MCLK->APBBMASK = 0x%08lx\n", MCLK->APBBMASK.reg);
    printf(" MCLK->APBCMASK = 0x%08lx\n", MCLK->APBCMASK.reg);
    printf(" MCLK->APBDMASK = 0x%08lx\n", MCLK->APBDMASK.reg);
    printf(" MCLK->APBEMASK = 0x%08lx\n", MCLK->APBEMASK.reg);


    puts("Power manager:");
    printf(" PM->CTRLA.IORET        = %d\n", (PM->CTRLA.reg & 4) ? 1 : 0);
    printf(" PM->SLEEPCFG.SLEEPMODE = %d\n", PM->SLEEPCFG.reg & 0x07);
    printf(" PM->PLCFG.PLSEL        = %d\n", PM->PLCFG.reg & 0x03);

    puts("GPIO:");
    for (int i=0; i<4; i++) {
        for (int j=0; j<32; j++) {
            if (PORT->Group[i].PINCFG[j].reg || (PORT->Group[i].DIR.reg & (1 << j))) {
                printf(" P%c.%02d: ", 'A' + i, j);
            }
            if (PORT->Group[i].PINCFG[j].reg & PORT_PINCFG_PMUXEN) {
                printf("MUX%c", 'A' + (((PORT->Group[i].PMUX[j/2].reg >> 4*(j % 2))) & 0x0f));
            }else if (PORT->Group[i].DIR.reg & (1 << j)){
                printf("OUT ");
                if (PORT->Group[i].PINCFG[j].reg & PORT_PINCFG_DRVSTR) {
                    printf("DRVSTR ");
                }
                printf("value=%d", (PORT->Group[i].OUT.reg & (1 << j)) ? 1 : 0);
            }else if (PORT->Group[i].PINCFG[j].reg) {
                printf("IN ");
                if (PORT->Group[i].PINCFG[j].reg & PORT_PINCFG_PULLEN) {
                    printf("PULL%s ", (PORT->Group[i].OUT.reg & (1 << j)) ? "UP" : "DOWN");
                }
                if ((PORT->Group[i].PINCFG[j].reg & PORT_PINCFG_INEN) == 0) {
                    printf("DISABLED ");
                }
                printf("value=%d", (PORT->Group[i].IN.reg & (1 << j)) ? 1 : 0);
            }
            if (PORT->Group[i].PINCFG[j].reg || (PORT->Group[i].DIR.reg & (1 << j))) {
                printf("\n");
            }
        }
    }
}
