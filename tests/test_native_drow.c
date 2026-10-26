/* Native Drow commands against the actual shipped world and relic templates.
 * Scratch world/player data only; never touches live area or player files. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "merc.h"

static CHAR_DATA *actor;
static DESCRIPTOR_DATA descriptor;
static ROOM_INDEX_DATA arena;
static AREA_DATA area;
static char output[65536];
static unsigned checks;

static void copy(const char *from,const char *to)
{
    FILE *in=fopen(from,"rb"),*out=fopen(to,"wb");
    assert(in&&out);
    char data[8192];size_t n;
    while((n=fread(data,1,sizeof data,in)))assert(fwrite(data,1,n,out)==n);
    assert(!ferror(in)&&!fclose(in)&&!fclose(out));
}

static void command(const char *text)
{
    char line[MAX_INPUT_LENGTH];
    assert(strlen(text)<sizeof line);
    strcpy(line,text);
    descriptor.outtop=0;output[0]='\0';
    actor->wait=0;
    interpret(actor,line);
    ++checks;
}

static void prayers(void)
{
    const struct {const char *name;long flag,cost;} powers[]={
        {"acidbolt",DROW_ACID,5000},{"llothsweb",DROW_WEB,8000},
        {"darkness",DROW_DARK,9500},{"faeriefire",DROW_FIRE,5000},
        {"spiderbite",DROW_BITE,7500},{"dualwield",DROW_DUALWIELD,10000},
        {"blindfight",DROW_BFIGHT,5000},{"summonspider",DROW_SPIDER,8000},
        {"spidereyes",DROW_EYES,7500},{"readaura",DROW_READ,6000},
        {"darkvision",DROW_NIGHT,5000},{"heighten",DROW_SIGHT,20000},
        {"poisonimmunity",DROW_PIMM,5000},{"skipdisc",DROW_SKIP,5000},
        {"attunement",DROW_ATTUNE,1000},{"might",DROW_MIGHT,8000},
        {"toughness",DROW_TOUGH,8000},
    };
    for(size_t i=0;i<sizeof powers/sizeof *powers;++i) {
        char line[128];
        snprintf(line,sizeof line,"prayer NativeDrow %s",powers[i].name);
        actor->pcdata->classpoints[CP_CURRENT]=powers[i].cost-1;
        long before=actor->pcdata->drow[DROW_POWERS];
        command(line);
        assert(actor->pcdata->drow[DROW_POWERS]==before);
        assert(actor->pcdata->classpoints[CP_CURRENT]==powers[i].cost-1);
        actor->pcdata->classpoints[CP_CURRENT]=powers[i].cost;
        command(line);
        assert(get_drow(actor,powers[i].flag));
        assert(actor->pcdata->classpoints[CP_CURRENT]==0);
        actor->pcdata->classpoints[CP_CURRENT]=123456;
        command(line);
        assert(actor->pcdata->classpoints[CP_CURRENT]==123456);
    }
    for(int i=0;i<5;++i) {
        actor->pcdata->classpoints[CP_CURRENT]=10000;
        command("prayer NativeDrow speed");
        assert(actor->pcdata->drow[DROW_SPEED]==(i<4?i+1:4));
        assert(actor->pcdata->classpoints[CP_CURRENT]==(i<4?0:10000));
    }
    assert(save_char_obj_checked(actor));
    DESCRIPTOR_DATA reload={0};
    assert(load_char_obj(&reload,"NativeDrow"));
    assert(reload.character->class==CLASS_DROW);
    assert(reload.character->pcdata->drow[DROW_POWERS]==actor->pcdata->drow[DROW_POWERS]);
    assert(reload.character->pcdata->drow[DROW_SPEED]==4);
    free_char(reload.character);
}

static void equipment(void)
{
    const struct {const char *piece,*wear;long vnum,slot;} pieces[]={
        {"cloak","piwafwi",29652,WEAR_ABOUT},
        {"helmet","helmet",29653,WEAR_HEAD},
        {"breastplate","breastplate",29654,WEAR_BODY},
        {"bracer","bracer",29655,WEAR_WRIST_L},
        {"crossbow","crossbow",OBJ_VNUM_DROW_CROSSBOW,WEAR_HOLD},
    };
    for(size_t i=0;i<sizeof pieces/sizeof *pieces;++i) {
        char line[128];
        actor->pcdata->classpoints[CP_CURRENT]=8000;
        actor->pcdata->quest=400;
        snprintf(line,sizeof line,"drowarmor %s",pieces[i].piece);
        command(line);
        OBJ_DATA *o=actor->carrying;
        assert(o&&o->pIndexData->vnum==pieces[i].vnum);
        assert(!str_cmp(o->questowner,actor->name)&&!str_cmp(o->questmaker,actor->name));
        assert(actor->pcdata->classpoints[CP_CURRENT]==0&&actor->pcdata->quest==0);
        if(i==0) {
            command("attunement piwafwi");
            assert(IS_SET(o->extra_flags,ITEM_LOYAL));
        }
        snprintf(line,sizeof line,"wear %s",pieces[i].wear);
        command(line);
        if(get_eq_char(actor,pieces[i].slot)!=o)
            fprintf(stderr,"Drow gear %s expected slot %ld actual %ld: %s\n",
                    pieces[i].piece,pieces[i].slot,o->wear_loc,output);
        assert(get_eq_char(actor,pieces[i].slot)==o);
        assert(save_char_obj_checked(actor));
        DESCRIPTOR_DATA reload={0};
        assert(load_char_obj(&reload,"NativeDrow"));
        OBJ_DATA *saved=get_eq_char(reload.character,pieces[i].slot);
        assert(saved&&saved->pIndexData->vnum==pieces[i].vnum);
        free_char(reload.character);
        extract_obj(o);
    }
    /* OLC-damaged templates must not charge for unusable relics. */
    OBJ_INDEX_DATA *p=get_obj_index(29653);
    assert(p);long type=p->item_type,flags=p->wear_flags,weight=p->weight;
    actor->pcdata->classpoints[CP_CURRENT]=8000;actor->pcdata->quest=400;
    p->item_type=ITEM_TRASH;
    command("drowarmor helmet");
    assert(!actor->carrying&&actor->pcdata->classpoints[CP_CURRENT]==8000);
    assert(actor->pcdata->quest==400);
    p->item_type=type;p->wear_flags=ITEM_TAKE;
    command("drowarmor helmet");
    assert(!actor->carrying&&actor->pcdata->classpoints[CP_CURRENT]==8000);
    p->wear_flags=flags;p->weight=-1;
    command("drowarmor helmet");
    assert(!actor->carrying&&actor->pcdata->quest==400);
    p->weight=weight;
    actor->carry_number=LONG_MAX;
    command("drowarmor helmet");
    assert(!actor->carrying&&actor->pcdata->quest==400);
    actor->carry_number=0;
    actor->carry_weight=LONG_MAX;
    command("drowarmor helmet");
    assert(!actor->carrying&&actor->pcdata->classpoints[CP_CURRENT]==8000);
    actor->carry_weight=0;
}

