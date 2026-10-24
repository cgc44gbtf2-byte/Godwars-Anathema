/* Read Aura: real dispatch, original grants, independent learned Monk grant. */
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
static PC_DATA pcs[3];
static DESCRIPTOR_DATA descs[3];
static char outputs[3][32768];
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static OBJ_DATA relic;
static OBJ_INDEX_DATA prototype;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
static const long mana_values[]={0,499,500};
typedef struct snapshot {long values[12];uint64_t text;} SNAPSHOT;

static void fixture(long primary,long secondary,long rank,int focus,int native)
{
    memset(actors,0,sizeof(actors));memset(pcs,0,sizeof(pcs));
    memset(descs,0,sizeof(descs));memset(outputs,0,sizeof(outputs));
    memset(&room,0,sizeof(room));memset(&area,0,sizeof(area));
    memset(&relic,0,sizeof(relic));memset(&prototype,0,sizeof(prototype));
    room.vnum=901;room.light=1;room.area=&area;area.nplayer=3;
    room.name="Private aura fixture";room.description="A private room.\n\r";room.people=actors;
    room_index_hash[901%MAX_KEY_HASH]=&room;
    char_list=actors;descriptor_list=descs;world_affects=0;current_time=123;
    weather_info.sunlight=SUN_LIGHT;time_info.hour=12;first_fight=last_fight=NULL;
    for(int n=0;n<3;n++){
        CHAR_DATA *ch=&actors[n];clear_char(ch);ch->pcdata=&pcs[n];
        ch->name=ch->short_descr=n==0?"alpha":n==1?"beta":"gamma";
        ch->long_descr=ch->morph=ch->contingency="";
        pcs[n].love=pcs[n].conding=pcs[n].title="";
        ch->class=n?CLASS_NONE:primary;ch->level=LEV_AWA;ch->status=100;
        ch->position=POS_STANDING;ch->hit=ch->max_hit=50000;
        ch->mana=ch->max_mana=50000;ch->vampgen=4;ch->in_room=&room;
        ch->primal=1000;pcs[n].quest=50000;
        ch->style[0]=STYLE_UNARMED;for(int k=0;k<5;k++)ch->stats[k]=3;
        ch->next=ch->next_in_room=n<2?&actors[n+1]:NULL;
        ch->desc=&descs[n];descs[n].character=ch;descs[n].connected=CON_PLAYING;
        descs[n].outbuf=outputs[n];descs[n].outsize=sizeof(outputs[n]);
        descs[n].next=n<2?&descs[n+1]:NULL;
    }
    pcs[0].monk[MONK_MANTRA]=primary==CLASS_MONK?rank:8; /* Decoy must not grant. */
    if(native==1||native==3){
        pcs[0].powers[DISC_AUSPEX]=2;pcs[0].tribes[TRIBE_GLASS_WALKERS]=2;
        pcs[0].ninja[NINJA_SORA]=4;actors[0].spheres[MLIF]=1;
        SET_BIT(pcs[0].drow[DROW_POWERS],DROW_READ);
        SET_BIT(pcs[0].dragskills,DRAG_READAURA);
    }
    if(native==2)SET_BIT(actors[0].itemaffect,ITEMA_VISION);
    if(native==3)pcs[0].breed[BREED_HOMID]=5;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        if(secondary==CLASS_MONK){
            if(focus>=0)assert(hybrid_secondary_specialize(&state,CLASS_MONK,focus));
            state.secondary_progression.monk[MONK_MANTRA]=rank;
        }
        assert(hybrid_player_attach(actors,&state));
    }
    prototype.vnum=901;prototype.name="relic";prototype.short_descr="a test relic";
    relic.pIndexData=&prototype;relic.name="relic";relic.short_descr="a test relic";
    relic.item_type=ITEM_TREASURE;relic.wear_flags=ITEM_TAKE;relic.wear_loc=WEAR_NONE;
    relic.weight=1;relic.cost=25;relic.level=10;relic.carried_by=actors;
    actors[0].carrying=&relic;init_mm();
}

static uint64_t fnv(const void *data,size_t n,uint64_t hash)
{
    const unsigned char *p=data;
    while(n--)hash=(hash^*p++)*UINT64_C(1099511628211);
    return hash;
}

