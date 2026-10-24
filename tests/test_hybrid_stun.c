/* Real command/interrupt/timers. No permission, rank or combat stubs. */
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern void init_mm(void);
static CHAR_DATA actors[3];
static PC_DATA pcs[3];
static DESCRIPTOR_DATA descs[3];
static char output[3][32768];
static ROOM_INDEX_DATA room,elsewhere;
static AREA_DATA area;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
typedef struct snapshot { long v[20];uint64_t messages; } SNAPSHOT;

static void fixture(long primary,long secondary,long rank,long focus)
{
    for(int n=0;n<3;n++)if(actors[n].fighting)stop_fighting(&actors[n],FALSE);
    memset(pcs,0,sizeof(pcs));memset(descs,0,sizeof(descs));memset(output,0,sizeof(output));
    memset(&room,0,sizeof(room));memset(&elsewhere,0,sizeof(elsewhere));
    memset(&area,0,sizeof(area));room.area=&area;room.light=1;
    char_list=room.people=actors;descriptor_list=NULL;first_fight=last_fight=NULL;
    current_time=123;
    for(int n=0;n<3;n++){
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->name=ch->short_descr=n==0?"alpha":n==1?"beta":"gamma";
        ch->contingency="";ch->pcdata=&pcs[n];ch->desc=&descs[n];
        ch->class=n==0?primary:n==2?CLASS_MONK:CLASS_NONE;
        ch->level=LEV_AWA;ch->status=100;ch->position=POS_STANDING;
        ch->hit=ch->max_hit=10000;ch->mana=10000;ch->max_mana=20000;ch->in_room=&room;
        ch->next=ch->next_in_room=n==2?NULL:&actors[n+1];
        ch->style[0]=STYLE_UNARMED;
        for(int i=0;i<5;i++)ch->stats[i]=3;
        pcs[n].love=pcs[n].conding="";pcs[n].flag=FLAG_PK;
        descs[n].character=ch;descs[n].connected=CON_PLAYING;
        descs[n].outbuf=output[n];descs[n].outsize=sizeof(output[n]);
    }
    pcs[2].monk[MONK_TECH]=7;
    pcs[0].monk[MONK_TECH]=primary==CLASS_MONK?rank:8; /* Primary-bank decoy. */
    pcs[0].monk[MONK_MANTRA]=8;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_MONK){
            if(focus>=0)assert(hybrid_secondary_specialize(&state,CLASS_MONK,focus));
            long allowed=focus==MONK_TECH?rank:UMIN(rank,4);
            assert(hybrid_secondary_rank_write(&state,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_TECH,allowed));
            /* A structurally valid migrated raw bank must still be masked. */
            if(rank>allowed){
                assert(!hybrid_secondary_rank_write(&state,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_TECH,rank));
                state.secondary_progression.monk[MONK_TECH]=rank;
            }
        }
        assert(hybrid_player_attach(actors,&state));
    }
}

static void run_case(int mode,SNAPSHOT *s)
{
    char text[64]="stunfist beta";
    switch(mode){
    case 1:actors[0].fighting=&actors[1];actors[1].fighting=actors;
           actors[0].position=actors[1].position=POS_FIGHTING;
           strcpy(text,"stunfist");break;
    case 2:strcpy(text,"stunfist");break;
    case 3:strcpy(text,"stunfist absent");break;
    case 4:strcpy(text,"stunfist alpha");break;
    case 5:actors[1].position=POS_SLEEPING;break;
    case 6:actors[1].in_room=&elsewhere;break;
    case 7:actors[0].in_room=NULL;break;
    case 8:actors[0].position=POS_RESTING;break;
    case 9:actors[0].affected_by=AFF_CHARM;actors[0].master=&actors[1];break;
    case 10:actors[1].stun_immunity=1;break;
    case 11:actors[1].combat_pause=2;break;
    case 12:actors[0].combat_pause=1;break;
    case 13:room.room_flags=ROOM_SAFE;break;
    case 14:actors[1].act=ACT_IS_NPC|ACT_NOKILL;actors[1].pcdata=NULL;actors[1].desc=NULL;break;
    case 15:actors[1].desc=NULL;break;
    case 16:actors[1].affected_by=AFF_ETHEREAL;break;
    case 17:actors[0].mana=STUN_FIST_COST-1;break;
    case 18:actors[0].mana=0;break;
    case 19:actors[0].mana=STUN_FIST_COST;break;
    case 20:actors[0].mana=LONG_MAX;break;
    case 21:actors[0].mana=LONG_MIN;break;
    case 22:actors[1].act=ACT_IS_NPC;actors[1].pcdata=NULL;actors[1].desc=NULL;break;
    case 23:actors[0].level=LEV_SLE;break;
    }
    PC_DATA before=pcs[0];long primary=actors[0].class;
    init_mm();
    if(mode==7)do_stunfist(actors,"beta"); /* Native null-room guard itself. */
    else interpret(actors,text);
    assert(!memcmp(&pcs[0],&before,sizeof(before))&&actors[0].class==primary);
    memset(s,0,sizeof(*s));
    long values[]={actors[0].hit,actors[0].mana,actors[0].wait,actors[0].combat_pause,
        actors[0].stun_immunity,actors[0].position,actors[0].init,
        actors[1].hit,actors[1].mana,actors[1].wait,actors[1].combat_pause,
        actors[1].stun_immunity,actors[1].position,actors[1].init,
        actors[0].fighting==&actors[1],actors[1].fighting==actors,
        actors[0].affected_by,actors[1].affected_by,number_range(1,1000000),pcs[0].quest};
    memcpy(s->v,values,sizeof(values));s->messages=UINT64_C(1469598103934665603);
    for(int n=0;n<3;n++)for(int i=0;i<descs[n].outtop;i++)
        s->messages=(s->messages^(unsigned char)output[n][i])*UINT64_C(1099511628211);
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(1469598103934665603);
    for(long rank=-1;rank<=9;rank++)for(int mode=0;mode<24;mode++){
        fixture(CLASS_MONK,0,rank,-1);SNAPSHOT s;run_case(mode,&s);
        char row[1024];int used=snprintf(row,sizeof(row),"%ld,%d",rank,mode);
        for(size_t i=0;i<20;i++){
            assert(used>0&&(size_t)used<sizeof(row));
            used+=snprintf(row+used,sizeof(row)-(size_t)used,",%ld",s.v[i]);
        }
        assert(used>0&&(size_t)used<sizeof(row));
        used+=snprintf(row+used,sizeof(row)-(size_t)used,",%" PRIu64 "\n",s.messages);
        assert(used>0&&(size_t)used<sizeof(row));
        if(emit)fputs(row,stdout);
        for(int i=0;i<used;i++)hash=(hash^(unsigned char)row[i])*UINT64_C(1099511628211);
    }
    return hash;
}

