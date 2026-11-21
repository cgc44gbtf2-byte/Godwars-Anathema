/* Real interpreter, world lookup, room transfer and learned-bank access. */
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern void init_mm(void);
extern char *string_space,*top_string;
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
static CHAR_DATA actors[3];
static PC_DATA pcs[2];
static DESCRIPTOR_DATA descs[2];
static ROOM_INDEX_DATA rooms[2];
static AREA_DATA areas[2];
static MOB_INDEX_DATA mob;
static char outputs[2][32768];
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
typedef struct snapshot {long v[12];uint64_t text;} SNAPSHOT;

static void fixture(long primary,long secondary,long rank,long focus)
{
    memset(actors,0,sizeof(actors));memset(pcs,0,sizeof(pcs));
    memset(descs,0,sizeof(descs));memset(rooms,0,sizeof(rooms));
    memset(areas,0,sizeof(areas));memset(&mob,0,sizeof(mob));
    memset(outputs,0,sizeof(outputs));
    for(int n=0;n<2;n++){
        rooms[n].area=&areas[n];rooms[n].vnum=900+n;rooms[n].light=1;
        rooms[n].name=n?"Fixture destination":"Fixture origin";
        rooms[n].description="A private test room.\n\r";
        areas[n].nplayer=1;
        room_index_hash[(900+n)%MAX_KEY_HASH]=&rooms[n];
    }
    char_list=actors;descriptor_list=descs;
    rooms[0].people=actors;rooms[1].people=&actors[1];
    current_time=123;first_fight=last_fight=NULL;world_affects=0;
    weather_info.sunlight=SUN_LIGHT;time_info.hour=12;
    for(int n=0;n<3;n++){
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->name=ch->short_descr=n==0?"alpha":n==1?"beta":"goblin";
        ch->long_descr="";ch->contingency="";
        ch->pcdata=n==2?NULL:&pcs[n];ch->class=n?CLASS_NONE:primary;
        ch->level=LEV_AWA;ch->status=100;ch->position=POS_STANDING;
        ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=50000;
        ch->in_room=&rooms[n?1:0];
        ch->next=n==2?NULL:&actors[n+1];
        ch->next_in_room=n==1?&actors[2]:NULL;
        ch->style[0]=STYLE_UNARMED;for(int i=0;i<5;i++)ch->stats[i]=3;
        if(n==2){ch->act=ACT_IS_NPC;ch->pIndexData=&mob;mob.vnum=9003;}
        else {
            ch->desc=&descs[n];pcs[n].love=pcs[n].conding="";
            pcs[n].title="";
            pcs[n].flag=FLAG_PK;
            descs[n].character=ch;descs[n].connected=CON_PLAYING;
            descs[n].outbuf=outputs[n];descs[n].outsize=sizeof(outputs[n]);
            descs[n].next=n?NULL:&descs[1];
        }
    }
    SET_BIT(actors[1].extra,EXTRA_SUMMON);
    pcs[0].monk[MONK_MANTRA]=primary==CLASS_MONK?rank:8;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_MONK){
            if(focus>=0)assert(hybrid_secondary_specialize(&state,CLASS_MONK,focus));
            assert(hybrid_secondary_rank_write(&state,CLASS_MONK,HYBRID_TRAIN_MONK,
                MONK_MANTRA,focus==MONK_MANTRA?rank:UMIN(rank,4)));
            state.secondary_progression.monk[MONK_MANTRA]=rank;
        }
        assert(hybrid_player_attach(actors,&state));
    }
    init_mm();
}

