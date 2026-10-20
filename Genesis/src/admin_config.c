/* Bounded, transactional readers for administrative policy, not player data. */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "admin_config.h"
#include "file_number.h"
#include "file_word.h"

#define ADMIN_RECORD_LIMIT 1024
#define ADMIN_FILE_LIMIT 262144
static bool disabled_ready=TRUE,bans_ready=TRUE;
bool admin_disabled_ready(void){return disabled_ready;}
bool admin_bans_ready(void){return bans_ready;}
bool admin_configuration_healthy(void){return disabled_ready&&bans_ready;}

static FILE *config_open(const char *path,bool *missing)
{
    struct stat st;
    *missing=FALSE;
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
    if(fd<0){*missing=errno==ENOENT;return NULL;}
    if(fstat(fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size>ADMIN_FILE_LIMIT){
        close(fd);return NULL;
    }
    FILE *fp=fdopen(fd,"r");
    if(!fp)close(fd);
    return fp;
}
static bool plain_token(const char *word)
{
    if(!word[0])return FALSE;
    for(const unsigned char *p=(const unsigned char *)word;*p;p++)
        if(isspace(*p)||iscntrl(*p))return FALSE;
    return TRUE;
}
static bool field(const char *word)
{
    return word && plain_token(word) && strlen(word)<MAX_INPUT_LENGTH;
}
static bool charge(size_t *used,size_t count)
{
    if(count>ADMIN_FILE_LIMIT-*used)return FALSE;
    *used+=count;return TRUE;
}
bool admin_ban_can_add(const char *name)
{
    if(!bans_ready || !field(name))return FALSE;
    size_t bytes=strlen(name)+1;unsigned records=1;
    for(BAN_DATA *p=ban_list;p;p=p->next)
        if(++records>ADMIN_RECORD_LIMIT || !field(p->name)
            || !charge(&bytes,strlen(p->name)+1))return FALSE;
    return TRUE;
}
static bool disabled_charge(size_t *bytes,const struct cmd_type *command,
                            const char *by,long level)
{
    if(!command || !field(command->name) || !field(by) || level<0
        || level>MAX_LEVEL)return FALSE;
    size_t digits=1;
    for(long remaining=level;remaining>=10;remaining/=10)digits++;
    return charge(bytes,strlen(command->name)+strlen(by)+digits+3);
}
bool admin_disabled_can_add(const struct cmd_type *command,const char *by,long level)
{
    size_t bytes=4;unsigned records=1; /* END plus newline. */
    if(!disabled_ready || !disabled_charge(&bytes,command,by,level))return FALSE;
    for(DISABLED_DATA *p=disabled_first;p;p=p->next)
        if(++records>ADMIN_RECORD_LIMIT
            || !disabled_charge(&bytes,p->command,p->disabled_by,p->level))return FALSE;
    return TRUE;
}
static void disabled_drop(DISABLED_DATA *head)
{
    while(head){
        DISABLED_DATA *next=head->next;
        free_string(head->disabled_by);free_mem(head,sizeof *head);head=next;
    }
}
/* Ban nodes share the existing mixed permanent/recycled pool used by ban and
 * allow. Never pass an old permanent node to free_mem. */
static void bans_drop(BAN_DATA *head)
{
    while(head){
        BAN_DATA *next=head->next;
        free_string(head->name);head->name=NULL;
        head->next=ban_free;ban_free=head;head=next;
    }
}
static BAN_DATA *ban_node(void)
{
    BAN_DATA *p;
    if(ban_free){p=ban_free;ban_free=ban_free->next;}
    else p=alloc_perm(sizeof *p);
    p->name=NULL;p->next=NULL;return p;
}
void load_disabled(void)
{
    DISABLED_DATA *staged=NULL;
    bool missing,good=TRUE,ended=FALSE;
    unsigned records=0,unknown=0;
    FILE *fp=config_open(DISABLED_FILE,&missing);
    if(!fp && !missing)good=FALSE;
    while(fp && good && !ended){
        char command[MAX_INPUT_LENGTH],by[MAX_INPUT_LENGTH];long level;
        if(fread_word_checked(fp,command,sizeof command)!=FILE_WORD_OK){good=FALSE;break;}
        if(!str_cmp(command,"END")){
            ended=TRUE;
            good=fread_word_checked(fp,command,sizeof command)==FILE_WORD_EOF;
            break;
        }
        if(++records>ADMIN_RECORD_LIMIT
            || fread_number_checked(fp,&level)!=FILE_NUMBER_OK
            || level<0 || level>MAX_LEVEL
            || fread_word_checked(fp,by,sizeof by)!=FILE_WORD_OK
            || !plain_token(by)){good=FALSE;break;}
        unsigned i;
        for(i=0;cmd_table[i].name[0];i++)
            if(!str_cmp(cmd_table[i].name,command))break;
        if(!cmd_table[i].name[0]){unknown++;continue;}
        DISABLED_DATA *p=alloc_mem(sizeof *p);
        p->command=&cmd_table[i];p->level=level;p->disabled_by=str_dup(by);
        p->next=staged;staged=p; /* Preserve legacy prepend/reverse ordering. */
    }
    if(fp && (ferror(fp) || !ended))good=FALSE;
    if(fp && fclose(fp)!=0)good=FALSE;
    if(!good){
        disabled_drop(staged);disabled_ready=FALSE;
        log_string("disabled.txt rejected; previous command policy retained; writes blocked");
        return;
    }
    DISABLED_DATA *old=disabled_first;disabled_first=staged;
    disabled_drop(old);disabled_ready=TRUE;
    if(unknown)log_string("Unknown disabled commands skipped after a valid configuration read");
}
void load_bans(void)
{
    BAN_DATA *staged=NULL,**tail=&staged;
    bool missing,good=TRUE;
    unsigned records=0;
    FILE *fp=config_open("ban.txt",&missing);
    if(!fp && !missing)good=FALSE;
    while(fp && good){
        char name[MAX_INPUT_LENGTH];
        enum file_word_status status=fread_word_checked(fp,name,sizeof name);
        if(status==FILE_WORD_EOF)break;
        if(status!=FILE_WORD_OK || !plain_token(name) || ++records>ADMIN_RECORD_LIMIT){
            good=FALSE;break;
        }
        /* Legacy ban files use the first word on a line and ignore its comment.
         * Bound that skip too, rather than using the fatal legacy reader. */
        unsigned skipped=0;int c;
        while((c=getc(fp))!=EOF && c!='\n' && c!='\r'){
            if(c==0 || ++skipped>FILE_WORD_MAX_READS){good=FALSE;break;}
        }
        if(!good)break;
        BAN_DATA *p=ban_node();p->name=str_dup(name);
        *tail=p;tail=&p->next;
    }
    if(fp && ferror(fp))good=FALSE;
    if(fp && fclose(fp)!=0)good=FALSE;
    if(!good){
        bans_drop(staged);bans_ready=FALSE;
        log_string("ban.txt rejected; previous ban policy retained; writes blocked");
        return;
    }
    BAN_DATA *old=ban_list;ban_list=staged;
    bans_drop(old);bans_ready=TRUE;
}
