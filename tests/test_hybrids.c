/* Full-engine tests of the read-only hybrid registry and command.
 * Deliberately no player save, conversion, economy or power activation. */
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"
#include "../Genesis/src/classes/hybrid/hybrid.h"

extern void init_mm(void);
static CHAR_DATA actor;
static PC_DATA player;
static DESCRIPTOR_DATA descriptor;
static char output[65536];

static void plain(char *text)
{
    char *in = text, *out = text;
    while (*in) {
        if (*in == '#' && in[1]) in += 2;
        else *out++ = *in++;
    }
    *out = '\0';
}

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
    /* Canonical text assertions ignore cosmetic MUD color markers. */
    plain(output);
}

static void cosmetics(void)
{
    for (size_t i = 0; i < hybrid_count(); i++) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        char first[128], again[128], clean[128];
        init_mm();
        long next = number_range(1, 1000000);
        init_mm();
        assert(hybrid_display_name(h, first, sizeof(first)));
        assert(hybrid_display_name(h, again, sizeof(again)));
        assert(!strcmp(first, again));
        assert(number_range(1, 1000000) == next);
        char *second = strchr(first + 2, '#');
        assert(second && second > first + 2);
        assert(first[1] != second[1]);
        assert(first[1] == hybrid_parent_color(h->parents[0])[1] ||
               first[1] == hybrid_parent_color(h->parents[1])[1]);
        assert(second[1] == hybrid_parent_color(h->parents[0])[1] ||
               second[1] == hybrid_parent_color(h->parents[1])[1]);
        assert(!strcmp(strchr(second + 2, '#'), "#n"));
        strcpy(clean, first);
        plain(clean);
        assert(!strcmp(clean, h->name));
        for (size_t size = 1; size <= strlen(first); size++) {
            struct { char text[128]; char guard; } tiny;
            memset(&tiny, 0xa5, sizeof(tiny));
            assert(!hybrid_display_name(h, tiny.text, size));
            assert(tiny.text[0] == '\0' && (unsigned char)tiny.guard == 0xa5);
            for (size_t j = size; j < sizeof(tiny.text); j++)
                assert((unsigned char)tiny.text[j] == 0xa5);
        }
        long cap = hybrid_book_chain_policy(h);
        if (!hybrid_has_parent(h, CLASS_SORCERER)) assert(cap == 0);
        else if (hybrid_has_parent(h, CLASS_MAGE)) assert(cap == 6);
        else if (hybrid_has_parent(h, CLASS_DROW) || hybrid_has_parent(h, CLASS_ELEM) ||
                 hybrid_has_parent(h, CLASS_VAMPIRE)) assert(cap == 4);
        else assert(cap == 3);
        assert(hybrid_book_chain_policy(hybrid_by_parents(h->parents[1],
                                                         h->parents[0])) == cap);
    }
    char buffer[16] = "sentinel";
    assert(!hybrid_display_name(NULL, buffer, sizeof(buffer)) && !buffer[0]);
    assert(!hybrid_display_name(hybrid_at(0), NULL, 10));
    assert(!hybrid_display_name(hybrid_at(0), buffer, 0));
    HYBRID_DEFINITION fake = *hybrid_at(0);
    assert(!hybrid_display_name(&fake, buffer, sizeof(buffer)));
    assert(!hybrid_book_chain_policy(&fake) && !hybrid_book_chain_policy(NULL));
    assert(!hybrid_parent_color(0));
}

static void help_request(char *argument)
{
    unsigned char before[sizeof(actor)], pc_before[sizeof(player)];
    memcpy(before, &actor, sizeof(actor));
    memcpy(pc_before, &player, sizeof(player));
    descriptor.outtop = 0;
    init_mm();
    long next = number_range(1, 1000000);
    init_mm();
    do_help(&actor, argument);
    assert(number_range(1, 1000000) == next);
    assert(!memcmp(before, &actor, sizeof(actor)));
    assert(!memcmp(pc_before, &player, sizeof(player)));
    output[descriptor.outtop] = '\0';
    plain(output);
}

