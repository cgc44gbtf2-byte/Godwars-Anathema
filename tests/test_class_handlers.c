/* Full-engine characterization of the first shared-clan handler extraction.
 * Synthetic actor only: no accounts, area archive, network or disk saves. */
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Genesis/src/merc.h"

extern void init_mm(void);
extern char *string_space, *top_string;
static CHAR_DATA actor;
static PC_DATA player;
static DESCRIPTOR_DATA descriptor;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static char output[65536];

static void fresh(long class)
{
    free_string(actor.morph);
    clear_char(&actor);
    memset(&player, 0, sizeof(player));
    memset(&descriptor, 0, sizeof(descriptor));
    memset(&room, 0, sizeof(room));
    memset(&area, 0, sizeof(area));
    memset(output, 0, sizeof(output));
    actor.name = actor.short_descr = "tester";
    actor.pcdata = &player;
    actor.in_room = &room;
    actor.desc = &descriptor;
    actor.class = class;
    actor.level = LEV_AWA;
    actor.vampgen = 9;
    actor.style[0] = STYLE_UNARMED;
    actor.mana = 1000;
    actor.primal = 123;
    actor.blood[0] = 100;
    actor.blood[1] = 50;
    player.cland[0] = player.cland[1] = player.cland[2] = -1;
    player.love = player.conding = "";
    room.area = &area;
    room.light = 1;
    room.people = &actor;
    descriptor.character = &actor;
    descriptor.connected = CON_PLAYING;
    descriptor.outbuf = output;
    descriptor.outsize = sizeof(output);
    char_list = &actor;
    descriptor_list = NULL;
    first_fight = last_fight = NULL;
}

static void costless(void)
{
    assert(actor.mana == 1000 && actor.primal == 123 && actor.hit == 10000);
    assert(actor.blood[0] == 100 && actor.blood[1] == 50);
}

static void command(const char *name);

static void test_vampire_training_help(void)
{
    static const struct {
        char *clan, *name;
        long disc, build, price, cap;
        bool in_clan;
    } profiles[] = {
        {"Ventrue", "dominate", DISC_DOMINATE, -1, 5, 7, TRUE},
        {"Ventrue", "animalism", DISC_ANIMALISM, -1, 15, 5, FALSE},
        {"Caitiff", "animalism", DISC_ANIMALISM, -1, 5, 6, FALSE},
        {"Panders", "animalism", DISC_ANIMALISM, -1, 5, 6, FALSE},
        {"Caitiff", "dominate", DISC_DOMINATE, -1, 5, 6, TRUE},
        {"Panders", "dominate", DISC_DOMINATE, -1, 5, 6, TRUE},
        {"Ventrue", "thaumaturgy", DISC_THAUMATURGY, -1, 15, 5, FALSE},
        {"Ventrue", "thaumaturgy", DISC_THAUMATURGY, -1, 5, 5, TRUE},
        {"Caitiff", "thaumaturgy", DISC_THAUMATURGY, -1, 5, 5, FALSE},
        {"Panders", "thaumaturgy", DISC_THAUMATURGY, -1, 5, 5, FALSE},
        {"", "visceratika", DISC_THAUMATURGY, VAMPIRE_BUILD_GARGOYLE, 15, 5, FALSE},
        {"", "animalism", DISC_ANIMALISM, VAMPIRE_BUILD_GARGOYLE, 15, 5, FALSE}
    };
    unsigned views = 0, boundaries = 0;
    for (size_t p = 0; p < sizeof(profiles) / sizeof(profiles[0]); ++p)
    for (int rank = -1; rank <= 7; ++rank)
    for (int mode = 0; mode < 3; ++mode)
    {
        fresh(CLASS_VAMPIRE);
        actor.clan = profiles[p].clan;
        player.cland[0] = profiles[p].in_clan ? profiles[p].disc : profiles[p].build;
        player.powers[profiles[p].disc] = rank;
        const long cost = (rank == -1 ? 1 : rank + 1) * profiles[p].price;
        actor.primal = cost - (mode == 1);
        actor.wait = 7;
        CHAR_DATA expected = actor;
        PC_DATA expected_pc = player;
        const bool accepted = mode == 2 && rank != 0 && rank < profiles[p].cap;
        expected.primal -= accepted ? cost : 0;
        if (accepted) expected_pc.powers[profiles[p].disc] = rank < 1 ? 1 : rank + 1;
        do_discipline(&actor, mode == 0 ? "" : profiles[p].name);
        assert(!memcmp(&actor, &expected, sizeof(actor)));
        assert(!memcmp(&player, &expected_pc, sizeof(player)));
        if (mode == 0)
        {
            assert(strstr(output, "Viewing is free; discipline <name> buys one rank immediately."));
            assert(strstr(output, "In-clan: 5 x next rank primal (max 7)"));
            assert(strstr(output, "Caitiff/Panders: 5 x next rank primal (max 6)"));
            assert(strstr(output, "Thaumaturgy/Visceratika always max 5."));
            assert(strstr(output, "training does not activate them."));
            ++views;
        }
        else
        {
            assert(strstr(output, rank == 0 ? "no such discipline" :
                rank >= profiles[p].cap ? "any further" :
                accepted ? "You improve your discipline." : "You need another"));
            ++boundaries;
        }
    }
    assert(views == 108 && boundaries == 216);
    puts("Vampire: 108 free menu views and 216 training cost/cap boundaries passed");
}

