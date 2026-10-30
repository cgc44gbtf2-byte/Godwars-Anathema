/* Parent-owned native progression; no live handler or trainer calls this yet. */
#include <sys/types.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../merc.h"
#include "state.h"
#include "specialization.h"

struct field_spec {
    long owner;
    size_t count, record_offset, native_offset;
    int character_field;
};

#define PC_FIELD(owner, member, native, count) \
    {owner, count, offsetof(HYBRID_PROGRESSION, member), \
     offsetof(PC_DATA, native), 0}
#define CH_FIELD(owner, member, native, count) \
    {owner, count, offsetof(HYBRID_PROGRESSION, member), \
     offsetof(CHAR_DATA, native), 1}

/* Enum order and wire order are explicit and stable. Do not reorder entries. */
static const struct field_spec fields[] = {
    PC_FIELD(CLASS_WEREWOLF, breed, breed, 3),
    PC_FIELD(CLASS_WEREWOLF, auspice, auspice, 5),
    PC_FIELD(CLASS_WEREWOLF, tribes, tribes, 20),
    PC_FIELD(CLASS_WEREWOLF, selected_gifts, tgift, 3),
    PC_FIELD(CLASS_WEREWOLF, wolf_rank, rank, 1),
    PC_FIELD(CLASS_SORCERER, colors, colors, 6),
    PC_FIELD(CLASS_HIGHLANDER, highland, highland, 4),
    PC_FIELD(CLASS_NINJA, ninja, ninja, 5),
    PC_FIELD(CLASS_MONK, monk, monk, 4),
    PC_FIELD(CLASS_DROW, drow, drow, 2),
    PC_FIELD(CLASS_DRAGON, dragon_color, dragcol, 1),
    PC_FIELD(CLASS_DRAGON, dragon_skills, dragskills, 1),
    CH_FIELD(CLASS_MAGE, spheres, spheres, 9),
    CH_FIELD(CLASS_MAGE, quint, quint, 3),
    CH_FIELD(CLASS_MAGE, arete, arete, 1),
    CH_FIELD(CLASS_MAGE, paradox, paradox, 1),
    CH_FIELD(CLASS_MAGE, paradox_ward, paradox_ward, 1)
};
#undef PC_FIELD
#undef CH_FIELD
_Static_assert(sizeof(fields) / sizeof(fields[0]) == HYBRID_PROGRESS_KIND_COUNT,
               "progression field inventory");

/* memcpy keeps these metadata accesses const-correct and independent of
 * pointer alignment/aliasing. Offsets address explicit long fields only. */
static long read_field(const void *record, size_t offset, size_t index)
{
    long value;
    memcpy(&value, (const unsigned char *)record + offset + index * sizeof(long),
           sizeof(value));
    return value;
}

static void write_field(void *record, size_t offset, size_t index, long value)
{
    memcpy((unsigned char *)record + offset + index * sizeof(long), &value,
           sizeof(value));
}

static const struct field_spec *find_field(enum hybrid_progression_kind kind)
{
    if ((unsigned int)kind >= HYBRID_PROGRESS_KIND_COUNT) return NULL;
    return &fields[kind];
}

static size_t gift_count(enum hybrid_gift_family family)
{
    switch (family) {
    case HYBRID_GIFT_BREED: return BREED_MAX + 1;
    case HYBRID_GIFT_AUSPICE: return AUSPICE_AHROUN + 1;
    case HYBRID_GIFT_TRIBE: return TRIBE_WENDIGOS + 1;
    default: return 0;
    }
}

