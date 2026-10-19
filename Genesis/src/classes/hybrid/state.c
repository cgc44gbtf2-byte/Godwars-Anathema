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

int hybrid_state_write(FILE *stream, const HYBRID_STATE *state)
{
    if (!stream || !hybrid_state_valid(state)) return 0;
    if (fprintf(stream, "AnathemaHybrid 1 %ld %ld %ld",
                state->hybrid_id, state->primary_class,
                state->secondary_class) < 0) return 0;
    for (size_t i = 0; i < 20; ++i)
        if (fprintf(stream, " %ld", state->secondary_powers[i]) < 0) return 0;
    for (size_t i = 0; i < 2; ++i)
        if (fprintf(stream, " %ld", state->secondary_classpoints[i]) < 0)
            return 0;
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
    char line[1024];
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
    if (!next_number(&cursor, &version) || version != 1 ||
        !next_number(&cursor, &candidate.hybrid_id) ||
        !next_number(&cursor, &candidate.primary_class) ||
        !next_number(&cursor, &candidate.secondary_class)) return 0;
    for (size_t i = 0; i < 20; ++i)
        if (!next_number(&cursor, &candidate.secondary_powers[i])) return 0;
    for (size_t i = 0; i < 2; ++i)
        if (!next_number(&cursor, &candidate.secondary_classpoints[i])) return 0;
    while (isspace((unsigned char)*cursor)) ++cursor;
    if (*cursor || !hybrid_state_valid(&candidate)) return 0;
    *destination = candidate;
    return 1;
}