static void combat_and_travel(void)
{
    CHAR_DATA *target=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
    free_string(target->name);target->name=str_dup("NativeTarget");
    target->act=ACT_IS_NPC;
    target->position=POS_STANDING;target->hit=target->max_hit=100000000;
    char_to_room(target,&arena);
    command("faeriefire NativeTarget");
    assert(IS_AFFECTED(target,AFF_FAERIE_FIRE));
    long sn=skill_lookup("faerie fire");unsigned affects=0;
    for(AFFECT_DATA *a=target->affected;a;a=a->next)
        if(a->type==sn)++affects;
    assert(affects==3);
    command("faeriefire NativeTarget");
    affects=0;
    for(AFFECT_DATA *a=target->affected;a;a=a->next)
        if(a->type==sn)++affects;
    assert(affects==3);
    command("web NativeTarget");
    assert(IS_AFFECTED(target,AFF_WEBBED));
    command("acidbolt NativeTarget");
    assert(actor->wait==8&&target->hit>0);
    assert(strstr(output,"bolt of acid"));
    stop_fighting(actor,TRUE);
    actor->position=POS_STANDING;
    spell_poison(gsn_poison,100,target,actor);
    assert(!IS_AFFECTED(actor,AFF_POISON));
    ROOM_INDEX_DATA destination={0};
    destination.vnum=999902;destination.name="Native Drow destination";
    destination.description="";destination.area=&area;destination.light=1;
    char_from_room(target);char_to_room(target,&destination);
    SET_BIT(destination.room_flags,ROOM_SAFE);
    command("dimensiondoor NativeTarget");assert(actor->in_room==&arena);
    REMOVE_BIT(destination.room_flags,ROOM_SAFE);
    command("dimensiondoor NativeTarget");
    assert(actor->in_room==&destination);
    char_from_room(actor);char_to_room(actor,&arena);
    extract_char(target,TRUE);
}

