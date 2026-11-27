#ifndef ANATHEMA_HYBRID_ACCESS_H
#define ANATHEMA_HYBRID_ACCESS_H
#include "specialization.h"
struct char_data;
struct hybrid_state;
/* Primary class predicates/getters stay native. Never broaden IS_* macros:
 * many native commands subsequently write a shared field or activation flag. */
const struct hybrid_state *hybrid_live_secondary(const struct char_data *ch);
int hybrid_power_parent(const struct char_data *ch,long parent);
long hybrid_power_rank(struct char_data *ch, long parent,
                       enum hybrid_training_tree tree, size_t index);
int hybrid_power_drow(struct char_data *ch, long power);
int hybrid_drow_globe_active(const struct char_data *ch);
int hybrid_drow_globe_set(struct char_data *ch, int active);
long hybrid_drow_points_current(const struct char_data *ch);
int hybrid_drow_points_charge(struct char_data *ch, long amount);
int hybrid_drow_points_set_current(struct char_data *ch, long amount);
/* Secondary-only upkeep: returns 1 while active, 0 when it fades, -1 if the
 * character is not a valid secondary Drow. */
int hybrid_drow_globe_upkeep(struct char_data *ch, long amount);
int hybrid_power_dragon(struct char_data *ch, long skill);
#endif
