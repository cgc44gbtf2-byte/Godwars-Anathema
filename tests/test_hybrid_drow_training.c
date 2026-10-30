/* Secondary Drow utility purchases through the real selfbrid interpreter. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/passives.h"
#include "classes/hybrid/player.h"

extern char *string_space,*top_string;
extern void init_mm(void);
extern void mobile_update(void);

static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static char output[16384];
static unsigned purchases;
static const long originals[] = {
    CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,CLASS_MAGE,
    CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF
};

static CHAR_DATA *fresh(long primary,long secondary,long dp)
{
    memset(&shell,0,sizeof(shell));
    memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"AbsentDrowfixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);
    ch->name=str_dup("Drowfixture");
    free_string(ch->short_descr);
    ch->short_descr=str_dup("Drowfixture");
    ch->desc=&shell;
    shell.connected=CON_PLAYING;
    shell.outbuf=output;
    shell.outsize=sizeof(output);
    ch->level=LEV_AWA;
    ch->position=POS_STANDING;
    ch->class=primary;
    ch->hit=ch->max_hit=50000;
    ch->mana=ch->max_mana=50000;
    ch->primal=9876;
    ch->exp=7654321;
    ch->pcdata->quest=50000;
    ch->pcdata->classpoints[CP_CURRENT]=321;
    ch->in_room=&room;
    ch->next=char_list;
    char_list=ch;
    ch->next_in_room=room.people;
    room.people=ch;

    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,hybrid_by_parents(primary,secondary),&state));
    if(secondary==CLASS_DROW)
        state.secondary_classpoints[CP_CURRENT]=dp;
    assert(hybrid_player_attach(ch,&state));
    return ch;
}

static void release(CHAR_DATA *ch)
{
    assert(char_list==ch&&room.people==ch);
    char_list=ch->next;
    room.people=ch->next_in_room;
    ch->next=ch->next_in_room=NULL;
    ch->desc=NULL;
    ch->in_room=NULL;
    free_char(ch);
    shell.character=NULL;
    unlink("../player/Drowfixture");
    unlink("../backup/Drowfixture");
}

static void command(CHAR_DATA *ch,const char *argument)
{
    char line[128];
    assert(strlen(argument)<sizeof(line)-16);
    snprintf(line,sizeof(line),"selfbrid %s",argument);
    shell.outtop=0;
    output[0]='\0';
    interpret(ch,line);
}

static void unchanged(CHAR_DATA *ch,const char *argument)
{
    PC_DATA before=*ch->pcdata;
    long primal=ch->primal,exp=ch->exp,quest=ch->pcdata->quest;
    command(ch,argument);
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(ch->primal==primal&&ch->exp==exp&&ch->pcdata->quest==quest);
}

static void loaded_heighten(CHAR_DATA *ch)
{
    PC_DATA before=*ch->pcdata;
    long mana=ch->mana;
    assert(!IS_SET(ch->act,PLR_HOLYLIGHT));
    do_truesight(ch,"");
    assert(IS_SET(ch->act,PLR_HOLYLIGHT)&&ch->wait==6&&ch->mana==mana);
    ch->wait=0;do_truesight(ch,"");
    assert(!IS_SET(ch->act,PLR_HOLYLIGHT)&&ch->wait==0&&ch->mana==mana);
    assert(!memcmp(&before,ch->pcdata,sizeof before));
}

static void loaded_spidereyes(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *before_room=ch->in_room;
    DESCRIPTOR_DATA *before_desc=ch->desc;
    CHAR_DATA *before_character=shell.character;
    PC_DATA before=*ch->pcdata;
    long mana=ch->mana;
    /* The list view needs no remote fixture and still exercises permission,
     * zero charge, context preservation and the real interpreter. */
    ch->in_room=&room;
    ch->desc=&shell;shell.character=ch;
    shell.outtop=0;output[0]='\0';
    interpret(ch,"scry");
    assert(strstr(output,"mentally survey"));
    assert(ch->mana==mana&&ch->in_room==&room);
    assert(!memcmp(&before,ch->pcdata,sizeof before));
    ch->in_room=before_room;
    ch->desc=before_desc;shell.character=before_character;
}

