#ifndef ANATHEMA_HYBRID_STATE_H
#define ANATHEMA_HYBRID_STATE_H

#include <stdio.h>
#include "hybrid.h"

struct char_data;

/* Staged storage for confirmed shared-field collisions and Vampire resources.
 * The original parent's live legacy fields remain authoritative and are
 * never copied, cleared or modified by this component. This is not a complete
 * secondary class: native initialization, forms and other fields remain pending.
 * No game command attaches these records to a player or enables their powers. */
typedef struct hybrid_state {
    long hybrid_id;
    long primary_class;
    long secondary_class;
    long secondary_powers[20];
    long secondary_classpoints[2];
    /* Generation/age zero means not initialized; it grants no usable class. */
    long secondary_generation;
    long secondary_branches[3];
    long secondary_rage;
    long secondary_speed_timer;
    long secondary_blood[2];
    long secondary_beast;
    long secondary_truebeast;
} HYBRID_STATE;

enum hybrid_bank_kind {
    HYBRID_BANK_POWERS,
    HYBRID_BANK_CLASSPOINTS
};

/* Raw storage, NOT effective ranks or ability permission. Index zero for
 * scalars; exact native array index for banks. No primary write API exists. */
enum hybrid_value_kind {
    HYBRID_VALUE_POWER,
    HYBRID_VALUE_CLASSPOINT,
    HYBRID_VALUE_GENERATION,
    HYBRID_VALUE_BRANCH,
    HYBRID_VALUE_RAGE,
    HYBRID_VALUE_SPEED_TIMER,
    HYBRID_VALUE_BLOOD,
    HYBRID_VALUE_BEAST,
    HYBRID_VALUE_TRUEBEAST
};

int hybrid_state_prepare(const struct char_data *ch,
                         const HYBRID_DEFINITION *definition,
                         HYBRID_STATE *destination);
int hybrid_state_valid(const HYBRID_STATE *state);
long *hybrid_secondary_bank(HYBRID_STATE *state, long requested_parent,
                            enum hybrid_bank_kind kind, size_t *count);
int hybrid_parent_value_read(const struct char_data *ch,
                             const HYBRID_STATE *state, long parent,
                             enum hybrid_value_kind kind, size_t index,
                             long *destination);
/* A rejected assignment leaves the whole record unchanged. Blood current
 * cannot exceed pool; initialize pool before current (lower current first
 * when reducing pool). Positive resources are never provided by prepare. */
int hybrid_secondary_value_write(HYBRID_STATE *state, long parent,
                                 enum hybrid_value_kind kind, size_t index,
                                 long value);

/* Standalone, single-record v2 writer, v1/v2 reader. Not in player files.
 * v1 loads the new fields as uninitialized/empty; owned branch slots use -1.
 * Read failure leaves the destination unchanged. The caller must abort its
 * load after failure; do not continue scanning a malformed/overlong record.
 * Write failure is not a durable commit: the eventual caller must use the
 * existing atomic-save transaction and check flush/fsync/rename outcomes. */
int hybrid_state_write(FILE *stream, const HYBRID_STATE *state);
int hybrid_state_read(FILE *stream, HYBRID_STATE *destination);

#endif