static void test_monk_training_help(void)
{
    static const struct { char *name; int path; } paths[] = {
        {"mantra", MONK_MANTRA}, {"art", MONK_ART}, {"tech", MONK_TECH}
    };
    unsigned views = 0, purchases = 0;
    for (size_t p = 0; p < sizeof(paths) / sizeof(paths[0]); ++p)
    for (int rank = 0; rank <= 8; ++rank)
    {
        for (int menu = 0; menu <= 1; ++menu)
        {
            fresh(CLASS_MONK);
            player.monk[paths[p].path] = rank;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            do_monklearn(&actor, menu ? paths[p].name : "");
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (!menu)
            {
                assert(strstr(output, "View: monklearn <mantra/art/tech> (free)."));
                assert(strstr(output, "Train: monklearn <mantra/art/tech> gain."));
                assert(strstr(output, "10 x next rank primal; maximum 8."));
            }
            else
                assert(strstr(output, p == 0 ? "Mantras -" :
                                     p == 1 ? "Arts -" : "Techs -"));
            ++views;
        }
        for (int enough = 0; enough <= 1; ++enough)
        {
            fresh(CLASS_MONK);
            player.monk[paths[p].path] = rank;
            const long cost = (rank + 1) * 10;
            actor.primal = cost - !enough;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            const bool accepted = enough && rank < 8;
            expected.primal -= accepted ? cost : 0;
            expected_pc.monk[paths[p].path] += accepted;
            char arg[48];
            snprintf(arg, sizeof(arg), "%s gain", paths[p].name);
            do_monklearn(&actor, arg);
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (rank == 8) assert(strstr(output, "may not gain any more"));
            else if (enough) assert(strstr(output, "You use"));
            else assert(strstr(output, "It costs"));
            ++purchases;
        }
    }
    fresh(CLASS_MONK);
    CHAR_DATA expected = actor;
    PC_DATA expected_pc = player;
    do_monklearn(&actor, "unknown");
    assert(strstr(output, "View: monklearn <mantra/art/tech> (free)."));
    assert(!memcmp(&actor, &expected, sizeof(actor)));
    assert(!memcmp(&player, &expected_pc, sizeof(player)));
    assert(views == 54 && purchases == 54);
    puts("Monk: 54 free help views and 54 training cost/cap boundaries passed");
}

