/* Real engine accounting and real mobile loop. No pool/update mocks. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/resources.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"
#include "classes/hybrid/build.h"

extern void init_mm(void);
extern void mobile_update(void);
extern char *string_space,*top_string;
static const long parents[]={CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,
    CLASS_DROW,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_DRAGON,CLASS_NINJA,
    CLASS_MONK,CLASS_ELEM};
static const long owners[]={CLASS_VAMPIRE,CLASS_DROW,CLASS_DRAGON,
    CLASS_NINJA,CLASS_MAGE};
static CHAR_DATA actor;
static PC_DATA pc;
static DESCRIPTOR_DATA desc;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static char output[32768];

static void fixture(long primary)
{
    current_time=123; /* clear_char stamps logon; both replay fixtures agree. */
    memset(&pc,0,sizeof(pc));memset(&desc,0,sizeof(desc));
    memset(&room,0,sizeof(room));memset(&area,0,sizeof(area));
    memset(output,0,sizeof(output));clear_char(&actor);
    actor.name=actor.short_descr="alpha";actor.contingency="";
    actor.pcdata=&pc;actor.desc=&desc;actor.in_room=&room;
    actor.class=primary;actor.level=LEV_AWA;actor.status=100;
    actor.position=POS_STANDING;
    actor.hit=actor.max_hit=actor.mana=actor.max_mana=100000;
    for(int i=0;i<5;++i)actor.stats[i]=3;
    actor.blood[BLOOD_POOL]=1000;actor.blood[BLOOD_CURRENT]=100;
    actor.vampgen=5;actor.beast=25;actor.arete=3;
    actor.quint[BODY]=2;actor.paradox=1;
    pc.classpoints[CP_CURRENT]=91;pc.classpoints[CP_TOTAL]=123;
    pc.ninja[NINJA_KI]=20;pc.ninja[NINJA_HARAKIRI]=7;
    pc.love=pc.conding="";pc.flag=FLAG_PK;pc.rank=1;
    desc.character=&actor;desc.connected=CON_PLAYING;
    desc.outbuf=output;desc.outsize=sizeof(output);
    room.area=&area;room.light=1;room.people=&actor;
    char_list=&actor;descriptor_list=NULL;first_fight=last_fight=NULL;
}

static void attach(long secondary)
{
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(&actor,
        hybrid_by_parents(actor.class,secondary),&state));
    if(secondary==CLASS_VAMPIRE)assert(hybrid_secondary_build_choose(&state,1));
    if(secondary==CLASS_MAGE)assert(hybrid_secondary_build_choose(&state,101));
    if(secondary==CLASS_DRAGON)assert(hybrid_secondary_build_choose(&state,201));
    assert(hybrid_player_attach(&actor,&state));
}

static void primary_unchanged(const CHAR_DATA *before,const PC_DATA *beforepc)
{
    PC_DATA actual=pc;
    memcpy(&actual.hybrid,&beforepc->hybrid,sizeof(actual.hybrid));
    assert(!memcmp(&actual,beforepc,sizeof(actual)));
    assert(!memcmp(&actor,before,sizeof(actor)));
}

static long *slot(enum hybrid_resource kind)
{
    HYBRID_STATE *s=&pc.hybrid;
    switch(kind) {
    case HYBRID_RESOURCE_BLOOD:return &s->secondary_blood[BLOOD_CURRENT];
    case HYBRID_RESOURCE_DROW_DP:
    case HYBRID_RESOURCE_DRAGON_DP:return &s->secondary_classpoints[CP_CURRENT];
    case HYBRID_RESOURCE_KI:return &s->secondary_progression.ninja[NINJA_KI];
    case HYBRID_RESOURCE_BODY_QUINT:return &s->secondary_progression.quint[BODY];
    default:abort();
    }
}

static long capacity(enum hybrid_resource kind)
{
    if(kind==HYBRID_RESOURCE_BLOOD)return pc.hybrid.secondary_blood[BLOOD_POOL];
    if(kind==HYBRID_RESOURCE_KI)return 300;
    if(kind==HYBRID_RESOURCE_BODY_QUINT)return pc.hybrid.secondary_progression.arete;
    return LONG_MAX;
}

