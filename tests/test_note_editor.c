/* Private-world real-engine note editor and descriptor lifecycle regression. */
#define RESTART_FIXTURE_ONLY
#include "test_restart.c"
extern NOTE_DATA *new_note(void);
static unsigned checks;
static NOTE_DATA *draft(CHAR_DATA *ch,const char *text)
{
    NOTE_DATA *n=new_note();
    n->sender=str_dup(ch->name);n->date=str_dup("fixed date");
    n->to_list=str_dup("all");n->subject=str_dup("Draft");
    n->expire=current_time+86400;n->text=text?str_dup(text):NULL;
    ch->pcdata->board=&boards[1];ch->pcdata->in_progress=n;
    SET_BIT(ch->pcdata->flag,FLAG_AFK);
    return n;
}
static void drop_boards(void)
{
    for(unsigned i=0;i<MAX_BOARD;i++){
        while(boards[i].note_first){
            NOTE_DATA *n=boards[i].note_first;boards[i].note_first=n->next;
            free_note(n);
        }
        boards[i].changed=boards[i].load_failed=FALSE;
    }
}
static void release_owned(void)
{
    CHAR_DATA *ch=player("Draftfree",FALSE);
    NOTE_DATA *n=draft(ch,"private");
    extract_char(ch,TRUE);
    assert(note_free==n);
    NOTE_DATA *reused=new_note();assert(reused==n);
    assert(!reused->text&&!reused->sender&&!reused->next);
    free_note(reused);checks++;
}
static void capacity(void)
{
    CHAR_DATA *ch=player("Draftcap",TRUE);DESCRIPTOR_DATA *d=ch->desc;
    char *body=malloc(MAX_NOTE_TEXT+1);assert(body);
    memset(body,'x',MAX_NOTE_TEXT);body[MAX_NOTE_TEXT]=0;
    NOTE_DATA *n=draft(ch,body);char *owned=n->text;
    d->connected=CON_NOTE_TEXT;handle_con_note_text(d,"x");
    assert(ch->pcdata->in_progress==n&&n->text==owned);
    assert(strlen(n->text)==MAX_NOTE_TEXT&&d->connected==CON_NOTE_TEXT);
    assert(IS_SET(ch->pcdata->flag,FLAG_AFK));checks++;
    /* Count CRLF and raw colour bytes, not rendered character count. */
    free_string(n->text);body[MAX_NOTE_TEXT-3]=0;n->text=str_dup(body);
    handle_con_note_text(d,"x");assert(strlen(n->text)==MAX_NOTE_TEXT);
    owned=n->text;handle_con_note_text(d,"#Rx");
    assert(n->text==owned&&strlen(n->text)==MAX_NOTE_TEXT);checks++;
    char *oversized=malloc(MAX_STRING_LENGTH+1);assert(oversized);
    memset(oversized,'x',MAX_STRING_LENGTH);oversized[MAX_STRING_LENGTH]=0;
    handle_con_note_text(d,oversized);assert(n->text==owned);checks++;
    free(oversized);free(body);clean();
}
static void posting(void)
{
    CHAR_DATA *ch=player("Draftpost",TRUE);DESCRIPTOR_DATA *d=ch->desc;
    NOTE_DATA *n=draft(ch,"body\r\n");d->connected=CON_NOTE_FINISH;
    ch->level=boards[1].write_level-1;
    handle_con_note_finish(d,"p");
    assert(!boards[1].note_first&&ch->pcdata->in_progress==n);
    assert(d->connected==CON_NOTE_FINISH);checks++;
    ch->level=LEV_IMP;atomic_save_test_fault=SAVE_FAULT_RENAME;
    handle_con_note_finish(d,"p");
    assert(!boards[1].note_first&&ch->pcdata->in_progress==n);checks++;
    atomic_save_test_fault=SAVE_FAULT_NONE;handle_con_note_finish(d,"p");
    assert(boards[1].note_first==n&&!n->next&&!ch->pcdata->in_progress);
    assert(d->connected==CON_PLAYING&&!IS_SET(ch->pcdata->flag,FLAG_AFK));
    /* Freeing the author must not recycle a published board-owned note. */
    clean();assert(boards[1].note_first==n&&!strcmp(n->text,"body\r\n"));
    checks++;drop_boards();
}
static void links(void)
{
    const int states[]={CON_NOTE_TO,CON_NOTE_SUBJECT,CON_NOTE_EXPIRE,
                       CON_NOTE_TEXT,CON_NOTE_FINISH};
    for(unsigned k=0;k<2;k++)for(unsigned i=0;i<5;i++){
        CHAR_DATA *ch=player("Draftlink",TRUE);DESCRIPTOR_DATA *d=ch->desc;
        NOTE_DATA *n=draft(ch,"unfinished\r\n");d->connected=states[i];
        if(k)close_socket2(d);else close_socket(d);
        assert(!ch->desc&&ch->pcdata->in_progress==n&&!boards[1].note_first);
        assert(!strcmp(n->text,"unfinished\r\n"));
        session(pair(),ch,CON_PLAYING);
        do_note(ch,"write");
        assert(ch->pcdata->in_progress==n&&ch->desc->connected==CON_NOTE_TEXT);
        /* A board switch must not silently redirect a preserved draft. */
        do_board(ch,"1");assert(ch->pcdata->board==&boards[1]);
        ch->desc->connected=CON_NOTE_FINISH;
        handle_con_note_finish(ch->desc,"f");
        assert(!ch->pcdata->in_progress&&note_free==n);
        clean();checks++;
    }
}
static void native_flow(void)
{
    CHAR_DATA *ch=player("Draftnative",TRUE);DESCRIPTOR_DATA *d=ch->desc;
    ch->pcdata->board=&boards[1];do_note(ch,"write");
    assert(d->connected==CON_NOTE_TO);
    handle_con_note_to(d,"");assert(d->connected==CON_NOTE_SUBJECT);
    handle_con_note_subject(d,"Unchanged subject");assert(d->connected==CON_NOTE_EXPIRE);
    handle_con_note_expire(d,"1");assert(d->connected==CON_NOTE_TEXT);
    handle_con_note_text(d,"First line");handle_con_note_text(d,"");
    assert(!strcmp(ch->pcdata->in_progress->text,"First line\r\n\r\n"));
    handle_con_note_text(d,"END");assert(d->connected==CON_NOTE_FINISH);
    handle_con_note_finish(d,"v");handle_con_note_finish(d,"c");
    assert(d->connected==CON_NOTE_TEXT);
    handle_con_note_text(d,"Second line");handle_con_note_text(d,"~");
    NOTE_DATA *n=ch->pcdata->in_progress;
    handle_con_note_finish(d,"p");
    assert(boards[1].note_first==n&&!strcmp(n->subject,"Unchanged subject"));
    assert(!strcmp(n->text,"First line\r\n\r\nSecond line\r\n"));
    /* DEF_INCLUDE preserves the legacy leading space on a default recipient. */
    assert(!strcmp(n->to_list," all")&&!strcmp(n->sender,"Draftnative"));
    clean();drop_boards();checks++;
}
int main(int argc,char **argv)
{
    char cwd[4096],root[]="/tmp/anathema-note-editor-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&chdir(root)==0);
    const char *dirs[]={"player","backup","notes","run"};
    for(unsigned i=0;i<4;i++)assert(mkdir(dirs[i],0700)==0);
    assert(chdir("run")==0);globals();
    if(argc>1&&!strcmp(argv[1],"--release"))release_owned();
    else if(argc>1&&!strcmp(argv[1],"--capacity"))capacity();
    else if(argc>1&&!strcmp(argv[1],"--permission"))posting();
    else if(argc>1&&!strcmp(argv[1],"--native"))native_flow();
    else{release_owned();capacity();posting();links();native_flow();}
    clean();drop_boards();fclose(fpReserve);assert(chdir("..")==0);
    for(unsigned i=0;i<4;i++)remove_directory(dirs[i]);
    assert(chdir(cwd)==0&&rmdir(root)==0);
    printf("NOTE_EDITOR_CHECKS=%u\n",checks);return 0;
}
