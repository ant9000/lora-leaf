#include "common.h"

#include "periph/gpio.h"
#include "net/loramac.h"
#include "sx127x.h"
#include "sx127x_netdev.h"
#include "sx127x_params.h"

static semtech_loramac_t loramac;
static sx127x_t sx127x;

void persist_loramac_state (semtech_loramac_t *mac) {
    loramac_state_t state;
    memcpy(state.magic, "RIOT", 4);
    semtech_loramac_get_deveui(mac,  state.deveui);
    semtech_loramac_get_appeui(mac,  state.appeui);
    semtech_loramac_get_appkey(mac,  state.appkey);
    semtech_loramac_get_appskey(mac, state.appskey);
    semtech_loramac_get_nwkskey(mac, state.nwkskey);
    semtech_loramac_get_devaddr(mac, state.devaddr);
    state.rx2_freq = semtech_loramac_get_rx2_freq(mac);
    state.rx2_dr   = semtech_loramac_get_rx2_dr(mac);
    save_to_flash(&state);
}

int restore_loramac_state (semtech_loramac_t *mac, uint32_t uplink_counter, bool joined_state) {
    loramac_state_t state;
    if (load_from_flash(&state) < 0) {
       return -1;
    }
    puts("Restoring loramac state from FLASH config.");
    semtech_loramac_set_deveui(mac,  state.deveui);
    semtech_loramac_set_appeui(mac,  state.appeui);
    semtech_loramac_set_appkey(mac,  state.appkey);
    semtech_loramac_set_appskey(mac, state.appskey);
    semtech_loramac_set_nwkskey(mac, state.nwkskey);
    semtech_loramac_set_devaddr(mac, state.devaddr);
    semtech_loramac_set_rx2_freq(mac, state.rx2_freq);
    semtech_loramac_set_rx2_dr(mac, state.rx2_dr);
    // also restore volatile state
    semtech_loramac_set_uplink_counter(mac, uplink_counter);
    semtech_loramac_set_join_state(mac, joined_state);
    return 0;
}

int main(void)
{
    char *message = "LoRaWAN test";
    debug_reset();

    // keep volatile state outside flash, to minimize rewriting
    uint32_t uplink_counter = 0;
    bool joined_state = false;
    uint16_t failures=0, cycles=1;

    if (RSTC->RCAUSE.reg == RSTC_RCAUSE_BACKUP) {
        // if data is present on flash, assume we have already joined
        joined_state = true;
        // uplink counter is saved in GP0 to persist it during backup mode
        uplink_counter = RTC->MODE0.GP[0].reg;
        // -1 indicates we were not joined before sleeping
        if (uplink_counter == 0xFFFFFFFF) {
            joined_state = false;
            uplink_counter = 0;
        }

        // we use GP1 lower word for counting waking cycles, upper word for failures
        failures = (uint16_t)((RTC->MODE0.GP[1].reg & 0xffff0000) >> 16);
        cycles   = (uint16_t)(RTC->MODE0.GP[1].reg & 0x0000ffff) + 1;
    } else {
        gpio_init(BTN0_PIN, BTN0_MODE);
        if (gpio_read(BTN0_PIN) == 0) {
            enter_configuration_mode();
        }
        // we are not resuming from backup mode, reconfigure RTC
        // select ULP oscillator for RTC
        OSC32KCTRL->RTCCTRL.reg = OSC32KCTRL_RTCCTRL_RTCSEL_ULP32K;
        // reset RTC
        RTC->MODE0.CTRLA.bit.SWRST = 1;
        while (RTC->MODE0.CTRLA.bit.SWRST) {}
        // configure RTC in 32bit counter mode at 1kHz
        RTC->MODE0.CTRLA.reg = RTC_MODE0_CTRLA_MODE(0)
                            | RTC_MODE0_CTRLA_COUNTSYNC
                            | RTC_MODE0_CTRLA_MATCHCLR
                            | RTC_MODE0_CTRLA_PRESCALER_DIV32
                            | RTC_MODE0_CTRLA_ENABLE;
        while (RTC->MODE0.SYNCBUSY.reg) {}
    }

puts("before sx127x init");
    sx127x_setup(&sx127x, &sx127x_params[0], 0);
    loramac.netdev = &sx127x.netdev;
    loramac.netdev->driver = &sx127x_driver;
puts("before lora init");
    semtech_loramac_init(&loramac);
puts("before lora set dr");
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);

#if VERBOSE_DEBUG
    debug_peripherals();
#endif
#if defined(LED0_PIN)
    gpio_init(LED0_PIN, GPIO_OUT);
#endif

    if (restore_loramac_state(&loramac, uplink_counter, joined_state) < 0) {
        puts("FLASH empty - entering configuration mode.");
        enter_configuration_mode();
    }
    if (!semtech_loramac_is_mac_joined(&loramac)) {
        puts("Starting join procedure");
#if defined(LED0_PIN)
        gpio_clear(LED0_PIN);
#endif
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) == SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            puts("Join procedure succeeded.");
        } else {
            RTC->MODE0.GP[0].reg = 0xFFFFFFFF;
        }
#if defined(LED0_PIN)
        gpio_set(LED0_PIN);
#endif
    }

    if (semtech_loramac_is_mac_joined(&loramac)) {
        // TODO: acquire sensors and serialize samples into message
        // send message
        printf("Sending: %s\n", message);
#if defined(LED0_PIN)
        gpio_clear(LED0_PIN);
#endif
        uint8_t ret = semtech_loramac_send(&loramac,
                                       (uint8_t *)message, strlen(message));
#if defined(LED0_PIN)
        gpio_set(LED0_PIN);
#endif
        uplink_counter = semtech_loramac_get_uplink_counter(&loramac);
        RTC->MODE0.GP[0].reg = uplink_counter;
        if (ret == SEMTECH_LORAMAC_TX_DONE)  {
            printf("Message #%lu successfully sent.\n", uplink_counter - 1);
        } else {
            printf("Cannot send message '%s', ret code: %d\n", message, ret);
            failures += 1;  // increment failure count
            if(ret == SEMTECH_LORAMAC_NOT_JOINED) {
                // save condition, to join network at next wakeup
                RTC->MODE0.GP[0].reg = 0xFFFFFFFF;
            }
        }

    } else {
        puts("Join procedure failed");
        failures += 1;  // increment failure count
    }

    printf("Faults: %u/%u\n", failures, cycles);

    puts("Persisting LoRaWAN state.");
    persist_loramac_state(&loramac);
    // persist counters
    RTC->MODE0.GP[1].reg = (failures << 16) + cycles;
    // persist state
    // put device into deep sleep
    sx127x_reset(&sx127x);
    sx127x_set_sleep(&sx127x);
    enter_backup_mode();

    // never reached
    return 0;
}