static void test_dragon_training_help(void)
{
    static const struct { char *name; long age, bit; } skills[] = {
        {"fangs", AGE_VYOUNG, DRAG_FANGS}, {"nightsight", AGE_VYOUNG, DRAG_NIGHT},
        {"claws", AGE_YOUNG, DRAG_CLAWS}, {"horns", AGE_YOUNG, DRAG_HORNS},
        {"wings", AGE_JUVENILE, DRAG_WINGS}, {"breath", AGE_JUVENILE, DRAG_BREATH},
        {"tail", AGE_YADULT, DRAG_TAIL}, {"truesight", AGE_ADULT, DRAG_TRUESIGHT},
        {"dust", AGE_MATURE, DRAG_DUST}, {"magic", AGE_OLD, DRAG_MAGIC},
        {"scry", AGE_OLD, DRAG_SCRY}, {"readaura", AGE_VOLD, DRAG_READAURA},
        {"shield", AGE_ANCIENT, DRAG_SHIELD}
    };
    static const struct { long age, dp, qp; } ages[] = {
        {AGE_HATCHLING, 2500, 0}, {AGE_VYOUNG, 5000, 0},
        {AGE_YOUNG, 7500, 0}, {AGE_JUVENILE, 10000, 0},
        {AGE_YADULT, 15000, 500}, {AGE_ADULT, 25000, 1000},
        {AGE_MATURE, 35000, 1500}, {AGE_OLD, 50000, 2000},
        {AGE_VOLD, 70000, 2500}, {AGE_ANCIENT, 100000, 3000},
        {AGE_WYRM, 0, 0}, {AGE_AWYRM, 0, 0}
    };
    unsigned views = 0, purchases = 0, age_cases = 0;
    for (size_t a = 0; a < sizeof(ages) / sizeof(ages[0]); ++a)
    {
        for (int invalid = 0; invalid <= 1; ++invalid)
        {
            fresh(CLASS_DRAGON);
            actor.vampgen = ages[a].age;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            do_dragonskill(&actor, invalid ? "unknown" : "");
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            assert(strstr(output, "View eligible choices: dragonskill (free)."));
            assert(strstr(output, "Purchases spend immediately"));
            assert(strstr(output, "learning a skill does not activate it"));
            assert(strstr(output, "Age purchases stop at Wyrm."));
            ++views;
        }
        for (size_t s = 0; s < sizeof(skills) / sizeof(skills[0]); ++s)
        for (int known = 0; known <= 1; ++known)
        for (int enough = 0; enough <= 1; ++enough)
        {
            fresh(CLASS_DRAGON);
            actor.vampgen = ages[a].age;
            actor.wait = 7;
            const long cost = 1000 * (12 - skills[s].age);
            player.classpoints[CP_CURRENT] = cost - !enough;
            if (known) player.dragskills = skills[s].bit;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            const bool eligible = actor.vampgen <= skills[s].age;
            const bool accepted = eligible && !known && enough;
            if (accepted)
            {
                expected_pc.dragskills |= skills[s].bit;
                expected_pc.classpoints[CP_CURRENT] -= cost;
            }
            do_dragonskill(&actor, skills[s].name);
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (!eligible) assert(strstr(output, "Eligible choices:"));
            else if (known) assert(strstr(output, "already have that skill"));
            else if (enough) assert(strstr(output, "You learn a Dragon skill"));
            else assert(strstr(output, "You need"));
            ++purchases;
        }
        for (int dp_short = 0; dp_short <= 1; ++dp_short)
        for (int qp_short = 0; qp_short <= 1; ++qp_short)
        {
            fresh(CLASS_DRAGON);
            actor.vampgen = ages[a].age;
            actor.wait = 7;
            player.classpoints[CP_CURRENT] = ages[a].dp - dp_short;
            player.quest = ages[a].qp - qp_short;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            const bool accepted = ages[a].dp && !dp_short && !qp_short;
            if (accepted)
            {
                --expected.vampgen;
                expected_pc.classpoints[CP_CURRENT] -= ages[a].dp;
                expected_pc.quest -= ages[a].qp;
            }
            do_dragonskill(&actor, "age");
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (!ages[a].dp) assert(strstr(output, "highest possible age"));
            else if (accepted) assert(strstr(output, "gone up in age"));
            else assert(strstr(output, "You require"));
            ++age_cases;
        }
    }
    assert(views == 24 && purchases == 624 && age_cases == 48);
    puts("Dragon: 24 free views, 624 skill and 48 age purchase boundaries passed");
}

