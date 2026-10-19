/* Full-engine linkage; synthetic characters and disposable codec streams.
 * No player-file reads/writes, live character migration or power activation. */
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"
#include "../Genesis/src/classes/hybrid/state.h"

static CHAR_DATA actor;
static PC_DATA player;

static void fresh(long parent)
{
    memset(&actor, 0, sizeof(actor));
    memset(&player, 0, sizeof(player));
    actor.class = parent;
    actor.pcdata = &player;
    actor.hit = actor.max_hit = 50000;
    actor.mana = actor.max_mana = 50000;
    player.quest = 50000;
    for (size_t i = 0; i < 20; ++i) player.powers[i] = 1000 + (long)i;
    player.classpoints[0] = 123456;
    player.classpoints[1] = 234567;
    player.ninja[0] = 99;
    player.monk[0] = 88;
    player.highland[0] = 77;
    actor.vampgen = 7;
    actor.blood[BLOOD_CURRENT] = 41;
    actor.blood[BLOOD_POOL] = 97;
    actor.beast = 31;
    actor.truebeast = 42;
    player.wolf = 37;
    player.celerity = 53;
    player.cland[0] = 2;
    player.cland[1] = 3;
    player.cland[2] = 4;
}

static void unchanged(const unsigned char *before, const unsigned char *pc_before)
{
    assert(!memcmp(before, &actor, sizeof(actor)));
    assert(!memcmp(pc_before, &player, sizeof(player)));
}

static void roundtrip(const HYBRID_STATE *state)
{
    HYBRID_STATE loaded;
    FILE *fp = tmpfile();
    assert(fp);
    assert(hybrid_state_write(fp, state));
    assert(fputs("NEXT\n", fp) >= 0);
    assert(fflush(fp) == 0);
    rewind(fp);
    memset(&loaded, 0xa5, sizeof(loaded));
    assert(hybrid_state_read(fp, &loaded));
    assert(hybrid_state_valid(&loaded));
    assert(loaded.hybrid_id == state->hybrid_id);
    assert(loaded.primary_class == state->primary_class);
    assert(loaded.secondary_class == state->secondary_class);
    assert(!memcmp(loaded.secondary_powers, state->secondary_powers,
                   sizeof(loaded.secondary_powers)));
    assert(!memcmp(loaded.secondary_classpoints, state->secondary_classpoints,
                   sizeof(loaded.secondary_classpoints)));
    assert(loaded.secondary_generation == state->secondary_generation);
    assert(!memcmp(loaded.secondary_branches, state->secondary_branches,
                   sizeof(loaded.secondary_branches)));
    assert(loaded.secondary_rage == state->secondary_rage);
    assert(loaded.secondary_speed_timer == state->secondary_speed_timer);
    assert(!memcmp(loaded.secondary_blood, state->secondary_blood,
                   sizeof(loaded.secondary_blood)));
    assert(loaded.secondary_beast == state->secondary_beast);
    assert(loaded.secondary_truebeast == state->secondary_truebeast);
    long loaded_values[HYBRID_PROGRESSION_WORDS], expected_values[HYBRID_PROGRESSION_WORDS];
    assert(hybrid_progression_export(&loaded.secondary_progression, loaded_values,
                                     HYBRID_PROGRESSION_WORDS));
    assert(hybrid_progression_export(&state->secondary_progression, expected_values,
                                     HYBRID_PROGRESSION_WORDS));
    assert(!memcmp(loaded_values, expected_values, sizeof(loaded_values)));
    char next[16];
    assert(fgets(next, sizeof(next), fp) && !strcmp(next, "NEXT\n"));
    assert(fclose(fp) == 0);
}

