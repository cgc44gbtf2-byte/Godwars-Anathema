#ifndef ANATHEMA_HYBRID_PLAYER_H
#define ANATHEMA_HYBRID_PLAYER_H

#include "state.h"
#include "../../atomic_save.h"

/* Initialization is a side-effect-free template, not conversion or mastery.
 * Selection-dependent identities remain unselected; no quest charge or powers
 * are granted. Do not call native autoclass on the primary actor. */
int hybrid_secondary_initialize(const struct char_data *ch,
                                const HYBRID_DEFINITION *definition,
                                HYBRID_STATE *destination);
/* Checked attachment, never called by a player command in this stage.
 * Does not persist: the future conversion transaction must commit atomically. */
int hybrid_player_attach(struct char_data *ch, const HYBRID_STATE *state);
int hybrid_player_save_valid(const struct char_data *ch);
int hybrid_player_read_tail(struct char_data *ch, FILE *stream);
int hybrid_player_load_finish(struct char_data *ch);
/* Write a candidate state/payment through the primary atomic transaction.
 * FAILED leaves progression/currency unchanged. UNCERTAIN means rename took
 * place: keep the visible candidate in memory and retry durability, NEVER
 * refund/reapply it. No backup or multi-player atomicity is promised. */
enum atomic_save_result hybrid_player_commit(struct char_data *ch,
    const HYBRID_STATE *state, long primal, long experience, long quest);

#endif
