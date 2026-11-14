/* Actual interpreter/controller, paid lesson backend and atomic player files. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/hybrid/books.h"
#include "classes/hybrid/player.h"

extern char *string_space,*top_string;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static char output[32768];
static const char *names[]={"purple","red","blue","green","yellow","black"};

static CHAR_DATA *fresh(long primary)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentcolorfixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);ch->name=str_dup("Colorfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;
    shell.outbuf=output;shell.outsize=sizeof(output);
    ch->level=LEV_SOR;ch->position=POS_STANDING;ch->class=primary;
    ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=51000;
    ch->primal=1000;ch->exp=123456;ch->pcdata->quest=50000;
    ch->vampgen=4;
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=4;
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,CLASS_SORCERER),&state));
    assert(hybrid_player_attach(ch,&state));
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;
    return ch;
}

static void release(CHAR_DATA *ch)
{
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;
    ch->next=ch->next_in_room=NULL;ch->desc=NULL;ch->in_room=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Colorfixture");unlink("../backup/Colorfixture");
}

static void command(CHAR_DATA *ch,const char *argument)
{
    char line[256];snprintf(line,sizeof(line),"selfbrid %s",argument);
    shell.outtop=0;output[0]='\0';
    interpret(ch,line);
}

static void reload(CHAR_DATA *ch)
{
    for(int short_load=0;short_load<2;short_load++) {
        DESCRIPTOR_DATA d={0};
        assert(short_load?load_char_short(&d,"Colorfixture"):load_char_obj(&d,"Colorfixture"));
        CHAR_DATA *loaded=d.character;
        assert(!loaded->pcdata->player_load_failed&&loaded->class==ch->class);
        assert(loaded->primal==ch->primal&&loaded->pcdata->quest==50000);
        assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
        for(int i=0;i<6;i++)assert(loaded->pcdata->colors[i]==4);
        loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);
    }
}

static void unchanged(CHAR_DATA *ch,const char *arg)
{
    HYBRID_STATE state=ch->pcdata->hybrid;
    long primal=ch->primal,quest=ch->pcdata->quest,experience=ch->exp,parent=ch->class;
    command(ch,arg);
    assert(!memcmp(&state,&ch->pcdata->hybrid,sizeof(state)));
    assert(ch->primal==primal&&ch->pcdata->quest==quest&&ch->exp==experience&&ch->class==parent);
}

static void paths(void)
{
    const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
        CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_VAMPIRE,CLASS_WEREWOLF};
    for(size_t p=0;p<9;p++)for(int focus=0;focus<6;focus++) {
        CHAR_DATA *ch=fresh(parents[p]);char arg[96];
        unchanged(ch,"colors");assert(strstr(output,"not selected"));
        unchanged(ch,"colors blue");assert(strstr(output,"Choose and confirm"));
        snprintf(arg,sizeof(arg),"colors focus %s",names[focus]);unchanged(ch,arg);
        assert(strstr(output,"Repeat with confirm")&&access("../player/Colorfixture",F_OK)!=0);
        strcat(arg," confirm");command(ch,arg);
        assert(strstr(output,"completed and saved"));
        assert(ch->pcdata->hybrid.secondary_progression.full_branch==focus&&ch->primal==1000);
        unchanged(ch,arg);assert(strstr(output,"already chosen"));
        for(int color=0;color<6;color++) {
            int cap=color==focus?5:3;
            snprintf(arg,sizeof(arg),"colors %s",names[color]);
            for(int n=1;n<=cap;n++) {
                long old=ch->primal;command(ch,arg);
                assert(strstr(output,"completed and saved"));
                assert(hybrid_book_color_rank(ch,color)==n&&ch->primal==old-10*n);
            }
            unchanged(ch,arg);assert(strstr(output,"secondary cap"));
        }
        assert(ch->class==parents[p]&&ch->vampgen==4&&ch->exp==123456&&ch->primal==550);
        assert(ch->hit==50000&&ch->mana==51000&&ch->pcdata->quest==50000);
        for(int i=0;i<6;i++)assert(ch->pcdata->colors[i]==4);
        reload(ch);release(ch);
    }
}

static void guards_and_faults(void)
{
    CHAR_DATA *ch=fresh(CLASS_HIGHLANDER);
    const char *bad[]={"colors blue extra","colors focus no confirm","colors focus blue yes",
        "colors focus blue confirm extra","colors focus","colors bl","colors 123"};
    for(size_t i=0;i<sizeof(bad)/sizeof(*bad);i++)unchanged(ch,bad[i]);
    char huge[4096];memset(huge,'z',sizeof(huge));memcpy(huge,"colors ",7);huge[sizeof(huge)-1]='\0';
    HYBRID_STATE state=ch->pcdata->hybrid;do_selfbrid(ch,huge);
    assert(!memcmp(&state,&ch->pcdata->hybrid,sizeof(state))&&ch->primal==1000);
    ch->level=LEV_AWA;unchanged(ch,"colors focus blue confirm");ch->level=LEV_SOR;
    ch->fighting=ch;unchanged(ch,"colors focus blue confirm");ch->fighting=NULL;
    ch->position=POS_SITTING;unchanged(ch,"colors focus blue confirm");ch->position=POS_STANDING;
    shell.original=ch;unchanged(ch,"colors focus blue confirm");shell.original=NULL;
    ch->save_pending|=PLAYER_SAVE_PRIMARY;unchanged(ch,"colors focus blue confirm");
    ch->save_pending=0;
    command(ch,"COLORS FOCUS BlUe CoNfIrM");assert(strstr(output,"completed and saved"));
    ch->primal=9;unchanged(ch,"colors blue");assert(strstr(output,"Not enough primal"));ch->primal=1000;
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(*faults);i++) {
        atomic_save_test_fault=faults[i];unchanged(ch,"colors blue");
        assert(strstr(output,"No lesson or payment"));atomic_save_test_fault=SAVE_FAULT_NONE;
        unchanged(ch,"colors blue");assert(strstr(output,"recovery is pending"));
        assert(save_char_obj_checked(ch));reload(ch);
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;command(ch,"colors blue");
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat")&&ch->primal==990&&hybrid_book_color_rank(ch,BLUE_MAGIC)==1);
    assert(ch->save_pending&PLAYER_SAVE_PRIMARY);
    unchanged(ch,"colors blue");assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));command(ch,"colors blue");
    assert(ch->primal==970&&hybrid_book_color_rank(ch,BLUE_MAGIC)==2);reload(ch);
    ch->pcdata->hybrid.secondary_progression.colors[RED_MAGIC]=4;
    unchanged(ch,"colors red");assert(strstr(output,"staff review"));
    assert(ch->pcdata->hybrid.secondary_progression.colors[RED_MAGIC]==4);
    ch->pcdata->hybrid.secondary_progression.colors[RED_MAGIC]=0;
    ch->pcdata->hybrid_present=0;unchanged(ch,"colors blue");
    ch->pcdata->hybrid_present=2;unchanged(ch,"colors blue");
    ch->pcdata->hybrid_present=1;ch->pcdata->player_load_failed=1;unchanged(ch,"colors blue");
    ch->pcdata->player_load_failed=0;ch->class=CLASS_SORCERER;
    unchanged(ch,"colors blue");assert(strstr(output,"native colors"));
    ch->class=CLASS_HIGHLANDER;release(ch);
}

int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-color-ui-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(mkdir("player",0700)==0&&mkdir("backup",0700)==0&&mkdir("run",0700)==0&&chdir("run")==0);
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    current_time=123456;init_mm();fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    paths();guards_and_faults();
    assert(!char_list&&!room.people&&fclose(fpReserve)==0&&chdir("..")==0);
    assert(rmdir("player")==0&&rmdir("backup")==0&&rmdir("run")==0&&chdir(cwd)==0&&rmdir(root)==0);
    puts("secondary color UI:54 parent/focus paths,1134 durable lessons, caps/native costs/reloads/parser/actor guards and atomic failures/uncertainty passed");
    return 0;
}
