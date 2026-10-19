/* Real interpreter/HP/mana and owned ranks, not mocked power permissions. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern void init_mm(void);
extern char *string_space,*top_string;
static const long parents[]={CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,
    CLASS_DROW,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_DRAGON,CLASS_NINJA,CLASS_MONK};
static CHAR_DATA actors[2];
static PC_DATA pcs[2];
static DESCRIPTOR_DATA descs[2];
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static char output[2][8192];

static void fixture(long parent,int secondary,long rank)
{
    current_time=123;
    memset(pcs,0,sizeof(pcs));memset(descs,0,sizeof(descs));
    memset(output,0,sizeof(output));memset(&room,0,sizeof(room));
    memset(&area,0,sizeof(area));room.area=&area;room.light=1;
    first_fight=last_fight=NULL;descriptor_list=NULL;
    char_list=room.people=actors;
    for(int n=0;n<2;++n) {
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->name=ch->short_descr=n?"beta":"alpha";ch->contingency="";
        ch->pcdata=&pcs[n];ch->desc=&descs[n];
        ch->class=n?CLASS_SORCERER:parent;ch->level=LEV_AWA;ch->status=100;
        ch->position=POS_STANDING;ch->hit=100;ch->max_hit=10000;
        ch->mana=ch->max_mana=10000;ch->in_room=&room;
        ch->next=ch->next_in_room=n?NULL:&actors[1];
        for(int i=0;i<5;++i)ch->stats[i]=3;
        pcs[n].love=pcs[n].conding="";pcs[n].flag=FLAG_PK;
        descs[n].character=ch;descs[n].connected=CON_PLAYING;
        descs[n].outbuf=output[n];descs[n].outsize=sizeof(output[n]);
    }
    if(secondary) {
        HYBRID_STATE s;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(parent,CLASS_ELEM),&s));
        assert(hybrid_secondary_rank_write(&s,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,ELEM_SUN,rank));
        assert(hybrid_player_attach(actors,&s));
    } else pcs[0].powers[ELEM_SUN]=rank;
}

static void replay(long *values,int mode)
{
    switch(mode) {
    case 1:actors[0].fighting=&actors[1];break;
    case 2:actors[0].hit=actors[0].max_hit-1;break;
    case 3:actors[0].mana=800;break;
    case 4:actors[0].mana=799;break;
    case 5:actors[0].hit=actors[0].max_hit;break;
    }
    init_mm();
    interpret(actors,mode==6?"sunbath beta":mode==7?"sunbath missing":"sunbath");
    values[0]=actors[0].hit;values[1]=actors[1].hit;
    values[2]=actors[0].mana;values[3]=actors[0].wait;
    values[4]=actors[0].position;values[5]=actors[1].position;
    values[6]=number_range(1,100000);
}

static void native_dump(void)
{
    for(int mode=0;mode<8;++mode) {
        fixture(CLASS_ELEM,0,3);long values[7];replay(values,mode);
        for(int j=0;j<7;++j)printf("%ld%c",values[j],j==6?'\n':',');
    }
}

/* Recorded on bdd8224's unmodified Sunbath, before arithmetic fixes/adapters. */
static void native_snapshots(void)
{
    static const long baseline[8][7]={
        {1247,100,9200,18,9,9,47208},{1247,100,9200,18,9,9,47208},
        {10000,100,9200,18,9,9,47208},{1247,100,0,18,9,9,47208},
        {100,100,799,0,9,9,47251},{10000,100,9200,18,9,9,47208},
        {100,1247,9200,18,9,9,47208},{100,100,10000,0,9,9,47251}
    };
    for(int mode=0;mode<8;++mode) {
        fixture(CLASS_ELEM,0,3);long actual[7];replay(actual,mode);
        if(memcmp(actual,baseline[mode],sizeof(actual))) {
            fprintf(stderr,"native Sunbath snapshot mode %d:",mode);
            for(int j=0;j<7;++j)fprintf(stderr," %ld/%ld",actual[j],baseline[mode][j]);
            fputc('\n',stderr);
            fprintf(stderr,"Sunbath output: %.*s\n",(int)descs[0].outtop,output[0]);
        }
        assert(!memcmp(actual,baseline[mode],sizeof(actual)));
    }
}

static void overflow(void)
{
    fixture(CLASS_ELEM,0,3);
    actors[0].hit=LONG_MAX-1;actors[0].max_hit=LONG_MAX;
    interpret(actors,"sunbath self");
    assert(actors[0].hit==LONG_MAX&&actors[0].mana==9200);
}

static void malformed_rank(void)
{
    fixture(CLASS_ELEM,0,11);actors[0].fighting=&actors[1];
    interpret(actors,"sunbath self");
    assert(actors[0].hit>=1101&&actors[0].hit<=1600);
    assert(actors[0].mana==9200&&actors[0].wait==18);
}

