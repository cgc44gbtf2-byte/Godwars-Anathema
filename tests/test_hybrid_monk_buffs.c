/* Real command, combat-death, weather, and record access paths. */
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern void init_mm(void);
extern void weather_update(void);
static CHAR_DATA actors[4];
static PC_DATA pcs[4];
static DESCRIPTOR_DATA descs[3];
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static MOB_INDEX_DATA mob;
static char outputs[3][32768];
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
typedef struct snapshot { long v[17];uint64_t text; } SNAPSHOT;

static void fixture(long primary,long secondary,long rank,long focus)
{
    memset(actors,0,sizeof(actors));memset(pcs,0,sizeof(pcs));
    memset(descs,0,sizeof(descs));memset(outputs,0,sizeof(outputs));
    memset(&room,0,sizeof(room));memset(&area,0,sizeof(area));
    memset(&mob,0,sizeof(mob));
    room.area=&area;room.light=1;room.vnum=900;
    char_list=room.people=actors;descriptor_list=descs;
    first_fight=last_fight=NULL;current_time=123;
    time_info.hour=5;world_affects=0;weather_info.sunlight=SUN_LIGHT;
    for(int n=0;n<4;n++){
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->name=ch->short_descr=n==0?"alpha":n==1?"beta":n==2?"gamma":"goblin";
        ch->long_descr="";ch->contingency="";
        ch->pcdata=n==3?NULL:&pcs[n];ch->class=n==0?primary:CLASS_NONE;
        ch->level=LEV_AWA;ch->status=100;ch->position=POS_STANDING;
        ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=50000;
        ch->in_room=&room;
        ch->next=ch->next_in_room=n==3?NULL:&actors[n+1];
        ch->style[0]=STYLE_UNARMED;for(int i=0;i<5;i++)ch->stats[i]=3;
        if(n==3){ch->act=ACT_IS_NPC;ch->pIndexData=&mob;mob.vnum=9003;}
        else {
            ch->desc=&descs[n];pcs[n].love=pcs[n].conding="";
            pcs[n].flag=FLAG_PK;
            descs[n].character=ch;descs[n].connected=CON_PLAYING;
            descs[n].outbuf=outputs[n];descs[n].outsize=sizeof(outputs[n]);
            descs[n].next=n==2?NULL:&descs[n+1];
        }
    }
    actors[1].leader=actors;actors[2].leader=actors;actors[3].leader=actors;
    pcs[0].monk[MONK_MANTRA]=primary==CLASS_MONK?rank:8;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_MONK){
            if(focus>=0)assert(hybrid_secondary_specialize(&state,CLASS_MONK,focus));
            long allowed=focus==MONK_MANTRA?rank:UMIN(rank,4);
            assert(hybrid_secondary_rank_write(&state,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA,allowed));
            /* Valid raw rank, but an unselected tree must not borrow it. */
            state.secondary_progression.monk[MONK_MANTRA]=rank;
        }
        assert(hybrid_player_attach(actors,&state));
    }
    init_mm();
}

