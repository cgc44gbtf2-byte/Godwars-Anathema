/* Real damage/death/reward paths; synthetic actors, no accounts or world. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"

void init_mm(void);
static CHAR_DATA a, b, mob;
static PC_DATA pa, pb;
static DESCRIPTOR_DATA da, db;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static MOB_INDEX_DATA index_data;
static char outa[32768], outb[32768];
extern char *string_space, *top_string;

static void corpse_case(bool overlong, bool invalid_format)
{
    static char shared[] = "corpse\0the corpse of %s\0The corpse of %s lies here.\0%n";
    static char huge[3 * MAX_STRING_LENGTH];
    static OBJ_INDEX_DATA corpse_index, item_index;
    char *saved_space = string_space, *saved_top = top_string;
    OBJ_INDEX_DATA *saved_index = obj_index_hash[OBJ_VNUM_CORPSE_NPC % MAX_KEY_HASH];
    memset(&corpse_index, 0, sizeof(corpse_index));
    memset(&item_index, 0, sizeof(item_index));
    string_space = shared; top_string = shared + sizeof(shared);
    corpse_index.vnum = OBJ_VNUM_CORPSE_NPC;
    corpse_index.item_type = ITEM_CORPSE_NPC;
    corpse_index.name = shared;
    corpse_index.short_descr = shared + strlen(shared) + 1;
    corpse_index.description = corpse_index.short_descr + strlen(corpse_index.short_descr) + 1;
    if (invalid_format)
        corpse_index.short_descr = corpse_index.description + strlen(corpse_index.description) + 1;
    item_index = corpse_index; item_index.vnum = 12345; item_index.item_type = ITEM_TRASH;
    obj_index_hash[OBJ_VNUM_CORPSE_NPC % MAX_KEY_HASH] = &corpse_index;
    OBJ_DATA *item = create_object(&item_index, 0);
    OBJ_DATA *vanish = create_object(&item_index, 0);
    SET_BIT(vanish->extra_flags, ITEM_VANISH);
    obj_to_char(item, &mob); obj_to_char(vanish, &mob);
    SET_BIT(mob.affected_by, AFF_SHADOWPLANE);
    if (overlong) {
        memset(huge, 'N', sizeof(huge) - 1); huge[sizeof(huge) - 1] = '\0';
        mob.short_descr = huge;
    }
    make_corpse(&mob, &a);
    OBJ_DATA *corpse = room.contents;
    assert(corpse && corpse->pIndexData == &corpse_index);
    assert(corpse->contains == item && item->next_content == NULL);
    assert(mob.carrying == NULL && corpse->value[2] == 12345);
    assert(corpse->timer >= 60 && corpse->timer <= 120);
    assert(IS_SET(corpse->extra_flags, ITEM_SHADOWPLANE));
    assert(IS_SET(item->extra_flags, ITEM_SHADOWPLANE));
    assert(!strcmp(corpse->questmaker, "Killer"));
    assert(strlen(corpse->short_descr) < MAX_STRING_LENGTH);
    assert(strlen(corpse->description) < MAX_STRING_LENGTH);
    if (!overlong) assert(!strcmp(corpse->short_descr, "the corpse of Testmobile"));
    extract_obj(corpse);
    assert(object_list == NULL && room.contents == NULL);
    obj_index_hash[OBJ_VNUM_CORPSE_NPC % MAX_KEY_HASH] = saved_index;
    string_space = saved_space; top_string = saved_top;
}

static void fixture(void)
{
    clear_char(&a); clear_char(&b); clear_char(&mob);
    memset(&pa, 0, sizeof(pa)); memset(&pb, 0, sizeof(pb));
    memset(&da, 0, sizeof(da)); memset(&db, 0, sizeof(db));
    memset(&room, 0, sizeof(room)); memset(&area, 0, sizeof(area));
    memset(&index_data, 0, sizeof(index_data));
    memset(outa, 0, sizeof(outa)); memset(outb, 0, sizeof(outb));
    a.name = a.short_descr = "Killer"; b.name = b.short_descr = "Victim";
    a.pcdata = &pa; b.pcdata = &pb;
    pa.cdest = pb.cdest = "";
    a.level = b.level = LEV_AWA;
    a.in_room = b.in_room = mob.in_room = &room;
    room.area = &area; room.light = 1; room.people = &a;
    a.next_in_room = &b; b.next_in_room = &mob;
    a.next = &b; b.next = &mob; char_list = &a;
    first_fight = last_fight = NULL;
    a.hit = b.hit = a.max_hit = b.max_hit = 1000;
    a.position = b.position = POS_STANDING;
    a.style[0] = b.style[0] = STYLE_UNARMED;
    for (int i = 0; i < 5; ++i) a.stats[i] = b.stats[i] = 8;
    da.connected = db.connected = CON_PLAYING;
    da.character = &a; db.character = &b;
    a.desc = &da; b.desc = &db; da.next = &db; descriptor_list = &da;
    da.outbuf = outa; db.outbuf = outb;
    da.outsize = sizeof(outa); db.outsize = sizeof(outb);
    mob.act = ACT_IS_NPC;
    mob.name = mob.short_descr = "Testmobile";
    mob.position = POS_STANDING;
    mob.pIndexData = &index_data;
    mob.level = 100; mob.hit = mob.max_hit = 1000;
    index_data.vnum = 12345;
    exp_info.exp = 100; exp_info.bit = 0;
    current_time = 123; init_mm();
}

static void baseline(void)
{
    fixture();
    b.hit = 1; b.position = POS_STUNNED; update_pos(&b);
    assert(b.position == POS_STANDING);
    const struct { long hp, expected; } positions[] = {
        {0, POS_STUNNED}, {-2, POS_STUNNED}, {-3, POS_INCAP},
        {-5, POS_INCAP}, {-6, POS_MORTAL}, {-10, POS_MORTAL},
        {-1000, POS_MORTAL},
    };
    for (unsigned n = 0; n < sizeof(positions) / sizeof(positions[0]); ++n) {
        b.hit = positions[n].hp; update_pos(&b);
        assert(b.position == positions[n].expected);
        assert(b.hit == (positions[n].hp < -10 ? -10 : positions[n].hp));
    }
    mob.hit = 0; update_pos(&mob); assert(mob.position == POS_DEAD);
    fixture();
    hurt_person(&a, &b, 100); assert(b.hit == 900);
    hurt_person(&a, &b, -50); assert(b.hit == 950); /* Legacy signed healing. */
    assert(get_damcap(&a) == 1300);
    assert(mod_damcap(&a, &b) == 900);
    b.act = ACT_IS_NPC;
    assert(mod_damcap(&a, &b) == 1100);
    b.act = 0; SET_BIT(b.itemaffect, ITEMA_RESISTANCE);
    assert(mod_damcap(&a, &b) == 500);
    fixture();
    assert(xp_compute(&mob) == 15100);
    mob.level = 101; assert(xp_compute(&mob) == 15401);
    SET_BIT(mob.affected_by, AFF_SANCTUARY);
    assert(xp_compute(&mob) == 19251);
    SET_BIT(mob.act, ACT_NOEXP); assert(xp_compute(&mob) == 0);
    assert(xp_compute(&a) == 0);
    fixture(); b.leader = &a;
    group_gain(&a, &mob);
    assert(a.exp == 18875 && b.exp == 18875);
    fixture(); a.class = CLASS_DROW;
    dp_gain(&a, &mob);
    assert(pa.classpoints[CP_CURRENT] == 100 && pa.classpoints[CP_TOTAL] == 100);
    SET_BIT(mob.act, ACT_NOEXP); dp_gain(&a, &mob);
    assert(pa.classpoints[CP_CURRENT] == 100);
    fixture();
    pa.cdest = "%s defeats %s!";
    death_message(&a, &b);
    assert(strstr(outa, "Killer defeats Victim!"));
    fixture(); corpse_case(FALSE, FALSE);
}

