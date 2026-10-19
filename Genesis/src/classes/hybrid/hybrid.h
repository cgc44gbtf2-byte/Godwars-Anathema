#ifndef ANATHEMA_HYBRID_H
#define ANATHEMA_HYBRID_H

#include <stddef.h>

/* Agreed server price. This stage displays it but cannot charge it.
 * Runtime configuration/transaction handling belongs to the conversion stage. */
#define HYBRID_DEFAULT_QUEST_COST 50000L

/* New identities are deliberately separate from serialized base-class IDs.
 * IDs are permanent: append new definitions; never reorder or recycle IDs. */
typedef struct hybrid_definition {
    long id;
    const char *name;
    long parents[2];
} HYBRID_DEFINITION;

size_t hybrid_count(void);
const HYBRID_DEFINITION *hybrid_at(size_t index);
const HYBRID_DEFINITION *hybrid_by_id(long id);
const HYBRID_DEFINITION *hybrid_by_name(const char *name);
const HYBRID_DEFINITION *hybrid_by_parents(long first, long second);
const char *hybrid_parent_name(long parent);
int hybrid_has_parent(const HYBRID_DEFINITION *hybrid, long parent);

#endif
