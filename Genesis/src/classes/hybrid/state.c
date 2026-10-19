/* Original Anathema isolated-bank foundation; no live save-schema change. */
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../merc.h"
#include "state.h"

static int owns_powers(long parent)
{
    return parent == CLASS_VAMPIRE || parent == CLASS_ELEM;
}

static int owns_classpoints(long parent)
{
    return parent == CLASS_DROW || parent == CLASS_DRAGON;
}

static int owns_generation(long parent)
{
    return parent == CLASS_VAMPIRE || parent == CLASS_DRAGON;
}

static int owns_branches(long parent)
{
    return parent == CLASS_VAMPIRE || parent == CLASS_MAGE;
}

static int owns_rage(long parent)
{
    return parent == CLASS_VAMPIRE || parent == CLASS_WEREWOLF ||
           parent == CLASS_NINJA;
}

static int owns_speed_timer(long parent)
{
    return parent == CLASS_VAMPIRE || parent == CLASS_HIGHLANDER;
}

static void empty_core(HYBRID_STATE *state)
{
    /* Native generation/age/learning defaults are deliberately NOT invented. */
    if (owns_branches(state->secondary_class))
        for (size_t i = 0; i < 3; ++i) state->secondary_branches[i] = -1;
}

int hybrid_state_valid(const HYBRID_STATE *state)
{
    const HYBRID_DEFINITION *definition;
    if (!state) return 0;
    definition = hybrid_by_id(state->hybrid_id);
    if (!definition || state->primary_class == state->secondary_class ||
        !hybrid_has_parent(definition, state->primary_class) ||
        !hybrid_has_parent(definition, state->secondary_class)) return 0;
    for (size_t i = 0; i < 20; ++i)
        if (state->secondary_powers[i] < 0 ||
            (!owns_powers(state->secondary_class) &&
             state->secondary_powers[i] != 0)) return 0;
    for (size_t i = 0; i < 2; ++i)
        if (state->secondary_classpoints[i] < 0 ||
            (!owns_classpoints(state->secondary_class) &&
             state->secondary_classpoints[i] != 0)) return 0;
    if (state->secondary_generation < 0 ||
        (state->secondary_class == CLASS_VAMPIRE &&
         state->secondary_generation > 13) ||
        (state->secondary_class == CLASS_DRAGON &&
         state->secondary_generation > AGE_HATCHLING) ||
        (!owns_generation(state->secondary_class) &&
         state->secondary_generation != 0)) return 0;
    for (size_t i = 0; i < 3; ++i) {
        long branch = state->secondary_branches[i];
        if (owns_branches(state->secondary_class)) {
            long max = state->secondary_class == CLASS_VAMPIRE ?
                       DISC_MAX : SPHERE_TIME;
            if (branch < -1 || branch > max) return 0;
        } else if (branch != 0) return 0;
    }
    if (state->secondary_rage < 0 ||
        (!owns_rage(state->secondary_class) && state->secondary_rage != 0) ||
        state->secondary_speed_timer < 0 ||
        (!owns_speed_timer(state->secondary_class) &&
         state->secondary_speed_timer != 0)) return 0;
    if (state->secondary_blood[BLOOD_CURRENT] < 0 ||
        state->secondary_blood[BLOOD_POOL] < 0 ||
        state->secondary_blood[BLOOD_CURRENT] >
            state->secondary_blood[BLOOD_POOL] ||
        state->secondary_beast < 0 || state->secondary_truebeast < 0) return 0;
    if (state->secondary_class != CLASS_VAMPIRE &&
        (state->secondary_blood[0] != 0 || state->secondary_blood[1] != 0 ||
         state->secondary_beast != 0 || state->secondary_truebeast != 0)) return 0;
    return 1;
}

int hybrid_state_prepare(const CHAR_DATA *ch,
                         const HYBRID_DEFINITION *definition,
                         HYBRID_STATE *destination)
{
    HYBRID_STATE candidate = {0};
    if (!ch || IS_NPC(ch) || !ch->pcdata || !definition || !destination ||
        hybrid_by_id(definition->id) != definition ||
        !hybrid_has_parent(definition, ch->class)) return 0;
    candidate.hybrid_id = definition->id;
    candidate.primary_class = ch->class;
    candidate.secondary_class = definition->parents[
        definition->parents[0] == ch->class ? 1 : 0];
    empty_core(&candidate);
    if (!hybrid_state_valid(&candidate)) return 0;
    *destination = candidate;
    return 1;
}

