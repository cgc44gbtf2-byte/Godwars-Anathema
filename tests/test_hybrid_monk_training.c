/* Real interpreter, paid lesson transactions and both player-file loaders. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/monk_training.h"
#include "classes/hybrid/player.h"
extern char *string_space,*top_string;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static char output[32768];
static const char *const names[]={"tech","mantra","art"};
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
static unsigned paid_lessons,confirmed_focuses,reloads;

static CHAR_DATA *fresh(long primary,long secondary)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentmonkfixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);ch->name=str_dup("Monkfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;
    shell.outbuf=output;shell.outsize=sizeof(output);
    ch->level=LEV_AWA;ch->position=POS_STANDING;ch->class=primary;
    ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=51000;
    ch->primal=1000;ch->exp=123456;ch->pcdata->quest=50000;
    for(int i=0;i<4;i++)ch->pcdata->monk[i]=5; /* Never read as secondary ranks. */
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=4;
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
    assert(hybrid_player_attach(ch,&state));
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;return ch;
}

static void release(CHAR_DATA *ch)
{
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;
    ch->next=ch->next_in_room=NULL;ch->desc=NULL;ch->in_room=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Monkfixture");unlink("../backup/Monkfixture");
}

static void command(CHAR_DATA *ch,const char *argument)
{
    char line[512];assert(strlen(argument)<sizeof(line)-10);
    snprintf(line,sizeof(line),"selfbrid %s",argument);
    shell.outtop=0;output[0]='\0';interpret(ch,line);
}

static void unchanged(CHAR_DATA *ch,const char *argument)
{
    PC_DATA before=*ch->pcdata;long primal=ch->primal,experience=ch->exp;
    long parent=ch->class,hit=ch->hit,mana=ch->mana;
    command(ch,argument);
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(ch->primal==primal&&ch->exp==experience&&ch->class==parent);
    assert(ch->hit==hit&&ch->mana==mana);
}

static void reload(CHAR_DATA *ch)
{
    for(int brief=0;brief<2;brief++){
        DESCRIPTOR_DATA d={0};
        assert(brief?load_char_short(&d,"Monkfixture"):load_char_obj(&d,"Monkfixture"));
        CHAR_DATA *loaded=d.character;assert(!loaded->pcdata->player_load_failed);
        assert(loaded->class==ch->class&&loaded->primal==ch->primal);
        assert(loaded->pcdata->quest==50000&&loaded->exp==ch->exp);
        assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
        for(int i=0;i<4;i++)assert(loaded->pcdata->monk[i]==ch->pcdata->monk[i]);
        for(int i=0;i<6;i++)assert(loaded->pcdata->colors[i]==4);
        loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);reloads++;
    }
}

static void all_paths(void)
{
    for(unsigned p=0;p<9;p++)for(int focus=0;focus<3;focus++){
        CHAR_DATA *ch=fresh(parents[p],CLASS_MONK);char arg[96];
        unchanged(ch,"monk");assert(strstr(output,"not selected"));
        assert(strstr(output,"mantra: 0/4 (stored 0)"));
        unchanged(ch,"monk mantra gain");assert(strstr(output,"Choose and confirm"));
        unchanged(ch,"monk mantra");assert(strstr(output,"next rank costs 10"));
        snprintf(arg,sizeof(arg),"monk focus %s",names[focus]);unchanged(ch,arg);
        assert(strstr(output,"Repeat with confirm")&&access("../player/Monkfixture",F_OK)!=0);
        strcat(arg," confirm");command(ch,arg);
        assert(strstr(output,"completed and saved")&&ch->primal==1000);
        assert(ch->pcdata->hybrid.secondary_progression.full_branch==focus);
        confirmed_focuses++;reload(ch);
        unchanged(ch,arg);assert(strstr(output,"already chosen"));
        for(int path=0;path<3;path++){
            int cap=path==focus?8:4;
            snprintf(arg,sizeof(arg),"monk %s gain",names[path]);
            for(int n=1;n<=cap;n++){
                long before=ch->primal;command(ch,arg);
                assert(strstr(output,"completed and saved"));
                assert(hybrid_power_rank(ch,CLASS_MONK,HYBRID_TRAIN_MONK,path)==n);
                assert(ch->primal==before-10*n);paid_lessons++;reload(ch);
                for(int i=0;i<4;i++)assert(ch->pcdata->monk[i]==5);
            }
            unchanged(ch,arg);assert(strstr(output,"already at"));
        }
        assert(ch->primal==440&&ch->pcdata->quest==50000);
        assert(!IS_MORE(ch,MORE_GUARDIAN)); /* Training is not automatic activation. */
        shell.outtop=0;output[0]='\0';char guardian[]="spiritguardian";
        interpret(ch,guardian);assert(!!IS_MORE(ch,MORE_GUARDIAN)==(focus==MONK_MANTRA));
        release(ch);
    }
    assert(confirmed_focuses==27&&paid_lessons==432&&reloads==918);
}

