/* Real combat and permissions. The existing test-only damage seam models
 * removal/movement during callbacks; it is never compiled into production. */
#define main restored_fixture_main
#define combat_test_hurt_person restored_fixture_hurt_person
#include "test_restored_spells.c"
#undef combat_test_hurt_person
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static unsigned callback_count;
static int callback_action;
static ROOM_INDEX_DATA elsewhere;
static const long other_classes[] = {CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,
    CLASS_HIGHLANDER,CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};
static const char *commands[] = {"elemental fire target","elemental water target",
    "elemental earth target","elemental air target","invoke lightning target",
    "cutting target","surge"};

void combat_test_hurt_person(CHAR_DATA *ch, CHAR_DATA *victim, long dam)
{
    ++callback_count;
    if (callback_action == 1) {
        assert(victim == mobile);
        remove_mobile();
        return;
    }
    hurt_person(ch,victim,dam);
    if (callback_action == 2) {
        char_from_room(victim);char_to_room(victim,&elsewhere);
    } else if (callback_action == 3) {
        char_from_room(ch);char_to_room(ch,&elsewhere);
    } else if (callback_action == 4) {
        ch->hit=0;ch->position=POS_STUNNED;
    } else if (callback_action == 5 && mobile) {
        remove_mobile();
    }
}

static void wolf_setup(int mode, unsigned seed)
{
    if (mobile) remove_mobile();
    reset();
    memset(&elsewhere,0,sizeof elsewhere);elsewhere.area=&area;
    callback_count=0;callback_action=0;
    chars[0].class=mode>=10?other_classes[(mode-10)%9]:CLASS_WEREWOLF;
    for (int n=0;n<3;n++) {
        chars[n].hit=chars[n].max_hit=1000000;
        chars[n].fighting=NULL;
        chars[n].mana=1000000;
        chars[n].contingency="";
    }
    players[0].breed[BREED_LUPUS]=5;
    players[0].tribes[TRIBE_GLASS_WALKERS]=3;
    players[0].tribes[TRIBE_WENDIGOS]=5;
    players[0].rank=1;
    room.sector_type=SECT_FIELD;
    if (mode) {
        long secondary=mode>=10?CLASS_WEREWOLF:other_classes[mode-1];
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(chars,
            hybrid_by_parents(chars[0].class,secondary),&state));
        assert(hybrid_player_attach(chars,&state));
    }
    init_mm();while(seed--) (void)number_mm();
}

static void execute_gift(unsigned gift)
{
    char argument[MAX_INPUT_LENGTH];
    strcpy(argument,commands[gift]);
    interpret(chars,argument);
}

static uint64_t text_hash(const char *text, uint64_t h)
{
    for (const unsigned char *p=(const unsigned char *)text;*p;p++)
        h=(h^*p)*UINT64_C(1099511628211);
    return h;
}

static uint64_t native_cases(int dump)
{
    uint64_t total=UINT64_C(1469598103934665603);
    unsigned cases=0;
    for (int mode=0;mode<19;mode++)for (unsigned gift=0;gift<7;gift++)
    for (unsigned seed=0;seed<8;seed++) {
        wolf_setup(mode,seed);
        execute_gift(gift);
        char row[1024];size_t used=0;
        used+=(size_t)snprintf(row+used,sizeof row-used,"%d,%u,%u",mode,gift,seed);
        for(int n=0;n<3;n++) {
            int opponent=chars[n].fighting==chars?0:
                chars[n].fighting==chars+1?1:chars[n].fighting==chars+2?2:-1;
            used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%ld,%ld,%d",
                chars[n].hit,chars[n].mana,chars[n].wait,chars[n].position,
                players[n].weather,opponent);
        }
        uint64_t messages=UINT64_C(1469598103934665603);
        for(int n=0;n<3;n++)messages=text_hash(output[n],messages);
        used+=(size_t)snprintf(row+used,sizeof row-used,",%llu,%d\n",
            (unsigned long long)messages,number_mm());
        assert(used<sizeof row);
        if(dump)fputs(row,stdout);
        total=text_hash(row,total);++cases;
    }
    assert(cases==1064);
    return total;
}

static void target_removed(unsigned gift)
{
    unsigned removed=0;
    for(unsigned seed=0;seed<(gift==5?64u:1u);seed++) {
        wolf_setup(0,seed);add_mobile();
        callback_action=1;
        if(gift==2)do_elemental(chars,"earth mobile");
        else if(gift==4)do_invoke(chars,"lightning mobile");
        else do_cutting(chars,"mobile");
        if(!callback_count) {
            assert(gift==5&&mobile);remove_mobile();continue;
        }
        assert(mobile==NULL&&callback_count==1);++removed;
        assert(chars[0].wait==(gift==4?0:12));
        assert(players[0].weather==(gift==4?10:0));
    }
    assert(removed>0);
}

