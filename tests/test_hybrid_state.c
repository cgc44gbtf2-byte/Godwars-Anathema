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
    state.secondary_powers[0] = -1; bad_state(state);
    fresh(CLASS_DROW);
    assert(hybrid_state_prepare(&actor, hybrid_by_id(1), &state));
    state.secondary_classpoints[1] = -1; bad_state(state);
    assert(!hybrid_state_valid(NULL));
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
    const char *banks = good + strlen("AnathemaHybrid 1 45 2 1");
    snprintf(changed, sizeof(changed), "AnathemaHybrid 1 46 2 1%s", banks);
    rejects(changed, strlen(changed));
    snprintf(changed, sizeof(changed), "AnathemaHybrid 1 45 1 1%s", banks);
    rejects(changed, strlen(changed));
    /* Same pair, wrong bank owner: Werewolf cannot own Vampire power slots. */
    snprintf(changed, sizeof(changed), "AnathemaHybrid 1 45 1 2%s", banks);
    rejects(changed, strlen(changed));
    /* Replace the first zero rank with -1 while retaining the full field set. */
    assert(banks[0] == ' ' && banks[1] == '0');
    snprintf(changed, sizeof(changed), "AnathemaHybrid 1 45 2 1 -1%s", banks + 2);
    rejects(changed, strlen(changed));
    strcpy(changed, good);
    changed[strlen("AnathemaHybrid ")] = '2';
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

int main(void)
{
    all_pairs();
    invalid_prepare();
    invalid_states();
    codec_errors();
    puts("hybrid parent-bank and standalone codec regressions passed");
    return 0;
}
