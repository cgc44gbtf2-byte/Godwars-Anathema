#ifndef ANATHEMA_HYBRID_MASTERY_H
#define ANATHEMA_HYBRID_MASTERY_H

#include "readiness.h"

enum hybrid_mastery_kind {
    HYBRID_MASTERY_CHOICE, HYBRID_MASTERY_DISCIPLINE,
    HYBRID_MASTERY_BREED, HYBRID_MASTERY_AUSPICE, HYBRID_MASTERY_TRIBE,
    HYBRID_MASTERY_WOLF_RANK, HYBRID_MASTERY_SPHERE, HYBRID_MASTERY_ARETE,
    HYBRID_MASTERY_ELEMENT, HYBRID_MASTERY_COLOR, HYBRID_MASTERY_HIGHLAND,
    HYBRID_MASTERY_NINJA, HYBRID_MASTERY_MONK, HYBRID_MASTERY_DROW,
    HYBRID_MASTERY_DROW_SPEED, HYBRID_MASTERY_DRAGON, HYBRID_MASTERY_DRAGON_AGE,
    HYBRID_MASTERY_RESERVED
};
typedef struct hybrid_mastery_item {
    enum hybrid_mastery_kind kind;
    long index, current, required;
    const char *name; /* Static labels, no ownership transfer. */
    enum hybrid_readiness_status status;
} HYBRID_MASTERY_ITEM;
#define HYBRID_MASTERY_ITEMS 64
typedef struct hybrid_mastery {
    HYBRID_MASTERY_ITEM items[HYBRID_MASTERY_ITEMS];
    size_t count, applicable, met, review;
} HYBRID_MASTERY;
typedef struct hybrid_prerequisites {
    HYBRID_READINESS numeric;
    HYBRID_MASTERY mastery;
    long quest, quest_required;
    int existing_hybrid, rejected_player, requirements_met;
} HYBRID_PREREQUISITES;

/* Read raw ORIGINAL-parent training. Never grants abilities, caps ranks,
 * selects builds, charges currency, saves, or authorizes conversion.
 * Reports choices and suspicious legacy data instead of silently repairing it.
 * Report-built return values and requirements_met are not conversion permits. */
int hybrid_mastery_read(const struct char_data *ch, HYBRID_MASTERY *report);
int hybrid_prerequisites_read(const struct char_data *ch,
                              HYBRID_PREREQUISITES *report);
void hybrid_mastery_show(struct char_data *ch);

#endif