static int native_range(enum hybrid_progression_kind kind, size_t index,
                        long value)
{
    if (kind == HYBRID_PROGRESS_PARADOX)
        return 1; /* Native backlash can legitimately leave a negative value. */
    if (kind == HYBRID_PROGRESS_SELECTED_GIFT) {
        size_t count = gift_count((enum hybrid_gift_family)index);
        return value >= -1 && value < (long)count;
    }
    if (kind == HYBRID_PROGRESS_DRAGON_COLOR)
        return value >= -1 && value <= BLACK_MAGIC;
    if (value < 0) return 0;
    switch (kind) {
    case HYBRID_PROGRESS_BREED:
    case HYBRID_PROGRESS_AUSPICE: return value <= 5;
    case HYBRID_PROGRESS_TRIBE:
        return index <= TRIBE_WENDIGOS ? value <= 5 : value == 0;
    case HYBRID_PROGRESS_WOLF_RANK:
    case HYBRID_PROGRESS_COLOR:
    case HYBRID_PROGRESS_SPHERE: return value <= 5;
    case HYBRID_PROGRESS_HIGHLAND: return value <= 6;
    case HYBRID_PROGRESS_NINJA:
        if (index == NINJA_KI) return value <= 300;
        if (index == NINJA_HARAKIRI) return 1;
        return value <= 10;
    case HYBRID_PROGRESS_MONK: return index < 3 ? value <= 8 : value == 0;
    case HYBRID_PROGRESS_DROW:
        if (index == DROW_SPEED) return value <= 4;
        return (value & ~((DROW_MAX * 2L - 1) | DROW_GLOBE)) == 0;
    case HYBRID_PROGRESS_DRAGON_SKILLS:
        return (value & ~((DRAG_SHIELD * 2L - 1) |
                         DRAG_WINGSOUT | DRAG_HORNSOUT)) == 0;
    case HYBRID_PROGRESS_ARETE: return value <= 10;
    default: return 1;
    }
}

int hybrid_progression_prepare(long parent, HYBRID_PROGRESSION *destination)
{
    HYBRID_PROGRESSION candidate = {0};
    if (!destination || !hybrid_parent_name(parent)) return 0;
    if (parent == CLASS_WEREWOLF) {
        for (size_t i = 0; i < 3; ++i) candidate.selected_gifts[i] = -1;
    } else if (parent == CLASS_DRAGON) candidate.dragon_color = -1;
    if (hybrid_specialization_count(parent)) candidate.full_branch = -1;
    *destination = candidate;
    return 1;
}

int hybrid_progression_valid(long parent, const HYBRID_PROGRESSION *record)
{
    if (!record || !hybrid_parent_name(parent)) return 0;
    for (size_t k = 0; k < HYBRID_PROGRESS_KIND_COUNT; ++k) {
        const struct field_spec *f = &fields[k];
        for (size_t i = 0; i < f->count; ++i) {
            long value = read_field(record, f->record_offset, i);
            if (parent != f->owner) {
                if (value != 0) return 0;
            } else if (!native_range((enum hybrid_progression_kind)k, i, value))
                return 0;
        }
    }
    if (hybrid_specialization_count(parent)) {
        if (record->full_branch != -1 &&
            !hybrid_specialization_branch_valid(parent, record->full_branch)) return 0;
    } else if (record->full_branch != 0) return 0;
    if (parent == CLASS_VAMPIRE) {
        if (record->vampire_profile < HYBRID_VAMPIRE_UNINITIALIZED ||
            record->vampire_profile > HYBRID_VAMPIRE_CLANLESS) return 0;
    } else if (record->vampire_profile != 0) return 0;
    return 1;
}

static int actor_matches(const CHAR_DATA *ch, const HYBRID_STATE *state, long parent)
{
    return ch && !IS_NPC(ch) && ch->pcdata && hybrid_state_valid(state) &&
           ch->class == state->primary_class &&
           (parent == state->primary_class || parent == state->secondary_class);
}

int hybrid_progression_record_read(long parent, const HYBRID_PROGRESSION *record,
                                   enum hybrid_progression_kind kind,
                                   size_t index, long *destination)
{
    const struct field_spec *f = find_field(kind);
    if (!destination || !f || f->owner != parent || index >= f->count ||
        !hybrid_progression_valid(parent, record)) return 0;
    *destination = read_field(record, f->record_offset, index);
    return 1;
}