enum {MODES=20};
static void run(int mode,SNAPSHOT *s)
{
    char command[64]="celestial beta";
    switch(mode){
    case 1:strcpy(command,"celestial goblin");break;
    case 2:strcpy(command,"celestial");break;
    case 3:strcpy(command,"celestial missing");break;
    case 4:strcpy(command,"celestial alpha");break;
    case 5:char_from_room(&actors[1]);char_to_room(&actors[1],&rooms[0]);break;
    case 6:SET_BIT(rooms[1].room_flags,ROOM_PRIVATE);break;
    case 7:SET_BIT(rooms[1].room_flags,ROOM_SOLITARY);break;
    case 8:SET_BIT(rooms[1].room_flags,ROOM_NO_TELEPORT);break;
    case 9:REMOVE_BIT(actors[1].extra,EXTRA_SUMMON);break;
    case 10:actors[1].level=801;break;
    case 11:SET_BIT(rooms[1].room_flags,ROOM_PRIVATE);
        SET_BIT(actors[1].more,MORE_GUARDIAN);break;
    case 12:actors[0].mana=1500;break;
    case 13:actors[0].mana=1501;break;
    case 14:actors[0].mana=LONG_MAX;break;
    case 15:SET_BIT(rooms[0].room_flags,ROOM_NO_TELEPORT);break;
    case 16:actors[0].position=POS_SLEEPING;break;
    case 17:actors[0].position=POS_FIGHTING;break;
    case 18:SET_BIT(rooms[1].room_flags,ROOM_PRIVATE);
        strcpy(command,"celestial goblin");break;
    case 19:SET_BIT(actors[2].more,MORE_GUARDIAN);
        strcpy(command,"celestial goblin");break;
    }
    PC_DATA before=*actors[0].pcdata;long primary=actors[0].class;
    interpret(actors,command);
    assert(!memcmp(&before,actors[0].pcdata,sizeof(before))&&actors[0].class==primary);
    memset(s,0,sizeof(*s));
    long v[]={actors[0].in_room==&rooms[0],actors[0].in_room==&rooms[1],
        actors[0].hit,actors[0].mana,actors[0].wait,actors[0].position,
        actors[1].in_room==&rooms[1],actors[1].hit,actors[1].mana,
        areas[0].nplayer,areas[1].nplayer,number_range(1,1000000)};
    memcpy(s->v,v,sizeof(v));s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<2;n++)for(int i=0;i<descs[n].outtop;i++)
        s->text=(s->text^(unsigned char)outputs[n][i])*UINT64_C(1099511628211);
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(1469598103934665603);
    for(long rank=0;rank<=8;rank++)for(int mode=0;mode<MODES;mode++){
        fixture(CLASS_MONK,0,rank,-1);SNAPSHOT s;run(mode,&s);
        char row[1024];int used=snprintf(row,sizeof(row),"%ld,%d",rank,mode);
        for(int n=0;n<12;n++)used+=snprintf(row+used,sizeof(row)-(size_t)used,",%ld",s.v[n]);
        used+=snprintf(row+used,sizeof(row)-(size_t)used,",%" PRIu64 "\n",s.text);
        assert(used>0&&(size_t)used<sizeof(row));
        if(emit)fputs(row,stdout);
        for(int n=0;n<used;n++)hash=(hash^(unsigned char)row[n])*UINT64_C(1099511628211);
    }
    return hash;
}

static void boundary(int mode)
{
    fixture(CLASS_MONK,0,7,-1);
    if(mode==1||mode==2||mode==3){
        actors[0].mana=mode==1?0:mode==2?1499:LONG_MIN;
        long before=actors[0].mana;do_celestial(actors,"beta");
        assert(actors[0].in_room==&rooms[0]&&actors[0].mana==before&&!actors[0].wait);
        assert(rooms[0].people==actors&&areas[0].nplayer==1&&areas[1].nplayer==1);
    }else if(mode==4){
        actors[0].in_room=NULL;do_celestial(actors,"beta");
        assert(actors[0].in_room==NULL&&actors[0].mana==50000);
    }else if(mode==5){
        actors[0].pcdata=NULL;do_celestial(actors,"beta");
        assert(actors[0].in_room==&rooms[0]&&actors[0].mana==50000);
    }else if(mode==6){
        do_celestial(NULL,"beta");do_celestial(actors,NULL);
        assert(actors[0].in_room==&rooms[0]&&actors[0].mana==50000);
    }else if(mode==7){
        actors[1].in_room=NULL;do_celestial(actors,"beta");
        assert(actors[0].in_room==&rooms[0]&&actors[0].mana==50000);
    }
}

static void pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)
    for(long focus=-1;focus<=2;focus++)for(long rank=0;rank<=8;rank++)
    for(int mode=0;mode<MODES;mode++){
        SNAPSHOT expected,actual;
        long effective=focus==MONK_MANTRA?rank:UMIN(rank,4);
        fixture(CLASS_MONK,0,effective,-1);run(mode,&expected);
        fixture(parents[p],CLASS_MONK,rank,focus);run(mode,&actual);
        assert(!memcmp(&expected,&actual,sizeof(expected)));count++;
    }
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)
    for(long rank=0;rank<=8;rank++)for(int mode=0;mode<MODES;mode++){
        SNAPSHOT expected,actual;
        fixture(CLASS_MONK,0,rank,-1);run(mode,&expected);
        fixture(CLASS_MONK,parents[p],rank,-1);run(mode,&actual);
        assert(!memcmp(&expected,&actual,sizeof(expected)));count++;
    }
    assert(count==8100);
}

