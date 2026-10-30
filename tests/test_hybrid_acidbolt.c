/* Real interpreter/combat; wrapper observes real hits and injects callback
 * removal. No rank, permission, damage or RNG stubs. */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern void init_mm(void);
extern void __real_one_hit(CHAR_DATA *,CHAR_DATA *,long,char *);
static CHAR_DATA actors[2];
static PC_DATA pcs[2];
static DESCRIPTOR_DATA descs[2];
static MOB_INDEX_DATA mob;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static char output[2][32768];
static int hits,callback;
static const long parents[]={CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,
    CLASS_WEREWOLF};
typedef struct snapshot { long v[15];uint64_t text; } SNAPSHOT;

void __wrap_one_hit(CHAR_DATA *ch,CHAR_DATA *victim,long dt,char *message)
{
    ++hits;
    __real_one_hit(ch,victim,dt,message);
    if(callback) {
        int action=callback;
        callback=0;
        stop_fighting(ch,TRUE);
        char_from_room(ch);
        if(action==2) {
            assert(char_list==ch);
            char_list=ch->next;
            descs[0].character=NULL;
            free(ch);
        }
    }
}

static void fixture(long primary,long secondary,int trained,int npc)
{
    for(int i=0;i<2;i++) {
        if(actors[i].fighting)stop_fighting(&actors[i],TRUE);
        while(actors[i].affected)affect_remove(&actors[i],actors[i].affected);
    }
    memset(pcs,0,sizeof(pcs));memset(descs,0,sizeof(descs));
    memset(output,0,sizeof(output));memset(&room,0,sizeof(room));
    memset(&area,0,sizeof(area));memset(&mob,0,sizeof(mob));
    first_fight=last_fight=NULL;descriptor_list=NULL;
    char_list=room.people=actors;room.area=&area;room.light=1;
    room.vnum=999;current_time=123;hits=callback=0;
    for(int i=0;i<2;i++) {
        CHAR_DATA *ch=&actors[i];clear_char(ch);
        ch->name=ch->short_descr=i?"beta":"alpha";
        ch->contingency="";ch->pcdata=&pcs[i];ch->desc=&descs[i];
        ch->class=i?CLASS_NONE:primary;ch->level=LEV_AWA;
        ch->status=100;ch->position=POS_STANDING;
        ch->hit=ch->max_hit=50000000;ch->mana=ch->max_mana=50000;
        ch->primal=9876;ch->exp=7654321;ch->in_room=&room;
        ch->next=ch->next_in_room=i?NULL:&actors[1];
        ch->style[0]=STYLE_UNARMED;
        for(int n=0;n<5;n++)ch->stats[n]=3;
        pcs[i].love=pcs[i].conding="";pcs[i].flag=FLAG_PK;
        pcs[i].classpoints[CP_CURRENT]=321;pcs[i].quest=50000;
        descs[i].character=ch;descs[i].connected=CON_PLAYING;
        descs[i].outbuf=output[i];descs[i].outsize=sizeof(output[i]);
    }
    if(npc) {
        actors[1].act=ACT_IS_NPC;actors[1].pIndexData=&mob;
        actors[1].desc=NULL;mob.vnum=9999;
    }
    pcs[0].drow[DROW_POWERS]=primary==CLASS_DROW&&trained?DROW_ACID:0;
    if(secondary) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actors,
            hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_DROW) {
            state.secondary_progression.drow[DROW_POWERS]=trained?DROW_ACID:0;
            state.secondary_classpoints[CP_CURRENT]=456;
        }
        assert(hybrid_player_attach(actors,&state));
    }
    init_mm();
}

static void run(int mode,SNAPSHOT *s)
{
    char command[64]="acidbolt beta";
    switch(mode) {
    case 1:strcpy(command,"acidbolt absent");break;
    case 2:strcpy(command,"acidbolt");break;
    case 3:strcpy(command,"acidbolt");set_fighting(actors,&actors[1]);break;
    case 4:room.room_flags=ROOM_SAFE;actors[0].status=actors[1].status=0;break;
    case 5:actors[1].itemaffect=ITEMA_PEACE;break;
    case 6:pcs[1].flag|=FLAG_AFK;break;
    case 7:actors[1].desc=NULL;break;
    case 8:actors[1].act|=ACT_NOKILL;break;
    case 9:actors[0].combat_pause=1;break;
    case 10:actors[0].polyaff=POLY_DRAGONFORM;break;
    case 11:actors[0].polyaff=POLY_CHIMERA;break;
    }
    PC_DATA before=pcs[0];
    interpret(actors,command);
    assert(!memcmp(&before,&pcs[0],sizeof(before)));
    memset(s,0,sizeof(*s));
    long v[]={actors[0].hit,actors[0].mana,actors[0].wait,
        actors[0].primal,actors[0].exp,actors[0].class,
        actors[1].hit,actors[1].position,actors[1].affected_by,
        actors[0].fighting==&actors[1],actors[1].fighting==actors,
        hits,number_range(1,1000000),actors[0].position,
        actors[0].combat_pause};
    memcpy(s->v,v,sizeof(v));s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<descs[0].outtop;n++)
        s->text=(s->text^(unsigned char)output[0][n])*UINT64_C(1099511628211);
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(1469598103934665603);
    for(int trained=0;trained<2;trained++)for(int npc=0;npc<2;npc++)
    for(int mode=0;mode<12;mode++) {
        SNAPSHOT s;fixture(CLASS_DROW,0,trained,npc);run(mode,&s);
        char row[1024];int used=snprintf(row,sizeof(row),"%d,%d,%d",trained,npc,mode);
        for(int n=0;n<15;n++)
            used+=snprintf(row+used,sizeof(row)-(size_t)used,",%ld",s.v[n]);
        used+=snprintf(row+used,sizeof(row)-(size_t)used,",%" PRIu64 "\n",s.text);
        assert(used>0&&(size_t)used<sizeof(row));
        if(emit)fputs(row,stdout);
        for(int n=0;n<used;n++)hash=(hash^(unsigned char)row[n])*UINT64_C(1099511628211);
    }
    return hash;
}