static void dual_wield(void)
{
    OBJ_INDEX_DATA *p=NULL;
    for(int h=0;h<MAX_KEY_HASH&&!p;++h)
        for(OBJ_INDEX_DATA *v=obj_index_hash[h];v;v=v->next)
            if(v->item_type==ITEM_WEAPON&&v->vnum<29600&&
               IS_SET(v->wear_flags,ITEM_TAKE)){p=v;break;}
    assert(p);
    OBJ_DATA *a=create_object(p,0),*b=create_object(p,0);
    free_string(a->name);a->name=str_dup("nativebladeone");
    free_string(b->name);b->name=str_dup("nativebladetwo");
    obj_to_char(a,actor);obj_to_char(b,actor);
    command("wear nativebladeone");
    command("wear nativebladetwo");
    assert(get_eq_char(actor,WEAR_WEAPON)==a&&get_eq_char(actor,WEAR_HOLD)==b);
    extract_obj(a);extract_obj(b);
}

static void spider(void)
{
    long sn=skill_lookup("guardian");
    assert(sn>=0);
    command("summonspider");
    CHAR_DATA *pet=arena.people;
    assert(pet!=actor&&IS_NPC(pet)&&pet->master==actor);
    assert(actor->pcdata->followers==1&&IS_AFFECTED(pet,AFF_CHARM));
    assert(pet->hit==30000&&!str_cmp(pet->lord,actor->name));
    command("summonspider");
    assert(actor->pcdata->followers==1&&arena.people==pet);
    extract_char(pet,TRUE);
    assert(actor->pcdata->followers==0&&arena.people==actor);
    /* Missing guardian template used to terminate the whole process. */
    long bucket=MOB_VNUM_GUARDIAN%MAX_KEY_HASH;
    MOB_INDEX_DATA **link=&mob_index_hash[bucket];
    while(*link&&(*link)->vnum!=MOB_VNUM_GUARDIAN)link=&(*link)->next;
    assert(*link);MOB_INDEX_DATA *prototype=*link;*link=prototype->next;
    command("summonspider");
    assert(actor->pcdata->followers==0&&arena.people==actor);
    *link=prototype;
    /* No dependency on the unrelated disconnection room is required. */
    bucket=ROOM_VNUM_DISCONNECTION%MAX_KEY_HASH;
    ROOM_INDEX_DATA **room_link=&room_index_hash[bucket];
    while(*room_link&&(*room_link)->vnum!=ROOM_VNUM_DISCONNECTION)
        room_link=&(*room_link)->next;
    assert(*room_link);ROOM_INDEX_DATA *disconnection=*room_link;
    *room_link=disconnection->next;
    command("summonspider");
    *room_link=disconnection;
    pet=arena.people;assert(pet!=actor&&pet->master==actor);
    extract_char(pet,TRUE);
    assert(!actor->pcdata->followers);
    actor->pcdata->followers=-1;
    command("summonspider");
    assert(actor->pcdata->followers==-1&&arena.people==actor);
    actor->pcdata->followers=0;
    ROOM_INDEX_DATA *current=actor->in_room;
    actor->in_room=NULL;
    do_summonspider(actor,"");do_drowarmor(actor,"helmet");
    assert(!actor->pcdata->followers&&!actor->carrying);
    actor->in_room=current;
    do_summonspider(NULL,"");do_drowarmor(NULL,"helmet");
    /* Real speech callback moves the summoner after all pet state is ready. */
    ROOMTEXT_DATA trigger={0};
    trigger.input="#rcome forth, minion of lloth, and serve me!#n";
    trigger.name="";trigger.output="";trigger.choutput="";
    trigger.type=RT_TELEPORT|RT_RETURN;trigger.power=ROOM_VNUM_SCHOOL;
    arena.roomtext=&trigger;
    command("summonspider");
    arena.roomtext=NULL;
    pet=arena.people;
    assert(actor->in_room==get_room_index(ROOM_VNUM_SCHOOL));
    assert(pet&&IS_NPC(pet)&&pet->master==actor&&IS_AFFECTED(pet,AFF_CHARM));
    assert(actor->pcdata->followers==1);
    extract_char(pet,TRUE);
    char_from_room(actor);char_to_room(actor,&arena);
    assert(!actor->pcdata->followers);
}