static void test_sorcerer_training_help(void)
{
    static const struct { char *name; int color; } colors[] = {
        {"purple", PURPLE_MAGIC}, {"red", RED_MAGIC}, {"blue", BLUE_MAGIC},
        {"green", GREEN_MAGIC}, {"yellow", YELLOW_MAGIC}, {"black", BLACK_MAGIC}
    };
    unsigned views = 0, purchases = 0;
    for (size_t c = 0; c < sizeof(colors) / sizeof(colors[0]); ++c)
    for (int rank = 0; rank <= 5; ++rank)
    {
        for (int invalid = 0; invalid <= 1; ++invalid)
        {
            fresh(CLASS_SORCERER);
            player.colors[colors[c].color] = rank;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            do_colors(&actor, invalid ? "unknown" : "");
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            assert(strstr(output, "View ranks: colors (free)."));
            assert(strstr(output, "Train: colors <purple/red/blue/green/yellow/black>"));
            assert(strstr(output, "10 x next rank primal; maximum 5."));
            ++views;
        }
        for (int enough = 0; enough <= 1; ++enough)
        {
            fresh(CLASS_SORCERER);
            player.colors[colors[c].color] = rank;
            const long cost = (rank + 1) * 10;
            actor.primal = cost - !enough;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            const bool accepted = enough && rank < 5;
            expected.primal -= accepted ? cost : 0;
            expected_pc.colors[colors[c].color] += accepted;
            do_colors(&actor, colors[c].name);
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (rank == 5) assert(strstr(output, "already have the maximum"));
            else if (enough) assert(strstr(output, "You use"));
            else assert(strstr(output, "You need"));
            ++purchases;
        }
    }
    assert(views == 72 && purchases == 72);
    puts("Sorcerer: 72 free help views and 72 training cost/cap boundaries passed");
}

static void test_ninja_training_help(void)
{
    static const struct { char *name; int path; } paths[] = {
        {"sora", NINJA_SORA}, {"chikyu", NINJA_CHIKYU},
        {"ningenno", NINJA_NINGENNO}
    };
    unsigned views = 0, purchases = 0;
    for (int night = 0; night <= 1; ++night)
    for (size_t p = 0; p < sizeof(paths) / sizeof(paths[0]); ++p)
    for (int rank = 0; rank <= 10; ++rank)
    {
        for (int menu = 0; menu < 3; ++menu)
        {
            fresh(CLASS_NINJA);
            if (night) player.cland[0] = NINJA_BUILD_NIGHTSTALKER;
            player.ninja[paths[p].path] = rank;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            do_principles(&actor, menu == 0 ? "" :
                          menu == 1 ? paths[p].name : "unknown");
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (menu != 1)
            {
                assert(strstr(output, "View powers: principle <sora/chikyu/ningenno>"));
                assert(strstr(output, "Train: principle <sora/chikyu/ningenno> improve"));
                assert(strstr(output, "10 x next rank primal; maximum 10."));
                assert(!!strstr(output, "Nightstalker: no Michi") == night);
            }
            else if (p == 0)
            {
                assert(!!strstr(output, "replace Michi with shadow senses") == night);
                assert(!!strstr(output, "1 : Michi") == (!night && rank >= 1));
            }
            ++views;
        }
        for (int enough = 0; enough <= 1; ++enough)
        {
            char argument[64];
            fresh(CLASS_NINJA);
            if (night) player.cland[0] = NINJA_BUILD_NIGHTSTALKER;
            player.ninja[paths[p].path] = rank;
            const long cost = (rank + 1) * 10;
            actor.primal = cost - !enough;
            actor.wait = 7;
            CHAR_DATA expected = actor;
            PC_DATA expected_pc = player;
            const bool accepted = enough && rank < 10;
            expected.primal -= accepted ? cost : 0;
            expected_pc.ninja[paths[p].path] += accepted;
            snprintf(argument, sizeof(argument), "%s improve", paths[p].name);
            do_principles(&actor, argument);
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (rank == 10) assert(strstr(output, "already gained all"));
            else if (enough) assert(strstr(output, "You improve your ability"));
            else assert(strstr(output, "It costs"));
            ++purchases;
        }
    }
    fresh(CLASS_NINJA);
    CHAR_DATA expected = actor;
    PC_DATA expected_pc = player;
    do_principles(&actor, "sora invalid");
    assert(strstr(output, "To improve, type: principle <sora/chikyu/ningenno> improve."));
    assert(!memcmp(&actor, &expected, sizeof(actor)));
    assert(!memcmp(&player, &expected_pc, sizeof(player)));
    assert(views == 198 && purchases == 132);
    puts("Ninja/Nightstalker: 198 free help views and 132 training boundaries passed");
}