static void pools(void)
{
    unsigned cases=0;
    for(size_t p=0;p<10;++p)for(int k=0;k<5;++k) {
        long owner=owners[k],value=-1;
        enum hybrid_resource kind=(enum hybrid_resource)k;
        fixture(parents[p]);
        if(parents[p]==owner) {
            CHAR_DATA before=actor;PC_DATA beforepc=pc;
            assert(hybrid_resource_read(&actor,owner,kind,&value));
            assert(value>=0);
            assert(!hybrid_secondary_resource_spend(&actor,owner,kind,0));
            assert(!hybrid_secondary_resource_gain(&actor,owner,kind,1));
            assert(!memcmp(&actor,&before,sizeof(actor)));
            assert(!memcmp(&pc,&beforepc,sizeof(pc)));
            continue;
        }
        ++cases;attach(owner);
        if(owner==CLASS_MAGE)pc.hybrid.secondary_progression.arete=10;
        CHAR_DATA before=actor;PC_DATA beforepc=pc;
        assert(hybrid_resource_read(&actor,owner,kind,&value)&&value==0);
        assert(!hybrid_secondary_resource_spend(&actor,owner,kind,1));
        assert(!hybrid_secondary_resource_gain(&actor,owner,kind,-1));
        assert(!hybrid_secondary_resource_spend(&actor,owner,kind,LONG_MIN));
        assert(!hybrid_secondary_resource_spend(&actor,actor.class,kind,0));
        assert(!memcmp(&pc,&beforepc,sizeof(pc)));
        assert(hybrid_secondary_resource_gain(&actor,owner,kind,7));
        assert(hybrid_secondary_resource_spend(&actor,owner,kind,3));
        assert(hybrid_resource_read(&actor,owner,kind,&value)&&value==4);
        int dp=owner==CLASS_DROW||owner==CLASS_DRAGON;
        if(dp)assert(pc.hybrid.secondary_classpoints[CP_TOTAL]==7);
        primary_unchanged(&before,&beforepc);
        beforepc=pc;
        assert(!hybrid_secondary_resource_gain(&actor,owner,kind,LONG_MAX));
        assert(!hybrid_secondary_resource_spend(&actor,owner,kind,LONG_MAX));
        assert(!memcmp(&pc,&beforepc,sizeof(pc)));
        *slot(kind)=capacity(kind);
        beforepc=pc;
        assert(!hybrid_secondary_resource_gain(&actor,owner,kind,1));
        assert(!memcmp(&pc,&beforepc,sizeof(pc)));
        if(dp) {
            *slot(kind)=0;pc.hybrid.secondary_classpoints[CP_TOTAL]=LONG_MAX;
            beforepc=pc;
            assert(!hybrid_secondary_resource_gain(&actor,owner,kind,1));
            assert(!memcmp(&pc,&beforepc,sizeof(pc)));
        }
        *slot(kind)=0;
        if(dp)pc.hybrid.secondary_classpoints[CP_TOTAL]=0;
        long model=0,total=0,cap=capacity(kind);
        unsigned seed=1234567;
        for(int i=0;i<500;++i) {
            seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
            long amount=(seed>>1)%16;
            int gain=seed&1;
            int expected=gain?amount<=cap-model:amount<=model;
            int got=gain?
                hybrid_secondary_resource_gain(&actor,owner,kind,amount):
                hybrid_secondary_resource_spend(&actor,owner,kind,amount);
            assert(got==expected);
            if(got) {
                model+=gain?amount:-amount;
                if(dp&&gain)total+=amount;
            }
            assert(hybrid_resource_read(&actor,owner,kind,&value)&&value==model);
            if(dp)assert(pc.hybrid.secondary_classpoints[CP_TOTAL]==total);
        }
        FILE *stream=tmpfile();HYBRID_STATE loaded;
        assert(stream&&hybrid_state_write(stream,&pc.hybrid));
        rewind(stream);assert(hybrid_state_read(stream,&loaded));fclose(stream);
        assert(!memcmp(&loaded,&pc.hybrid,sizeof(loaded)));
        primary_unchanged(&before,&beforepc);
    }
    assert(cases==45);
}

static void rejected(void)
{
    fixture(CLASS_DROW);attach(CLASS_VAMPIRE);
    CHAR_DATA before=actor;PC_DATA beforepc=pc;long value=93;
    assert(!hybrid_resource_read(NULL,CLASS_VAMPIRE,HYBRID_RESOURCE_BLOOD,&value));
    assert(!hybrid_resource_read(&actor,CLASS_VAMPIRE,HYBRID_RESOURCE_BLOOD,NULL));
    assert(!hybrid_resource_read(&actor,CLASS_VAMPIRE,(enum hybrid_resource)-1,&value));
    assert(!hybrid_secondary_resource_spend(&actor,CLASS_VAMPIRE,(enum hybrid_resource)99,0));
    assert(!hybrid_secondary_resource_gain(NULL,CLASS_VAMPIRE,HYBRID_RESOURCE_BLOOD,0));
    assert(value==93&&!memcmp(&pc,&beforepc,sizeof(pc)));
    primary_unchanged(&before,&beforepc);
    for(int i=0;i<5;++i) {
        fixture(CLASS_DROW);attach(CLASS_VAMPIRE);
        if(i==0)pc.hybrid.secondary_build=0;
        if(i==1)pc.hybrid.secondary_generation=0;
        if(i==2)pc.hybrid.primary_class=CLASS_MAGE;
        if(i==3)pc.player_load_failed=1;
        if(i==4)SET_BIT(actor.act,ACT_IS_NPC);
        before=actor;beforepc=pc;value=93;
        assert(!hybrid_resource_read(&actor,CLASS_VAMPIRE,HYBRID_RESOURCE_BLOOD,&value));
        assert(!hybrid_secondary_resource_gain(&actor,CLASS_VAMPIRE,HYBRID_RESOURCE_BLOOD,1));
        assert(!hybrid_secondary_resource_spend(&actor,CLASS_VAMPIRE,HYBRID_RESOURCE_BLOOD,0));
        assert(value==93&&!memcmp(&pc,&beforepc,sizeof(pc)));
        assert(!memcmp(&actor,&before,sizeof(actor)));
    }
    fixture(CLASS_DROW);attach(CLASS_MAGE);
    pc.hybrid.secondary_progression.quint[BODY]=500;
    assert(hybrid_resource_read(&actor,CLASS_MAGE,HYBRID_RESOURCE_BODY_QUINT,&value)&&value==500);
    assert(hybrid_secondary_resource_gain(&actor,CLASS_MAGE,HYBRID_RESOURCE_BODY_QUINT,0));
    assert(!hybrid_secondary_resource_gain(&actor,CLASS_MAGE,HYBRID_RESOURCE_BODY_QUINT,1));
    assert(hybrid_secondary_resource_spend(&actor,CLASS_MAGE,HYBRID_RESOURCE_BODY_QUINT,499));
    assert(pc.hybrid.secondary_progression.quint[BODY]==1);
}

