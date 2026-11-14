/* Real interpreter/page compiler, native snapshots, both parent orders and saves. */
#include <assert.h>
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
typedef struct snapshot {long v[64];uint64_t text[27];} SNAPSHOT;
static const char *scripts[][9]={
 {"page title Red Embers","book title A book",NULL},
 {"page line start.damage.spell","page line end.red",NULL},
 {"page line start.affect.spell","page line apply: hp","page line affect: blind",
  "page line duration: 10","page line reversed","page line end.blue",NULL},
 {"page line start.action.spell","page line end.green",NULL},
 {"page line start.damage.spell","page line next.page","page line message.1: Cast",
  "page line message.2: Hit","page line message.3: Watch","page line end.random",NULL},
 {"page line start.damage.spell","page line end.yellow",NULL},
 {"page line start.damage.spell","page line end.purple",NULL},
 {"page line start.damage.spell","page line end.black",NULL},
 {"page line start.damage.spell","page line end.green",NULL},
 {"page line start.damage.spell","page line end.blue",NULL},
 {"page line not.a.start","page line end.red",NULL},
 {"page line end.red",NULL},
 {"page line start.damage.spell","page line start.damage.spell",NULL},
 {"page line start.affect.spell","page line apply: unknown","page line end.blue",NULL},
 {"page line start.affect.spell","page line apply: hp","page line apply: hp",NULL},
 {"page line start.affect.spell","page line duration: -1",NULL},
 {"page line start.affect.spell","page line duration: 51",NULL},
 {"page line start.affect.spell","page line duration: nope",NULL},
 {"page line start.affect.spell","page line duration: 5","page line duration: 5",NULL},
 {"page line start.affect.spell","page line affect: unknown",NULL},
 {"page line start.action.spell","page line apply: hp",NULL},
 {"page line start.damage.spell","page line duration: 5",NULL},
 {"page line start.damage.spell","page line reversed",NULL},
 {"page line start.damage.spell","page line next.page","page line next.page",NULL},
 {"page line start.affect.spell","page line reversed","page line reversed",NULL},
 {"page line start.damage.spell","page line message.1: One","page line message.1: Two",NULL},
 {"page line start.damage.spell","page line end.red","page line A finished annotation",NULL},
 {"book line start.damage.spell",NULL},
 {"page wrong text","page line","page title Title~test",NULL},
 {"page line start.affect.spell","page line affect: sanct","page line apply: mana",
  "page line duration: 50","page line end.random",NULL},
 {"page line start.damage.spell","page line message.1:",NULL},
 {"page line start.affect.spell","page line apply:",NULL}
};