static void test_highlander_training_help(void)
{
    static const struct { char *name; int path; } paths[] = {
        {"attack", HIGH_ATTACK}, {"defense", HIGH_DEFENSE},
        {"weaponry", HIGH_WEAPON}, {"powers", HIGH_ABILITY}
    };
    static const char *const hints[] = {
        "bflash (Powers 2)", "bash (Powers 3)", "htwist (Powers 4)",
        "judge (Powers 5)", "lunge (Powers 6)",
        "counter (Powers 4 + Defense 4)", "your own forged katana",
        "15 x next rank primal; maximum 6.", "Training buys one rank immediately",
        "counter off"
    };
    unsigned views = 0, purchases = 0;
    for (size_t p = 0; p < sizeof(paths) / sizeof(paths[0]); ++p)
    for (int rank = 0; rank <= 6; ++rank)
    {
        fresh(CLASS_HIGHLANDER);
        player.highland[paths[p].path] = rank;
        actor.wait = 7;
        CHAR_DATA expected = actor;
        PC_DATA expected_pc = player;
        do_increase(&actor, "");
        assert(!memcmp(&actor, &expected, sizeof(actor)));
        assert(!memcmp(&player, &expected_pc, sizeof(player)));
        assert(strstr(output, "Syntax: increase <attack/defense/weaponry/powers>\n\r"));
        for (size_t h = 0; h < sizeof(hints) / sizeof(hints[0]); ++h)
            assert(strstr(output, hints[h]));
        ++views;
        for (int enough = 0; enough <= 1; ++enough)
        {
            fresh(CLASS_HIGHLANDER);
            player.highland[paths[p].path] = rank;
            const long cost = (rank + 1) * 15;
            actor.primal = cost - !enough;
            actor.wait = 7;
            expected = actor;
            expected_pc = player;
            const bool accepted = enough && rank < 6;
            expected.primal -= accepted ? cost : 0;
            expected_pc.highland[paths[p].path] += accepted;
            do_increase(&actor, paths[p].name);
            assert(!memcmp(&actor, &expected, sizeof(actor)));
            assert(!memcmp(&player, &expected_pc, sizeof(player)));
            if (rank == 6) assert(strstr(output, "maximum proficiency"));
            else if (enough) assert(strstr(output, "improved your skill"));
            else assert(strstr(output, "It costs"));
            ++purchases;
        }
    }
    fresh(CLASS_HIGHLANDER);
    CHAR_DATA before = actor;
    PC_DATA before_pc = player;
    do_increase(&actor, "unknown");
    assert(strstr(output, "Training buys one rank immediately"));
    assert(!memcmp(&actor, &before, sizeof(actor)));
    assert(!memcmp(&player, &before_pc, sizeof(player)));
    assert(views == 28 && purchases == 56);
    puts("Highlander: 28 free help views and 56 training cost/cap boundaries passed");
}