static void all_pairs(void)
{
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        for (size_t order = 0; order < 2; ++order) {
            HYBRID_STATE state;
            unsigned char before[sizeof(actor)], pc_before[sizeof(player)];
            fresh(h->parents[order]);
            memcpy(before, &actor, sizeof(actor));
            memcpy(pc_before, &player, sizeof(player));
            memset(&state, 0xa5, sizeof(state));
            assert(hybrid_state_prepare(&actor, h, &state));
            unchanged(before, pc_before);
            assert(hybrid_state_valid(&state));
            assert(state.primary_class == h->parents[order]);
            assert(state.secondary_class == h->parents[1 - order]);
            for (size_t j = 0; j < 20; ++j) assert(state.secondary_powers[j] == 0);
            assert(state.secondary_classpoints[0] == 0 &&
                   state.secondary_classpoints[1] == 0);
            roundtrip(&state);
            size_t count = 999;
            assert(!hybrid_secondary_bank(&state, state.primary_class,
                                          HYBRID_BANK_POWERS, &count));
            assert(count == 0);
            long *powers = hybrid_secondary_bank(&state, state.secondary_class,
                                                 HYBRID_BANK_POWERS, &count);
            if (state.secondary_class == CLASS_VAMPIRE ||
                state.secondary_class == CLASS_ELEM) {
                assert(powers == state.secondary_powers && count == 20);
                assert(powers != player.powers);
                powers[0] = 1;
                powers[19] = LONG_MAX;
            } else {
                assert(!powers && count == 0);
            }
            long *points = hybrid_secondary_bank(&state, state.secondary_class,
                                                 HYBRID_BANK_CLASSPOINTS, &count);
            if (state.secondary_class == CLASS_DROW ||
                state.secondary_class == CLASS_DRAGON) {
                assert(points == state.secondary_classpoints && count == 2);
                assert(points != player.classpoints);
                points[0] = 5;
                points[1] = LONG_MAX;
            } else {
                assert(!points && count == 0);
            }
            assert(!hybrid_secondary_bank(&state, state.secondary_class,
                                          (enum hybrid_bank_kind)999, &count));
            assert(count == 0);
            unchanged(before, pc_before);
            roundtrip(&state);
        }
    }
}

static void invalid_prepare(void)
{
    HYBRID_STATE state, before;
    memset(&state, 0x37, sizeof(state));
    memcpy(&before, &state, sizeof(state));
    fresh(CLASS_VAMPIRE);
    assert(!hybrid_state_prepare(NULL, hybrid_by_id(45), &state));
    assert(!hybrid_state_prepare(&actor, NULL, &state));
    assert(!hybrid_state_prepare(&actor, hybrid_by_id(45), NULL));
    assert(!hybrid_state_prepare(&actor, hybrid_by_id(1), &state));
    HYBRID_DEFINITION forged = *hybrid_by_id(45);
    assert(!hybrid_state_prepare(&actor, &forged, &state));
    actor.class = CLASS_VAMPIRE | CLASS_WEREWOLF;
    assert(!hybrid_state_prepare(&actor, hybrid_by_id(45), &state));
    fresh(CLASS_VAMPIRE);
    SET_BIT(actor.act, ACT_IS_NPC);
    assert(!hybrid_state_prepare(&actor, hybrid_by_id(45), &state));
    fresh(CLASS_VAMPIRE);
    actor.pcdata = NULL;
    assert(!hybrid_state_prepare(&actor, hybrid_by_id(45), &state));
    assert(!memcmp(&state, &before, sizeof(state)));
}

static void bad_state(HYBRID_STATE state)
{
    assert(!hybrid_state_valid(&state));
    size_t count = 1;
    assert(!hybrid_secondary_bank(&state, state.secondary_class,
                                  HYBRID_BANK_POWERS, &count) && count == 0);
    long output = -999;
    HYBRID_STATE before;
    memcpy(&before, &state, sizeof(state));
    assert(!hybrid_parent_value_read(&actor, &state, state.primary_class,
                                     HYBRID_VALUE_POWER, 0, &output));
    assert(output == -999);
    assert(!hybrid_secondary_value_write(&state, state.secondary_class,
                                         HYBRID_VALUE_POWER, 0, 0));
    assert(!memcmp(&before, &state, sizeof(state)));
    FILE *fp = tmpfile();
    assert(fp && !hybrid_state_write(fp, &state) && ftell(fp) == 0);
    assert(fclose(fp) == 0);
}

