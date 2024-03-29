#include "common.h"

#include "periph/gpio.h"
#include "periph/pm.h"

void enter_backup_mode(void) {
    // led levels are inverted
#if defined(LED0_PIN)
    gpio_set(LED0_PIN);
#endif
#if defined(LED1_PIN)
    gpio_set(LED1_PIN);
#endif

#if ENABLE_WAKEUP_PIN
    // NB: the wakeup pin needs 130uA extra
    puts("Enabling PA07 as external wakeup pin.\n");
    gpio_init(GPIO_PIN(PA, 7), GPIO_IN_PU);
    RSTC->WKEN.reg = 1 << 7;
    RSTC->WKPOL.reg = 0;
#endif

    puts("Putting radio in sleep mode.");
    gpio_clear(TX_OUTPUT_SEL_PIN);
    gpio_clear(TCXO_PWR_PIN);

    printf("Scheduling an RTC interrupt in %u seconds.\n", SLEEP_SECONDS);
    // reset count and set compare value at SLEEP_SECONDS
    RTC->MODE0.COUNT.reg = 0;
    RTC->MODE0.COMP[0].reg = SLEEP_SECONDS * 1000;
    // clear compare flag and enable interrupt
    RTC->MODE0.INTFLAG.reg |= RTC_MODE0_INTFLAG_CMP0;
    RTC->MODE0.INTENSET.reg |= RTC_MODE0_INTENSET_CMP0;

#if VERBOSE_DEBUG
    debug_peripherals();
#endif

    puts("Entering BACKUP mode.");

    // power down radio spi clock
    MCLK->APBCMASK.reg &= !MCLK_APBCMASK_SERCOM4;
    GCLK->PCHCTRL[SERCOM4_GCLK_ID_CORE].reg &= ~GCLK_PCHCTRL_CHEN;
    while (GCLK->PCHCTRL[SERCOM4_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN) {}

#ifdef MODULE_STDIO_UART
    // power down serial port clock
    MCLK->APBCMASK.reg &= !MCLK_APBCMASK_SERCOM0;
    GCLK->PCHCTRL[SERCOM0_GCLK_ID_CORE].reg &= ~GCLK_PCHCTRL_CHEN;
    while (GCLK->PCHCTRL[SERCOM0_GCLK_ID_CORE].reg & GCLK_PCHCTRL_CHEN) {}
    // mux serial port pins to gpio and set them off
    gpio_init(GPIO_PIN(PA, 4), GPIO_OUT);
    gpio_init(GPIO_PIN(PA, 5), GPIO_OUT);
    gpio_clear(GPIO_PIN(PA, 4));
    gpio_clear(GPIO_PIN(PA, 5));
#endif

#ifdef MODULE_STDIO_CDC_ACM
    // power down USB clock
    OSCCTRL->DFLLCTRL.bit.ENABLE = 0;
    while (!(OSCCTRL->STATUS.reg & OSCCTRL_STATUS_DFLLRDY)) {}
    // mux usb port pins to gpio and set them off
    gpio_init(GPIO_PIN(PA, 24), GPIO_OUT);
    gpio_init(GPIO_PIN(PA, 25), GPIO_OUT);
    gpio_clear(GPIO_PIN(PA, 24));
    gpio_clear(GPIO_PIN(PA, 25));
#endif

    pm_set(0);
}