static void visibility(void)
{
    fixture(CLASS_MONK,0,7,-1);
    assert(!can_see(NULL,actors)&&!can_see(actors,NULL)&&!can_see(NULL,NULL));
    actors[1].in_room=NULL;assert(!can_see(actors,&actors[1]));
    actors[0].in_room=NULL;assert(!can_see(actors,&actors[1]));
    assert(can_see(actors,actors));
    actors[0].level=LEV_BUI;assert(can_see(actors,&actors[1]));
    actors[0].level=LEV_AWA;SET_BIT(actors[0].itemaffect,ITEMA_VISION);
    assert(can_see(actors,&actors[1]));
}

static void corruptions(void)
{
    for(int n=0;n<10;n++){
        fixture(CLASS_DROW,CLASS_MONK,8,MONK_MANTRA);
        HYBRID_STATE *s=&pcs[0].hybrid;
        switch(n){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].player_load_failed=1;break;
        case 2:s->hybrid_id=LONG_MAX;break;
        case 3:s->primary_class=CLASS_MAGE;break;
        case 4:s->secondary_class=CLASS_NINJA;break;
        case 5:s->secondary_progression.monk[MONK_MANTRA]=9;break;
        case 6:s->secondary_progression.monk[MONK_MANTRA]=-1;break;
        case 7:s->secondary_progression.full_branch=LONG_MAX;break;
        case 8:pcs[0].hybrid_present=2;break;
        case 9:actors[0].act=ACT_IS_NPC;actors[0].pIndexData=&mob;break;
        }
        do_celestial(actors,"beta");
        assert(actors[0].in_room==&rooms[0]&&actors[0].mana==50000);
    }
}

static void loaders(void)
{
    char root[]="/tmp/anathema-celestial-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    unsigned count=0;
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)
    for(int order=0;order<2;order++)for(long focus=-1;focus<=2;focus++){
        if(order&&focus!=-1)continue;
        for(int brief=0;brief<2;brief++){
            fixture(order?CLASS_MONK:parents[p],order?parents[p]:CLASS_MONK,7,focus);
            DESCRIPTOR_DATA saved;memset(&saved,0,sizeof(saved));
            assert(!load_char_obj(&saved,"Pathfixture"));
            CHAR_DATA *ch=saved.character;ch->desc=NULL;ch->class=actors[0].class;
            ch->level=LEV_AWA;ch->in_room=&rooms[0];
            memcpy(ch->pcdata->monk,pcs[0].monk,sizeof(pcs[0].monk));
            ch->pcdata->flag=pcs[0].flag;
            assert(hybrid_player_attach(ch,&pcs[0].hybrid));
            assert(save_char_obj_checked(ch));
            ch->in_room=NULL;free_char(ch);
            memset(&saved,0,sizeof(saved));
            assert(brief?load_char_short(&saved,"Pathfixture"):load_char_obj(&saved,"Pathfixture"));
            ch=saved.character;assert(!ch->pcdata->player_load_failed);
            PC_DATA *own=actors[0].pcdata;actors[0].pcdata=ch->pcdata;
            long effective=hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA);
            assert(effective==(order||focus==MONK_MANTRA?7:4));
            SNAPSHOT result;run(0,&result);
            assert(actors[0].in_room==&rooms[effective>=7?1:0]);
            assert(actors[0].mana==(effective>=7?48500:50000));
            actors[0].pcdata=own;ch->desc=NULL;ch->in_room=NULL;free_char(ch);
            assert(!unlink("../player/Pathfixture"));count++;
        }
    }
    assert(count==90&&fclose(fpReserve)==0);fpReserve=NULL;
    assert(!chdir("..")&&!rmdir("player")&&!rmdir("backup")&&!rmdir("run"));
    assert(!chdir(cwd)&&!rmdir(root));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_dump(1);return 0;}
    if(argc==2&&!strcmp(argv[1],"--visibility")){visibility();return 0;}
    if(argc==3&&!strcmp(argv[1],"--boundary")){boundary(atoi(argv[2]));return 0;}
    uint64_t hash=native_dump(0);
    assert(hash==UINT64_C(0x4e015ab4b8be236a));
    printf("celestial native hash:%016" PRIx64 "\n",hash);
    for(int n=1;n<=7;n++)boundary(n);
    visibility();pairs();corruptions();loaders();
    puts("Celestial:180 native snapshots,8100 paired commands,7 safety boundaries,"
         "10 corrupt access guards,90 full/short loads passed.");
    return 0;
}