static void invalid_states(void)
{
    HYBRID_STATE base, state;
    fresh(CLASS_VAMPIRE);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(45), &base));
    state = base; state.hybrid_id = 0; bad_state(state);
    state = base; state.hybrid_id = LONG_MAX; bad_state(state);
    state = base; state.secondary_class = CLASS_VAMPIRE; bad_state(state);
    state = base; state.secondary_class = CLASS_DROW; bad_state(state);
    state = base; state.primary_class = -1; bad_state(state);
    state = base; state.secondary_powers[0] = 1; bad_state(state);
    state = base; state.secondary_classpoints[0] = 1; bad_state(state);
    fresh(CLASS_ELEM);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(23), &state));
    state.secondary_powers[0] = -2; bad_state(state);
    fresh(CLASS_DROW);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(1), &state));
    state.secondary_classpoints[1] = -1; bad_state(state);
    assert(!hybrid_state_valid(NULL));
}

/* Deliberately explicit ownership table, separate from implementation dispatch. */
static size_t owned_count(long parent, enum hybrid_value_kind kind)
{
    static const struct {
        long parent;
        size_t counts[9];
    } owners[] = {
        {CLASS_DRAGON,     {0,2,1,0,0,0,0,0,0}},
        {CLASS_DROW,       {0,2,0,0,0,0,0,0,0}},
        {CLASS_ELEM,       {20,0,0,0,0,0,0,0,0}},
        {CLASS_HIGHLANDER, {0,0,0,0,0,1,0,0,0}},
        {CLASS_MAGE,       {0,0,0,3,0,0,0,0,0}},
        {CLASS_MONK,       {0,0,0,0,0,0,0,0,0}},
        {CLASS_NINJA,      {0,0,0,0,1,0,0,0,0}},
        {CLASS_SORCERER,   {0,0,0,0,0,0,0,0,0}},
        {CLASS_VAMPIRE,    {20,0,1,3,1,1,2,1,1}},
        {CLASS_WEREWOLF,   {0,0,0,0,1,0,0,0,0}}
    };
    if ((int)kind < 0 || kind > HYBRID_VALUE_TRUEBEAST) return 0;
    for (size_t i = 0; i < sizeof(owners) / sizeof(owners[0]); ++i)
        if (owners[i].parent == parent) return owners[i].counts[kind];
    return 0;
}

static long native_value(enum hybrid_value_kind kind, size_t index)
{
    switch (kind) {
    case HYBRID_VALUE_POWER: return player.powers[index];
    case HYBRID_VALUE_CLASSPOINT: return player.classpoints[index];
    case HYBRID_VALUE_GENERATION: return actor.vampgen;
    case HYBRID_VALUE_BRANCH: return player.cland[index];
    case HYBRID_VALUE_RAGE: return player.wolf;
    case HYBRID_VALUE_SPEED_TIMER: return player.celerity;
    case HYBRID_VALUE_BLOOD: return actor.blood[index];
    case HYBRID_VALUE_BEAST: return actor.beast;
    case HYBRID_VALUE_TRUEBEAST: return actor.truebeast;
    }
    assert(0);
    return 0;
}

