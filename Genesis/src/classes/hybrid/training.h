#ifndef ANATHEMA_HYBRID_TRAINING_H
#define ANATHEMA_HYBRID_TRAINING_H

#include "state.h"
#include "specialization.h"
struct char_data;

enum hybrid_lesson_action {
    HYBRID_LESSON_BUILD,
    HYBRID_LESSON_GIFT_SELECT,
    HYBRID_LESSON_SPHERE_SELECT,
    HYBRID_LESSON_SPECIALIZE,
    HYBRID_LESSON_ELEMENT_FOCUS,
    HYBRID_LESSON_RANK,
    HYBRID_LESSON_ARETE,
    HYBRID_LESSON_WOLF_RANK,
    HYBRID_LESSON_DROW_POWER,
    HYBRID_LESSON_DROW_SPEED,
    HYBRID_LESSON_DRAGON_AGE,
    HYBRID_LESSON_DRAGON_SKILL
};
enum hybrid_lesson_result {
    HYBRID_LESSON_OK,
    HYBRID_LESSON_INVALID,
    HYBRID_LESSON_PRIMARY, /* Use unchanged native primary trainer. */
    HYBRID_LESSON_CHOICE_REQUIRED,
    HYBRID_LESSON_NEEDS_TEACHER,
    HYBRID_LESSON_AT_CAP,
    HYBRID_LESSON_PREREQUISITE,
    HYBRID_LESSON_NO_FUNDS,
    HYBRID_LESSON_UNCHANGED,
    HYBRID_LESSON_SAVE_PENDING,
    HYBRID_LESSON_SAVE_FAILED,
    HYBRID_LESSON_SAVE_UNCERTAIN
};
typedef struct hybrid_lesson_request {
    enum hybrid_lesson_action action;
    long parent;
    enum hybrid_training_tree tree; /* Ranked lesson only. */
    size_t index;                   /* Slot, rank index, or exact unlock bit. */
    long value;                     /* Choice/focus/build ID only. */
} HYBRID_LESSON_REQUEST;
typedef struct hybrid_lesson {
    HYBRID_STATE state;
    long primal, experience, quest;
    long primal_cost, experience_cost, quest_cost, secondary_dp_cost;
} HYBRID_LESSON;

/* Side-effect-free quote/candidate. Never accept a caller-supplied quote at
 * commit: recompute from the current actor and current funds. */
enum hybrid_lesson_result hybrid_training_prepare(const struct char_data *ch,
    const HYBRID_LESSON_REQUEST *request, HYBRID_LESSON *destination);
/* Backend only: requires the real connected original actor, out of combat.
 * Not called by selfbrid/native trainers yet. UNCERTAIN keeps the visible
 * rank/payment, blocks another lesson until primary save retry succeeds. */
enum hybrid_lesson_result hybrid_training_commit(struct char_data *ch,
    const HYBRID_LESSON_REQUEST *request);

#endif