static void paired(void)
{
    int pairs=0;
    for(int p=0;p<9;p++)for(long rank=0;rank<=8;rank++)
    for(long focus=-1;focus<3;focus++)for(int mode=0;mode<24;mode++){
        long effective=focus==MONK_TECH?rank:UMIN(rank,4);
        SNAPSHOT control,actual;
        fixture(CLASS_MONK,0,effective,-1);run_case(mode,&control);
        fixture(parents[p],CLASS_MONK,rank,focus);
        assert(hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_TECH)==effective);
        run_case(mode,&actual);assert(!memcmp(&control,&actual,sizeof(actual)));pairs++;
    }
    assert(pairs==7776);
    for(int p=0;p<9;p++)for(long rank=0;rank<=8;rank++)for(int mode=0;mode<24;mode++){
        SNAPSHOT control,actual;
        fixture(CLASS_MONK,0,rank,-1);run_case(mode,&control);
        fixture(CLASS_MONK,parents[p],rank,-1);run_case(mode,&actual);
        assert(!memcmp(&control,&actual,sizeof(actual)));pairs++;
    }
    assert(pairs==9720);
}

static void guards(void)
{
    for(int mode=0;mode<9;mode++){
        fixture(CLASS_NINJA,CLASS_MONK,7,MONK_TECH);
        if(mode==0)pcs[0].hybrid_present=0;
        if(mode==1)pcs[0].hybrid_present=2;
        if(mode==2)pcs[0].player_load_failed=1;
        if(mode==3)pcs[0].hybrid.primary_class=CLASS_DROW;
        if(mode==4)pcs[0].hybrid.secondary_class=CLASS_SORCERER;
        if(mode==5)pcs[0].hybrid.secondary_progression.monk[MONK_TECH]=9;
        if(mode==6){actors[0].act=ACT_IS_NPC;actors[0].pcdata=NULL;}
        if(mode==7){actors[0].class=CLASS_NONE;actors[0].level=LEV_IMP;}
        if(mode==8)actors[0].pcdata=NULL;
        do_stunfist(actors,"beta");
        assert(actors[0].mana==10000&&!actors[0].combat_pause&&!actors[1].combat_pause);
        assert(!actors[1].stun_immunity&&!actors[0].fighting&&!actors[1].fighting);
    }
}

static void immunity(void)
{
    for(int p=0;p<9;p++){
        fixture(parents[p],CLASS_MONK,7,MONK_TECH);
        do_stunfist(actors,"beta");
        assert(actors[0].mana==5000&&actors[0].combat_pause==STUN_FIST_RECOVERY);
        assert(actors[1].combat_pause==STUN_FIST_DURATION&&actors[1].stun_immunity==STUN_FIST_IMMUNITY);
        do_stunfist(&actors[2],"beta");
        assert(actors[2].mana==10000&&!actors[2].combat_pause);
        for(int pulse=0;pulse<STUN_FIST_IMMUNITY;pulse++){
            combat_status_update();
            if(pulse+1==STUN_FIST_RECOVERY){
                do_stunfist(actors,"beta");
                assert(actors[0].mana==5000&&!actors[0].combat_pause);
            }
        }
        assert(!actors[1].stun_immunity&&!actors[1].combat_pause);
        do_stunfist(actors,"beta");
        assert(actors[0].mana==0&&actors[1].stun_immunity==STUN_FIST_IMMUNITY);
    }
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_dump(1);return 0;}
    if(argc==2&&!strcmp(argv[1],"--secondary-repro")){
        fixture(CLASS_NINJA,CLASS_MONK,7,MONK_TECH);do_stunfist(actors,"beta");
        assert(actors[1].combat_pause==STUN_FIST_DURATION);return 0;
    }
    assert(native_dump(0)==UINT64_C(0xb967ef86a4987bad));
    paired();guards();immunity();
    for(int n=0;n<3;n++)if(actors[n].fighting)stop_fighting(&actors[n],FALSE);
    char_list=NULL;first_fight=last_fight=NULL;
    puts("Hybrid Stun Fist:9720 paired replays,9 fail-closed guards,9 shared-immunity sequences; primary banks/classes/RNG/cost/timers preserved.");
    return 0;
}
