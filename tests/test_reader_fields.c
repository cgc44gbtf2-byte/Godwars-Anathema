/* Real constructors/readers with private data and targeted fopen fault injection. */
#define FILE_NUMBERS_FIXTURE_ONLY
#include "test_file_numbers.c"
#include "object_numbers.h"
#include "file_io.h"
#include <dirent.h>
#include <fcntl.h>
extern void append_file(CHAR_DATA *,char *,char *);
extern bool valid_name(char *);
extern bool char_exists(char *);

static int deny_player_read,deny_reserve,deny_close,read_error;
static int player_errno=EACCES;
static FILE *close_target;
static size_t cookie_at;
static ssize_t read_failure(void *cookie,char *data,size_t size)
{
    const char prefix[]="#PLAYER\nName Numfixture~\nLevel 2\nGold 42\nEnd\n";
    (void)cookie;
    if(cookie_at==sizeof prefix-1){errno=EIO;return -1;}
    size_t left=sizeof prefix-1-cookie_at;if(size>left)size=left;
    memcpy(data,prefix+cookie_at,size);cookie_at+=size;return size;
}
FILE *__real_fopen(const char *,const char *);
int __real_fclose(FILE *);
int __wrap_fclose(FILE *fp)
{
    int failed=deny_close&&fp==close_target;
    if(fp==close_target)close_target=NULL;
    int result=__real_fclose(fp);
    if(failed){deny_close=0;errno=EIO;return EOF;}
    return result;
}
FILE *__wrap_fopen(const char *path,const char *mode)
{
    if(deny_reserve&&!strcmp(path,NULL_FILE)) {
        deny_reserve=0;errno=EMFILE;return NULL;
    }
    if(deny_player_read&&!strcmp(path,"../player/Numfixture")&&!strcmp(mode,"r")
       &&!--deny_player_read) {errno=player_errno;return NULL;}
    if(read_error&&!strcmp(path,"../player/Numfixture")&&!strcmp(mode,"r")){
        read_error=0;cookie_at=0;cookie_io_functions_t io={.read=read_failure};
        return fopencookie(NULL,"r",io);
    }
    FILE *fp=__real_fopen(path,mode);
    if(deny_close&&!strcmp(path,"../player/Numfixture")&&!strcmp(mode,"r"))close_target=fp;
    return fp;
}
static unsigned fd_count(void)
{
    DIR *dir=opendir("/proc/self/fd");assert(dir);
    unsigned count=0;struct dirent *entry;
    while((entry=readdir(dir)))if(strcmp(entry->d_name,".")&&strcmp(entry->d_name,".."))count++;
    assert(closedir(dir)==0);return count;
}
static void seed(unsigned n){current_time=50000000+n;init_mm();}
static const int item_types[]={ITEM_TRASH,ITEM_MONEY,ITEM_WEAPON,ITEM_ARMOR,
    ITEM_SCROLL,ITEM_WAND,ITEM_STAFF,ITEM_POTION,ITEM_PILL};
