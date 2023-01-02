#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "semtech_loramac.h"

#ifdef __cplusplus
extern "C" {
#endif

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

void debug_reset (void);
#if VERBOSE_DEBUG
void debug_peripherals (void);
#endif

void enter_configuration_mode(void);
void enter_backup_mode(void);

void print_loramac_state (loramac_state_t *state);
int load_from_flash (loramac_state_t *state);
int save_to_flash (loramac_state_t *state);
void erase_flash (void);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
