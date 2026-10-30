/* Real engine and permissions. Only the Tribe module's kill/retaliation
 * callbacks have explicit test wrappers, modeling synchronous extraction,
 * movement or interruption. These wrappers are never linked into production. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static CHAR_DATA *actor, *heap_actor, *successor;
static ROOM_INDEX_DATA moved_room;
static MOB_INDEX_DATA mob_index;
static unsigned reaction_calls;
static int reaction_action;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};

static void unlink_heap(CHAR_DATA **slot)
{
    CHAR_DATA *ch=*slot,**link=&char_list;
    assert(ch);
    stop_fighting(ch,TRUE);
    while(*link&&*link!=ch)link=&(*link)->next;
    assert(*link==ch);*link=ch->next;
    char_from_room(ch);
    if(ch->desc)ch->desc->character=NULL;
    free(ch);*slot=NULL;
}

static void reaction(void)
{
    ++reaction_calls;
    if(reaction_action==1)unlink_heap(&heap_actor);
    else if(reaction_action==2) {
        char_from_room(actor);char_to_room(actor,&moved_room);
    } else if(reaction_action==3) {
        actor->hit=0;actor->position=POS_STUNNED;
    } else if(reaction_action==4&&successor)unlink_heap(&successor);
}

void werewolf_test_do_kill(CHAR_DATA *ch,char *argument)
{
    if(!reaction_action) {do_kill(ch,argument);return;}
    reaction();
}

void werewolf_test_killperson(CHAR_DATA *ch,CHAR_DATA *victim)
{
    if(!reaction_action) {killperson(ch,victim);return;}
    reaction();
    if(victim==mobile)remove_mobile();
}

static void setup(int mode,unsigned seed,bool npc)
{
    if(successor)unlink_heap(&successor);
    if(mobile)remove_mobile();
    if(heap_actor)unlink_heap(&heap_actor);
    reset();actor=chars;reaction_action=0;reaction_calls=0;
    memset(&moved_room,0,sizeof moved_room);moved_room.area=&area;
    memset(&mob_index,0,sizeof mob_index);mob_index.vnum=1;
    chars[0].class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].tribes[TRIBE_BLACK_FURIES]=5;
    players[0].tribes[TRIBE_SHADOW_LORDS]=5;
    for(int n=0;n<3;n++) {
        chars[n].hit=chars[n].max_hit=1000000;
        chars[n].fighting=NULL;chars[n].mana=1000000;
    }
    if(mode) {
        HYBRID_STATE state;
        long second=mode>=10?CLASS_WEREWOLF:parents[mode-1];
        assert(hybrid_secondary_initialize(chars,
            hybrid_by_parents(chars[0].class,second),&state));
        assert(hybrid_player_attach(chars,&state));
    }
    if(npc) {
        add_mobile();mobile->hit=mobile->max_hit=1000000;
        mobile->pIndexData=&mob_index;mobile->act|=ACT_NOEXP;
    }
    init_mm();while(seed--)(void)number_mm();
}

static uint64_t hash_text(const char *s,uint64_t h)
{
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        h=(h^*p)*UINT64_C(1099511628211);
    return h;
}

static uint64_t native_cases(bool dump)
{
    uint64_t hash=UINT64_C(1469598103934665603);unsigned count=0;
    for(int mode=0;mode<19;mode++)for(int gift=0;gift<2;gift++)
    for(int npc=0;npc<2;npc++)for(unsigned seed=0;seed<16;seed++) {
        setup(mode,seed,npc);
        char command[MAX_INPUT_LENGTH],row[1024];size_t used=0;
        strcpy(command,gift?"clapthunder":npc?"bodywrack mobile":"bodywrack target");
        interpret(actor,command);
        used+=(size_t)snprintf(row+used,sizeof row-used,"%d,%d,%d,%u",mode,gift,npc,seed);
        for(int n=0;n<3;n++) {
            int foe=chars[n].fighting==chars?0:chars[n].fighting==chars+1?1:
                chars[n].fighting==chars+2?2:chars[n].fighting==mobile&&mobile?3:-1;
            used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%ld,%d",
                chars[n].hit,chars[n].mana,chars[n].wait,chars[n].position,foe);
        }
        uint64_t messages=UINT64_C(1469598103934665603);
        for(int n=0;n<3;n++)messages=hash_text(output[n],messages);
        used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%llu,%d\n",
            mobile?mobile->hit:0,mobile?mobile->wait:0,mobile?mobile->position:0,
            (unsigned long long)messages,number_mm());
        assert(used<sizeof row);
        if(dump)fputs(row,stdout);hash=hash_text(row,hash);++count;
    }
    assert(count==1216);return hash;
}

static void make_heap_actor(void)
{
    heap_actor=malloc(sizeof *heap_actor);assert(heap_actor);
    *heap_actor=chars[0];actor=heap_actor;
    CHAR_DATA **p=&char_list;
    while(*p!=chars)p=&(*p)->next;
    *p=actor;
    p=&room.people;while(*p!=chars)p=&(*p)->next_in_room;
    *p=actor;descriptors[0].character=actor;chars[0].in_room=NULL;
}

static void callback_case(bool clap,bool lethal,int action)
{
    unsigned triggered=0;
    for(unsigned seed=0;seed<16;seed++) {
        setup(0,seed,TRUE);
        if(lethal)mobile->hit=1;
        if(action==1)make_heap_actor();
        if(action==4) {
            successor=calloc(1,sizeof *successor);assert(successor);
            clear_char(successor);successor->act=ACT_IS_NPC|ACT_NOEXP;
            successor->name=successor->short_descr="successor";
            successor->hit=successor->max_hit=1000000;
            successor->position=POS_STANDING;successor->pIndexData=&mob_index;
            successor->next=char_list;char_list=successor;
            /* The first NPC's callback removes its cached next room member. */
            successor->in_room=&room;successor->next_in_room=mobile->next_in_room;
            mobile->next_in_room=successor;
        }
        reaction_action=action;
        if(clap)do_clap(actor,"");else do_bodywrack(actor,"mobile");
        if(!reaction_calls)continue; /* Clap's native stun can prevent retaliation. */
        ++triggered;
        if(action==1)assert(!heap_actor);
        if(action==2)assert(actor->in_room==&moved_room&&actor->wait==0);
        if(action==3)assert(actor->position==POS_STUNNED);
        if(action==4)assert(!successor);
        if(clap)assert(chars[1].hit==1000000&&chars[2].hit==1000000);
        assert(reaction_calls==1);
    }
    assert(triggered);
}