int hybrid_parent_progression_read(const CHAR_DATA *ch, const HYBRID_STATE *state,
                                  long parent, enum hybrid_progression_kind kind,
                                  size_t index, long *destination)
{
    const struct field_spec *f = find_field(kind);
    const void *base;
    size_t offset;
    if (!destination || !f || f->owner != parent || index >= f->count ||
        !actor_matches(ch, state, parent)) return 0;
    if (parent == state->primary_class) {
        base = f->character_field ? (const void *)ch : (const void *)ch->pcdata;
        offset = f->native_offset;
    } else {
        base = &state->secondary_progression;
        offset = f->record_offset;
    }
    *destination = read_field(base, offset, index);
    return 1;
}

int hybrid_secondary_progression_write(HYBRID_STATE *state, long parent,
                                      enum hybrid_progression_kind kind,
                                      size_t index, long value)
{
    const struct field_spec *f = find_field(kind);
    HYBRID_STATE candidate;
    if (!f || f->owner != parent || index >= f->count ||
        !hybrid_state_valid(state) || state->secondary_class != parent) return 0;
    memcpy(&candidate, state, sizeof(candidate));
    write_field(&candidate.secondary_progression, f->record_offset, index, value);
    if (!hybrid_state_valid(&candidate)) return 0;
    memcpy(state, &candidate, sizeof(*state));
    return 1;
}

int hybrid_secondary_gift_choose(HYBRID_STATE *state, long parent,
                                enum hybrid_gift_family family)
{
    return parent == CLASS_WEREWOLF && gift_count(family) &&
           hybrid_secondary_specialize(state, parent, family);
}

int hybrid_parent_gift_training_cap(const CHAR_DATA *ch, const HYBRID_STATE *state,
                                   long parent, enum hybrid_gift_family family,
                                   size_t index, long *destination)
{
    return parent == CLASS_WEREWOLF && gift_count(family) &&
        hybrid_parent_training_cap(ch, state, parent,
            (enum hybrid_training_tree)family, index, destination);
}

int hybrid_parent_gift_rank(const CHAR_DATA *ch, const HYBRID_STATE *state,
                           long parent, enum hybrid_gift_family family,
                           size_t index, long *destination)
{
    return parent == CLASS_WEREWOLF && gift_count(family) &&
        hybrid_parent_rank_read(ch, state, parent,
            (enum hybrid_training_tree)family, index, destination);
}

int hybrid_progression_export(const HYBRID_PROGRESSION *record,
                              long *values, size_t count)
{
    size_t word = 0;
    if (!record || !values || count != HYBRID_PROGRESSION_WORDS) return 0;
    for (size_t k = 0; k < HYBRID_PROGRESS_KIND_COUNT; ++k)
        for (size_t i = 0; i < fields[k].count; ++i) {
            if (word >= count - 2) return 0;
            values[word++] = read_field(record, fields[k].record_offset, i);
        }
    if (word != count - 2) return 0;
    values[word++] = record->full_branch;
    values[word++] = record->vampire_profile;
    return word == count;
}

int hybrid_progression_import(long parent, const long *values, size_t count,
                              HYBRID_PROGRESSION *destination)
{
    HYBRID_PROGRESSION candidate = {0};
    size_t word = 0;
    if (!values || !destination || count != HYBRID_PROGRESSION_WORDS) return 0;
    for (size_t k = 0; k < HYBRID_PROGRESS_KIND_COUNT; ++k)
        for (size_t i = 0; i < fields[k].count; ++i) {
            if (word >= count - 2) return 0;
            write_field(&candidate, fields[k].record_offset, i, values[word++]);
        }
    if (word != count - 2) return 0;
    candidate.full_branch = values[word++];
    candidate.vampire_profile = values[word++];
    if (word != count || !hybrid_progression_valid(parent, &candidate)) return 0;
    *destination = candidate;
    return 1;
}
