/* Real engine, atomic player files, secondary-only Werewolf lesson commands. */
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
#include "classes/hybrid/player.h"
#include "classes/hybrid/werewolf_training.h"

extern char *string_space,*top_string;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static char output[32768];
static const long primaries[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,
    CLASS_HIGHLANDER,CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,
    CLASS_VAMPIRE};
static const char *const families[]={"breed","auspice","tribe"};
static const char *const chosen_gifts[]={"homid","ragabash","black-furies"};
static const char *const alternate_gifts[]={"lupus","theurge","bone-gnawers"};
static unsigned focus_count,gift_count,rank_count,reload_count;

static CHAR_DATA *fresh_pair(long primary,long secondary)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentwolffixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);ch->name=str_dup("Wolfhybridfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;
    shell.outbuf=output;shell.outsize=sizeof(output);
    ch->level=LEV_AWA;ch->position=POS_STANDING;ch->class=primary;
    ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=51000;
    ch->primal=10000;ch->exp=123456;ch->pcdata->quest=50000;
    ch->pcdata->tgift[0]=BREED_METIS;
    ch->pcdata->tgift[1]=AUSPICE_PHILODOX;
    ch->pcdata->tgift[2]=TRIBE_WENDIGOS;
    ch->pcdata->breed[0]=2;ch->pcdata->breed[1]=1;ch->pcdata->breed[2]=3;
    ch->pcdata->auspice[0]=1;ch->pcdata->auspice[1]=2;
    ch->pcdata->auspice[2]=3;ch->pcdata->auspice[3]=1;ch->pcdata->auspice[4]=2;
    ch->pcdata->tribes[0]=2;ch->pcdata->tribes[TRIBE_WENDIGOS]=1;
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
    assert(hybrid_player_attach(ch,&state));
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;
    return ch;
}

static CHAR_DATA *fresh(long primary)
{
    return fresh_pair(primary,CLASS_WEREWOLF);
}

static void release(CHAR_DATA *ch)
{
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;
    ch->next=ch->next_in_room=NULL;ch->desc=NULL;ch->in_room=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Wolfhybridfixture");unlink("../backup/Wolfhybridfixture");
}

static void command(CHAR_DATA *ch,const char *argument)
{
    char line[512];assert(strlen(argument)<sizeof(line)-12);
    snprintf(line,sizeof(line),"selfbrid %s",argument);
    shell.outtop=0;output[0]='\0';interpret(ch,line);
    if(shell.outtop<sizeof(output))output[shell.outtop]='\0';
}

static void unchanged(CHAR_DATA *ch,const char *argument)
{
    PC_DATA before=*ch->pcdata;HYBRID_STATE state=ch->pcdata->hybrid;
    long primal=ch->primal,experience=ch->exp,class=ch->class;
    command(ch,argument);
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!memcmp(&state,&ch->pcdata->hybrid,sizeof(state)));
    assert(ch->primal==primal&&ch->exp==experience&&ch->class==class);
}

static void assert_native_banks(CHAR_DATA *ch)
{
    assert(ch->pcdata->tgift[0]==BREED_METIS&&
        ch->pcdata->tgift[1]==AUSPICE_PHILODOX&&
        ch->pcdata->tgift[2]==TRIBE_WENDIGOS);
    assert(ch->pcdata->breed[0]==2&&ch->pcdata->breed[1]==1&&ch->pcdata->breed[2]==3);
    assert(ch->pcdata->auspice[0]==1&&ch->pcdata->auspice[1]==2&&
        ch->pcdata->auspice[2]==3&&ch->pcdata->auspice[3]==1&&
        ch->pcdata->auspice[4]==2);
    assert(ch->pcdata->tribes[0]==2&&
        ch->pcdata->tribes[TRIBE_WENDIGOS]==1);
}