long *hybrid_secondary_bank(HYBRID_STATE *state, long requested_parent,
                            enum hybrid_bank_kind kind, size_t *count)
{
    if (count) *count = 0;
    if (!hybrid_state_valid(state) ||
        state->secondary_class != requested_parent) return NULL;
    if (kind == HYBRID_BANK_POWERS && owns_powers(requested_parent)) {
        if (count) *count = 20;
        return state->secondary_powers;
    }
    if (kind == HYBRID_BANK_CLASSPOINTS && owns_classpoints(requested_parent)) {
        if (count) *count = 2;
        return state->secondary_classpoints;
    }
    return NULL;
}

static size_t value_count(long parent, enum hybrid_value_kind kind)
{
    switch (kind) {
    case HYBRID_VALUE_POWER: return owns_powers(parent) ? 20 : 0;
    case HYBRID_VALUE_CLASSPOINT: return owns_classpoints(parent) ? 2 : 0;
    case HYBRID_VALUE_GENERATION: return owns_generation(parent) ? 1 : 0;
    case HYBRID_VALUE_BRANCH: return owns_branches(parent) ? 3 : 0;
    case HYBRID_VALUE_RAGE: return owns_rage(parent) ? 1 : 0;
    case HYBRID_VALUE_SPEED_TIMER: return owns_speed_timer(parent) ? 1 : 0;
    case HYBRID_VALUE_BLOOD: return parent == CLASS_VAMPIRE ? 2 : 0;
    case HYBRID_VALUE_BEAST:
    case HYBRID_VALUE_TRUEBEAST: return parent == CLASS_VAMPIRE ? 1 : 0;
    }
    return 0;
}

static long *secondary_slot(HYBRID_STATE *state, enum hybrid_value_kind kind,
                            size_t index)
{
    if (index >= value_count(state->secondary_class, kind)) return NULL;
    switch (kind) {
    case HYBRID_VALUE_POWER: return &state->secondary_powers[index];
    case HYBRID_VALUE_CLASSPOINT: return &state->secondary_classpoints[index];
    case HYBRID_VALUE_GENERATION: return &state->secondary_generation;
    case HYBRID_VALUE_BRANCH: return &state->secondary_branches[index];
    case HYBRID_VALUE_RAGE: return &state->secondary_rage;
    case HYBRID_VALUE_SPEED_TIMER: return &state->secondary_speed_timer;
    case HYBRID_VALUE_BLOOD: return &state->secondary_blood[index];
    case HYBRID_VALUE_BEAST: return &state->secondary_beast;
    case HYBRID_VALUE_TRUEBEAST: return &state->secondary_truebeast;
    }
    return NULL;
}

int hybrid_parent_value_read(const CHAR_DATA *ch, const HYBRID_STATE *state,
                             long parent, enum hybrid_value_kind kind,
                             size_t index, long *destination)
{
    long value;
    if (!ch || IS_NPC(ch) || !ch->pcdata || !destination ||
        !hybrid_state_valid(state) || ch->class != state->primary_class ||
        (parent != state->primary_class && parent != state->secondary_class) ||
        index >= value_count(parent, kind)) return 0;
    if (parent == state->secondary_class) {
        /* Keep const correctness; neither a primary nor secondary read can
         * expose writable pointers to the original character's storage. */
        switch (kind) {
        case HYBRID_VALUE_POWER: value = state->secondary_powers[index]; break;
        case HYBRID_VALUE_CLASSPOINT: value = state->secondary_classpoints[index]; break;
        case HYBRID_VALUE_GENERATION: value = state->secondary_generation; break;
        case HYBRID_VALUE_BRANCH: value = state->secondary_branches[index]; break;
        case HYBRID_VALUE_RAGE: value = state->secondary_rage; break;
        case HYBRID_VALUE_SPEED_TIMER: value = state->secondary_speed_timer; break;
        case HYBRID_VALUE_BLOOD: value = state->secondary_blood[index]; break;
        case HYBRID_VALUE_BEAST: value = state->secondary_beast; break;
        case HYBRID_VALUE_TRUEBEAST: value = state->secondary_truebeast; break;
        default: return 0;
        }
    } else {
        switch (kind) {
        case HYBRID_VALUE_POWER: value = ch->pcdata->powers[index]; break;
        case HYBRID_VALUE_CLASSPOINT: value = ch->pcdata->classpoints[index]; break;
        case HYBRID_VALUE_GENERATION: value = ch->vampgen; break;
        case HYBRID_VALUE_BRANCH: value = ch->pcdata->cland[index]; break;
        case HYBRID_VALUE_RAGE: value = ch->pcdata->wolf; break;
        case HYBRID_VALUE_SPEED_TIMER: value = ch->pcdata->celerity; break;
        case HYBRID_VALUE_BLOOD: value = ch->blood[index]; break;
        case HYBRID_VALUE_BEAST: value = ch->beast; break;
        case HYBRID_VALUE_TRUEBEAST: value = ch->truebeast; break;
        default: return 0;
        }
    }
    *destination = value;
    return 1;
}