static void supported_utilities(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);++i) {
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,52500);
        long native_dp=ch->pcdata->classpoints[CP_CURRENT];
        long primal=ch->primal,exp=ch->exp,quest=ch->pcdata->quest;
        assert(!hybrid_power_drow(ch,DROW_ATTUNE));
        assert(!hybrid_power_drow(ch,DROW_FIRE));
        command(ch,"drow");
        assert(strstr(output,"attunement: available"));
        assert(strstr(output,"1000 DP"));
        assert(strstr(output,"faeriefire: available"));
        assert(strstr(output,"acidbolt: available"));
        assert(strstr(output,"llothsweb: available"));
        assert(strstr(output,"readaura: available"));
        assert(strstr(output,"heighten: available"));
        assert(strstr(output,"spidereyes: available"));
        assert(strstr(output,"Speed: rank 0/4"));
        assert(strstr(output,"next rank costs 10000 DP"));
        assert(strstr(output,"5000 DP"));
        assert(strstr(output,"Other secondary Drow powers"));

        command(ch,"drow attunement");
        assert(strstr(output,"1000 DP to learn"));
        unchanged(ch,"drow summonspider learn");
        unchanged(ch,"drow faeriefire learn trailing");
        unchanged(ch,"drow faeriefire unknown");
        assert(!hybrid_power_drow(ch,DROW_ACID));

        command(ch,"drow attunement learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_ATTUNE));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==51500);
        command(ch,"drow attunement learn");
        assert(strstr(output,"already learned"));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==51500);

        OBJ_INDEX_DATA index={0};
        OBJ_DATA item={0};
        item.pIndexData=&index;
        item.name="token";
        item.short_descr="a test token";
        item.questowner=ch->name;
        item.item_type=ITEM_ARMOR;
        item.wear_loc=WEAR_NONE;
        item.carried_by=ch;
        ch->carrying=&item;
        interpret(ch,"attunement token");
        assert(IS_SET(item.extra_flags,ITEM_LOYAL));
        ch->carrying=NULL;

        command(ch,"drow faeriefire learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_FIRE));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==46500);
        command(ch,"drow acidbolt learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_ACID));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==41500);
        unchanged(ch,"drow acidbolt learn");
        assert(strstr(output,"already learned"));
        command(ch,"drow llothsweb learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_WEB));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==33500);
        unchanged(ch,"drow llothsweb learn");
        assert(strstr(output,"already learned"));
        command(ch,"drow readaura learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_READ));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==27500);
        unchanged(ch,"drow readaura learn");
        assert(strstr(output,"already learned"));
        command(ch,"drow heighten learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_SIGHT));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==7500);
        unchanged(ch,"drow heighten learn");
        assert(strstr(output,"already learned"));
        command(ch,"drow spidereyes learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_EYES));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        unchanged(ch,"drow spidereyes learn");
        assert(strstr(output,"already learned"));
        assert(ch->pcdata->classpoints[CP_CURRENT]==native_dp);
        assert(ch->primal==primal&&ch->exp==exp&&ch->pcdata->quest==quest);
        assert(ch->class==originals[i]);
        assert(ch->pcdata->drow[DROW_POWERS]==0);

        DESCRIPTOR_DATA reload={0};
        assert(load_char_obj(&reload,"Drowfixture"));
        assert(reload.character&&hybrid_power_drow(reload.character,DROW_ATTUNE));
        assert(hybrid_power_drow(reload.character,DROW_FIRE));
        assert(hybrid_power_drow(reload.character,DROW_ACID));
        assert(hybrid_power_drow(reload.character,DROW_WEB));
        assert(hybrid_power_drow(reload.character,DROW_READ));
        assert(hybrid_power_drow(reload.character,DROW_SIGHT));
        assert(hybrid_power_drow(reload.character,DROW_EYES));
        assert(reload.character->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        assert(reload.character->pcdata->classpoints[CP_CURRENT]==native_dp);
        assert(reload.character->class==originals[i]);
        loaded_heighten(reload.character);
        loaded_spidereyes(reload.character);
        free_char(reload.character);
        reload.character=NULL;
        assert(load_char_short(&reload,"Drowfixture"));
        assert(reload.character&&hybrid_power_drow(reload.character,DROW_WEB));
        assert(hybrid_power_drow(reload.character,DROW_READ));
        assert(hybrid_power_drow(reload.character,DROW_SIGHT));
        assert(hybrid_power_drow(reload.character,DROW_EYES));
        assert(reload.character->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        assert(reload.character->class==originals[i]);
        loaded_heighten(reload.character);
        loaded_spidereyes(reload.character);
        free_char(reload.character);
        reload.character=NULL;
        purchases+=7;
        release(ch);
    }
    assert(purchases==63);
}

static void darkvision_lessons(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);++i) {
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,5000);
        PC_DATA before=*ch->pcdata;
        long mana=ch->mana,primal=ch->primal,exp=ch->exp;
        interpret(ch,"darkvision");
        assert(!IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
        assert(!memcmp(&before,ch->pcdata,sizeof before));
        command(ch,"drow darkvision");
        assert(strstr(output,"darkvision: available"));
        command(ch,"drow darkvision learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_NIGHT));
        assert(hybrid_live_secondary(ch)->secondary_classpoints[CP_CURRENT]==0);
        unchanged(ch,"drow darkvision learn");
        before=*ch->pcdata;
        CHAR_DATA target={0};
        target.name="Nighttarget";
        target.in_room=&room;
        target.act=ACT_IS_NPC;
        OBJ_DATA object={0};
        object.item_type=ITEM_WEAPON;
        object.in_room=&room;
        SET_BIT(room.room_flags,ROOM_DARK);
        assert(!can_see(ch,&target)&&!can_see_obj(ch,&object));
        interpret(ch,"darkvision");
        assert(IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
        assert(can_see(ch,&target)&&can_see_obj(ch,&object));
        SET_BIT(ch->affected_by,AFF_BLIND);
        assert(!can_see(ch,&target)&&!can_see_obj(ch,&object));
        REMOVE_BIT(ch->affected_by,AFF_BLIND);
        SET_BIT(room.added_flags,ROOM2_DARKNESS);
        assert(!can_see(ch,&target));
        REMOVE_BIT(room.added_flags,ROOM2_DARKNESS);
        REMOVE_BIT(room.room_flags,ROOM_DARK);
        assert(!memcmp(&before,ch->pcdata,sizeof before));
        assert(ch->mana==mana&&ch->primal==primal&&ch->exp==exp);
        assert(ch->pcdata->classpoints[CP_CURRENT]==321);
        assert(save_char_obj_checked(ch));
        for(int short_load=0;short_load<2;++short_load) {
            DESCRIPTOR_DATA reload={0};
            assert(short_load?load_char_short(&reload,"Drowfixture"):
                              load_char_obj(&reload,"Drowfixture"));
            assert(hybrid_power_drow(reload.character,DROW_NIGHT));
            assert(IS_VAMPAFF(reload.character,VAM_NIGHTSIGHT));
            do_nightsight(reload.character,"");
            assert(!IS_VAMPAFF(reload.character,VAM_NIGHTSIGHT));
            free_char(reload.character);
        }
        /* Aliases operate one shared toggle, not two stacked effects. */
        interpret(ch,"nightsight");
        assert(!IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
        /* Sorcerer-owned sight is independent and survives the Drow toggle. */
        if(ch->class==CLASS_SORCERER) {
            sorcerer_vision_toggle(ch);
            assert(effective_nightsight(ch));
            interpret(ch,"darkvision");
            assert(IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
            interpret(ch,"darkvision");
            assert(!IS_VAMPAFF(ch,VAM_NIGHTSIGHT)&&effective_nightsight(ch));
            sorcerer_vision_toggle(ch);
            assert(!effective_nightsight(ch));
        }
        interpret(ch,"darkvision");
        assert(IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
        if(ch->class==CLASS_VAMPIRE) {
            ch->pcdata->wolf=1;
            interpret(ch,"darkvision");
            assert(IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
            ch->pcdata->wolf=0;
        }
        /* A pre-existing native effect remains toggleable, not duplicated. */
        interpret(ch,"darkvision");
        assert(!IS_VAMPAFF(ch,VAM_NIGHTSIGHT));
        assert(!memcmp(&before,ch->pcdata,sizeof before));
        release(ch);
        ch=fresh(originals[i],CLASS_DROW,4999);
        unchanged(ch,"drow darkvision learn");
        assert(!hybrid_power_drow(ch,DROW_NIGHT));
        release(ch);
    }
}

static void darkness_lessons(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);++i){
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,10500);
        command(ch,"drow");
        assert(strstr(output,"darkness: available"));
        assert(strstr(output,"500 DP activation, 150 DP upkeep"));
        command(ch,"drow darkness learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_DARK));
        assert(hybrid_drow_points_current(ch)==1000);
        unchanged(ch,"drow darkness learn");
        shell.outtop=0;output[0]='\0';
        interpret(ch,"globedarkness");
        assert(hybrid_drow_globe_active(ch));
        assert(hybrid_drow_points_current(ch)==500);
        CHAR_DATA watcher={0};
        PC_DATA watcher_pc={0};
        DESCRIPTOR_DATA watcher_desc={0};
        char watcher_output[4096]={0};
        watcher.pcdata=&watcher_pc;
        watcher.name="Watcher";
        watcher.position=POS_STANDING;
        watcher.level=LEV_AWA;
        watcher.in_room=&room;
        watcher.desc=&watcher_desc;
        watcher_desc.character=&watcher;
        watcher_desc.connected=CON_PLAYING;
        watcher_desc.outbuf=watcher_output;
        watcher_desc.outsize=sizeof watcher_output;
        assert(!can_see(&watcher,ch));
        do_look(&watcher,"auto");
        assert(strstr(watcher_output,"pitch black darkness pervades"));
        assert(ch->pcdata->drow[DROW_POWERS]==0);
        assert(ch->pcdata->classpoints[CP_CURRENT]==321);
        assert(save_char_obj_checked(ch));
        for(int short_file=0;short_file<2;short_file++){
            DESCRIPTOR_DATA reload={0};
            assert(short_file?load_char_short(&reload,"Drowfixture"):
                              load_char_obj(&reload,"Drowfixture"));
            assert(hybrid_power_drow(reload.character,DROW_DARK));
            assert(hybrid_drow_globe_active(reload.character));
            assert(hybrid_drow_points_current(reload.character)==500);
            free_char(reload.character);
        }
        interpret(ch,"globedarkness");
        assert(!hybrid_drow_globe_active(ch));
        assert(hybrid_drow_points_current(ch)==500);
        assert(hybrid_drow_points_set_current(ch,499));
        interpret(ch,"globedarkness");
        assert(!hybrid_drow_globe_active(ch));
        assert(hybrid_drow_points_current(ch)==499);
        assert(hybrid_drow_points_set_current(ch,151));
        assert(hybrid_drow_globe_set(ch,1));
        assert(hybrid_drow_globe_upkeep(ch,150)==1);
        assert(hybrid_drow_points_current(ch)==1);
        assert(hybrid_drow_globe_active(ch));
        assert(hybrid_drow_globe_upkeep(ch,150)==0);
        assert(hybrid_drow_points_current(ch)==0);
        assert(!hybrid_drow_globe_active(ch));
        assert(hybrid_drow_points_set_current(ch,151));
        assert(hybrid_drow_globe_set(ch,1));
        mobile_update();
        assert(hybrid_drow_points_current(ch)==1);
        assert(hybrid_drow_globe_active(ch));
        mobile_update();
        assert(hybrid_drow_points_current(ch)==0);
        assert(!hybrid_drow_globe_active(ch));
        assert(hybrid_drow_points_set_current(ch,150));
        assert(hybrid_drow_globe_set(ch,1));
        assert(hybrid_drow_globe_upkeep(ch,150)==0);
        assert(hybrid_drow_points_current(ch)==0);
        assert(!hybrid_drow_globe_active(ch));
        assert(hybrid_drow_globe_set(ch,1));
        ch->hit=0;
        update_pos(ch);
        assert(!hybrid_drow_globe_active(ch));
        ch->hit=50000;
        assert(ch->pcdata->classpoints[CP_CURRENT]==321);
        assert(ch->pcdata->drow[DROW_POWERS]==0);
        release(ch);
    }
}

static void poison_lessons(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);++i){
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,5000);
        unchanged(ch,"drow poisonimmunity");
        assert(strstr(output,"5000 DP to learn"));
        command(ch,"drow poisonimmunity learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_PIMM));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        unchanged(ch,"drow poisonimmunity learn");
        for(int short_file=0;short_file<2;short_file++){
            DESCRIPTOR_DATA reload={0};
            assert(short_file?load_char_short(&reload,"Drowfixture"):
                              load_char_obj(&reload,"Drowfixture"));
            assert(hybrid_power_drow(reload.character,DROW_PIMM));
            assert(reload.character->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
            /* A real self-poison attempt cannot consume mana or add an affect. */
            PC_DATA before=*reload.character->pcdata;
            long mana=reload.character->mana;
            spell_poison(gsn_poison,1000,reload.character,reload.character);
            assert(!IS_AFFECTED(reload.character,AFF_POISON));
            assert(reload.character->mana==mana);
            assert(!memcmp(&before,reload.character->pcdata,sizeof before));
            free_char(reload.character);
        }
        release(ch);
    }
}

static void blindfight_lessons(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);i++){
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,4999);
        unchanged(ch,"drow blindfight learn");
        assert(strstr(output,"Not enough secondary Drow points"));
        ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]=5000;
        unchanged(ch,"drow blindfight");
        assert(strstr(output,"5000 DP to learn"));
        command(ch,"drow blindfight learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_BFIGHT));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        unchanged(ch,"drow blindfight learn");
        for(int short_file=0;short_file<2;short_file++){
            DESCRIPTOR_DATA reload={0};
            assert(short_file?load_char_short(&reload,"Drowfixture"):
                              load_char_obj(&reload,"Drowfixture"));
            assert(hybrid_power_drow(reload.character,DROW_BFIGHT));
            assert(reload.character->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
            /* Use the loaded record in the real defensive calculation. */
            PC_DATA before=*reload.character->pcdata;
            CHAR_DATA *loaded=reload.character;
            loaded->in_room=&room;loaded->position=POS_FIGHTING;
            ch->position=POS_FIGHTING;
            init_mm();(void)check_avoid(ch,loaded,TRUE);
            assert(!memcmp(&before,loaded->pcdata,sizeof before));
            loaded->in_room=NULL;
            free_char(loaded);
        }
        release(ch);
    }
}

static void toughness_lessons(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);i++){
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,7999);
        unchanged(ch,"drow toughness learn");
        assert(strstr(output,"Not enough secondary Drow points"));
        ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]=8000;
        unchanged(ch,"drow toughness");
        assert(strstr(output,"8000 DP to learn"));
        long before=get_soak(ch);
        command(ch,"drow toughness learn");
        assert(strstr(output,"learned and saved"));
        assert(hybrid_power_drow(ch,DROW_TOUGH));
        assert(get_soak(ch)==before+300);
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
        unchanged(ch,"drow toughness learn");
        for(int short_file=0;short_file<2;short_file++){
            DESCRIPTOR_DATA reload={0};
            assert(short_file?load_char_short(&reload,"Drowfixture"):
                              load_char_obj(&reload,"Drowfixture"));
            CHAR_DATA *loaded=reload.character;
            assert(hybrid_power_drow(loaded,DROW_TOUGH));
            assert(loaded->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
            PC_DATA snapshot=*loaded->pcdata;
            long trained=get_soak(loaded);
            assert(!memcmp(&snapshot,loaded->pcdata,sizeof snapshot));
            REMOVE_BIT(loaded->pcdata->hybrid.secondary_progression.drow[DROW_POWERS],DROW_TOUGH);
            assert(get_soak(loaded)==trained-300);
            free_char(loaded);
        }
        release(ch);
    }
}

static void speed_lessons(void)
{
    for(size_t i=0;i<sizeof(originals)/sizeof(originals[0]);i++){
        CHAR_DATA *ch=fresh(originals[i],CLASS_DROW,9999);
        long native_dp=ch->pcdata->classpoints[CP_CURRENT];
        long speed=get_speed(ch);
        long damroll=char_damroll(ch);
        long damcap=get_damcap(ch);

        unchanged(ch,"drow speed");
        assert(strstr(output,"Speed: rank 0/4"));
        assert(strstr(output,"10000 DP"));
        unchanged(ch,"drow speed learn");
        assert(strstr(output,"Not enough secondary Drow points"));
        assert(ch->pcdata->hybrid.secondary_progression.drow[DROW_SPEED]==0);

        ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]=40000;
        for(long rank=1;rank<=4;rank++){
            char expected[64];
            snprintf(expected,sizeof(expected),"rank %ld/4",rank-1);
            unchanged(ch,"drow speed");
            assert(strstr(output,expected));
            assert(strstr(output,"next rank costs 10000 DP"));
            command(ch,"drow speed learn");
            assert(strstr(output,"learned and saved"));
            assert(ch->pcdata->hybrid.secondary_progression.drow[DROW_SPEED]==rank);
            assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==
                   40000-rank*10000);
            assert(ch->pcdata->classpoints[CP_CURRENT]==native_dp);
            assert(ch->class==originals[i]);
            assert(get_speed(ch)==(speed-rank<6?6:speed-rank));
            assert(char_damroll(ch)==damroll+rank*50);
            assert(get_damcap(ch)==damcap+rank*100);
            assert(hybrid_secondary_passive(ch,HYBRID_PASSIVE_AVOID,100)==
                   100+(rank*5)/2);
            assert(hybrid_secondary_passive(ch,HYBRID_PASSIVE_BREAK,100)==
                   100+rank*5);
        }
        unchanged(ch,"drow speed");
        assert(strstr(output,"rank 4/4"));
        assert(strstr(output,"maximum reached"));
        unchanged(ch,"drow speed learn");
        assert(strstr(output,"already learned"));
        assert(ch->pcdata->hybrid.secondary_progression.drow[DROW_SPEED]==4);
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);

        for(int short_file=0;short_file<2;short_file++){
            DESCRIPTOR_DATA reload={0};
            assert(short_file?load_char_short(&reload,"Drowfixture"):
                              load_char_obj(&reload,"Drowfixture"));
            CHAR_DATA *loaded=reload.character;
            assert(loaded->pcdata->hybrid.secondary_progression.drow[DROW_SPEED]==4);
            assert(loaded->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
            assert(loaded->pcdata->classpoints[CP_CURRENT]==native_dp);
            assert(loaded->class==originals[i]);
            assert(get_speed(loaded)==(speed-4<6?6:speed-4));
            assert(char_damroll(loaded)==damroll+200);
            assert(get_damcap(loaded)==damcap+400);
            free_char(loaded);
        }
        release(ch);
    }
}