static void test_highlander_flash(void)
{
    CHAR_DATA victim;
    PC_DATA victim_pc;
    DESCRIPTOR_DATA victim_desc;
    char victim_output[4096] = {0};

    fresh(CLASS_HIGHLANDER);
    memset(&victim_pc, 0, sizeof(victim_pc));
    memset(&victim_desc, 0, sizeof(victim_desc));
    clear_char(&victim);
    victim.name = victim.short_descr = "opponent";
    victim.pcdata = &victim_pc;
    victim_pc.love = victim_pc.conding = "";
    victim.level = LEV_AWA;
    victim.status = actor.status = 100;
    victim.in_room = &room;
    victim.desc = &victim_desc;
    victim_desc.character = &victim;
    victim_desc.connected = CON_PLAYING;
    victim_desc.outbuf = victim_output;
    victim_desc.outsize = sizeof(victim_output);
    victim.next = NULL;
    actor.next = &victim;
    actor.next_in_room = &victim;
    room.people = &actor;
    player.highland[HIGH_ABILITY] = 2;
    SET_BIT(victim.act, PLR_HOLYLIGHT);

    command("bflash tester"); /* No hostile action against oneself. */
    assert(!actor.fighting && !victim.fighting && actor.wait == 0);
    SET_BIT(victim.itemaffect, ITEMA_PEACE);
    command("bflash opponent");
    assert(!actor.fighting && !victim.fighting && actor.wait == 0 &&
           !victim.affected && IS_SET(victim.act, PLR_HOLYLIGHT));
    REMOVE_BIT(victim.itemaffect, ITEMA_PEACE);
    victim.desc = NULL;
    command("bflash opponent"); /* Link-dead target, not yet fighting. */
    assert(!actor.fighting && !victim.fighting && actor.wait == 0 &&
           !victim.affected && IS_SET(victim.act, PLR_HOLYLIGHT));
    victim.desc = &victim_desc;
    command("bflash opponent");
    assert(actor.fighting == &victim && victim.fighting == &actor);
    assert(actor.wait == 24 && victim.affected != NULL);
    assert(IS_AFFECTED(&victim, AFF_BLIND));
    assert(!IS_SET(victim.act, PLR_HOLYLIGHT));
    assert(victim.affected->duration == 20 &&
           victim.affected->modifier == -50);
    stop_fighting(&actor, TRUE);
    affect_remove(&victim, victim.affected);
    char description[MAX_INPUT_LENGTH * 2];
    memset(description, 'X', sizeof(description) - 1);
    description[sizeof(description) - 1] = '\0';
    victim.short_descr = description;
    SET_BIT(victim.act, ACT_IS_NPC);
    player.highland[HIGH_ABILITY] = 5;
    descriptor.outtop = 0; output[0] = '\0';
    command("judge opponent");
    assert(strstr(output, "Health:") && strstr(output, "Magic:") &&
           strstr(output, "Armor:"));
    actor.next = actor.next_in_room = NULL;
    char_list = room.people = &actor;
}

static void command(const char *name)
{
    snprintf(descriptor.incomm, sizeof(descriptor.incomm), "%s", name);
    interpret(&actor, descriptor.incomm);
    descriptor.incomm[0] = '\0';
}

static void test_presence(void)
{
    fresh(CLASS_VAMPIRE);
    do_awe(&actor, "");
    do_majesty(&actor, "");
    assert(!IS_EXTRA(&actor, EXTRA_AWE) && !IS_ITEMAFF(&actor, ITEMA_PEACE));
    assert(strstr(output, "level 1 Presence") && strstr(output, "level 5 Presence"));
    player.powers[DISC_PRESENCE] = 1;
    command("awe");
    assert(IS_EXTRA(&actor, EXTRA_AWE));
    command("awe");
    assert(!IS_EXTRA(&actor, EXTRA_AWE));
    player.powers[DISC_PRESENCE] = 4;
    command("majesty");
    assert(!IS_ITEMAFF(&actor, ITEMA_PEACE) && player.majesty == 0);
    player.powers[DISC_PRESENCE] = 5;
    command("majesty");
    assert(IS_ITEMAFF(&actor, ITEMA_PEACE) && player.majesty == 0);
    command("majesty");
    assert(!IS_ITEMAFF(&actor, ITEMA_PEACE) && player.majesty == 120);
    command("majesty");
    assert(!IS_ITEMAFF(&actor, ITEMA_PEACE) && player.majesty == 120);
    assert(strstr(output, "another 120 seconds"));
    player.majesty = 1;
    command("majesty");
    assert(!IS_ITEMAFF(&actor, ITEMA_PEACE));
    assert(strstr(output, "another 1 second"));
    player.majesty = 0;
    command("majesty");
    assert(IS_ITEMAFF(&actor, ITEMA_PEACE));
    assert(actor.wait == 0);
    costless();
}

