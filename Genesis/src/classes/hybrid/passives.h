#ifndef ANATHEMA_HYBRID_PASSIVES_H
#define ANATHEMA_HYBRID_PASSIVES_H
struct char_data;
enum hybrid_passive_metric {
    HYBRID_PASSIVE_SPEED,
    HYBRID_PASSIVE_SOAK,
    HYBRID_PASSIVE_AVOID,
    HYBRID_PASSIVE_BREAK,
    HYBRID_PASSIVE_DAMCAP,
    HYBRID_PASSIVE_HITROLL,
    HYBRID_PASSIVE_DAMROLL,
    HYBRID_PASSIVE_ARMOR
};
/* Apply learned SECONDARY effects once, after native class contribution and
 * before shared floors/conditions. No RNG, activation flags or resource writes.
 * Numeric contributions add; no copied primary flags or damage multipliers. */
long hybrid_secondary_passive(struct char_data *ch,
                             enum hybrid_passive_metric metric, long value);
int hybrid_secondary_unwounded(struct char_data *ch);
#endif