static void object_fixture(int type,long vnum)
{
    index_data.item_type=type;index_data.vnum=vnum;
    index_data.value[0]=3;index_data.value[1]=4;index_data.value[2]=5;
    index_data.value[3]=0;
}
static void native_fields(int dump)
{
    const long levels[]={-10,-1,0,1,5,10,25,50,100,1000};
    const long vnums[]={900,29500,29650};
    const int fuzz[]={-1000,-1,0,1,10,1000,INT_MIN+1,INT_MAX-1};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(unsigned v=0;v<3;v++)for(unsigned t=0;t<9;t++)
    for(unsigned n=0;n<10;n++)for(unsigned s=0;s<8;s++){
        object_fixture(item_types[t],vnums[v]);seed(s);
        OBJ_DATA *obj=create_object(&index_data,levels[n]);assert(obj);
        char row[256];snprintf(row,sizeof row,"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%d",
            obj->level,obj->cost,obj->value[0],obj->value[1],obj->value[2],
            obj->value[3],obj->quest,obj->spectype,number_bits(24));
        uint64_t h=hash(UINT64_C(1469598103934665603),row,strlen(row));
        total=hash(total,&h,sizeof h);if(dump)printf("object,%u,%u,%u,%u,%" PRIu64 "\n",v,t,n,s,h);
        extract_obj(obj);assert(!object_list&&!index_data.count);rows++;
    }
    for(unsigned n=0;n<8;n++)for(unsigned s=0;s<8;s++){
        seed(s);int value=number_fuzzy(fuzz[n]),next=number_bits(24);
        char row[100];snprintf(row,sizeof row,"%d,%d",value,next);
        uint64_t h=hash(UINT64_C(1469598103934665603),row,strlen(row));
        total=hash(total,&h,sizeof h);if(dump)printf("fuzzy,%u,%u,%" PRIu64 "\n",n,s,h);rows++;
    }
    assert(rows==2224);fprintf(stderr,"READER_FIELDS_NATIVE_HASH=%" PRIu64 "\n",total);
}
static int model_input(long n){return n<INT_MIN?INT_MIN:n>INT_MAX?INT_MAX:(int)n;}
static int model_fuzzy(long n)
{
    int draw=number_bits(2);
    int64_t wide=model_input(n);wide+=draw==0?-1:draw==3?1:0;
    return wide<1?1:wide>INT_MAX?INT_MAX:(int)wide;
}
static long model_cost(long a,long b,long c)
{
    if(a<0||b<0||c<0)return LONG_MAX;
    __int128 first=(__int128)a*b;
    if(first>LONG_MAX)return LONG_MAX;
    __int128 full=first*c;return full>LONG_MAX?LONG_MAX:(long)full;
}
static void extreme_fields(void)
{
    const long values[]={LONG_MIN,LONG_MIN+1,(long)INT_MIN-1,INT_MIN,
        (long)INT_MIN+1,-1,0,1,1000,20000,(long)INT_MAX-1,INT_MAX,
        (long)INT_MAX+1,LONG_MAX};
    for(unsigned n=0;n<14;n++)for(unsigned s=0;s<8;s++){
        assert(object_fuzzy_input(values[n])==model_input(values[n]));
        seed(s);int expected=model_fuzzy(values[n]),next=number_bits(24);
        seed(s);assert(number_fuzzy(object_fuzzy_input(values[n]))==expected);
        assert(number_bits(24)==next);
        for(unsigned t=0;t<9;t++)for(unsigned v=0;v<3;v++){
            long vnum=v==0?900:v==1?29500:29650;
            seed(s);long base=model_fuzzy(10),a=model_fuzzy(values[n]),b=model_fuzzy(values[n]);
            long cost=v==1?1000000:model_cost(base,a,b);
            object_fixture(item_types[t],vnum);seed(s);
            OBJ_DATA *obj=create_object(&index_data,values[n]);assert(obj);
            assert(obj->cost==cost&&obj->level==(v==1?60:values[n]));
            extract_obj(obj);assert(!object_list&&!index_data.count);
        }
        for(unsigned t=4;t<9;t++){
            object_fixture(item_types[t],900);
            index_data.value[0]=index_data.value[1]=values[n];
            seed(s);for(unsigned k=0;k<3;k++)model_fuzzy(k?1:10);
            long first=model_fuzzy(values[n]),second=0;
            if(item_types[t]==ITEM_POTION||item_types[t]==ITEM_PILL)first=model_fuzzy(first);
            if(item_types[t]==ITEM_WAND||item_types[t]==ITEM_STAFF)second=model_fuzzy(values[n]);
            seed(s);OBJ_DATA *obj=create_object(&index_data,1);assert(obj);
            assert(obj->value[0]==first);
            if(second)assert(obj->value[1]==second&&obj->value[2]==second);
            extract_obj(obj);assert(!object_list&&!index_data.count);
        }
    }
    const long factors[]={-1,0,1,2,3,9,10,11,46340,46341,INT_MAX,LONG_MAX/2,LONG_MAX-1,LONG_MAX};
    for(unsigned a=0;a<14;a++)for(unsigned b=0;b<14;b++)for(unsigned c=0;c<14;c++)
        assert(object_cost_bounded(factors[a],factors[b],factors[c])==
               model_cost(factors[a],factors[b],factors[c]));
    object_fixture(ITEM_TRASH,900);
    player_fixture("End\n#O\n900 20000 0 -1 42 0 0 0 0 0 0 0 0");
    CHAR_DATA *ch=read_player(0);
    assert(!ch->pcdata->player_load_failed&&ch->carrying&&ch->carrying->level==20000&&ch->carrying->cost==42);
    drop_player(ch);assert(!object_list&&!index_data.count);
}
static void reader_errors(void)
{
    object_fixture(ITEM_TRASH,900);pc.title=str_empty;
    unsigned before=fd_count();
    for(unsigned n=0;n<16;n++){
        /* An existing diagnostic is emitted while the player file is open. */
        player_fixture("End\n#UNRECOGNIZED");
        CHAR_DATA *ch=read_player(n%2);drop_player(ch);
        assert(fpReserve&&fcntl(fileno(fpReserve),F_GETFD)!=-1);
        assert(fd_count()==before);
    }
    player_fixture("Gold 42");
    deny_player_read=2;desc.outtop=0;output[0]=0;
    char name[]="Numfixture";do_finger(&actor,name);
    assert(!deny_player_read&&strstr(output,"could not be opened"));
    assert(fpReserve&&fd_count()==before);
    file_reserve_close();assert(!fpReserve);
    file_reserve_close();assert(!fpReserve);
    deny_reserve=1;file_reserve_open();assert(!fpReserve);
    bug("Private reserve exhaustion fixture.",0);
    assert(fpReserve&&fd_count()==before);
    for(int shorter=0;shorter<2;shorter++){
        file_reserve_close();CHAR_DATA *ch=read_player(shorter);
        assert(ch->gold==42&&!ch->pcdata->player_load_failed);drop_player(ch);
        assert(fpReserve&&fd_count()==before);
    }
    const int errors[]={EACCES,EIO,EMFILE,ENOTDIR,EINTR};
    for(int shorter=0;shorter<2;shorter++)for(unsigned n=0;n<5;n++){
        deny_player_read=1;player_errno=errors[n];
        CHAR_DATA *ch=read_player(shorter);
        assert(ch->pcdata->player_load_failed);drop_player(ch);
        assert(!deny_player_read&&fpReserve&&fd_count()==before);
    }
    player_errno=EACCES;
    for(int shorter=0;shorter<2;shorter++){
        read_error=1;CHAR_DATA *ch=read_player(shorter);
        assert(ch->pcdata->player_load_failed);drop_player(ch);
        assert(!read_error&&fpReserve&&fd_count()==before);
        deny_close=1;ch=read_player(shorter);
        assert(ch->pcdata->player_load_failed);drop_player(ch);
        assert(!deny_close&&!close_target&&fpReserve&&fd_count()==before);
    }
    unlink("../player/Numfixture");
    assert(valid_name("Numfixture"));
    for(int shorter=0;shorter<2;shorter++){
        DESCRIPTOR_DATA shell={0};
        assert(!(shorter?load_char_short(&shell,"Numfixture"):load_char_obj(&shell,"Numfixture")));
        assert(!shell.character->pcdata->player_load_failed);drop_player(shell.character);
        assert(fpReserve&&fd_count()==before);
    }
    player_fixture("Gold 42");CHAR_DATA *ch=read_player(0);
    assert(ch->gold==42&&!ch->pcdata->player_load_failed);drop_player(ch);
    char nickname[]="Numfixture";
    file_reserve_close();assert(!valid_name(nickname));
    assert(fpReserve&&char_exists(nickname)&&fd_count()==before);
    deny_player_read=1;assert(!valid_name(nickname));
    assert(!deny_player_read&&fpReserve&&fd_count()==before);
    BAN_DATA ban={0};ban.name="Private fixture";ban_list=&ban;
    file_reserve_close();save_bans();ban_list=NULL;
    assert(fpReserve&&fd_count()==before&&unlink("ban.txt")==0);
    file_reserve_close();append_file(&actor,"../area/missing/file","Private fixture");
    assert(fpReserve&&fd_count()==before);
}
static void red_case(int n)
{
    object_fixture(ITEM_TRASH,900);
    if(n==0){OBJ_DATA *obj=create_object(&index_data,20000);extract_obj(obj);}
    else if(n==1||n==2){
        unsigned s=0;for(;;s++){seed(s);if(number_bits(2)==(n==1?3:0))break;}
        seed(s);printf("%d\n",number_fuzzy(n==1?INT_MAX:INT_MIN));
    }
    else if(n==3){player_fixture("End\n#UNRECOGNIZED");drop_player(read_player(0));}
    else if(n==4){
        pc.title=str_empty;player_fixture("Gold 42");deny_player_read=2;
        char name[]="Numfixture";do_finger(&actor,name);
    }
    else if(n<7){player_fixture("Gold 42");file_reserve_close();drop_player(read_player(n%2));}
    else {
        player_fixture("Gold 42");deny_player_read=1;
        CHAR_DATA *ch=read_player(n%2);
        assert(ch->pcdata->player_load_failed);drop_player(ch);
    }
}
int main(int argc,char **argv)
{
    sandbox_begin();assert(mkdir("../player",0700)==0);
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);fBootDb=FALSE;metrics_fixture();
    room.vnum=ROOM_VNUM_TEMPLE;room.name="Private reader fixture";
    room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=&room;
    index_data.name="Number token";index_data.short_descr="a token";
    index_data.description="A token is here.";index_data.weight=1;index_data.vnum=900;
    obj_index_hash[900%MAX_KEY_HASH]=&index_data;
    if(argc>2&&!strcmp(argv[1],"--boundary"))red_case(atoi(argv[2]));
    else {
        native_fields(argc>1&&!strcmp(argv[1],"--native-dump"));
        if(argc==1){extreme_fields();reader_errors();
            puts("Reader fields:2224 native snapshots;3024 level cases;560 template cases;2744 cost oracles;reader/FD recovery passed.");}
    }
    file_reserve_close();board_names(0);unlink("../player/Numfixture");assert(rmdir("../player")==0);
    char_list=NULL;descriptor_list=NULL;room_index_hash[ROOM_VNUM_TEMPLE%MAX_KEY_HASH]=NULL;
    obj_index_hash[900%MAX_KEY_HASH]=NULL;sandbox_end();return 0;
}
