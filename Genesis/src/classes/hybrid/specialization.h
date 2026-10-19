#ifndef ANATHEMA_HYBRID_SPECIALIZATION_H
#define ANATHEMA_HYBRID_SPECIALIZATION_H

#include <stddef.h>
struct char_data;
struct hybrid_state;

/* WW chooses a FAMILY, not an individual gift. Other classes choose one
 * discipline/sphere/element/color/training path. No resources or flags here. */
enum hybrid_training_tree {
    HYBRID_TRAIN_BREED = 0,
    HYBRID_TRAIN_AUSPICE,
    HYBRID_TRAIN_TRIBE,
    HYBRID_TRAIN_DISCIPLINE,
    HYBRID_TRAIN_SPHERE,
    HYBRID_TRAIN_ELEMENT,
    HYBRID_TRAIN_COLOR,
    HYBRID_TRAIN_HIGHLAND,
    HYBRID_TRAIN_NINJA,
    HYBRID_TRAIN_MONK,
    HYBRID_TRAIN_TREE_COUNT
};

enum hybrid_vampire_profile {
    HYBRID_VAMPIRE_UNINITIALIZED = 0,
    HYBRID_VAMPIRE_STANDARD,
    HYBRID_VAMPIRE_CLANLESS
};

size_t hybrid_specialization_count(long parent);
int hybrid_specialization_branch_valid(long parent, long branch);
int hybrid_secondary_specialize(struct hybrid_state *state, long parent, long branch);
/* Cap profile only, NOT clan identity/affiliation. Future native initializer
 * must supply this from a legitimate clan choice; cannot replace an old mode. */
int hybrid_secondary_vampire_profile(struct hybrid_state *state, long parent,
                                    enum hybrid_vampire_profile profile);
int hybrid_parent_training_cap(const struct char_data *ch,
                              const struct hybrid_state *state, long parent,
                              enum hybrid_training_tree tree, size_t index,
                              long *destination);
int hybrid_parent_rank_read(const struct char_data *ch,
                           const struct hybrid_state *state, long parent,
                           enum hybrid_training_tree tree, size_t index,
                           long *destination);
/* Checked storage, NOT a trainer: no payment, prerequisites or live effects.
 * Legacy raw storage APIs are not effective-ability accessors. */
int hybrid_secondary_rank_write(struct hybrid_state *state, long parent,
                                enum hybrid_training_tree tree, size_t index,
                                long value);
int hybrid_secondary_ranks_valid(const struct hybrid_state *state);
/* Standalone native-focus analogue: all elements at their allowed pre-focus
 * caps, then focused 10, opposite 3, others 4. No live aura/form effects. */
int hybrid_secondary_element_focus(struct hybrid_state *state, long element);

#endif
