/* Private real-engine fixtures: never open live player records. */
#define FILE_NUMBERS_FIXTURE_ONLY
#include "test_file_numbers.c"
#include "file_word.h"
#include <ctype.h>

static uint64_t words_native(int dump)
{
    const char *words[]={"a","Alpha","0","-1","A|B","END","#PLAYER",
                        "armor","two words",""};
    const char *spaces[]={""," ","\t","\r\n"};
    const char *ends[]={""," ","\t","\n","\r\n"," X"};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(unsigned w=0;w<10;w++)for(unsigned space=0;space<4;space++)
    for(unsigned end=0;end<6;end++)for(unsigned quote=0;quote<3;quote++){
        if(!quote&&(w==8||w==9))continue;
        char text[256],marker=quote==1?'\'':'"';
        if(quote)snprintf(text,sizeof text,"%s%c%s%c%s",
                         spaces[space],marker,words[w],marker,ends[end]);
        else snprintf(text,sizeof text,"%s%s%s",spaces[space],words[w],ends[end]);
        FILE *fp=input(text);char *actual=fread_word(fp);int next=fgetc(fp);
        assert(!strcmp(actual,words[w]));assert(fclose(fp)==0);
        char row[256];snprintf(row,sizeof row,"%s:%d",actual,next);
        uint64_t h=hash(UINT64_C(1469598103934665603),row,strlen(row));
        total=hash(total,&h,sizeof h);
        if(dump)printf("word,%u,%u,%u,%u,%" PRIu64 "\n",w,space,end,quote,h);
        rows++;
    }
    for(unsigned quote=0;quote<3;quote++)for(int shorter=0;shorter<2;shorter++){
        FILE *fp=fopen("../player/Numfixture","w");assert(fp);
        const char *q=quote==0?"":quote==1?"'":"\"";
        assert(fprintf(fp,"#%sPLAYER%s\nName Numfixture~\n%sLevel%s 2\n"
                       "%sGold%s 31\n%sEnd%s\n#%sEND%s\n",
                       q,q,q,q,q,q,q,q,q,q)>0);
        assert(fclose(fp)==0);
        CHAR_DATA *ch=read_player(shorter);assert(!ch->pcdata->player_load_failed);
        assert(ch->level==2&&ch->gold==31);
        char row[80];snprintf(row,sizeof row,"%ld,%ld,%ld",ch->level,ch->gold,ch->sex);
        uint64_t h=hash(UINT64_C(1469598103934665603),row,strlen(row));
        total=hash(total,&h,sizeof h);
        if(dump)printf("player,%u,%d,%" PRIu64 "\n",quote,shorter,h);
        rows++;drop_player(ch);
    }
    assert(rows==678&&total==UINT64_C(7745626798489316615));
    fprintf(stderr,"FILE_WORDS_NATIVE_HASH=%" PRIu64 "\n",total);
    return total;
}
static void fixture_begin(void)
{
    sandbox_begin();assert(mkdir("../player",0700)==0);
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);init_mm();fBootDb=FALSE;
    metrics_fixture();pc.title=str_empty;
    room.vnum=ROOM_VNUM_TEMPLE;room.name="Private words fixture";
    room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=&room;
    index_data.vnum=900;index_data.item_type=ITEM_TRASH;
    index_data.name="Word token";index_data.short_descr="a token";
    index_data.description="A token is here.";index_data.weight=1;
    obj_index_hash[900%MAX_KEY_HASH]=&index_data;
}
static void bad_player(unsigned kind,int shorter)
{
    char big[MAX_INPUT_LENGTH+80];memset(big,'X',sizeof big-1);big[sizeof big-1]=0;
    if(kind==0)player_fixture("'Unterminated");
    else if(kind==1)player_fixture(big);
    else if(kind==2){
        FILE *fp=fopen("../player/Numfixture","w");assert(fp);
        assert(fputs("#'Unterminated\n",fp)>=0);assert(fclose(fp)==0);
    } else if(kind==3) {
        FILE *fp=fopen("../player/Numfixture","w");assert(fp);
        assert(fputs("#PLAYER\nName Numfixture~\nLevel 2\n",fp)>=0);
        assert(fclose(fp)==0);
    } else if(kind==4) {
        player_fixture("End\n#OBJECT\nVnum 900\nSpell 0 'Unterminated");
    } else if(kind==5||kind==6) {
        FILE *fp=fopen("../player/Numfixture","w");assert(fp);
        assert(fputs(kind==5?"#UNKNOWN\n":"missing-section-marker\n",fp)>=0);
        assert(fclose(fp)==0);
    } else {
        FILE *fp=fopen("../player/Numfixture","w");assert(fp);
        const char *prefix=kind==7?"#PLAYER\nName Numfixture~\nLe":
                                  "#PLAYER\nEnd\n#OBJECT\nVnum 900\nNa";
        assert(fputs(prefix,fp)>=0&&fputc(0,fp)==0&&fputs("me anything\nEnd\n#END\n",fp)>=0);
        assert(fclose(fp)==0);
    }
    CHAR_DATA *ch=read_player(shorter);
    assert(ch->pcdata->player_load_failed);
    char before[1200];FILE *fp=fopen("../player/Numfixture","r");assert(fp);
    size_t n=fread(before,1,sizeof before,fp);assert(fclose(fp)==0);
    assert(!save_char_obj_checked(ch));
    fp=fopen("../player/Numfixture","r");assert(fp);
    char after[1200];size_t m=fread(after,1,sizeof after,fp);assert(fclose(fp)==0);
    assert(n==m&&!memcmp(before,after,n));drop_player(ch);
}
#ifndef FILE_WORDS_BASELINE
static enum file_word_status checked(const char *text,char *out,size_t cap)
{
    FILE *fp=input(text);enum file_word_status status=fread_word_checked(fp,out,cap);
    assert(fclose(fp)==0);return status;
}
static void word_boundaries(void)
{
    char out[MAX_INPUT_LENGTH];
    assert(checked("",out,sizeof out)==FILE_WORD_EOF&&!out[0]);
    assert(checked(" \t\r\n",out,sizeof out)==FILE_WORD_EOF&&!out[0]);
    assert(checked("''",out,sizeof out)==FILE_WORD_OK&&!out[0]);
    assert(checked("\"\"",out,1)==FILE_WORD_OK&&!out[0]);
    assert(checked("'partial",out,sizeof out)==FILE_WORD_FORMAT&&!out[0]);
    assert(checked("\"partial",out,sizeof out)==FILE_WORD_FORMAT&&!out[0]);
    char text[MAX_INPUT_LENGTH+4];
    memset(text,'a',MAX_INPUT_LENGTH-1);text[MAX_INPUT_LENGTH-1]=0;
    assert(checked(text,out,sizeof out)==FILE_WORD_OK&&strlen(out)==MAX_INPUT_LENGTH-1);
    text[MAX_INPUT_LENGTH-1]='a';text[MAX_INPUT_LENGTH]=0;
    assert(checked(text,out,sizeof out)==FILE_WORD_LIMIT&&!out[0]);
    for(unsigned c=0;c<256;c++){
        unsigned char bytes[]={(unsigned char)c,' '};
        FILE *fp=tmpfile();assert(fp&&fwrite(bytes,1,2,fp)==2);rewind(fp);
        enum file_word_status r=fread_word_checked(fp,out,sizeof out);
        if(c==0)assert(r==FILE_WORD_FORMAT&&!out[0]);
        else if(c=='\''||c=='"')assert(r==FILE_WORD_FORMAT&&!out[0]);
        else if(isspace((unsigned char)c))assert(r==FILE_WORD_EOF&&!out[0]);
        else assert(r==FILE_WORD_OK&&(unsigned char)out[0]==c&&out[1]==0);
        assert(fclose(fp)==0);
    }
    char nul[]={'a',0,'b',' '};
    FILE *fp=tmpfile();assert(fp&&fwrite(nul,1,sizeof nul,fp)==sizeof nul);rewind(fp);
    assert(fread_word_checked(fp,out,sizeof out)==FILE_WORD_FORMAT&&!out[0]);
    assert(fclose(fp)==0);
    char *padding=malloc(FILE_WORD_MAX_READS+2);assert(padding);
    memset(padding,' ',FILE_WORD_MAX_READS);padding[FILE_WORD_MAX_READS]='x';
    padding[FILE_WORD_MAX_READS+1]=0;
    assert(checked(padding,out,sizeof out)==FILE_WORD_LIMIT&&!out[0]);free(padding);
    int calls=0;cookie_io_functions_t io={.read=failed_read};
    fp=fopencookie(&calls,"r",io);assert(fp);
    assert(fread_word_checked(fp,out,sizeof out)==FILE_WORD_IO&&!out[0]);
    assert(ferror(fp));
    assert(ungetc(' ',fp)!=EOF&&ungetc('x',fp)!=EOF&&ferror(fp));
    assert(fread_word_checked(fp,out,sizeof out)==FILE_WORD_IO&&!out[0]);
    assert(fclose(fp)==0);
    assert(fread_word_checked(NULL,out,sizeof out)==FILE_WORD_FORMAT&&!out[0]);
    fp=input("valid");
    assert(fread_word_checked(fp,NULL,sizeof out)==FILE_WORD_FORMAT&&ftell(fp)==0);
    assert(fread_word_checked(fp,out,0)==FILE_WORD_FORMAT&&ftell(fp)==0);
    assert(fclose(fp)==0);
}
static void live_words(void)
{
    for(unsigned bad=0;bad<9;bad++)for(int shorter=0;shorter<2;shorter++){
        if((bad==4||bad==8)&&shorter)continue;
        bad_player(bad,shorter);
        player_fixture("Gold 17");
        CHAR_DATA *ch=read_player(shorter);assert(!ch->pcdata->player_load_failed&&ch->gold==17);
        drop_player(ch);
    }
    player_fixture("'Broken finger");
    desc.outbuf[0]=0;desc.outtop=0;do_finger(&actor,"Numfixture");
    assert(strstr(desc.outbuf,"could not be read safely"));
}
#endif
#ifndef FILE_WORDS_FIXTURE_ONLY
int main(int argc,char **argv)
{
    fixture_begin();
    if(argc>2&&!strcmp(argv[1],"--boundary"))bad_player((unsigned)atoi(argv[2]),0);
    else {
        words_native(argc>1&&!strcmp(argv[1],"--native-dump"));
#ifndef FILE_WORDS_BASELINE
        if(argc==1){word_boundaries();live_words();
            puts("File words:678 native snapshots,all-byte/quote/length/IO checks and player/finger rejection/recovery passed.");}
#endif
    }
    assert(fclose(fpReserve)==0);fpReserve=NULL;board_names(0);
    unlink("../player/Numfixture");assert(rmdir("../player")==0);
    char_list=NULL;descriptor_list=NULL;
    room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=NULL;
    obj_index_hash[900%MAX_KEY_HASH]=NULL;sandbox_end();return 0;
}
#endif
