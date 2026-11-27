/* Full engine: no live account/world, real restart and TERM-loop boundaries. */
#define RESTART_FIXTURE_ONLY
#include "test_restart.c"
#include "auxiliary_save.h"
#include <signal.h>
#include <stdint.h>
extern volatile sig_atomic_t shutdown_requested;
extern NOTE_DATA *new_note(void);
extern void game_loop_unix(int);
static unsigned checks;

static uint64_t fingerprint(const char *name)
{
    FILE *fp=fopen(name,"rb");assert(fp);
    uint64_t h=UINT64_C(1469598103934665603);int c;
    while((c=fgetc(fp))!=EOF)h=(h^(unsigned char)c)*UINT64_C(1099511628211);
    assert(!ferror(fp)&&fclose(fp)==0);return h;
}
static void aux_clear(void)
{
    atomic_save_test_fault=SAVE_FAULT_NONE;
    for(long i=0;i<MAX_BOARD;i++){
        while(boards[i].note_first){
            NOTE_DATA *n=boards[i].note_first;boards[i].note_first=n->next;
            free_note(n);
        }
        boards[i].changed=FALSE;boards[i].load_failed=FALSE;
    }
    do_leaderclear(NULL,"");
}
static void seed_aux(void)
{
    aux_clear();
    NOTE_DATA *n=new_note();
    n->sender=str_dup("System");n->date=str_dup("fixed");
    n->to_list=str_dup("all");n->subject=str_dup("baseline");
    n->text=str_dup("baseline body");n->expire=current_time+86400;
    assert(finish_note(&boards[1],n));
}
static void pending_aux(void)
{
    atomic_save_test_fault=SAVE_FAULT_RENAME;
    make_note("General","System","all","accepted pending note",1,"pending body");
    leader_board.pk_number+=7;save_leaderboard();
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(boards[1].changed);
}
static unsigned note_count(void)
{
    unsigned n=0;
    for(NOTE_DATA *p=boards[1].note_first;p;p=p->next)assert(++n<10);
    return n;
}
static void block_file(const char *name)
{
    char backup[256];assert(snprintf(backup,sizeof backup,"%s.saved",name)>0);
    assert(rename(name,backup)==0&&mkdir(name,0700)==0);
}
static void unblock_file(const char *name)
{
    char backup[256];assert(snprintf(backup,sizeof backup,"%s.saved",name)>0);
    assert(rmdir(name)==0&&rename(backup,name)==0);
}
static void manual_shutdown(void)
{
    seed_aux();pending_aux();block_file("../notes/General");
    CHAR_DATA *ch=player("Auxstop",TRUE);DESCRIPTOR_DATA *d=ch->desc;
    uint64_t old=fingerprint("../area/leader.txt");
    do_shutdown(ch,"");
    assert(!merc_down&&ch->desc==d&&descriptor_list==d);
    assert(access(SHUTDOWN_FILE,F_OK)!=0&&boards[1].changed);
#ifndef AUX_SHUTDOWN_BASELINE
    assert(fingerprint("../area/leader.txt")!=old); /* No short circuit. */
    unblock_file("../notes/General");do_shutdown(ch,"");
    assert(merc_down&&!boards[1].changed&&note_count()==2);checks++;
    clean();
#else
    (void)old;
#endif
}
static void copyover_pending(void)
{
    seed_aux();pending_aux();block_file("../area/leader.txt");
    CHAR_DATA *ch=player("Auxcopy",TRUE);DESCRIPTOR_DATA *d=ch->desc;
    restart_test_exec_count=0;do_copyover(ch,"");
    assert(restart_test_exec_count==0&&ch->desc==d&&access(COPYOVER_FILE,F_OK)!=0);
#ifndef AUX_SHUTDOWN_BASELINE
    assert(!boards[1].changed&&note_count()==2);
    unblock_file("../area/leader.txt");do_copyover(ch,"");
    assert(restart_test_exec_count==1&&ch->desc==d);checks++;clean();
#endif
}
static void term_checkpoint(bool fail)
{
    seed_aux();pending_aux();
    if(fail)block_file("../notes/General");
    uint64_t leader=fingerprint("../area/leader.txt");
    CHAR_DATA *ch=player("Auxterm",TRUE);(void)ch;
    int capture[2],saved=dup(STDERR_FILENO);assert(saved>=0&&pipe(capture)==0);
    fflush(stderr);assert(dup2(capture[1],STDERR_FILENO)>=0);close(capture[1]);
    shutdown_requested=1;merc_down=FALSE;game_loop_unix(-1);
    fflush(stderr);assert(dup2(saved,STDERR_FILENO)>=0);close(saved);
    char messages[4096];ssize_t n=read(capture[0],messages,sizeof messages-1);
    assert(n>=0);messages[n]=0;close(capture[0]);
    assert(merc_down&&descriptor_list==NULL);
    if(!fail)assert(!boards[1].changed);
#ifndef AUX_SHUTDOWN_BASELINE
    assert(fingerprint("../area/leader.txt")!=leader);
    if(fail){
        assert(boards[1].changed&&strstr(messages,"stopping with unconfirmed"));
        unblock_file("../notes/General");
        assert(auxiliary_save_checkpoint());
    }else assert(!strstr(messages,"stopping with unconfirmed"));
    assert(note_count()==2);checks++;shutdown_requested=0;clean();
#else
    (void)leader;
#endif
}
#ifndef AUX_SHUTDOWN_BASELINE
static void final_passes(void)
{
    seed_aux();
    uint64_t board=fingerprint("../notes/General"),leader=fingerprint("../area/leader.txt");
    pending_aux();
    /* The ordinary pulse honors backoff; the final pass must not. */
    save_notes();retry_leaderboard_save();
    assert(fingerprint("../notes/General")==board&&fingerprint("../area/leader.txt")==leader);
    assert(auxiliary_save_checkpoint());
    assert(fingerprint("../notes/General")!=board&&fingerprint("../area/leader.txt")!=leader);
    assert(!boards[1].changed&&note_count()==2);checks++;
    struct stat b,l;assert(stat("../notes/General",&b)==0&&stat("../area/leader.txt",&l)==0);
    assert(auxiliary_save_checkpoint());
    struct stat b2,l2;assert(stat("../notes/General",&b2)==0&&stat("../area/leader.txt",&l2)==0);
    assert(b.st_ino==b2.st_ino&&l.st_ino==l2.st_ino);checks++;
    enum atomic_save_fault faults[]={SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
                                    SAVE_FAULT_RENAME,SAVE_FAULT_DIRSYNC};
    for(unsigned i=0;i<sizeof faults/sizeof *faults;i++){
        seed_aux();pending_aux();atomic_save_test_fault=faults[i];
        assert(!auxiliary_save_checkpoint()&&boards[1].changed);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(auxiliary_save_checkpoint()&&!boards[1].changed&&note_count()==2);
        checks++;
    }
    seed_aux();pending_aux();board=fingerprint("../notes/General");
    boards[1].load_failed=TRUE;
    assert(!auxiliary_save_checkpoint()&&boards[1].changed);
    assert(fingerprint("../notes/General")==board);boards[1].load_failed=FALSE;
    assert(auxiliary_save_checkpoint());checks++;
    seed_aux();pending_aux();
    FILE *fp=fopen("../area/leader.txt","w");assert(fp);
    assert(fputs("broken~\n",fp)>=0&&fclose(fp)==0);
    load_leaderboard();leader=fingerprint("../area/leader.txt");
    assert(!auxiliary_save_checkpoint()&&fingerprint("../area/leader.txt")==leader);
    assert(!boards[1].changed);do_leaderclear(NULL,"");
    assert(auxiliary_save_checkpoint());checks++;
    seed_aux();pending_aux();
    atomic_save_test_fault=SAVE_FAULT_RENAME;
    make_note(boards[0].short_name,"System","all","other board",1,"other pending body");
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(boards[0].changed);
    block_file("../notes/General");
    assert(!auxiliary_save_checkpoint()&&!boards[0].changed&&boards[1].changed);
    unblock_file("../notes/General");assert(auxiliary_save_checkpoint());checks++;
}
#endif
int main(int argc,char **argv)
{
    char root[]="/tmp/anathema-auxstop-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&chdir(root)==0);
    const char *dirs[]={"player","backup","run","src","notes","area"};
    for(unsigned i=0;i<sizeof dirs/sizeof *dirs;i++)assert(mkdir(dirs[i],0700)==0);
    assert(chdir("run")==0);globals();
    FILE *exe=fopen("../src/merc","w");assert(exe);
    assert(fputs("#!/bin/sh\nexit 99\n",exe)>=0&&fclose(exe)==0&&chmod("../src/merc",0700)==0);
    control=socket(AF_INET,SOCK_STREAM,0);assert(control>=3);
    struct sockaddr_in address={0};address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(control,(struct sockaddr*)&address,sizeof address)==0&&listen(control,8)==0);
    socklen_t length=sizeof address;
    assert(getsockname(control,(struct sockaddr*)&address,&length)==0);port=ntohs(address.sin_port);
    if(argc>1&&!strcmp(argv[1],"--red-shutdown"))manual_shutdown();
    else if(argc>1&&!strcmp(argv[1],"--red-copyover"))copyover_pending();
    else if(argc>1&&!strcmp(argv[1],"--red-term"))term_checkpoint(FALSE);
#ifndef AUX_SHUTDOWN_BASELINE
    else{final_passes();manual_shutdown();copyover_pending();term_checkpoint(TRUE);term_checkpoint(FALSE);}
#endif
    aux_clear();clean();shutdown_requested=0;close(control);fclose(fpReserve);
    assert(unlink("../src/merc")==0&&chdir("..")==0);
    remove_directory("player");remove_directory("backup");
    remove_directory("notes");remove_directory("area");
    assert(rmdir("run")==0&&rmdir("src")==0&&chdir(cwd)==0&&rmdir(root)==0);
    printf("AUXILIARY_SHUTDOWN_CHECKS=%u\n",checks);return 0;
}
