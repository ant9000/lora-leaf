#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "sx127x.h"
#include "semtech_loramac.h"

#ifdef __cplusplus
extern "C" {
#endif

extern sx127x_t sx127x;

void debug_reset(void);
void debug_peripherals(void);
void enter_backup_mode(void);

void persist_loramac_state (semtech_loramac_t *mac);
void restore_loramac_state (
    semtech_loramac_t *mac, uint32_t uplink_counter, bool joined_state
);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
