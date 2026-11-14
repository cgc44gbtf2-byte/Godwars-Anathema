/* Tilde strings and real saved records in a disposable world, never live data. */
#define FILE_WORDS_FIXTURE_ONLY
#include "test_file_words.c"
#ifndef FILE_STRINGS_BASELINE
#include "file_string.h"
#endif
static unsigned string_rows;
static uint64_t string_row(uint64_t total,int dump,const char *text,int next)
{
    uint64_t h=hash(UINT64_C(1469598103934665603),text,strlen(text));
    h=hash(h,&next,sizeof next);string_rows++;
    if(dump)printf("%u,%llu\n",string_rows,(unsigned long long)h);
    return hash(total,&h,sizeof h);
}
static uint64_t strings_native(int dump)
{
    const char *values[]={"","alpha","two words","A\nB","A\rB","A\r\nB",
                         "A\n\nB","A\tB","#rRed#n","'quoted'","\"double\"","\377X"};
    const char *spaces[]={""," ","\t","\r\n"};
    const char *ends[]={""," ","\nNEXT"};
    uint64_t total=UINT64_C(1469598103934665603);string_rows=0;
    for(unsigned v=0;v<12;v++)for(unsigned lead=0;lead<4;lead++)
    for(unsigned end=0;end<3;end++){
        char text[128];snprintf(text,sizeof text,"%s%s~%s",spaces[lead],values[v],ends[end]);
        FILE *fp=input(text);char *value=fread_string(fp);int next=fgetc(fp);
        total=string_row(total,dump,value,next);free_string(value);assert(fclose(fp)==0);
    }
    for(unsigned shorter=0;shorter<2;shorter++)for(unsigned n=0;n<12;n++){
        char fields[512];snprintf(fields,sizeof fields,
            "Description %s~\nTitle #rTitle%u#n~\nEmail fixture%u~\n"
            "Createtime fixed~\nLasttime fixed~\nLasthost private~\nMarriage none~",
            values[n],n,n);
        player_fixture(fields);CHAR_DATA *ch=read_player(shorter);
        assert(!ch->pcdata->player_load_failed);
        total=string_row(total,dump,ch->description,0);
        total=string_row(total,dump,ch->pcdata->title,1);drop_player(ch);
    }
    for(unsigned n=0;n<12;n++){
        char fields[512];snprintf(fields,sizeof fields,
            "Title title%u~\nEmail fixture%u~\nCreatetime fixed~\n"
            "Lasttime fixed~\nLasthost private~\nMarriage none~",n,n);
        player_fixture(fields);desc.outtop=0;output[0]=0;
        char name[]="Numfixture";do_finger(&actor,name);
        total=string_row(total,dump,output,2);
    }
    if(!dump)printf("FILE_STRINGS_NATIVE_ROWS=%u HASH=%llu\n",string_rows,(unsigned long long)total);
    return total;
}
static void string_bad(unsigned kind,int shorter)
{
    FILE *fp=fopen("../player/Numfixture","wb");assert(fp);
    assert(fputs("#PLAYER\nName Numfixture~\nLevel 2\n",fp)>=0);
    if(kind==0)assert(fputs("Description missing terminator",fp)>=0);
    if(kind==1)assert(fputs("Email missing terminator",fp)>=0);
    if(kind==2)assert(fputs("End\n#OBJECT\nVnum 900\nDescription missing",fp)>=0);
    if(kind==3){
        assert(fputs("Title ",fp)>=0);
        for(unsigned i=0;i<MAX_STRING_LENGTH-1;i++)assert(fputc('A',fp)!=EOF);
        assert(fputs("~\nEnd\n#END\n",fp)>=0);
    }
    if(kind==4){
        assert(fputs("Description A",fp)>=0);assert(fputc(0,fp)!=EOF);
        assert(fputs("B~\nEnd\n#END\n",fp)>=0);
    }
    assert(fclose(fp)==0);
    char before[MAX_STRING_LENGTH+256],after[sizeof before];
    fp=fopen("../player/Numfixture","rb");assert(fp);
    size_t n=fread(before,1,sizeof before,fp);assert(!ferror(fp)&&feof(fp));assert(fclose(fp)==0);
    CHAR_DATA *ch=read_player(shorter);assert(ch->pcdata->player_load_failed);
    assert(!save_char_obj_checked(ch));fp=fopen("../player/Numfixture","rb");assert(fp);
    size_t m=fread(after,1,sizeof after,fp);assert(!ferror(fp)&&feof(fp));assert(fclose(fp)==0);
    assert(n==m&&!memcmp(before,after,n));drop_player(ch);
}
#ifndef FILE_STRINGS_BASELINE
static enum file_string_status string_checked(const char *text,char *out,size_t cap)
{
    FILE *fp=input(text);enum file_string_status s=fread_tilde_checked(fp,out,cap);
    assert(fclose(fp)==0);return s;
}
static void string_boundaries(void)
{
    char out[MAX_STRING_LENGTH];
    assert(string_checked("~",out,sizeof out)==FILE_STRING_OK&&!out[0]);
    assert(string_checked(" \n\tA\r\nB~",out,sizeof out)==FILE_STRING_OK&&!strcmp(out,"A\n\rB"));
    assert(string_checked("",out,sizeof out)==FILE_STRING_FORMAT&&!out[0]);
    assert(string_checked("unterminated",out,sizeof out)==FILE_STRING_FORMAT&&!out[0]);
    assert(string_checked("A~",out,1)==FILE_STRING_LIMIT&&!out[0]);
    assert(string_checked("~",out,1)==FILE_STRING_OK&&!out[0]);
    char boundary[MAX_STRING_LENGTH+3];memset(boundary,'A',sizeof boundary);
    boundary[MAX_STRING_LENGTH-1]='~';boundary[MAX_STRING_LENGTH]='\0';
    assert(string_checked(boundary,out,sizeof out)==FILE_STRING_OK&&
           strlen(out)==MAX_STRING_LENGTH-1);
    boundary[MAX_STRING_LENGTH-1]='A';boundary[MAX_STRING_LENGTH]='~';
    boundary[MAX_STRING_LENGTH+1]='\0';
    assert(string_checked(boundary,out,sizeof out)==FILE_STRING_LIMIT&&!out[0]);
    for(unsigned byte=0;byte<256;byte++){
        unsigned char text[]={'A',byte,'B','~'};
        FILE *fp=fmemopen(text,sizeof text,"r");assert(fp);
        enum file_string_status s=fread_tilde_checked(fp,out,sizeof out);
        assert(s==(byte==0?FILE_STRING_FORMAT:FILE_STRING_OK));
        assert(fclose(fp)==0);
    }
    unsigned calls=0;cookie_io_functions_t io={.read=failed_read};
    FILE *fp=fopencookie(&calls,"r",io);assert(fp);
    assert(fread_tilde_checked(fp,out,sizeof out)==FILE_STRING_IO&&!out[0]);
    assert(ungetc('~',fp)!=EOF&&ferror(fp));
    assert(fread_tilde_checked(fp,out,sizeof out)==FILE_STRING_IO);assert(fclose(fp)==0);
    fp=tmpfile();assert(fp);
    for(unsigned i=0;i<FILE_STRING_MAX_READS+1;i++)assert(fputc(' ',fp)!=EOF);
    assert(fputs("A~",fp)>=0);rewind(fp);
    assert(fread_tilde_checked(fp,out,sizeof out)==FILE_STRING_LIMIT&&!out[0]);
    assert(fclose(fp)==0);
    assert(fread_tilde_checked(NULL,out,sizeof out)==FILE_STRING_FORMAT);
    char *result=(char *)1;assert(fread_string_checked(NULL,&result)!=FILE_STRING_OK&&!result);
    /* Boot/runtime interning agrees with the untouched legacy boot reader. */
    fBootDb=TRUE;fp=input("shared tilde string~");char *shared=fread_string(fp);fclose(fp);
    fp=input("shared tilde string~");assert(fread_string_checked(fp,&result)==FILE_STRING_OK&&result==shared);fclose(fp);
    fBootDb=FALSE;fp=input("shared tilde string~");assert(fread_string_checked(fp,&result)==FILE_STRING_OK&&result==shared);fclose(fp);
}
static void live_strings(void)
{
    for(unsigned shorter=0;shorter<2;shorter++)for(unsigned kind=0;kind<5;kind++){
        if(shorter&&kind==2)continue;string_bad(kind,shorter);
        player_fixture("Gold 19");CHAR_DATA *ch=read_player(shorter);
        assert(!ch->pcdata->player_load_failed&&ch->gold==19);drop_player(ch);
    }
    /* Maximum generic strings and space-prefixed titles survive a real save. */
    for(unsigned shorter=0;shorter<2;shorter++){
        FILE *longfile=fopen("../player/Numfixture","w");assert(longfile);
        assert(fputs("#PLAYER\nName Numfixture~\nLevel 2\nDescription ",longfile)>=0);
        for(unsigned i=0;i<MAX_STRING_LENGTH-1;i++)assert(fputc('D',longfile)!=EOF);
        assert(fputs("~\nTitle ",longfile)>=0);
        for(unsigned i=0;i<MAX_STRING_LENGTH-2;i++)assert(fputc('A',longfile)!=EOF);
        assert(fputs("~\nEnd\n#END\n",longfile)>=0);assert(fclose(longfile)==0);
        CHAR_DATA *ch=read_player(shorter);assert(!ch->pcdata->player_load_failed);
        assert(strlen(ch->description)==MAX_STRING_LENGTH-1);
        assert(strlen(ch->pcdata->title)==MAX_STRING_LENGTH-1);
        if(!shorter){
            ch->in_room=&room;
            assert(save_char_obj_checked(ch));drop_player(ch);ch=read_player(0);
            assert(!ch->pcdata->player_load_failed&&strlen(ch->description)==MAX_STRING_LENGTH-1);
            assert(strlen(ch->pcdata->title)==MAX_STRING_LENGTH-1);
        }
        drop_player(ch);
    }
    FILE *fp=fopen("../player/Numfixture","w");assert(fp);
    assert(fputs("#PLAYER\nEmail unterminated",fp)>=0);assert(fclose(fp)==0);
    desc.outtop=0;output[0]=0;char name[]="Numfixture";do_finger(&actor,name);
    assert(strstr(output,"could not be read safely"));
    /* Each accepted field fits by itself; combined finger lines stay bounded. */
    fp=fopen("../player/Numfixture","w");assert(fp);
    assert(fputs("#PLAYER\nTitle ",fp)>=0);
    for(unsigned i=0;i<MAX_STRING_LENGTH-2;i++)assert(fputc('A',fp)!=EOF);
    assert(fputs("~\nLasthost ",fp)>=0);
    for(unsigned i=0;i<MAX_STRING_LENGTH-2;i++)assert(fputc('B',fp)!=EOF);
    assert(fputs("~\nLasttime ",fp)>=0);
    for(unsigned i=0;i<MAX_STRING_LENGTH-2;i++)assert(fputc('C',fp)!=EOF);
    assert(fputs("~\nEnd\n#END\n",fp)>=0);assert(fclose(fp)==0);
    desc.outtop=0;output[0]=0;actor.level=LEV_ST;do_finger(&actor,name);actor.level=LEV_AWA;
    assert(!strstr(output,"could not be read safely"));
    /* Repeated fields and successful offline lookups reuse allocator storage. */
    fp=fopen("../player/Numfixture","w");assert(fp);
    assert(fputs("#PLAYER\nTitle borrowed-check~\nCreatetime fixed~\n",fp)>=0);
    for(unsigned i=0;i<100;i++)
        assert(fputs("Email repeated-private-fixture-value-for-replacement-checks~\n",fp)>=0);
    assert(fputs("End\n#END\n",fp)>=0);assert(fclose(fp)==0);
    extern long nAllocPerm;
    long warmed=0;
    for(unsigned i=0;i<52;i++){
        desc.outtop=0;output[0]=0;do_finger(&actor,name);
        assert(!strstr(output,"could not be read safely"));
        if(i==1)warmed=nAllocPerm;
        if(i>1)assert(nAllocPerm==warmed);
    }
    /* Online lookups only borrow target strings and must not free them. */
    player_fixture("Email borrowed-live-value~\nCreatetime fixed~\nTitle title~");
    CHAR_DATA *online=read_player(0);assert(!online->pcdata->player_load_failed);
    char_list=online;desc.outtop=0;output[0]=0;do_finger(&actor,name);
    assert(!strcmp(online->pcdata->email,"borrowed-live-value"));
    char_list=NULL;drop_player(online);
}
#endif
int main(int argc,char **argv)
{
    fixture_begin();int dump=argc>1&&!strcmp(argv[1],"--native-dump");
    if(argc>2&&!strcmp(argv[1],"--boundary")){
        unsigned kind=(unsigned)atoi(argv[2]);
        if(kind==5){
            FILE *fp=fopen("../player/Numfixture","w");assert(fp);
            assert(fputs("#PLAYER\nEmail unterminated",fp)>=0);assert(fclose(fp)==0);
            char name[]="Numfixture";do_finger(&actor,name);
            assert(strstr(output,"could not be read safely"));
        } else string_bad(kind,0);
    }
    else {strings_native(dump);
#ifndef FILE_STRINGS_BASELINE
        if(!dump){string_boundaries();live_strings();puts("File strings: native snapshots, checked limits/IO/interning and player/finger rejection/recovery passed.");}
#endif
    }
    assert(fclose(fpReserve)==0);fpReserve=NULL;board_names(0);
    unlink("../player/Numfixture");assert(rmdir("../player")==0);
    char_list=NULL;descriptor_list=NULL;
    room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=NULL;
    obj_index_hash[900%MAX_KEY_HASH]=NULL;sandbox_end();return 0;
}