static void parent_values(void)
{
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        for (size_t order = 0; order < 2; ++order) {
            HYBRID_STATE state, prior;
            unsigned char before[sizeof(actor)], pc_before[sizeof(player)];
            fresh(h->parents[order]);
            memcpy(before, &actor, sizeof(actor));
            memcpy(pc_before, &player, sizeof(player));
            assert(hybrid_state_prepare(&actor, h, &state));
            assert(state.secondary_generation == 0);
            assert(state.secondary_rage == 0 && state.secondary_speed_timer == 0);
            assert(state.secondary_blood[0] == 0 && state.secondary_blood[1] == 0);
            assert(state.secondary_beast == 0 && state.secondary_truebeast == 0);
            for (size_t b = 0; b < 3; ++b)
                assert(state.secondary_branches[b] ==
                       ((state.secondary_class == CLASS_VAMPIRE ||
                         state.secondary_class == CLASS_MAGE) ? -1 : 0));

            /* Set resource capacity before current; no prepare-time free blood. */
            if (state.secondary_class == CLASS_VAMPIRE)
                assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                                    HYBRID_VALUE_BLOOD, 1, 100));
            for (int k = -1; k <= (int)HYBRID_VALUE_TRUEBEAST + 1; ++k) {
                enum hybrid_value_kind kind = (enum hybrid_value_kind)k;
                for (size_t slot = 0; slot < 22; ++slot) {
                    long output = -999;
                    memcpy(&prior, &state, sizeof(prior));
                    int primary_ok = hybrid_parent_value_read(
                        &actor, &state, state.primary_class, kind, slot, &output);
                    assert(primary_ok == (slot < owned_count(state.primary_class, kind)));
                    assert(output == (primary_ok ? native_value(kind, slot) : -999));
                    assert(!hybrid_secondary_value_write(
                        &state, state.primary_class, kind, slot, 1));
                    assert(!memcmp(&prior, &state, sizeof(state)));

                    int secondary_ok = slot < owned_count(state.secondary_class, kind);
                    assert(hybrid_secondary_value_write(
                        &state, state.secondary_class, kind, slot, 1) == secondary_ok);
                    if (!secondary_ok) assert(!memcmp(&prior, &state, sizeof(state)));
                    output = -999;
                    assert(hybrid_parent_value_read(
                        &actor, &state, state.secondary_class, kind, slot, &output)
                           == secondary_ok);
                    assert(output == (secondary_ok ? 1 : -999));
                    unchanged(before, pc_before);
                }
            }
            roundtrip(&state);
            memcpy(&prior, &state, sizeof(state));
            long output = -999;
            assert(!hybrid_parent_value_read(NULL, &state, state.secondary_class,
                                            HYBRID_VALUE_POWER, 0, &output));
            assert(!hybrid_parent_value_read(&actor, NULL, state.secondary_class,
                                            HYBRID_VALUE_POWER, 0, &output));
            assert(!hybrid_parent_value_read(&actor, &state, 0,
                                            HYBRID_VALUE_POWER, 0, &output));
            assert(!hybrid_parent_value_read(&actor, &state, state.secondary_class,
                                            HYBRID_VALUE_POWER, 0, NULL));
            assert(!hybrid_secondary_value_write(NULL, state.secondary_class,
                                                 HYBRID_VALUE_POWER, 0, 1));
            for (int k = 0; k <= (int)HYBRID_VALUE_TRUEBEAST; ++k) {
                enum hybrid_value_kind kind = (enum hybrid_value_kind)k;
                assert(!hybrid_secondary_value_write(
                    &state, state.secondary_class, kind, (size_t)-1, 1));
            }
            actor.class = 0;
            assert(!hybrid_parent_value_read(&actor, &state, state.secondary_class,
                                            HYBRID_VALUE_POWER, 0, &output));
            actor.class = state.primary_class;
            SET_BIT(actor.act, ACT_IS_NPC);
            assert(!hybrid_parent_value_read(&actor, &state, state.secondary_class,
                                            HYBRID_VALUE_POWER, 0, &output));
            REMOVE_BIT(actor.act, ACT_IS_NPC);
            actor.pcdata = NULL;
            assert(!hybrid_parent_value_read(&actor, &state, state.secondary_class,
                                            HYBRID_VALUE_POWER, 0, &output));
            actor.pcdata = &player;
            assert(output == -999 && !memcmp(&prior, &state, sizeof(state)));
            unchanged(before, pc_before);
        }
    }
}