static char *intern(const char *value)
{
    FILE *f=tmpfile();assert(f);fprintf(f,"%s~",value);rewind(f);
    fBootDb=TRUE;char *text=fread_string(f);fBootDb=FALSE;assert(fclose(f)==0);return text;
}
static uint64_t bytes(const char *text)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    if(!text)return 0;
    for(const unsigned char *p=(const unsigned char *)text;*p;p++)
        hash=(hash^*p)*UINT64_C(1099511628211);
    return hash;
}
static OBJ_DATA *item(CHAR_DATA *ch,const char *name)
{
    for(OBJ_DATA *o=ch->carrying;o;o=o->next_content)
        if(is_name((char *)name,o->name))return o;
    return NULL;
}
static void capture(CHAR_DATA *ch,SNAPSHOT *s)
{
    memset(s,0,sizeof(*s));s->v[0]=ch->primal;s->v[1]=ch->mana;s->v[2]=ch->hit;
    s->v[3]=ch->exp;s->v[4]=ch->carry_number;s->v[5]=ch->carry_weight;
    s->v[6]=ch->wait;s->v[7]=number_range(1,1000000);
    const char *names[]={"page","book","pen"};
    for(int i=0;i<3;i++){
        OBJ_DATA *o=item(ch,names[i]);assert(o);
        long *v=&s->v[8+i*18];
        v[0]=o->item_type;v[1]=o->spectype;v[2]=o->specpower;v[3]=o->toughness;
        v[4]=o->level;v[5]=o->cost;v[6]=o->points;v[7]=o->quest;v[8]=o->extra_flags;
        v[9]=o->wear_flags;v[10]=o->weight;v[11]=o->timer;v[12]=o->wear_loc;
        for(int k=0;k<4;k++)v[13+k]=o->value[k];
        v[17]=bytes(o->questmaker);
        const char *text[]={o->name,o->short_descr,o->description,o->chpoweron,o->chpoweroff,
            o->chpoweruse,o->victpoweron,o->victpoweroff,o->victpoweruse};
        for(int k=0;k<9;k++)s->text[i*9+k]=bytes(text[k]);
    }
}
static CHAR_DATA *fresh(long primary,long secondary)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentwritefixture"));
    CHAR_DATA *ch=shell.character;free_string(ch->name);ch->name=str_dup("Writefixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;shell.fcommand=TRUE;
    shell.outbuf=output;shell.outsize=sizeof(output);ch->class=primary;
    ch->level=LEV_SOR;ch->position=POS_STANDING;ch->hit=ch->max_hit=50000;
    ch->mana=ch->max_mana=51000;ch->primal=1000;ch->exp=123456;
    ch->pcdata->quest=50000;ch->vampgen=4;
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=4;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
        assert(hybrid_player_attach(ch,&state));
    }
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;descriptor_list=&shell;
    const char *materials[]={"sorccreate book","sorccreate page","sorccreate pen"};
    for(int i=0;i<3;i++){char command[40];strcpy(command,materials[i]);interpret(ch,command);}
    assert(ch->carry_number==3&&ch->primal==980);
    equip_char(ch,item(ch,"pen"),WEAR_HOLD);init_mm();return ch;
}
static void release(CHAR_DATA *ch)
{
    while(ch->carrying)extract_obj(ch->carrying);
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;descriptor_list=NULL;
    ch->next=ch->next_in_room=NULL;ch->in_room=NULL;ch->desc=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Writefixture");unlink("../backup/Writefixture");
}
static void write_line(CHAR_DATA *ch,const char *line,SNAPSHOT *out)
{
    PC_DATA before=*ch->pcdata;long primary=ch->class;
    long mana=ch->mana,hit=ch->hit,primal=ch->primal,exp=ch->exp;
    char command[400];int n=snprintf(command,sizeof(command),"write %s",line);
    assert(n>0&&(size_t)n<sizeof(command));
    shell.outtop=0;output[0]='\0';interpret(ch,command);
    assert(!memcmp(&before,ch->pcdata,sizeof(before))&&ch->class==primary);
    assert(ch->mana==mana&&ch->hit==hit&&ch->primal==primal&&ch->exp==exp);
    capture(ch,out);
}
static uint64_t dump(int emit)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    for(size_t c=0;c<32;c++){
        CHAR_DATA *ch=fresh(CLASS_SORCERER,0);
        for(int step=0;scripts[c][step];step++){
            SNAPSHOT s;write_line(ch,scripts[c][step],&s);
            char line[4096];int n=snprintf(line,sizeof(line),"%zu,%d",c,step);
            for(int i=0;i<64;i++)n+=snprintf(line+n,sizeof(line)-n,",%ld",s.v[i]);
            for(int i=0;i<27;i++)n+=snprintf(line+n,sizeof(line)-n,",%llx",(unsigned long long)s.text[i]);
            assert(n>0&&(size_t)n+2<sizeof(line));line[n++]='\n';line[n]='\0';
            for(int i=0;i<n;i++)hash=(hash^(unsigned char)line[i])*UINT64_C(1099511628211);
            if(emit)fputs(line,stdout);
        }
        release(ch);
    }
    return hash;
}
static void paired(void)
{
    size_t comparisons=0;
    for(size_t c=0;c<32;c++){
        SNAPSHOT native[9];CHAR_DATA *ch=fresh(CLASS_SORCERER,0);
        for(int s=0;scripts[c][s];s++)write_line(ch,scripts[c][s],&native[s]);
        release(ch);
        for(int p=0;p<9;p++)for(int order=0;order<2;order++){
            ch=fresh(order?CLASS_SORCERER:parents[p],order?parents[p]:CLASS_SORCERER);
            for(int s=0;scripts[c][s];s++){
                SNAPSHOT actual;write_line(ch,scripts[c][s],&actual);
                assert(!memcmp(&native[s],&actual,sizeof(actual)));comparisons++;
            }
            assert(IS_SORCERER(ch)==!!order);
            if(!order)assert(get_color(ch,BLUE_MAGIC)==0);
            release(ch);
        }
    }
    printf("hybrid writing: %zu paired step snapshots;",comparisons);
}
static void guards(void)
{
    for(int mode=0;mode<13;mode++){
        CHAR_DATA *ch=fresh(CLASS_HIGHLANDER,CLASS_SORCERER);
        if(mode==0)ch->pcdata->hybrid_present=0;
        if(mode==1)ch->pcdata->hybrid_present=2;
        if(mode==2)ch->pcdata->player_load_failed=1;
        if(mode==3)ch->pcdata->hybrid.primary_class=CLASS_NINJA;
        if(mode==4)ch->pcdata->hybrid.secondary_class=CLASS_NONE;
        if(mode==5)ch->pcdata->hybrid.secondary_progression.colors[BLUE_MAGIC]=6;
        if(mode==6)ch->class=CLASS_NONE;
        if(mode==7){ch->pcdata->hybrid_present=0;ch->level=LEV_IMP;}
        if(mode==8)SET_BIT(ch->act,ACT_IS_NPC);
        if(mode==9)unequip_char(ch,item(ch,"pen"));
        if(mode==10)ch->level=LEV_SLE;
        if(mode==11){ch->pcdata->hybrid_present=0;SET_BIT(item(ch,"page")->spectype,ADV_FINISHED);}
        if(mode==12){
            HYBRID_STATE other;
            assert(hybrid_secondary_initialize(ch,hybrid_by_parents(CLASS_HIGHLANDER,CLASS_VAMPIRE),&other));
            ch->pcdata->hybrid=other;assert(hybrid_player_save_valid(ch));
        }
        SNAPSHOT s;write_line(ch,"page line start.damage.spell",&s);
        OBJ_DATA *page=item(ch,"page");
        if(mode==7||mode==10)assert(IS_SET(page->spectype,ADV_STARTED)&&!IS_SET(page->spectype,ADV_FAILED));
        else if(mode==8||mode==9)assert(!page->spectype&&!strcmp(page->chpoweruse,"(null)"));
        else assert(IS_SET(page->spectype,ADV_FAILED)); /* Native ordinary writing, not a magic grant. */
        REMOVE_BIT(ch->act,ACT_IS_NPC);release(ch);
    }
    CHAR_DATA *ch=fresh(CLASS_HIGHLANDER,CLASS_SORCERER);ch->pcdata->hybrid_present=0;
    OBJ_DATA *page=item(ch,"page");free_string(page->chpoweruse);
    page->chpoweruse=str_dup("Previously finished");SET_BIT(page->spectype,ADV_FINISHED);
    SNAPSHOT s;write_line(ch,"page line Ordinary annotation",&s);
    assert(!IS_SET(page->spectype,ADV_FAILED)&&IS_SET(page->spectype,ADV_FINISHED));
    assert(strstr(page->chpoweruse,"Ordinary annotation"));
    write_line(ch,"book title Ordinary title",&s);assert(!strcmp(item(ch,"book")->victpoweruse,"Ordinary title"));
    release(ch);
}
static void limits(void)
{
    CHAR_DATA *ch=fresh(CLASS_HIGHLANDER,CLASS_SORCERER);
    OBJ_DATA *page=item(ch,"page");char large[MAX_STRING_LENGTH];
    memset(large,'x',MAX_STRING_LENGTH-5);large[MAX_STRING_LENGTH-5]='\0';
    free_string(page->chpoweruse);page->chpoweruse=str_dup(large);
    page->spectype=ADV_STARTED|ADV_DAMAGE;SNAPSHOT s;
    write_line(ch,"page line x",&s);
    assert(!strcmp(page->chpoweruse,large)&&page->spectype==(ADV_STARTED|ADV_DAMAGE));
    release(ch);
}
static void roundtrips(void)
{
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER);SNAPSHOT s;
        for(int i=0;scripts[2][i];i++)write_line(ch,scripts[2][i],&s);
        write_line(ch,"page title A lasting spell",&s);
        assert(save_char_obj_checked(ch));
        for(int short_load=0;short_load<2;short_load++){
            DESCRIPTOR_DATA d={0};
            assert(short_load?load_char_short(&d,"Writefixture"):load_char_obj(&d,"Writefixture"));
            CHAR_DATA *loaded=d.character;
            assert(loaded->class==ch->class&&loaded->primal==980&&loaded->pcdata->quest==50000);
            assert(loaded->pcdata->hybrid_present==ch->pcdata->hybrid_present);
            if(p>=0)assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
            if(!short_load){
                OBJ_DATA *o=item(loaded,"page"),*original=item(ch,"page");
                assert(o&&o->spectype==original->spectype&&o->toughness==BLUE_MAGIC);
                assert(o->level==10&&o->value[1]==original->value[1]&&o->value[3]==original->value[3]);
                assert(!strcmp(o->chpoweruse,original->chpoweruse)&&!strcmp(o->victpoweruse,"A lasting spell"));
                assert(o->cost==0&&o->points==0&&!strcmp(o->questmaker,"Writefixture"));
            }
            loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);
        }
        release(ch);
    }
}
int main(int argc,char **argv)
{
    char cwd[4096],root[]="/tmp/anathema-hybrid-write-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(mkdir("player",0700)==0&&mkdir("backup",0700)==0&&mkdir("area",0700)==0&&chdir("area")==0);
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;*top_string='\0';
    current_time=2000000000;init_mm();fBootDb=FALSE;fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private writing fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    prototype.vnum=OBJ_VNUM_PROTOPLASM;prototype.item_type=ITEM_TRASH;
    prototype.name=intern("protoplasm");prototype.short_descr=intern("some protoplasm");
    prototype.description=intern("Some protoplasm lies here.");prototype.wear_flags=ITEM_TAKE;
    prototype.weight=1;obj_index_hash[OBJ_VNUM_PROTOPLASM%MAX_KEY_HASH]=&prototype;
    uint64_t hash=dump(argc>1&&!strcmp(argv[1],"--native-dump"));
    assert(hash==UINT64_C(0x5eb0dc614b3ede8c)); /* Measured before the two access adapters. */
    if(argc==1){paired();guards();limits();roundtrips();}
    assert(!char_list&&!room.people&&!descriptor_list&&fclose(fpReserve)==0);
    assert(chdir("..")==0&&rmdir("player")==0&&rmdir("backup")==0&&rmdir("area")==0&&chdir(cwd)==0&&rmdir(root)==0);
    if(argc==1)printf("native fingerprint %llx;32scripts/19save pairs; NPC,pen,record,staff,ordinary text and bounded append passed\n",(unsigned long long)hash);
    return 0;
}