static void guards_and_faults(void)
{
    CHAR_DATA *ch=fresh(CLASS_DROW,CLASS_MONK);
    const char *bad[]={"monk nonsense","monk focus","monk focus nonsense confirm",
        "monk focus mantra maybe","monk focus mantra confirm trailing","monk tech gain 2",
        "monk tech gain confirm","monk focus aaaaaaaaaaaaaaaa confirm",
        "monk mantra gain aaaaaaaaaaaaaaaa","monk tech 1","monks focus tech confirm"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)unchanged(ch,bad[i]);
    assert(!hybrid_monk_training_command(ch,NULL));
    assert(!hybrid_monk_training_command(ch,"monks"));
    assert(hybrid_monk_training_command(NULL,"monk"));
    ch->fighting=ch;unchanged(ch,"monk focus mantra confirm");ch->fighting=NULL;
    ch->position=POS_SLEEPING;unchanged(ch,"monk focus mantra confirm");ch->position=POS_STANDING;
    shell.original=ch;unchanged(ch,"monk focus mantra confirm");shell.original=NULL;
    ch->desc=NULL;unchanged(ch,"monk focus mantra confirm");ch->desc=&shell;
    ch->save_pending|=PLAYER_SAVE_PRIMARY;unchanged(ch,"monk focus mantra confirm");
    ch->save_pending=0;
    atomic_save_test_fault=SAVE_FAULT_RENAME;
    unchanged(ch,"monk focus mantra confirm");assert(strstr(output,"No lesson or payment"));
    atomic_save_test_fault=SAVE_FAULT_NONE;
    unchanged(ch,"monk focus mantra confirm");assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));reload(ch);
    command(ch,"MONK FOCUS MaNtRa CoNfIrM");assert(strstr(output,"completed and saved"));
    ch->primal=9;unchanged(ch,"monk mantra gain");assert(strstr(output,"Not enough primal"));
    ch->primal=1000;
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(unsigned i=0;i<sizeof(faults)/sizeof(*faults);i++){
        atomic_save_test_fault=faults[i];unchanged(ch,"monk mantra gain");
        assert(strstr(output,"No lesson or payment"));atomic_save_test_fault=SAVE_FAULT_NONE;
        unchanged(ch,"monk mantra gain");assert(strstr(output,"recovery is pending"));
        assert(save_char_obj_checked(ch));reload(ch);
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;command(ch,"monk mantra gain");
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat")&&ch->primal==990);
    assert(hybrid_power_rank(ch,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA)==1);
    unchanged(ch,"monk mantra gain");assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));command(ch,"monk mantra gain");
    assert(ch->primal==970);reload(ch);
    ch->pcdata->hybrid.secondary_progression.monk[MONK_TECH]=5;
    unchanged(ch,"monk tech gain");assert(strstr(output,"staff review"));
    unchanged(ch,"monk");assert(strstr(output,"tech: 4/4 (stored 5)"));
    assert(ch->pcdata->hybrid.secondary_progression.monk[MONK_TECH]==5);
    ch->pcdata->hybrid.secondary_progression.monk[MONK_TECH]=0;
    ch->pcdata->hybrid_present=0;unchanged(ch,"monk mantra gain");
    ch->pcdata->hybrid_present=2;unchanged(ch,"monk mantra gain");
    ch->pcdata->hybrid_present=1;ch->pcdata->player_load_failed=1;
    unchanged(ch,"monk mantra gain");ch->pcdata->player_load_failed=0;
    release(ch);
}

static void original_monk(void)
{
    for(unsigned p=0;p<9;p++){
        CHAR_DATA *ch=fresh(CLASS_MONK,parents[p]);
        unchanged(ch,"monk focus tech confirm");assert(strstr(output,"native monklearn"));
        unchanged(ch,"monk mantra gain");assert(strstr(output,"native monklearn"));
        HYBRID_STATE before=ch->pcdata->hybrid;
        for(int i=0;i<3;i++){
            char arg[64];snprintf(arg,sizeof(arg),"monklearn %s gain",names[i]);
            interpret(ch,arg);assert(ch->pcdata->monk[i]==6);
        }
        assert(ch->primal==820&&!memcmp(&before,&ch->pcdata->hybrid,sizeof(before)));
        assert(access("../player/Monkfixture",F_OK)!=0);release(ch);
    }
}

int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-monk-ui-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    current_time=123456;init_mm();fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    all_paths();guards_and_faults();original_monk();
    assert(!char_list&&!room.people&&!fclose(fpReserve)&&!chdir(".."));
    assert(!rmdir("player")&&!rmdir("backup")&&!rmdir("run")&&!chdir(cwd)&&!rmdir(root));
    puts("Secondary Monk UI:27 focuses,432 paid lessons,918 path loads,9 original-parent "
         "paths,parser/context/cap guards,atomic failures and recovery passed.");
    return 0;
}
