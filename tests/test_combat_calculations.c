/* Real combat calculations, synthetic actors, fixed RNG seeds; no live data. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"

void init_mm(void);
int number_mm(void);

static CHAR_DATA a, b;
static PC_DATA pa, pb;
static ROOM_INDEX_DATA room;
static AREA_DATA area;

static void assist_room_chain(void)
{
    static CHAR_DATA helper, victim, local, foreign;
    static PC_DATA ph, pv, pl, pf;
    static ROOM_INDEX_DATA here, elsewhere;
    static AREA_DATA zone;
    int saw_local = 0;
    clear_char(&helper); clear_char(&victim);
    clear_char(&local); clear_char(&foreign);
    memset(&ph, 0, sizeof(ph)); memset(&pv, 0, sizeof(pv));
    memset(&pl, 0, sizeof(pl)); memset(&pf, 0, sizeof(pf));
    memset(&here, 0, sizeof(here)); memset(&elsewhere, 0, sizeof(elsewhere));
    memset(&zone, 0, sizeof(zone));
    helper.pcdata = &ph; victim.pcdata = &pv;
    local.pcdata = &pl; foreign.pcdata = &pf;
    helper.name = "Helper"; victim.name = "Victim";
    local.name = "Local"; foreign.name = "Foreign";
    helper.in_room = victim.in_room = local.in_room = &here;
    foreign.in_room = &elsewhere;
    here.area = elsewhere.area = &zone;
    here.light = elsewhere.light = 2;
    helper.level = victim.level = local.level = foreign.level = LEV_AWA;
    local.leader = foreign.leader = &victim;
    here.people = &victim;
    victim.next_in_room = &local;
    victim.next = &foreign; /* Deliberately different global/room ordering. */
    for (int n = 0; n < 256; ++n) {
        CHAR_DATA *target = combat_assist_target(&helper, &victim);
        assert(target == &victim || target == &local);
        if (target == &local) saw_local = 1;
    }
    assert(saw_local);
    local.next_in_room = &foreign; /* Reject a stale foreign-room list member too. */
    for (int n = 0; n < 32; ++n) {
        CHAR_DATA *target = combat_assist_target(&helper, &victim);
        assert(target == &victim || target == &local);
    }
    assert(combat_assist_target(NULL, &victim) == NULL);
    assert(combat_assist_target(&helper, NULL) == NULL);
    helper.in_room = NULL;
    assert(combat_assist_target(&helper, &victim) == NULL);
    helper.in_room = &elsewhere;
    assert(combat_assist_target(&helper, &victim) == NULL);
}

static void fixture(void)
{
    clear_char(&a);
    clear_char(&b);
    memset(&pa, 0, sizeof(pa));
    memset(&pb, 0, sizeof(pb));
    memset(&room, 0, sizeof(room));
    memset(&area, 0, sizeof(area));
    a.name = "Mathactor";
    b.name = "Mathtarget";
    a.pcdata = &pa;
    b.pcdata = &pb;
    a.level = b.level = LEV_AWA;
    a.position = b.position = POS_FIGHTING;
    a.fighting = &b;
    b.fighting = &a;
    a.in_room = b.in_room = &room;
    room.area = &area;
    room.light = 2;
    room.people = &a;
    a.next_in_room = &b;
    a.hit = b.hit = a.max_hit = b.max_hit = 10000;
    a.armor = b.armor = 1000;
    a.hitroll = b.hitroll = 1500;
    a.style[0] = b.style[0] = STYLE_UNARMED;
    a.style[STYLE_UNARMED] = b.style[STYLE_UNARMED] = 100;
    for (int n = 0; n < 5; ++n) a.stats[n] = b.stats[n] = 8;
}

