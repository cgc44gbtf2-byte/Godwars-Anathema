/* Real engine, synthetic characters and save fragments only. */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../Genesis/src/merc.h"

void fread_char(CHAR_DATA *ch, FILE *fp);

static CHAR_DATA a, b;
static PC_DATA pa, pb;
static DESCRIPTOR_DATA descriptor, target_descriptor;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static char output[16384], target_output[16384];

static void fixture(void)
{
    clear_char(&a);
    clear_char(&b);
    memset(&pa, 0, sizeof(pa));
    memset(&pb, 0, sizeof(pb));
    memset(&descriptor, 0, sizeof(descriptor));
    memset(&target_descriptor, 0, sizeof(target_descriptor));
    memset(&room, 0, sizeof(room));
    memset(&area, 0, sizeof(area));
    memset(output, 0, sizeof(output));
    memset(target_output, 0, sizeof(target_output));
    a.name = "Styletest";
    b.name = "Other";
    a.pcdata = &pa;
    b.pcdata = &pb;
    a.style[0] = b.style[0] = STYLE_UNARMED;
    a.style[STYLE_UNARMED] = 100;
    a.position = b.position = POS_STANDING;
    a.in_room = b.in_room = &room;
    room.people = &a;
    room.area = &area;
    room.light = 1;
    a.next_in_room = &b;
    a.level = b.level = LEV_AWA;
    a.hit = b.hit = a.max_hit = b.max_hit = 100000;
    a.desc = &descriptor;
    descriptor.character = &a;
    descriptor.connected = CON_PLAYING;
    descriptor.outbuf = output;
    descriptor.outsize = sizeof(output);
    b.desc = &target_descriptor;
    target_descriptor.character = &b;
    target_descriptor.connected = CON_PLAYING;
    target_descriptor.outbuf = target_output;
    target_descriptor.outsize = sizeof(target_output);
    pa.flag = pb.flag = FLAG_PK;
    pa.love = pb.love = pa.conding = pb.conding = "";
}

static void save_fragment(const char *fragment)
{
    FILE *fp = tmpfile();
    assert(fp != NULL);
    assert(fputs(fragment, fp) >= 0);
    rewind(fp);
    fread_char(&a, fp);
    fclose(fp);
}

static void persisted_ranges(void)
{
    fixture();
    save_fragment("Style 99 999 -9 175 201 0 0\n"
                  "Stance -1 999 -5 200 201 0 0 0 0 0 0 99\nEnd\n");
    assert(a.style[0] == STYLE_UNARMED && a.style[1] == 200);
    assert(a.style[2] == 0 && a.style[3] == 175 && a.style[4] == 200);
    assert(a.stance[0] == -1 && a.stance[1] == 200);
    assert(a.stance[2] == 0 && a.stance[3] == 200);
    assert(a.stance[4] == 200 && a.stance[11] == STANCE_NONE);

    /* A valid legacy sentinel and maxed Highlander training survive. */
    fixture();
    save_fragment("Stance 8 200 200 200 200 200 200 200 200 200 200 6\n"
                  "Style 5 100 100 200 200 200 100\nEnd\n");
    assert(a.stance[0] == STANCE_TIGER && a.stance[8] == 200);
    assert(a.stance[11] == STANCE_MANTIS);
    assert(a.style[0] == STYLE_TWOHAND && a.style[5] == 200);

    fixture();
    save_fragment("Stance 999 0 0 0 0 0 0 0 0 0 0 -7\n"
                  "Style -88 0 0 0 0 0 0\nEnd\n");
    assert(a.stance[0] == STANCE_NONE && a.stance[11] == STANCE_NONE);
    assert(a.style[0] == STYLE_UNARMED);
}

static void invalid_live_indexes(void)
{
    fixture();
    /* Reader guards matter even for values introduced after loading. */
    long expected = breakavoid(&a);
    a.style[0] = 99;
    assert(breakavoid(&a) == expected);
    do_skill(&a, "self");
    assert(strstr(output, "not using a fighting style") != NULL);

    a.style[0] = STYLE_UNARMED;
    a.stance[0] = -1;
    descriptor.outtop = 0;
    output[0] = '\0';
    do_skill(&a, "self");
    assert(strstr(output, "not in a fighting stance") != NULL);
}