static void core_limits(void)
{
    HYBRID_STATE state, before;
    fresh(CLASS_ELEM);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(23), &state));
    assert(state.secondary_class == CLASS_VAMPIRE);
    const struct {
        enum hybrid_value_kind kind;
        size_t index;
        long value;
    } bad[] = {
        {HYBRID_VALUE_GENERATION,0,-1}, {HYBRID_VALUE_GENERATION,0,14},
        {HYBRID_VALUE_GENERATION,0,LONG_MAX},
        {HYBRID_VALUE_BRANCH,0,-2}, {HYBRID_VALUE_BRANCH,2,DISC_MAX+1},
        {HYBRID_VALUE_RAGE,0,-1}, {HYBRID_VALUE_SPEED_TIMER,0,-1},
        {HYBRID_VALUE_BLOOD,0,1}, {HYBRID_VALUE_BLOOD,1,-1},
        {HYBRID_VALUE_BEAST,0,-1}, {HYBRID_VALUE_TRUEBEAST,0,-1},
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        memcpy(&before, &state, sizeof(state));
        assert(!hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                             bad[i].kind, bad[i].index, bad[i].value));
        assert(!memcmp(&before, &state, sizeof(state)));
    }
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_BLOOD, 1, LONG_MAX));
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_BLOOD, 0, LONG_MAX));
    memcpy(&before, &state, sizeof(state));
    assert(!hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                         HYBRID_VALUE_BLOOD, 1, LONG_MAX-1));
    assert(!memcmp(&before, &state, sizeof(state)));
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_RAGE, 0, LONG_MAX));
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_SPEED_TIMER, 0, LONG_MAX));
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_GENERATION, 0, 13));
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_BRANCH, 0, DISC_MAX));
    assert(hybrid_secondary_value_write(&state, CLASS_VAMPIRE,
                                        HYBRID_VALUE_BRANCH, 1, -1));
    roundtrip(&state);
    fresh(CLASS_DROW);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(1), &state));
    assert(!hybrid_secondary_value_write(&state, CLASS_DRAGON,
                                         HYBRID_VALUE_GENERATION,0,AGE_HATCHLING+1));
    assert(hybrid_secondary_value_write(&state, CLASS_DRAGON,
                                        HYBRID_VALUE_GENERATION,0,AGE_HATCHLING));
    fresh(CLASS_VAMPIRE);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(34), &state));
    assert(hybrid_secondary_value_write(&state, CLASS_MAGE,
                                        HYBRID_VALUE_BRANCH, 0, SPHERE_TIME));
    assert(!hybrid_secondary_value_write(&state, CLASS_MAGE,
                                         HYBRID_VALUE_BRANCH, 0, SPHERE_TIME+1));
    /* Direct corruption must block reads/writes and serialization too. */
    state.secondary_rage = 1;
    bad_state(state);
}

static void rejects(const char *record, size_t length)
{
    HYBRID_STATE destination, before;
    memset(&destination, 0x5a, sizeof(destination));
    memcpy(&before, &destination, sizeof(before));
    FILE *fp = tmpfile();
    assert(fp);
    assert(fwrite(record, 1, length, fp) == length && fflush(fp) == 0);
    rewind(fp);
    assert(!hybrid_state_read(fp, &destination));
    assert(!memcmp(&destination, &before, sizeof(before)));
    assert(fclose(fp) == 0);
}

static void codec_errors(void)
{
    HYBRID_STATE state;
    fresh(CLASS_WEREWOLF);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(45), &state));
    state.secondary_powers[19] = LONG_MAX;
    FILE *fp = tmpfile();
    assert(fp && hybrid_state_write(fp, &state) && fflush(fp) == 0);
    rewind(fp);
    char good[1024], changed[2048];
    assert(fgets(good, sizeof(good), fp));
    assert(fclose(fp) == 0);
    size_t length = strlen(good);
    for (size_t i = 0; i < length; ++i) rejects(good, i);
    for (size_t i = 0; i < length; ++i) {
        memcpy(changed, good, length);
        changed[i] = (char)((unsigned char)changed[i] ^ 0x80);
        rejects(changed, length);
    }
    const char *banks = good + strlen("AnathemaHybrid 3 45 2 1");
    snprintf(changed, sizeof(changed), "AnathemaHybrid 3 46 2 1%s", banks);
    rejects(changed, strlen(changed));
    snprintf(changed, sizeof(changed), "AnathemaHybrid 3 45 1 1%s", banks);
    rejects(changed, strlen(changed));
    /* Same pair, wrong bank owner: Werewolf cannot own Vampire power slots. */
    snprintf(changed, sizeof(changed), "AnathemaHybrid 3 45 1 2%s", banks);
    rejects(changed, strlen(changed));
    /* -1 is native Vampire "unknown"; values below that are malformed. */
    assert(banks[0] == ' ' && banks[1] == '0');
    snprintf(changed, sizeof(changed), "AnathemaHybrid 3 45 2 1 -2%s", banks + 2);
    rejects(changed, strlen(changed));
    strcpy(changed, good);
    changed[strlen("AnathemaHybrid ")] = '4';
    rejects(changed, length);
    snprintf(changed, sizeof(changed), "%.*s 99\n", (int)length - 1, good);
    rejects(changed, strlen(changed));
    strcpy(changed, good);
    changed[22] = '\0';
    rejects(changed, length);
    memset(changed, '9', sizeof(changed));
    changed[sizeof(changed) - 1] = '\n';
    rejects(changed, sizeof(changed));
    const char overflow[] =
        "AnathemaHybrid 1 999999999999999999999999999999999999999999 1 2\n";
    rejects(overflow, sizeof(overflow) - 1);
    const char joined[] = "AnathemaHybrid 1x 45 1 2\n";
    rejects(joined, sizeof(joined) - 1);
    const char unknown[] = "AnathemaHybridX 1 45 1 2\n";
    rejects(unknown, sizeof(unknown) - 1);
    assert(!hybrid_state_read(NULL, &state));
    assert(!hybrid_state_write(NULL, &state));
    fp = tmpfile();
    assert(fp && !hybrid_state_read(fp, NULL) && fclose(fp) == 0);
    fp = fopen("/dev/full", "w");
    assert(fp && setvbuf(fp, NULL, _IONBF, 0) == 0);
    assert(!hybrid_state_write(fp, &state));
    fclose(fp);
    /* Valid CRLF record is accepted; no dependence on host text line endings. */
    snprintf(changed, sizeof(changed), "%.*s\r\n", (int)length - 1, good);
    fp = tmpfile();
    assert(fp && fputs(changed, fp) >= 0 && fflush(fp) == 0);
    rewind(fp);
    assert(hybrid_state_read(fp, &state));
    assert(fclose(fp) == 0);
}

