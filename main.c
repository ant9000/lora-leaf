#include "common.h"

#include <string.h>

#include "periph/gpio.h"
#include "periph/pm.h"

#include "fmt.h"
#include "net/loramac.h"

semtech_loramac_t loramac;
static uint8_t deveui[LORAMAC_DEVEUI_LEN];
static uint8_t appeui[LORAMAC_APPEUI_LEN];
static uint8_t appkey[LORAMAC_APPKEY_LEN];

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

    semtech_loramac_init(&loramac);

    // TODO: add a way to configure parameters via USB
    fmt_hex_bytes(deveui, DEVEUI);
    fmt_hex_bytes(appeui, APPEUI);
    fmt_hex_bytes(appkey, APPKEY);
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);
    semtech_loramac_set_dr(&loramac, LORAMAC_DR_5);

#if VERBOSE_DEBUG
    debug_peripherals();
#endif

    restore_loramac_state(&loramac, uplink_counter, joined_state);
    if (!semtech_loramac_is_mac_joined(&loramac)) {
        puts("Starting join procedure");
        if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) == SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
            puts("Join procedure succeeded, persisting data.");
            persist_loramac_state(&loramac);
        } else {
            RTC->MODE0.GP[0].reg = 0xFFFFFFFF;
        }
    }

    if (semtech_loramac_is_mac_joined(&loramac)) {
        // TODO: acquire sensors and serialize samples into message
        // send message
        printf("Sending: %s\n", message);
        uint8_t ret = semtech_loramac_send(&loramac,
                                       (uint8_t *)message, strlen(message));
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

    // persist counters
    RTC->MODE0.GP[1].reg = (failures << 16) + cycles;
    // put device into deep sleep
    enter_backup_mode();

    // never reached
    return 0;
}