int hybrid_secondary_value_write(HYBRID_STATE *state, long parent,
                                 enum hybrid_value_kind kind, size_t index,
                                 long value)
{
    HYBRID_STATE candidate;
    long *slot;
    if (!hybrid_state_valid(state) || parent != state->secondary_class) return 0;
    memcpy(&candidate, state, sizeof(candidate));
    slot = secondary_slot(&candidate, kind, index);
    if (!slot) return 0;
    *slot = value;
    if (!hybrid_state_valid(&candidate)) return 0;
    memcpy(state, &candidate, sizeof(*state));
    return 1;
}

int hybrid_state_write(FILE *stream, const HYBRID_STATE *state)
{
    if (!stream || !hybrid_state_valid(state)) return 0;
    if (fprintf(stream, "AnathemaHybrid 2 %ld %ld %ld",
                state->hybrid_id, state->primary_class,
                state->secondary_class) < 0) return 0;
    for (size_t i = 0; i < 20; ++i)
        if (fprintf(stream, " %ld", state->secondary_powers[i]) < 0) return 0;
    for (size_t i = 0; i < 2; ++i)
        if (fprintf(stream, " %ld", state->secondary_classpoints[i]) < 0)
            return 0;
    const long core[] = {
        state->secondary_generation, state->secondary_branches[0],
        state->secondary_branches[1], state->secondary_branches[2],
        state->secondary_rage, state->secondary_speed_timer,
        state->secondary_blood[0], state->secondary_blood[1],
        state->secondary_beast, state->secondary_truebeast
    };
    for (size_t i = 0; i < sizeof(core) / sizeof(core[0]); ++i)
        if (fprintf(stream, " %ld", core[i]) < 0) return 0;
    return fputc('\n', stream) != EOF && !ferror(stream);
}

static int next_number(const char **cursor, long *value)
{
    const char *start = *cursor;
    char *end;
    long parsed;
    while (isspace((unsigned char)*start)) ++start;
    errno = 0;
    parsed = strtol(start, &end, 10);
    if (end == start || errno == ERANGE ||
        (*end && !isspace((unsigned char)*end))) return 0;
    *cursor = end;
    *value = parsed;
    return 1;
}

int hybrid_state_read(FILE *stream, HYBRID_STATE *destination)
{
    char line[2048];
    const char header[] = "AnathemaHybrid";
    const char *cursor;
    long version;
    HYBRID_STATE candidate = {0};
    if (!stream || !destination || !fgets(line, sizeof(line), stream) ||
        ferror(stream)) return 0;
    size_t length = strlen(line);
    if (!length || line[length - 1] != '\n' ||
        strncmp(line, header, sizeof(header) - 1) ||
        !isspace((unsigned char)line[sizeof(header) - 1])) return 0;
    cursor = line + sizeof(header) - 1;
    if (!next_number(&cursor, &version) || (version != 1 && version != 2) ||
        !next_number(&cursor, &candidate.hybrid_id) ||
        !next_number(&cursor, &candidate.primary_class) ||
        !next_number(&cursor, &candidate.secondary_class)) return 0;
    empty_core(&candidate);
    for (size_t i = 0; i < 20; ++i)
        if (!next_number(&cursor, &candidate.secondary_powers[i])) return 0;
    for (size_t i = 0; i < 2; ++i)
        if (!next_number(&cursor, &candidate.secondary_classpoints[i])) return 0;
    if (version == 2) {
        long *core[] = {
            &candidate.secondary_generation, &candidate.secondary_branches[0],
            &candidate.secondary_branches[1], &candidate.secondary_branches[2],
            &candidate.secondary_rage, &candidate.secondary_speed_timer,
            &candidate.secondary_blood[0], &candidate.secondary_blood[1],
            &candidate.secondary_beast, &candidate.secondary_truebeast
        };
        for (size_t i = 0; i < sizeof(core) / sizeof(core[0]); ++i)
            if (!next_number(&cursor, core[i])) return 0;
    }
    while (isspace((unsigned char)*cursor)) ++cursor;
    if (*cursor || !hybrid_state_valid(&candidate)) return 0;
    *destination = candidate;
    return 1;
}