static void run(int skill,int mode,SNAPSHOT *s)
{
    char command[64];strcpy(command,skill?"lifecloak":"almighty beta");
    switch(mode){
    case 1:if(!skill)strcpy(command,"almighty group");break;
    case 2:if(!skill)strcpy(command,"almighty alpha");break;
    case 3:SET_BIT(actors[1].more,MORE_FAVOUR);break;
    case 4:SET_BIT(actors[0].more,MORE_FAVOUR);
           SET_BIT(actors[1].more,MORE_FAVOUR);break;
    case 5:for(int i=0;i<3;i++)SET_BIT(actors[i].more,MORE_FAVOUR);break;
    case 6:actors[0].mana=4999;break;
    case 7:actors[0].mana=5000;break;
    case 8:actors[0].mana=19999;if(!skill)strcpy(command,"almighty group");break;
    case 9:actors[0].mana=20000;if(!skill)strcpy(command,"almighty group");break;
    case 10:if(!skill)strcpy(command,"almighty");break;
    case 11:if(!skill)strcpy(command,"almighty missing");break;
    case 12:if(!skill)strcpy(command,"almighty me");break;
    case 13:break;
    case 14:actors[0].hit=49999;break;
    case 15:actors[0].mana=49999;break;
    case 16:SET_BIT(actors[0].more,MORE_CLOAK);break;
    case 17:break;
    }
    PC_DATA before=pcs[0];long primary=actors[0].class;
    interpret(actors,command);
    assert(!memcmp(&before,&pcs[0],sizeof(before))&&actors[0].class==primary);
    if(skill&&mode==17){actors[0].hit=0;update_pos(actors);}
    memset(s,0,sizeof(*s));
    long values[]={actors[0].more,actors[1].more,actors[2].more,actors[3].more,
        actors[0].hit,actors[0].max_hit,actors[0].mana,actors[0].wait,
        actors[0].position,actors[1].hit,actors[1].mana,
        actors[2].hit,actors[2].mana,
        actors[0].in_room==&room,actors[1].in_room==&room,
        actors[0].fighting!=NULL,number_range(1,1000000)};
    memcpy(s->v,values,sizeof(values));
    s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<3;n++)for(int i=0;i<descs[n].outtop;i++)
        s->text=(s->text^(unsigned char)outputs[n][i])*UINT64_C(1099511628211);
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(1469598103934665603);
    for(int skill=0;skill<2;skill++)for(long rank=0;rank<=8;rank++)
    for(int mode=0;mode<18;mode++){
        fixture(CLASS_MONK,0,rank,-1);SNAPSHOT s;run(skill,mode,&s);
        char row[1024];int used=snprintf(row,sizeof(row),"%d,%ld,%d",skill,rank,mode);
        for(int i=0;i<17;i++)used+=snprintf(row+used,sizeof(row)-(size_t)used,",%ld",s.v[i]);
        used+=snprintf(row+used,sizeof(row)-(size_t)used,",%" PRIu64 "\n",s.text);
        assert(used>0&&(size_t)used<sizeof(row));
        if(emit)fputs(row,stdout);
        for(int i=0;i<used;i++)hash=(hash^(unsigned char)row[i])*UINT64_C(1099511628211);
    }
    return hash;
}

static void boundaries(int mode)
{
    fixture(CLASS_MONK,0,8,-1);
    if(mode==1){
        room.people=&actors[1];actors[1].next_in_room=actors;
        actors[0].next_in_room=&actors[2];
        do_almighty(actors,"");
        assert(IS_MORE(actors,MORE_FAVOUR)&&!IS_MORE(&actors[1],MORE_FAVOUR)
               &&actors[0].mana==45000);
    }else if(mode==2){
        do_almighty(actors,"goblin");
        assert(IS_MORE(&actors[3],MORE_FAVOUR)&&actors[0].mana==45000);
        descriptor_list=NULL;weather_update();
        assert(!IS_MORE(&actors[3],MORE_FAVOUR));
    }else if(mode==3){
        descriptor_list=NULL;
        SET_BIT(actors[0].more,MORE_FAVOUR);SET_BIT(actors[1].more,MORE_FAVOUR);
        SET_BIT(actors[3].more,MORE_FAVOUR);
        weather_update();
        assert(!IS_MORE(actors,MORE_FAVOUR)&&!IS_MORE(&actors[1],MORE_FAVOUR)
               &&!IS_MORE(&actors[3],MORE_FAVOUR));
    }else if(mode==4){
        actors[0].class=CLASS_NINJA;SET_BIT(actors[0].more,MORE_CLOAK);
        actors[0].hit=0;update_pos(actors);
        assert(!IS_MORE(actors,MORE_CLOAK)&&actors[0].hit==0);
    }else if(mode==5){
        actors[0].hit=actors[0].max_hit=LONG_MAX;
        do_lifecloak(actors,"");
        assert(actors[0].hit==LONG_MAX/5*4+(LONG_MAX%5)*4/5
               &&actors[0].mana==0&&IS_MORE(actors,MORE_CLOAK));
        actors[0].hit=0;update_pos(actors);
        assert(actors[0].hit==LONG_MAX/5&&!IS_MORE(actors,MORE_CLOAK));
    }else if(mode==6){
        pcs[0].monk[MONK_MANTRA]=0;
        SET_BIT(actors[0].more,MORE_CLOAK);actors[0].hit=0;
        update_pos(actors);
        assert(actors[0].hit==0&&!IS_MORE(actors,MORE_CLOAK));
    }else if(mode==7){
        actors[0].max_mana=actors[0].mana=0;
        do_lifecloak(actors,"");
        assert(!IS_MORE(actors,MORE_CLOAK)&&actors[0].hit==50000);
    }else if(mode==8){
        actors[0].hit=actors[0].max_hit=3;
        do_lifecloak(actors,"");
        assert(IS_MORE(actors,MORE_CLOAK));
        actors[0].hit=0;update_pos(actors);
        assert(actors[0].hit==1&&!IS_MORE(actors,MORE_CLOAK));
    }
}

