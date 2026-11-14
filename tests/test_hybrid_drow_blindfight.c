/* Real visibility/avoidance calculations; no substituted engine functions. */
#define main calculation_original_main
#include "test_combat_calculations.c"
#undef main
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

static const long bf_classes[]={
    CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_MONK,
    CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_DROW
};

static void bf_fixture(long primary,long secondary,int learned,int side,
                       int blindness,int bad)
{
    fixture();
    a.class=b.class=CLASS_MAGE;
    CHAR_DATA *focus=side?&b:&a;
    PC_DATA *pc=focus->pcdata;
    focus->class=primary;
    a.vampgen=b.vampgen=9;
    for(int i=0;i<3;i++)pa.cland[i]=pb.cland[i]=-1;
    /* A foreign raw bank is not permission. */
    pc->drow[DROW_POWERS]=primary==CLASS_DROW?
        (learned?DROW_BFIGHT:0):DROW_BFIGHT;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(focus,
            hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_DROW)
            state.secondary_progression.drow[DROW_POWERS]=learned?DROW_BFIGHT:0;
        assert(hybrid_player_attach(focus,&state));
    }
    if(blindness&1)SET_BIT(a.extra,BLINDFOLDED);
    if(blindness&2)SET_BIT(b.extra,BLINDFOLDED);
    switch(bad){
    case 1:pc->hybrid_present=0;break;
    case 2:pc->hybrid_present=2;break;
    case 3:pc->player_load_failed=1;break;
    case 4:pc->hybrid.hybrid_id=0;break;
    case 5:pc->hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
    case 6:pc->hybrid.secondary_class=CLASS_MONK;break;
    }
}

static void bf_seed(int seed)
{
    init_mm();
    for(int n=0;n<seed;n++)number_percent();
}

struct bf_snapshot { long avoided,rng,avoid,attack,see_a,see_b; };

static struct bf_snapshot bf_run(int seed,int weap)
{
    PC_DATA oldpa=pa,oldpb=pb;
    CHAR_DATA olda=a,oldb=b;
    struct bf_snapshot out={0};
    out.avoid=avoidblow(&b);out.attack=breakavoid(&a);
    out.see_a=can_see(&a,&b);out.see_b=can_see(&b,&a);
    bf_seed(seed);
    out.avoided=check_avoid(&a,&b,weap);
    out.rng=number_mm();
    assert(!memcmp(&pa,&oldpa,sizeof pa)&&!memcmp(&pb,&oldpb,sizeof pb));
    assert(!memcmp(&a,&olda,sizeof a)&&!memcmp(&b,&oldb,sizeof b));
    return out;
}

/* Preserve native operation order, random ranges and compound truncation. */
static struct bf_snapshot bf_model(int seed,int weap,int grant_a,int grant_b)
{
    struct bf_snapshot out={0};
    out.avoid=avoidblow(&b);out.attack=breakavoid(&a);
    out.see_a=can_see(&a,&b);out.see_b=can_see(&b,&a);
    bf_seed(seed);
    long av=number_range(1,out.avoid),br=number_range(1,out.attack);
    if(!weap)av*=.5;
    if(!grant_b&&!out.see_b)av*=.75;
    if(!grant_a&&!out.see_a)br*=.75;
    out.avoided=av>br;out.rng=number_mm();
    return out;
}

static void bf_native_dump(void)
{
    for(size_t p=0;p<10;p++)for(int learned=0;learned<2;learned++)
    for(int side=0;side<2;side++)for(int blind=0;blind<4;blind++)
    for(int weap=0;weap<2;weap++)for(int seed=0;seed<32;seed++){
        bf_fixture(bf_classes[p],0,learned,side,blind,0);
        struct bf_snapshot s=bf_run(seed,weap);
        printf("%zu %d %d %d %d %d: %ld %ld %ld %ld %ld %ld\n",
            p,learned,side,blind,weap,seed,s.avoided,s.rng,
            s.avoid,s.attack,s.see_a,s.see_b);
    }
}

static void bf_pairs(void)
{
    unsigned count=0,changed=0;
    for(size_t p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int learned=0;learned<2;learned++)for(int side=0;side<2;side++)
    for(int blind=0;blind<4;blind++)for(int weap=0;weap<2;weap++)
    for(int seed=0;seed<32;seed++){
        bf_fixture(order?CLASS_DROW:bf_classes[p],
                   order?bf_classes[p]:CLASS_DROW,learned,side,blind,0);
        struct bf_snapshot expected=bf_model(seed,weap,side?0:learned,side?learned:0);
        struct bf_snapshot actual=bf_run(seed,weap);
        assert(!memcmp(&expected,&actual,sizeof actual));
        if(learned){
            struct bf_snapshot plain=bf_model(seed,weap,0,0);
            changed+=plain.avoided!=actual.avoided;
        }
        ++count;
    }
    assert(count==18432&&changed>0);
    printf("Blind Fighting: %u exact learned/untrained paired calculations; %u intentional outcomes.\n",
        count,changed);
}

static void bf_bad_records_and_guards(void)
{
    unsigned count=0;
    for(size_t p=0;p<9;p++)for(int bad=1;bad<=6;bad++)
    for(int side=0;side<2;side++)for(int blind=0;blind<4;blind++)
    for(int weap=0;weap<2;weap++)for(int seed=0;seed<8;seed++){
        bf_fixture(bf_classes[p],CLASS_DROW,1,side,blind,bad);
        struct bf_snapshot expected=bf_model(seed,weap,0,0);
        struct bf_snapshot actual=bf_run(seed,weap);
        assert(!memcmp(&expected,&actual,sizeof actual));++count;
    }
    assert(count==6912);
    bf_fixture(CLASS_MAGE,CLASS_DROW,1,0,3,0);
    a.beginners_luck=2;
    bf_seed(3);int next=number_mm();
    bf_seed(3);assert(!check_avoid(&a,&b,1)&&a.beginners_luck==1);
    assert(number_mm()==next);
    b.position=POS_STANDING;
    bf_seed(3);assert(!check_avoid(&a,&b,0)&&a.beginners_luck==1);
    assert(number_mm()==next);
    bf_fixture(CLASS_MAGE,CLASS_DROW,1,0,3,0);
    assert(hybrid_power_drow(&a,DROW_BFIGHT));
    pa.hybrid.secondary_progression.drow[DROW_POWERS]=0;
    assert(!hybrid_power_drow(&a,DROW_BFIGHT));
    struct bf_snapshot expected=bf_model(9,1,0,0),actual=bf_run(9,1);
    assert(!memcmp(&expected,&actual,sizeof actual));
    SET_BIT(a.act,ACT_IS_NPC);
    assert(!hybrid_power_drow(&a,DROW_BFIGHT));
    assert(!hybrid_power_drow(NULL,DROW_BFIGHT));
    printf("Blind Fighting: %u corrupt-record fallbacks, training loss and early-return RNG guards.\n",count);
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){bf_native_dump();return 0;}
    assert(calculation_original_main(1,argv)==0);
    bf_pairs();
    bf_bad_records_and_guards();
    return 0;
}
