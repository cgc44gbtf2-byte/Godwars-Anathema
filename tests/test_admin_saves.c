/* Actual staff commands and policy writers, never live world/player files. */
#define ATOMIC_SAVE_TESTING
#define ADMIN_CONFIG_FIXTURE_ONLY
#include "test_admin_config.c"
#include <sys/resource.h>

static uint64_t file_snapshot(const char *name)
{
    FILE *fp=fopen(name,"rb");
    uint64_t h=UINT64_C(1469598103934665603);
    if(!fp){assert(errno==ENOENT);return hash(h,"absent",6);}
    unsigned char buf[1024];size_t n;
    while((n=fread(buf,1,sizeof buf,fp))!=0)h=hash(h,buf,n);
    assert(!ferror(fp)&&fclose(fp)==0);return h;
}
static void output_reset(void){desc.outtop=0;output[0]=0;}
static void command(int disabled,int remove)
{
    output_reset();
    if(disabled)do_disable(&actor,remove?"help":"look");
    else if(remove)do_allow(&actor,"old.example");
    else do_ban(&actor,"new.example");
}
static const char *policy_path(int disabled)
{
    return disabled?DISABLED_FILE:"ban.txt";
}
static unsigned temporaries(void)
{
    DIR *d=opendir(".");assert(d);struct dirent *e;unsigned n=0;
    while((e=readdir(d)))if(!strncmp(e->d_name,".save-",6))n++;
    assert(closedir(d)==0);return n;
}
static void as_native(int dump)
{
    uint64_t total=UINT64_C(1469598103934665603);
    for(unsigned n=0;n<96;n++){
        clean_policy();
        char first[64],second[64];
        snprintf(first,sizeof first,"first-%u.example",n);
        snprintf(second,sizeof second,"second-%u.example",n);
        for(unsigned step=0;step<10;step++){
            output_reset();
            switch(step){
            case 0:do_ban(&actor,first);break;
            case 1:do_ban(&actor,second);break;
            case 2:do_disable(&actor,"look");break;
            case 3:do_disable(&actor,"chat");break;
            case 4:do_disable(&actor,"look");break;
            case 5:do_allow(&actor,first);break;
            case 6:do_disable(&actor,"help");break;
            case 7:do_allow(&actor,second);break;
            case 8:do_disable(&actor,"chat");break;
            case 9:do_allow(&actor,"old.example");break;
            }
            uint64_t h=hash(UINT64_C(1469598103934665603),output,strlen(output));
            char row[256];
            int length=snprintf(row,sizeof row,"%u,%u,%llu,%llu,%llu,%llu,%llu\n",n,step,
                (unsigned long long)snapshot(0),(unsigned long long)snapshot(1),
                (unsigned long long)file_snapshot("ban.txt"),
                (unsigned long long)file_snapshot(DISABLED_FILE),(unsigned long long)h);
            assert(length>0&&(size_t)length<sizeof row);
            total=hash(total,row,(size_t)length);
            if(dump)fputs(row,stdout);
            checks++;
        }
    }
    assert(total==UINT64_C(12643009505926205027));
}
/* Kernel-generated EFBIG: old fopen("w") destroyed the valid policy and said
 * success. Both the disk bytes and the old live restriction must now survive. */