static void run(CHAR_DATA *ch,int mode,SNAPSHOT *s)
{
    const char *commands[]={"readaura beta","readaura relic","readaura self",
        "readaura missing","readaura","readaura beta","readaura beta","readaura beta"};
    if(mode==5)SET_BIT(actors[1].act,ACT_IS_NPC);
    if(mode==6){
        SET_BIT(actors[1].extra,EXTRA_FAKE_CON);
        pcs[1].fake_hit=11;pcs[1].fake_dam=12;pcs[1].fake_ac=13;
        pcs[1].fake_hp=14;pcs[1].fake_max_hp=15;pcs[1].fake_mana=16;pcs[1].fake_max_mana=17;
    }
    if(mode==7){
        actors[1].class=CLASS_MONK;pcs[1].monk[MONK_MANTRA]=6;
        SET_BIT(actors[1].more,MORE_GUARDIAN);
    }
    PC_DATA before=*ch->pcdata;long cls=ch->class,primal=ch->primal;
    ROOM_INDEX_DATA *where=ch->in_room;long af=ch->affected_by,va=ch->vampaff;
    char command[64];strcpy(command,mode==2?"readaura alpha":commands[mode]);
    interpret(ch,command);
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(ch->class==cls&&ch->primal==primal&&ch->in_room==where);
    assert(ch->affected_by==af&&ch->vampaff==va&&ch->carrying==&relic);
    assert(!pcs[1].familiar&&!actors[1].wizard&&area.nplayer==3);
    memset(s,0,sizeof(*s));
    long values[]={ch->mana,ch->hit,ch->wait,ch->position,ch->more,ch->gold,
        ch->extra,ch->in_room?ch->in_room->vnum:0,actors[1].hit,actors[1].mana,
        area.nplayer,number_range(0,1000000)};
    memcpy(s->values,values,sizeof(values));s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<3;n++)s->text=fnv(outputs[n],descs[n].outtop,s->text);
}

static unsigned native_cases(int dump)
{
    const long classes[]={CLASS_MONK,CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
        CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
    unsigned count=0;
    for(unsigned p=0;p<10;p++)for(int native=0;native<4;native++)
    for(int mode=0;mode<8;mode++)for(int m=0;m<3;m++){
        fixture(classes[p],0,native==1||native==3?2:0,-1,native);
        actors[0].mana=mana_values[m];SNAPSHOT s;run(actors,mode,&s);count++;
        if(dump){
            printf("%u,%d,%d,%d",p,native,mode,m);
            for(unsigned n=0;n<12;n++)printf(",%ld",s.values[n]);
            printf(",%016" PRIx64 "\n",s.text);
        }
    }
    assert(count==960);return count;
}

static void paired(void)
{
    unsigned pairs=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int focus=-1;focus<3;focus++)for(long rank=0;rank<=8;rank++)
    for(int mode=0;mode<8;mode++)for(int m=0;m<3;m++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        fixture(primary,secondary,rank,focus,0);actors[0].mana=mana_values[m];
        long effective=hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA);
        assert(effective==UMIN(rank,order||focus==MONK_MANTRA?8:4));
        /* A self-read reports genuinely different original-class combat
         * stats. Use real fake-condition handling for paired self views;
         * the native matrix separately pins unmasked self statistics. */
        if(mode==2)SET_BIT(actors[0].extra,EXTRA_FAKE_CON);
        SNAPSHOT actual,expected;run(actors,mode,&actual);
        fixture(effective>=2?CLASS_MONK:primary,0,effective,-1,0);
        if(mode==2)SET_BIT(actors[0].extra,EXTRA_FAKE_CON);
        actors[0].mana=mana_values[m];run(actors,mode,&expected);
        assert(!memcmp(&actual,&expected,sizeof(actual)));pairs++;
    }
    assert(pairs==15552);
}

static void invalid_records_and_overlaps(void)
{
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int bad=0;bad<5;bad++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        fixture(primary,secondary,2,MONK_MANTRA,0);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.secondary_progression.monk[MONK_MANTRA]=LONG_MAX;break;
        case 4:pcs[0].hybrid.hybrid_id=0;break;
        }
        SNAPSHOT actual,expected;run(actors,0,&actual);
        fixture(primary,0,2,-1,0);run(actors,0,&expected);
        assert(!memcmp(&actual,&expected,sizeof(actual)));
    }
    /* A secondary grant supplements—not replaces—native class access. */
    for(unsigned p=0;p<9;p++){
        fixture(parents[p],CLASS_MONK,0,MONK_TECH,1);
        SNAPSHOT actual,expected;run(actors,0,&actual);
        fixture(parents[p],0,0,-1,1);run(actors,0,&expected);
        assert(!memcmp(&actual,&expected,sizeof(actual)));
    }
    /* A learned Monk source is free even for Mage originals, and permits
     * aura/object reading despite Werewolf-specific native restrictions. */
    fixture(CLASS_MAGE,CLASS_MONK,2,MONK_ART,0);actors[0].mana=LONG_MIN;
    SNAPSHOT s;run(actors,0,&s);
    assert(strstr(outputs[0],"Hp:")&&actors[0].mana==LONG_MIN);
    fixture(CLASS_WEREWOLF,CLASS_MONK,2,MONK_TECH,3);run(actors,1,&s);
    assert(strstr(outputs[0],"Object 'relic'"));
    fixture(CLASS_WEREWOLF,CLASS_MONK,2,MONK_TECH,3);run(actors,7,&s);
    assert(strstr(outputs[0],"Hp:")&&!strstr(outputs[1],"Read Aura"));
}

