/* Full-engine auxiliary files in a private sandbox; no live notes or scores. */
#define FILE_NUMBERS_FIXTURE_ONLY
#include "test_file_numbers.c"
#include <signal.h>
#include <dirent.h>
extern NOTE_DATA *new_note(void);
extern long nAllocPerm;
static unsigned aux_rows;
static void aux_fields(LEADER_BOARD *b,char ***names,long **values)
{
    char **n[]={&b->bestpk_name,&b->worstpk_name,&b->pk_name,&b->pd_name,
        &b->mk_name,&b->md_name,&b->tt_name,&b->qc_name};
    long *v[]={&b->bestpk_number,&b->worstpk_number,&b->pk_number,&b->pd_number,
        &b->mk_number,&b->md_number,&b->tt_number,&b->qc_number};
    for(unsigned i=0;i<8;i++){names[i]=n[i];values[i]=v[i];}
}
static void aux_drop_notes(void)
{
    for(unsigned i=0;i<MAX_BOARD;i++){
        NOTE_DATA *p=boards[i].note_first;
        while(p){NOTE_DATA *next=p->next;free_note(p);p=next;}
        boards[i].note_first=NULL;boards[i].changed=FALSE;
    }
}
static uint64_t aux_snapshot(int dump,uint64_t total,int leader)
{
    uint64_t value=UINT64_C(1469598103934665603);
    if(leader){
        char **names[8];long *values[8];aux_fields(&leader_board,names,values);
        for(unsigned i=0;i<8;i++){
            assert(*names[i]);value=hash(value,*names[i],strlen(*names[i])+1);
            value=hash(value,values[i],sizeof(long));
        }
    }else{
        unsigned count=0;
        for(NOTE_DATA *p=boards[1].note_first;p;p=p->next){
            assert(++count<=3);
            const char *fields[]={p->sender,p->date,p->to_list,p->subject,p->text};
            for(unsigned i=0;i<5;i++)value=hash(value,fields[i],strlen(fields[i])+1);
            value=hash(value,&p->date_stamp,sizeof p->date_stamp);
            value=hash(value,&p->expire,sizeof p->expire);
        }
        value=hash(value,&count,sizeof count);
        value=hash(value,&boards[1].changed,sizeof boards[1].changed);
    }
    if(dump)printf("%u,%d,%llu\n",aux_rows,leader,(unsigned long long)value);
    aux_rows++;return hash(total,&value,sizeof value);
}
static void aux_note(FILE *fp,unsigned n,int expired,int format)
{
    const char *keys[2][7]={{"Sender","Date","Stamp","Expire","To","Subject","Text"},
        {"'sender'","DATE","stamp","EXPIRE","'to'","subject","TEXT"}};
    const char **k=keys[format&1];
    assert(fprintf(fp,"%s Player%u~\n%s fixed date~\n%s %u\n%s %u\n"
        "%s all~\n%s subject %u~\n%s\nbody %u%s~\n\n",k[0],n,k[1],k[2],
        n+101,k[3],expired?99:1000,k[4],k[5],n,k[6],n,
        format&2?"\nsecond line":"")>0);
}
static uint64_t aux_native(int dump)
{
    uint64_t total=UINT64_C(1469598103934665603);aux_rows=0;
    for(unsigned mask=0;mask<8;mask++)for(unsigned form=0;form<4;form++){
        aux_drop_notes();FILE *fp=fopen("../notes/General","w");assert(fp);
        for(unsigned i=0;i<3;i++)aux_note(fp,i,(mask>>i)&1,form);
        assert(fclose(fp)==0);load_boards();
        total=aux_snapshot(dump,total,0);
        save_notes();aux_drop_notes();load_boards();
        total=aux_snapshot(dump,total,0);
    }
    const long values[]={LONG_MIN,-1000000,-100,-1,0,1,2,3,10,100,1000,1000000,LONG_MAX};
    char **names[8];long *numbers[8];aux_fields(&leader_board,names,numbers);
    for(unsigned v=0;v<13;v++)for(unsigned rotate=0;rotate<8;rotate++){
        for(unsigned i=0;i<8;i++){
            char name[40];snprintf(name,sizeof name,"Player%u",i+rotate);
            free_string(*names[i]);*names[i]=str_dup(name);
            *numbers[i]=values[(v+i)%13];
        }
        save_leaderboard();board_names(0);load_leaderboard();
        total=aux_snapshot(dump,total,1);
    }
    assert(aux_rows==168);
    assert(total==UINT64_C(13354354163155205809));
    if(!dump)printf("AUX_NATIVE_ROWS=%u HASH=%llu\n",aux_rows,
                   (unsigned long long)total);
    return total;
}
static uint64_t aux_file_hash(const char *path)
{
    FILE *fp=fopen(path,"rb");assert(fp);unsigned char bytes[2048];size_t n;
    uint64_t value=UINT64_C(1469598103934665603);
    while((n=fread(bytes,1,sizeof bytes,fp)))value=hash(value,bytes,n);
    assert(!ferror(fp)&&fclose(fp)==0);return value;
}
static void aux_bad(unsigned kind)
{
    const int leader=kind>=10;const char *path=leader?"../area/leader.txt":"../notes/General";
    aux_drop_notes();metrics_fixture();current_time=100;
    NOTE_DATA *saved=new_note();saved->sender=str_dup("Saved");
    saved->date=str_dup("date");saved->to_list=str_dup("all");
    saved->subject=str_dup("keep");saved->text=str_dup("keep");
    saved->date_stamp=50;saved->expire=1000;boards[1].note_first=saved;
    FILE *fp=fopen(path,"wb");assert(fp);
    if(!leader){
        if(kind==0)assert(fputs("Sender unfinished",fp)>=0);
        if(kind==1)assert(fputs("Sender one~\nDate",fp)>=0);
        if(kind==2)assert(fputs("Sender one~\nDate day~\nStamp +",fp)>=0);
        if(kind==3)assert(fputs("Sender one~\nDate day~\nStamp 1\nExpire 999999999999999999999999999",fp)>=0);
        if(kind==4){for(unsigned i=0;i<MAX_INPUT_LENGTH+1;i++)assert(fputc('K',fp)!=EOF);}
        if(kind==5)assert(fwrite("Sender A\0B~",1,11,fp)==11);
        if(kind==6){assert(fputs("Sender ",fp)>=0);for(unsigned i=0;i<MAX_INPUT_LENGTH;i++)assert(fputc('N',fp)!=EOF);assert(fputc('~',fp)!=EOF);}
        if(kind==7){aux_note(fp,0,1,0);assert(fputs("Sender broken~\nWrong key",fp)>=0);}
        if(kind==8)assert(fputs("Sender A~\nDate d~\nStamp 1\nExpire 1000\nTo all~\nTo duplicate~\nText body~\n",fp)>=0);
        if(kind==9){assert(fputs("Sender A~\nDate d~\nStamp 1\nExpire 1000\nTo all~\nSubject x~\nText ",fp)>=0);for(unsigned i=0;i<MAX_NOTE_TEXT+1;i++)assert(fputc('T',fp)!=EOF);assert(fputc('~',fp)!=EOF);}
    }else{
        if(kind==10)assert(fputs("unfinished name",fp)>=0);
        if(kind==11)assert(fputs("Name~\n1\n",fp)>=0);
        if(kind==12)assert(fputs("Name~\n999999999999999999999999999999999999\n",fp)>=0);
        if(kind==13)assert(fwrite("A\0B~\n1\n",1,7,fp)==7);
        if(kind==14){for(unsigned i=0;i<MAX_INPUT_LENGTH;i++)assert(fputc('N',fp)!=EOF);assert(fputs("~\n1\n",fp)>=0);}
        if(kind==15){for(unsigned i=0;i<8;i++)assert(fputs("Name~\n1\n",fp)>=0);assert(fputs("extra~\n2\n",fp)>=0);}
        if(kind==16)assert(fputs("Name~\n1|",fp)>=0);
    }
    assert(fclose(fp)==0);uint64_t before=aux_file_hash(path);
    uint64_t state=aux_snapshot(0,0,leader);
    if(leader)load_leaderboard();else load_boards();
    assert(aux_snapshot(0,0,leader)==state);
    assert(aux_file_hash(path)==before);
#ifndef AUX_READERS_BASELINE
    if(leader){save_leaderboard();assert(aux_file_hash(path)==before);}
    else {
        assert(boards[1].load_failed);
        boards[1].changed=TRUE;save_notes();assert(aux_file_hash(path)==before);
        NOTE_DATA *draft=new_note();assert(!finish_note(&boards[1],draft));free_note(draft);
    }
#endif
    if(leader){unlink(path);load_leaderboard();assert(leader_board.pk_name);save_leaderboard();}
    else {
        fp=fopen(path,"w");assert(fp);aux_note(fp,1,0,0);assert(fclose(fp)==0);
        aux_drop_notes();load_boards();assert(boards[1].note_first);
#ifndef AUX_READERS_BASELINE
        assert(!boards[1].load_failed);
#endif
    }
}
#ifndef AUX_READERS_BASELINE
static unsigned aux_descriptors(void)
{
    DIR *dir=opendir("/proc/self/fd");assert(dir);unsigned count=0;
    while(readdir(dir))count++;
    assert(closedir(dir)==0);return count;
}
static void aux_recovery(void)
{
    /* Boot failures must stay local even with the legacy fatal-reader flag set. */
    fBootDb=TRUE;aux_bad(3);aux_bad(12);fBootDb=FALSE;
    aux_drop_notes();
    FILE *fp=fopen("../notes/General","w");assert(fp);
    assert(fputs("Wrong key\n",fp)>=0&&fclose(fp)==0);
    unsigned descriptors=aux_descriptors();
    for(unsigned i=0;i<64;i++){load_boards();assert(boards[1].load_failed);}
    assert(aux_descriptors()==descriptors);
    /* A directory can open successfully but fails on read. */
    assert(mkdir("../notes/Personal",0700)==0);load_boards();
    assert(boards[4].load_failed);assert(rmdir("../notes/Personal")==0);
    unlink("../area/leader.txt");assert(mkdir("../area/leader.txt",0700)==0);
    load_leaderboard();save_leaderboard();
    assert(rmdir("../area/leader.txt")==0);load_leaderboard();
    assert(leader_board.pk_name&&!strcmp(leader_board.pk_name,"Noone"));
    /* All eight replacement names must be released on successful reload. */
    for(unsigned i=0;i<3;i++)load_leaderboard();
    long allocations=nAllocPerm;
    for(unsigned i=0;i<128;i++){load_leaderboard();assert(nAllocPerm==allocations);}
    /* A maximum-length legitimate editor body must survive and display fully. */
    fp=fopen("../notes/General","w");assert(fp);
    assert(fputs("Sender Writer~\nDate fixed date~\nStamp 101\nExpire 1000\n"
                 "To all~\nSubject maximum body~\nText ",fp)>=0);
    for(unsigned i=0;i<MAX_NOTE_TEXT;i++)assert(fputc('Z',fp)!=EOF);
    assert(fputs("~\n",fp)>=0&&fclose(fp)==0);
    load_boards();assert(!boards[1].load_failed);
    assert(strlen(boards[1].note_first->text)==MAX_NOTE_TEXT);
    pc.board=&boards[1];desc.outtop=0;output[0]=0;
    do_note(&actor,"read 1");
    char *body=strchr(output,'Z');assert(body);
    /* The engine appends its ordinary ANSI reset after each output chunk. */
    unsigned length=0;for(const char *p=body;*p;p++)if(*p=='Z')length++;
    assert(length==MAX_NOTE_TEXT);
    /* Complete reload replaces old records; it must not append/cycle/leak FDs. */
    for(unsigned i=0;i<64;i++){load_boards();assert(!boards[1].note_first->next);}
    assert(aux_descriptors()==descriptors);
    aux_drop_notes();
    /* Denied posts keep the editor draft, without broadcasting a false post. */
    boards[1].load_failed=TRUE;pc.board=&boards[1];
    pc.in_progress=new_note();NOTE_DATA *draft=pc.in_progress;
    desc.connected=CON_NOTE_FINISH;desc.outtop=0;output[0]=0;
    handle_con_note_finish(&desc,"p");
    assert(pc.in_progress==draft&&desc.connected==CON_NOTE_FINISH);
    assert(strstr(output,"draft has been kept")&&!strstr(output,"Note posted"));
    free_note(pc.in_progress);pc.in_progress=NULL;
    boards[1].load_failed=FALSE;
}
#endif
int main(int argc,char **argv)
{
    sandbox_begin();assert(mkdir("../notes",0700)==0);
    metrics_fixture();current_time=100;init_mm();fBootDb=FALSE;
    if(argc>2&&!strcmp(argv[1],"--boundary")){alarm(3);aux_bad((unsigned)atoi(argv[2]));alarm(0);}
    else {
        aux_native(argc>1&&!strcmp(argv[1],"--native-dump"));
#ifndef AUX_READERS_BASELINE
        if(argc==1){for(unsigned n=0;n<17;n++)aux_bad(n);aux_recovery();}
#endif
    }
    aux_drop_notes();board_names(0);unlink("../notes/General");
    assert(rmdir("../notes")==0);sandbox_end();return 0;
}
