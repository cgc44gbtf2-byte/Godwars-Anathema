/* Real item/enchantment/equipment/interpreter and player-file replays. */
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
static const int types[]={ITEM_WEAPON,ITEM_ARMOR,ITEM_TRASH};
static const long tags[]={0,QUEST_SORCENCHANT,QUEST_ARTIFACT,QUEST_ARTIFACT|QUEST_SORCENCHANT};
static const char *args[]={"gear","missing",""};
typedef struct snapshot {long v[64];} SNAPSHOT;

static char *intern(const char *value)
{
    FILE *f=tmpfile();assert(f);fprintf(f,"%s~",value);rewind(f);
    fBootDb=TRUE;char *text=fread_string(f);fBootDb=FALSE;assert(fclose(f)==0);return text;
}
static CHAR_DATA *fresh(long primary,long secondary,int type,long tag,int existing,int worn)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentechfixture"));
    CHAR_DATA *ch=shell.character;free_string(ch->name);ch->name=str_dup("Enchfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;shell.fcommand=TRUE;
    shell.outbuf=output;shell.outsize=sizeof(output);ch->class=primary;
    ch->level=LEV_SOR;ch->position=POS_STANDING;ch->hit=ch->max_hit=50000;
    ch->mana=ch->max_mana=51000;ch->primal=1234;ch->exp=123456;ch->gold=9876;
    ch->hitroll=17;ch->damroll=19;ch->pcdata->quest=50000;ch->vampgen=4;
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=4;
    if(secondary){
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
        assert(hybrid_player_attach(ch,&state));
    }
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;descriptor_list=&shell;
    OBJ_DATA *obj=create_object(&prototype,0);obj->item_type=type;obj->quest=tag;
    obj->wear_flags=ITEM_TAKE|(type==ITEM_ARMOR?ITEM_WEAR_BODY:ITEM_WEAR_HOLD);
    obj->cost=345;obj->points=67;obj->level=0;
    free_string(obj->questmaker);obj->questmaker=str_dup("Enchfixture");
    free_string(obj->questowner);obj->questowner=str_dup("Enchfixture");
    if(existing){
        AFFECT_DATA *af;
        if(affect_free){af=affect_free;affect_free=af->next;}
        else af=alloc_perm(sizeof(*af));
        memset(af,0,sizeof(*af));af->type=skill_lookup("enchant weapon");
        af->duration=-1;af->location=APPLY_AC;af->modifier=-3;obj->affected=af;
    }
    obj_to_char(obj,ch);
    if(worn)equip_char(ch,obj,type==ITEM_WEAPON?WEAR_WEAPON:type==ITEM_ARMOR?WEAR_BODY:WEAR_HOLD);
    init_mm();return ch;
}
static void release(CHAR_DATA *ch)
{
    while(ch->carrying)extract_obj(ch->carrying);
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;descriptor_list=NULL;
    ch->next=ch->next_in_room=NULL;ch->in_room=NULL;ch->desc=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Enchfixture");unlink("../backup/Enchfixture");
}
static int count(const OBJ_DATA *obj)
{
    int n=0;for(const AFFECT_DATA *af=obj->affected;af;af=af->next)assert(++n<8);
    return n;
}
static long text_hash(const char *text)
{
    if(!text)return 0;
    uint64_t h=UINT64_C(14695981039346656037);
    for(const unsigned char *p=(const unsigned char *)text;*p;p++)h=(h^*p)*UINT64_C(1099511628211);
    return (long)h;
}
static void capture(CHAR_DATA *ch,SNAPSHOT *s)
{
    OBJ_DATA *obj=ch->carrying;assert(obj&&!obj->next_content);
    memset(s,0,sizeof(*s));
    long values[]={ch->hit,ch->mana,ch->primal,ch->exp,ch->gold,ch->pcdata->quest,
        ch->hitroll,ch->damroll,ch->armor,ch->wait,ch->carry_number,ch->carry_weight,
        obj->item_type,obj->quest,obj->cost,obj->points,obj->wear_loc,obj->level,
        obj->spectype,obj->specpower,obj->toughness,obj->wear_flags,obj->extra_flags,
        number_range(1,1000000),count(obj)};
    memcpy(s->v,values,sizeof(values));
    int at=25;
    for(AFFECT_DATA *af=obj->affected;af;af=af->next){
        s->v[at++]=af->type;s->v[at++]=af->duration;s->v[at++]=af->location;
        s->v[at++]=af->modifier;s->v[at++]=af->bitvector;
    }
    s->v[62]=text_hash(obj->questmaker);s->v[63]=text_hash(obj->questowner);
    assert(!strcmp(obj->name,"gear"));
}
static void command(CHAR_DATA *ch,const char *argument,SNAPSHOT *s)
{
    PC_DATA before=*ch->pcdata;long primary=ch->class;
    long mana=ch->mana,hit=ch->hit,primal=ch->primal,exp=ch->exp,gold=ch->gold;
    char text[64];snprintf(text,sizeof(text),"enchant %s",argument);
    shell.outtop=0;output[0]='\0';interpret(ch,text);
    assert(!memcmp(&before,ch->pcdata,sizeof(before))&&ch->class==primary);
    assert(ch->mana==mana&&ch->hit==hit&&ch->primal==primal&&ch->exp==exp&&ch->gold==gold);
    capture(ch,s);
}
static uint64_t dump(int emit)
{
    uint64_t hash=UINT64_C(14695981039346656037);int cases=0;
    for(int t=0;t<3;t++)for(int q=0;q<4;q++)for(int a=0;a<2;a++)
    for(int w=0;w<2;w++)for(int arg=0;arg<3;arg++){
        CHAR_DATA *ch=fresh(CLASS_SORCERER,0,types[t],tags[q],a,w);
        for(int repeat=0;repeat<2;repeat++){
            SNAPSHOT s;command(ch,args[arg],&s);
            int allowed=t<2&&!tags[q]&&!w&&arg==0;
            assert(count(ch->carrying)==a+2*allowed);
            assert(ch->carrying->quest==(tags[q]|(allowed?QUEST_SORCENCHANT:0)));
            char line[2048];int n=snprintf(line,sizeof(line),"%d,%d",cases,repeat);
            for(int i=0;i<64;i++)n+=snprintf(line+n,sizeof(line)-n,",%ld",s.v[i]);
            assert(n>0&&(size_t)n+2<sizeof(line));line[n++]='\n';line[n]='\0';
            for(int i=0;i<n;i++)hash=(hash^(unsigned char)line[i])*UINT64_C(1099511628211);
            if(emit)fputs(line,stdout);
        }
        release(ch);cases++;
    }
    assert(cases==144);return hash;
}
static void paired(void)
{
    int comparisons=0;
    for(int t=0;t<3;t++)for(int q=0;q<4;q++)for(int a=0;a<2;a++)
    for(int w=0;w<2;w++)for(int arg=0;arg<3;arg++){
        SNAPSHOT native[2];CHAR_DATA *ch=fresh(CLASS_SORCERER,0,types[t],tags[q],a,w);
        for(int repeat=0;repeat<2;repeat++)command(ch,args[arg],&native[repeat]);
        release(ch);
        for(int p=0;p<9;p++)for(int order=0;order<2;order++){
            ch=fresh(order?CLASS_SORCERER:parents[p],order?parents[p]:CLASS_SORCERER,types[t],tags[q],a,w);
            for(int repeat=0;repeat<2;repeat++){
                SNAPSHOT actual;command(ch,args[arg],&actual);
                assert(!memcmp(&native[repeat],&actual,sizeof(actual)));comparisons++;
            }
            assert(IS_SORCERER(ch)==!!order);
            if(!order)assert(get_color(ch,BLUE_MAGIC)==0);
            release(ch);
        }
    }
    assert(comparisons==5184);
}
static void guards(void)
{
    for(int mode=0;mode<11;mode++){
        CHAR_DATA *ch=fresh(CLASS_HIGHLANDER,CLASS_SORCERER,ITEM_WEAPON,0,0,0);
        if(mode==0)ch->pcdata->hybrid_present=0;
        if(mode==1)ch->pcdata->hybrid_present=2;
        if(mode==2)ch->pcdata->player_load_failed=1;
        if(mode==3)ch->pcdata->hybrid.primary_class=CLASS_NINJA;
        if(mode==4)ch->pcdata->hybrid.secondary_class=CLASS_NONE;
        if(mode==5)ch->pcdata->hybrid.secondary_progression.colors[BLUE_MAGIC]=6;
        if(mode==6)ch->class=CLASS_NONE;
        if(mode==7){ch->pcdata->hybrid_present=0;ch->level=LEV_IMP;}
        if(mode==8)SET_BIT(ch->act,ACT_IS_NPC);
        if(mode==9)ch->level=LEV_AWA;
        if(mode==10){
            HYBRID_STATE other;
            assert(hybrid_secondary_initialize(ch,hybrid_by_parents(CLASS_HIGHLANDER,CLASS_VAMPIRE),&other));
            ch->pcdata->hybrid=other;assert(hybrid_player_save_valid(ch));
        }
        SNAPSHOT s;command(ch,"gear",&s);assert(!ch->carrying->affected&&!ch->carrying->quest);
        REMOVE_BIT(ch->act,ACT_IS_NPC);release(ch);
    }
}
static void ownership(void)
{
    const char *owners[]={"",NULL,"Otherfixture","Enchfixture Other","Enchfixturex",
                          "eNCHfixture","Enchfixture","Otherfixture"};
    int cases=0;
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++)for(int mode=0;mode<8;mode++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER,ITEM_WEAPON,0,0,0);
        OBJ_DATA *obj=ch->carrying;free_string(obj->questowner);
        obj->questowner=owners[mode]?str_dup(owners[mode]):NULL;
        if(mode==6){free_string(obj->questmaker);obj->questmaker=str_dup("Otherfixture");}
        if(mode==7)ch->level=LEV_IMP; /* No owner bypass for staff. */
        SNAPSHOT s;
        command(ch,"gear",&s);
        assert(!strcmp(obj->questmaker,mode==6?"Otherfixture":"Enchfixture"));
        assert(count(obj)==((mode==5||mode==6)?2:0));
        if(owners[mode])assert(!strcmp(obj->questowner,owners[mode]));
        else assert(!obj->questowner);
        if(mode==0||mode==1){
            char claim[]="claim gear";interpret(ch,claim);
            assert(!str_cmp(ch->name,obj->questowner)&&!obj->affected);
            command(ch,"gear",&s);assert(count(obj)==2);
        }else if(mode==2){
            char claim[]="claim gear";interpret(ch,claim);
            assert(!strcmp(obj->questowner,"Otherfixture")&&!obj->affected);
        }
        release(ch);cases++;
    }
    assert(cases==152);
    CHAR_DATA *ch=fresh(CLASS_SORCERER,0,ITEM_ARMOR,0,0,0);SNAPSHOT s;
    command(ch,"gear",&s);assert(count(ch->carrying)==2);
    free_string(ch->carrying->questowner);ch->carrying->questowner=str_dup("Otherfixture");
    command(ch,"gear",&s);
    assert(count(ch->carrying)==2&&ch->carrying->quest==QUEST_SORCENCHANT);
    release(ch);
}
static void roundtrips(void)
{
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++)for(int t=0;t<2;t++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER,types[t],0,1,0);
        OBJ_DATA *obj=ch->carrying;SNAPSHOT s;command(ch,"gear",&s);assert(count(obj)==3);
        assert(obj->affected->location==APPLY_DAMROLL&&obj->affected->modifier==5);
        assert(obj->affected->next->location==APPLY_HITROLL&&obj->affected->next->modifier==5);
        assert(obj->affected->duration==-1&&obj->affected->next->duration==-1);
        equip_char(ch,obj,t==0?WEAR_WEAPON:WEAR_BODY);
        assert(ch->hitroll==22&&ch->damroll==24);
        unequip_char(ch,obj);assert(ch->hitroll==17&&ch->damroll==19);
        assert(save_char_obj_checked(ch));
        for(int short_load=0;short_load<2;short_load++){
            DESCRIPTOR_DATA d={0};
            assert(short_load?load_char_short(&d,"Enchfixture"):load_char_obj(&d,"Enchfixture"));
            CHAR_DATA *loaded=d.character;
            assert(loaded->class==ch->class&&loaded->primal==1234&&loaded->gold==9876&&loaded->pcdata->quest==50000);
            assert(loaded->pcdata->hybrid_present==ch->pcdata->hybrid_present);
            if(p>=0)assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
            if(!short_load){
                OBJ_DATA *o=loaded->carrying;assert(o&&o->quest==QUEST_SORCENCHANT&&count(o)==3);
                int hit=0,dam=0,armor=0;
                for(AFFECT_DATA *af=o->affected;af;af=af->next){
                    /* Native object files intentionally store duration/modifier/
                     * location only; fread_obj restores type=-1, bitvector=0. */
                    assert(af->type==-1&&af->duration==-1&&!af->bitvector);
                    if(af->location==APPLY_HITROLL){assert(af->modifier==5);hit++;}
                    if(af->location==APPLY_DAMROLL){assert(af->modifier==5);dam++;}
                    if(af->location==APPLY_AC){assert(af->modifier==-3);armor++;}
                }
                assert(hit==1&&dam==1&&armor==1&&o->cost==345&&o->points==67);
                assert(!str_cmp(o->questowner,"Enchfixture"));
            }
            loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);
        }
        release(ch);
    }
}
static void shared_spell(void)
{
    const char *owners[]={"Enchfixture","",NULL,"Otherfixture","eNCHfixture","Otherfixture"};
    int cases=0;
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++)for(int mode=0;mode<6;mode++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER,ITEM_WEAPON,0,0,0);
        OBJ_DATA *obj=ch->carrying;free_string(obj->questowner);
        obj->questowner=owners[mode]?str_dup(owners[mode]):NULL;
        if(mode==5)ch->level=LEV_IMP;
        PC_DATA before=*ch->pcdata;long primary=ch->class;
        for(int repeat=0;repeat<2;repeat++){
            spell_enchant_weapon(skill_lookup("enchant weapon"),30,ch,obj);
            assert(count(obj)==((mode==0||mode==4)?2:0));
            assert(obj->quest==((mode==0||mode==4)?QUEST_ENCHANTED:0));
            assert(!memcmp(&before,ch->pcdata,sizeof(before))&&ch->class==primary);
            assert(ch->mana==51000&&ch->primal==1234&&ch->gold==9876);
        }
        release(ch);cases++;
    }
    assert(cases==114);
    /* Shared spells keep their native non-Sorcerer access, but not an owner bypass. */
    CHAR_DATA *ch=fresh(CLASS_MAGE,0,ITEM_WEAPON,0,0,0);
    OBJ_DATA *obj=ch->carrying;
    free_string(obj->questowner);obj->questowner=str_dup("Otherfixture");
    spell_enchant_weapon(skill_lookup("enchant weapon"),30,ch,obj);assert(!obj->affected);
    free_string(obj->questowner);obj->questowner=str_dup(ch->name);
    spell_enchant_weapon(skill_lookup("enchant weapon"),30,ch,obj);assert(count(obj)==2);
    release(ch);
}
int main(int argc,char **argv)
{
    char cwd[4096],root[]="/tmp/anathema-hybrid-enchant-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(mkdir("player",0700)==0&&mkdir("backup",0700)==0&&mkdir("area",0700)==0&&chdir("area")==0);
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;*top_string='\0';
    current_time=2000000000;init_mm();fBootDb=FALSE;fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private enchant fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    prototype.vnum=OBJ_VNUM_PROTOPLASM;prototype.item_type=ITEM_WEAPON;
    prototype.name=intern("gear");prototype.short_descr=intern("some gear");
    prototype.description=intern("Some gear lies here.");prototype.wear_flags=ITEM_TAKE;
    prototype.weight=1;obj_index_hash[OBJ_VNUM_PROTOPLASM%MAX_KEY_HASH]=&prototype;
    assert(skill_lookup("enchant weapon")>=0);
    uint64_t hash=dump(argc>1&&!strcmp(argv[1],"--native-dump"));
    assert(hash==UINT64_C(0x9c4e5177d17c534f)); /* Pre-policy owned-item golden. */
    if(argc>1&&!strcmp(argv[1],"--ownership-only"))ownership();
    else if(argc==1){paired();guards();ownership();shared_spell();roundtrips();}
    assert(!char_list&&!room.people&&!descriptor_list&&fclose(fpReserve)==0);
    assert(chdir("..")==0&&rmdir("player")==0&&rmdir("backup")==0&&rmdir("area")==0&&chdir(cwd)==0&&rmdir(root)==0);
    if(argc==1)printf("hybrid enchant: native %llx;288owned-native snapshots/5184paired attempts/152ownership+114shared spell cases/38save pairs; claim,artifact,worn,type,repeat,tier,record,NPC and equip5+5 passed\n",(unsigned long long)hash);
    return 0;
}
