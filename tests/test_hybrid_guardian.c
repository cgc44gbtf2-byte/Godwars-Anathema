/* Real command dispatch, warnings, shared toggle and player-file codecs. */
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
static CHAR_DATA actors[2];
static PC_DATA pcs[2];
static DESCRIPTOR_DATA descs[2];
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static char outputs[2][32768];
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
typedef struct snapshot {long v[8];uint64_t text;} SNAPSHOT;

static void fixture(long primary,long secondary,long rank,long focus,long native)
{
    memset(actors,0,sizeof(actors));memset(pcs,0,sizeof(pcs));
    memset(descs,0,sizeof(descs));memset(outputs,0,sizeof(outputs));
    memset(&room,0,sizeof(room));memset(&area,0,sizeof(area));
    room.area=&area;room.name="Guardian fixture";room.description="A private test room.\n\r";
    room.light=1;room.vnum=902;room.people=actors;area.nplayer=2;
    room_index_hash[902%MAX_KEY_HASH]=&room;
    char_list=actors;descriptor_list=descs;current_time=123;world_affects=0;
    first_fight=last_fight=NULL;weather_info.sunlight=SUN_LIGHT;time_info.hour=12;
    for(int n=0;n<2;n++){
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->pcdata=&pcs[n];ch->name=ch->short_descr=n?"beta":"alpha";
        ch->long_descr=ch->contingency="";pcs[n].love=pcs[n].conding=pcs[n].title="";
        ch->class=n?CLASS_NONE:primary;ch->level=LEV_AWA;ch->status=100;
        ch->position=POS_STANDING;ch->hit=ch->max_hit=50000;
        ch->mana=ch->max_mana=50000;ch->in_room=&room;ch->style[0]=STYLE_UNARMED;
        for(int k=0;k<5;k++)ch->stats[k]=3;
        ch->next=ch->next_in_room=n?NULL:&actors[1];ch->desc=&descs[n];
        descs[n].character=ch;descs[n].connected=CON_PLAYING;
        descs[n].outbuf=outputs[n];descs[n].outsize=sizeof(outputs[n]);
        descs[n].next=n?NULL:&descs[1];
    }
    pcs[0].monk[MONK_MANTRA]=primary==CLASS_MONK?rank:8;
    pcs[0].powers[DISC_NECROMANCY]=native;pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=native;
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
    actors[0].more=MORE_BRISTLES;init_mm();
}

static uint64_t fnv(const void *data,size_t len,uint64_t hash)
{
    const unsigned char *p=data;
    while(len--)hash=(hash^*p++)*UINT64_C(1099511628211);
    return hash;
}
enum {MODES=6};
static void run(int mode,int active,SNAPSHOT *s)
{
    if(active)SET_BIT(actors[0].more,MORE_GUARDIAN);
    if(mode==2)actors[0].mana=0;
    if(mode==3)actors[0].mana=LONG_MIN;
    if(mode==4)actors[0].position=POS_SLEEPING;
    if(mode==5)actors[0].position=POS_FIGHTING;
    PC_DATA before=*actors[0].pcdata;long cls=actors[0].class,mana=actors[0].mana;
    char command[64];strcpy(command,mode==1?"spiritguardian ignored":"spiritguardian");
    interpret(actors,command);
    assert(!memcmp(&before,actors[0].pcdata,sizeof(before))&&actors[0].class==cls);
    assert(actors[0].mana==mana&&IS_MORE(actors,MORE_BRISTLES));
    assert(!actors[0].pcdata->familiar&&!actors[0].wizard&&!actors[0].desc->original);
    assert(char_list==actors&&actors[0].next==&actors[1]&&!actors[1].next);
    memset(s,0,sizeof(*s));
    long v[]={actors[0].more,actors[0].hit,actors[0].mana,actors[0].wait,
        actors[0].position,actors[1].more,area.nplayer,number_range(0,1000000)};
    memcpy(s->v,v,sizeof(v));s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<2;n++)s->text=fnv(outputs[n],descs[n].outtop,s->text);
}

