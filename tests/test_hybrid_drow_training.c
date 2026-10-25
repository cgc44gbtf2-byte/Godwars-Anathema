/* Secondary Drow utility purchases through the real selfbrid interpreter. */
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

extern char *string_space,*top_string;
extern void init_mm(void);

static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static char output[16384];
static unsigned purchases;
static const long originals[] = {
    CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,CLASS_MAGE,
    CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF
};

static CHAR_DATA *fresh(long primary,long secondary,long dp)
{
    memset(&shell,0,sizeof(shell));
    memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"AbsentDrowfixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);
    ch->name=str_dup("Drowfixture");
    free_string(ch->short_descr);
    ch->short_descr=str_dup("Drowfixture");
    ch->desc=&shell;
    shell.connected=CON_PLAYING;
    shell.outbuf=output;
    shell.outsize=sizeof(output);
    ch->level=LEV_AWA;
    ch->position=POS_STANDING;
    ch->class=primary;
    ch->hit=ch->max_hit=50000;
    ch->mana=ch->max_mana=50000;
    ch->primal=9876;
    ch->exp=7654321;
    ch->pcdata->quest=50000;
    ch->pcdata->classpoints[CP_CURRENT]=321;
    ch->in_room=&room;
    ch->next=char_list;
    char_list=ch;
    ch->next_in_room=room.people;
    room.people=ch;

    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
    if(secondary==CLASS_DROW)
        state.secondary_classpoints[CP_CURRENT]=dp;
    assert(hybrid_player_attach(ch,&state));
    return ch;
}

static void release(CHAR_DATA *ch)
{
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;
    room.people=ch->next_in_room;
    ch->next=ch->next_in_room=NULL;
    ch->desc=NULL;
    ch->in_room=NULL;
    free_char(ch);
    shell.character=NULL;
    unlink("../player/Drowfixture");
    unlink("../backup/Drowfixture");
}

static void command(CHAR_DATA *ch,const char *argument)
{
    char line[128];
    assert(strlen(argument)<sizeof(line)-16);
    snprintf(line,sizeof(line),"selfbrid %s",argument);
    shell.outtop=0;
    output[0]='\0';
    interpret(ch,line);
}

static void unchanged(CHAR_DATA *ch,const char *argument)
{
    PC_DATA before=*ch->pcdata;
    long primal=ch->primal,exp=ch->exp,quest=ch->pcdata->quest;
    command(ch,argument);
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(ch->primal==primal&&ch->exp==exp&&ch->pcdata->quest==quest);
}

