#include "common.h"

#include "fmt.h"
#include "periph/flashpage.h"
#include "mutex.h"
#include "net/loramac.h"

#define RWWEE_PAGE 0

typedef struct {
    char    magic[4];
    uint8_t deveui[LORAMAC_DEVEUI_LEN];
    uint8_t appeui[LORAMAC_APPEUI_LEN];
    uint8_t appkey[LORAMAC_APPKEY_LEN];
    uint8_t appskey[LORAMAC_APPSKEY_LEN];
    uint8_t nwkskey[LORAMAC_NWKSKEY_LEN];
    uint8_t devaddr[LORAMAC_DEVADDR_LEN];
    uint32_t rx2_freq;
    uint8_t rx2_dr;
} loramac_state_t;

#if VERBOSE_DEBUG
void debug_loramac_state(loramac_state_t *state) {
    char s[33];
    int n;
    n = fmt_bytes_hex(s, state->deveui, sizeof(state->deveui));
    s[n] = 0;
    printf("DEVEUI:  %s, ", s);
    n = fmt_bytes_hex(s, state->appeui, sizeof(state->appeui));
    s[n] = 0;
    printf("APPEUI:  %s, ", s);
    n = fmt_bytes_hex(s, state->appkey, sizeof(state->appkey));
    s[n] = 0;
    printf("APPKEY:  %s\n", s);
    n = fmt_bytes_hex(s, state->appskey, sizeof(state->appskey));
    s[n] = 0;
    printf("APPSKEY: %s, ", s);
    n = fmt_bytes_hex(s, state->nwkskey, sizeof(state->nwkskey));
    s[n] = 0;
    printf("NWKSKEY: %s, ", s);
    n = fmt_bytes_hex(s, state->devaddr, sizeof(state->devaddr));
    s[n] = 0;
    printf("DEVADDR: %s\n", s);
    printf("RX2 FREQ: %lu, RX2 DR: %u\n", state->rx2_freq, state->rx2_dr);
}
#endif

void persist_loramac_state (semtech_loramac_t *mac) {
    uint8_t buffer[FLASHPAGE_SIZE];
    loramac_state_t state, *old_state;
    memcpy(state.magic, "RIOT", 4);
    semtech_loramac_get_deveui(mac,  state.deveui);
    semtech_loramac_get_appeui(mac,  state.appeui);
    semtech_loramac_get_appkey(mac,  state.appkey);
    semtech_loramac_get_appskey(mac, state.appskey);
    semtech_loramac_get_nwkskey(mac, state.nwkskey);
    semtech_loramac_get_devaddr(mac, state.devaddr);
    state.rx2_freq = semtech_loramac_get_rx2_freq(mac);
    state.rx2_dr   = semtech_loramac_get_rx2_dr(mac);

    // do not rewrite flash unless needed
    flashpage_rwwee_read(RWWEE_PAGE, buffer);
    old_state = (loramac_state_t *)buffer;
#if VERBOSE_DEBUG
    puts("OLD STATE:");
    debug_loramac_state(old_state);
    puts("STATE:");
    debug_loramac_state(&state);
#endif
    if(memcmp(&state, old_state, sizeof(state)) != 0) {
        memcpy(buffer, &state, sizeof(state));
        puts("Writing state to flash.");
        flashpage_rwwee_write(RWWEE_PAGE, buffer);
    }
}

void restore_loramac_state (semtech_loramac_t *mac, uint32_t uplink_counter, bool joined_state) {
    uint8_t buffer[FLASHPAGE_SIZE];
    loramac_state_t *state = (loramac_state_t *)buffer;

    flashpage_rwwee_read(RWWEE_PAGE, buffer);
    if (strncmp(state->magic, "RIOT", 4) != 0) {
       puts("No persisted loramac state found on FLASH.");
       return;
    }
#if VERBOSE_DEBUG
    puts("RECOVERED STATE:");
    debug_loramac_state(state);
#endif

    puts("Restoring loramac state from FLASH.");
    semtech_loramac_set_deveui(mac,  state->deveui);
    semtech_loramac_set_appeui(mac,  state->appeui);
    semtech_loramac_set_appkey(mac,  state->appkey);
    semtech_loramac_set_appskey(mac, state->appskey);
    semtech_loramac_set_nwkskey(mac, state->nwkskey);
    semtech_loramac_set_devaddr(mac, state->devaddr);
    semtech_loramac_set_rx2_freq(mac, state->rx2_freq);
    semtech_loramac_set_rx2_dr(mac, state->rx2_dr);
    // also restore volatile state
    semtech_loramac_set_uplink_counter(mac, uplink_counter);
    semtech_loramac_set_join_state(mac, joined_state);
}

void erase_loramac_state (void) {
    uint8_t buffer[FLASHPAGE_SIZE];
    memset(buffer, 0, FLASHPAGE_SIZE);
    flashpage_rwwee_write(RWWEE_PAGE, buffer);
}
