#include "common.h"

#include "fmt.h"
#include "periph/flashpage.h"

#define RWWEE_PAGE 0

int load_from_flash (loramac_state_t *state) {
    uint8_t buffer[FLASHPAGE_SIZE];
    loramac_state_t *tmp_state = (loramac_state_t *)buffer;
    flashpage_rwwee_read(RWWEE_PAGE, buffer);
    if (strncmp(tmp_state->magic, "RIOT", 4) != 0) {
#if VERBOSE_DEBUG
       puts("No loramac state found on FLASH.");
#endif
       return -1;
    }
    memcpy(state, tmp_state, sizeof(loramac_state_t));
#if VERBOSE_DEBUG
    puts("LOADED STATE:");
    print_loramac_state(state);
#endif
    return 0;
}

int save_to_flash (loramac_state_t *state) {
    uint8_t buffer[FLASHPAGE_SIZE];
    loramac_state_t *old_state = (loramac_state_t *)buffer;
    flashpage_rwwee_read(RWWEE_PAGE, buffer);
#if VERBOSE_DEBUG
    puts("OLD STATE:");
    print_loramac_state(old_state);
    puts("STATE:");
    print_loramac_state(state);
#endif
    // do not rewrite flash unless needed
    if(memcmp(state, old_state, sizeof(loramac_state_t)) != 0) {
        memcpy(buffer, state, sizeof(loramac_state_t));
        puts("Writing state to flash.");
        flashpage_rwwee_write(RWWEE_PAGE, buffer);
        return 1;
    }
    return 0;
}

void erase_flash (void) {
    uint8_t buffer[FLASHPAGE_SIZE];
    memset(buffer, 0, FLASHPAGE_SIZE);
    flashpage_rwwee_write(RWWEE_PAGE, buffer);
}

void print_loramac_state (loramac_state_t *state) {
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
