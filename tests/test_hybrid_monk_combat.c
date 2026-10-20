/* Full engine, including real hit/damage calls. Wrappers observe calls and
 * inject post-callback room changes, never grant permissions or fake damage. */
#include <assert.h>
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
extern void __real_one_hit(CHAR_DATA *,CHAR_DATA *,int,char *);
extern void __real_hurt_person(CHAR_DATA *,CHAR_DATA *,long);
static CHAR_DATA actors[3],*removed;
static PC_DATA pcs[3];
static DESCRIPTOR_DATA desc;
static MOB_INDEX_DATA mobs[3];
static char output[65536];
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static int hits,hurts,callback;
static long dealt;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
static const char *commands[]={"repeat beta","aurashot beta","blitz","cripple beta","darkblaze beta"};
static const long requirements[]={1,2,3,5,6};
typedef struct snapshot { long v[23];uint64_t text; } SNAPSHOT;

void __wrap_one_hit(CHAR_DATA *ch,CHAR_DATA *victim,int dt,char *message)
{
    hits++;__real_one_hit(ch,victim,dt,message);
    if(callback==1){callback=0;char_from_room(ch);}
}

void __wrap_hurt_person(CHAR_DATA *ch,CHAR_DATA *victim,long damage)
{
    hurts++;dealt+=damage;__real_hurt_person(ch,victim,damage);
    if(callback==2){callback=0;char_from_room(ch);}
    if(callback==3){
        callback=0;char_from_room(removed);
        CHAR_DATA **link=&char_list;
        while(*link&&*link!=removed)link=&(*link)->next;
        assert(*link==removed);*link=removed->next;
        free(removed);removed=NULL;
    }
}

static void fixture(long primary,long secondary,long rank,long focus)
{
    for(int n=0;n<3;n++){
        if(actors[n].fighting)stop_fighting(&actors[n],FALSE);
        while(actors[n].affected)affect_remove(&actors[n],actors[n].affected);
    }
    memset(pcs,0,sizeof(pcs));memset(mobs,0,sizeof(mobs));
    memset(&desc,0,sizeof(desc));memset(output,0,sizeof(output));
    memset(&room,0,sizeof(room));memset(&area,0,sizeof(area));
    char_list=room.people=actors;room.area=&area;room.light=1;room.vnum=900;
    first_fight=last_fight=NULL;descriptor_list=NULL;current_time=123;
    hits=hurts=callback=0;dealt=0;removed=NULL;
    for(int n=0;n<3;n++){
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->name=ch->short_descr=n==0?"alpha":n==1?"beta":"gamma";
        ch->long_descr="";ch->contingency="";ch->class=n==0?primary:CLASS_NONE;
        ch->pcdata=&pcs[n];ch->in_room=&room;ch->level=LEV_AWA;ch->status=100;
        ch->hit=n==0?50000:50000000;ch->max_hit=50000000;
        ch->mana=ch->max_mana=50000;ch->position=POS_STANDING;
        ch->next=ch->next_in_room=n==2?NULL:&actors[n+1];
        ch->style[0]=STYLE_UNARMED;for(int i=0;i<5;i++)ch->stats[i]=3;
        pcs[n].love=pcs[n].conding="";pcs[n].flag=FLAG_PK;
        if(n){ch->act=ACT_IS_NPC;ch->pIndexData=&mobs[n];mobs[n].vnum=9000+n;}
    }
    desc.character=actors;desc.connected=CON_PLAYING;desc.outbuf=output;desc.outsize=sizeof(output);
    actors[0].desc=&desc;pcs[0].monk[MONK_TECH]=primary==CLASS_MONK?rank:8;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_MONK){
            if(focus>=0)assert(hybrid_secondary_specialize(&state,CLASS_MONK,focus));
            long allowed=focus==MONK_TECH?rank:UMIN(rank,4);
            assert(hybrid_secondary_rank_write(&state,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_TECH,allowed));
            state.secondary_progression.monk[MONK_TECH]=rank;
        }
        assert(hybrid_player_attach(actors,&state));
    }
    init_mm();
}