static void profiles(void)
{
    const struct {
        long cls, speed, cap, soak, avoid, attack;
    } cases[] = {
        {CLASS_NONE, 18, 1300, 400, 37, 23},
        {CLASS_VAMPIRE, 14, 1790, 550, 52, 38},
        {CLASS_WEREWOLF, 12, 1600, 460, 46, 32},
        {CLASS_SORCERER, 18, 1300, 400, 37, 23},
        {CLASS_DROW, 12, 1600, 400, 54, 38},
        {CLASS_HIGHLANDER, 14, 1900, 550, 65, 45},
        {CLASS_MAGE, 13, 1700, 400, 52, 31},
        {CLASS_DRAGON, 9, 2200, 625, 58, 44},
        {CLASS_NINJA, 15, 1400, 400, 39, 35},
        {CLASS_MONK, 14, 1400, 600, 53, 34},
        {CLASS_ELEM, 14, 1900, 745, 53, 41},
    };
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        fixture();
        a.class = cases[n].cls;
        switch (a.class) {
        case CLASS_VAMPIRE:
            pa.celerity = 1;
            pa.powers[DISC_CELERITY] = 3;
            pa.powers[DISC_POTENCE] = pa.powers[DISC_FORTITUDE] = 2;
            break;
        case CLASS_WEREWOLF:
            pa.rank = 2;
            pa.tribes[TRIBE_SILENT_STRIDERS] = 3;
            pa.tribes[TRIBE_BONE_GNAWERS] = 2;
            pa.auspice[AUSPICE_AHROUN] = 2;
            break;
        case CLASS_DROW:
            pa.drow[DROW_SPEED] = 3;
            SET_BIT(pa.drow[DROW_POWERS], DROW_SKIP);
            break;
        case CLASS_HIGHLANDER:
            pa.celerity = 1;
            pa.highland[HIGH_ATTACK] = 2;
            pa.highland[HIGH_DEFENSE] = 3;
            pa.highland[HIGH_WEAPON] = 4;
            break;
        case CLASS_MAGE:
            a.spheres[MTIM] = 2;
            a.spheres[MENT] = 1;
            break;
        case CLASS_DRAGON:
            SET_BIT(a.polyaff, POLY_DRAGONFORM);
            a.vampgen = 4;
            break;
        case CLASS_NINJA:
            pa.ninja[NINJA_NINGENNO] = 3;
            pa.ninja[NINJA_CHIKYU] = 2;
            break;
        case CLASS_MONK:
            pa.monk[MONK_ART] = 5;
            break;
        case CLASS_ELEM:
            pa.powers[ELEM_WIND] = pa.powers[ELEM_TIME] = 6;
            pa.powers[ELEM_MOON] = pa.powers[ELEM_FIRE] = 6;
            pa.powers[ELEM_EARTH] = 7;
            pa.powers[ELEM_INFIN] = 8;
            pa.powers[ELEM_WATER] = 5;
            pa.powers[ELEM_SUN] = 6;
            SET_BIT(a.itemaffect, ITEMA_AQUABARRIER | ITEMA_SUNSHIELD | ITEMA_AURA);
            break;
        }
        long actual[] = {get_speed(&a), get_damcap(&a), get_soak(&a),
                         avoidblow(&a), breakavoid(&a)};
        long expected[] = {cases[n].speed, cases[n].cap, cases[n].soak,
                           cases[n].avoid, cases[n].attack};
        for (unsigned k = 0; k < 5; ++k) {
            if (actual[k] != expected[k])
                fprintf(stderr, "class %ld calculation %u: %ld != %ld\n",
                        cases[n].cls, k, actual[k], expected[k]);
            assert(actual[k] == expected[k]);
        }
    }
}

static void stance_and_truncation(void)
{
    const struct { long style, avoid, attack; } styles[] = {
        {STYLE_UNARMED, 37, 23}, {STYLE_ONEHAND, 27, 28},
        {STYLE_SHIELD, 27, 28}, {STYLE_TWOHAND, 22, 38},
        {STYLE_NATURAL, 22, 28}, {STYLE_TWOWEAPON, 17, 28},
    };
    for (unsigned n = 0; n < sizeof(styles) / sizeof(styles[0]); ++n) {
        fixture();
        a.style[0] = styles[n].style;
        a.style[styles[n].style] = 100;
        assert(avoidblow(&a) == styles[n].avoid);
        assert(breakavoid(&a) == styles[n].attack);
    }
    fixture();
    a.style[STYLE_UNARMED] = 199;
    assert(avoidblow(&a) == 60);
    assert(breakavoid(&a) == 27);
    a.stance[0] = STANCE_VIPER;
    a.stance[STANCE_VIPER] = 200;
    assert(get_speed(&a) == 16);
    b.stance[0] = STANCE_TIGER;
    assert(get_speed(&a) == 18);
    a.stance[0] = STANCE_MONGOOSE;
    a.stance[STANCE_MONGOOSE] = 200;
    assert(avoidblow(&a) == 70);
    b.stance[0] = STANCE_MONKEY;
    assert(avoidblow(&a) == 60);
    a.stance[0] = STANCE_CRANE;
    a.stance[STANCE_CRANE] = 200;
    assert(breakavoid(&a) == 37);
    b.stance[0] = STANCE_DRAGON;
    assert(breakavoid(&a) == 27);
    a.stance[0] = STANCE_CRAB;
    a.stance[STANCE_CRAB] = 200;
    assert(get_soak(&a) == 600);
    b.stance[0] = STANCE_MANTIS;
    assert(get_soak(&a) == 400);
    a.stats[STAT_DEX] = 1000;
    a.class = CLASS_NINJA;
    pa.ninja[NINJA_NINGENNO] = 10;
    assert(get_speed(&a) == 6);
}

