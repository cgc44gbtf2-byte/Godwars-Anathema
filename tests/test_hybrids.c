/* Full-engine tests of the read-only hybrid registry and command.
 * Deliberately no player save, conversion, economy or power activation. */
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"
#include "../Genesis/src/classes/hybrid/hybrid.h"

static CHAR_DATA actor;
static PC_DATA player;
static DESCRIPTOR_DATA descriptor;
static char output[65536];

static const long parents[] = {
    CLASS_DRAGON, CLASS_DROW, CLASS_ELEM, CLASS_HIGHLANDER, CLASS_MAGE,
    CLASS_MONK, CLASS_NINJA, CLASS_SORCERER, CLASS_VAMPIRE, CLASS_WEREWOLF
};

static void registry(void)
{
    assert(hybrid_count() == 45);
    assert(HYBRID_DEFAULT_QUEST_COST == 50000);
    assert(!hybrid_at(45) && !hybrid_at((size_t)-1));
    assert(!hybrid_by_id(0) && !hybrid_by_id(-1));
    assert(!hybrid_by_id(46) && !hybrid_by_id(LONG_MIN));
    assert(!hybrid_by_id(LONG_MAX));
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        assert(h->id == (long)i + 1);
        assert(h->parents[0] != h->parents[1]);
        assert(hybrid_by_id(h->id) == h);
        assert(hybrid_by_name(h->name) == h);
        assert(hybrid_by_parents(h->parents[0], h->parents[1]) == h);
        assert(hybrid_by_parents(h->parents[1], h->parents[0]) == h);
        assert(hybrid_parent_name(h->parents[0]));
        assert(hybrid_parent_name(h->parents[1]));
        for (size_t j = i + 1; j < hybrid_count(); ++j)
            assert(strcmp(h->name, hybrid_at(j)->name));
    }
    size_t combinations = 0;
    for (size_t i = 0; i < 10; ++i) {
        size_t choices = 0;
        assert(!hybrid_by_parents(parents[i], parents[i]));
        assert(!hybrid_by_parents(parents[i], 0));
        assert(!hybrid_by_parents(parents[i], -1));
        for (size_t j = 0; j < hybrid_count(); ++j)
            choices += !!hybrid_has_parent(hybrid_at(j), parents[i]);
        assert(choices == 9);
        for (size_t j = i + 1; j < 10; ++j) {
            assert(hybrid_by_parents(parents[i], parents[j]));
            assert(!hybrid_by_parents(parents[i] | parents[j], parents[i]));
            ++combinations;
        }
    }
    assert(combinations == 45);
    assert(!hybrid_has_parent(NULL, CLASS_VAMPIRE));
    assert(!hybrid_has_parent(hybrid_at(0), CLASS_NONE));
    assert(!hybrid_by_name(NULL) && !hybrid_by_name(""));
    assert(!hybrid_by_name("Bloodmoon") && !hybrid_by_name("unknown"));
    assert(!hybrid_by_name("'") && !hybrid_by_name("''"));
    assert(hybrid_by_name(" \t\"bLoOdMoOn ReVeNaNt\"\r\n")->id == 45);
    assert(hybrid_by_name("'Umbral Drake'")->id == 1);
    const char high_bytes[] = {(char)0xff, (char)0x80, '\0'};
    assert(!hybrid_by_name(high_bytes));
    char huge[16384];
    memset(huge, 'x', sizeof(huge) - 1);
    huge[sizeof(huge) - 1] = '\0';
    assert(!hybrid_by_name(huge));
}

