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

extern void init_mm(void);

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
        snprintf(line,sizeof line,"attunement %s",pieces[i].wear);
        command(line);
        assert(IS_SET(o->extra_flags,ITEM_LOYAL));
        long points=actor->pcdata->classpoints[CP_CURRENT];
        command(line);assert(strstr(output,"already attuned"));
        assert(actor->pcdata->classpoints[CP_CURRENT]==points);
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
        assert(IS_SET(saved->extra_flags,ITEM_LOYAL)&&
               !str_cmp(saved->questowner,actor->name)&&
               !str_cmp(saved->questmaker,actor->name));
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

static long projectile_count(void)
{
    long count=0;
    for(OBJ_DATA *o=actor->carrying;o;o=o->next_content)
        if(o->pIndexData->vnum==OBJ_VNUM_DROW_BOLT)++count;
    return count;
}

static void equipment_use(void)
{
    const char *dirs[]={"north","east","south","west","up","down"};
    unsigned long long hash=1469598103934665603ULL;
    time_t old_time=current_time;current_time=2000000000;init_mm();
    long old_damroll=actor->damroll;actor->damroll=10;
    long old_points=actor->pcdata->classpoints[CP_CURRENT],old_quest=actor->pcdata->quest;
    actor->pcdata->classpoints[CP_CURRENT]=8000;actor->pcdata->quest=400;
    command("drowarmor crossbow");
    OBJ_DATA *bow=actor->carrying;
    assert(bow&&bow->pIndexData->vnum==OBJ_VNUM_DROW_CROSSBOW);
    command("attunement crossbow");command("wear crossbow");
    assert(get_eq_char(actor,WEAR_HOLD)==bow&&IS_SET(bow->extra_flags,ITEM_LOYAL));
    long old_flags=actor->affected_by,old_more=actor->more;
    SET_BIT(actor->affected_by,AFF_HIDE);SET_BIT(actor->more,MORE_DARKNESS|MORE_SILENCE);
    for(int dir=0;dir<6;++dir) {
        ROOM_INDEX_DATA destination={0};EXIT_DATA exit={0},*old_exit=arena.exit[dir];
        destination.vnum=999902+dir;destination.name="Drow crossbow range";
        destination.description="";destination.area=&area;destination.light=1;
        exit.to_room=&destination;arena.exit[dir]=&exit;
        CHAR_DATA *target=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
        free_string(target->name);target->name=str_dup("NativeBoltTarget");
        target->act=ACT_IS_NPC;target->position=POS_STANDING;
        target->hit=target->max_hit=100000000;
        char_to_room(target,&destination);
        actor->pcdata->classpoints[CP_CURRENT]=1200;
        command("drowarmor bolts 2");
        assert(projectile_count()==2&&actor->pcdata->classpoints[CP_CURRENT]==1000);
        command("attunement bolt");
        assert(strstr(output,"cannot attune")&&projectile_count()==2);
        char line[128];snprintf(line,sizeof line,"shoot %s NativeBoltTarget",dirs[dir]);
        SET_BIT(destination.room_flags,ROOM_SAFE);command(line);
        assert(projectile_count()==2&&actor->pcdata->classpoints[CP_CURRENT]==1000&&
               target->hit==target->max_hit);
        REMOVE_BIT(destination.room_flags,ROOM_SAFE);
        /* World boot fuzzes mob template levels. Isolate the shot stream
         * after real target/ammunition construction, not those boot draws. */
        current_time=2000000000+dir;init_mm();
        for(int shot=0;shot<2;++shot) {
            long before=target->hit;command(line);
            assert(target->hit<before&&IS_AFFECTED(target,AFF_SLEEP));
            assert(projectile_count()==1-shot&&
                   actor->pcdata->classpoints[CP_CURRENT]==500*(1-shot));
            assert(actor->in_room==&arena&&arena.people==actor&&
                   target->in_room==&destination&&destination.people==target);
            assert(IS_AFFECTED(actor,AFF_HIDE)&&
                   IS_MORE(actor,MORE_DARKNESS)&&IS_MORE(actor,MORE_SILENCE));
            hash=(hash^(unsigned long long)target->hit)*1099511628211ULL;
            hash=(hash^(unsigned long long)actor->pcdata->classpoints[CP_CURRENT])*
                 1099511628211ULL;
        }
        hash=(hash^(unsigned long long)number_mm())*1099511628211ULL;
        arena.exit[dir]=old_exit;extract_char(target,TRUE);
    }
    actor->affected_by=old_flags;actor->more=old_more;actor->damroll=old_damroll;
    actor->pcdata->classpoints[CP_CURRENT]=old_points;actor->pcdata->quest=old_quest;
    current_time=old_time;extract_obj(bow);
    assert(hash==6470082533908826985ULL);
    printf("Native Drow shipped crossbow: 12 hits/six directions hash=%llu; safe-target refusals, DP/bolts, poison, stealth and room lists passed.\n",hash);
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

static void upkeep_check(long powers,long speed)
{
    actor->pcdata->drow[DROW_POWERS]=powers;
    actor->pcdata->drow[DROW_SPEED]=speed;
    PC_DATA before=*actor->pcdata;
    command("drowupkeep");
    assert(!memcmp(&before,actor->pcdata,sizeof before));
    if(speed<0||speed>4)assert(strstr(output,"invalid Speed rank"));
    else if(speed) {
        char expected[40];snprintf(expected,sizeof expected,"Speed %ld\n\r",speed);
        assert(strstr(output,expected));
        assert(!strstr(output,"no drow abilities"));
    } else if(!powers)assert(strstr(output,"no drow abilities"));
}

static void training_and_reports(void)
{
    long powers=actor->pcdata->drow[DROW_POWERS];
    long speed=actor->pcdata->drow[DROW_SPEED];
    long points=actor->pcdata->classpoints[CP_CURRENT];
    const long invalid[]={-1,LONG_MIN,5,LONG_MAX};
    for(size_t i=0;i<sizeof invalid/sizeof *invalid;++i) {
        actor->pcdata->drow[DROW_SPEED]=invalid[i];
        actor->pcdata->classpoints[CP_CURRENT]=10000;
        command("prayer NativeDrow speed");
        assert(actor->pcdata->drow[DROW_SPEED]==invalid[i]);
        assert(actor->pcdata->classpoints[CP_CURRENT]==10000);
        assert(strstr(output,"invalid Speed rank"));
    }
    for(long rank=0;rank<=4;++rank)
        for(int mask=0;mask<3;++mask)
            upkeep_check(mask==0?0:mask==1?DROW_ACID:powers,rank);
    for(size_t i=0;i<sizeof invalid/sizeof *invalid;++i) {
        upkeep_check(0,invalid[i]);
        upkeep_check(powers,invalid[i]);
    }
    actor->pcdata->drow[DROW_POWERS]=powers;
    actor->pcdata->drow[DROW_SPEED]=speed;
    actor->pcdata->classpoints[CP_CURRENT]=points;
    do_prayer(NULL,"");do_drowupkeep(NULL,"");
    PC_DATA *original_pc=actor->pcdata;actor->pcdata=NULL;
    do_prayer(actor,"");do_drowupkeep(actor,"");actor->pcdata=original_pc;
}

static void prayer_menu(void)
{
    const char *help[]={"prayer","prayer self","prayer acidbolt",
                       "prayer self nosuchgift"};
    for(size_t i=0;i<sizeof help/sizeof *help;++i) {
        PC_DATA before=*actor->pcdata;command(help[i]);
        assert(!memcmp(&before,actor->pcdata,sizeof before));
        assert(strstr(output,"no deity or trainer is required"));
        assert(strstr(output,"prayer self <power>"));
        assert(strstr(output,"prayer <name> <power>"));
        assert(strstr(output,"prayer self acidbolt"));
        assert(strstr(output,"You pay the DP costs"));
        assert(strstr(output,"DualWield (10000)"));
        assert(!strstr(output,"Dual Wield (10000)"));
    }
    long powers=actor->pcdata->drow[DROW_POWERS];
    long points=actor->pcdata->classpoints[CP_CURRENT];
    REMOVE_BIT(actor->pcdata->drow[DROW_POWERS],DROW_ACID);
    actor->pcdata->classpoints[CP_CURRENT]=5000;
    command("prayer self acidbolt");
    assert(get_drow(actor,DROW_ACID)&&actor->pcdata->classpoints[CP_CURRENT]==0);
    command("prayer self acidbolt");
    assert(strstr(output,"already got")&&actor->pcdata->classpoints[CP_CURRENT]==0);
    DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
    assert(get_drow(reload.character,DROW_ACID)&&
           reload.character->pcdata->classpoints[CP_CURRENT]==0);
    free_char(reload.character);
    actor->pcdata->drow[DROW_POWERS]=powers;
    actor->pcdata->classpoints[CP_CURRENT]=points;
}

static void house_training(void)
{
    DESCRIPTOR_DATA other_descriptor={0};
    char other_output[65536]={0};
    assert(!load_char_obj(&other_descriptor,"DrowStudent"));
    other_descriptor.outbuf=other_output;other_descriptor.outsize=sizeof other_output;
    other_descriptor.connected=CON_PLAYING;
    CHAR_DATA *other=other_descriptor.character;
    other->class=CLASS_DROW;other->level=LEV_AWA;other->position=POS_STANDING;
    other->next=char_list;char_list=other;char_to_room(other,&arena);
    long points=actor->pcdata->classpoints[CP_CURRENT];
    char *old_house=actor->clan;
    actor->clan=str_dup("AuditHouse");
    free_string(other->clan);other->clan=str_dup("OtherHouse");
    actor->pcdata->classpoints[CP_CURRENT]=5000;
    command("prayer DrowStudent acidbolt");
    assert(!get_drow(other,DROW_ACID)&&actor->pcdata->classpoints[CP_CURRENT]==5000);
    free_string(other->clan);other->clan=str_dup("AuditHouse");
    command("prayer DrowStudent acidbolt");
    assert(get_drow(other,DROW_ACID)&&actor->pcdata->classpoints[CP_CURRENT]==0);
    actor->pcdata->classpoints[CP_CURRENT]=5000;
    command("prayer DrowStudent acidbolt");
    assert(actor->pcdata->classpoints[CP_CURRENT]==5000);
    command("prayer DrowStudent speed");
    assert(other->pcdata->drow[DROW_SPEED]==0);
    actor->pcdata->classpoints[CP_CURRENT]=10000;
    command("prayer DrowStudent speed");
    assert(other->pcdata->drow[DROW_SPEED]==1&&actor->pcdata->classpoints[CP_CURRENT]==0);
    assert(save_char_obj_checked(actor)&&save_char_obj_checked(other));
    DESCRIPTOR_DATA reload={0};
    assert(load_char_obj(&reload,"DrowStudent"));
    assert(get_drow(reload.character,DROW_ACID)&&
           reload.character->pcdata->drow[DROW_SPEED]==1);
    free_char(reload.character);
    free_string(actor->clan);actor->clan=old_house;
    actor->pcdata->classpoints[CP_CURRENT]=points;
    extract_char(other,TRUE);
}

/* Unlike ordinary command tests, governance must receive the player's actual
 * descriptor input buffer. Do not bypass the interpreter's consent guard. */
static void house_input(CHAR_DATA *who,const char *text)
{
    DESCRIPTOR_DATA *d=who->desc;
    assert(d&&strlen(text)<sizeof d->incomm);
    d->outtop=0;d->outbuf[0]='\0';d->fcommand=TRUE;
    strcpy(d->incomm,text);interpret(who,d->incomm);++checks;
}

static char *house_registry(void)
{
    FILE *fp=fopen("../clans/Registry","rb");assert(fp);
    char *text=calloc(262145,1);assert(text);
    size_t n=fread(text,1,262144,fp);
    assert(n<262144&&!ferror(fp)&&!fclose(fp));return text;
}

static void house_refusal(CHAR_DATA *who,const char *text)
{
    char *before=house_registry();house_input(who,text);
    char *after=house_registry();assert(!strcmp(before,after));
    free(before);free(after);
}

static long house_role(CHAR_DATA *who,const char *role)
{
    char expected[64];long id=0;
    house_input(who,"clan info");
    snprintf(expected,sizeof expected,"your role: %s.",role);
    assert(strstr(who->desc->outbuf,expected));
    assert(sscanf(who->desc->outbuf,"Clan ID %ld:",&id)==1&&id>0);
    return id;
}

static void house_commands(void)
{
    DESCRIPTOR_DATA peers[3]={0};char buffers[3][16384]={0};
    const char *names[]={"HouseMember","HouseRival","HouseStaff"};
    CHAR_DATA *people[3];
    long powers=actor->pcdata->drow[DROW_POWERS];
    long speed=actor->pcdata->drow[DROW_SPEED];
    long points=actor->pcdata->classpoints[CP_CURRENT],rank=actor->pcdata->rank;
    assert(!actor->clan[0]);actor->wait=0;
    for(int i=0;i<3;++i) {
        assert(!load_char_obj(&peers[i],(char *)names[i]));
        peers[i].outbuf=buffers[i];peers[i].outsize=sizeof buffers[i];
        peers[i].connected=CON_PLAYING;people[i]=peers[i].character;
        people[i]->class=i==2?CLASS_NONE:CLASS_DROW;
        people[i]->level=i==2?LEV_IMM:LEV_AWA;
        people[i]->position=POS_STANDING;people[i]->desc=&peers[i];
        people[i]->next=char_list;char_list=people[i];char_to_room(people[i],&arena);
    }
    people[0]->pcdata->drow[DROW_POWERS]=DROW_WEB;
    people[0]->pcdata->drow[DROW_SPEED]=2;
    people[0]->pcdata->classpoints[CP_CURRENT]=123456;
    house_input(people[2],"clanadmin create NativeDrow AuditHouse");
    assert(!strcmp(actor->clan,"AuditHouse"));house_role(actor,"Leader");
    house_input(people[2],"clanadmin create HouseRival RivalHouse");
    long rival_id=house_role(people[1],"Leader");
    house_refusal(actor,"dinduct HouseRival");
    house_refusal(actor,"outcast HouseRival");
    house_refusal(people[2],"clanadmin create HouseMember AuditHouse");
    people[0]->class=CLASS_MAGE;
    house_refusal(actor,"dinduct HouseMember");people[0]->class=CLASS_DROW;
    SET_BIT(people[0]->affected_by,AFF_SHADOWPLANE);
    house_refusal(actor,"dinduct HouseMember");
    REMOVE_BIT(people[0]->affected_by,AFF_SHADOWPLANE);
    peers[0].outtop=0;buffers[0][0]='\0';
    house_input(actor,"dinduct HouseMember");
    assert(strstr(buffers[0],"clan accept Nativedrow")&&!people[0]->clan[0]);
    house_refusal(people[0],"clan accept HouseRival");
    house_input(people[0],"clan accept NativeDrow");
    assert(!strcmp(people[0]->clan,"AuditHouse"));house_role(people[0],"Member");
    house_refusal(people[0],"promotion NativeDrow");
    house_refusal(people[0],"clanadmin create HouseMember Stolen");
    house_input(actor,"promotion HouseMember");house_role(people[0],"Officer");
    house_refusal(people[0],"outcast NativeDrow");
    house_input(actor,"failure HouseMember");house_role(people[0],"Member");
    house_refusal(actor,"clan leave");
    house_input(actor,"clan roster 1");
    assert(strstr(output,"Nativedrow")&&strstr(output,"Housemember"));
    assert(save_char_obj_checked(actor)&&save_char_obj_checked(people[0]));
    clan_registry_close();assert(clan_load_registry());
    house_role(actor,"Leader");house_role(people[0],"Member");
    DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
    assert(!strcmp(reload.character->clan,"AuditHouse")&&
           reload.character->pcdata->drow[DROW_POWERS]==powers&&
           reload.character->pcdata->drow[DROW_SPEED]==speed);
    free_char(reload.character);
    extract_char(people[0],TRUE);people[0]=NULL; /* Actually offline, not a stale pointer. */
    house_input(actor,"outcast HouseMember");
    clan_registry_close();assert(clan_load_registry());
    assert(load_char_obj(&peers[0],"HouseMember"));
    people[0]=peers[0].character;
    assert(!people[0]->clan[0]&&people[0]->class==CLASS_DROW&&
           people[0]->pcdata->drow[DROW_POWERS]==DROW_WEB&&
           people[0]->pcdata->drow[DROW_SPEED]==2&&
           people[0]->pcdata->classpoints[CP_CURRENT]==123456);
    peers[0].connected=CON_PLAYING;people[0]->desc=&peers[0];
    people[0]->position=POS_STANDING;people[0]->wait=0;
    people[0]->next=char_list;char_list=people[0];char_to_room(people[0],&arena);
    house_input(actor,"dinduct HouseMember");
    house_input(people[0],"clan accept NativeDrow");house_role(people[0],"Member");
    house_input(actor,"clan transfer HouseMember");house_role(actor,"Leader");
    house_input(people[0],"clan accept NativeDrow");
    house_role(actor,"Officer");house_role(people[0],"Leader");
    house_refusal(actor,"outcast HouseMember");
    house_input(people[0],"failure NativeDrow");house_role(actor,"Member");
    house_refusal(actor,"promotion HouseMember");
    house_input(people[0],"clan disband confirm");
    assert(!actor->clan[0]&&!people[0]->clan[0]);
    char line[128];snprintf(line,sizeof line,"clanadmin disband %ld confirm",rival_id);
    house_input(people[2],line);assert(!people[1]->clan[0]);
    assert(actor->pcdata->drow[DROW_POWERS]==powers&&
           actor->pcdata->drow[DROW_SPEED]==speed&&
           actor->pcdata->classpoints[CP_CURRENT]==points&&actor->pcdata->rank==rank);
    assert(people[0]->pcdata->drow[DROW_POWERS]==DROW_WEB&&
           people[0]->pcdata->drow[DROW_SPEED]==2&&
           people[0]->pcdata->classpoints[CP_CURRENT]==123456);
    for(int i=0;i<3;++i)extract_char(people[i],TRUE);
    puts("Native Drow houses: real input, named consent, cross-house/rank refusals, "
         "registry reload, offline outcast/relogin, transfer and class-training retention passed.");
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
    if(argc==2&&!strcmp(argv[1],"--report-only"))upkeep_check(0,1);
    else if(argc==2&&!strcmp(argv[1],"--report-extreme"))upkeep_check(DROW_ACID,LONG_MAX);
    else if(argc==2&&!strcmp(argv[1],"--training-bounds"))training_and_reports();
    else if(argc==2&&!strcmp(argv[1],"--prayer-menu"))prayer_menu();
    else if(argc==2&&!strcmp(argv[1],"--equipment-use")) {
        prayers();equipment();equipment_use();
    }
    else if(argc==2&&!strcmp(argv[1],"--house-commands")) {
        prayers();house_commands();
    }
    else {
        prayers();equipment_use();toggles();training_and_reports();prayer_menu();house_training();house_commands();
        if(argc==2&&!strcmp(argv[1],"--spider"))spider();
        else {equipment();dual_wield();combat_and_travel();spider();}
    }
    extract_char(actor,TRUE);actor=NULL;
    clan_registry_close();
    assert(!fclose(fpReserve)&&!chdir(cwd));
    for(size_t i=0;i<6;++i){
        snprintf(from,sizeof from,"%s/%s",scratch,dirs[i]);DIR *dir=opendir(from);assert(dir);
        struct dirent *e;while((e=readdir(dir)))if(strcmp(e->d_name,".")&&strcmp(e->d_name,"..")){
            snprintf(to,sizeof to,"%s/%s",from,e->d_name);assert(!unlink(to));
        }
        assert(!closedir(dir)&&!rmdir(from));
    }
    assert(!rmdir(scratch));
    if(argc==2)
        printf("Native Drow focused mode %s: %u real-world command checks passed.\n",argv[1],checks);
    else
        printf("Native Drow: %u real-world command checks, 17 prayers, five worn/reloaded relics, native toggles and spider lifecycle passed.\n",checks);
    return 0;
}