static void reload(CHAR_DATA *ch)
{
    for(int brief=0;brief<2;brief++){
        DESCRIPTOR_DATA d={0};
        assert(brief?load_char_short(&d,"Wolfhybridfixture"):
                     load_char_obj(&d,"Wolfhybridfixture"));
        CHAR_DATA *loaded=d.character;
        assert(!loaded->pcdata->player_load_failed&&loaded->pcdata->hybrid_present);
        assert(loaded->class==ch->class&&loaded->primal==ch->primal&&
            loaded->exp==ch->exp&&loaded->pcdata->quest==ch->pcdata->quest);
        assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
        assert(loaded->pcdata->breed[0]==2&&loaded->pcdata->breed[1]==1&&
            loaded->pcdata->breed[2]==3);
        assert(loaded->pcdata->auspice[0]==1&&loaded->pcdata->auspice[1]==2&&
            loaded->pcdata->auspice[2]==3&&loaded->pcdata->auspice[3]==1&&
            loaded->pcdata->auspice[4]==2);
        assert(loaded->pcdata->tgift[0]==BREED_METIS&&
            loaded->pcdata->tgift[1]==AUSPICE_PHILODOX&&
            loaded->pcdata->tgift[2]==TRIBE_WENDIGOS);
        assert(loaded->pcdata->tribes[TRIBE_WENDIGOS]==1);
        loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);reload_count++;
    }
}

static void branch_caps_and_costs(void)
{
    for(size_t p=0;p<sizeof(primaries)/sizeof(primaries[0]);p++)
        for(int focus=0;focus<3;focus++){
            CHAR_DATA *ch=fresh(primaries[p]);
            command(ch,"wolf");
            assert(strstr(output,"Secondary Werewolf full family: not selected"));
            assert(strstr(output,"Homid")&&strstr(output,"Ragabash")&&
                   strstr(output,"Black Furies"));
            unchanged(ch,"wolf gain breed homid");
            assert(strstr(output,"Choose and confirm"));
            char arg[96];
            snprintf(arg,sizeof(arg),"wolf focus %s",families[focus]);
            unchanged(ch,arg);
            assert(strstr(output,"Repeat with confirm")&&
                   access("../player/Wolfhybridfixture",F_OK)!=0);
            snprintf(arg,sizeof(arg),"wolf focus %s confirm",families[focus]);
            command(ch,arg);
            assert(strstr(output,"completed and saved"));
            assert(ch->pcdata->hybrid.secondary_progression.full_branch==focus);
            focus_count++;assert_native_banks(ch);reload(ch);

            for(int f=0;f<3;f++){
                snprintf(arg,sizeof(arg),"wolf gift %s %s",families[f],chosen_gifts[f]);
                unchanged(ch,arg);
                assert(strstr(output,"Repeat with confirm"));
                snprintf(arg,sizeof(arg),"wolf gift %s %s confirm",families[f],chosen_gifts[f]);
                command(ch,arg);
                assert(strstr(output,"completed and saved"));
                assert(ch->pcdata->hybrid.secondary_progression.selected_gifts[f]==0);
                gift_count++;assert_native_banks(ch);
            }
            snprintf(arg,sizeof(arg),"wolf gift %s %s confirm",families[focus],chosen_gifts[focus]);
            unchanged(ch,arg);assert(strstr(output,"already selected"));
            snprintf(arg,sizeof(arg),"wolf focus %s confirm",families[focus]);
            unchanged(ch,arg);assert(strstr(output,"already chosen"));

            for(int f=0;f<3;f++){
                int cap=f==focus?5:3;
                snprintf(arg,sizeof(arg),"wolf gain %s %s",families[f],chosen_gifts[f]);
                for(int rank=1;rank<=cap;rank++){
                    long funds=ch->primal;
                    command(ch,arg);
                    assert(strstr(output,"completed and saved"));
                    assert(ch->primal==funds-10*rank);
                    long actual=hybrid_power_rank(ch,CLASS_WEREWOLF,
                        (enum hybrid_training_tree)f,0);
                    assert(actual==rank);
                    rank_count++;
                    if(rank==1)reload(ch);
                    assert_native_banks(ch);
                }
                long funds=ch->primal;
                unchanged(ch,arg);assert(strstr(output,"at its secondary cap"));
                assert(ch->primal==funds);
            }
            /* An unselected gift within the full family retains its native
             * rank-4 ceiling and 15x cost rather than inheriting the cap 5. */
            size_t extra=1;
            snprintf(arg,sizeof(arg),"wolf gain %s %s",families[focus],
                     alternate_gifts[focus]);
            for(int rank=1;rank<=4;rank++){
                long funds=ch->primal;command(ch,arg);
                assert(strstr(output,"completed and saved"));
                assert(ch->primal==funds-15*rank);
                rank_count++;
            }
            unchanged(ch,arg);assert(strstr(output,"at its secondary cap"));
            assert(hybrid_power_rank(ch,CLASS_WEREWOLF,
                (enum hybrid_training_tree)focus,extra)==4);
            assert_native_banks(ch);reload(ch);release(ch);
        }
    assert(focus_count==27&&gift_count==81&&rank_count==405);
}

