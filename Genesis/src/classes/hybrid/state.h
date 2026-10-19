#ifndef ANATHEMA_HYBRID_STATE_H
#define ANATHEMA_HYBRID_STATE_H

#include <stdio.h>
#include "hybrid.h"

struct char_data;

/* Staged storage for the confirmed shared-field collisions ONLY.
 * The original parent's live legacy fields remain authoritative and are
 * never copied, cleared or modified by this component. This is not a complete
 * secondary class: generation, forms and other owned fields are still pending.
 * No game command attaches these records to a player or enables their powers. */
typedef struct hybrid_state {
    long hybrid_id;
    long primary_class;
    long secondary_class;
    long secondary_powers[20];
    long secondary_classpoints[2];
} HYBRID_STATE;

enum hybrid_bank_kind {
    HYBRID_BANK_POWERS,
    HYBRID_BANK_CLASSPOINTS
};

int hybrid_state_prepare(const struct char_data *ch,
                         const HYBRID_DEFINITION *definition,
                         HYBRID_STATE *destination);
int hybrid_state_valid(const HYBRID_STATE *state);
long *hybrid_secondary_bank(HYBRID_STATE *state, long requested_parent,
                            enum hybrid_bank_kind kind, size_t *count);

/* Standalone, single-record versioned codec. Not wired into player files.
 * Read failure leaves the destination unchanged. The caller must abort its
 * load after failure; do not continue scanning a malformed/overlong record.
 * Write failure is not a durable commit: the eventual caller must use the
 * existing atomic-save transaction and check flush/fsync/rename outcomes. */
int hybrid_state_write(FILE *stream, const HYBRID_STATE *state);
int hybrid_state_read(FILE *stream, HYBRID_STATE *destination);

#endif