static void fresh(long class)
{
    memset(&actor, 0, sizeof(actor));
    memset(&player, 0, sizeof(player));
    memset(&descriptor, 0, sizeof(descriptor));
    memset(output, 0, sizeof(output));
    actor.name = "tester";
    actor.class = class;
    actor.level = LEV_AWA;
    actor.pcdata = &player;
    actor.desc = &descriptor;
    actor.hit = actor.max_hit = 123456;
    actor.mana = actor.max_mana = 654321;
    actor.gold = 777;
    player.quest = 888;
    player.powers[0] = 99;
    player.classpoints[0] = 444;
    player.ninja[0] = 77;
    descriptor.character = &actor;
    descriptor.connected = CON_PLAYING;
    descriptor.outbuf = output;
    descriptor.outsize = sizeof(output);
}

static void preview(char *argument)
{
    unsigned char character_before[sizeof(actor)], player_before[sizeof(player)];
    memcpy(character_before, &actor, sizeof(actor));
    memcpy(player_before, &player, sizeof(player));
    do_selfbrid(&actor, argument);
    assert(!memcmp(character_before, &actor, sizeof(actor)));
    assert(!memcmp(player_before, &player, sizeof(player)));
    output[descriptor.outtop] = '\0';
}

static void commands(void)
{
    fresh(CLASS_VAMPIRE);
    preview("list \t");
    assert(strstr(output, "conversion is not enabled"));
    for (size_t i = 0; i < hybrid_count(); ++i)
        assert(strstr(output, hybrid_at(i)->name));
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        for (size_t j = 0; j < 10; ++j) {
            char argument[256];
            fresh(parents[j]);
            snprintf(argument, sizeof(argument), "info %s", h->name);
            preview(argument);
            assert(strstr(output, h->name));
            assert(strstr(output, hybrid_has_parent(h, parents[j])
                ? "your current class matches one parent"
                : "your current class does not match either parent"));
            assert(strstr(output, "second parent starts untrained"));
            assert(strstr(output, "Master/Elite"));
            assert(strstr(output, "not configured"));
        }
    }
    fresh(CLASS_VAMPIRE | CLASS_WEREWOLF);
    preview("Bloodmoon Revenant");
    assert(strstr(output, "does not match either parent"));
    assert(!IS_VAMPIRE(&actor) && !IS_WEREWOLF(&actor));
    fresh(CLASS_NONE);
    preview("info");
    assert(strstr(output, "Unknown hybrid"));
    fresh(CLASS_VAMPIRE);
    preview("confirm Bloodmoon Revenant");
    assert(strstr(output, "Unknown hybrid"));
    const long balances[] = {LONG_MIN, -1, 0, 49999, 50000, 50001, LONG_MAX};
    for (size_t i = 0; i < sizeof(balances) / sizeof(balances[0]); ++i) {
        fresh(CLASS_VAMPIRE);
        player.quest = balances[i];
        preview("info Bloodmoon Revenant");
        assert(strstr(output, "50000 quest points"));
        assert(strstr(output, balances[i] >= HYBRID_DEFAULT_QUEST_COST
            ? "check: enough for the planned price"
            : "check: not enough for the planned price"));
    }
    fresh(CLASS_VAMPIRE);
    preview(NULL);
    assert(strstr(output, "Use: selfbrid info"));
    fresh(CLASS_VAMPIRE);
    SET_BIT(actor.act, ACT_IS_NPC);
    preview("list");
    assert(descriptor.outtop == 0);
    fresh(CLASS_VAMPIRE);
    actor.pcdata = NULL;
    preview("list");
    assert(descriptor.outtop == 0);
    fresh(CLASS_VAMPIRE);
    actor.desc = NULL;
    preview("Bloodmoon Revenant");
    assert(descriptor.outtop == 0);
    do_selfbrid(NULL, "list");
    size_t count = 0;
    for (size_t i = 0; cmd_table[i].name[0]; ++i)
        if (!strcmp(cmd_table[i].name, "selfbrid")) {
            ++count;
            assert(cmd_table[i].do_fun == do_selfbrid);
            assert(cmd_table[i].level == LEV_AWA);
        }
    assert(count == 1);
}

int main(void)
{
    registry();
    commands();
    puts("hybrid registry and read-only command regressions passed");
    return 0;
}