static void callback_boundaries(int surge_only)
{
    for(unsigned gift=2;!surge_only&&gift<=5;gift++) {
        if(gift==3)continue;
        for(int action=2;action<=4;action++) {
            wolf_setup(0,0);callback_action=action;
            execute_gift(gift);
            assert(callback_count==1);
            if(action==2)assert(chars[1].in_room==&elsewhere);
            if(action==3)assert(chars[0].in_room==&elsewhere);
            if(gift==5)assert(chars[1].position!=POS_STUNNED);
        }
    }
    for(int action=3;action<=5;action++) {
        wolf_setup(0,0);callback_action=action;
        if(action==5) {
            add_mobile();
            /* The callback removes the cached next room-list entry. */
            room.people=chars;chars[0].next_in_room=chars+1;
            chars[1].next_in_room=mobile;mobile->next_in_room=chars+2;
        }
        do_surge(chars,"");
        assert(callback_count==1);
        assert(chars[2].hit==chars[2].max_hit);
    }
}

static void target_selection(int invoke_only)
{
    for(int mode=invoke_only;mode<2;mode++) {
        wolf_setup(0,0);chars[0].fighting=chars+2;
        if(mode)do_invoke(chars,"lightning target");
        else do_elemental(chars,"earth target");
        assert(chars[1].hit<chars[1].max_hit);
        assert(chars[2].hit==chars[2].max_hit);
        wolf_setup(0,0);chars[0].fighting=chars+2;
        if(mode)do_invoke(chars,"lightning missing");
        else do_elemental(chars,"earth missing");
        assert(callback_count==0&&chars[2].hit==chars[2].max_hit);
        assert(chars[0].wait==0&&players[0].weather==0);
        wolf_setup(0,0);chars[0].fighting=chars+1;
        if(mode)do_invoke(chars,"lightning");
        else do_elemental(chars,"earth");
        assert(chars[1].hit<chars[1].max_hit);
        wolf_setup(0,0);
        char_from_room(chars+1);char_to_room(chars+1,&elsewhere);
        chars[0].fighting=chars+1;
        if(mode)do_invoke(chars,"lightning");
        else do_elemental(chars,"earth");
        assert(callback_count==0&&chars[1].hit==chars[1].max_hit);
    }
}

static void entry_guards(void)
{
    void (*functions[])(CHAR_DATA *,char *)={
        do_elemental,do_invoke,do_cutting,do_surge};
    char *arguments[]={"earth target","lightning target","target",""};
    for(unsigned f=0;f<4;f++)for(int mode=0;mode<8;mode++) {
        wolf_setup(0,0);
        if(mode==0){functions[f](NULL,arguments[f]);continue;}
        if(mode==1){
            if(f<3){functions[f](chars,NULL);assert(callback_count==0);}
            continue; /* Surge has no parsed argument and accepts NULL. */
        }
        if(mode==2)chars[0].pcdata=NULL;
        if(mode==3)chars[0].in_room=NULL;
        if(mode==4)chars[0].hit=0;
        if(mode==5)chars[0].position=POS_STUNNED;
        if(mode==6)SET_BIT(chars[0].act,ACT_IS_NPC);
        if(mode==7)room.people=chars+1;
        functions[f](chars,arguments[f]);
        assert(callback_count==0&&chars[1].hit==chars[1].max_hit);
    }
}

int main(int argc, char **argv)
{
    static char strings[2];string_space=strings;top_string=strings+sizeof strings;
    if(argc>1&&!strcmp(argv[1],"--native-dump")) {
        (void)native_cases(1);return 0;
    }
    if(argc>1&&!strcmp(argv[1],"--native-hash")) {
        printf("%llu\n",(unsigned long long)native_cases(0));return 0;
    }
    if(argc>1&&!strcmp(argv[1],"--removed-earth")) {target_removed(2);return 0;}
    if(argc>1&&!strcmp(argv[1],"--removed-lightning")) {target_removed(4);return 0;}
    if(argc>1&&!strcmp(argv[1],"--removed-cutting")) {target_removed(5);return 0;}
    if(argc>1&&!strcmp(argv[1],"--callback-boundaries")) {callback_boundaries(0);return 0;}
    if(argc>1&&!strcmp(argv[1],"--surge-boundaries")) {callback_boundaries(1);return 0;}
    if(argc>1&&!strcmp(argv[1],"--target-selection")) {target_selection(0);return 0;}
    if(argc>1&&!strcmp(argv[1],"--invoke-selection")) {target_selection(1);return 0;}
    uint64_t native=native_cases(0);
    /* Filled from the unchanged 715db0f engine before applying the fixes. */
    assert(native==UINT64_C(15599035253278121217));
    target_removed(2);target_removed(4);target_removed(5);
    callback_boundaries(0);target_selection(0);entry_guards();
    puts("Werewolf combat: 1064 native cases, extraction/movement/stun/list boundaries and exact targets passed");
    return 0;
}