static void npc_boundaries_and_rng(void)
{
    fixture();
    SET_BIT(a.act, ACT_IS_NPC);
    a.pcdata = NULL;
    const long levels[] = {499, 500, 749, 750, 1000};
    const long av[] = {49, 48, 50, 50, 52};
    const long br[] = {49, 50, 74, 75, 77};
    for (unsigned n = 0; n < 5; ++n) {
        a.level = levels[n];
        assert(avoidblow(&a) == av[n]);
        assert(breakavoid(&a) == br[n]);
    }
    for (int sight = 0; sight < 5; ++sight) {
      for (int weapon = 0; weapon <= 1; ++weapon) {
        for (int seed = 1; seed <= 16; ++seed) {
            fixture();
            if (sight == 1 || sight >= 3) SET_BIT(a.affected_by, AFF_BLIND);
            if (sight == 2 || sight >= 3) SET_BIT(b.affected_by, AFF_BLIND);
            if (sight == 4) {
                a.class = b.class = CLASS_DROW;
                SET_BIT(pa.drow[DROW_POWERS], DROW_BFIGHT);
                SET_BIT(pb.drow[DROW_POWERS], DROW_BFIGHT);
            }
            current_time = seed;
            init_mm();
            long avoid = number_range(1, avoidblow(&b));
            long attack = number_range(1, breakavoid(&a));
            if (!weapon) avoid *= .5;
            if (!get_drow(&b, DROW_BFIGHT) && !can_see(&b, &a)) avoid *= .75;
            if (!get_drow(&a, DROW_BFIGHT) && !can_see(&a, &b)) attack *= .75;
            int after = number_mm();
            init_mm();
            assert(check_avoid(&a, &b, weapon) == (avoid > attack));
            assert(number_mm() == after);
        }
      }
    }
    fixture();
    current_time = 77;
    init_mm();
    int first = number_mm();
    init_mm();
    a.beginners_luck = 2;
    assert(!check_avoid(&a, &b, TRUE));
    assert(a.beginners_luck == 1 && number_mm() == first);
    init_mm();
    b.position = POS_STANDING;
    assert(!check_avoid(&a, &b, TRUE));
    assert(a.beginners_luck == 1 && number_mm() == first);
}

static void brittle_expiry(void)
{
    for (int multiple = 0; multiple < 2; ++multiple) {
    fixture();
    AFFECT_DATA af = {0}, recycled = {0};
    if (multiple) {
        af.type = skill_lookup("armor");
        af.modifier = 7;
        af.location = APPLY_NONE;
        affect_to_char(&a, &af);
    }
    af.type = gsn_brittle_bones;
    af.duration = 5;
    af.modifier = 1;
    af.location = APPLY_NONE;
    affect_to_char(&a, &af);
    if (multiple) {
        af.modifier = 2;
        affect_to_char(&a, &af);
    }
    AFFECT_DATA *saved_free = affect_free;
    recycled.type = gsn_brittle_bones;
    recycled.modifier = 9;
    affect_free = &recycled;
    assert(get_soak(&a) == 200);
    assert(!is_affected(&a, gsn_brittle_bones));
    if (multiple) {
        assert(a.affected != NULL && a.affected->next == NULL);
        assert(a.affected->modifier == 7);
    } else assert(a.affected == NULL);
    /* Stripping the live effect must not walk into the recycled affect pool. */
    assert(recycled.modifier == 9);
    affect_free = saved_free;
    assert(get_soak(&a) == 400);
    if (multiple) affect_strip(&a, skill_lookup("armor"));
    }
}

static void philodox_not_tribe(void)
{
    fixture();
    a.class = CLASS_WEREWOLF;
    long baseline = get_damcap(&a);
    pa.auspice[AUSPICE_PHILODOX] = 3;
    assert(get_damcap(&a) == baseline + 100);
    pa.auspice[AUSPICE_PHILODOX] = 0;
    pa.tribes[TRIBE_CHILDREN_OF_GAIA] = 3;
    assert(get_damcap(&a) == baseline);
}

