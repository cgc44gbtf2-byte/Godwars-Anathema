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
#include "classes/hybrid/mastery.h"
#include "classes/hybrid/access.h"

extern void init_mm(void);
extern void char_update(void);
extern void mobile_update(void);

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

static void crossbow_death_cleanup(void)
{
    const char *dirs[]={"north","east","south","west","up","down"};
    time_t old_time=current_time;
    long old_damroll=actor->damroll,old_points=actor->pcdata->classpoints[CP_CURRENT];
    long old_quest=actor->pcdata->quest,old_kills=actor->mkill;
    actor->damroll=10;
    actor->pcdata->classpoints[CP_CURRENT]=8000;actor->pcdata->quest=400;
    command("drowarmor crossbow");command("wear crossbow");
    OBJ_DATA *bow=get_eq_char(actor,WEAR_HOLD);
    assert(bow&&bow->pIndexData->vnum==OBJ_VNUM_DROW_CROSSBOW);
    for(int dir=0;dir<6;++dir) {
        ROOM_INDEX_DATA destination={0};EXIT_DATA exit={0},*saved=arena.exit[dir];
        destination.vnum=999920+dir;destination.name="Lethal Drow range";
        destination.description="";destination.area=&area;destination.light=1;
        exit.to_room=&destination;arena.exit[dir]=&exit;
        CHAR_DATA *victim=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
        free_string(victim->name);victim->name=str_dup("DrowDeathTarget");
        /* Suppress rewards so the exact activation charge is observable. */
        victim->act=ACT_IS_NPC|ACT_NOEXP;victim->position=POS_STANDING;
        victim->hit=1;victim->max_hit=1000;
        char_to_room(victim,&destination);
        actor->pcdata->classpoints[CP_CURRENT]=600;
        command("drowarmor bolts");
        assert(projectile_count()==1&&actor->pcdata->classpoints[CP_CURRENT]==500);
        current_time=2000001000+dir;init_mm();
        char line[128];snprintf(line,sizeof line,"shoot %s DrowDeathTarget",dirs[dir]);
        command(line);
        assert(actor->in_room==&arena&&arena.people==actor);
        assert(descriptor.character==actor&&get_eq_char(actor,WEAR_HOLD)==bow);
        assert(destination.people==NULL&&projectile_count()==0);
        assert(actor->pcdata->classpoints[CP_CURRENT]==0&&actor->mkill==old_kills+dir+1);
        for(CHAR_DATA *live=char_list;live;live=live->next)assert(live!=victim);
        /* No world object may retain a pointer to this stack room. */
        while(destination.contents)extract_obj(destination.contents);
        arena.exit[dir]=saved;
    }
    extract_obj(bow);actor->damroll=old_damroll;
    actor->pcdata->classpoints[CP_CURRENT]=old_points;
    actor->pcdata->quest=old_quest;actor->mkill=old_kills;current_time=old_time;
    puts("Native Drow crossbow death: six real NPC deaths/extractions, exact DP/ammo charges, live actor/descriptor and room cleanup passed.");
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

/* Full-HP native stats isolate the summon subtraction from wounded scaling.
 * The optional LONG_MIN probe is intentionally outside the passing suite:
 * run it with UBSan to reproduce a documented, still-unfixed defect. */
static void rejection_cost_check(const char *text);

static void spider_stat_boundaries(int extreme_stat)
{
    long hr=actor->hitroll,dr=actor->damroll,ac=actor->armor;
    long speed=actor->pcdata->drow[DROW_SPEED];
    long old_hit=actor->hit,old_max=actor->max_hit;
    const long values[]={LONG_MIN+50,LONG_MIN+51,-100,0,49,50,1000,LONG_MAX};
    actor->hit=actor->max_hit=50000;
    actor->pcdata->drow[DROW_SPEED]=0;
    for(int stat=0;stat<3;++stat) {
        for(size_t i=0;i<sizeof(values)/sizeof(*values);++i) {
            if(extreme_stat>=0&&(stat!=extreme_stat||i>=2))continue;
            long value=extreme_stat>=0?(i==0?LONG_MIN:LONG_MIN+49):values[i];
            actor->hitroll=actor->damroll=actor->armor=0;
            if(stat==0)actor->hitroll=value;
            else if(stat==1)actor->damroll=value;
            else actor->armor=value;
            long computed=stat==0?char_hitroll(actor):
                          stat==1?char_damroll(actor):char_ac(actor);
            assert(computed==value);
            long dp=actor->pcdata->classpoints[CP_CURRENT];
            if(extreme_stat>=0) {
                CHAR_DATA *head=char_list,*next=actor->next;
                rejection_cost_check("summonspider");
                assert(strstr(output,"cannot safely summon a spider"));
                assert(char_list==head&&actor->next==next&&arena.people==actor);
                assert(actor->pcdata->followers==0);
                continue;
            }
            command("summonspider");
            CHAR_DATA *pet=arena.people;
            assert(pet!=actor&&pet->master==actor&&actor->pcdata->followers==1);
            if(extreme_stat<0) {
                assert(pet->hitroll==(stat==0?value-50:-50));
                assert(pet->damroll==(stat==1?value-50:-50));
                assert(pet->armor==(stat==2?value-50:-50));
            }
            assert(actor->pcdata->classpoints[CP_CURRENT]==dp&&actor->wait==0);
            extract_char(pet,TRUE);
            assert(actor->pcdata->followers==0&&arena.people==actor);
        }
    }
    actor->hitroll=hr;actor->damroll=dr;actor->armor=ac;
    actor->pcdata->drow[DROW_SPEED]=speed;
    actor->hit=old_hit;actor->max_hit=old_max;
    if(extreme_stat<0) {
        puts("Native spider: 24 representable stat boundaries, free summon and follower cleanup passed.");
        spider_stat_boundaries(0);
        spider_stat_boundaries(1);
        spider_stat_boundaries(2);
    }
}

static void timed_effects(void)
{
    /* Isolate the real timer from unrelated boot-world NPCs, retaining every
     * world link for restoration before the normal fixture teardown. */
    assert(char_list==actor&&arena.people==actor);
    CHAR_DATA *world_tail=actor->next;actor->next=NULL;
    long old_timer=actor->timer;actor->timer=0;
    CHAR_DATA *victim=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
    free_string(victim->name);victim->name=str_dup("DrowExpiryTarget");
    victim->act=ACT_IS_NPC;victim->hit=victim->max_hit=100000;
    victim->position=POS_STANDING;
    victim->hitroll=123;victim->damroll=456;victim->armor=789;
    char_to_room(victim,&arena);
    command("faeriefire DrowExpiryTarget");
    assert(victim->hitroll==73&&victim->damroll==406&&victim->armor==739);
    for(int tick=0;tick<10;++tick)char_update();
    assert(IS_AFFECTED(victim,AFF_FAERIE_FIRE)&&victim->hitroll==73);
    char_update();
    assert(!IS_AFFECTED(victim,AFF_FAERIE_FIRE)&&!victim->affected);
    assert(victim->hitroll==123&&victim->damroll==456&&victim->armor==789);
    command("web DrowExpiryTarget");
    assert(IS_AFFECTED(victim,AFF_WEBBED)&&victim->armor==689);
    char_update();char_update();
    assert(IS_AFFECTED(victim,AFF_WEBBED)&&victim->armor==689);
    char_update();
    assert(!IS_AFFECTED(victim,AFF_WEBBED)&&!victim->affected&&victim->armor==789);
    extract_char(victim,TRUE);

    long mana=actor->mana,primal=actor->primal;
    long points=actor->pcdata->classpoints[CP_CURRENT];
    command("summonspider");
    CHAR_DATA *pet=arena.people;long sn=skill_lookup("guardian");
    assert(pet!=actor&&pet->master==actor&&actor->pcdata->followers==1);
    AFFECT_DATA *guardian=NULL;
    for(AFFECT_DATA *af=pet->affected;af;af=af->next)
        if(af->type==sn)guardian=af;
    assert(guardian&&guardian->duration==6666&&guardian->modifier==DROW_SPIDER);
    /* Keep another live effect behind the expiring guardian: extraction
     * recycles both nodes and must not resume the old affect iterator. */
    AFFECT_DATA copy=*guardian,other={0};
    affect_remove(pet,guardian);
    other.type=skill_lookup("armor");other.duration=99;other.location=APPLY_NONE;
    affect_to_char(pet,&other);affect_to_char(pet,&copy);
    guardian=pet->affected;guardian->duration=1;
    ROOM_INDEX_DATA remote={0};remote.name="Remote spider expiry";
    remote.description="";remote.area=&area;remote.light=1;
    free_string(pet->name);pet->name=str_dup("renamedspider");
    char_from_room(pet);char_to_room(pet,&remote);
    char_update();
    assert(guardian->duration==0&&pet->master==actor&&IS_AFFECTED(pet,AFF_CHARM));
    assert(actor->pcdata->followers==1);
    char_update();
    bool present=FALSE;
    for(CHAR_DATA *live=char_list;live;live=live->next)if(live==pet)present=TRUE;
    assert(!present&&actor->pcdata->followers==0&&arena.people==actor&&!remote.people);
    assert(actor->mana==mana&&actor->primal==primal&&
           actor->pcdata->classpoints[CP_CURRENT]==points);
    command("summonspider");
    pet=arena.people;
    assert(pet!=actor&&pet->master==actor&&IS_AFFECTED(pet,AFF_CHARM));
    assert(actor->pcdata->followers==1);
    extract_char(pet,TRUE);

    /* A generic guardian has no Drow stamp and keeps its old lifetime policy. */
    spell_guardian(sn,50,actor,actor);
    pet=arena.people;
    assert(pet!=actor&&pet->master==actor&&actor->pcdata->followers==1);
    guardian=NULL;
    for(AFFECT_DATA *af=pet->affected;af;af=af->next)if(af->type==sn)guardian=af;
    assert(guardian&&guardian->modifier==0);guardian->duration=0;
    char_update();
    assert(arena.people==pet&&!IS_AFFECTED(pet,AFF_CHARM));
    assert(actor->pcdata->followers==1);
    extract_char(pet,TRUE);

    /* Neither a different spell with the same modifier nor a player carrying
     * the stamped effect can be mistaken for the summoned NPC. */
    pet=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
    pet->act=ACT_IS_NPC;pet->hit=pet->max_hit=1000;pet->position=POS_STANDING;
    char_to_room(pet,&arena);
    other.type=gsn_charm_person;other.duration=0;other.modifier=DROW_SPIDER;
    other.bitvector=AFF_CHARM;affect_to_char(pet,&other);
    char_update();assert(arena.people==pet&&!pet->affected);
    extract_char(pet,TRUE);
    other.type=sn;affect_to_char(actor,&other);
    char_update();assert(char_list==actor&&arena.people==actor&&!actor->affected);
    assert(actor->pcdata->followers==0&&char_list==actor&&arena.people==actor);
    actor->next=world_tail;actor->timer=old_timer;
    puts("Native Drow timed effects: Faerie Fire/Web restore stats; expired spider vanishes, releases its slot and permits resummoning.");
}

/* Rejected commands must not spend unrelated pools or start a wait.
 * Real interpreter/world objects, not permission or resource mocks. */
static void rejection_cost_check(const char *text)
{
    long points=actor->pcdata->classpoints[CP_CURRENT],quest=actor->pcdata->quest;
    long mana=actor->mana,hit=actor->hit,exp=actor->exp,primal=actor->primal;
    long ki=actor->pcdata->ninja[NINJA_KI],powers=actor->pcdata->drow[DROW_POWERS];
    long speed=actor->pcdata->drow[DROW_SPEED],followers=actor->pcdata->followers;
    long count=actor->carry_number,weight=actor->carry_weight;
    ROOM_INDEX_DATA *room=actor->in_room;
    command(text);
    assert(output[0] && actor->wait==0);
    assert(actor->pcdata->classpoints[CP_CURRENT]==points&&actor->pcdata->quest==quest);
    assert(actor->mana==mana&&actor->hit==hit&&actor->exp==exp&&actor->primal==primal);
    assert(actor->pcdata->ninja[NINJA_KI]==ki);
    assert(actor->pcdata->drow[DROW_POWERS]==powers&&actor->pcdata->drow[DROW_SPEED]==speed);
    assert(actor->pcdata->followers==followers&&actor->carry_number==count&&
           actor->carry_weight==weight&&actor->in_room==room);
}

static void equipment_refusal_boundaries(void)
{
    assert(!actor->carrying&&actor->carry_number==0&&actor->carry_weight==0);
    long dp=actor->pcdata->classpoints[CP_CURRENT],qp=actor->pcdata->quest;
    const char *invalid[]={"0","-1","+1","21","1x","1 2","999999999999999999999999999999999999"};
    actor->pcdata->classpoints[CP_CURRENT]=LONG_MAX;
    actor->pcdata->quest=LONG_MAX;
    for(size_t i=0;i<sizeof invalid/sizeof *invalid;++i) {
        char text[128];snprintf(text,sizeof text,"drowarmor bolts %s",invalid[i]);
        rejection_cost_check(text);
        assert(!actor->carrying);
    }
    const long poor[]={LONG_MIN,-1,0,99};
    for(size_t i=0;i<sizeof poor/sizeof *poor;++i) {
        actor->pcdata->classpoints[CP_CURRENT]=poor[i];
        rejection_cost_check("drowarmor bolts");
        assert(!actor->carrying);
    }
    actor->pcdata->classpoints[CP_CURRENT]=LONG_MAX;
    OBJ_INDEX_DATA *p=get_obj_index(OBJ_VNUM_DROW_BOLT);assert(p);
    long type=p->item_type,flags=p->wear_flags,weight=p->weight,spell=p->value[3];
    p->item_type=ITEM_TRASH;rejection_cost_check("drowarmor bolts");p->item_type=type;
    p->wear_flags=0;rejection_cost_check("drowarmor bolts");p->wear_flags=flags;
    p->weight=-1;rejection_cost_check("drowarmor bolts");p->weight=weight;
    p->value[3]=-1;rejection_cost_check("drowarmor bolts");p->value[3]=spell;
    actor->carry_number=LONG_MAX;rejection_cost_check("drowarmor bolts 2");
    actor->carry_number=-1;rejection_cost_check("drowarmor bolts");
    actor->carry_number=0;actor->carry_weight=LONG_MAX;
    /* Force a positive weight without relying on future template edits. */
    p->weight=1;rejection_cost_check("drowarmor bolts 2");
    actor->carry_weight=-1;rejection_cost_check("drowarmor bolts");
    actor->carry_weight=0;p->weight=weight;
    assert(!actor->carrying);
    const int batches[]={1,20};
    for(size_t i=0;i<sizeof batches/sizeof *batches;++i) {
        char text[80];snprintf(text,sizeof text,"drowarmor bolts %d",batches[i]);
        actor->pcdata->classpoints[CP_CURRENT]=batches[i]*100;
        actor->pcdata->quest=357;
        command(text);
        assert(actor->pcdata->classpoints[CP_CURRENT]==0&&actor->pcdata->quest==357);
        assert(projectile_count()==batches[i]&&actor->wait==0);
        for(OBJ_DATA *o=actor->carrying;o;o=o->next_content)
            assert(o->cost==0&&o->points==0&&!str_cmp(o->questmaker,actor->name)&&
                   (!o->questowner||!o->questowner[0]));
        while(actor->carrying)extract_obj(actor->carrying);
        assert(actor->carry_number==0&&actor->carry_weight==0);
    }

    /* Ownership is the bound owner, not the maker or present carrier. */
    OBJ_DATA *item=create_object(get_obj_index(29653),50);
    free_string(item->name);item->name=str_dup("auditedrelic");
    free_string(item->questmaker);item->questmaker=str_dup(actor->name);
    free_string(item->questowner);item->questowner=str_dup("OtherDrow");
    REMOVE_BIT(item->extra_flags,ITEM_LOYAL);
    obj_to_char(item,actor);
    long extras=item->extra_flags;
    rejection_cost_check("attunement auditedrelic");
    assert(item->extra_flags==extras&&!str_cmp(item->questowner,"OtherDrow"));
    free_string(item->questowner);item->questowner=str_dup("");
    rejection_cost_check("attunement auditedrelic");assert(item->extra_flags==extras);
    free_string(item->questowner);item->questowner=NULL;
    rejection_cost_check("attunement auditedrelic");assert(item->extra_flags==extras);
    free_string(item->questowner);item->questowner=str_dup(actor->name);
    const long excluded[]={ITEM_QUEST,ITEM_AMMO,ITEM_EGG,ITEM_VOODOO,ITEM_MONEY,ITEM_TREASURE,ITEM_PAGE};
    for(size_t i=0;i<sizeof excluded/sizeof *excluded;++i) {
        item->item_type=excluded[i];
        rejection_cost_check("attunement auditedrelic");assert(item->extra_flags==extras);
    }
    item->item_type=ITEM_ARMOR;
    long quest=item->quest;SET_BIT(item->quest,QUEST_ARTIFACT);
    rejection_cost_check("attunement auditedrelic");assert(item->extra_flags==extras);
    item->quest=quest;
    command("attunement auditedrelic");assert(IS_SET(item->extra_flags,ITEM_LOYAL));
    long loyal=item->extra_flags;
    rejection_cost_check("attunement auditedrelic");assert(item->extra_flags==loyal);
    extract_obj(item);
    assert(!actor->carrying&&actor->carry_number==0&&actor->carry_weight==0);
    actor->pcdata->classpoints[CP_CURRENT]=dp;actor->pcdata->quest=qp;
    puts("Native Drow equipment: invalid batches/templates/carry limits, exact crafting costs, ownership and attunement exclusions passed.");
}

static void rejected_costs(void)
{
    const char *missing[]={
        "acidbolt","acidbolt NoSuchDrowTarget","acidbolt self",
        "faeriefire","faeriefire NoSuchDrowTarget","web NoSuchDrowTarget",
        "attune","attune NoSuchDrowObject","dimensiondoor","dimensiondoor NoSuchDrowTarget",
        "prayer","prayer NoSuchDrowTarget speed","prayer self NotAPower",
        "prayer self acidbolt","prayer self speed",
        "drowarmor NotARelic","drowarmor bolts 0","drowarmor bolts 21",
        "drowarmor bolts -1","drowarmor bolts 9999999999999999999999999",
        "drowarmor bolts 2 extra","readaura NoSuchDrowTarget","scry NoSuchDrowTarget",
        "shoot north NoSuchDrowTarget"
    };
    long points=actor->pcdata->classpoints[CP_CURRENT];
    long powers=actor->pcdata->drow[DROW_POWERS],speed=actor->pcdata->drow[DROW_SPEED];
    actor->pcdata->classpoints[CP_CURRENT]=123456;
    for(size_t i=0;i<sizeof missing/sizeof *missing;++i)rejection_cost_check(missing[i]);
    const char *untrained[]={"acidbolt self","faeriefire self","web self",
        "summonspider","attune","skipdisc","globedarkness","darkvision","heighten",
        "readaura self","scry self"};
    actor->pcdata->drow[DROW_POWERS]=0;
    for(size_t i=0;i<sizeof untrained/sizeof *untrained;++i)
        rejection_cost_check(untrained[i]);
    actor->pcdata->drow[DROW_POWERS]=powers;
    actor->pcdata->classpoints[CP_CURRENT]=0;
    actor->pcdata->drow[DROW_SPEED]=0;
    const char *poor[]={"globedarkness","drowarmor helmet","drowarmor bolts",
        "prayer self speed","shoot north NoSuchDrowTarget"};
    for(size_t i=0;i<sizeof poor/sizeof *poor;++i)rejection_cost_check(poor[i]);
    long followers=actor->pcdata->followers;
    actor->pcdata->followers=1;
    rejection_cost_check("summonspider");
    actor->pcdata->followers=followers;
    actor->pcdata->drow[DROW_SPEED]=speed;
    actor->pcdata->classpoints[CP_CURRENT]=points;

    CHAR_DATA *target=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
    free_string(target->name);target->name=str_dup("RejectedDrowTarget");
    char_to_room(target,&arena);
    long flags=arena.room_flags;
    SET_BIT(arena.room_flags,ROOM_SAFE);
    rejection_cost_check("acidbolt RejectedDrowTarget");
    rejection_cost_check("faeriefire RejectedDrowTarget");
    rejection_cost_check("web RejectedDrowTarget");
    assert(!target->affected&&!actor->fighting);
    arena.room_flags=flags;
    command("faeriefire RejectedDrowTarget");
    assert(actor->wait==12&&IS_AFFECTED(target,AFF_FAERIE_FIRE));
    rejection_cost_check("faeriefire RejectedDrowTarget");
    command("web RejectedDrowTarget");
    assert(actor->wait==12&&IS_AFFECTED(target,AFF_WEBBED));
    /* Already-webbed is an attempted cast, not an input rejection.
     * Preserve the native wait, without charging DP or adding effects. */
    long before=actor->pcdata->classpoints[CP_CURRENT];
    AFFECT_DATA *affects=target->affected;
    command("web RejectedDrowTarget");
    assert(actor->wait==12&&actor->pcdata->classpoints[CP_CURRENT]==before&&
           target->affected==affects);
    extract_char(target,TRUE);
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

static void skipdisc_travel(void)
{
    long original_class=actor->class,original_powers=actor->pcdata->drow[DROW_POWERS];
    long original_affects=actor->affected_by,original_points=actor->pcdata->classpoints[CP_CURRENT];
    long original_sector=arena.sector_type;
    long mana=actor->mana,hit=actor->hit;
    const long classes[]={CLASS_DROW,CLASS_MAGE};
    const long points[]={0,123456};
    for(unsigned cls=0;cls<2;++cls)
        for(int learned=0;learned<2;++learned)
            for(int flying=0;flying<2;++flying)
                for(unsigned funds=0;funds<2;++funds) {
                    actor->class=classes[cls];
                    actor->pcdata->drow[DROW_POWERS]=learned?DROW_SKIP:0;
                    actor->affected_by=original_affects&~AFF_FLYING;
                    if(flying)SET_BIT(actor->affected_by,AFF_FLYING);
                    actor->pcdata->classpoints[CP_CURRENT]=points[funds];
                    long before_speed=get_speed(actor);
                    command("skipdisc");
                    bool permitted=cls==0&&learned;
                    assert(!!IS_AFFECTED(actor,AFF_FLYING)==(permitted||flying));
                    assert(actor->affected_by==((original_affects&~AFF_FLYING)|
                           (flying?AFF_FLYING:0)));
                    assert(actor->pcdata->drow[DROW_POWERS]==
                           ((learned?DROW_SKIP:0)|(permitted?DROW_DISC:0)));
                    assert(actor->pcdata->classpoints[CP_CURRENT]==points[funds]);
                    assert(actor->mana==mana&&actor->hit==hit&&get_speed(actor)==before_speed);
                    if(cls)assert(strstr(output,"Huh?"));
                    else if(!learned)assert(strstr(output,"obtain skip disc first"));
                    else {
                        assert(strstr(output,"leap onto"));
                        command("skipdisc");
                        assert(strstr(output,"leap off"));
                        assert(!DROW_DISC_ACTIVE(actor));
                        assert(!!IS_AFFECTED(actor,AFF_FLYING)==flying);
                    }
                }
    actor->class=CLASS_DROW;actor->pcdata->drow[DROW_POWERS]=original_powers|DROW_SKIP;
    actor->pcdata->classpoints[CP_CURRENT]=0;
    actor->affected_by=original_affects&~AFF_FLYING;
    /* The learned speed bonus is independent of whether flight is toggled on. */
    long learned_speed=get_speed(actor);
    REMOVE_BIT(actor->pcdata->drow[DROW_POWERS],DROW_SKIP);
    assert(get_speed(actor)==learned_speed+3);
    SET_BIT(actor->pcdata->drow[DROW_POWERS],DROW_SKIP);
    for(int flying=0;flying<2;++flying) {
        if(flying)SET_BIT(actor->pcdata->drow[DROW_POWERS],DROW_DISC);
        else REMOVE_BIT(actor->pcdata->drow[DROW_POWERS],DROW_DISC);
        assert(save_char_obj_checked(actor));
        DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
        assert(reload.character->class==CLASS_DROW);
        assert(get_drow(reload.character,DROW_SKIP));
        assert(!!IS_AFFECTED(reload.character,AFF_FLYING)==flying);
        assert(!!DROW_DISC_ACTIVE(reload.character)==flying);
        assert(!IS_SET(reload.character->affected_by,AFF_FLYING));
        assert(reload.character->pcdata->classpoints[CP_CURRENT]==0);
        free_char(reload.character);
    }
    REMOVE_BIT(actor->pcdata->drow[DROW_POWERS],DROW_DISC);
    REMOVE_BIT(actor->affected_by,AFF_FLYING);
    const char *directions[]={"north","east","south","west","up","down"};
    const int reverse[]={2,3,0,1,5,4};
    const long sectors[]={SECT_AIR,SECT_WATER_NOSWIM};
    for(unsigned terrain=0;terrain<2;++terrain)
        for(int dir=0;dir<6;++dir) {
            ROOM_INDEX_DATA destination={0};EXIT_DATA outward={0},homeward={0};
            destination.vnum=999904;destination.name="Skip Disc travel audit";
            destination.description="";destination.area=&area;destination.light=1;
            destination.sector_type=sectors[terrain];arena.sector_type=SECT_INSIDE;
            EXIT_DATA *saved=arena.exit[dir];outward.to_room=&destination;
            homeward.to_room=&arena;arena.exit[dir]=&outward;
            destination.exit[reverse[dir]]=&homeward;
            command(directions[dir]);assert(actor->in_room==&arena);
            assert(strstr(output,terrain?"need a boat":"can't fly"));
            command("skipdisc");assert(IS_AFFECTED(actor,AFF_FLYING));
            command(directions[dir]);assert(actor->in_room==&destination);
            command("skipdisc");assert(!IS_AFFECTED(actor,AFF_FLYING));
            command(directions[reverse[dir]]);assert(actor->in_room==&destination);
            assert(strstr(output,terrain?"need a boat":"can't fly"));
            command("skipdisc");assert(IS_AFFECTED(actor,AFF_FLYING));
            command(directions[reverse[dir]]);assert(actor->in_room==&arena);
            command("skipdisc");assert(!IS_AFFECTED(actor,AFF_FLYING));
            assert(destination.people==NULL);
            assert(actor->pcdata->classpoints[CP_CURRENT]==0&&actor->mana==mana&&actor->hit==hit);
            assert(get_speed(actor)==learned_speed);
            arena.exit[dir]=saved;
        }
    actor->class=original_class;actor->pcdata->drow[DROW_POWERS]=original_powers;
    actor->affected_by=original_affects;actor->pcdata->classpoints[CP_CURRENT]=original_points;
    arena.sector_type=original_sector;
    puts("Native Drow Skip Disc: class/training gates, zero-DP toggles, saved flight, "
         "learned speed bonus and all six air/deep-water exit directions passed.");
}

/* Independent flight sources must survive one another's removal. */
static void skipdisc_ownership_probe(void)
{
    long powers=actor->pcdata->drow[DROW_POWERS],flags=actor->affected_by;
    actor->pcdata->drow[DROW_POWERS]=DROW_SKIP;
    REMOVE_BIT(actor->affected_by,AFF_FLYING);
    long fly=skill_lookup("fly");assert(fly>=0);
    spell_fly(fly,20,actor,actor);
    assert(is_affected(actor,fly)&&IS_AFFECTED(actor,AFF_FLYING));
    command("skipdisc");
    assert(is_affected(actor,fly)&&IS_AFFECTED(actor,AFF_FLYING)&&DROW_DISC_ACTIVE(actor));
    command("skipdisc");
    assert(is_affected(actor,fly)&&IS_AFFECTED(actor,AFF_FLYING)&&!DROW_DISC_ACTIVE(actor));
    command("skipdisc");
    affect_strip(actor,fly);
    assert(DROW_DISC_ACTIVE(actor)&&IS_AFFECTED(actor,AFF_FLYING));
    assert(!IS_SET(actor->affected_by,AFF_FLYING));
    command("skipdisc");assert(!IS_AFFECTED(actor,AFF_FLYING));

    OBJ_DATA *item=create_object(get_obj_index(29653),50);
    item->item_type=ITEM_ARMOR;item->value[3]=39;
    obj_to_char(item,actor);equip_char(actor,item,WEAR_HEAD);
    assert(get_eq_char(actor,WEAR_HEAD)==item&&IS_AFFECTED(actor,AFF_FLYING));
    command("skipdisc");
    assert(get_eq_char(actor,WEAR_HEAD)==item&&DROW_DISC_ACTIVE(actor));
    command("skipdisc");
    assert(!DROW_DISC_ACTIVE(actor)&&IS_AFFECTED(actor,AFF_FLYING));
    unequip_char(actor,item);extract_obj(item);

    command("skipdisc");assert(IS_AFFECTED(actor,AFF_FLYING));
    item=create_object(get_obj_index(29653),50);
    item->item_type=ITEM_ARMOR;item->value[3]=39;
    obj_to_char(item,actor);equip_char(actor,item,WEAR_HEAD);
    unequip_char(actor,item);extract_obj(item);
    assert(DROW_DISC_ACTIVE(actor)&&IS_AFFECTED(actor,AFF_FLYING));
    command("skipdisc");assert(!IS_AFFECTED(actor,AFF_FLYING));

    /* Equipping while the disc is active must still establish item flight. */
    for(int spell=3;spell<=39;spell+=36) {
        command("skipdisc");
        item=create_object(get_obj_index(29653),50);
        item->item_type=ITEM_ARMOR;item->value[3]=spell;
        /* Native saves omit unowned worn objects; use real owned equipment. */
        free_string(item->questowner);item->questowner=str_dup(actor->name);
        obj_to_char(item,actor);equip_char(actor,item,WEAR_HEAD);
        assert(save_char_obj_checked(actor));
        DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
        OBJ_DATA *worn=get_eq_char(reload.character,WEAR_HEAD);
        assert(worn&&DROW_DISC_ACTIVE(reload.character));
        do_skipdisc(reload.character,"");
        assert(!DROW_DISC_ACTIVE(reload.character)&&
               IS_AFFECTED(reload.character,AFF_FLYING));
        unequip_char(reload.character,worn);
        assert(!IS_AFFECTED(reload.character,AFF_FLYING));
        free_char(reload.character);
        command("skipdisc");
        assert(!DROW_DISC_ACTIVE(actor)&&IS_AFFECTED(actor,AFF_FLYING));
        unequip_char(actor,item);extract_obj(item);
        assert(!IS_AFFECTED(actor,AFF_FLYING));
    }

    REMOVE_BIT(actor->affected_by,AFF_FLYING);
    command("skipdisc");assert(IS_AFFECTED(actor,AFF_FLYING));
    REMOVE_BIT(actor->pcdata->drow[DROW_POWERS],DROW_SKIP);
    command("skipdisc");
    assert(!IS_AFFECTED(actor,AFF_FLYING)&&!DROW_DISC_ACTIVE(actor));
    command("skipdisc");
    assert(strstr(output,"obtain skip disc")&&!IS_AFFECTED(actor,AFF_FLYING));
    actor->pcdata->drow[DROW_POWERS]=powers;actor->affected_by=flags;
    puts("Skip Disc ownership: spell/item removal, independent dismissal and training-loss cleanup passed.");
}

static void skipdisc_source_guards(void)
{
    long powers=actor->pcdata->drow[DROW_POWERS],flags=actor->affected_by,cls=actor->class;
    long points=actor->pcdata->classpoints[CP_CURRENT];
    HYBRID_MASTERY before,after;
    actor->pcdata->drow[DROW_POWERS]=DROW_SKIP;
    REMOVE_BIT(actor->affected_by,AFF_FLYING);
    assert(hybrid_mastery_read(actor,&before));
    command("skipdisc");
    assert(hybrid_mastery_read(actor,&after));
    assert(before.count==after.count&&before.applicable==after.applicable&&
           before.met==after.met&&before.review==after.review);
    for(size_t i=0;i<before.count;++i)
        assert(before.items[i].current==after.items[i].current&&
               before.items[i].status==after.items[i].status);
    assert(!hybrid_power_drow(actor,DROW_DISC)); /* Active state is never training. */
    SET_BIT(actor->affected_by,AFF_BLIND);
    assert(IS_AFFECTED(actor,AFF_FLYING|AFF_BLIND)==(AFF_FLYING|AFF_BLIND));
    assert(IS_AFFECTED(actor,AFF_BLIND)==AFF_BLIND);
    actor->class=CLASS_MAGE;
    assert(!DROW_DISC_ACTIVE(actor)&&!IS_AFFECTED(actor,AFF_FLYING));
    actor->class=CLASS_DROW;
    REMOVE_BIT(actor->pcdata->drow[DROW_POWERS],DROW_SKIP);
    assert(DROW_DISC_ACTIVE(actor));
    actor->affected_by=flags&~AFF_FLYING;
    assert(save_char_obj_checked(actor));
    DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
    assert(DROW_DISC_ACTIVE(reload.character)&&
           !get_drow(reload.character,DROW_SKIP));
    do_skipdisc(reload.character,"");
    assert(!DROW_DISC_ACTIVE(reload.character)&&
           !IS_AFFECTED(reload.character,AFF_FLYING));
    assert(reload.character->pcdata->classpoints[CP_CURRENT]==points);
    free_char(reload.character);
    command("skipdisc");assert(!IS_AFFECTED(actor,AFF_FLYING));
    do_skipdisc(NULL,NULL);
    CHAR_DATA invalid={0};invalid.class=CLASS_DROW;
    do_skipdisc(&invalid,NULL);assert(!DROW_DISC_ACTIVE(&invalid));
    PC_DATA npcpc={0};invalid.pcdata=&npcpc;invalid.act=ACT_IS_NPC;
    npcpc.drow[DROW_POWERS]=DROW_DISC|DROW_SKIP;
    do_skipdisc(&invalid,NULL);assert(!DROW_DISC_ACTIVE(&invalid));
    actor->class=cls;actor->pcdata->drow[DROW_POWERS]=powers;
    actor->affected_by=flags;
    puts("Skip Disc: native-only/null/NPC guards, bitmask semantics and saved training-loss cleanup passed.");
}

static void skipdisc_blocked_movement(void)
{
    long powers=actor->pcdata->drow[DROW_POWERS],flags=actor->affected_by;
    long sector=arena.sector_type,points=actor->pcdata->classpoints[CP_CURRENT];
    long mana=actor->mana,hit=actor->hit;
    const char *directions[]={"north","east","south","west","up","down"};
    const long terrain[]={SECT_AIR,SECT_WATER_NOSWIM};
    const long barriers[]={EX_CLOSED,EX_WALL_WATER,EX_WALL_GRANITE};
    actor->pcdata->drow[DROW_POWERS]=DROW_SKIP;
    SET_BIT(actor->pcdata->drow[DROW_POWERS],DROW_DISC);
    for(unsigned t=0;t<2;++t)for(int dir=0;dir<6;++dir) {
        ROOM_INDEX_DATA destination={0};EXIT_DATA exit={0};
        CHAR_DATA first={0},second={0};
        EXIT_DATA *saved=arena.exit[dir];
        destination.name="Blocked Skip Disc audit";destination.description="";
        destination.area=&area;destination.light=1;destination.sector_type=terrain[t];
        arena.sector_type=SECT_INSIDE;
        exit.to_room=&destination;exit.keyword="audit door";
        for(int mode=0;mode<6;++mode) {
            arena.exit[dir]=mode==0?NULL:&exit;
            exit.exit_info=mode>=1&&mode<=3?barriers[mode-1]:0;
            destination.room_flags=mode==4?ROOM_SOLITARY:mode==5?ROOM_PRIVATE:0;
            destination.people=mode>=4?&first:NULL;
            first.next_in_room=mode==5?&second:NULL;
            command(directions[dir]);
            assert(actor->in_room==&arena&&IS_AFFECTED(actor,AFF_FLYING));
            assert(actor->wait==0&&actor->pcdata->classpoints[CP_CURRENT]==points);
            assert(actor->mana==mana&&actor->hit==hit);
        }
        destination.people=NULL;arena.exit[dir]=saved;
    }
    arena.sector_type=sector;actor->affected_by=flags;
    actor->pcdata->drow[DROW_POWERS]=powers;
    puts("Skip Disc: 72 blocked exits/barriers/private-room movements preserve flight, room and resources.");
}

static void darkness_pulse(void)
{
    assert(char_list==actor);
    CHAR_DATA *tail=actor->next;
    actor->next=NULL;
    mobile_update();
    actor->next=tail;
}

static void darkness_lifecycle(void)
{
    long powers=actor->pcdata->drow[DROW_POWERS];
    long points=actor->pcdata->classpoints[CP_CURRENT];
    long room_flags=arena.room_flags,added_flags=arena.added_flags,light=arena.light;
    long mana=actor->mana,hit=actor->hit,quest=actor->pcdata->quest;
    const long funds[]={LONG_MIN,-1,0,1,149,150,151,499,500,650,651,LONG_MAX};
    for(size_t i=0;i<sizeof funds/sizeof *funds;++i) {
        actor->pcdata->drow[DROW_POWERS]=DROW_DARK;
        actor->pcdata->classpoints[CP_CURRENT]=funds[i];
        command("globedarkness");
        assert(!!get_drow(actor,DROW_GLOBE)==(funds[i]>=500));
        assert(actor->pcdata->classpoints[CP_CURRENT]==
               (funds[i]>=500?funds[i]-500:funds[i]));
        assert(actor->wait==0&&actor->mana==mana&&actor->hit==hit);
        if(funds[i]>=500) {
            long after=actor->pcdata->classpoints[CP_CURRENT];
            command("globedarkness");
            assert(!get_drow(actor,DROW_GLOBE)&&
                   actor->pcdata->classpoints[CP_CURRENT]==after&&actor->wait==0);
        }
    }
    /* All signed balances are safe, including the formerly overflowing minimum. */
    for(size_t i=0;i<sizeof funds/sizeof *funds;++i) {
        actor->pcdata->drow[DROW_POWERS]=DROW_DARK|DROW_GLOBE;
        actor->pcdata->classpoints[CP_CURRENT]=funds[i];
        darkness_pulse();
        assert(actor->pcdata->classpoints[CP_CURRENT]==
               (funds[i]>150?funds[i]-150:0));
        assert(!!get_drow(actor,DROW_GLOBE)==(funds[i]>150));
        assert(actor->mana==mana&&actor->hit==hit&&actor->pcdata->quest==quest);
    }
    /* Movement follows the caster without borrowing room darkness flags. */
    ROOM_INDEX_DATA destination={0};
    destination.vnum=999905;destination.name="Darkness lifecycle audit";
    destination.description="";destination.area=&area;destination.light=1;
    actor->pcdata->drow[DROW_POWERS]=DROW_DARK|DROW_GLOBE;
    actor->pcdata->classpoints[CP_CURRENT]=1000;
    char_from_room(actor);char_to_room(actor,&destination);
    assert(get_drow(actor,DROW_GLOBE)&&destination.added_flags==0&&destination.light==1);
    char_from_room(actor);char_to_room(actor,&arena);
    assert(!destination.people&&arena.added_flags==added_flags&&arena.light==light);
    assert(arena.room_flags==room_flags&&actor->pcdata->classpoints[CP_CURRENT]==1000);
    assert(save_char_obj_checked(actor));
    DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
    assert(get_drow(reload.character,DROW_GLOBE)&&
           reload.character->pcdata->classpoints[CP_CURRENT]==1000);
    free_char(reload.character);

    /* Native upkeep only runs for a playing descriptor; disconnected
     * characters retain the globe and balance in the current policy. */
    actor->desc=NULL;
    darkness_pulse();
    assert(get_drow(actor,DROW_GLOBE)&&actor->pcdata->classpoints[CP_CURRENT]==1000);
    actor->desc=&descriptor;
    actor->hit=0;update_pos(actor);
    assert(!get_drow(actor,DROW_GLOBE));
    actor->hit=hit;actor->position=POS_STANDING;
    actor->pcdata->drow[DROW_POWERS]=powers;
    actor->pcdata->classpoints[CP_CURRENT]=points;
    puts("Darkness: 12 activation boundaries, 12 upkeep boundaries, movement, save/reload, disconnect and zero-HP cleanup passed.");
}

static void darkness_loss_cleanup(void)
{
    long powers=actor->pcdata->drow[DROW_POWERS];
    long points=actor->pcdata->classpoints[CP_CURRENT];
    long mana=actor->mana,hit=actor->hit,primal=actor->primal,quest=actor->pcdata->quest;
    long added=arena.added_flags,light=arena.light;
    const long funds[]={LONG_MIN,-1,0,499,500,1000,LONG_MAX};
    for(size_t i=0;i<sizeof funds/sizeof *funds;++i) {
        actor->pcdata->drow[DROW_POWERS]=DROW_SKIP|DROW_GLOBE;
        actor->pcdata->classpoints[CP_CURRENT]=funds[i];
        command("globedarkness");
        assert(strstr(output,"banish your globe"));
        assert(actor->pcdata->drow[DROW_POWERS]==DROW_SKIP);
        assert(actor->pcdata->classpoints[CP_CURRENT]==funds[i]&&actor->wait==0);
        assert(actor->mana==mana&&actor->hit==hit&&actor->primal==primal&&
               actor->pcdata->quest==quest&&arena.added_flags==added&&arena.light==light);
        /* Dismissal is not a new grant: a repeated command must reject even
         * when there is enough DP, and subsequent upkeep must charge nothing. */
        command("globedarkness");
        assert(strstr(output,"obtain darkness first"));
        assert(actor->pcdata->drow[DROW_POWERS]==DROW_SKIP&&actor->wait==0);
        darkness_pulse();
        assert(actor->pcdata->classpoints[CP_CURRENT]==funds[i]);
        assert(actor->pcdata->drow[DROW_POWERS]==DROW_SKIP);
    }
    actor->pcdata->classpoints[CP_CURRENT]=1000;
    assert(save_char_obj_checked(actor));
    DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
    assert(reload.character->pcdata->drow[DROW_POWERS]==DROW_SKIP);
    assert(reload.character->pcdata->classpoints[CP_CURRENT]==1000);
    free_char(reload.character);
    actor->pcdata->drow[DROW_POWERS]=powers;
    actor->pcdata->classpoints[CP_CURRENT]=points;
    puts("Darkness training loss: free dismissal, denied reactivation, no later upkeep and saved cleanup passed.");
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

/* Sign is a class channel, not a house-private channel. Exercise actual
 * descriptor routing as well as the interpreter and persisted Deaf field. */
static void sign_message(const char *text,DESCRIPTOR_DATA *peers,
                         unsigned recipients,bool allowed)
{
    for(int i=0;i<4;++i) {
        peers[i].outtop=0;peers[i].outbuf[0]='\0';
    }
    house_input(actor,text);
    assert((strstr(output,"audit-message")!=NULL)==allowed);
    for(int i=0;i<4;++i)
        assert((strstr(peers[i].outbuf,"audit-message")!=NULL)==
               ((recipients&(1u<<i))!=0));
}

static void communication(void)
{
    DESCRIPTOR_DATA peers[4]={0},*saved_descriptors=descriptor_list;
    char buffers[4][16384]={0};
    const char *names[]={"SignMember","SignRival","SignMage","SignStaff"};
    CHAR_DATA *people[4];
    ROOM_INDEX_DATA remote={0};
    long deaf=actor->deaf,position=actor->position;
    long powers=actor->pcdata->drow[DROW_POWERS];
    long speed=actor->pcdata->drow[DROW_SPEED];
    long points=actor->pcdata->classpoints[CP_CURRENT];
    char *old_house=actor->clan;actor->clan=str_dup("AuditHouse");
    remote.vnum=999902;remote.name="Remote sign audit";remote.description="";
    remote.light=1;remote.area=&area;
    actor->wait=0;actor->deaf=0;
    assert(saved_descriptors==NULL);
    descriptor_list=&descriptor;descriptor.next=&peers[0];
    for(int i=0;i<4;++i) {
        assert(!load_char_obj(&peers[i],(char *)names[i]));
        peers[i].outbuf=buffers[i];peers[i].outsize=sizeof buffers[i];
        peers[i].connected=CON_PLAYING;people[i]=peers[i].character;
        peers[i].next=i<3?&peers[i+1]:NULL;
        people[i]->class=i<2?CLASS_DROW:CLASS_MAGE;
        people[i]->level=i==3?LEV_IMM:LEV_AWA;
        people[i]->position=POS_STANDING;people[i]->desc=&peers[i];
        people[i]->deaf=0;
        free_string(people[i]->clan);
        people[i]->clan=str_dup(i==0?"AuditHouse":i==1?"RivalHouse":"");
        people[i]->next=char_list;char_list=people[i];char_to_room(people[i],&arena);
    }
    sign_message("sign audit-message",peers,11,TRUE);
    house_input(people[2],"sign denied-mage");
    assert(strstr(buffers[2],"aren't on that channel"));
    house_input(people[3],"sign staff-message");
    assert(strstr(buffers[3],"staff-message")&&strstr(output,"staff-message"));
    /* Different house, room, plane and sleep do not change global class access. */
    char_from_room(people[1]);char_to_room(people[1],&remote);
    SET_BIT(people[1]->affected_by,AFF_SHADOWPLANE);
    people[1]->position=POS_SLEEPING;actor->position=POS_SLEEPING;
    sign_message("sign audit-message",peers,11,TRUE);
    assert(actor->position==POS_SLEEPING&&people[1]->position==POS_SLEEPING);
    actor->position=POS_STANDING;people[1]->position=POS_STANDING;
    REMOVE_BIT(people[1]->affected_by,AFF_SHADOWPLANE);
    char_from_room(people[1]);char_to_room(people[1],&arena);
    house_input(actor,"channels");assert(strstr(output,"+SIGN"));
    house_input(actor,"channels -sign");assert(IS_SET(actor->deaf,CHANNEL_DROW));
    house_input(actor,"channels");assert(strstr(output,"-sign"));
    sign_message("sign audit-message",peers,0,FALSE);
    assert(save_char_obj_checked(actor));
    DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"NativeDrow"));
    assert(IS_SET(reload.character->deaf,CHANNEL_DROW));
    assert(reload.character->pcdata->drow[DROW_POWERS]==powers&&
           reload.character->pcdata->drow[DROW_SPEED]==speed);
    free_char(reload.character);
    house_input(actor,"channels +sign");assert(!IS_SET(actor->deaf,CHANNEL_DROW));
    house_input(people[0],"channels -sign");
    house_input(people[3],"channels -sign");
    sign_message("sign audit-message",peers,2,TRUE);
    assert(save_char_obj_checked(people[0]));
    assert(load_char_obj(&reload,"SignMember"));
    assert(IS_SET(reload.character->deaf,CHANNEL_DROW));
    free_char(reload.character);
    house_input(people[0],"channels +sign");house_input(people[3],"channels +sign");
    house_input(actor,"channels -all");
    sign_message("sign audit-message",peers,0,FALSE);
    house_input(actor,"channels +all");
    sign_message("sign audit-message",peers,11,TRUE);
    /* Silent/gagged senders cannot transmit; mute and login state gate receipt. */
    SET_BIT(actor->extra,GAGGED);
    sign_message("sign audit-message",peers,0,FALSE);
    REMOVE_BIT(actor->extra,GAGGED);SET_BIT(actor->act,PLR_SILENCE);
    sign_message("sign audit-message",peers,0,FALSE);
    REMOVE_BIT(actor->act,PLR_SILENCE);SET_BIT(arena.added_flags,ROOM2_SILENCE);
    sign_message("sign audit-message",peers,0,FALSE);
    REMOVE_BIT(arena.added_flags,ROOM2_SILENCE);
    peers[1].connected=CON_GET_OLD_PASSWORD;
    sign_message("sign audit-message",peers,9,TRUE);
    peers[1].connected=CON_PLAYING;
    house_input(actor,"sign");assert(strstr(output,"Sign what?"));
    sign_message("sign audit-message $n $N #Rliteral#n",peers,11,TRUE);
    assert(strstr(output,"$n $N")); /* Player message is not a format string. */
    house_input(actor,"channels -sign");house_input(actor,"channels +sign");
    assert(save_char_obj_checked(actor));
    assert(load_char_obj(&reload,"NativeDrow"));
    assert(!IS_SET(reload.character->deaf,CHANNEL_DROW));
    assert(reload.character->pcdata->classpoints[CP_CURRENT]==points);
    free_char(reload.character);
    assert(actor->pcdata->drow[DROW_POWERS]==powers&&
           actor->pcdata->drow[DROW_SPEED]==speed&&
           actor->pcdata->classpoints[CP_CURRENT]==points);
    descriptor_list=saved_descriptors;descriptor.next=NULL;
    for(int i=0;i<4;++i)extract_char(people[i],TRUE);
    free_string(actor->clan);actor->clan=old_house;
    actor->deaf=deaf;actor->position=position;
    puts("Native Drow sign: class/staff routing across houses, mute persistence, "
         "sender silence, login state and literal message checks passed.");
}

static void door_check(const char *text,ROOM_INDEX_DATA *destination,bool moved)
{
    if(actor->in_room!=&arena){char_from_room(actor);char_to_room(actor,&arena);}
    long points=actor->pcdata->classpoints[CP_CURRENT];
    long mana=actor->mana,hit=actor->hit;
    long powers=actor->pcdata->drow[DROW_POWERS];
    command(text);
    assert(actor->in_room==(moved?destination:&arena));
    assert((strstr(output,"You step through a dimensional door.")!=NULL)==moved);
    assert(actor->pcdata->classpoints[CP_CURRENT]==points&&
           actor->pcdata->drow[DROW_POWERS]==powers&&actor->mana==mana&&actor->hit==hit);
    unsigned occurrences=0;
    for(CHAR_DATA *p=actor->in_room->people;p;p=p->next_in_room)
        if(p==actor)++occurrences;
    assert(occurrences==1);
}

static void dimensional_travel(void)
{
    DESCRIPTOR_DATA target_session={0};char target_output[16384]={0};
    assert(!load_char_obj(&target_session,"DoorTarget"));
    CHAR_DATA *target=target_session.character;
    target->class=CLASS_MONK;target->pcdata->monk[MONK_MANTRA]=6;
    target->level=LEV_AWA;target->position=POS_STANDING;
    target->desc=&target_session;target_session.connected=CON_PLAYING;
    target_session.outbuf=target_output;target_session.outsize=sizeof target_output;
    target->next=char_list;char_list=target;
    ROOM_INDEX_DATA destination={0};
    destination.vnum=999903;destination.name="Drow travel audit";destination.description="";
    destination.area=&area;destination.light=1;
    char_to_room(target,&destination);
    long points=actor->pcdata->classpoints[CP_CURRENT];
    door_check("dimensiondoor",&destination,FALSE);
    door_check("dimensiondoor AbsentDoorTarget",&destination,FALSE);
    door_check("dimensiondoor NativeDrow",&destination,FALSE);
    house_input(target,"summon off");
    assert(!IS_EXTRA(target,EXTRA_SUMMON));
    door_check("dimensiondoor DoorTarget",&destination,FALSE);
    house_input(target,"spiritguardian");
    assert(IS_MORE(target,MORE_GUARDIAN));
    const long flags[]={ROOM_SAFE,ROOM_PRIVATE,ROOM_SOLITARY};
    /* Exhaust every combination, not just each protection in isolation. */
    for(unsigned rooms=0;rooms<8;++rooms)
        for(int illusion=0;illusion<2;++illusion)
            for(int consent=0;consent<2;++consent) {
                destination.room_flags=0;
                for(unsigned bit=0;bit<3;++bit)
                    if(rooms&(1u<<bit))SET_BIT(destination.room_flags,flags[bit]);
                if(illusion)SET_BIT(target->extra,EXTRA_ILLUSION);
                else REMOVE_BIT(target->extra,EXTRA_ILLUSION);
                if(consent)SET_BIT(target->extra,EXTRA_SUMMON);
                else REMOVE_BIT(target->extra,EXTRA_SUMMON);
                target_session.outtop=0;target_output[0]='\0';
                door_check("dimensiondoor DoorTarget",&destination,
                           rooms==0&&!illusion&&consent);
                if(rooms||illusion||!consent)
                    assert(strstr(target_output,"failed to open a Dimensional Door"));
            }
    destination.room_flags=0;REMOVE_BIT(target->extra,EXTRA_ILLUSION);
    SET_BIT(target->extra,EXTRA_SUMMON);
    char_from_room(target);char_to_room(target,&arena);
    door_check("dimensiondoor DoorTarget",&destination,FALSE);
    char_from_room(target);char_to_room(target,&destination);
    target->level=801;door_check("dimensiondoor DoorTarget",&destination,FALSE);
    target->level=800;door_check("dimensiondoor DoorTarget",&destination,TRUE);
    target->level=LEV_AWA;
    SET_BIT(actor->extra,BLINDFOLDED);
    door_check("dimensiondoor DoorTarget",&destination,FALSE);
    REMOVE_BIT(actor->extra,BLINDFOLDED);
    SET_BIT(target->affected_by,AFF_SHADOWPLANE);
    door_check("dimensiondoor DoorTarget",&destination,FALSE);
    REMOVE_BIT(target->affected_by,AFF_SHADOWPLANE);
    actor->class=CLASS_MAGE;
    door_check("dimensiondoor DoorTarget",&destination,FALSE);
    actor->class=CLASS_DROW;
    /* Consent is a normal player preference, not an NPC-only travel fixture. */
    for(int consent=0;consent<2;++consent) {
        house_input(target,consent?"summon on":"summon off");
        assert(save_char_obj_checked(target));
        DESCRIPTOR_DATA reload={0};assert(load_char_obj(&reload,"DoorTarget"));
        assert(!!IS_EXTRA(reload.character,EXTRA_SUMMON)==consent);
        assert(reload.character->class==CLASS_MONK&&
               reload.character->pcdata->monk[MONK_MANTRA]==6&&
               IS_MORE(reload.character,MORE_GUARDIAN));
        /* Reloaded player is actually used for the next travel attempt. */
        extract_char(target,TRUE);target=reload.character;
        target_session.character=target;target_session.outtop=0;target_output[0]='\0';
        target->desc=&target_session;target->position=POS_STANDING;
        target->next=char_list;char_list=target;char_to_room(target,&destination);
        door_check("dimensiondoor DoorTarget",&destination,consent!=0);
    }
    /* NPCs need no summon preference, but eye mobs and protected rooms refuse. */
    CHAR_DATA *npc=create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
    free_string(npc->name);npc->name=str_dup("DoorMob");npc->level=1;
    char_to_room(npc,&destination);
    door_check("dimensiondoor DoorMob",&destination,TRUE);
    for(unsigned bit=0;bit<3;++bit) {
        destination.room_flags=flags[bit];
        door_check("dimensiondoor DoorMob",&destination,FALSE);
    }
    destination.room_flags=0;
    MOB_INDEX_DATA *eye=get_mob_index(MOB_VNUM_EYE);assert(eye);
    CHAR_DATA *eye_mob=create_mobile(eye);
    free_string(eye_mob->name);eye_mob->name=str_dup("DoorEye");eye_mob->level=1;
    char_to_room(eye_mob,&destination);
    door_check("dimensiondoor DoorEye",&destination,FALSE);
    assert(actor->pcdata->classpoints[CP_CURRENT]==points);
    if(actor->in_room!=&arena){char_from_room(actor);char_to_room(actor,&arena);}
    extract_char(eye_mob,TRUE);extract_char(npc,TRUE);extract_char(target,TRUE);
    assert(destination.people==NULL);
    puts("Native Drow dimensional travel: all destination/illusion/consent combinations, "
         "visibility, NPC exclusions, Guardian notices and real player reloads passed.");
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
        prayers();equipment();equipment_use();crossbow_death_cleanup();
    }
    else if(argc==2&&!strcmp(argv[1],"--equipment-boundaries")) {
        prayers();equipment_refusal_boundaries();spider_stat_boundaries(-1);
    }
    else if(argc==2&&(!strcmp(argv[1],"--spider-stat-hitroll")||
                     !strcmp(argv[1],"--spider-stat-damroll")||
                     !strcmp(argv[1],"--spider-stat-armor"))) {
        prayers();
        spider_stat_boundaries(!strcmp(argv[1],"--spider-stat-hitroll")?0:
                               !strcmp(argv[1],"--spider-stat-damroll")?1:2);
    }
    else if(argc==2&&!strcmp(argv[1],"--effect-expiry")) {
        prayers();timed_effects();
    }
    else if(argc==2&&!strcmp(argv[1],"--rejected-costs")) {
        prayers();rejected_costs();
    }
    else if(argc==2&&!strcmp(argv[1],"--house-commands")) {
        prayers();house_commands();
    }
    else if(argc==2&&!strcmp(argv[1],"--communication")) {
        prayers();communication();
    }
    else if(argc==2&&!strcmp(argv[1],"--dimensional-travel")) {
        prayers();dimensional_travel();
    }
    else if(argc==2&&!strcmp(argv[1],"--skipdisc-travel")) {
        prayers();skipdisc_travel();skipdisc_blocked_movement();
    }
    else if(argc==2&&!strcmp(argv[1],"--skipdisc-ownership")) {
        skipdisc_ownership_probe();skipdisc_source_guards();
    }
    else if(argc==2&&!strcmp(argv[1],"--darkness-lifecycle")) {
        darkness_lifecycle();darkness_loss_cleanup();
    }
    else if(argc==2&&!strcmp(argv[1],"--darkness-dismissal")) {
        darkness_loss_cleanup();
    }
    else if(argc==2&&!strcmp(argv[1],"--darkness-extreme")) {
        actor->pcdata->drow[DROW_POWERS]=DROW_DARK|DROW_GLOBE;
        actor->pcdata->classpoints[CP_CURRENT]=LONG_MIN;
        darkness_pulse();
        assert(actor->pcdata->classpoints[CP_CURRENT]==0);
        assert(!get_drow(actor,DROW_GLOBE));
    }
    else {
        prayers();rejected_costs();equipment_use();crossbow_death_cleanup();toggles();training_and_reports();prayer_menu();house_training();house_commands();communication();dimensional_travel();skipdisc_travel();skipdisc_blocked_movement();skipdisc_ownership_probe();skipdisc_source_guards();
        darkness_lifecycle();darkness_loss_cleanup();
        if(argc==2&&!strcmp(argv[1],"--spider"))spider();
        else {equipment();equipment_refusal_boundaries();dual_wield();combat_and_travel();spider();spider_stat_boundaries(-1);timed_effects();}
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
