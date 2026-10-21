/* Independent native trainer boundaries, raw-data checks, no conversion.
 * Reuse the existing numerical native-cap oracle, not replacement getters. */
#define main numeric_fixture_main
#include "test_hybrid_readiness.c"
#undef main
#include "../Genesis/src/classes/hybrid/mastery.h"
#include "../Genesis/src/classes/hybrid/build.h"

static const long discs[]={DISC_ANIMALISM,DISC_AUSPEX,DISC_CELERITY,DISC_CHIMERSTRY,
    DISC_DOMINATE,DISC_FORTITUDE,DISC_NECROMANCY,DISC_OBFUSCATE,DISC_OBTENEBRATION,
    DISC_POTENCE,DISC_PRESENCE,DISC_PROTEAN,DISC_QUIETUS,DISC_SERPENTIS,
    DISC_THAUMATURGY,DISC_VICISSITUDE};
static const char *const disc_args[]={"animalism","auspex","celerity","chimerstry",
    "dominate","fortitude","necromancy","obfuscate","obtenebration","potence",
    "presence","protean","quietus","serpentis","thaumaturgy","vicissitude"};
static const char *const sphere_args[]={"correspondence learn","life learn",
    "prime learn","entropy learn","mind learn","spirit learn","forces learn",
    "matter learn","time learn"};
static const long opposite[]={0,2,1,4,3,6,5,8,7};
static size_t checks;