static void safety(int which)
{
    fixture(CLASS_MONK,0,2,-1,0);
    if(which==0){actors[0].pcdata=NULL;do_readaura(actors,"beta");return;}
    if(which==1){actors[0].in_room=NULL;do_readaura(actors,"beta");return;}
    if(which==2){do_readaura(NULL,"beta");return;}
    char long_label[2*MAX_STRING_LENGTH+1];
    memset(long_label,'x',sizeof(long_label)-1);long_label[sizeof(long_label)-1]='\0';
    actors[1].short_descr=which==3?NULL:long_label;
    if(which==4)long_label[2*MAX_INPUT_LENGTH]='\0';
    SNAPSHOT s;run(actors,5,&s);
    assert(strstr(outputs[0]," is an NPC.")&&strstr(outputs[2],"someone"));
}

static void loaders(void)
{
    char cwd[4096],root[]="/tmp/anathema-aura-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)for(int full=0;full<2;full++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        fixture(primary,secondary,2,full?MONK_MANTRA:MONK_TECH,0);
        HYBRID_STATE state=pcs[0].hybrid;DESCRIPTOR_DATA d={0};
        assert(!load_char_obj(&d,"Aurafixture"));CHAR_DATA *ch=d.character;
        ch->class=primary;ch->level=LEV_AWA;ch->position=POS_STANDING;
        ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=50000;
        ch->primal=1000;ch->pcdata->quest=50000;ch->vampgen=4;ch->in_room=&room;
        ch->pcdata->monk[MONK_MANTRA]=order?2:8;
        assert(hybrid_player_attach(ch,&state)&&save_char_obj_checked(ch));
        ch->desc=NULL;ch->in_room=NULL;free_char(ch);
        for(int brief=0;brief<2;brief++){
            fixture(primary,secondary,2,full?MONK_MANTRA:MONK_TECH,0);
            memset(&d,0,sizeof(d));
            assert(brief?load_char_short(&d,"Aurafixture"):load_char_obj(&d,"Aurafixture"));
            ch=d.character;assert(!ch->pcdata->player_load_failed&&ch->class==primary);
            assert(hybrid_power_rank(ch,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA)==2);
            ch->in_room=&room;ch->next=ch->next_in_room=&actors[1];
            room.people=char_list=ch;ch->desc=&descs[0];descs[0].character=ch;
            ch->carrying=&relic;relic.carried_by=ch;
            long mana=ch->mana;SNAPSHOT s;run(ch,0,&s);
            assert(strstr(outputs[0],"Hp:")&&ch->mana==mana);
            ch->carrying=NULL;ch->next=ch->next_in_room=NULL;
            ch->desc=NULL;ch->in_room=NULL;free_char(ch);count++;
            char_list=NULL;room.people=NULL;descriptor_list=NULL;
        }
        unlink("../player/Aurafixture");unlink("../backup/Aurafixture");
    }
    assert(count==72&&!fclose(fpReserve)&&!chdir(".."));
    assert(!rmdir("player")&&!rmdir("backup")&&!rmdir("run")&&!chdir(cwd)&&!rmdir(root));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_cases(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--safety")){safety(atoi(argv[2]));return 0;}
    native_cases(0);paired();invalid_records_and_overlaps();for(int n=0;n<6;n++)safety(n);
    fixture(CLASS_MONK,0,2,-1,0);do_readaura(actors,NULL);
    assert(strstr(outputs[0],"Read the aura on what?"));
    loaders();
    puts("Read Aura:960 native snapshots,15552 parent/focus/rank/target/mana pairs,"
         "90 rejected-record/original-access cases,12 independent overlaps,"
         "72 full/short loads and6 safety boundaries passed.");
    return 0;
}