static uint64_t native_dump(int print)
{
    const long classes[]={CLASS_MONK,CLASS_VAMPIRE,CLASS_WEREWOLF};
    uint64_t hash=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int c=0;c<3;c++)for(long r=0;r<=8;r++)for(int active=0;active<2;active++){
        if(active&&r<(c?4:6))continue;
        for(int mode=0;mode<MODES;mode++){
            fixture(classes[c],0,r,-1,r);SNAPSHOT s;run(mode,active,&s);
            char line[512];int used=snprintf(line,sizeof(line),"%d,%ld,%d,%d",c,r,active,mode);
            for(int k=0;k<8;k++)used+=snprintf(line+used,sizeof(line)-used,",%ld",s.v[k]);
            used+=snprintf(line+used,sizeof(line)-used,",%016" PRIx64 "\n",s.text);
            assert(used>0&&(size_t)used<sizeof(line));if(print)fputs(line,stdout);
            hash=fnv(line,(size_t)used,hash);rows++;
        }
    }
    assert(rows==240);return hash;
}

static void boundary(int mode)
{
    fixture(CLASS_MONK,0,0,-1,0);
    if(mode==1){do_spiritguardian(NULL,"");return;}
    if(mode==2){actors[0].pcdata=NULL;do_spiritguardian(actors,"");return;}
    SET_BIT(actors[0].more,MORE_GUARDIAN);
    if(mode==4)actors[0].class=CLASS_NONE;
    do_spiritguardian(actors,"");
    assert(!IS_MORE(actors,MORE_GUARDIAN)&&IS_MORE(actors,MORE_BRISTLES));
    assert(actors[0].mana==50000&&strstr(outputs[0],"dismiss"));
}

static void pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)
    for(long focus=-1;focus<=2;focus++)for(long rank=0;rank<=8;rank++)
    for(int active=0;active<2;active++)for(int mode=0;mode<MODES;mode++){
        long effective=focus==MONK_MANTRA?rank:UMIN(rank,4);
        long reference=parents[p]==CLASS_VAMPIRE||parents[p]==CLASS_WEREWOLF?
            parents[p]:CLASS_MONK;
        if(effective>=6)reference=CLASS_MONK;
        fixture(reference,0,effective,-1,0);SNAPSHOT expected,actual;run(mode,active,&expected);
        fixture(parents[p],CLASS_MONK,rank,focus,0);run(mode,active,&actual);
        assert(!memcmp(&expected,&actual,sizeof(actual)));count++;
    }
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)
    for(long rank=0;rank<=8;rank++)for(int active=0;active<2;active++)
    for(int mode=0;mode<MODES;mode++){
        fixture(CLASS_MONK,0,rank,-1,0);SNAPSHOT expected,actual;run(mode,active,&expected);
        fixture(CLASS_MONK,parents[p],rank,-1,0);run(mode,active,&actual);
        assert(!memcmp(&expected,&actual,sizeof(actual)));count++;
    }
    assert(count==4860);
    count=0;
    const long overlap[]={CLASS_VAMPIRE,CLASS_WEREWOLF};
    for(int c=0;c<2;c++)for(long native=0;native<=5;native++)
    for(long focus=-1;focus<=2;focus++)for(long rank=0;rank<=8;rank++)
    for(int active=0;active<2;active++)for(int mode=0;mode<MODES;mode++){
        long effective=focus==MONK_MANTRA?rank:UMIN(rank,4);
        long reference=native>=4||effective<6?overlap[c]:CLASS_MONK;
        fixture(reference,0,effective,-1,native);SNAPSHOT expected,actual;run(mode,active,&expected);
        fixture(overlap[c],CLASS_MONK,rank,focus,native);run(mode,active,&actual);
        assert(!memcmp(&expected,&actual,sizeof(actual)));count++;
    }
    assert(count==5184);
}

static void corruptions(void)
{
    for(int bad=0;bad<10;bad++){
        fixture(CLASS_DROW,CLASS_MONK,8,MONK_MANTRA,0);
        HYBRID_STATE *s=&pcs[0].hybrid;
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:s->hybrid_id=LONG_MAX;break;
        case 4:s->primary_class=CLASS_NINJA;break;
        case 5:s->secondary_class=CLASS_NINJA;break;
        case 6:s->secondary_progression.monk[MONK_MANTRA]=9;break;
        case 7:s->secondary_progression.full_branch=LONG_MAX;break;
        case 8:actors[0].act=ACT_IS_NPC;break;
        case 9:actors[0].pcdata=NULL;break;
        }
        do_spiritguardian(actors,"");assert(!IS_MORE(actors,MORE_GUARDIAN));
        if(bad<8){
            SET_BIT(actors[0].more,MORE_GUARDIAN);
            do_spiritguardian(actors,"");assert(!IS_MORE(actors,MORE_GUARDIAN));
        }
    }
}