static void refusals_and_native_isolation(void)
{
    CHAR_DATA *ch=fresh(CLASS_MAGE,CLASS_DROW,4999);
    PC_DATA before=*ch->pcdata;
    command(ch,"drow faeriefire learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_FIRE));
    command(ch,"drow acidbolt learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_ACID));
    command(ch,"drow llothsweb learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_WEB));
    command(ch,"drow readaura learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_READ));
    command(ch,"drow heighten learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_SIGHT));
    command(ch,"drow spidereyes learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    assert(!hybrid_power_drow(ch,DROW_EYES));
    command(ch,"drow speed learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    assert(!memcmp(&before,ch->pcdata,sizeof(before)));
    release(ch);

    ch=fresh(CLASS_MAGE,CLASS_MONK,6000);
    unchanged(ch,"drow attunement learn");
    assert(strstr(output,"valid secondary Drow record"));
    release(ch);

    ch=fresh(CLASS_DROW,CLASS_MONK,6000);
    ch->pcdata->drow[DROW_POWERS]=DROW_ATTUNE|DROW_FIRE;
    PC_DATA native=*ch->pcdata;
    command(ch,"drow");
    assert(strstr(output,"original class"));
    assert(!memcmp(&native,ch->pcdata,sizeof(native)));
    assert(hybrid_power_drow(ch,DROW_ATTUNE)&&hybrid_power_drow(ch,DROW_FIRE));
    release(ch);
}

static void context_and_save_faults(void)
{
    CHAR_DATA *ch=fresh(CLASS_MAGE,CLASS_DROW,6000);
    ch->fighting=ch;
    unchanged(ch,"drow attunement learn");
    ch->fighting=NULL;
    assert(!hybrid_power_drow(ch,DROW_ATTUNE));
    shell.original=ch;
    unchanged(ch,"drow attunement learn");
    shell.original=NULL;
    ch->save_pending|=PLAYER_SAVE_PRIMARY;
    unchanged(ch,"drow attunement learn");
    assert(strstr(output,"recovery is pending"));
    ch->save_pending=0;
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
                        SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);++i) {
        atomic_save_test_fault=faults[i];
        unchanged(ch,"drow attunement learn");
        assert(strstr(output,"No lesson or points"));
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(!hybrid_power_drow(ch,DROW_ATTUNE));
        unchanged(ch,"drow attunement learn");
        assert(strstr(output,"recovery is pending"));
        assert(save_char_obj_checked(ch));
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    command(ch,"drow attunement learn");
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat"));
    assert(hybrid_power_drow(ch,DROW_ATTUNE));
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==5000);
    unchanged(ch,"drow faeriefire learn");
    assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));
    command(ch,"drow faeriefire learn");
    assert(strstr(output,"learned and saved"));
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
    release(ch);
}

static void utility_save_faults(const char *lesson,long flag,long price)
{
    CHAR_DATA *ch=fresh(CLASS_NINJA,CLASS_DROW,price);
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
                        SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);i++) {
        atomic_save_test_fault=faults[i];
        unchanged(ch,lesson);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(!hybrid_power_drow(ch,flag));
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==price);
        assert(save_char_obj_checked(ch));
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    command(ch,lesson);
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat"));
    assert(hybrid_power_drow(ch,flag));
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
    unchanged(ch,lesson);
    assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));
    unchanged(ch,lesson);
    assert(strstr(output,"already learned"));
    release(ch);
}