static void toggles(void)
{
    command("darkvision");assert(effective_nightsight(actor));
    command("darkvision");assert(!effective_nightsight(actor));
    command("skipdisc");assert(IS_AFFECTED(actor,AFF_FLYING));
    command("skipdisc");assert(!IS_AFFECTED(actor,AFF_FLYING));
    command("heighten");assert(effective_holylight(actor));
    command("heighten");assert(!effective_holylight(actor));
    actor->pcdata->classpoints[CP_CURRENT]=1000;
    command("globedarkness");assert(get_drow(actor,DROW_GLOBE));
    assert(actor->pcdata->classpoints[CP_CURRENT]==500);
    command("globedarkness");assert(!get_drow(actor,DROW_GLOBE));
    command("drowupkeep");assert(strstr(output,"Summon spider"));
    command("readaura");command("scry");
}

int main(int argc,char **argv)
{
    char cwd[4096],from[8192],to[16384],name[256];
    char scratch[]="/tmp/anathema-native-drow-XXXXXX";
    const char *dirs[]={"area","player","backup","notes","log","clans"};
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(scratch));
    for(size_t i=0;i<6;++i){snprintf(to,sizeof to,"%s/%s",scratch,dirs[i]);assert(!mkdir(to,0700));}
    snprintf(from,sizeof from,"%s/Genesis/area/area.lst",cwd);
    snprintf(to,sizeof to,"%s/area/area.lst",scratch);copy(from,to);
    FILE *list=fopen(from,"r");assert(list);
    while(fscanf(list,"%255s",name)==1&&strcmp(name,"$")){
        assert(!strchr(name,'/')&&!strchr(name,'\\'));
        snprintf(from,sizeof from,"%s/Genesis/area/%s",cwd,name);
        snprintf(to,sizeof to,"%s/area/%s",scratch,name);copy(from,to);
    }
    assert(!fclose(list));
    snprintf(to,sizeof to,"%s/area",scratch);assert(!chdir(to));
    current_time=time(NULL);fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);boot_db();
    assert(!load_char_obj(&descriptor,"NativeDrow"));actor=descriptor.character;
    free_string(actor->name);actor->name=str_dup("NativeDrow");
    actor->class=CLASS_DROW;actor->level=LEV_AWA;
    actor->position=POS_STANDING;actor->desc=&descriptor;
    descriptor.connected=CON_PLAYING;descriptor.outbuf=output;descriptor.outsize=sizeof output;
    actor->hit=actor->max_hit=actor->mana=actor->max_mana=50000;
    actor->stance[0]=-1;
    arena.vnum=999901;arena.name="Native Drow audit";arena.description="";
    arena.light=1;area.name="Native Drow audit";arena.area=&area;
    actor->next=char_list;char_list=actor;char_to_room(actor,&arena);
    prayers();toggles();
    if(argc==2&&!strcmp(argv[1],"--spider"))spider();
    else {equipment();dual_wield();combat_and_travel();spider();}
    extract_char(actor,TRUE);actor=NULL;
    assert(!fclose(fpReserve)&&!chdir(cwd));
    for(size_t i=0;i<6;++i){
        snprintf(from,sizeof from,"%s/%s",scratch,dirs[i]);DIR *dir=opendir(from);assert(dir);
        struct dirent *e;while((e=readdir(dir)))if(strcmp(e->d_name,".")&&strcmp(e->d_name,"..")){
            snprintf(to,sizeof to,"%s/%s",from,e->d_name);assert(!unlink(to));
        }
        assert(!closedir(dir)&&!rmdir(from));
    }
    assert(!rmdir(scratch));
    printf("Native Drow: %u real-world command checks, 17 prayers, five worn/reloaded relics, native toggles and spider lifecycle passed.\n",checks);
    return 0;
}
