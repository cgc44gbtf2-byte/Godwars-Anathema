/* Real attack pipeline; isolated synthetic actors, no live world or accounts. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"

void init_mm(void);
int number_mm(void);
extern long nAllocPerm;
static CHAR_DATA a, b;
static PC_DATA pa, pb;
static OBJ_DATA mainhand, offhand;
static MOB_INDEX_DATA mobile;
static ROOM_INDEX_DATA room;
static AREA_DATA area;

static void fixture(long style)
{
    while (b.affected) affect_remove(&b, b.affected);
    clear_char(&a); clear_char(&b);
    memset(&pa, 0, sizeof(pa)); memset(&pb, 0, sizeof(pb));
    memset(&mainhand, 0, sizeof(mainhand));
    memset(&offhand, 0, sizeof(offhand));
    memset(&mobile, 0, sizeof(mobile));
    memset(&room, 0, sizeof(room)); memset(&area, 0, sizeof(area));
    a.pcdata = &pa; b.pcdata = &pb;
    a.name = a.short_descr = "Attacker";
    b.name = b.short_descr = "Target";
    pa.flag = pb.flag = FLAG_PK;
    pa.love = pb.love = pa.conding = pb.conding = "";
    a.level = b.level = LEV_AWA;
    a.hit = b.hit = a.max_hit = b.max_hit = 50000000;
    a.hitroll = b.hitroll = 1500;
    a.position = b.position = POS_FIGHTING;
    a.fighting = &b; b.fighting = &a;
    a.next_fight = &b; b.prev_fight = &a;
    first_fight = &a; last_fight = &b;
    a.next = a.next_in_room = &b; char_list = &a;
    descriptor_list = NULL;
    a.in_room = b.in_room = &room;
    room.people = &a; room.area = &area; room.light = 2;
    a.style[0] = style; b.style[0] = STYLE_UNARMED;
    for (int i = 0; i < 5; ++i) a.stats[i] = b.stats[i] = 8;
    for (int i = 1; i <= 6; ++i) a.style[i] = b.style[i] = 200;
    mainhand.name = offhand.name = "blade";
    mainhand.short_descr = "a test blade"; offhand.short_descr = "another blade";
    mainhand.item_type = offhand.item_type = ITEM_WEAPON;
    mainhand.wear_loc = WEAR_WEAPON; offhand.wear_loc = WEAR_HOLD;
    mainhand.value[1] = offhand.value[1] = 3;
    mainhand.value[2] = offhand.value[2] = 5;
    mainhand.value[3] = offhand.value[3] = TYPE_SLASH;
    mainhand.level = offhand.level = 25;
    if (style != STYLE_UNARMED && style != STYLE_NATURAL && style != STYLE_SHIELD)
        a.carrying = &mainhand;
    if (style == STYLE_TWOWEAPON) mainhand.next_content = &offhand;
    current_time = 123;
    init_mm();
}

static void baseline(void)
{
    /* Captured on 49c3511 before repairs/extraction, including RNG progression. */
    const long style_hp[] = {50000000, 50000000, 50000000, 50000000, 49999840, 50000000};
    const int style_rng[] = {11104418, 6862907, 9001954, 6862907, 9001954, 14711334};
    for (long style = STYLE_UNARMED; style <= STYLE_TWOWEAPON; ++style) {
        fixture(style);
        multi_hit(&a, &b);
        int rng = number_mm();
        printf("style=%ld hp=%ld rng=%d\n", style, b.hit, rng);
        assert(b.hit == style_hp[style - 1] && rng == style_rng[style - 1]);
        assert(b.hit > 0 && b.hit <= b.max_hit && a.fighting == &b);
    }
    const long levels[] = {250, 500, 1000, 2500, 5000};
    const long npc_hp[] = {49999171, 49997628, 49997582, 49996773, 49996730};
    const int npc_rng[] = {1577615, 2041353, 14061195, 9321173, 14576993};
    for (unsigned i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i) {
        fixture(STYLE_ONEHAND);
        a.act = ACT_IS_NPC; a.pcdata = NULL; a.pIndexData = &mobile;
        a.level = levels[i];
        multi_hit(&a, &b);
        int rng = number_mm();
        printf("npc=%ld hp=%ld rng=%d\n", a.level, b.hit, rng);
        assert(b.hit == npc_hp[i] && rng == npc_rng[i]);
        assert(b.hit > 0 && b.hit <= b.max_hit);
    }
    fixture(STYLE_UNARMED);
    a.init = 1; b.init = 100;
    violence_update();
    assert(a.init == get_speed(&a) && b.init == 99);
    assert(a.fighting == &b && b.fighting == &a);
    int rng = number_mm();
    printf("pulse hp=%ld rng=%d\n", b.hit, rng);
    assert(b.hit == 50000000 && rng == 11104418);
    a.combat_pause = 1;
    long hp = b.hit;
    multi_hit(&a, &b);
    one_hit(&a, &b, TYPE_UNARMED, "1");
    assert(b.hit == hp);
    multi_hit(NULL, &b); multi_hit(&a, NULL);
    one_hit(NULL, &b, TYPE_UNARMED, "1");
    a.combat_pause = 0;
    b.in_room = NULL;
    multi_hit(&a, &b);
    one_hit(&a, &b, TYPE_UNARMED, "1");
    assert(b.hit == hp);
}