static void boundaries(int mode)
{
    fixture(CLASS_DROW,0,1,1);
    if(mode==1) {
        callback=1;do_acidbolt(actors,"beta");
        assert(hits==1&&actors[0].in_room==NULL&&actors[0].wait==0);
    }else if(mode==2) {
        CHAR_DATA *heap=malloc(sizeof(*heap));assert(heap);
        *heap=actors[0];char_list=room.people=heap;
        descs[0].character=heap;actors[0].next=actors[0].next_in_room=NULL;
        callback=2;do_acidbolt(heap,"beta");
        assert(hits==1&&descs[0].character==NULL);
    }else if(mode==3) {
        actors[0].in_room=NULL;do_acidbolt(actors,"beta");
        assert(!hits&&!actors[0].wait);
        actors[0].in_room=&room;
    }
}

static void paired(void)
{
    unsigned pairs=0;
    for(int p=0;p<9;p++)for(int trained=0;trained<2;trained++)
    for(int npc=0;npc<2;npc++)for(int mode=0;mode<12;mode++) {
        SNAPSHOT native,hybrid;
        fixture(CLASS_DROW,0,trained,npc);run(mode,&native);
        fixture(parents[p],CLASS_DROW,trained,npc);run(mode,&hybrid);
        /* Hit damage/target position/RNG/messages may reflect the original
         * parent's combat rules. This is one real one_hit, not Drow damage. */
        for(int n=0;n<15;n++) {
            if(n==5||n==6||n==7||n==12)continue;
            if(native.v[n]!=hybrid.v[n])
                fprintf(stderr,"acid difference p=%d trained=%d npc=%d mode=%d field=%d\n",
                    p,trained,npc,mode,n);
            assert(native.v[n]==hybrid.v[n]);
        }
        fixture(CLASS_DROW,parents[p],trained,npc);run(mode,&hybrid);
        assert(!memcmp(&native,&hybrid,sizeof(native)));
        pairs+=2;
    }
    assert(pairs==864);
}

static void fail_closed(void)
{
    for(int mode=0;mode<10;mode++) {
        fixture(CLASS_NINJA,CLASS_DROW,1,1);
        pcs[0].drow[DROW_POWERS]=DROW_ACID; /* stale primary field cannot grant it */
        if(mode==0)pcs[0].hybrid_present=0;
        if(mode==1)pcs[0].hybrid_present=2;
        if(mode==2)pcs[0].player_load_failed=1;
        if(mode==3)pcs[0].hybrid.primary_class=CLASS_MAGE;
        if(mode==4)pcs[0].hybrid.secondary_class=CLASS_MONK;
        if(mode==5)pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=DROW_FIRE;
        if(mode==6)pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=0;
        if(mode==7){actors[0].act=ACT_IS_NPC;actors[0].pcdata=NULL;}
        if(mode==8)actors[0].pcdata=NULL;
        if(mode==9)actors[0].class=CLASS_NONE;
        do_acidbolt(actors,"beta");
        assert(!hits&&!actors[0].wait&&actors[1].hit==50000000);
    }
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_dump(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--boundary")){boundaries(atoi(argv[2]));return 0;}
    if(argc==2&&!strcmp(argv[1],"--secondary-repro")) {
        fixture(CLASS_NINJA,CLASS_DROW,1,1);
        interpret(actors,"acidbolt beta");
        assert(hits==1&&actors[0].wait==8);
        return 0;
    }
    assert(native_dump(0)==UINT64_C(13561486456634868849));
    for(int mode=1;mode<=3;mode++)boundaries(mode);
    paired();fail_closed();
    puts("Acid Bolt: 48 native snapshots, 864 paired commands, 10 refusal and 3 callback/context cases passed.");
    return 0;
}