static void real_updates(void)
{
    unsigned pairs=0;
    for(size_t p=0;p<10;++p)for(size_t q=0;q<10;++q)if(p!=q) {
        fixture(parents[p]);current_time=123;init_mm();
        mobile_update();int rng=number_range(1,100000);
        CHAR_DATA native=actor;PC_DATA nativepc=pc;
        fixture(parents[p]);attach(parents[q]);
        HYBRID_STATE old=pc.hybrid;
        current_time=123;init_mm();mobile_update();
        assert(number_range(1,100000)==rng);
        HYBRID_STATE expected=old;
        if(parents[q]==CLASS_NINJA)expected.secondary_progression.ninja[NINJA_KI]=2;
        assert(!memcmp(&expected,&pc.hybrid,sizeof(expected)));
        PC_DATA actual=pc;
        actual.hybrid_present=0;memset(&actual.hybrid,0,sizeof(actual.hybrid));
        assert(!memcmp(&actual,&nativepc,sizeof(actual)));
        assert(!memcmp(&actor,&native,sizeof(actor)));
        ++pairs;
    }
    assert(pairs==90);
}

static void ninja_timers(void)
{
    for(size_t i=0;i<10;++i)if(parents[i]!=CLASS_NINJA) {
        fixture(parents[i]);attach(CLASS_NINJA);
        HYBRID_STATE *s=&pc.hybrid;
        HYBRID_PROGRESSION *p=&s->secondary_progression;
        assert(hybrid_secondary_specialize(s,CLASS_NINJA,NINJA_NINGENNO));
        p->ninja[NINJA_SORA]=8; /* Retained native rank; effective half-cap=5. */
        p->ninja[NINJA_KI]=299;p->ninja[NINJA_HARAKIRI]=LONG_MAX;
        s->secondary_rage=LONG_MAX;actor.position=POS_MEDITATING;
        CHAR_DATA before=actor;PC_DATA beforepc=pc;
        hybrid_secondary_mobile_update(&actor);
        assert(p->ninja[NINJA_KI]==300);
        assert(p->ninja[NINJA_HARAKIRI]==LONG_MAX-1);
        assert(s->secondary_rage==LONG_MAX-1);
        primary_unchanged(&before,&beforepc);
        actor.position=POS_FIGHTING;s->secondary_rage=124;
        hybrid_secondary_mobile_update(&actor);assert(s->secondary_rage==125);
        hybrid_secondary_mobile_update(&actor);assert(s->secondary_rage==125);
        s->secondary_rage=0;p->ninja[NINJA_HARAKIRI]=0;
        SET_BIT(actor.affected_by,AFF_KAKUSU);p->ninja[NINJA_KI]=0;
        hybrid_secondary_mobile_update(&actor);
        assert(IS_AFFECTED(&actor,AFF_KAKUSU));
        assert(!p->ninja[NINJA_KI]&&!p->ninja[NINJA_HARAKIRI]&&!s->secondary_rage);
        for(int bad=0;bad<4;++bad) {
            fixture(parents[i]);attach(CLASS_NINJA);
            if(bad==0)actor.desc=NULL;
            if(bad==1)actor.in_room=NULL;
            if(bad==2)desc.connected=CON_GET_NAME;
            if(bad==3)pc.player_load_failed=1;
            before=actor;beforepc=pc;
            hybrid_secondary_mobile_update(&actor);
            assert(!memcmp(&actor,&before,sizeof(actor)));
            assert(!memcmp(&pc,&beforepc,sizeof(pc)));
        }
    }
}

int main(void)
{
    string_space=calloc(1,1024*1024);assert(string_space);
    top_string=string_space;init_mm();
    pools();rejected();real_updates();ninja_timers();
    puts("hybrid resources:45 pool orientations/22500 modeled operations;90 real mobile updates;owned costs/totals/limits/Ninja clocks/RNG passed");
    return 0;
}