static void dead_targets(void)
{
    for(int gift=0;gift<2;gift++) {
        setup(0,0,FALSE);chars[1].hit=LONG_MIN;
        if(gift)do_clap(actor,"");else do_bodywrack(actor,"target");
        assert(chars[1].hit==LONG_MIN);
    }
}

static void stun_feedback(void)
{
    unsigned stunned=0;
    for(unsigned seed=0;seed<32;seed++) {
        setup(0,seed,FALSE);do_clap(actor,"");
        for(int n=1;n<3;n++)if(chars[n].position==POS_STUNNED) {
            assert(strstr(output[n],"You fall to the ground, stunned!"));
            ++stunned;
        }
        assert(!strstr(output[0],"You fall to the ground, stunned!"));
    }
    assert(stunned);
}

static void entry_guards(void)
{
    for(int gift=0;gift<2;gift++)for(int mode=0;mode<7;mode++) {
        setup(0,0,FALSE);CHAR_DATA *ch=actor;char *arg=gift?"":"target";
        if(mode==0)ch=NULL;
        if(mode==1)arg=NULL;
        if(mode==2)ch->pcdata=NULL;
        if(mode==3)ch->in_room=NULL;
        if(mode==4)ch->hit=0;
        if(mode==5)ch->position=POS_STUNNED;
        if(mode==6){char_from_room(ch);ch->in_room=&room;}
        if(gift)do_clap(ch,arg);else do_bodywrack(ch,arg);
        assert(chars[1].hit==1000000&&chars[2].hit==1000000);
    }
}

int main(int argc,char **argv)
{
    string_space=calloc(1,1024*1024);assert(string_space);top_string=string_space;
    current_time=123456;
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native_cases(TRUE);return 0;}
    if(argc>1&&!strcmp(argv[1],"--native-hash")){
        printf("%llu\n",(unsigned long long)native_cases(FALSE));return 0;}
    if(argc>1&&!strcmp(argv[1],"--dead-targets")){dead_targets();return 0;}
    if(argc>1&&!strcmp(argv[1],"--body-retaliation")){callback_case(FALSE,FALSE,1);return 0;}
    if(argc>1&&!strcmp(argv[1],"--clap-retaliation")){callback_case(TRUE,FALSE,1);return 0;}
    if(argc>1&&!strcmp(argv[1],"--cached-next")){callback_case(TRUE,TRUE,4);return 0;}
    if(argc>1&&!strcmp(argv[1],"--stun-feedback")){stun_feedback();return 0;}
    /* 8658ffd hash was 7350283483851042777. Only the Clap feedback recipient
     * changes 210 message hashes; all 1216 HP/timer/position/RNG rows agree.
     * The corrected full-output baseline is pinned below. */
    assert(native_cases(FALSE)==UINT64_C(2466425606829114923));
    for(int gift=0;gift<2;gift++)for(int lethal=0;lethal<2;lethal++)
    for(int action=1;action<=3;action++)callback_case(gift,lethal,action);
    callback_case(TRUE,TRUE,4);dead_targets();entry_guards();stun_feedback();
    puts("Werewolf reactions: 1216 native snapshots, callback interruption/removal and dead-target guards passed");
    return 0;
}
