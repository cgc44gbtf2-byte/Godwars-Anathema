#ifndef ANATHEMA_HYBRID_H
#define ANATHEMA_HYBRID_H

#include <stddef.h>
struct char_data;

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
/* Palette follows HELP CLASS, not a new class/permission identity. */
const char *hybrid_parent_color(long parent);
/* Stable cosmetic mix of both canonical parents. Never consumes game RNG.
 * Canonical names/IDs stay plain for command/save/help lookup. */
int hybrid_display_name(const HYBRID_DEFINITION *hybrid, char *out, size_t size);
/* Approved fixed book-chain budget for valid player records.
 * Zero means this is not a Sorcerer pairing, not a casting permission. */
long hybrid_book_chain_policy(const HYBRID_DEFINITION *hybrid);
int hybrid_help_topic(struct char_data *ch, const char *argument);
void hybrid_help_parent_links(struct char_data *ch, const char *keywords);
int hybrid_has_parent(const HYBRID_DEFINITION *hybrid, long parent);

#endif
