#ifndef ANATHEMA_HYBRID_BUILD_H
#define ANATHEMA_HYBRID_BUILD_H

#include <stddef.h>
struct hybrid_state;

typedef struct hybrid_build_definition {
    long id, parent;
    const char *name;
    long branches[3], color;
} HYBRID_BUILD_DEFINITION;

/* Only native autoclass options, with explicit stable IDs. No invented clans.
 * These are class build labels, NOT social clan membership/leader privileges. */
size_t hybrid_build_count(void);
const HYBRID_BUILD_DEFINITION *hybrid_build_at(size_t index);
const HYBRID_BUILD_DEFINITION *hybrid_build_by_id(long id);
int hybrid_build_record_valid(const struct hybrid_state *state);
/* Candidate operations only. Existing choices cannot be replaced. */
int hybrid_secondary_build_choose(struct hybrid_state *state, long id);
int hybrid_secondary_gift_select(struct hybrid_state *state, size_t family, long gift);
int hybrid_secondary_sphere_select(struct hybrid_state *state, size_t slot, long sphere);

#endif
