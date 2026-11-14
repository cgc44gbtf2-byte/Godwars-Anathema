#define _POSIX_C_SOURCE 200809L
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include "merc.h"
#include "atomic_save.h"

extern char *string_space, *top_string;
extern void init_mm(void);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA d;

static void command(CHAR_DATA *ch, const char *text, bool direct)
{
    snprintf(d.incomm, sizeof(d.incomm), "%s", text);
    d.fcommand = TRUE;
    if (direct) interpret(ch, d.incomm);
    else { char nested[128]; snprintf(nested,sizeof(nested),"%s",text); interpret(ch,nested); }
}

int main(void)
{
    char root[]="/tmp/anathema-owner-XXXXXX", cwd[4096];
    assert(getcwd(cwd,sizeof(cwd)) && mkdtemp(root) && chdir(root)==0);
    assert(mkdir("player",0700)==0 && mkdir("backup",0700)==0 &&
           mkdir("run",0700)==0 && chdir("run")==0);
    string_space=malloc(4*1024*1024); assert(string_space); top_string=string_space;
    current_time=time(NULL); init_mm(); fpReserve=fopen(NULL_FILE,"r"); assert(fpReserve);
    room.vnum=ROOM_VNUM_TEMPLE; room.area=&area; room.name="Owner test"; room.description="";
    room_index_hash[room.vnum%MAX_KEY_HASH]=&room;
    DESCRIPTOR_DATA shell={0}; load_char_obj(&shell,"Luto");
    CHAR_DATA *ch=shell.character; ch->level=LEV_SLE; ch->position=POS_STANDING;
    free_string(ch->pcdata->pwd);
    ch->pcdata->pwd=str_dup("fixture-only-not-a-usable-login-hash");
    ch->next=char_list; char_list=ch; char_to_room(ch,&room);
    ch->desc=&d; d.character=ch; d.connected=CON_PLAYING;
    d.outsize=32000; d.outbuf=alloc_mem(d.outsize);
    long hp=ch->hit, maxhp=ch->max_hit, mana=ch->mana, maxmana=ch->max_mana;
    command(ch,"authorize",FALSE); assert(ch->level==LEV_SLE);
    command(ch,"auth",TRUE); assert(ch->level==LEV_SLE);
    command(ch,"authorize Luto",TRUE); assert(ch->level==LEV_SLE);
    char *original=ch->name; ch->name=str_dup("Yami");
    command(ch,"authorize",TRUE); assert(ch->level==LEV_SLE); free_string(ch->name);
    ch->name=str_dup("Lutob"); command(ch,"authorize",TRUE);
    assert(ch->level==LEV_SLE); free_string(ch->name); ch->name=original;
    d.original=ch; command(ch,"authorize",TRUE); assert(ch->level==LEV_SLE); d.original=NULL;
    SET_BIT(ch->affected_by,AFF_CHARM); command(ch,"authorize",TRUE);
    assert(ch->level==LEV_SLE); REMOVE_BIT(ch->affected_by,AFF_CHARM);
    d.connected=CON_GET_OLD_PASSWORD; command(ch,"authorize",TRUE);
    assert(ch->level==LEV_SLE); d.connected=CON_PLAYING;
    ch->desc=NULL; do_authorize(ch,""); assert(ch->level==LEV_SLE); ch->desc=&d;
    d.character=NULL; do_authorize(ch,""); assert(ch->level==LEV_SLE); d.character=ch;
    d.fcommand=FALSE; do_authorize(ch,""); assert(ch->level==LEV_SLE); d.fcommand=TRUE;
    char *password=ch->pcdata->pwd; ch->pcdata->pwd="";
    command(ch,"authorize",TRUE); assert(ch->level==LEV_SLE); ch->pcdata->pwd=password;
    atomic_save_test_fault=SAVE_FAULT_RENAME;
    command(ch,"authorize",TRUE);
    assert(ch->level==LEV_SLE && get_trust(ch)==LEV_SLE && access("../player/Luto",F_OK)!=0);
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(rename("../backup","../backup-off")==0);
    command(ch,"authorize",TRUE);
    assert(ch->level==LEV_ST && get_trust(ch)==MAX_LEVEL);
    assert(ch->hit==hp && ch->max_hit==maxhp && ch->mana==mana && ch->max_mana==maxmana);
    assert(strstr(d.outbuf,"backup retry pending")!=NULL);
    assert(access("../player/Luto",F_OK)==0 && access("../backup/Luto",F_OK)!=0);
    assert(rename("../backup-off","../backup")==0);
    assert(player_save_state(ch,PLAYER_SAVE_BACKUP) & PLAYER_SAVE_BACKUP);
    assert(access("../player/Luto",F_OK)==0 && access("../backup/Luto",F_OK)==0);
    DESCRIPTOR_DATA loaded={0}; assert(load_char_obj(&loaded,"Luto"));
    assert(loaded.character->level==LEV_ST && get_trust(loaded.character)==LEV_ST);
    free_char(loaded.character);
    command(ch,"authorize",TRUE); assert(ch->level==LEV_ST);
    ch->desc=NULL; d.character=NULL; extract_char(ch,TRUE);
    free_mem(d.outbuf,d.outsize); assert(fclose(fpReserve)==0);
    assert(unlink("../player/Luto")==0 && unlink("../backup/Luto")==0);
    assert(chdir("..")==0 && rmdir("player")==0 && rmdir("backup")==0 &&
           rmdir("run")==0 && chdir(cwd)==0 && rmdir(root)==0);
    puts("Luto-only direct-input authorization, save failure, durable highest rank and unchanged stats passed");
    return 0;
}