static void label_lifetime(void)
{
    for (int unarmed = 0; unarmed <= 1; ++unarmed) {
        fixture(unarmed ? STYLE_UNARMED : STYLE_ONEHAND);
        /* Warm the engine's allocator pools and all avoidance/damage paths. */
        for (int i = 0; i < 128; ++i) one_hit(&a, &b, TYPE_SLASH, "1");
        long allocations = nAllocPerm;
        for (int i = 0; i < 256; ++i) one_hit(&a, &b, TYPE_SLASH, "1");
        assert(b.hit < b.max_hit && b.hit > 0);
        assert(nAllocPerm == allocations);
    }
}

static void friction_incapacitation(void)
{
    fixture(STYLE_UNARMED);
    DESCRIPTOR_DATA da = {0}, db = {0};
    char output_a[32768] = {0}, output_b[32768] = {0};
    da.character = &a; db.character = &b;
    da.connected = db.connected = CON_PLAYING;
    da.outbuf = output_a; db.outbuf = output_b;
    da.outsize = sizeof(output_a); db.outsize = sizeof(output_b);
    a.desc = &da; b.desc = &db;
    AFFECT_DATA af = {0};
    af.type = gsn_friction_curse; af.duration = 1;
    affect_to_char(&a, &af);
    a.hit = 50;
    b.position = POS_STANDING; /* Standing defenders do not auto-parry. */
    assert(is_affected(&a, gsn_friction_curse));
    a.beginners_luck = 1;
    long hp = b.hit;
    one_hit(&a, &b, TYPE_UNARMED, "1");
    assert(a.hit <= 0 && a.position < POS_FIGHTING);
    assert(b.hit == hp);
    while (a.affected) affect_remove(&a, a.affected);
    a.desc = b.desc = NULL;
}

static void bad_procs(void)
{
    const long invalid[] = {MAX_SKILL, 999, MAX_SKILL + 1000, LONG_MAX};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        fixture(STYLE_ONEHAND);
        mainhand.value[0] = invalid[i];
        multi_hit(&a, &b);
        assert(b.affected == NULL && b.hit > 0);
    }
    long empty = -1;
    for (long i = 1; i < MAX_SKILL; ++i)
        if (skill_table[i].spell_fun == NULL) { empty = i; break; }
    assert(empty > 0);
    fixture(STYLE_ONEHAND);
    mainhand.value[0] = empty;
    multi_hit(&a, &b);
    assert(b.affected == NULL);
    /* An object-only spell must never receive a CHAR_DATA masquerading as OBJ_DATA. */
    long object_spell = skill_lookup("enchant weapon");
    assert(object_spell > 0 && skill_table[object_spell].target == TAR_OBJ_INV);
    fixture(STYLE_ONEHAND);
    mainhand.value[0] = object_spell;
    multi_hit(&a, &b);
    assert(b.affected == NULL);
    const long inert[] = {0, -1, LONG_MIN, 1000};
    for (unsigned i = 0; i < sizeof(inert) / sizeof(inert[0]); ++i) {
        fixture(STYLE_ONEHAND); mainhand.value[0] = inert[i];
        multi_hit(&a, &b);
        assert(b.affected == NULL);
    }
    long armor = skill_lookup("armor");
    assert(armor > 0);
    for (int prefix = 0; prefix <= 1; ++prefix) {
        fixture(STYLE_ONEHAND);
        mainhand.value[0] = armor + prefix * 1000;
        multi_hit(&a, &b);
        assert(is_affected(&b, armor));
        assert(b.affected->modifier == 20);
    }
    while (b.affected) affect_remove(&b, b.affected);
    /* TAR_DEATH is historical character damage, not an object target. */
    long death = skill_lookup("cause light");
    assert(death > 0 && skill_table[death].target == TAR_DEATH);
    fixture(STYLE_ONEHAND);
    mainhand.value[0] = death;
    multi_hit(&a, &b);
    assert(b.hit < b.max_hit && b.hit > 0);
}

int main(int argc, char **argv)
{
    gsn_friction_curse = skill_lookup("friction curse");
    gsn_brittle_bones = skill_lookup("brittle bones");
    gsn_alter_weight = skill_lookup("alter weight");
    assert(gsn_friction_curse > 0 && gsn_brittle_bones > 0 && gsn_alter_weight > 0);
    if (argc > 1 && !strcmp(argv[1], "--baseline")) { baseline(); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--bad-proc")) { bad_procs(); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--leak")) { label_lifetime(); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--friction")) { friction_incapacitation(); return 0; }
    baseline();
    bad_procs();
    label_lifetime();
    friction_incapacitation();
    puts("native attack pulse/styles/NPC thresholds, proc validation and label reuse passed");
    return 0;
}