static void warnings(void)
{
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++){
        fixture(CLASS_MONK,0,2,-1,0);
        actors[1].class=parents[p];HYBRID_STATE state;
        assert(hybrid_secondary_initialize(&actors[1],
            hybrid_by_parents(parents[p],CLASS_MONK),&state));
        assert(hybrid_secondary_specialize(&state,CLASS_MONK,MONK_MANTRA));
        assert(hybrid_secondary_rank_write(&state,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA,6));
        assert(hybrid_player_attach(&actors[1],&state));
        do_spiritguardian(&actors[1],"");assert(IS_MORE(&actors[1],MORE_GUARDIAN));
        memset(outputs,0,sizeof(outputs));descs[0].outtop=descs[1].outtop=0;
        do_readaura(actors,"beta");
        const char *header="Your guardian spirit sends you a telepathic message:";
        char *first=strstr(outputs[1],header);assert(first&&!strstr(first+strlen(header),header));
        assert(strstr(outputs[1],"Read Aura"));
        assert(!actors[1].pcdata->familiar&&!actors[1].wizard);
    }
}

static void loaders(void)
{
    char root[]="/tmp/anathema-guardian-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);unsigned count=0;
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)
    for(int order=0;order<2;order++)for(long focus=-1;focus<=2;focus++){
        if(order&&focus!=-1)continue;
        for(int brief=0;brief<2;brief++){
            fixture(order?CLASS_MONK:parents[p],order?parents[p]:CLASS_MONK,6,focus,0);
            int eligible=order||focus==MONK_MANTRA;
            DESCRIPTOR_DATA saved;memset(&saved,0,sizeof(saved));
            assert(!load_char_obj(&saved,"Guardfixture"));
            CHAR_DATA *ch=saved.character;ch->desc=NULL;ch->class=actors[0].class;
            ch->level=LEV_AWA;ch->in_room=&room;ch->more=MORE_BRISTLES;
            if(eligible)SET_BIT(ch->more,MORE_GUARDIAN);
            memcpy(ch->pcdata->monk,pcs[0].monk,sizeof(pcs[0].monk));
            assert(hybrid_player_attach(ch,&pcs[0].hybrid));assert(save_char_obj_checked(ch));
            ch->in_room=NULL;free_char(ch);memset(&saved,0,sizeof(saved));
            assert(brief?load_char_short(&saved,"Guardfixture"):load_char_obj(&saved,"Guardfixture"));
            ch=saved.character;assert(!ch->pcdata->player_load_failed);
            assert(!!IS_MORE(ch,MORE_GUARDIAN)==eligible&&IS_MORE(ch,MORE_BRISTLES));
            PC_DATA *own=actors[0].pcdata;actors[0].pcdata=ch->pcdata;actors[0].more=ch->more;
            do_spiritguardian(actors,"");assert(!IS_MORE(actors,MORE_GUARDIAN));
            if(eligible){do_spiritguardian(actors,"");assert(IS_MORE(actors,MORE_GUARDIAN));}
            assert(actors[0].mana==50000&&IS_MORE(actors,MORE_BRISTLES));
            actors[0].pcdata=own;ch->desc=NULL;ch->in_room=NULL;free_char(ch);
            assert(!unlink("../player/Guardfixture"));count++;
        }
    }
    assert(count==90&&!fclose(fpReserve));fpReserve=NULL;
    assert(!chdir("..")&&!rmdir("player")&&!rmdir("backup")&&!rmdir("run"));
    assert(!chdir(cwd)&&!rmdir(root));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_dump(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--boundary")){boundary(atoi(argv[2]));return 0;}
    uint64_t hash=native_dump(0);assert(hash==UINT64_C(0x4caf6723345865ab));
    printf("guardian native hash:%016" PRIx64 "\n",hash);
    for(int n=1;n<=4;n++)boundary(n);
    pairs();corruptions();warnings();loaders();
    puts("Guardian:240 native snapshots,4860 paired commands,5184 overlapping-parent "
         "replays,4 safety boundaries,10 corrupt guards,9 warning consumers,90 full/short loads passed.");
    return 0;
}
