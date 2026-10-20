/* Full-engine administrative config reads, only in a disposable directory. */
#define FILE_NUMBERS_FIXTURE_ONLY
#include "test_file_numbers.c"
#include <dirent.h>
#include <signal.h>
extern bool check_disabled(const struct cmd_type *command);
#ifndef ADMIN_CONFIG_BASELINE
#include "admin_config.h"
#endif
static unsigned checks;
static void put(const char *path,const void *data,size_t length)
{
    FILE *fp=fopen(path,"wb");assert(fp);
    assert(fwrite(data,1,length,fp)==length&&fclose(fp)==0);
}
static void textfile(const char *path,const char *text){put(path,text,strlen(text));}
static void clear_disabled(void)
{
    while(disabled_first){
        DISABLED_DATA *p=disabled_first;disabled_first=p->next;
        free_string(p->disabled_by);free_mem(p,sizeof *p);
    }
}
static void clear_bans(void)
{
    while(ban_list){
        BAN_DATA *p=ban_list;ban_list=p->next;free_string(p->name);p->name=NULL;
        p->next=ban_free;ban_free=p;
    }
}
static uint64_t snapshot(int disabled)
{
    uint64_t h=UINT64_C(1469598103934665603);
    if(disabled)for(DISABLED_DATA *p=disabled_first;p;p=p->next){
        h=hash(h,p->command->name,strlen(p->command->name));
        h=hash(h,&p->level,sizeof p->level);
        h=hash(h,p->disabled_by,strlen(p->disabled_by));
    }else for(BAN_DATA *p=ban_list;p;p=p->next)h=hash(h,p->name,strlen(p->name));
    return h;
}
static void ac_native(int dump)
{
    const char *commands[]={"help","look","chat","score"};
    for(unsigned n=0;n<128;n++){
        char records[2048]={0},line[200];
        clear_disabled();clear_bans();
        for(unsigned j=0;j<1+n%4;j++){
            snprintf(line,sizeof line,"%s %u %s\n",commands[j],(n+j)%8,
                     n%2?"Luto":"Staff");strcat(records,line);
        }
        strcat(records,"END\n");textfile(DISABLED_FILE,records);load_disabled();
        uint64_t first=snapshot(1);
        records[0]=0;
        for(unsigned j=0;j<1+n%4;j++){
            snprintf(line,sizeof line,"host%u-%u.example%s\n",n,j,n%2?" comment":"");
            strcat(records,line);
        }
        textfile("ban.txt",records);load_bans();
#ifdef ADMIN_CONFIG_BASELINE
        /* The legacy reader left the final next pointer uninitialized.
         * Characterize record content/order only, not that invalid pointer.
         * The real-engine fixed run has NO such repair. */
        BAN_DATA *last=ban_list;
        for(unsigned j=1;j<1+n%4;j++)last=last->next;
        last->next=NULL;
#endif
        uint64_t second=snapshot(0);
        if(dump)printf("%u,%llu,%llu\n",n,(unsigned long long)first,(unsigned long long)second);
        checks+=2;
    }
    clear_disabled();clear_bans();
}
#ifndef ADMIN_CONFIG_BASELINE
static unsigned fds(void)
{
    DIR *d=opendir("/proc/self/fd");assert(d);unsigned n=0;
    while(readdir(d))n++;assert(closedir(d)==0);return n;
}
static void clean_policy(void)
{
    textfile(DISABLED_FILE,"help 7 Luto\nEND\n");
    textfile("ban.txt","old.example\n");
    load_disabled();load_bans();assert(admin_configuration_healthy());
}
static void rejected(int disabled,const void *bad,size_t length)
{
    clean_policy();
    uint64_t before=snapshot(disabled);void *first=disabled?(void*)disabled_first:(void*)ban_list;
    const char *path=disabled?DISABLED_FILE:"ban.txt";put(path,bad,length);
    unsigned count=fds();
    if(disabled)load_disabled();else load_bans();
    assert(fds()==count&&snapshot(disabled)==before);
    assert(first==(disabled?(void*)disabled_first:(void*)ban_list));
    assert(!(disabled?admin_disabled_ready():admin_bans_ready()));
    /* Quarantine even blocks the old writer's empty-list unlink path. */
    if(disabled)save_disabled();else save_bans();
    FILE *fp=fopen(path,"rb");assert(fp);unsigned char data[4096];size_t offset=0,n;
    while((n=fread(data,1,sizeof data,fp))){
        assert(offset+n<=length&&!memcmp(data,(const char*)bad+offset,n));offset+=n;
    }
    assert(offset==length&&fclose(fp)==0);checks++;
}
static void cases(void)
{
    const char *disabled[]={"","help 7 Luto\n","help x Luto\nEND\n","help 8 Luto\nEND\n",
        "help -1 Luto\nEND\n","help 7\nEND\n","help 7 ''\nEND\n",
        "help 7 'Two Words'\nEND\n","END trailing\n","help 999999999999999999999 Luto\nEND\n"};
    for(unsigned i=0;i<sizeof disabled/sizeof *disabled;i++)rejected(1,disabled[i],strlen(disabled[i]));
    const char *bans[]={"''\n","'bad host'\n","'unclosed\n"};
    for(unsigned i=0;i<sizeof bans/sizeof *bans;i++)rejected(0,bans[i],strlen(bans[i]));
    const char nul[]="first.example\0second.example\n";rejected(0,nul,sizeof nul-1);
    char large[MAX_INPUT_LENGTH+20];memset(large,'a',sizeof large);large[sizeof large-1]='\n';
    rejected(0,large,sizeof large);rejected(1,large,sizeof large);
    char *oversize=malloc(262145);assert(oversize);memset(oversize,' ',262145);
    rejected(0,oversize,262145);rejected(1,oversize,262145);free(oversize);
    char many[20000]={0};
    for(unsigned i=0;i<1025;i++)strcat(many,"x.example\n");
    rejected(0,many,strlen(many));
    many[0]=0;for(unsigned i=0;i<1025;i++)strcat(many,"help 7 Luto\n");
    strcat(many,"END\n");rejected(1,many,strlen(many));
    for(unsigned which=0;which<2;which++)for(unsigned kind=0;kind<3;kind++){
        clean_policy();const char *path=which?DISABLED_FILE:"ban.txt";
        uint64_t before=snapshot(which);assert(unlink(path)==0);
        if(kind==0)assert(mkdir(path,0700)==0);
        else if(kind==1)assert(symlink("missing-target",path)==0);
        else assert(mkfifo(path,0600)==0);
        alarm(3);if(which)load_disabled();else load_bans();alarm(0);
        assert(snapshot(which)==before&&!(which?admin_disabled_ready():admin_bans_ready()));
        if(kind==0)assert(rmdir(path)==0);else assert(unlink(path)==0);checks++;
    }
    clean_policy();unsigned before=fds();
    for(unsigned i=0;i<128;i++){load_bans();load_disabled();}
    assert(fds()==before&&admin_configuration_healthy());checks++;
    /* Empty/missing ban policy really means no bans, including after reload. */
    textfile("ban.txt","");load_bans();assert(!ban_list&&admin_bans_ready());
    unlink("ban.txt");load_bans();assert(!ban_list&&admin_bans_ready());checks+=2;
    unlink(DISABLED_FILE);load_disabled();assert(!disabled_first&&admin_disabled_ready());checks++;
    /* Unknown commands are still skipped; aliases still share do_fun policy. */
    textfile(DISABLED_FILE,"notarealcommand 7 Luto\nchat 7 Luto\nEND\n");load_disabled();
    assert(admin_disabled_ready()&&disabled_first&&!disabled_first->next);
    for(unsigned i=0;cmd_table[i].name[0];i++)if(!strcmp(cmd_table[i].name,"."))assert(check_disabled(&cmd_table[i]));
    checks++;
    /* Mutating commands refuse a rejected policy, but listing is safe. */
    clean_policy();textfile("ban.txt","''\n");load_bans();
    uint64_t b=snapshot(0);desc.outtop=0;output[0]=0;
    do_ban(&actor,"new.example");do_allow(&actor,"old.example");
    assert(snapshot(0)==b&&strstr(output,"read-only"));checks++;
    textfile(DISABLED_FILE,"help 7");load_disabled();b=snapshot(1);
    desc.outtop=0;output[0]=0;do_disable(&actor,"look");
    assert(snapshot(1)==b&&strstr(output,"read-only"));checks++;
    /* Multi-kilobyte lists and long disabler labels do not overflow displays. */
    clean_policy();FILE *fp=fopen("ban.txt","w");assert(fp);
    for(unsigned i=0;i<100;i++)fprintf(fp,"%080u.example\n",i);
    assert(fclose(fp)==0);load_bans();desc.outtop=0;output[0]=0;
    do_ban(&actor,"");assert(desc.outtop>MAX_STRING_LENGTH&&strstr(output,"Banned sites:"));checks++;
    char by[MAX_INPUT_LENGTH-1];memset(by,'z',sizeof by-1);by[sizeof by-1]=0;
    fp=fopen(DISABLED_FILE,"w");assert(fp);fprintf(fp,"help 7 %s\nEND\n",by);assert(fclose(fp)==0);
    load_disabled();assert(admin_disabled_ready());desc.outtop=0;output[0]=0;
    do_disable(&actor,"");assert(strstr(output,by));checks++;
    /* Mutation cannot create records the bounded reader would later reject. */
    clean_policy();assert(!admin_ban_can_add("two words")&&!admin_ban_can_add(NULL));
    fp=fopen("ban.txt","w");assert(fp);
    for(unsigned i=0;i<1024;i++)fprintf(fp,"host%u.example\n",i);
    assert(fclose(fp)==0);load_bans();assert(admin_bans_ready());
    b=snapshot(0);desc.outtop=0;output[0]=0;do_ban(&actor,"next.invalid");
    assert(snapshot(0)==b&&strstr(output,"limit reached"));checks++;
    fp=fopen(DISABLED_FILE,"w");assert(fp);
    for(unsigned i=0;i<1024;i++)fprintf(fp,"help 7 Luto\n");
    fprintf(fp,"END\n");assert(fclose(fp)==0);load_disabled();
    assert(admin_disabled_ready());actor.level=LEV_ST;
    b=snapshot(1);desc.outtop=0;output[0]=0;do_disable(&actor,"look");
    assert(snapshot(1)==b&&strstr(output,"limit reached"));checks++;
    fp=fopen("ban.txt","w");assert(fp);
    char hostname[399];memset(hostname,'z',398);hostname[398]=0;
    for(unsigned i=0;i<657;i++){hostname[0]='a'+i%26;fprintf(fp,"%s\n",hostname);}
    assert(fclose(fp)==0);
    struct stat st;assert(stat("ban.txt",&st)==0&&st.st_size==262143);
    load_bans();assert(admin_bans_ready()&&!admin_ban_can_add("next.invalid"));
    assert(!admin_disabled_can_add(NULL,"Luto",7));checks+=2;
    puts("ADMIN_CONFIG_RELOAD_QUARANTINE_RECOVERY_PASSED");
}
#endif
int main(int argc,char **argv)
{
    sandbox_begin();metrics_fixture();init_mm();fBootDb=FALSE;
    ac_native(argc>1&&!strcmp(argv[1],"--native-dump"));
#ifndef ADMIN_CONFIG_BASELINE
    if(argc==1)cases();
#endif
    clear_disabled();clear_bans();board_names(0);
    unlink(DISABLED_FILE);unlink("ban.txt");sandbox_end();
    if(argc==1)printf("ADMIN_CONFIG_CHECKS=%u\n",checks);return 0;
}