static void test_beast(void)
{
    fresh(CLASS_VAMPIRE);
    actor.beast = 99;
    for (int i = 0; i < 128 && actor.beast == 99; ++i)
        do_beastlike(&actor, "");
    assert(actor.beast == 100 && IS_VAMPAFF(&actor, VAM_FANGS));
    assert(!IS_VAMPAFF(&actor, VAM_CLAWS));
    do_beastlike(&actor, "");
    do_humanity(&actor, "");
    assert(actor.beast == 100);
    actor.beast = 99;
    for (int i = 0; i < 128 && actor.beast == 99; ++i)
        do_humanity(&actor, "");
    assert(actor.beast == 98);
    actor.beast = 1;
    for (int i = 0; i < 16384 && actor.beast == 1; ++i)
        do_humanity(&actor, "");
    assert(actor.beast == 0 && strstr(output, "attained Golconda"));
    do_humanity(&actor, "");
    assert(actor.beast == 0 && actor.wait == 0);
    costless();
    fresh(CLASS_VAMPIRE);
    actor.beast = 99;
    player.powers[DISC_PROTEAN] = 2;
    for (int i = 0; i < 128 && actor.beast == 99; ++i)
        do_beastlike(&actor, "");
    assert(actor.beast == 100 && IS_VAMPAFF(&actor, VAM_NIGHTSIGHT));
    assert(IS_VAMPAFF(&actor, VAM_FANGS) && IS_VAMPAFF(&actor, VAM_CLAWS));
    costless();
}

static void test_werewolf(void)
{
    fresh(CLASS_WEREWOLF);
    command("umbralvision");
    assert(IS_AFFECTED(&actor, AFF_SHADOWSIGHT));
    command("umbralvision");
    assert(!IS_AFFECTED(&actor, AFF_SHADOWSIGHT) && actor.wait == 0);
    command("rage");
    assert(player.wolf >= 10 && player.wolf <= 20);
    assert(!IS_ACT(&actor, PLR_WOLFMAN) && actor.wait == 12);
    costless();
    fresh(CLASS_WEREWOLF);
    player.wolf = 99;
    command("rage");
    assert(IS_ACT(&actor, PLR_WOLFMAN));
    assert(player.wolf >= 134 && player.wolf <= 144);
    assert(IS_VAMPAFF(&actor, VAM_FANGS) && IS_VAMPAFF(&actor, VAM_CLAWS));
    assert(IS_AFFECTED(&actor, AFF_POLYMORPH) && actor.wait == 12);
    costless();
    fresh(CLASS_WEREWOLF);
    player.wolf = 99;
    player.auspice[AUSPICE_AHROUN] = 4;
    do_rage(&actor, "");
    assert(IS_ACT(&actor, PLR_WOLFMAN) && player.wolf == 300);
    actor.wait = 0;
    do_rage(&actor, "");
    assert(player.wolf == 300 && actor.wait == 0);
    assert(strstr(output, "already in a rage"));
    costless();
}