static void prepared(long parent,long variant)
{
    maxed(parent,0);
    actor.clan="Ventrue";
    actor.primal=1000000;
    player.cland[0]=DISC_DOMINATE;
    player.cland[1]=DISC_FORTITUDE;
    player.cland[2]=DISC_PRESENCE;
    switch(parent){
    case CLASS_VAMPIRE:
        if(variant==1)actor.clan="Caitiff";
        if(variant==2)actor.clan="Panders";
        for(size_t i=0;i<sizeof(discs)/sizeof(*discs);i++){
            long d=discs[i],cap=variant?6:5;
            if(!variant&&(d==DISC_DOMINATE||d==DISC_FORTITUDE||d==DISC_PRESENCE))cap=7;
            player.powers[d]=d==DISC_THAUMATURGY?5:cap;
        }
        break;
    case CLASS_WEREWOLF:
        player.rank=4;
        player.tgift[0]=variant%3;player.tgift[1]=variant%5;player.tgift[2]=variant%13;
        for(long i=0;i<3;i++)player.breed[i]=i==player.tgift[0]?5:4;
        for(long i=0;i<5;i++)player.auspice[i]=i==player.tgift[1]?5:4;
        for(long i=0;i<13;i++)player.tribes[i]=i==player.tgift[2]?5:4;
        break;
    case CLASS_MAGE:
        player.cland[0]=0;player.cland[1]=1;player.cland[2]=2;
        for(long i=0;i<9;i++)actor.spheres[i]=i<3?5:4;
        actor.arete=10;break;
    case CLASS_ELEM:{
        long f=ELEM_FIRE+variant%8;
        player.powers[ELEM_CHOICE]=f;
        for(long i=ELEM_FIRE;i<=ELEM_INFIN;i++)
            player.powers[i]=i==f?10:i==opposite[f]?6:8;
        break;
    }
    case CLASS_SORCERER:for(long i=0;i<6;i++)player.colors[i]=5;break;
    case CLASS_HIGHLANDER:for(long i=0;i<4;i++)player.highland[i]=6;break;
    case CLASS_NINJA:for(long i=0;i<3;i++)player.ninja[i]=10;break;
    case CLASS_MONK:for(long i=0;i<3;i++)player.monk[i]=8;break;
    case CLASS_DROW:player.drow[DROW_POWERS]=(DROW_MAX<<1)-1;player.drow[DROW_SPEED]=4;break;
    case CLASS_DRAGON:
        player.dragcol=variant%6;actor.vampgen=AGE_WYRM;
        player.dragskills=(DRAG_SHIELD<<1)-1;break;
    default:assert(0);
    }
}
static HYBRID_PREREQUISITES read_prerequisites(void)
{
    HYBRID_PREREQUISITES r;
    unsigned char before[sizeof(actor)],pc_before[sizeof(player)];
    memcpy(before,&actor,sizeof actor);memcpy(pc_before,&player,sizeof player);
    init_mm();long rng=number_range(1,1000000);init_mm();
    assert(hybrid_prerequisites_read(&actor,&r));
    assert(number_range(1,1000000)==rng);
    assert(!memcmp(before,&actor,sizeof actor)&&!memcmp(pc_before,&player,sizeof player));
    assert(r.mastery.count&&r.mastery.count<=HYBRID_MASTERY_ITEMS);
    checks++;return r;
}
static void is_ready(void)
{
    HYBRID_PREREQUISITES r=read_prerequisites();
    if(!r.requirements_met){
        fprintf(stderr,"Readiness failed: class %ld, check %zu, numeric %zu/%zu\n",
                actor.class,checks,r.numeric.met,r.numeric.applicable);
        for(size_t i=0;i<r.mastery.count;i++)
            if(r.mastery.items[i].status!=HYBRID_READY_MET&&r.mastery.items[i].status!=HYBRID_READY_EXEMPT)
                fprintf(stderr,"%s: %ld/%ld (status %d)\n",r.mastery.items[i].name,
                        r.mastery.items[i].current,r.mastery.items[i].required,r.mastery.items[i].status);
    }
    assert(r.requirements_met&&r.mastery.met==r.mastery.applicable&&!r.mastery.review);
}
static void is_blocked(void)
{
    HYBRID_PREREQUISITES r=read_prerequisites();
    assert(!r.requirements_met);
}
static long *rank_storage(const HYBRID_MASTERY_ITEM *v)
{
    switch(v->kind){
    case HYBRID_MASTERY_DISCIPLINE:return &player.powers[v->index];
    case HYBRID_MASTERY_BREED:return &player.breed[v->index];
    case HYBRID_MASTERY_AUSPICE:return &player.auspice[v->index];
    case HYBRID_MASTERY_TRIBE:return &player.tribes[v->index];
    case HYBRID_MASTERY_WOLF_RANK:return &player.rank;
    case HYBRID_MASTERY_SPHERE:return &actor.spheres[v->index];
    case HYBRID_MASTERY_ARETE:return &actor.arete;
    case HYBRID_MASTERY_ELEMENT:return &player.powers[v->index];
    case HYBRID_MASTERY_COLOR:return &player.colors[v->index];
    case HYBRID_MASTERY_HIGHLAND:return &player.highland[v->index];
    case HYBRID_MASTERY_NINJA:return &player.ninja[v->index];
    case HYBRID_MASTERY_MONK:return &player.monk[v->index];
    case HYBRID_MASTERY_DROW_SPEED:return &player.drow[DROW_SPEED];
    case HYBRID_MASTERY_DRAGON_AGE:return &actor.vampgen;
    default:return NULL;
    }
}
static void boundaries(void)
{
    for(size_t p=0;p<sizeof(parents)/sizeof(*parents);p++){
        prepared(parents[p],0);is_ready();
        HYBRID_PREREQUISITES base=read_prerequisites();
        for(size_t i=0;i<base.mastery.count;i++){
            const HYBRID_MASTERY_ITEM *v=&base.mastery.items[i];
            long *slot=rank_storage(v);
            if(slot){
                long old=*slot;
                if(v->kind==HYBRID_MASTERY_DRAGON_AGE)*slot=AGE_WYRM+1;
                else *slot=v->required-1;
                is_blocked();
                const long bad[]={v->kind==HYBRID_MASTERY_DRAGON_AGE?
                                  AGE_WYRM-1:v->required+1,LONG_MIN,LONG_MAX};
                for(size_t j=0;j<3;j++){
                    *slot=bad[j];HYBRID_PREREQUISITES r=read_prerequisites();
                    assert(!r.requirements_met&&r.mastery.review);
                }
                *slot=old;is_ready();
            }else if(v->kind==HYBRID_MASTERY_DROW||v->kind==HYBRID_MASTERY_DRAGON){
                slot=v->kind==HYBRID_MASTERY_DROW?&player.drow[DROW_POWERS]:&player.dragskills;
                long old=*slot;*slot&=~v->index;is_blocked();*slot=old;is_ready();
            }
        }
        player.quest=49999;is_blocked();player.quest=50000;is_ready();
        player.quest=LONG_MIN;is_blocked();player.quest=LONG_MAX;is_ready();
        player.hybrid_present=1;is_blocked();player.hybrid_present=0;
        player.player_load_failed=1;is_blocked();player.player_load_failed=0;is_ready();
        actor.max_hit=49999;is_blocked();actor.max_hit=50000;
        actor.max_mana=49999;is_blocked();actor.max_mana=50000;is_ready();
        actor.level=LEV_ST;
        for(int i=STAT_STR;i<=STAT_WIS;i++)actor.stats[i]=stat_max(&actor,i);
        is_ready();player.quest=49999;is_blocked();
    }
}
static void choices_and_transients(void)
{
    const long bad[]={-1,LONG_MIN,LONG_MAX};
    for(size_t v=0;v<3;v++){
        prepared(CLASS_VAMPIRE,(long)v);is_ready();
        for(size_t i=0;i<sizeof(discs)/sizeof(*discs);i++){
            long index=discs[i],old=player.powers[index];
            player.powers[index]=-1;is_blocked();player.powers[index]=old;
        }
        /* Optional locked out-of-clan discipline, never an unavailable-rank bypass. */
        player.powers[DISC_NECROMANCY]=0;is_ready();
        player.powers[DISC_NECROMANCY]=-1;is_blocked();
        player.powers[DISC_NECROMANCY]=v?6:5;is_ready();
        player.powers[DISC_ANIMALISM]=0;is_blocked();
    }
    prepared(CLASS_VAMPIRE,0);
    player.cland[0]=DISC_THAUMATURGY;player.powers[DISC_DOMINATE]=5;is_ready();
    for(long slot=0;slot<20;slot++)if(slot==4||slot==8||slot==16||slot==19){
        prepared(CLASS_VAMPIRE,0);player.powers[slot]=1;is_blocked();
    }
    for(long parent=CLASS_VAMPIRE;parent<=CLASS_MAGE;parent*=32){
        prepared(parent,0);
        for(long i=0;i<3;i++)for(size_t v=0;v<3;v++){
            long old=player.cland[i];player.cland[i]=bad[v];is_blocked();player.cland[i]=old;
        }
        player.cland[1]=player.cland[0];is_blocked();
    }
    for(long a=0;a<9;a++)for(long b=0;b<9;b++)for(long c=0;c<9;c++){
        if(a==b||a==c||b==c)continue;
        prepared(CLASS_MAGE,0);player.cland[0]=a;player.cland[1]=b;player.cland[2]=c;
        for(long i=0;i<9;i++)actor.spheres[i]=(i==a||i==b||i==c)?5:4;
        is_ready();
    }
    for(long a=0;a<3;a++)for(long b=0;b<5;b++)for(long c=0;c<13;c++){
        prepared(CLASS_WEREWOLF,0);
        player.tgift[0]=a;player.tgift[1]=b;player.tgift[2]=c;
        for(long i=0;i<3;i++)player.breed[i]=i==a?5:4;
        for(long i=0;i<5;i++)player.auspice[i]=i==b?5:4;
        for(long i=0;i<13;i++)player.tribes[i]=i==c?5:4;
        is_ready();
    }
    for(long f=0;f<8;f++){prepared(CLASS_ELEM,f);is_ready();}
    for(long f=0;f<6;f++){prepared(CLASS_DRAGON,f);is_ready();}
    for(size_t i=0;i<3;i++){
        prepared(CLASS_WEREWOLF,0);player.tgift[i]=-1;is_blocked();
        prepared(CLASS_ELEM,0);player.powers[ELEM_CHOICE]=bad[i];
        HYBRID_PREREQUISITES r=read_prerequisites();
        assert(!r.requirements_met&&r.mastery.review);
        prepared(CLASS_DRAGON,0);player.dragcol=bad[i];r=read_prerequisites();
        assert(!r.requirements_met&&r.mastery.review);
    }
    prepared(CLASS_ELEM,0);player.powers[ELEM_CHOICE]=0;
    for(long i=ELEM_FIRE;i<=ELEM_INFIN;i++)player.powers[i]=8;
    is_blocked(); /* Finish choosing the original build first. */
    prepared(CLASS_DROW,0);player.drow[DROW_POWERS]|=DROW_GLOBE;is_ready();
    player.drow[DROW_POWERS]|=1L<<25;is_blocked();
    prepared(CLASS_DRAGON,0);player.dragskills|=DRAG_WINGSOUT|DRAG_HORNSOUT;is_ready();
    player.dragskills|=1L<<25;is_blocked();
    prepared(CLASS_NINJA,0);player.ninja[NINJA_KI]=LONG_MIN;
    player.ninja[NINJA_HARAKIRI]=LONG_MAX;is_ready();
    prepared(CLASS_MONK,0);player.monk[3]=LONG_MAX;is_ready();
}
static void native_lesson(void (*command)(CHAR_DATA *,char *),const char *arg,long *rank,long cap)
{
    char argument[128];long primal=actor.primal,old=*rank;
    assert(old==cap);
    snprintf(argument,sizeof argument,"%s",arg);command(&actor,argument);
    assert(*rank==old&&actor.primal==primal);
    *rank=cap-1;
    snprintf(argument,sizeof argument,"%s",arg);command(&actor,argument);
    assert(*rank==cap&&actor.primal<primal);
    is_ready();
}
static void native_trainers(void)
{
    prepared(CLASS_HIGHLANDER,0);
    const char *high[]={"weaponry","attack","defense","powers"};
    for(long i=0;i<4;i++)native_lesson(do_increase,high[i],&player.highland[i],6);
    prepared(CLASS_MONK,0);
    const char *monk[]={"tech gain","mantra gain","art gain"};
    for(long i=0;i<3;i++)native_lesson(do_monklearn,monk[i],&player.monk[i],8);
    prepared(CLASS_NINJA,0);
    const char *ninja[]={"sora improve","chikyu improve","ningenno improve"};
    for(long i=0;i<3;i++)native_lesson(do_principles,ninja[i],&player.ninja[i],10);
    prepared(CLASS_SORCERER,0);
    const char *color[]={"purple","red","blue","green","yellow","black"};
    for(long i=0;i<6;i++)native_lesson(do_colors,color[i],&player.colors[i],5);
    prepared(CLASS_MAGE,0);
    for(long i=0;i<9;i++)native_lesson(do_spheres,sphere_args[i],&actor.spheres[i],i<3?5:4);
    for(long v=0;v<3;v++){
        prepared(CLASS_VAMPIRE,v);
        for(size_t i=0;i<sizeof(discs)/sizeof(*discs);i++)
            native_lesson(do_discipline,disc_args[i],&player.powers[discs[i]],player.powers[discs[i]]);
    }
    prepared(CLASS_ELEM,0);
    do_element(&actor,"remove focus");
    for(long i=ELEM_FIRE;i<=ELEM_INFIN;i++)assert(player.powers[i]==8);
    const char *elem[]={"fire","water","earth","wind","sun","moon","time","infinity"};
    for(long i=ELEM_FIRE;i<=ELEM_INFIN;i++){
        char arg[80];snprintf(arg,sizeof arg,"%s improve",elem[i-1]);
        long old=actor.primal;do_element(&actor,arg);assert(actor.primal==old&&player.powers[i]==8);
        player.powers[i]=7;do_element(&actor,arg);assert(player.powers[i]==8&&actor.primal<old);
    }
    do_element(&actor,"fire focus");is_ready();
    for(long f=0;f<8;f++){
        char arg[80];do_element(&actor,"remove focus");
        snprintf(arg,sizeof arg,"%s focus",elem[f]);do_element(&actor,arg);
        assert(player.powers[ELEM_CHOICE]==f+ELEM_FIRE);is_ready();
    }
    prepared(CLASS_WEREWOLF,0);
    const char *breeds[]={"homid","lupus","metis"};
    const char *auspices[]={"ragabash","theurge","philodox","galliard","ahroun"};
    const char *tribes[]={"black furies","bone gnawers","children of gaia","fianna",
        "get of fenris","glass walkers","red talons","shadow lords","silent striders",
        "silver fangs","stargazers","uktena","wendigos"};
    for(long i=0;i<3;i++)native_lesson(do_gifts,breeds[i],&player.breed[i],i==0?5:4);
    for(long i=0;i<5;i++)native_lesson(do_gifts,auspices[i],&player.auspice[i],i==0?5:4);
    for(long i=0;i<13;i++)native_lesson(do_gifts,tribes[i],&player.tribes[i],i==0?5:4);
    do_advance(&actor,"");assert(player.rank==4);
    player.rank=3;player.score[SCORE_NUM_QUEST]=100000;actor.mkill=100000;
    do_advance(&actor,"");assert(player.rank==4);is_ready();
    prepared(CLASS_MAGE,0);
    actor.exp=5000000;do_arete(&actor,"");assert(actor.arete==10&&actor.exp==5000000);
    actor.arete=9;do_arete(&actor,"");assert(actor.arete==10&&actor.exp==0);is_ready();
    prepared(CLASS_DROW,0);
    ROOM_INDEX_DATA room={0};room.people=&actor;actor.in_room=&room;
    /* Native autosave explicitly skips pre-awakened actors: real grants,
     * no save stubs, filesystem writes, or fabricated ownership permissions. */
    actor.level=LEV_SLE;
    const char *drow[]={"acidbolt","llothsweb","darkness","faeriefire","spiderbite",
        "dualwield","blindfight","summonspider","spidereyes","readaura","darkvision",
        "heighten","poisonimmunity","skipdisc","attunement","might","toughness"};
    for(long i=0;i<17;i++){
        char arg[80];snprintf(arg,sizeof arg,"self %s",drow[i]);
        player.classpoints[CP_CURRENT]=1000000;
        do_prayer(&actor,arg);assert(player.classpoints[CP_CURRENT]==1000000);
        player.drow[DROW_POWERS]&=~(1L<<i);
        do_prayer(&actor,arg);
        assert((player.drow[DROW_POWERS]&(1L<<i))&&player.classpoints[CP_CURRENT]<1000000);
        is_ready();
    }
    player.classpoints[CP_CURRENT]=1000000;
    do_prayer(&actor,"self speed");assert(player.classpoints[CP_CURRENT]==1000000);
    player.drow[DROW_SPEED]=3;do_prayer(&actor,"self speed");
    assert(player.drow[DROW_SPEED]==4&&player.classpoints[CP_CURRENT]==990000);is_ready();
    prepared(CLASS_DRAGON,0);
    const char *drag[]={"fangs","nightsight","horns","claws","wings","breath","tail",
        "truesight","dust","scry","magic","readaura","shield"};
    for(long i=0;i<13;i++){
        player.classpoints[CP_CURRENT]=1000000;
        long old=player.classpoints[CP_CURRENT];
        do_dragonskill(&actor,(char *)drag[i]);assert(player.classpoints[CP_CURRENT]==old);
        player.dragskills&=~(1L<<i);
        do_dragonskill(&actor,(char *)drag[i]);
        assert((player.dragskills&(1L<<i))&&player.classpoints[CP_CURRENT]<old);is_ready();
    }
    long old=player.classpoints[CP_CURRENT];do_dragonskill(&actor,"age");
    assert(actor.vampgen==AGE_WYRM&&player.classpoints[CP_CURRENT]==old);
    actor.vampgen=AGE_ANCIENT;player.classpoints[CP_CURRENT]=1000000;
    do_dragonskill(&actor,"age");
    assert(actor.vampgen==AGE_WYRM&&player.classpoints[CP_CURRENT]==900000&&player.quest==47000);
    player.quest=50000;is_ready();
}
static void native_builds(void)
{
    assert(hybrid_build_count()==25);
    for(size_t b=0;b<hybrid_build_count();b++){
        const HYBRID_BUILD_DEFINITION *build=hybrid_build_at(b);
        prepared(build->parent,0);
        if(build->parent==CLASS_VAMPIRE){
            actor.clan=(char *)build->name;
            memcpy(player.cland,build->branches,sizeof player.cland);
            for(size_t d=0;d<sizeof(discs)/sizeof(*discs);d++){
                long i=discs[d];
                player.powers[i]=i==DISC_THAUMATURGY?5:
                    i==player.cland[0]||i==player.cland[1]||i==player.cland[2]?7:5;
            }
        }else if(build->parent==CLASS_MAGE){
            for(long i=0;i<3;i++)player.cland[i]=build->branches[i];
            for(long slot=0;slot<3;slot++)if(player.cland[slot]<0){
                for(long i=0;i<9;i++)if(i!=player.cland[0]&&i!=player.cland[1]&&i!=player.cland[2]){
                    player.cland[slot]=i;break;
                }
            }
            for(long i=0;i<9;i++)actor.spheres[i]=
                i==player.cland[0]||i==player.cland[1]||i==player.cland[2]?5:4;
        }else if(build->parent==CLASS_DRAGON)player.dragcol=build->color;
        else assert(0);
        is_ready();
    }
}
static void preview_and_invalid(void)
{
    HYBRID_MASTERY m;HYBRID_PREREQUISITES r;
    assert(!hybrid_mastery_read(NULL,&m)&&!m.count);
    assert(!hybrid_prerequisites_read(NULL,&r)&&!r.mastery.count&&!r.requirements_met);
    assert(!hybrid_mastery_read(&actor,NULL)&&!hybrid_prerequisites_read(&actor,NULL));
    prepared(CLASS_MONK,0);actor.pcdata=NULL;
    assert(!hybrid_mastery_read(&actor,&m));actor.pcdata=&player;
    SET_BIT(actor.act,ACT_IS_NPC);assert(!hybrid_prerequisites_read(&actor,&r));
    REMOVE_BIT(actor.act,ACT_IS_NPC);actor.class=CLASS_MONK|CLASS_NINJA;
    assert(!hybrid_mastery_read(&actor,&m)&&!m.count);
    hybrid_mastery_show(NULL);
    prepared(CLASS_MONK,0);
    memset(&descriptor,0,sizeof descriptor);memset(output,0,sizeof output);
    actor.desc=&descriptor;descriptor.character=&actor;descriptor.connected=CON_PLAYING;
    descriptor.outbuf=output;descriptor.outsize=sizeof output;
    do_selfbrid(&actor,"info Iron Disciple");
    output[descriptor.outtop]='\0';
    assert(strstr(output,"Original-class mastery: 3/3"));
    assert(strstr(output,"Prerequisite preview: requirements met"));
    assert(strstr(output,"Conversion remains disabled."));
    assert(player.quest==50000&&actor.class==CLASS_MONK&&!player.hybrid_present);
}
int main(void)
{
    init_mm();
    boundaries();choices_and_transients();native_trainers();native_builds();preview_and_invalid();
    printf("Hybrid mastery: %zu read-only prerequisite checks and native training boundaries passed\n",checks);
    return 0;
}