static void records(void)
{
    fixture();
    pa.score[SCORE_HIGH_LEVEL] = 90; pa.score[SCORE_HIGH_XP] = 100;
    group_gain(&a, &mob);
    assert(pa.score[SCORE_HIGH_LEVEL] == 100);
    assert(pa.score[SCORE_HIGH_XP] == 15100);
    group_gain(&a, &mob);
    assert(pa.score[SCORE_HIGH_LEVEL] == 100);
    assert(pa.score[SCORE_HIGH_XP] == 15100);
    assert(pa.score[SCORE_TOTAL_XP] == 30200 && a.exp == 30200);
}

static void occupancy(void)
{
    static CHAR_DATA online[10];
    static PC_DATA online_data[10];
    static DESCRIPTOR_DATA clients[10];
    fixture();
    memset(online_data, 0, sizeof(online_data));
    memset(clients, 0, sizeof(clients));
    for (int n = 0; n < 10; ++n) {
        clear_char(&online[n]);
        online[n].pcdata = &online_data[n];
        online[n].level = LEV_AWA;
        clients[n].character = &online[n];
        clients[n].connected = n == 9 ? CON_AEDITOR : CON_PLAYING;
        clients[n].next = n == 9 ? NULL : &clients[n + 1];
    }
    descriptor_list = clients;
    group_gain(&a, &mob);
    assert(a.exp == 16610); /* Ten in-world players grant the +10% bonus. */
    descriptor_list = NULL;
}