static void drow_metrics(long values[5])
{
    values[0]=get_speed(&a);values[1]=char_damroll(&a);
    values[2]=get_damcap(&a);values[3]=avoidblow(&a);
    values[4]=breakavoid(&a);
}

static void drow_speed_profiles(int invalid)
{
    const long styles[]={STYLE_UNARMED,STYLE_NATURAL,STYLE_ONEHAND,
                         STYLE_SHIELD,STYLE_TWOHAND,STYLE_TWOWEAPON};
    const long bad[]={-1,LONG_MIN,5,LONG_MAX};
    unsigned long long hash=1469598103934665603ULL;
    unsigned rows=0,refusals=0;
    for(int wounded=0;wounded<2;++wounded)
    for(int skip=0;skip<2;++skip)
    for(unsigned style=0;style<sizeof styles/sizeof *styles;++style)
    for(long rank=0;rank<=4;++rank) {
        fixture();a.class=CLASS_DROW;a.damroll=250;
        a.style[0]=styles[style];a.style[styles[style]]=100;
        if(wounded)a.hit=5000;
        if(skip)SET_BIT(pa.drow[DROW_POWERS],DROW_SKIP);
        pa.drow[DROW_SPEED]=rank;
        long expected[5];drow_metrics(expected);
        for(unsigned k=0;k<5;++k) {
            unsigned long long n=(unsigned long long)expected[k];
            for(unsigned byte=0;byte<8;++byte) {
                hash^=(n>>(byte*8))&255U;hash*=1099511628211ULL;
            }
        }
        rows++;
        if(invalid&&rank==0)
        for(unsigned bad_rank=0;bad_rank<sizeof bad/sizeof *bad;++bad_rank) {
            long actual[5];pa.drow[DROW_SPEED]=bad[bad_rank];
            PC_DATA before=pa;drow_metrics(actual);
            assert(!memcmp(&before,&pa,sizeof pa));
            for(unsigned k=0;k<5;++k)assert(actual[k]==expected[k]);
            assert(pa.drow[DROW_SPEED]==bad[bad_rank]);
            refusals++;
        }
    }
    printf("native Drow Speed: rows=%u hash=%llu invalid projections=%u\n",
           rows,hash,refusals);
    assert(rows==120&&hash==2722730096019922979ULL);
    assert(!invalid||refusals==96);
    assert(get_drow_speed(NULL)==0);
    fixture();a.class=CLASS_DROW;
    a.pcdata=NULL;assert(get_drow_speed(&a)==0);a.pcdata=&pa;
    pa.drow[DROW_SPEED]=4;assert(get_drow_speed(&a)==4);
    a.class=CLASS_NONE;assert(get_drow_speed(&a)==0);
    SET_BIT(a.act,ACT_IS_NPC);a.class=CLASS_DROW;
    assert(get_drow_speed(&a)==0);
}

int main(int argc, char **argv)
{
    assist_room_chain();
    gsn_brittle_bones = skill_lookup("brittle bones");
    gsn_friction_curse = skill_lookup("friction curse");
    gsn_alter_weight = skill_lookup("alter weight");
    assert(gsn_brittle_bones >= 0 && gsn_friction_curse >= 0 && gsn_alter_weight >= 0);
    if (argc > 1 && !strcmp(argv[1], "--expiry")) { brittle_expiry(); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--philodox")) { philodox_not_tribe(); return 0; }
    if(argc>1&&!strcmp(argv[1],"--drow-speed-golden")) { drow_speed_profiles(0);return 0; }
    if(argc>1&&!strcmp(argv[1],"--drow-speed")) { drow_speed_profiles(1);return 0; }
    if(argc>1&&!strcmp(argv[1],"--drow-speed-overflow")) {
        fixture();a.class=CLASS_DROW;pa.drow[DROW_SPEED]=LONG_MAX;
        (void)char_damroll(&a);return 0;
    }
    if(argc>1&&!strcmp(argv[1],"--drow-speed-min")) {
        fixture();a.class=CLASS_DROW;pa.drow[DROW_SPEED]=LONG_MIN;
        (void)get_speed(&a);return 0;
    }
    profiles();
    stance_and_truncation();
    npc_boundaries_and_rng();
    if (argc <= 1 || strcmp(argv[1], "--baseline")) {
        brittle_expiry();
        philodox_not_tribe();
        drow_speed_profiles(1);
    }
    puts("native combat profiles, truncation, stance suppression and RNG checks passed");
    return 0;
}
