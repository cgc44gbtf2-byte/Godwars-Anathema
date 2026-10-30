#ifndef ANATHEMA_HYBRID_STATE_H
#define ANATHEMA_HYBRID_STATE_H

#include <stdio.h>
#include "hybrid.h"
#include "progression.h"
#include "forms.h"

struct char_data;

/* Staged storage for confirmed shared-field collisions, Vampire resources,
 * and the persisted logical form ledger.
 * The original parent's live legacy fields remain authoritative and are
 * never copied, cleared or modified by this component. player.c supplies an
 * optional player-file adapter and an untrained initialization template.
 * No game command attaches these records to a player. Only explicit checked
 * access/passive adapters may use their learned powers; conversion is disabled.
 * Form tokens are persisted, but native transformation handlers remain
 * unwired. All trained powers are intentionally form-independent. Clan
 * identities, remaining fields and ability integration remain pending; raw
 * structural validity is not complete class readiness. */
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
    HYBRID_PROGRESSION secondary_progression;
    /* Stable native build identity, not a social clan membership or staff role.
     * Zero on v1/v2/v3 migration means unconfigured; never infer a free build. */
    long secondary_build;
    HYBRID_FORM_STATE forms;
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
/* Form mutations validate a candidate whole record before replacing it. Power
 * access is intentionally independent of the current body/form. */
unsigned long hybrid_state_form_begin(HYBRID_STATE *state, long parent,
                                      unsigned int logical_token,
                                      unsigned int grants,
                                      unsigned int compatible);
unsigned long hybrid_state_form_fuse(HYBRID_STATE *state, long parent,
                                     unsigned int logical_token,
                                     unsigned int grants,
                                     unsigned int compatible,
                                     const HYBRID_FUSION_PROFILE *profile);
int hybrid_state_form_end(HYBRID_STATE *state, long parent,
                          unsigned int logical_token,
                          unsigned long activation);
int hybrid_state_feature_request(HYBRID_STATE *state, long parent,
                                 unsigned int features, int enabled);
int hybrid_state_form_release_parent(HYBRID_STATE *state, long parent);
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

/* Backward-compatible v4 core writer, v1-v4 reader. Older core records load
 * missing banks as untrained/uninitialized and form state as normal/base form;
 * selections belonging to the secondary use -1 rather than selecting a free
 * branch. Form state is stored in a separate optional line so older binaries
 * can skip it safely without changing the save epoch.
 * Read failure leaves the destination unchanged. The caller must abort its
 * load after failure; do not continue scanning a malformed/overlong record.
 * Write failure is not a durable commit: the eventual caller must use the
 * existing atomic-save transaction and check flush/fsync/rename outcomes. */
int hybrid_state_write(FILE *stream, const HYBRID_STATE *state);
int hybrid_state_forms_write(FILE *stream, const HYBRID_STATE *state);
/* Called after the AnathemaHybridForms word has been read by the player parser. */
int hybrid_state_forms_read_tail(FILE *stream, HYBRID_STATE *state);
int hybrid_state_read(FILE *stream, HYBRID_STATE *destination);
/* Player parser has consumed only the literal AnathemaHybrid keyword.
 * Requires its remaining data on the SAME bounded line. */
int hybrid_state_read_tail(FILE *stream, HYBRID_STATE *destination);

#endif