static void help_topics(void)
{
    HELP_DATA *saved = help_first, topics[10];
    const char *keywords[] = {"Dragons", "Drow", "Elementalists", "Highlanders",
        "Mages", "Monks", "Ninja", "Sorcerers", "Vampires", "Werewolf"};
    memset(topics, 0, sizeof(topics));
    for (size_t i = 0; i < 10; i++) {
        topics[i].keyword = (char *)keywords[i];
        topics[i].text = "CUSTOM CLASS TEXT RETAINED\n\r";
        topics[i].next = i < 9 ? &topics[i + 1] : NULL;
    }
    help_first = topics;
    fresh(CLASS_NONE); /* Full HELP catalog is intentionally public. */
    help_request("hybrid");
    assert(strstr(output, "Hybrid catalog: 45"));
    assert(strstr(output, "Parent 1 is your original class"));
    size_t rows = 0;
    for (char *at = output; (at = strstr(at, "  help ")); at += 7) rows++;
    assert(rows == 90);
    for (size_t i = 0; i < hybrid_count(); i++) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        char row[128];
        snprintf(row, sizeof(row), "  help %s  (Parent 2:", h->name);
        char *first = strstr(output, row);
        assert(first);
        char *second = strstr(first + strlen(row), row);
        assert(second && !strstr(second + strlen(row), row));
    }
    help_request("selfbrid");
    assert(strstr(output, "Hybrid catalog: 45"));
    for (size_t p = 0; p < 10; p++) {
        help_request((char *)hybrid_parent_name(parents[p]));
        assert(strstr(output, "CUSTOM CLASS TEXT RETAINED"));
        for (size_t i = 0; i < hybrid_count(); i++) {
            const HYBRID_DEFINITION *h = hybrid_at(i);
            char row[128];
            snprintf(row, sizeof(row), "  help %s  (Parent 2:", h->name);
            assert(!!strstr(output, row) == !!hybrid_has_parent(h, parents[p]));
        }
    }
    for (size_t i = 0; i < hybrid_count(); i++) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        help_request((char *)h->name);
        assert(strstr(output, h->name) && strstr(output, "no playable conversion"));
        assert(strstr(output, hybrid_parent_name(h->parents[0])));
        assert(strstr(output, hybrid_parent_name(h->parents[1])));
        assert(!!strstr(output, "book-chain policy") == !!hybrid_book_chain_policy(h));
    }
    HELP_DATA custom = {0};
    custom.keyword = "'Bloodmoon Revenant'";
    custom.text = "CUSTOM HYBRID HELP TAKES PRIORITY";
    help_first = &custom;
    help_request("Bloodmoon Revenant");
    assert(strstr(output, custom.text) && !strstr(output, "planned hybrid"));
    help_request("unrecognizedword");
    assert(strstr(output, "No help on that word"));
    help_first = saved;
}

static void commands(void)
{
    for (size_t p = 0; p < 10; p++) {
        fresh(parents[p]);
        preview("list \t");
        assert(strstr(output, "conversion is not enabled"));
        size_t choices = 0;
        for (size_t i = 0; i < hybrid_count(); i++) {
            const HYBRID_DEFINITION *h = hybrid_at(i);
            char row[256];
            snprintf(row, sizeof(row), "%3ld  %-21s ", h->id, h->name);
            if (hybrid_has_parent(h, parents[p])) {
                assert(strstr(output, row));
                choices++;
            } else assert(!strstr(output, row));
        }
        assert(choices == 9);
    }
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        for (size_t j = 0; j < 10; ++j) {
            char argument[256];
            fresh(parents[j]);
            snprintf(argument, sizeof(argument), "info %s", h->name);
            preview(argument);
            if (!hybrid_has_parent(h, parents[j])) {
                assert(!strstr(output, h->name));
                assert(strstr(output, "unavailable to your current base class"));
                assert(!strstr(output, "Planned conversion price"));
                assert(!strstr(output, "Numeric prerequisites"));
                continue;
            }
            assert(strstr(output, h->name));
            assert(strstr(output, "your current class matches one parent"));
            assert(strstr(output, "second parent starts untrained"));
            assert(strstr(output, "Master/Elite"));
            assert(strstr(output, "not a final eligibility check"));
            assert(strstr(output, "Class/build power mastery and safe conversion"));
        }
    }
    fresh(CLASS_VAMPIRE | CLASS_WEREWOLF);
    preview("list");
    assert(strstr(output, "No hybrid choices"));
    fresh(CLASS_VAMPIRE | CLASS_WEREWOLF);
    preview("Bloodmoon Revenant");
    assert(strstr(output, "unavailable to your current base class"));
    assert(!IS_VAMPIRE(&actor) && !IS_WEREWOLF(&actor));
    fresh(CLASS_NONE);
    preview("list");
    assert(strstr(output, "No hybrid choices"));
    for (size_t i = 0; i < hybrid_count(); i++)
        assert(!strstr(output, hybrid_at(i)->name));
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
    cosmetics();
    commands();
    help_topics();
    puts("hybrid registry and read-only command regressions passed");
    return 0;
}