static void speed_save_faults(void)
{
    CHAR_DATA *ch=fresh(CLASS_NINJA,CLASS_DROW,10000);
    const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
                        SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);i++) {
        atomic_save_test_fault=faults[i];
        unchanged(ch,"drow speed learn");
        atomic_save_test_fault=SAVE_FAULT_NONE;
        assert(ch->pcdata->hybrid.secondary_progression.drow[DROW_SPEED]==0);
        assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==10000);
        assert(save_char_obj_checked(ch));
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    command(ch,"drow speed learn");
    atomic_save_test_fault=SAVE_FAULT_NONE;
    assert(strstr(output,"Do not repeat"));
    assert(ch->pcdata->hybrid.secondary_progression.drow[DROW_SPEED]==1);
    assert(ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]==0);
    unchanged(ch,"drow speed learn");
    assert(strstr(output,"recovery is pending"));
    assert(save_char_obj_checked(ch));
    unchanged(ch,"drow speed learn");
    assert(strstr(output,"Not enough secondary Drow points"));
    release(ch);
}

int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-hybrid-drow-training-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&
           !mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);
    assert(string_space);
    top_string=string_space;
    current_time=123456;
    init_mm();
    fpReserve=fopen(NULL_FILE,"r");
    assert(fpReserve);
    area.name="Hybrid Drow test";
    room.vnum=901;
    room.area=&area;
    room.name="Private fixture";
    room.description="";
    room_index_hash[901%MAX_KEY_HASH]=&room;
    supported_utilities();
    darkvision_lessons();
    darkness_lessons();
    poison_lessons();
    blindfight_lessons();
    toughness_lessons();
    speed_lessons();
    refusals_and_native_isolation();
    context_and_save_faults();
    utility_save_faults("drow llothsweb learn",DROW_WEB,8000);
    utility_save_faults("drow readaura learn",DROW_READ,6000);
    utility_save_faults("drow heighten learn",DROW_SIGHT,20000);
    utility_save_faults("drow spidereyes learn",DROW_EYES,7500);
    utility_save_faults("drow darkness learn",DROW_DARK,9500);
    utility_save_faults("drow darkvision learn",DROW_NIGHT,5000);
    utility_save_faults("drow poisonimmunity learn",DROW_PIMM,5000);
    utility_save_faults("drow blindfight learn",DROW_BFIGHT,5000);
    utility_save_faults("drow toughness learn",DROW_TOUGH,8000);
    speed_save_faults();
    assert(!char_list&&!room.people&&!fclose(fpReserve));
    assert(!chdir("..")&&!rmdir("player")&&!rmdir("backup")&&
           !rmdir("run")&&!chdir(cwd)&&!rmdir(root));
    printf("Secondary Drow UI: %u existing paid powers plus nine Darkness and nine Darkvision lessons, "
           "atomic reloads, DP isolation, adapter use and refusal paths passed.\n",
           purchases);
    return 0;
}