static void legacy_records(void)
{
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        for (size_t order = 0; order < 2; ++order) {
            HYBRID_STATE state;
            FILE *fp = tmpfile();
            assert(fp);
            assert(fprintf(fp, "AnathemaHybrid 1 %ld %ld %ld",
                           h->id, h->parents[order], h->parents[1-order]) > 0);
            for (size_t j = 0; j < 22; ++j) {
                long parent = h->parents[1-order];
                long value = j < 20 ?
                    ((parent == CLASS_VAMPIRE || parent == CLASS_ELEM) ?
                     (long)j + 1 : 0) :
                    ((parent == CLASS_DROW || parent == CLASS_DRAGON) ?
                     (long)j + 1 : 0);
                assert(fprintf(fp, " %ld", value) > 0);
            }
            assert(fputs("\nNEXT\n", fp) >= 0 && fflush(fp) == 0);
            rewind(fp);
            memset(&state, 0xa5, sizeof(state));
            assert(hybrid_state_read(fp, &state));
            assert(state.hybrid_id == h->id &&
                   state.primary_class == h->parents[order] &&
                   state.secondary_class == h->parents[1-order]);
            assert(state.secondary_generation == 0 &&
                   state.secondary_rage == 0 && state.secondary_speed_timer == 0);
            assert(state.secondary_blood[0] == 0 && state.secondary_blood[1] == 0);
            assert(state.secondary_beast == 0 && state.secondary_truebeast == 0);
            for (size_t j = 0; j < 20; ++j)
                assert(state.secondary_powers[j] ==
                       ((state.secondary_class == CLASS_VAMPIRE ||
                         state.secondary_class == CLASS_ELEM) ? (long)j+1 : 0));
            for (size_t j = 0; j < 2; ++j)
                assert(state.secondary_classpoints[j] ==
                       ((state.secondary_class == CLASS_DROW ||
                         state.secondary_class == CLASS_DRAGON) ? (long)j+21 : 0));
            for (size_t j = 0; j < 3; ++j)
                assert(state.secondary_branches[j] ==
                       ((state.secondary_class == CLASS_MAGE ||
                         state.secondary_class == CLASS_VAMPIRE) ? -1 : 0));
            char next[16];
            assert(fgets(next, sizeof(next), fp) && !strcmp(next, "NEXT\n"));
            assert(fclose(fp) == 0);
            roundtrip(&state);
        }
    }
    const char *bad[] = {
        "AnathemaHybrid 1 45 2 1 -2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n",
        "AnathemaHybrid 1 45 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n",
        "AnathemaHybrid 2 45 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
        rejects(bad[i], strlen(bad[i]));
}

int main(void)
{
    all_pairs();
    invalid_prepare();
    invalid_states();
    parent_values();
    core_limits();
    codec_errors();
    legacy_records();
    puts("hybrid isolated values: 45 pairs/both orders, primary preservation, "
         "resource/timer ownership, atomic rejects and v1/v2/v3 codecs passed");
    return 0;
}
