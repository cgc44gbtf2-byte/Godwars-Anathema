/* Real poison and crossbow effects: learned immunity is not damage immunity. */
#define main ammunition_original_main
#include "test_drow_ammo.c"
#undef main
#include "classes/hybrid/player.h"
#include "classes/hybrid/access.h"

static PC_DATA victim_pc;
static DESCRIPTOR_DATA victim_desc;
static char victim_output[8192];
static const long poison_parents[]={
    CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_MONK,
    CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF
};

static void poison_fixture(long primary,long secondary,int learned,int bad)
{
    fixture();
    memset(&victim_pc,0,sizeof victim_pc);
    memset(&victim_desc,0,sizeof victim_desc);
    memset(victim_output,0,sizeof victim_output);
    target.act=0;target.pcdata=&victim_pc;target.class=primary;
    target.level=LEV_AWA;target.vampgen=9;
    target.desc=&victim_desc;target.stance[0]=-1;
    victim_desc.character=&target;victim_desc.connected=CON_PLAYING;
    victim_desc.outbuf=victim_output;victim_desc.outsize=sizeof victim_output;
    victim_pc.conding=victim_pc.love=victim_pc.pwd="";
    victim_pc.cland[0]=victim_pc.cland[1]=victim_pc.cland[2]=-1;
    if(primary==CLASS_DROW)victim_pc.drow[DROW_POWERS]=learned?DROW_PIMM:0;
    else victim_pc.drow[DROW_POWERS]=DROW_PIMM; /* stale foreign storage cannot grant */
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(&target,
            hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_DROW)
            state.secondary_progression.drow[DROW_POWERS]=learned?DROW_PIMM:0;
        assert(hybrid_player_attach(&target,&state));
    }
    switch(bad){
    case 1:victim_pc.hybrid_present=0;break;
    case 2:victim_pc.hybrid_present=2;break;
    case 3:victim_pc.player_load_failed=1;break;
    case 4:victim_pc.hybrid.hybrid_id=0;break;
    case 5:victim_pc.hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
    case 6:victim_pc.hybrid.secondary_class=CLASS_MONK;break;
    }
}

struct poison_snapshot {long hit,poison,sleep,affects,rng,dp,ammo;};
static struct poison_snapshot poison_run(int bolt_mode,int seed)
{
    PC_DATA before=victim_pc;
    long mana=target.mana,primal=target.primal;
    init_mm();
    for(int i=0;i<seed;i++)number_percent();
    if(bolt_mode){bolt(1);shoot("north target");}
    else spell_poison(gsn_poison,1000,&actor,&target);
    long affects=0;
    for(AFFECT_DATA *af=target.affected;af;af=af->next)++affects;
    struct poison_snapshot s={target.hit,!!IS_AFFECTED(&target,AFF_POISON),
        !!IS_AFFECTED(&target,AFF_SLEEP),affects,number_range(0,1000000),
        pc.classpoints[CP_CURRENT],bolts(&actor)};
    assert(!memcmp(&before,&victim_pc,sizeof before));
    assert(target.mana==mana&&target.primal==primal&&target.in_room==&away);
    return s;
}

static void native_cases(int dump)
{
    for(unsigned p=0;p<10;p++)for(int learned=0;learned<2;learned++)
    for(int mode=0;mode<2;mode++)for(int seed=0;seed<8;seed++){
        long parent=p<9?poison_parents[p]:CLASS_DROW;
        poison_fixture(parent,0,learned,0);
        struct poison_snapshot s=poison_run(mode,seed);
        if(dump)printf("%u %d %d %d %ld %ld %ld %ld %ld %ld %ld\n",
            p,learned,mode,seed,s.hit,s.poison,s.sleep,s.affects,s.rng,s.dp,s.ammo);
    }
}

static void learned_cases(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int mode=0;mode<2;mode++)for(int seed=0;seed<8;seed++){
        long primary=order?CLASS_DROW:poison_parents[p];
        long secondary=order?poison_parents[p]:CLASS_DROW;
        poison_fixture(primary,secondary,1,0);
        struct poison_snapshot actual=poison_run(mode,seed);
        assert(!actual.poison&&!actual.sleep&&!actual.affects);
        if(mode)assert(actual.hit<100000&&actual.dp==500&&actual.ammo==0);
        else assert(actual.hit==100000&&actual.dp==1000);
        poison_fixture(primary,0,order,0);
        struct poison_snapshot plain=poison_run(mode,seed);
        if(mode)assert(actual.hit==plain.hit&&actual.rng==plain.rng);
        ++count;
    }
    assert(count==288);
    for(unsigned p=0;p<9;p++)for(int bad=1;bad<=6;bad++)
    for(int mode=0;mode<2;mode++){
        poison_fixture(poison_parents[p],CLASS_DROW,1,bad);
        struct poison_snapshot actual=poison_run(mode,0);
        poison_fixture(poison_parents[p],0,0,0);
        struct poison_snapshot plain=poison_run(mode,0);
        assert(!memcmp(&actual,&plain,sizeof actual));
    }
    /* Losing immunity does not cleanse existing poison or put someone to sleep. */
    poison_fixture(CLASS_MAGE,CLASS_DROW,1,0);
    SET_BIT(target.affected_by,AFF_POISON);
    poison_run(0,0);
    assert(IS_AFFECTED(&target,AFF_POISON));
    REMOVE_BIT(victim_pc.hybrid.secondary_progression.drow[DROW_POWERS],DROW_PIMM);
    assert(!hybrid_power_drow(&target,DROW_PIMM));
}

int main(int argc,char **argv)
{
    int dump=argc==2&&!strcmp(argv[1],"--native-dump");
    if(!dump)assert(!ammunition_original_main(0,NULL));
    else {
        string_space=malloc(1024*1024);assert(string_space);
        top_string=string_space;*top_string='\0';current_time=2000000000;init_mm();
        prototype(0,OBJ_VNUM_DROW_CROSSBOW,ITEM_MISSILE);
        prototype(1,OBJ_VNUM_DROW_BOLT,ITEM_AMMO);
        mobile.short_descr="a target";
    }
    gsn_poison=skill_lookup("poison");assert(gsn_poison>=0);
    native_cases(dump);
    if(!dump)learned_cases();
    fixture();
    while(actor.carrying)extract_obj(actor.carrying);
    char_list=NULL;descriptor_list=NULL;
    if(!dump)puts("Drow immunity:320 native cases,288 learned cases,108 invalid fallbacks; real poison and crossbow damage retained.");
    return 0;
}