static void supported_utilities(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);++i) {
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,25000);
        long native_dp=ch->pcdata->classpoints[CP_CURRENT];
        long primal=ch->primal,exp=ch->exp,quest=ch->pcdata->quest;
        assert(!hybrid_power_drow(ch,DROW_ATTUNE));
        assert(!hybrid_power_drow(ch,DROW_FIRE));
        command(ch,"drow");
        assert(strstr(output,"attunement: available"));
        assert(strstr(output,"1000 DP"));
        assert(strstr(output,"faeriefire: available"));
        assert(strstr(output,"acidbolt: available"));
        assert(strstr(output,"llothsweb: available"));
        assert(strstr(output,"readaura: available"));
        assert(strstr(output,"5000 DP"));
        assert(strstr(output,"Other secondary Drow powers"));

        command(ch,"drow attunement");
        assert(strstr(output,"1000 DP to learn"));
        unchanged(ch,"drow summonspider learn");
        unchanged(ch,"drow faeriefire learn trailing");
        unchanged(ch,"drow faeriefire unknown");
        assert(!hybrid_power_drow(ch,DROW_ACID));

        command(ch,"drow attunement learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_ATTUNE));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==24000);
        command(ch,"drow attunement learn");
        assert(strstr(output,"already learned"));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==24000);

        OBJ_INDEX_DATA index={0};
        OBJ_DATA item={0};
        item.pIndexData=&index;
        item.name="token";
        item.short_descr="a test token";
        item.questowner=ch->name;
        item.item_type=ITEM_ARMOR;
        item.wear_loc=WEAR_NONE;
        item.carried_by=ch;
        ch->carrying=&item;
        interpret(ch,"attunement token");
        assert(IS_SET(item.extra_flags,ITEM_LOYAL));
        ch->carrying=NULL;

        command(ch,"drow faeriefire learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_FIRE));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==19000);
        command(ch,"drow acidbolt learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_ACID));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==14000);
        unchanged(ch,"drow acidbolt learn");
        assert(strstr(output,"already learned"));
        command(ch,"drow llothsweb learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_WEB));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==6000);
        unchanged(ch,"drow llothsweb learn");
        assert(strstr(output,"already learned"));
        command(ch,"drow readaura learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_READ));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        unchanged(ch,"drow readaura learn");
        assert(strstr(output,"already learned"));
        assert(ch->pcdata->classpoints[CP_CURRENT]==native_dp);
        assert(ch->primal==primal&&ch->exp==exp&&ch->pcdata->quest==quest);
        assert(ch->class==originals[i]);
        assert(ch->pcdata->drow[DROW_POWERS]==0);

        DESCRIPTOR_DATA reload={0};
        assert(load_char_obj(&reload,"Drowfixture"));
        assert(reload.character&&hybrid_power_drow(reload.character,DROW_ATTUNE));
        assert(hybrid_power_drow(reload.character,DROW_FIRE));
        assert(hybrid_power_drow(reload.character,DROW_ACID));
        assert(hybrid_power_drow(reload.character,DROW_WEB));
        assert(hybrid_power_drow(reload.character,DROW_READ));
        assert(reload.character->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        assert(reload.character->pcdata->classpoints[CP_CURRENT]==native_dp);
        assert(reload.character->class==originals[i]);
        free_char(reload.character);
        reload.character=NULL;
        assert(load_char_short(&reload,"Drowfixture"));
        assert(reload.character&&hybrid_power_drow(reload.character,DROW_WEB));
        assert(hybrid_power_drow(reload.character,DROW_READ));
        assert(reload.character->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        assert(reload.character->class==originals[i]);
        free_char(reload.character);
        reload.character=NULL;
        purchases+=5;
        release(ch);
    }
    assert(purchases==45);
}

static void refusals_and_native_isolation(void)
{
    CHAR_DATA *ch=fresh(CLASS_MAGE,CLASS_DROW,4999);
    PC_DATA before=*ch->pcdata;
    command(ch,"drow faeriefire learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_FIRE));
    command(ch,"drow acidbolt learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_ACID));
    command(ch,"drow llothsweb learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_WEB));
    command(ch,"drow readaura learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_READ));
    release(ch);

    ch=fresh(CLASS_MAGE,CLASS_MONK,6000);
    unchanged(ch,"drow attunement learn");
    assert(strstr(output,"valid secondary Drow record"));
    release(ch);

    ch=fresh(CLASS_DROW,CLASS_MONK,6000);
    ch->pcdata->drow[DROW_POWERS]=DROW_ATTUNE|DROW_FIRE;
    PC_DATA native=*ch->pcdata;
    command(ch,"drow");
    assert(strstr(output,"original class"));
    assert(!memcmp(&native,ch->pcdata,sizeof(native)));
    assert(hybrid_power_drow(ch,DROW_ATTUNE)&&hybrid_power_drow(ch,DROW_FIRE));
    release(ch);
}

static void context_and_save_faults(void)
{
    CHAR_DATA *ch=fresh(CLASS_MAGE,CLASS_DROW,6000);
    ch->fighting=ch;
    unchanged(ch,"drow attunement learn");
    ch->fighting=NULL;
    assert(!hybrid_power_drow(ch,DROW_ATTUNE));
    shell.original=ch;
    unchanged(ch,"drow attunement learn");
    shell.original=NULL;
    ch->save_pending|=PLAYER_SAVE_PRIMARY;
    unchanged(ch,"drow attunement learn");
    assert(strstr(output,"recovery is pending"));
    ch->save_pending=0;
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
                        SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);++i) {
        atomic_save_test_fault=faults[i];
        unchanged(ch,"drow attunement learn");
        assert(strstr(output,"No lesson or points"));
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(!hybrid_power_drow(ch,DROW_ATTUNE));
        unchanged(ch,"drow attunement learn");
        assert(strstr(output,"recovery is pending"));
        assert(save_char_obj_checked(ch));
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    command(ch,"drow attunement learn");
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat"));
    assert(hybrid_power_drow(ch,DROW_ATTUNE));
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==5000);
    unchanged(ch,"drow faeriefire learn");
    assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));
    command(ch,"drow faeriefire learn");
    assert(strstr(output,"learned and saved"));
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
    release(ch);
}

static void utility_save_faults(const char *lesson,long flag,long price)
{
    CHAR_DATA *ch=fresh(CLASS_NINJA,CLASS_DROW,price);
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
                        SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);i++) {
        atomic_save_test_fault=faults[i];
        unchanged(ch,lesson);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(!hybrid_power_drow(ch,flag));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==price);
        assert(save_char_obj_checked(ch));
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    command(ch,lesson);
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat"));
    assert(hybrid_power_drow(ch,flag));
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
    unchanged(ch,lesson);
    assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));
    unchanged(ch,lesson);
    assert(strstr(output,"already learned"));
    release(ch);
}

int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-hybrid-drow-training-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&
           !mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);
    assert(string_space);
    top_string=string_space;
    current_time=123456;
    init_mm();
    fpReserve=fopen(NULL_FILE,"r");
    assert(fpReserve);
    area.name="Hybrid Drow test";
    room.vnum=901;
    room.area=&area;
    room.name="Private fixture";
    room.description="";
    room_index_hash[901%MAX_KEY_HASH]=&room;
    supported_utilities();
    refusals_and_native_isolation();
    context_and_save_faults();
    utility_save_faults("drow llothsweb learn",DROW_WEB,8000);
    utility_save_faults("drow readaura learn",DROW_READ,6000);
    assert(!char_list&&!room.people&&!fclose(fpReserve));
    assert(!chdir("..")&&!rmdir("player")&&!rmdir("backup")&&
           !rmdir("run")&&!chdir(cwd)&&!rmdir(root));
    printf("Secondary Drow UI: %u paid powers across nine original parents, "
           "atomic reloads, DP isolation, adapter use and refusal paths passed.\n",
           purchases);
    return 0;
}
