/* Real file readers in a private world; no live data paths are opened. */
#define _GNU_SOURCE
#define PROGRESS_METRICS_FIXTURE_ONLY
#include "test_progress_metrics.c"
#include "file_number.h"
#include <errno.h>
#include <sys/wait.h>
extern bool fBootDb;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
extern void fread_tiny_obj(CHAR_DATA *,FILE *);
static FILE *input(const char *text)
{
    FILE *fp=tmpfile();assert(fp);
    assert(fputs(text,fp)>=0&&fflush(fp)==0);rewind(fp);return fp;
}
static void player_fixture(const char *fields)
{
    FILE *fp=fopen("../player/Numfixture","w");assert(fp);
    assert(fprintf(fp,"#PLAYER\nName Numfixture~\nLevel 2\n%s\nEnd\n#END\n",fields)>0);
    assert(fclose(fp)==0);
}
static CHAR_DATA *read_player(int short_load)
{
    static DESCRIPTOR_DATA shell;shell=(DESCRIPTOR_DATA){0};
    assert(short_load?load_char_short(&shell,"Numfixture"):
                      load_char_obj(&shell,"Numfixture"));
    assert(shell.character&&shell.character->pcdata);
    return shell.character;
}
static void drop_player(CHAR_DATA *ch)
{ch->in_room=NULL;ch->desc=NULL;free_char(ch);}
static uint64_t numbers_native(int dump)
{
    const long values[]={-1000000,-1000,-100,-10,-1,0,1,2,3,10,100,1000,1000000};
    const long thirds[]={-3,0,1,7};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(unsigned a=0;a<13;a++)for(unsigned b=0;b<13;b++)
    for(unsigned c=0;c<4;c++)for(unsigned form=0;form<4;form++){
        char text[256];
        snprintf(text,sizeof text,"%s%ld|%s%ld|%ld%s",
                 form&1?"\t\n":"",values[a],form&2?" +":"",
                 values[b]<0&&form&2?-values[b]:values[b],thirds[c],
                 form==0?" X":form==1?"\tX":form==2?"\r\nX":"X");
        FILE *fp=input(text);long value=fread_number(fp);int next=fgetc(fp);
        assert(fclose(fp)==0);
        char row[128];snprintf(row,sizeof row,"%ld,%d",value,next);
        uint64_t h=hash(UINT64_C(1469598103934665603),row,strlen(row));
        total=hash(total,&h,sizeof h);
        if(dump)printf("core,%u,%u,%u,%u,%" PRIu64 "\n",a,b,c,form,h);rows++;
    }
    for(unsigned a=0;a<13;a++)for(unsigned b=0;b<13;b++)for(int short_load=0;short_load<2;short_load++){
        char fields[200];snprintf(fields,sizeof fields,"Gold %ld\nPlayed %ld\nPkPdMkMd 3 1 4 2",values[a],values[b]);
        player_fixture(fields);CHAR_DATA *ch=read_player(short_load);
        assert(!ch->pcdata->player_load_failed);
        char row[200];snprintf(row,sizeof row,"%ld,%ld,%ld,%ld,%ld,%ld",
                            ch->gold,ch->played,ch->pkill,ch->pdeath,ch->mkill,ch->mdeath);
        uint64_t h=hash(UINT64_C(1469598103934665603),row,strlen(row));total=hash(total,&h,sizeof h);
        if(dump)printf("player,%u,%u,%d,%" PRIu64 "\n",a,b,short_load,h);
        rows++;drop_player(ch);
    }
    assert(rows==3042);fprintf(stderr,"FILE_NUMBERS_NATIVE_HASH=%" PRIu64 "\n",total);
    assert(total==UINT64_C(612136340295879720));
    return total;
}
static enum file_number_status checked_text(const char *text,int valid,long expected)
{
    FILE *fp=input(text);long value=314159;
    enum file_number_status status=fread_number_checked(fp,&value);
    assert((status==FILE_NUMBER_OK)==valid);
    assert(value==(valid?expected:314159));assert(fclose(fp)==0);
    return status;
}
static int fits_number(__int128 n){return n>=LONG_MIN&&n<=LONG_MAX;}
static ssize_t failed_read(void *cookie,char *data,size_t len)
{
    int *calls=cookie;(void)len;
    if((*calls)++==0){*data='1';return 1;}
    errno=EIO;return -1;
}
static void checked_boundaries(void)
{
    const long values[]={LONG_MIN,LONG_MIN+1,-1000000000,-1000,-1,0,1,2,3,10,
                         1000,1000000000,LONG_MAX-1,LONG_MAX};
    for(unsigned a=0;a<14;a++)for(unsigned b=0;b<14;b++)for(unsigned c=0;c<14;c++){
        __int128 tail=(__int128)values[b]+values[c],all=(__int128)values[a]+tail;
        char text[256];snprintf(text,sizeof text,"%ld|%ld|%ld",values[a],values[b],values[c]);
        checked_text(text,fits_number(tail)&&fits_number(all),
                     fits_number(all)?(long)all:0);
    }
    char text[256];snprintf(text,sizeof text,"%ld",LONG_MIN);checked_text(text,1,LONG_MIN);
    snprintf(text,sizeof text,"+%ld",LONG_MAX);checked_text(text,1,LONG_MAX);
    const char *bad[]={""," ","+","-","1|","1| ","1||2","-+1","x","--1",
        "999999999999999999999999999999999999999",
        "-999999999999999999999999999999999999999"};
    for(unsigned n=0;n<sizeof bad/sizeof *bad;n++)
        assert(checked_text(bad[n],0,0)==(n<10?FILE_NUMBER_FORMAT:FILE_NUMBER_RANGE));
    for(int c=0;c<256;c++){
        unsigned char bytes[]={(unsigned char)c,'X'};
        FILE *fp=tmpfile();assert(fp&&fwrite(bytes,1,2,fp)==2);rewind(fp);
        long value=314159;int digit=c>='0'&&c<='9';
        assert((fread_number_checked(fp,&value)==FILE_NUMBER_OK)==digit);
        assert(value==(digit?c-'0':314159));assert(fclose(fp)==0);
        unsigned char suffix[]={'2',(unsigned char)c};
        fp=tmpfile();assert(fp&&fwrite(suffix,1,2,fp)==2);rewind(fp);value=314159;
        enum file_number_status status=fread_number_checked(fp,&value);
        if(c=='|')assert(status==FILE_NUMBER_FORMAT&&value==314159);
        else {
            assert(status==FILE_NUMBER_OK&&value==(digit?20+c-'0':2));
            if(!digit)assert(fgetc(fp)==(c==' '?EOF:c));
        }
        assert(fclose(fp)==0);
    }
    char *chain=malloc(FILE_NUMBER_MAX_BYTES+2);assert(chain);
    for(unsigned n=0;n<FILE_NUMBER_MAX_TERMS;n++){chain[n*2]='0';chain[n*2+1]='|';}
    chain[FILE_NUMBER_MAX_TERMS*2-1]=0;checked_text(chain,1,0);
    chain[FILE_NUMBER_MAX_TERMS*2-1]='|';chain[FILE_NUMBER_MAX_TERMS*2]='0';
    chain[FILE_NUMBER_MAX_TERMS*2+1]=0;
    assert(checked_text(chain,0,0)==FILE_NUMBER_LIMIT);
    memset(chain,' ',FILE_NUMBER_MAX_BYTES);chain[FILE_NUMBER_MAX_BYTES]='1';
    chain[FILE_NUMBER_MAX_BYTES+1]=0;
    assert(checked_text(chain,0,0)==FILE_NUMBER_LIMIT);free(chain);
    int calls=0;cookie_io_functions_t io={.read=failed_read};
    FILE *fp=fopencookie(&calls,"r",io);assert(fp);long value=314159;
    assert(fread_number_checked(fp,&value)==FILE_NUMBER_IO&&value==314159&&ferror(fp));
    assert(fclose(fp)==0);fp=input("1");
    assert(fread_number_checked(fp,NULL)==FILE_NUMBER_FORMAT&&ftell(fp)==0);
    assert(fread_number_checked(NULL,&value)==FILE_NUMBER_FORMAT&&value==314159);
    assert(fclose(fp)==0);
}
static void live_failures(void)
{
    pc.title=str_empty;
    const char *fields[]={"Gold +","Gold --1","Gold 999999999999999999999999",
        "Played 1|","Boards -1","Boards 1025","Boards +",
        "Boards 1 general +","PkPdMkMd 1 2 +","Affect 1 2 +",
        "End\n#OBJECT\nName Number item~\nNest 0\nVnum 900\nCost +",
        "End\n#OBJECT\nName Number item~\nNest 999999999\nVnum 900",
        "End\n#OBJECT\nNest 0\nVnum 900\nSpell +",
        "End\n#O\n+",
        "End\n#O\n900 1 -1",
        "End\n#O\n900 1 999999999999",
        "End\n#O\n900 1 0 -1 1 +",
        "End\n#OBJECT\nNest 0",
        "End\n#O\n-1 1","End\n#O\n999999 1",
        "End\n#OBJECT\nNest 0\nVnum -1"};
    for(unsigned n=0;n<sizeof fields/sizeof *fields;n++)for(int shorter=0;shorter<2;shorter++){
        if(shorter&&n>=10)continue; /* Short-load deliberately does not read inventory. */
        player_fixture(fields[n]);long objects=index_data.count;
        CHAR_DATA *ch=read_player(shorter);
        assert(ch->pcdata->player_load_failed&&!ch->carrying);
        drop_player(ch);assert(index_data.count==objects&&!object_list);
        /* Failure must remain local: a subsequent unrelated load succeeds. */
        player_fixture("Gold 15\nPlayed 25");ch=read_player(shorter);
        assert(!ch->pcdata->player_load_failed&&ch->gold==15&&ch->played==25);
        drop_player(ch);
    }
    const char *finger_bad[]={"Title numerical fixture~\nPlayed +",
        "Level 999999999999999999999999","PkPdMkMd 1 2 |"};
    for(unsigned n=0;n<3;n++){
        player_fixture(finger_bad[n]);desc.outtop=0;output[0]=0;
        char name[]="Numfixture";do_finger(&actor,name);
        assert(strstr(output,"could not be read safely"));
        assert(!pc.player_load_failed&&fpReserve&&!ferror(fpReserve));
    }
    for(int shorter=0;shorter<2;shorter++){
        const char *bad_ref[]={"Room -1","Vnum -1"};
        for(unsigned n=0;n<2;n++){
            player_fixture(bad_ref[n]);CHAR_DATA *ch=read_player(shorter);
            assert(ch->pcdata->player_load_failed);drop_player(ch);
        }
    }
    char extrema[256];snprintf(extrema,sizeof extrema,"Gold %ld\nPlayed %ld",LONG_MAX,LONG_MIN);
    for(int shorter=0;shorter<2;shorter++){
        player_fixture(extrema);CHAR_DATA *ch=read_player(shorter);
        assert(!ch->pcdata->player_load_failed&&ch->gold==LONG_MAX&&ch->played==LONG_MIN);
        drop_player(ch);
    }
    long *ranks[]={&leader_board.bestpk_number,&leader_board.worstpk_number,
        &leader_board.pk_number,&leader_board.pd_number,&leader_board.mk_number,
        &leader_board.md_number,&leader_board.tt_number,&leader_board.qc_number};
    for(unsigned n=0;n<8;n++)*ranks[n]=n&1?LONG_MIN:LONG_MAX;
    save_leaderboard();board_names(0);load_leaderboard();
    for(unsigned n=0;n<8;n++)assert(*ranks[n]==(n&1?LONG_MIN:LONG_MAX));
}
static void old_boundary(int n)
{
    char text[100];
    if(n==0)snprintf(text,sizeof text,"%ld",LONG_MAX);
    else if(n==1)snprintf(text,sizeof text,"%ld",LONG_MIN);
    else if(n==2)snprintf(text,sizeof text,"%ld|1",LONG_MAX);
    else if(n==3)snprintf(text,sizeof text,"%ld|-1",LONG_MIN);
    else if(n==4)strcpy(text,"999999999999999999999999999999999999");
    else if(n==5)strcpy(text,"Gold +");
    else if(n==6)strcpy(text,"Played 1|");
    else strcpy(text,"Gold 999999999999999999999999999999999999");
    if(n<5){FILE *fp=input(text);printf("%ld\n",fread_number(fp));fclose(fp);}
    else {player_fixture(text);CHAR_DATA *ch=read_player(n%2);drop_player(ch);}
}
#ifndef FILE_NUMBERS_FIXTURE_ONLY
int main(int argc,char **argv)
{
    sandbox_begin();assert(mkdir("../player",0700)==0);
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);init_mm();fBootDb=FALSE;
    metrics_fixture();room.vnum=ROOM_VNUM_TEMPLE;room.name="Private number fixture";
    room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=&room;
    index_data.vnum=900;index_data.item_type=ITEM_TRASH;
    index_data.name="Number token";index_data.short_descr="a token";
    index_data.description="A token is here.";index_data.weight=1;
    obj_index_hash[900%MAX_KEY_HASH]=&index_data;
    if(argc>2&&!strcmp(argv[1],"--boundary"))old_boundary(atoi(argv[2]));
    else {
        numbers_native(argc>1&&!strcmp(argv[1],"--native-dump"));
        if(argc==1){checked_boundaries();live_failures();
            puts("File numbers:3042 native snapshots,2744 right-associative oracles,all-byte/IO/limit cases and full/short/finger rejection/recovery passed.");}
    }
    assert(fclose(fpReserve)==0);fpReserve=NULL;board_names(0);
    unlink("../player/Numfixture");assert(rmdir("../player")==0);
    char_list=NULL;descriptor_list=NULL;room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=NULL;
    obj_index_hash[900%MAX_KEY_HASH]=NULL;
    sandbox_end();return 0;
}
#endif
