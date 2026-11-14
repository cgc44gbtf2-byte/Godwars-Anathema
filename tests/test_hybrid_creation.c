/* Real interpreter, native recipes, record validation and player-file reloads. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern char *string_space,*top_string;
extern bool fBootDb;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static OBJ_INDEX_DATA prototype;
static char output[32768];
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_VAMPIRE,CLASS_WEREWOLF};
static const char *arguments[]={"book","page","pen","","stone","BoOk","book ignored","pa"};
static const long funds[]={LONG_MIN,-1,0,9,10,11,1000,LONG_MAX};
typedef struct snapshot {long v[24];char text[256];} SNAPSHOT;

static char *intern(const char *value)
{
    FILE *stream=tmpfile();assert(stream);
    fprintf(stream,"%s~",value);rewind(stream);
    fBootDb=TRUE;char *text=fread_string(stream);fBootDb=FALSE;
    assert(fclose(stream)==0);return text;
}

static CHAR_DATA *fresh(long primary,long secondary)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentcraftfixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);ch->name=str_dup("Craftfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;shell.fcommand=TRUE;
    shell.outbuf=output;shell.outsize=sizeof(output);
    ch->class=primary;ch->level=LEV_SOR;ch->position=POS_STANDING;
    ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=51000;
    ch->primal=1000;ch->exp=123456;ch->pcdata->quest=50000;ch->vampgen=4;
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=4;
    if(secondary) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
        assert(hybrid_player_attach(ch,&state));
    }
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;descriptor_list=&shell;
    return ch;
}

static void release(CHAR_DATA *ch)
{
    while(ch->carrying)extract_obj(ch->carrying);
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;descriptor_list=NULL;
    ch->next=ch->next_in_room=NULL;ch->in_room=NULL;ch->desc=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Craftfixture");unlink("../backup/Craftfixture");
}

static void command(CHAR_DATA *ch,const char *argument,SNAPSHOT *out)
{
    char line[256];snprintf(line,sizeof(line),"sorccreate %s",argument);
    PC_DATA before=*ch->pcdata;long primary=ch->class;
    shell.outtop=0;output[0]='\0';init_mm();interpret(ch,line);
    assert(!memcmp(&before,ch->pcdata,sizeof(before))&&ch->class==primary);
    assert(ch->hit==50000&&ch->mana==51000&&ch->exp==123456);
    memset(out,0,sizeof(*out));
    out->v[0]=ch->primal;out->v[1]=ch->carry_number;out->v[2]=ch->carry_weight;
    out->v[3]=number_range(1,1000000);out->v[4]=ch->wait;
    OBJ_DATA *o=ch->carrying;
    if(o) {
        assert(!o->next_content&&o->carried_by==ch&&o->in_room==NULL&&o->in_obj==NULL);
        assert(o->pIndexData==&prototype&&!strcmp(o->questmaker,ch->name));
        out->v[5]=1;out->v[6]=o->item_type;out->v[7]=o->wear_flags;
        out->v[8]=o->cost;out->v[9]=o->points;out->v[10]=o->level;
        out->v[11]=o->quest;out->v[12]=o->extra_flags;out->v[13]=o->weight;
        for(int i=0;i<4;i++)out->v[14+i]=o->value[i];
        out->v[18]=o->spectype;out->v[19]=o->specpower;
        out->v[20]=o->toughness;out->v[21]=o->timer;
        snprintf(out->text,sizeof(out->text),"%s|%s|%s|%s",
                 o->name,o->short_descr,o->description,o->questmaker);
        assert(o->cost==0&&o->points==0);
    }
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    for(int present=0;present<2;present++) {
        obj_index_hash[OBJ_VNUM_PROTOPLASM%MAX_KEY_HASH]=present?&prototype:NULL;
        for(size_t a=0;a<8;a++)for(size_t n=0;n<8;n++) {
            CHAR_DATA *ch=fresh(CLASS_SORCERER,0);ch->primal=funds[n];SNAPSHOT s;
            command(ch,arguments[a],&s);
            char line[1024];int used=snprintf(line,sizeof(line),"%d,%zu,%zu",present,a,n);
            for(int i=0;i<24;i++)used+=snprintf(line+used,sizeof(line)-used,",%ld",s.v[i]);
            used+=snprintf(line+used,sizeof(line)-used,",%s\n",s.text);
            assert(used>0&&(size_t)used<sizeof(line));
            for(int i=0;i<used;i++)hash=(hash^(unsigned char)line[i])*UINT64_C(1099511628211);
            if(emit)fputs(line,stdout);
            release(ch);
        }
    }
    obj_index_hash[OBJ_VNUM_PROTOPLASM%MAX_KEY_HASH]=&prototype;
    return hash;
}

static void paired(void)
{
    size_t comparisons=0;
    for(int present=0;present<2;present++) {
        obj_index_hash[OBJ_VNUM_PROTOPLASM%MAX_KEY_HASH]=present?&prototype:NULL;
        for(size_t a=0;a<8;a++)for(size_t n=0;n<8;n++) {
            CHAR_DATA *ch=fresh(CLASS_SORCERER,0);ch->primal=funds[n];SNAPSHOT native,hybrid;
            command(ch,arguments[a],&native);release(ch);
            for(size_t p=0;p<9;p++)for(int order=0;order<2;order++) {
                ch=fresh(order?CLASS_SORCERER:parents[p],order?parents[p]:CLASS_SORCERER);
                ch->primal=funds[n];
                assert(IS_SORCERER(ch)==!!order);
                if(!order)assert(get_color(ch,BLUE_MAGIC)==0);
                command(ch,arguments[a],&hybrid);
                assert(!memcmp(&native,&hybrid,sizeof(native)));
                if(!order)assert(get_color(ch,BLUE_MAGIC)==0);
                release(ch);++comparisons;
            }
        }
    }
    obj_index_hash[OBJ_VNUM_PROTOPLASM%MAX_KEY_HASH]=&prototype;
    assert(comparisons==2304);
}

static void rejected(void)
{
    for(int mode=0;mode<11;mode++) {
        CHAR_DATA *ch=fresh(CLASS_HIGHLANDER,mode==9?CLASS_VAMPIRE:
            mode==10?CLASS_WEREWOLF:CLASS_SORCERER);SNAPSHOT s;
        if(mode==0)ch->pcdata->hybrid_present=0;
        if(mode==1)ch->pcdata->hybrid_present=2;
        if(mode==2)ch->pcdata->player_load_failed=1;
        if(mode==3)ch->pcdata->hybrid.primary_class=CLASS_NINJA;
        if(mode==4)ch->pcdata->hybrid.secondary_class=CLASS_NONE;
        if(mode==5)SET_BIT(ch->act,ACT_IS_NPC);
        if(mode==6)ch->level=LEV_AWA;
        if(mode==7)ch->pcdata->hybrid.secondary_progression.colors[BLUE_MAGIC]=6;
        if(mode==8){ch->pcdata->hybrid_present=0;ch->class=CLASS_NONE;}
        command(ch,"book",&s);assert(!ch->carrying&&ch->primal==1000);
        REMOVE_BIT(ch->act,ACT_IS_NPC);release(ch);
    }
    CHAR_DATA *ch=fresh(CLASS_SORCERER,0);SNAPSHOT s;
    SET_BIT(ch->act,ACT_IS_NPC);command(ch,"book",&s);
    assert(!ch->carrying&&ch->primal==1000);
    REMOVE_BIT(ch->act,ACT_IS_NPC);release(ch);
}

static void roundtrips(void)
{
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++) {
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER);
        for(int i=0;i<3;i++) {
            char line[64];snprintf(line,sizeof(line),"sorccreate %s",arguments[i]);
            interpret(ch,line);
        }
        assert(ch->carry_number==3&&ch->primal==980&&save_char_obj_checked(ch));
        for(int short_load=0;short_load<2;short_load++) {
            DESCRIPTOR_DATA d={0};
            assert(short_load?load_char_short(&d,"Craftfixture"):load_char_obj(&d,"Craftfixture"));
            CHAR_DATA *loaded=d.character;
            assert(loaded->class==ch->class&&loaded->primal==980&&loaded->pcdata->quest==50000);
            assert(loaded->pcdata->hybrid_present==ch->pcdata->hybrid_present);
            if(p>=0)assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
            if(!short_load) {
                int count=0;
                for(OBJ_DATA *o=loaded->carrying;o;o=o->next_content) {
                    assert(o->cost==0&&o->points==0&&!strcmp(o->questmaker,"Craftfixture"));
                    assert(o->item_type==ITEM_BOOK||o->item_type==ITEM_PAGE||o->item_type==ITEM_PEN);++count;
                }
                assert(count==3);
            }
            loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);
        }
        release(ch);
    }
}

int main(int argc,char **argv)
{
    char cwd[4096],root[]="/tmp/anathema-hybrid-create-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(mkdir("player",0700)==0&&mkdir("backup",0700)==0&&mkdir("area",0700)==0&&chdir("area")==0);
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;*top_string='\0';
    current_time=2000000000;init_mm();fBootDb=FALSE;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private crafting fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    prototype.vnum=OBJ_VNUM_PROTOPLASM;prototype.item_type=ITEM_TRASH;
    prototype.name=intern("protoplasm");prototype.short_descr=intern("some protoplasm");
    prototype.description=intern("Some protoplasm lies here.");
    prototype.wear_flags=ITEM_TAKE;prototype.weight=1;prototype.cost=567;prototype.points=45;
    uint64_t native=native_dump(argc>1&&!strcmp(argv[1],"--native-dump"));
    assert(native==UINT64_C(0x54d9b5797dc7217f));
    if(argc==1){paired();rejected();roundtrips();}
    assert(!char_list&&!room.people&&!descriptor_list&&fclose(fpReserve)==0);
    assert(chdir("..")==0&&rmdir("player")==0&&rmdir("backup")==0&&rmdir("area")==0&&chdir(cwd)==0&&rmdir(root)==0);
    if(argc==1)printf("hybrid creation: native fingerprint %llx;2304 paired recipes/costs/RNG;19 long+short roundtrips; invalid/NPC/tier guards passed\n",(unsigned long long)native);
    return 0;
}
