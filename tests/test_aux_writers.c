/* Fault injection against actual board/leaderboard saves in a private world. */
#define ATOMIC_SAVE_TESTING
#define FILE_NUMBERS_FIXTURE_ONLY
#include "test_file_numbers.c"
#include "atomic_save.h"
#include "auxiliary_save.h"
#include <dirent.h>
#include <signal.h>
#include <sys/resource.h>
extern NOTE_DATA *new_note(void);
extern void save_notes(void);
static unsigned checks;
static NOTE_DATA *note(const char *subject)
{
    NOTE_DATA *n=new_note();
    n->sender=str_dup("Writer");n->date=str_dup("fixed date");
    n->to_list=str_dup("all");n->subject=str_dup(subject);
    n->text=str_dup("fixed body");n->expire=1000000;
    return n;
}
static uint64_t contents(const char *path)
{
    FILE *fp=fopen(path,"rb");assert(fp);
    unsigned char buf[512];size_t n;uint64_t h=UINT64_C(1469598103934665603);
    while((n=fread(buf,1,sizeof buf,fp)))h=hash(h,buf,n);
    assert(!ferror(fp)&&fclose(fp)==0);return h;
}
static unsigned temps(const char *dir)
{
    DIR *d=opendir(dir);assert(d);unsigned n=0;struct dirent *e;
    while((e=readdir(d)))if(!strncmp(e->d_name,".save-",6))n++;
    assert(closedir(d)==0);return n;
}
static void board_failure(enum atomic_save_fault fault)
{
    NOTE_DATA *n=note("candidate");n->date_stamp=47;
    NOTE_DATA *first=boards[1].note_first;
    uint64_t before=contents("../notes/General");
    atomic_save_test_fault=fault;
    assert(!finish_note(&boards[1],n));
    assert(boards[1].note_first==first&&first->next==NULL);
    assert(n->date_stamp==47&&n->next==NULL);
    assert(contents("../notes/General")==before&&temps("../notes")==0);
    atomic_save_test_fault=SAVE_FAULT_NONE;free_note(n);checks++;
}
static void leader_failure(enum atomic_save_fault fault)
{
    uint64_t before=contents("../area/leader.txt");
    leader_board.pk_number++;
    atomic_save_test_fault=fault;save_leaderboard();
    assert(contents("../area/leader.txt")==before&&temps("../area")==0);
    atomic_save_test_fault=SAVE_FAULT_NONE;save_leaderboard();
    assert(contents("../area/leader.txt")!=before);checks++;
}
int main(int argc,char **argv)
{
    sandbox_begin();assert(mkdir("../notes",0700)==0);
    metrics_fixture();current_time=100;init_mm();fBootDb=FALSE;
    assert(finish_note(&boards[1],note("first")));save_leaderboard();
    if(argc>1&&!strcmp(argv[1],"--board-red"))board_failure(SAVE_FAULT_RENAME);
    else if(argc>1&&!strcmp(argv[1],"--leader-red"))leader_failure(SAVE_FAULT_RENAME);
    else {
        const enum atomic_save_fault failures[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,
            SAVE_FAULT_SYNC,SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
        for(unsigned i=0;i<sizeof failures/sizeof *failures;i++){
            board_failure(failures[i]);leader_failure(failures[i]);
        }
        /* Real kernel write failure, not an atomic-helper fault hook. Restore
         * process limits before assertions/logging; no live files are touched. */
        struct rlimit original,limited;
        assert(getrlimit(RLIMIT_FSIZE,&original)==0);
        limited=original;limited.rlim_cur=0;
        void (*previous_signal)(int)=signal(SIGXFSZ,SIG_IGN);
        NOTE_DATA *limited_note=note("kernel write failure");
        uint64_t board_before=contents("../notes/General");
        assert(setrlimit(RLIMIT_FSIZE,&limited)==0);
        bool accepted=finish_note(&boards[1],limited_note);
        assert(setrlimit(RLIMIT_FSIZE,&original)==0);
        clearerr(stderr);
        assert(!accepted&&contents("../notes/General")==board_before);
        free_note(limited_note);checks++;
        uint64_t score_before=contents("../area/leader.txt");
        leader_board.pk_number++;
        assert(setrlimit(RLIMIT_FSIZE,&limited)==0);
        save_leaderboard();
        assert(setrlimit(RLIMIT_FSIZE,&original)==0);
        clearerr(stderr);signal(SIGXFSZ,previous_signal);
        assert(contents("../area/leader.txt")==score_before);
        save_leaderboard();assert(contents("../area/leader.txt")!=score_before);checks++;
        NOTE_DATA *n=note("visible uncertain");
        atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
        assert(finish_note(&boards[1],n)&&boards[1].changed);
        assert(boards[1].note_first->next==n);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        current_time+=60;save_notes();assert(!boards[1].changed);
        assert(temps("../notes")==0);checks++;
        /* Replacement failure must retain the complete old file and dirty flag. */
        uint64_t old=contents("../notes/General");
        free_string(n->text);n->text=str_dup("changed body");
        boards[1].changed=TRUE;atomic_save_test_fault=SAVE_FAULT_RENAME;
        current_time+=60;save_notes();assert(boards[1].changed);
        assert(contents("../notes/General")==old);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        current_time+=60;save_notes();assert(!boards[1].changed);
        assert(contents("../notes/General")!=old);checks++;
        /* A failure in the real editor keeps ownership and no false broadcast. */
        PC_DATA pc={0};CHAR_DATA writer={0};DESCRIPTOR_DATA desc={0};char output[16384]={0};
        pc.board=&boards[1];pc.in_progress=note("editor");
        writer.pcdata=&pc;writer.name="Writer";writer.desc=&desc;
        desc.character=&writer;desc.connected=CON_NOTE_FINISH;
        desc.outbuf=output;desc.outsize=sizeof output;
        atomic_save_test_fault=SAVE_FAULT_RENAME;
        handle_con_note_finish(&desc,"p");
        assert(pc.in_progress&&desc.connected==CON_NOTE_FINISH);
        assert(strstr(output,"draft has been kept")&&!strstr(output,"Note posted"));
        atomic_save_test_fault=SAVE_FAULT_NONE;free_note(pc.in_progress);checks++;
        /* Periodic retries respect a delay and retain newer score changes. */
        uint64_t leader_before=contents("../area/leader.txt");
        atomic_save_test_fault=SAVE_FAULT_RENAME;leader_board.pk_number++;
        save_leaderboard();atomic_save_test_fault=SAVE_FAULT_NONE;
        retry_leaderboard_save();assert(contents("../area/leader.txt")==leader_before);
        current_time+=29;retry_leaderboard_save();
        assert(contents("../area/leader.txt")==leader_before);
        leader_board.qc_number+=3;current_time++;
        retry_leaderboard_save();assert(contents("../area/leader.txt")!=leader_before);checks++;
        /* Unsupported serialized fields never replace a loadable file. */
        NOTE_DATA *bad=note("bad~subject");uint64_t before=contents("../notes/General");
        assert(!finish_note(&boards[1],bad)&&bad->date_stamp==0);
        assert(contents("../notes/General")==before);free_note(bad);checks++;
        /* Valid automatic notices have no editor: retain in RAM and retry. */
        atomic_save_test_fault=SAVE_FAULT_RENAME;
        make_note("General","System","all","automatic retry",1,"notice body");
        assert(boards[1].changed&&contents("../notes/General")==before);
        atomic_save_test_fault=SAVE_FAULT_NONE;current_time+=30;save_notes();
        assert(!boards[1].changed&&contents("../notes/General")!=before);checks++;
        /* Do not grant the player-file API arbitrary filename/path access. */
        assert(!atomic_save_name_valid("leader.txt"));
        assert(!atomic_save_name_valid("../General"));checks++;
        /* Reposting a board-owned last note must never create a self-cycle. */
        NOTE_DATA *tail=boards[1].note_first;while(tail->next)tail=tail->next;
        assert(!finish_note(&boards[1],tail)&&tail->next==NULL);checks++;
        /* Timestamp exhaustion is a refusal, not signed overflow. */
        current_time=LONG_MAX;
        assert(finish_note(&boards[1],note("last timestamp")));
        bad=note("exhausted");assert(!finish_note(&boards[1],bad));
        free_note(bad);checks++;
        puts("AUX_WRITERS_FAILURES_AND_RECOVERY_PASSED");
    }
    while(boards[1].note_first){
        NOTE_DATA *n=boards[1].note_first;
        boards[1].note_first=n->next;free_note(n);
    }
    board_names(0);unlink("../notes/General");assert(rmdir("../notes")==0);
    sandbox_end();printf("AUX_WRITERS_CHECKS=%u\n",checks);return 0;
}
