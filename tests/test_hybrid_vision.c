#define _GNU_SOURCE
#include <assert.h>
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
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
extern bool check_blind(CHAR_DATA *);
extern void init_mm(void);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static OBJ_INDEX_DATA prototype;
static char output[32768];
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_VAMPIRE,CLASS_WEREWOLF};

static CHAR_DATA *fresh(long primary,long secondary)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentvisfixture"));
    CHAR_DATA *ch=shell.character;free_string(ch->name);ch->name=str_dup("Visfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;shell.fcommand=TRUE;
    shell.outbuf=output;shell.outsize=sizeof(output);descriptor_list=&shell;
    ch->class=primary;ch->level=LEV_SOR;ch->position=POS_STANDING;
    ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=51000;
    ch->primal=1234;ch->exp=123456;ch->gold=9876;ch->pcdata->quest=50000;ch->vampgen=4;
    ch->extra=EXTRA_PROMPT;
    if(secondary){
        HYBRID_STATE state;const HYBRID_DEFINITION *d=hybrid_by_parents(primary,secondary);
        assert(d&&hybrid_secondary_initialize(ch,d,&state)&&hybrid_player_attach(ch,&state));
    }
    ch->in_room=&room;room.people=ch;ch->next=NULL;ch->next_in_room=NULL;char_list=ch;
    return ch;
}
static void release(CHAR_DATA *ch)
{
    while(ch->carrying)extract_obj(ch->carrying);
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;room.people=ch->next_in_room;descriptor_list=NULL;
    ch->next=ch->next_in_room=NULL;ch->in_room=NULL;ch->desc=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Visfixture");unlink("../backup/Visfixture");
}
static int raw(const CHAR_DATA *ch)
{
    return (!!IS_SET(ch->act,PLR_HOLYLIGHT))|
        (!!IS_AFFECTED(ch,AFF_SHADOWSIGHT)<<1)| (!!IS_VAMPAFF(ch,VAM_NIGHTSIGHT)<<2);
}
static void setraw(CHAR_DATA *ch,int flags)
{
    REMOVE_BIT(ch->act,PLR_HOLYLIGHT);REMOVE_BIT(ch->affected_by,AFF_SHADOWSIGHT);
    REMOVE_BIT(ch->vampaff,VAM_NIGHTSIGHT);
    if(flags&1)SET_BIT(ch->act,PLR_HOLYLIGHT);
    if(flags&2)SET_BIT(ch->affected_by,AFF_SHADOWSIGHT);
    if(flags&4)SET_BIT(ch->vampaff,VAM_NIGHTSIGHT);
}
static int effective(const CHAR_DATA *ch)
{
    return (!!effective_holylight(ch))| (!!effective_shadowsight(ch)<<1)|
           (!!effective_nightsight(ch)<<2);
}
static void command(CHAR_DATA *ch)
{
    PC_DATA before=*ch->pcdata;long cl=ch->class,act=ch->act,af=ch->affected_by;
    long va=ch->vampaff,other=ch->extra&~EXTRA_SORC_VISION;
    long hit=ch->hit,mana=ch->mana,gold=ch->gold,primal=ch->primal,exp=ch->exp;
    char text[]="vision";shell.outtop=0;output[0]='\0';interpret(ch,text);
    assert(!memcmp(&before,ch->pcdata,sizeof(before))&&ch->class==cl);
    assert(ch->act==act&&ch->affected_by==af&&ch->vampaff==va);
    assert((ch->extra&~EXTRA_SORC_VISION)==other);
    assert(ch->hit==hit&&ch->mana==mana&&ch->gold==gold&&ch->primal==primal&&ch->exp==exp);
}
static void combinations(void)
{
    int cases=0;
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++)
    for(int before=0;before<8;before++)for(int after=0;after<8;after++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER);
        setraw(ch,before);assert(effective(ch)==before);
        command(ch);assert((ch->extra&EXTRA_SORC_VISION)==EXTRA_SORC_VISION);
        assert(raw(ch)==before&&effective(ch)==7);
        /* Grants added/removed by other providers while active stay independent. */
        setraw(ch,after);assert(effective(ch)==7);
        command(ch);assert(!(ch->extra&EXTRA_SORC_VISION));
        assert(raw(ch)==after&&effective(ch)==after);
        command(ch);strip_holylight(ch);
        assert(!(ch->extra&EXTRA_SORC_HOLY)&&raw(ch)==(after&6)&&effective(ch)==6);
        command(ch);assert(effective(ch)==7); /* Restore holy after hostile loss. */
        command(ch);assert(raw(ch)==(after&6)&&effective(ch)==(after&6));
        setraw(ch,after);command(ch);strip_shadowsight(ch);
        assert(!(ch->extra&EXTRA_SORC_SHADOW)&&raw(ch)==(after&5)&&effective(ch)==5);
        command(ch);assert(raw(ch)==(after&5)&&effective(ch)==(after&5));
        release(ch);cases++;
    }
    assert(cases==1216);
}
static void guards(void)
{
    for(int mode=0;mode<9;mode++){
        CHAR_DATA *ch=fresh(CLASS_HIGHLANDER,CLASS_SORCERER);
        if(mode==0){ch->pcdata->hybrid_present=0;memset(&ch->pcdata->hybrid,0,sizeof(HYBRID_STATE));}
        if(mode==1)ch->pcdata->hybrid_present=2;
        if(mode==2)ch->pcdata->hybrid.primary_class=CLASS_MONK;
        if(mode==3)ch->pcdata->hybrid.secondary_class=CLASS_DROW;
        if(mode==4)ch->pcdata->hybrid.secondary_progression.colors[BLUE_MAGIC]=6;
        if(mode==5)ch->pcdata->player_load_failed=1;
        if(mode==6)SET_BIT(ch->act,ACT_IS_NPC);
        if(mode==7){ch->class=CLASS_NONE;ch->level=LEV_IMP;}
        if(mode==8)ch->level=0;
        command(ch);assert(!(ch->extra&EXTRA_SORC_VISION));
        if(mode!=8){
            SET_BIT(ch->extra,EXTRA_SORC_VISION);
            assert(effective(ch)==0);setraw(ch,7);assert(effective(ch)==7);
        }
        release(ch);
    }
    assert(!effective(NULL));strip_holylight(NULL);strip_shadowsight(NULL);sorcerer_vision_toggle(NULL);
}
static void roundtrips(void)
{
    int pairs=0;long states[]={0,EXTRA_SORC_VISION,EXTRA_SORC_SHADOW|EXTRA_SORC_NIGHT,
                             EXTRA_SORC_HOLY|EXTRA_SORC_NIGHT};
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++)for(int s=0;s<4;s++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER);
        int provider=(p+2+order+s)%8;setraw(ch,provider);ch->extra|=states[s];
        assert(save_char_obj_checked(ch));
        for(int short_load=0;short_load<2;short_load++){
            DESCRIPTOR_DATA d={0};
            assert(short_load?load_char_short(&d,"Visfixture"):load_char_obj(&d,"Visfixture"));
            CHAR_DATA *loaded=d.character;
            assert(loaded->class==ch->class&&raw(loaded)==provider&&loaded->extra==ch->extra);
            assert(effective(loaded)==effective(ch));
            assert(loaded->mana==51000&&loaded->primal==1234&&loaded->gold==9876&&loaded->pcdata->quest==50000);
            assert(loaded->pcdata->hybrid_present==ch->pcdata->hybrid_present);
            if(p>=0)assert(!memcmp(&loaded->pcdata->hybrid,&ch->pcdata->hybrid,sizeof(HYBRID_STATE)));
            free_char(loaded);
        }
        release(ch);pairs++;
    }
    assert(pairs==76);
}
static void visibility(void)
{
    CHAR_DATA target={0};target.act=ACT_IS_NPC;target.name="a target";target.in_room=&room;
    for(int p=-1;p<9;p++)for(int order=0;order<(p<0?1:2);order++){
        CHAR_DATA *ch=fresh(p<0||order?CLASS_SORCERER:parents[p],
                           p<0?0:order?parents[p]:CLASS_SORCERER);
        OBJ_DATA *obj=create_object(&prototype,0);obj_to_room(obj,&room);
        room.room_flags=ROOM_DARK;room.light=0;
        assert(!can_see(ch,&target)&&!can_see_obj(ch,obj));
        command(ch);assert(can_see(ch,&target)&&can_see_obj(ch,obj)&&check_blind(ch));
        SET_BIT(ch->affected_by,AFF_BLIND);
        assert(can_see(ch,&target)&&can_see_obj(ch,obj)&&check_blind(ch));
        SET_BIT(ch->extra,BLINDFOLDED);assert(!can_see(ch,&target)&&!check_blind(ch));
        REMOVE_BIT(ch->extra,BLINDFOLDED);REMOVE_BIT(ch->affected_by,AFF_BLIND);
        command(ch);assert(!can_see(ch,&target)&&!can_see_obj(ch,obj));
        room.room_flags=0;SET_BIT(target.affected_by,AFF_SHADOWPLANE);
        assert(!can_see(ch,&target));command(ch);assert(can_see(ch,&target));
        strip_holylight(ch);assert(can_see(ch,&target)); /* Shadow grant alone crosses. */
        REMOVE_BIT(target.affected_by,AFF_SHADOWPLANE);extract_obj(obj);
        ch->level=LEV_IMP;
        if(p<0){
            char reset[]="clearclass Visfixture";interpret(ch,reset);
            assert(ch->class==CLASS_NONE&&!(ch->extra&EXTRA_SORC_VISION));
        }
        release(ch);
    }
}
int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-hybrid-vision-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("area",0700)&&!chdir("area"));
    fpReserve=fopen("/dev/null","r");assert(fpReserve);current_time=1792350000;fBootDb=FALSE;
    string_space=calloc(1,4*1024*1024);assert(string_space);top_string=string_space;init_mm();
    room.vnum=900;room.area=&area;room.name="Private vision fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    prototype.vnum=OBJ_VNUM_PROTOPLASM;prototype.item_type=ITEM_WEAPON;
    prototype.name="gear";prototype.short_descr="some gear";prototype.description="Some gear lies here.";
    prototype.wear_flags=ITEM_TAKE;prototype.weight=1;
    combinations();guards();roundtrips();visibility();
    assert(!char_list&&!room.people&&!room.contents&&!descriptor_list&&fclose(fpReserve)==0);
    assert(chdir("..")==0&&!rmdir("player")&&!rmdir("backup")&&!rmdir("area")&&!chdir(cwd)&&!rmdir(root));
    puts("hybrid vision:1216 independent source/provider flows,76full+short reload pairs,19real visibility scenarios; permission/cost/raw-state/hostile holy removal/dark/blind/umbra/shadow reset guards passed");
    return 0;
}
