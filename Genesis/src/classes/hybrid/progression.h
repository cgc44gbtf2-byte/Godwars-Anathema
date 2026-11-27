#ifndef ANATHEMA_HYBRID_PROGRESSION_H
#define ANATHEMA_HYBRID_PROGRESSION_H

#include <stddef.h>

struct char_data;
struct hybrid_state;

/* Staged secondary storage only. Some native arrays mix trained ranks,
 * resources and transient flags/timers; this is not a live class template. */
typedef struct hybrid_progression {
    long breed[3], auspice[5], tribes[20], selected_gifts[3], wolf_rank;
    long colors[6], highland[4], ninja[5], monk[4], drow[2];
    long dragon_color, dragon_skills;
    long spheres[9], quint[3], arete, paradox, paradox_ward;
    long full_branch, vampire_profile;
} HYBRID_PROGRESSION;

enum hybrid_progression_kind {
    HYBRID_PROGRESS_BREED,
    HYBRID_PROGRESS_AUSPICE,
    HYBRID_PROGRESS_TRIBE,
    HYBRID_PROGRESS_SELECTED_GIFT,
    HYBRID_PROGRESS_WOLF_RANK,
    HYBRID_PROGRESS_COLOR,
    HYBRID_PROGRESS_HIGHLAND,
    HYBRID_PROGRESS_NINJA,
    HYBRID_PROGRESS_MONK,
    HYBRID_PROGRESS_DROW,
    HYBRID_PROGRESS_DRAGON_COLOR,
    HYBRID_PROGRESS_DRAGON_SKILLS,
    HYBRID_PROGRESS_SPHERE,
    HYBRID_PROGRESS_QUINT,
    HYBRID_PROGRESS_ARETE,
    HYBRID_PROGRESS_PARADOX,
    HYBRID_PROGRESS_PARADOX_WARD,
    HYBRID_PROGRESS_KIND_COUNT
};

enum hybrid_gift_family {
    HYBRID_GIFT_UNSELECTED = -1,
    HYBRID_GIFT_BREED = 0,
    HYBRID_GIFT_AUSPICE = 1,
    HYBRID_GIFT_TRIBE = 2
};

/* Fixed v3 wire order: 70 native fields, full branch, Vampire cap profile.
 * Serialize fields, never the C structure image. */
#define HYBRID_PROGRESSION_WORDS 72

int hybrid_parent_progression_read(const struct char_data *ch,
                                  const struct hybrid_state *state, long parent,
                                  enum hybrid_progression_kind kind,
                                  size_t index, long *destination);
int hybrid_secondary_progression_write(struct hybrid_state *state, long parent,
                                      enum hybrid_progression_kind kind,
                                      size_t index, long value);
/* Initial specialization only; no player command or respec operation exists. */
int hybrid_secondary_gift_choose(struct hybrid_state *state, long parent,
                                enum hybrid_gift_family family);
int hybrid_parent_gift_training_cap(const struct char_data *ch,
                                   const struct hybrid_state *state, long parent,
                                   enum hybrid_gift_family family, size_t index,
                                   long *destination);
int hybrid_parent_gift_rank(const struct char_data *ch,
                           const struct hybrid_state *state, long parent,
                           enum hybrid_gift_family family, size_t index,
                           long *destination);

/* Record/codec backend. These functions never operate on live actors. */
int hybrid_progression_prepare(long parent, HYBRID_PROGRESSION *destination);
int hybrid_progression_valid(long parent, const HYBRID_PROGRESSION *record);
int hybrid_progression_record_read(long parent, const HYBRID_PROGRESSION *record,
                                   enum hybrid_progression_kind kind,
                                   size_t index, long *destination);
int hybrid_progression_export(const HYBRID_PROGRESSION *record,
                              long *values, size_t count);
int hybrid_progression_import(long parent, const long *values, size_t count,
                              HYBRID_PROGRESSION *destination);

#endif