static void test_shared(long class)
{
    fresh(class);
    player.powers[DISC_PROTEAN] = 2;
    player.breed[BREED_METIS] = 3;
    player.dragskills = DRAG_FANGS | DRAG_CLAWS | DRAG_NIGHT;
    player.drow[DROW_POWERS] = DROW_NIGHT;
    actor.spheres[MFOR] = 1;
    if (class == CLASS_DRAGON) actor.polyaff = POLY_DRAGONFORM;
    bool physical = class == CLASS_VAMPIRE || class == CLASS_WEREWOLF
        || class == CLASS_DRAGON;
    bool sight = physical || class == CLASS_DROW || class == CLASS_MAGE;
    do_fangs(&actor, "");
    do_claws(&actor, "");
    do_nightsight(&actor, "");
    assert(!!IS_VAMPAFF(&actor, VAM_FANGS) == physical);
    assert(!!IS_VAMPAFF(&actor, VAM_CLAWS) == physical);
    assert(!!IS_VAMPAFF(&actor, VAM_NIGHTSIGHT) == sight);
    do_fangs(&actor, "");
    do_claws(&actor, "");
    do_nightsight(&actor, "");
    assert(!IS_VAMPAFF(&actor, VAM_FANGS | VAM_CLAWS | VAM_NIGHTSIGHT));
    SET_BIT(actor.extra, TIED_UP);
    SET_BIT(actor.affected_by, AFF_WEBBED | AFF_SHADOW);
    do_flex(&actor, "");
    assert(!IS_EXTRA(&actor, TIED_UP));
    assert(!IS_AFFECTED(&actor, AFF_WEBBED | AFF_SHADOW));
    assert(actor.wait == 12);
    player.wolf = 75;
    actor.wait = 0;
    do_calm(&actor, "");
    assert(player.wolf == (class == CLASS_NINJA ? 0 : 75));
    if (class == CLASS_WEREWOLF)
    {
        player.tribes[TRIBE_CHILDREN_OF_GAIA] = 1;
        do_calm(&actor, "");
        assert(player.wolf == 0 && actor.wait == 12);
    }
    costless();
}

static void test_rejections(long class)
{
    fresh(class);
    player.powers[DISC_PRESENCE] = 7;
    player.powers[DISC_PROTEAN] = 7;
    if (class != CLASS_VAMPIRE)
    {
        actor.beast = 99;
        do_majesty(&actor, ""); do_awe(&actor, "");
        do_humanity(&actor, ""); do_beastlike(&actor, "");
        assert(!IS_ITEMAFF(&actor, ITEMA_PEACE) && !IS_EXTRA(&actor, EXTRA_AWE));
        assert(actor.beast == 99 && actor.vampaff == 0);
    }
    if (class != CLASS_WEREWOLF)
    {
        do_shadowsight(&actor, ""); do_rage(&actor, "");
        assert(!IS_AFFECTED(&actor, AFF_SHADOWSIGHT) && player.wolf == 0);
    }
    assert(actor.wait == 0);
    costless();
    fresh(class);
    SET_BIT(actor.act, ACT_IS_NPC);
    actor.pcdata = NULL;
    do_majesty(&actor, ""); do_awe(&actor, "");
    do_humanity(&actor, ""); do_beastlike(&actor, "");
    do_shadowsight(&actor, ""); do_rage(&actor, "");
    assert(actor.extra == 0 && actor.itemaffect == 0 && actor.vampaff == 0);
    assert(actor.wait == 0 && actor.affected_by == 0);
}

int main(void)
{
    string_space = calloc(1, 1024 * 1024);
    assert(string_space); top_string = string_space;
    current_time = 123456;
    init_mm();
    test_vampire_training_help();
    test_monk_training_help();
    test_dragon_training_help();
    test_sorcerer_training_help();
    test_ninja_training_help();
    test_highlander_training_help();
    test_highlander_flash();
    test_presence();
    test_beast();
    test_werewolf();
    const long classes[] = {CLASS_NONE, CLASS_VAMPIRE, CLASS_WEREWOLF,
        CLASS_SORCERER, CLASS_DROW, CLASS_HIGHLANDER, CLASS_MAGE,
        CLASS_DRAGON, CLASS_NINJA, CLASS_MONK, CLASS_ELEM};
    for (size_t i = 0; i < sizeof(classes) / sizeof(classes[0]); ++i)
    {
        test_shared(classes[i]);
        test_rejections(classes[i]);
    }
    fresh(CLASS_NONE);
    puts("class handler extraction full-engine regressions passed");
    return 0;
}
