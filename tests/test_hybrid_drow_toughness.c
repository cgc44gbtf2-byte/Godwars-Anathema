/* Exercise the existing soak path; this stage changes no combat formulas. */
#define main calculation_original_main
#include "test_combat_calculations.c"
#undef main
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

static const long tough_classes[]={
    CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_MONK,
    CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_DROW
};
static AFFECT_DATA brittle;

static void tough_fixture(long primary,long secondary,int learned,int context,int bad)
{
    fixture();
    a.class=primary;a.vampgen=9;
    for(int i=0;i<3;i++)pa.cland[i]=pb.cland[i]=-1;
    pa.drow[DROW_POWERS]=primary==CLASS_DROW?(learned?DROW_TOUGH:0):DROW_TOUGH;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(&a,hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_DROW)
            state.secondary_progression.drow[DROW_POWERS]=learned?DROW_TOUGH:0;
        assert(hybrid_player_attach(&a,&state));
    }
    a.stats[STAT_CON]=(context%3==0)?1:(context%3==1)?8:25;
    if(context&1)SET_BIT(a.itemaffect,ITEMA_RESISTANCE);
    if(context&2)SET_BIT(a.more,MORE_FAVOUR);
    a.stance[0]=(context&4)?STANCE_CRAB:STANCE_NONE;
    a.stance[STANCE_CRAB]=117;
    b.stance[0]=(context&8)?STANCE_MANTIS:STANCE_NONE;
    memset(&brittle,0,sizeof brittle);
    brittle.type=gsn_brittle_bones;brittle.modifier=100;
    if(context&16)a.affected=&brittle;
    switch(bad){
    case 1:pa.hybrid_present=0;break;
    case 2:pa.hybrid_present=2;break;
    case 3:pa.player_load_failed=1;break;
    case 4:pa.hybrid.hybrid_id=0;break;
    case 5:pa.hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
    case 6:pa.hybrid.secondary_class=CLASS_MONK;break;
    }
}

static long tough_run(void)
{
    PC_DATA before=pa;CHAR_DATA actor=a;
    init_mm();int expected=number_mm();init_mm();
    long value=get_soak(&a);
    assert(number_mm()==expected); /* passive calculation consumes no RNG */
    assert(!memcmp(&before,&pa,sizeof pa));
    assert(!memcmp(&actor,&a,sizeof a));
    if(a.affected)assert(brittle.modifier==99);
    return value;
}

static void native_cases(int dump)
{
    for(unsigned p=0;p<10;p++)for(int learned=0;learned<2;learned++)
    for(int context=0;context<96;context++){
        tough_fixture(tough_classes[p],0,learned,context,0);
        long soak=tough_run();
        if(dump)printf("%u %d %d %ld\n",p,learned,context,soak);
    }
}

static void learned_cases(void)
{
    unsigned pairs=0,fallbacks=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int learned=0;learned<2;learned++)for(int context=0;context<96;context++){
        long primary=order?CLASS_DROW:tough_classes[p];
        long secondary=order?tough_classes[p]:CLASS_DROW;
        /* Compare against the same primary and context before Brittle Bones. */
        tough_fixture(primary,secondary,0,context,0);
        a.affected=NULL;
        long base=tough_run();
        tough_fixture(primary,secondary,learned,context,0);
        long expected=base+(learned?300:0);
        if(context&16)expected/=2;
        assert(tough_run()==expected);
        ++pairs;
    }
    for(unsigned p=0;p<9;p++)for(int bad=1;bad<=6;bad++)
    for(int context=0;context<96;context++){
        tough_fixture(tough_classes[p],CLASS_DROW,1,context,bad);
        long actual=tough_run();
        tough_fixture(tough_classes[p],0,0,context,0);
        assert(tough_run()==actual);++fallbacks;
    }
    for(unsigned p=0;p<9;p++){
        tough_fixture(tough_classes[p],CLASS_DROW,1,0,0);
        long trained=tough_run();
        REMOVE_BIT(pa.hybrid.secondary_progression.drow[DROW_POWERS],DROW_TOUGH);
        assert(tough_run()==trained-300);
        a.act|=ACT_IS_NPC;
        assert(!hybrid_power_drow(&a,DROW_TOUGH));
    }
    assert(!hybrid_power_drow(NULL,DROW_TOUGH));
    assert(pairs==3456&&fallbacks==5184);
    puts("Toughness:3456 paired soak calculations;5184 invalid fallbacks;9 training-loss cases.");
}

int main(int argc,char **argv)
{
    int dump=argc==2&&!strcmp(argv[1],"--native-dump");
    if(!dump)assert(!calculation_original_main(1,argv));
    gsn_brittle_bones=skill_lookup("brittle bones");
    assert(gsn_brittle_bones>=0);
    native_cases(dump);
    if(!dump)learned_cases();
    a.affected=NULL;char_list=NULL;descriptor_list=NULL;
    return 0;
}