static void npc_fatality(void)
{
    fixture();
    death_message(&mob, &b);
    assert(strstr(outa, "Testmobile"));
}

static void unsafe_format(void)
{
    fixture();
    pa.cdest = "%n";
    death_message(&a, &b);
    assert(strstr(outa, "Killer") && strstr(outa, "Victim"));
    fixture();
    pa.cdest = "%s wins 100%% against %s!";
    death_message(&a, &b);
    assert(strstr(outa, "Killer wins 100% against Victim!"));
}

static void overflow(void)
{
    fixture();
    b.hit = LONG_MIN + 1; hurt_person(&a, &b, 100);
    assert(b.hit == -10 && b.position == POS_MORTAL);
    fixture();
    hurt_person(&a, &b, LONG_MIN);
    assert(b.hit == LONG_MAX);
}

static void null_guards(void)
{
    fixture();
    damage(NULL, &b, 100, TYPE_HIT, NULL);
    damage(&a, NULL, 100, TYPE_HIT, NULL);
    hurt_person(&a, NULL, 100);
    assert(!hurt_fighting(&a, NULL));
    assert(!hurt_fighting(NULL, &b));
    assert(mod_damcap(NULL, &b) == 0);
    assert(get_damcap(NULL) == 0);
    update_pos(NULL);
    assert(b.hit == 1000);
}

int main(int argc, char **argv)
{
    if (argc == 2) {
        if (!strcmp(argv[1], "--baseline")) baseline();
        else if (!strcmp(argv[1], "--records")) records();
        else if (!strcmp(argv[1], "--occupancy")) occupancy();
        else if (!strcmp(argv[1], "--npc")) npc_fatality();
        else if (!strcmp(argv[1], "--format")) unsafe_format();
        else if (!strcmp(argv[1], "--overflow")) overflow();
        else if (!strcmp(argv[1], "--null")) null_guards();
        else if (!strcmp(argv[1], "--corpse")) { fixture(); corpse_case(TRUE, FALSE); }
        else if (!strcmp(argv[1], "--corpse-format")) { fixture(); corpse_case(FALSE, TRUE); }
        else return 2;
    } else {
        baseline(); records(); occupancy(); npc_fatality(); unsafe_format();
        overflow(); null_guards();
        fixture(); corpse_case(TRUE, FALSE);
        fixture(); corpse_case(FALSE, TRUE);
    }
    puts("damage, position, cap, XP/class-point and death-message checks passed");
    return 0;
}