static void run(int skill,int mode,SNAPSHOT *s)
{
    char text[64];strcpy(text,commands[skill]);
    switch(mode){
    case 1:actors[0].mana=0;break;
    case 2:actors[0].mana=1199;break;
    case 3:actors[0].mana=1200;break;
    case 4:actors[0].mana=1999;break;
    case 5:actors[0].mana=2000;break;
    case 6:actors[0].mana=4999;break;
    case 7:actors[0].mana=5000;break;
    case 8:strcpy(text,"");strncat(text,commands[skill],strcspn(commands[skill]," "));break;
    case 9:snprintf(text,sizeof(text),"%.*s absent",(int)strcspn(commands[skill]," "),commands[skill]);break;
    case 10:room.room_flags=ROOM_SAFE;break;
    case 11:actors[1].master=actors[1].leader=actors;actors[2].master=actors[2].leader=actors;break;
    case 12:actors[0].combat_pause=1;break;
    case 13:actors[0].fighting=&actors[1];actors[1].fighting=actors;
            actors[0].position=actors[1].position=POS_FIGHTING;break;
    case 14:actors[1].affected_by=AFF_BLIND;break;
    case 15:actors[1].mana=1;break;
    }
    PC_DATA before=pcs[0];long primary=actors[0].class;
    interpret(actors,text);
    assert(!memcmp(&before,&pcs[0],sizeof(before))&&actors[0].class==primary);
    memset(s,0,sizeof(*s));
    long values[]={actors[0].hit,actors[0].mana,actors[0].wait,actors[0].combat_pause,
        actors[0].position,actors[0].init,actors[1].hit,actors[1].mana,
        actors[1].position,actors[1].affected_by,actors[1].hitroll,
        actors[2].hit,actors[2].mana,actors[2].affected_by,
        actors[0].fighting==&actors[1],actors[1].fighting==actors,
        actors[1].affected?actors[1].affected->duration:0,
        actors[1].affected?actors[1].affected->modifier:0,
        hits,hurts,dealt,number_range(1,1000000),actors[0].in_room==&room};
    memcpy(s->v,values,sizeof(values));s->text=UINT64_C(1469598103934665603);
    for(int i=0;i<desc.outtop;i++)s->text=(s->text^(unsigned char)output[i])*UINT64_C(1099511628211);
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(1469598103934665603);
    for(int skill=0;skill<5;skill++)for(long rank=0;rank<=8;rank++)for(int mode=0;mode<16;mode++){
        fixture(CLASS_MONK,0,rank,-1);SNAPSHOT s;run(skill,mode,&s);
        char row[1500];int used=snprintf(row,sizeof(row),"%d,%ld,%d",skill,rank,mode);
        for(int i=0;i<23;i++)used+=snprintf(row+used,sizeof(row)-(size_t)used,",%ld",s.v[i]);
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
        actors[0].hit=LONG_MAX-1;actors[0].max_hit=LONG_MAX;
        do_aurashot(actors,"beta");assert(actors[0].hit==LONG_MAX);
    }else if(mode==2){
        actors[1].mana=LONG_MIN;do_cripple(actors,"beta");assert(actors[1].mana==0);
    }else if(mode==3){
        actors[0].in_room=NULL;do_blitz(actors,"");assert(actors[0].mana==50000);
        actors[0].in_room=&room;
    }else if(mode==4){
        actors[0].pcdata=NULL;do_aurashot(actors,"beta");
        assert(actors[0].mana==50000&&actors[1].hit==50000000);
    }else if(mode==5){
        callback=1;do_repeat(actors,"beta");assert(hits==1&&actors[0].wait==0);
        actors[0].in_room=&room;
    }else if(mode==6){
        callback=2;do_blitz(actors,"");assert(hurts==1&&actors[0].wait==0);
        actors[0].in_room=&room;
    }else if(mode==7){
        removed=malloc(sizeof(*removed));assert(removed);*removed=actors[2];
        removed->next=removed->next_in_room=NULL;
        actors[1].next=actors[1].next_in_room=removed;
        callback=3;do_blitz(actors,"");
        assert(!removed&&hurts==1&&actors[2].hit==50000000);
    }
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_dump(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--boundary")){boundaries(atoi(argv[2]));return 0;}
    if(argc==2&&!strcmp(argv[1],"--secondary-repro")){
        fixture(CLASS_NINJA,CLASS_MONK,8,MONK_TECH);
        do_cripple(actors,"beta");assert(actors[0].mana==48000);return 0;
    }
    assert(native_dump(0)==UINT64_C(0xd7e43eace6e725b7));
    for(int mode=1;mode<=7;mode++)boundaries(mode);
    printf("Monk combat:720 native snapshots,7 numeric/lifecycle boundaries; native stage.\n");
    return 0;
}