static void secondary_repro(void)
{
    fixture(CLASS_NINJA,CLASS_MONK,8,MONK_MANTRA);
    do_lifecloak(actors,"");
    assert(IS_MORE(actors,MORE_CLOAK)&&actors[0].hit==40000
           &&actors[0].mana==0);
    actors[0].hit=0;update_pos(actors);
    assert(actors[0].hit==10000&&!IS_MORE(actors,MORE_CLOAK));
}

static void paired(void)
{
    unsigned secondary=0,original=0;
    for(unsigned p=0;p<9;p++)for(long rank=0;rank<=8;rank++)
    for(long focus=-1;focus<3;focus++)for(int skill=0;skill<2;skill++)
    for(int mode=0;mode<18;mode++){
        SNAPSHOT expected,actual;
        long effective=focus==MONK_MANTRA?rank:UMIN(rank,4);
        fixture(CLASS_MONK,0,effective,-1);run(skill,mode,&expected);
        fixture(parents[p],CLASS_MONK,rank,focus);run(skill,mode,&actual);
        if(memcmp(&expected,&actual,sizeof(expected))){
            fprintf(stderr,"Monk buff mismatch:p%u rank%ld focus%ld skill%d mode%d\n",
                    p,rank,focus,skill,mode);
            for(int i=0;i<17;i++)if(expected.v[i]!=actual.v[i])
                fprintf(stderr," v%d expected%ld actual%ld\n",i,expected.v[i],actual.v[i]);
            fprintf(stderr," text expected%" PRIu64 " actual%" PRIu64 "\n",
                    expected.text,actual.text);
            abort();
        }
        secondary++;
    }
    for(unsigned p=0;p<9;p++)for(long rank=0;rank<=8;rank++)
    for(int skill=0;skill<2;skill++)for(int mode=0;mode<18;mode++){
        SNAPSHOT expected,actual;
        fixture(CLASS_MONK,0,rank,-1);run(skill,mode,&expected);
        fixture(CLASS_MONK,parents[p],rank,-1);run(skill,mode,&actual);
        assert(!memcmp(&expected,&actual,sizeof(expected)));original++;
    }
    assert(secondary==11664&&original==2916);
}

static void malformed(void)
{
    for(int bad=0;bad<12;bad++){
        fixture(CLASS_NINJA,CLASS_MONK,8,MONK_MANTRA);
        switch(bad){
        case 0:pcs[0].hybrid.secondary_progression.monk[MONK_MANTRA]=9;break;
        case 1:pcs[0].hybrid.secondary_class=CLASS_DRAGON;break;
        case 2:pcs[0].hybrid_present=0;break;
        case 3:pcs[0].hybrid_present=2;break;
        case 4:pcs[0].player_load_failed=1;break;
        case 5:pcs[0].hybrid.primary_class=CLASS_MONK;break;
        case 6:actors[0].class=CLASS_NONE;break;
        case 7:actors[0].pcdata=NULL;break;
        case 8:SET_BIT(actors[0].act,ACT_IS_NPC);break;
        case 9:actors[0].in_room=NULL;break;
        case 10:pcs[0].hybrid.secondary_progression.monk[MONK_MANTRA]=-1;break;
        case 11:pcs[0].hybrid.secondary_progression.full_branch=MONK_TECH;break;
        }
        do_almighty(actors,"beta");do_lifecloak(actors,"");
        assert(actors[0].more==0&&actors[1].more==0&&actors[0].hit==50000
               &&actors[0].mana==50000);
        if(bad!=9){
            SET_BIT(actors[0].more,MORE_CLOAK);actors[0].hit=0;
            update_pos(actors);
            assert(!IS_MORE(actors,MORE_CLOAK)&&actors[0].hit<=0);
        }
    }
    do_almighty(NULL,"");do_lifecloak(NULL,"");
}