static void secondary_cases(void)
{
    for(size_t p=0;p<9;++p)for(int mode=0;mode<8;++mode) {
        long native[7],hybrid[7];
        fixture(CLASS_ELEM,0,3);replay(native,mode);
        fixture(parents[p],1,3);
        /* Stale original fields never supply the secondary's rank. */
        pcs[0].powers[ELEM_SUN]=11;
        HYBRID_STATE bank=pcs[0].hybrid;
        replay(hybrid,mode);
        assert(!memcmp(native,hybrid,sizeof(native)));
        assert(!memcmp(&bank,&pcs[0].hybrid,sizeof(bank)));
        assert(actors[0].class==parents[p]&&pcs[0].powers[ELEM_SUN]==11);
        int accepted=mode!=4&&mode!=7;
        if(actors[0].mana!=(mode==3?0:mode==4?799:accepted?9200:10000))
            fprintf(stderr,"Sunbath mana p=%zu mode=%d mana=%ld wait=%ld output=%.*s\n",
                p,mode,actors[0].mana,actors[0].wait,(int)descs[0].outtop,output[0]);
        assert(actors[0].mana==(mode==3?0:mode==4?799:accepted?9200:10000));
        assert(actors[0].wait==(accepted?18:0));
    }
    for(size_t p=0;p<9;++p)for(long rank=0;rank<3;++rank) {
        fixture(parents[p],1,rank);pcs[0].powers[ELEM_SUN]=10;
        HYBRID_STATE bank=pcs[0].hybrid;
        interpret(actors,"sunbath beta");
        assert(actors[0].mana==10000&&!actors[0].wait&&actors[1].hit==100);
        assert(!memcmp(&bank,&pcs[0].hybrid,sizeof(bank)));
    }
    for(size_t p=0;p<9;++p) {
        long native[7],hybrid[7];
        fixture(CLASS_ELEM,0,10);replay(native,6);
        fixture(parents[p],1,3);
        HYBRID_STATE s=pcs[0].hybrid;
        assert(hybrid_secondary_specialize(&s,CLASS_ELEM,ELEM_SUN));
        for(size_t i=ELEM_FIRE;i<=ELEM_INFIN;++i) {
            long cap;
            assert(hybrid_parent_training_cap(actors,&s,CLASS_ELEM,
                HYBRID_TRAIN_ELEMENT,i,&cap));
            assert(hybrid_secondary_rank_write(&s,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,i,cap));
        }
        assert(hybrid_secondary_element_focus(&s,ELEM_SUN));
        pcs[0].hybrid=s; /* Test fixture: legitimate fully focused training. */
        assert(hybrid_power_rank(actors,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,ELEM_SUN)==10);
        replay(hybrid,6);
        assert(!memcmp(native,hybrid,sizeof(native)));
        assert(!memcmp(&s,&pcs[0].hybrid,sizeof(s)));
        actors[1].hit=LONG_MAX-1;actors[1].max_hit=LONG_MAX;
        actors[0].wait=0;
        interpret(actors,"sunbath beta");
        assert(actors[1].hit==LONG_MAX&&actors[0].mana==8400&&actors[0].wait==18);
        assert(!memcmp(&s,&pcs[0].hybrid,sizeof(s)));
    }
    for(int bad=0;bad<6;++bad) {
        fixture(CLASS_NINJA,bad!=0&&bad!=5,3);
        if(bad==5) {
            HYBRID_STATE s;
            assert(hybrid_secondary_initialize(actors,
                hybrid_by_parents(CLASS_NINJA,CLASS_MONK),&s));
            assert(hybrid_player_attach(actors,&s));
        }
        pcs[0].powers[ELEM_SUN]=10;
        if(bad==1)pcs[0].hybrid.primary_class=CLASS_MAGE;
        if(bad==2)pcs[0].player_load_failed=1;
        if(bad==3)pcs[0].hybrid.hybrid_id=999;
        if(bad==4) {SET_BIT(actors[0].act,ACT_IS_NPC);actors[0].pcdata=NULL;}
        if(bad==4)do_sunbath(actors,"beta");else interpret(actors,"sunbath beta");
        assert(actors[0].mana==10000&&!actors[0].wait&&actors[1].hit==100);
    }
}

int main(int argc,char **argv)
{
    string_space=malloc(1024*1024);assert(string_space);top_string=string_space;
    char empty[8]="stale";
    assert(number_argument("",empty)==0&&empty[0]=='\0');
    if(argc>1&&!strcmp(argv[1],"--native")) {native_dump();return 0;}
    if(argc>1&&!strcmp(argv[1],"--overflow")) {overflow();return 0;}
    if(argc>1&&!strcmp(argv[1],"--rank")) {malformed_rank();return 0;}
    if(argc>1&&!strcmp(argv[1],"--secondary")) {secondary_cases();return 0;}
    if(argc>1&&!strcmp(argv[1],"--native-check")) {
        overflow();malformed_rank();native_snapshots();return 0;
    }
    overflow();malformed_rank();native_snapshots();secondary_cases();
    puts("hybrid healing: 81 native/secondary Sunbath replays; mana,lag,ownership,focused training,RNG and numeric boundaries passed");
    return 0;
}
