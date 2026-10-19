#ifndef ANATHEMA_HYBRID_READINESS_H
#define ANATHEMA_HYBRID_READINESS_H

#include <stddef.h>
struct char_data;

enum hybrid_readiness_kind {
    HYBRID_READY_HP, HYBRID_READY_MANA, HYBRID_READY_STAT,
    HYBRID_READY_STANCE, HYBRID_READY_WEAPON, HYBRID_READY_STYLE
};
enum hybrid_readiness_status {
    HYBRID_READY_MISSING, HYBRID_READY_MET, HYBRID_READY_REVIEW,
    HYBRID_READY_EXEMPT
};
typedef struct hybrid_readiness_item {
    enum hybrid_readiness_kind kind;
    long index, current, required;
    enum hybrid_readiness_status status;
} HYBRID_READINESS_ITEM;
#define HYBRID_READINESS_ITEMS 27
typedef struct hybrid_readiness {
    HYBRID_READINESS_ITEM items[HYBRID_READINESS_ITEMS];
    size_t count, applicable, met, review;
} HYBRID_READINESS;

/* Numeric training/capacity only, NEVER final conversion eligibility.
 * No mutation, payment, grants, effective getter normalization or disk I/O.
 * Positive return means report built, not qualified for conversion. */
int hybrid_readiness_read(const struct char_data *ch, HYBRID_READINESS *report);
void hybrid_readiness_show(struct char_data *ch);

#endif