static void ownership(void)
{
    fixture(CLASS_NINJA,CLASS_MONK,8,MONK_MANTRA);
    SET_BIT(actors[0].more,MORE_GUARDIAN);
    do_lifecloak(actors,"");actors[0].hit=actors[0].max_hit;
    actors[0].mana=actors[0].max_mana;
    do_almighty(actors,"alpha");
    assert(IS_MORE(actors,MORE_CLOAK)&&IS_MORE(actors,MORE_FAVOUR)
           &&IS_MORE(actors,MORE_GUARDIAN)&&actors[0].mana==45000);
    long mana=actors[0].mana;
    do_almighty(actors,"alpha");
    assert(actors[0].mana==mana); /* The same benefit cannot be paid for twice. */
    actors[0].hit=0;update_pos(actors);
    assert(actors[0].hit==10000&&!IS_MORE(actors,MORE_CLOAK)
           &&IS_MORE(actors,MORE_FAVOUR)&&IS_MORE(actors,MORE_GUARDIAN));
    actors[0].hit=0;update_pos(actors);
    assert(actors[0].hit==0); /* One-use rescue, not permanent invulnerability. */

    fixture(CLASS_NINJA,CLASS_MONK,8,MONK_MANTRA);
    do_lifecloak(actors,"");
    pcs[0].hybrid.secondary_progression.full_branch=MONK_TECH;
    actors[0].hit=0;update_pos(actors);
    assert(actors[0].hit==0&&!IS_MORE(actors,MORE_CLOAK));

    fixture(CLASS_MONK,CLASS_NINJA,8,-1);
    do_lifecloak(actors,"");actors[0].hit=actors[0].max_hit;
    actors[0].mana=actors[0].max_mana;
    do_almighty(actors,"alpha");
    SET_BIT(actors[0].more,MORE_GUARDIAN);
    descriptor_list=NULL;weather_update();
    assert(!IS_MORE(actors,MORE_FAVOUR)&&IS_MORE(actors,MORE_CLOAK)
           &&IS_MORE(actors,MORE_GUARDIAN));
    actors[0].hit=0;update_pos(actors);
    assert(actors[0].hit==10000&&!IS_MORE(actors,MORE_CLOAK)
           &&IS_MORE(actors,MORE_GUARDIAN));

    fixture(CLASS_NINJA,CLASS_MONK,8,MONK_MANTRA);
    long soak=get_soak(actors),cap=get_damcap(actors);
    do_almighty(actors,"alpha");
    assert(get_soak(actors)==soak+100&&get_damcap(actors)==cap+100);
    do_almighty(actors,"alpha");
    assert(get_soak(actors)==soak+100&&get_damcap(actors)==cap+100
           &&actors[0].mana==45000);

    /* The actual switched descriptor owns an NPC, not its original PC. */
    fixture(CLASS_NINJA,CLASS_MONK,8,MONK_MANTRA);
    do_lifecloak(actors,"");
    for(int n=0;n<4;n++)SET_BIT(actors[n].more,MORE_FAVOUR);
    actors[0].desc=NULL;descs[0].original=actors;
    descs[0].character=&actors[3];actors[3].desc=&descs[0];
    SET_BIT(actors[0].extra,EXTRA_OSWITCH);
    descs[1].connected=CON_NOTE_TEXT;actors[2].desc=NULL;descs[1].next=NULL;
    SET_BIT(actors[2].more,MORE_HEAD);
    weather_update();
    for(int n=0;n<4;n++)assert(!IS_MORE(&actors[n],MORE_FAVOUR));
    assert(IS_MORE(actors,MORE_CLOAK)&&IS_MORE(&actors[2],MORE_HEAD));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_dump(1);return 0;}
    if(argc==2&&!strcmp(argv[1],"--secondary-repro")){secondary_repro();return 0;}
    if(argc==3&&!strcmp(argv[1],"--boundary")){
        boundaries(atoi(argv[2]));return 0;
    }
    assert(argc==1);
    assert(native_dump(0)==UINT64_C(0x440019fda0994e7f));
    for(int mode=1;mode<=8;mode++)boundaries(mode);
    secondary_repro();paired();malformed();ownership();
    puts("Monk buffs:324 native snapshots,14580 paired replays,8 boundaries,"
         "12 corrupt guards,independent one-use cloak and dawn expiry.");
    return 0;
}