static void parser_security_and_guards(void)
{
    CHAR_DATA *ch=fresh(CLASS_DROW);
    const char *bad[]={"wolf nonsense","wolf extra","wolf focus","wolf focus mage confirm",
        "wolf focus breed maybe","wolf focus breed confirm trailing",
        "wolf gift tribe children-of-gaia","wolf gift tribe unknown confirm",
        "wolf gift tribe wendigos extra","wolf gain tribe wendigos extra",
        "wolf gain tribe wendigos","wolf gain breed homid trailing",
        "wolf focus aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa confirm","wolves focus breed confirm"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);i++)unchanged(ch,bad[i]);
    assert(!hybrid_werewolf_training_command(ch,NULL));
    assert(!hybrid_werewolf_training_command(ch,"wolves"));
    assert(hybrid_werewolf_training_command(NULL,"wolf"));
    unchanged(ch,"wolf gain breed homid");assert(strstr(output,"Choose and confirm"));
    ch->fighting=ch;unchanged(ch,"wolf focus breed confirm");ch->fighting=NULL;
    ch->position=POS_SLEEPING;unchanged(ch,"wolf focus breed confirm");ch->position=POS_STANDING;
    shell.original=ch;unchanged(ch,"wolf focus breed confirm");shell.original=NULL;
    ch->save_pending|=PLAYER_SAVE_PRIMARY;
    unchanged(ch,"wolf focus breed confirm");assert(strstr(output,"recovery is pending"));
    ch->save_pending=0;
    atomic_save_test_fault=SAVE_FAULT_RENAME;
    unchanged(ch,"wolf focus breed confirm");
    assert(strstr(output,"No lesson or payment"));
    atomic_save_test_fault=SAVE_FAULT_NONE;
    unchanged(ch,"wolf focus breed confirm");assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));reload(ch);
    command(ch,"wolf focus breed confirm");assert(strstr(output,"completed and saved"));
    ch->primal=1;unchanged(ch,"wolf gain breed homid");assert(strstr(output,"Not enough primal"));
    ch->primal=10000;
    ch->pcdata->hybrid.secondary_progression.breed[0]=5;
    unchanged(ch,"wolf gain breed homid");assert(strstr(output,"staff review"));
    ch->pcdata->hybrid.secondary_progression.breed[0]=0;
    ch->pcdata->hybrid_present=0;unchanged(ch,"wolf gain breed homid");
    ch->pcdata->hybrid_present=1;ch->pcdata->player_load_failed=1;
    unchanged(ch,"wolf gain breed homid");ch->pcdata->player_load_failed=0;
    assert_native_banks(ch);release(ch);

    ch=fresh_pair(CLASS_MAGE,CLASS_MONK);
    unchanged(ch,"wolf focus breed confirm");
    assert(strstr(output,"valid secondary Werewolf record"));
    release(ch);
}

static void original_werewolf(void)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentwolffixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);ch->name=str_dup("Wolfhybridfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;
    shell.outbuf=output;shell.outsize=sizeof(output);
    ch->level=LEV_AWA;ch->position=POS_STANDING;ch->class=CLASS_WEREWOLF;
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;
    unchanged(ch,"wolf focus breed confirm");
    assert(strstr(output,"unchanged native gifts"));
    command(ch,"gifts homid");
    assert(ch->pcdata->tgift[0]==BREED_HOMID);
    release(ch);
}

int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-wolf-ui-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    current_time=123456;init_mm();fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=901;room.area=&area;room.name="Private hybrid fixture";room.description="";
    room_index_hash[901%MAX_KEY_HASH]=&room;
    branch_caps_and_costs();parser_security_and_guards();original_werewolf();
    assert(!char_list&&!room.people&&!fclose(fpReserve)&&!chdir(".."));
    assert(!rmdir("player")&&!rmdir("backup")&&!rmdir("run")&&!chdir(cwd)&&!rmdir(root));
    printf("Secondary Werewolf UI: %u focus choices, %u gift choices, %u paid ranks, "
           "%u loader roundtrips; all 9 primary parents, 5/3 caps, native data, "
           "parser/context and atomic-save guards passed.\n",
           focus_count,gift_count,rank_count,reload_count);
    return 0;
}