static void kernel_write_failure(int disabled)
{
    clean_policy();
    const char *name=policy_path(disabled);
    uint64_t disk=file_snapshot(name),live=snapshot(disabled);
    struct rlimit saved,limit;
    assert(getrlimit(RLIMIT_FSIZE,&saved)==0);
    limit=saved;limit.rlim_cur=0;
    void (*handler)(int)=signal(SIGXFSZ,SIG_IGN);
    assert(handler!=SIG_ERR&&setrlimit(RLIMIT_FSIZE,&limit)==0);
    command(disabled,0);
    assert(setrlimit(RLIMIT_FSIZE,&saved)==0);
    signal(SIGXFSZ,handler);clearerr(stderr);
    assert(file_snapshot(name)==disk&&snapshot(disabled)==live);
    assert(strstr(output,"not saved")||strstr(output,"save failed"));
    assert(fpReserve&&temporaries()==0);checks++;
}
#ifndef ADMIN_SAVES_BASELINE
static void failures(void)
{
    const enum atomic_save_fault faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,
        SAVE_FAULT_SYNC,SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(int disabled=0;disabled<2;disabled++){
        for(unsigned f=0;f<sizeof faults/sizeof *faults;f++){
            clean_policy();
            uint64_t disk=file_snapshot(policy_path(disabled)),live=snapshot(disabled);
            unsigned count=fds();
            atomic_save_test_fault=faults[f];command(disabled,0);
            atomic_save_test_fault=SAVE_FAULT_NONE;
            assert(file_snapshot(policy_path(disabled))==disk&&snapshot(disabled)==live);
            assert(strstr(output,"not saved")||strstr(output,"save failed"));
            assert(fpReserve&&fds()==count&&temporaries()==0);
            command(disabled,0);
            assert(snapshot(disabled)!=live);checks++;
        }
        /* Empty-policy deletion has its own checked, durable commit point. */
        clean_policy();uint64_t disk=file_snapshot(policy_path(disabled));
        uint64_t live=snapshot(disabled);
        atomic_save_test_fault=SAVE_FAULT_REMOVE;command(disabled,1);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(file_snapshot(policy_path(disabled))==disk&&snapshot(disabled)==live);
        assert(strstr(output,"not saved")||strstr(output,"save failed"));checks++;
        command(disabled,1);
        assert(access(policy_path(disabled),F_OK)!=0&&errno==ENOENT);
        assert(disabled?!disabled_first:!ban_list);checks++;
        /* A completed rename is visible, but a directory-fsync failure is
         * explicitly uncertain: retain the new live state and pause edits. */
        for(int remove=0;remove<2;remove++){
            clean_policy();uint64_t old=snapshot(disabled);
            atomic_save_test_fault=SAVE_FAULT_DIRSYNC;command(disabled,remove);
            atomic_save_test_fault=SAVE_FAULT_NONE;
            assert(snapshot(disabled)!=old&&strstr(output,"durability is uncertain"));
            assert(disabled?!admin_disabled_ready():!admin_bans_ready());
            uint64_t visible=file_snapshot(policy_path(disabled));
            command(disabled,0);assert(strstr(output,"read-only"));
            assert(file_snapshot(policy_path(disabled))==visible);
            if(disabled)load_disabled();else load_bans();
            assert(admin_configuration_healthy());checks++;
        }
        kernel_write_failure(disabled);
        /* A symlink (or wrong destination type) cannot redirect a save OR
         * deletion. Both active restrictions and the outside data survive. */
        for(int remove=0;remove<2;remove++){
            clean_policy();uint64_t old=snapshot(disabled);
            textfile("anchor","untouched\n");
            assert(unlink(policy_path(disabled))==0);
            assert(symlink("anchor",policy_path(disabled))==0);
            uint64_t anchor=file_snapshot("anchor");
            command(disabled,remove);
            struct stat st;
            assert(lstat(policy_path(disabled),&st)==0&&S_ISLNK(st.st_mode));
            assert(file_snapshot("anchor")==anchor&&snapshot(disabled)==old);
            assert(unlink(policy_path(disabled))==0&&unlink("anchor")==0);
            checks++;
        }
        clean_policy();disk=file_snapshot(policy_path(disabled));live=snapshot(disabled);
        assert(chmod(".",0770)==0);command(disabled,0);assert(chmod(".",0700)==0);
        assert(file_snapshot(policy_path(disabled))==disk&&snapshot(disabled)==live);
        assert(temporaries()==0);checks++;
        /* A rejected non-final removal must restore head, middle AND tail
         * links. It is a rename, not the empty-policy unlink path. */
        const char *names[2][3]={{"first.example","middle.example","last.example"},
                                {"help","look","chat"}};
        for(unsigned position=0;position<3;position++){
            clean_policy();
            textfile("ban.txt","first.example\nmiddle.example\nlast.example\n");
            textfile(DISABLED_FILE,"help 7 Luto\nlook 7 Luto\nchat 7 Luto\nEND\n");
            load_disabled();load_bans();
            disk=file_snapshot(policy_path(disabled));live=snapshot(disabled);
            void *head=disabled?(void*)disabled_first:(void*)ban_list;
            atomic_save_test_fault=SAVE_FAULT_RENAME;output_reset();
            if(disabled)do_disable(&actor,(char*)names[1][position]);
            else do_allow(&actor,(char*)names[0][position]);
            atomic_save_test_fault=SAVE_FAULT_NONE;
            assert(file_snapshot(policy_path(disabled))==disk&&snapshot(disabled)==live);
            assert(head==(disabled?(void*)disabled_first:(void*)ban_list));
            assert(strstr(output,"not saved")||strstr(output,"save failed"));checks++;
        }
        /* A previously absent file must also pass directory fsync before a
         * staff removal can be acknowledged as durable. */
        clean_policy();assert(unlink(policy_path(disabled))==0);
        atomic_save_test_fault=SAVE_FAULT_DIRSYNC;command(disabled,1);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(strstr(output,"durability is uncertain"));
        assert(disabled?!admin_disabled_ready():!admin_bans_ready());
        if(disabled)load_disabled();else load_bans();checks++;
    }
    /* Direct compatibility savers also refuse structurally unreadable data
     * and bounded cycles before creating any temporary output. */
    clean_policy();uint64_t old=file_snapshot("ban.txt");
    BAN_DATA *saved_bans=ban_list,bad_ban={0};
    bad_ban.name="two words";ban_list=&bad_ban;
    assert(admin_save_bans()==ATOMIC_SAVE_FAILED);ban_list=saved_bans;
    assert(file_snapshot("ban.txt")==old);checks++;
    bad_ban.name="cycle.example";bad_ban.next=&bad_ban;ban_list=&bad_ban;
    assert(admin_save_bans()==ATOMIC_SAVE_FAILED);ban_list=saved_bans;
    assert(file_snapshot("ban.txt")==old&&temporaries()==0);checks++;
    DISABLED_DATA *saved_disabled=disabled_first,bad_disabled=*disabled_first;
    old=file_snapshot(DISABLED_FILE);bad_disabled.level=LONG_MAX;disabled_first=&bad_disabled;
    assert(admin_save_disabled()==ATOMIC_SAVE_FAILED);disabled_first=saved_disabled;
    assert(file_snapshot(DISABLED_FILE)==old);checks++;
    /* Fixed basenames do not weaken the arbitrary-player-name API. */
    assert(!atomic_save_name_valid("ban.txt")&&!atomic_save_name_valid("disabled.txt"));
    struct atomic_save_error error;
    assert(mkdir("protected",0700)==0&&symlink("protected","alias")==0);
    textfile("protected/ban.txt","keep\n");
    assert(atomic_remove_bans("alias",&error)==ATOMIC_SAVE_FAILED);
    assert(atomic_remove_bans("alias/",&error)==ATOMIC_SAVE_FAILED);
    assert(access("protected/ban.txt",F_OK)==0);
    assert(unlink("protected/ban.txt")==0);
    assert(atomic_remove_bans("protected",&error)==ATOMIC_SAVE_OK);
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    assert(atomic_remove_bans("protected",&error)==ATOMIC_SAVE_UNCERTAIN);
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(unlink("alias")==0&&rmdir("protected")==0);
    checks++;
}
#endif
int main(int argc,char **argv)
{
    sandbox_begin();metrics_fixture();init_mm();fBootDb=FALSE;actor.level=LEV_ST;
    if(argc>1&&!strcmp(argv[1],"--red-ban"))kernel_write_failure(0);
    else if(argc>1&&!strcmp(argv[1],"--red-disabled"))kernel_write_failure(1);
    else{
        as_native(argc>1&&!strcmp(argv[1],"--native-dump"));
#ifndef ADMIN_SAVES_BASELINE
        if(argc==1)failures();
#endif
    }
    clear_disabled();clear_bans();board_names(0);
    unlink(DISABLED_FILE);unlink("ban.txt");sandbox_end();
    if(argc==1)printf("ADMIN_ATOMIC_SAVE_CHECKS=%u\n",checks);
    return 0;
}