static void defender_stance_without_attacker_stance(void)
{
    fixture();
    b.stance[0] = STANCE_CRAB;
    b.stance[STANCE_CRAB] = 200;
    a.stance[0] = STANCE_NONE;
    assert(dambonus(&a, &b, 900) == 300);
    a.stance[0] = STANCE_CRANE;
    assert(dambonus(&a, &b, 900) == 300);
    a.stance[0] = STANCE_BULL;
    a.stance[STANCE_BULL] = 200;
    assert(dambonus(&a, &b, 900) == 900); /* 3x offense, /3 defense. */
    b.stance[0] = STANCE_DRAGON;
    b.stance[STANCE_DRAGON] = 200;
    a.stance[0] = STANCE_NONE;
    assert(dambonus(&a, &b, 900) == 300);
    b.stance[0] = STANCE_SWALLOW;
    b.stance[STANCE_SWALLOW] = 200;
    assert(dambonus(&a, &b, 900) == 300);
    b.stance[0] = STANCE_NONE;
    assert(dambonus(&a, &b, 900) == 900);
    assert(dambonus(&a, &b, 0) == 1);

    b.stance[0] = STANCE_CRAB;
    a.stance[0] = STANCE_CRANE;
    a.position = POS_FIGHTING;
    do_stance(&a, "");
    assert(a.stance[0] == STANCE_NONE && a.wait == 0);
    assert(dambonus(&a, &b, 900) == 300);
}

static void styles_and_promotion(void)
{
    OBJ_DATA off = {0}, weapon = {0};
    OBJ_INDEX_DATA index = {0};
    fixture();
    index.vnum = OBJ_VNUM_PROTOPLASM;
    off.pIndexData = weapon.pIndexData = &index;
    off.item_type = ITEM_WEAPON;
    off.wear_loc = WEAR_HOLD;
    off.spectype = SITEM_SILVER;
    off.name = "test blade";
    off.short_descr = "a test blade";
    off.carried_by = &a;
    a.carrying = &off;
    a.itemaffect = ITEMA_LEFT_SILVER;
    update_style(&a);
    assert(off.wear_loc == WEAR_WEAPON && a.style[0] == STYLE_ONEHAND);
    assert(IS_ITEMAFF(&a, ITEMA_RIGHT_SILVER));
    assert(!IS_ITEMAFF(&a, ITEMA_LEFT_SILVER));

    weapon.item_type = ITEM_WEAPON;
    weapon.pIndexData = &index;
    weapon.name = "test katana";
    weapon.short_descr = "a test katana";
    weapon.wear_loc = WEAR_WEAPON;
    weapon.carried_by = &a;
    a.carrying = &weapon;
    a.style[0] = STYLE_ONEHAND;
    do_grip(&a, "");
    assert(a.style[0] == STYLE_TWOHAND);
    do_grip(&a, "");
    assert(a.style[0] == STYLE_ONEHAND);

    off.item_type = ITEM_ARMOR;
    off.wear_loc = WEAR_HOLD;
    off.next_content = &weapon;
    a.carrying = &off;
    a.vampaff = VAM_CLAWS;
    a.class = CLASS_VAMPIRE;
    update_style(&a);
    assert(a.style[0] == STYLE_SHIELD);
    weapon.wear_loc = WEAR_NONE;
    off.wear_loc = WEAR_NONE;
    update_style(&a);
    assert(a.style[0] == STYLE_NATURAL);
    a.carrying = NULL; /* Local fixture items; never extract stack objects. */
}

static void class_caps_and_advanced_stances(void)
{
    const long classes[] = {CLASS_NONE, CLASS_VAMPIRE, CLASS_WEREWOLF,
        CLASS_SORCERER, CLASS_DROW, CLASS_HIGHLANDER, CLASS_MAGE,
        CLASS_DRAGON, CLASS_NINJA, CLASS_MONK, CLASS_ELEM};
    const int style_caps[][6] = {
        {100,100,100,100,100,100}, {100,100,100,100,100,100},
        {100,100,100,100,100,100}, {100,100,150,100,100,100},
        {100,100,100,100,100,150}, {100,100,200,200,200,100},
        {100,100,100,100,100,100}, {100,150,100,100,100,100},
        {150,100,100,100,100,100}, {200,100,100,100,100,100},
        {100,100,100,150,100,100}
    };
    const struct { char *name; int stance, first, second; } advanced[] = {
        {"mantis", STANCE_MANTIS, STANCE_MONGOOSE, STANCE_VIPER},
        {"dragon", STANCE_DRAGON, STANCE_BULL, STANCE_CRAB},
        {"tiger", STANCE_TIGER, STANCE_BULL, STANCE_CRANE},
        {"monkey", STANCE_MONKEY, STANCE_CRANE, STANCE_VIPER},
        {"swallow", STANCE_SWALLOW, STANCE_CRAB, STANCE_MONGOOSE}
    };
    for (size_t c = 0; c < sizeof(classes)/sizeof(classes[0]); ++c)
        for (int style = STYLE_UNARMED; style <= STYLE_TWOWEAPON; ++style)
        {
            fixture();
            a.class = classes[c];
            a.style[0] = style;
            a.style[style] = 250;
            improve_style(&a);
            assert(a.style[style] == style_caps[c][style-1]);
        }
    for (size_t i = 0; i < sizeof(advanced)/sizeof(advanced[0]); ++i)
    {
        fixture();
        a.stance[advanced[i].first] = 200;
        a.stance[advanced[i].second] = 199;
        do_stance(&a, advanced[i].name);
        assert(a.stance[0] == STANCE_NONE && a.wait == 0);
        a.stance[advanced[i].second] = 200;
        do_stance(&a, advanced[i].name);
        assert(a.stance[0] == advanced[i].stance && a.wait == 6);
    }
}

static void fight_list_ownership(void)
{
    fixture();
    first_fight = last_fight = NULL;
    set_fighting(&a, &b);
    assert(a.fighting == &b && first_fight == &a && last_fight == &a);
    set_fighting(&b, &a);
    assert(b.fighting == &a && a.next_fight == &b && b.prev_fight == &a);
    assert(first_fight == &a && last_fight == &b);
    stop_fighting(&a, FALSE);
    assert(a.fighting == NULL && b.fighting == &a);
    assert(first_fight == &b && last_fight == &b && b.prev_fight == NULL);
    stop_fighting(&a, TRUE);
    assert(b.fighting == NULL && first_fight == NULL && last_fight == NULL);
}

static void combat_safety_entry_guards(void)
{
    fixture();
    assert(is_safe(NULL, &b) && auto_safe(NULL, &b));
    assert(is_safe(&a, NULL) && auto_safe(&a, NULL));
    assert(is_safe(NULL, NULL) && auto_safe(NULL, NULL));
    multi_hit(NULL, &b);
    multi_hit(&a, NULL);
    set_fighting(&a, NULL);
    assert(first_fight == NULL && last_fight == NULL);
    a.in_room = NULL;
    assert(is_safe(&a, &b) && auto_safe(&a, &b));
    b.in_room = NULL;
    assert(is_safe(&a, &b) && auto_safe(&a, &b));
    fixture();
    assert(!is_safe(&a, &b) && !auto_safe(&a, &b));
    SET_BIT(room.room_flags, ROOM_SAFE);
    assert(is_safe(&a, &b) && auto_safe(&a, &b));
}

static void complete_player_load(void)
{
    char root[] = "/tmp/anathema-combat-style-XXXXXX";
    char player[sizeof(root) + 12], run[sizeof(root) + 12];
    char record[sizeof(root) + 32];
    DESCRIPTOR_DATA d = {0};
    FILE *fp, *previous = fpReserve;
    int olddir = open(".", O_RDONLY | O_DIRECTORY);
    assert(olddir >= 0 && mkdtemp(root) != NULL);
    snprintf(player, sizeof(player), "%s/player", root);
    snprintf(run, sizeof(run), "%s/run", root);
    snprintf(record, sizeof(record), "%s/Styletest", player);
    assert(mkdir(player, 0700) == 0 && mkdir(run, 0700) == 0);
    fp = fopen(record, "w");
    assert(fp != NULL);
    /* Player fields precede restored objects. Claws must override the
     * repaired Unarmed sentinel after the full #END load boundary. */
    assert(fprintf(fp, "#PLAYER\nClass %ld\nVampaff %ld\n"
               "Style 99 0 0 0 0 0 0\nEnd\n#END\n",
               (long)CLASS_VAMPIRE, (long)VAM_CLAWS) > 0);
    assert(fclose(fp) == 0 && chdir(run) == 0);
    fpReserve = tmpfile();
    assert(fpReserve != NULL);
    d.outbuf = output;
    d.outsize = sizeof(output);
    assert(load_char_obj(&d, "Styletest"));
    assert(d.character != NULL && d.character->class == CLASS_VAMPIRE);
    assert(d.character->style[0] == STYLE_NATURAL);
    assert(d.character->in_room == NULL);
    free_char(d.character);
    assert(fclose(fpReserve) == 0);
    fpReserve = previous;
    assert(fchdir(olddir) == 0 && close(olddir) == 0);
    assert(unlink(record) == 0 && rmdir(player) == 0);
    assert(rmdir(run) == 0 && rmdir(root) == 0);
}

int main(void)
{
    persisted_ranges();
    invalid_live_indexes();
    defender_stance_without_attacker_stance();
    styles_and_promotion();
    class_caps_and_advanced_stances();
    fight_list_ownership();
    combat_safety_entry_guards();
    complete_player_load();
    puts("stance/style persistence, defense, styles and hand flags passed");
    return 0;
}
