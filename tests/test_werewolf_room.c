/* Real engine. Only flee/interpret calls made by the Tribe module have
 * controlled callback injections; the ordinary path calls the real engine. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static CHAR_DATA *actor, *heap_actor, *successor;
static ROOM_INDEX_DATA moved_room;
static unsigned callback_calls;
static int callback_action;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};

static void unlink_heap(CHAR_DATA **slot)
{
    CHAR_DATA *ch=*slot,**link=&char_list;
    assert(ch);stop_fighting(ch,TRUE);
    while(*link&&*link!=ch)link=&(*link)->next;
    assert(*link==ch);*link=ch->next;char_from_room(ch);
    if(ch->desc)ch->desc->character=NULL;
    free(ch);*slot=NULL;
}

static void callback(CHAR_DATA *target)
{
    ++callback_calls;
    if(callback_action==1)unlink_heap(&heap_actor);
    else if(callback_action==2) {
        char_from_room(actor);char_to_room(actor,&moved_room);
    } else if(callback_action==3&&target==mobile)remove_mobile();
    else if(callback_action==4&&successor)unlink_heap(&successor);
    else if(callback_action==5) {
        char_from_room(target);char_to_room(target,&moved_room);
    } else if(callback_action==6) {
        actor->hit=0;actor->position=POS_STUNNED;
    }
}

void werewolf_room_flee(CHAR_DATA *ch,char *argument)
{
    if(!callback_action){do_flee(ch,argument);return;}
    callback(ch);
}

void werewolf_room_interpret(CHAR_DATA *ch,char *argument)
{
    if(!callback_action){interpret(ch,argument);return;}
    callback(ch);
}

static void setup(int mode,unsigned seed,bool npc)
{
    if(successor)unlink_heap(&successor);
    if(mobile)remove_mobile();
    if(heap_actor)unlink_heap(&heap_actor);
    reset();actor=chars;callback_calls=0;callback_action=0;
    memset(&moved_room,0,sizeof moved_room);moved_room.area=&area;
    chars[0].class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].tribes[TRIBE_BLACK_FURIES]=5;
    players[0].tribes[TRIBE_FIANNA]=5;
    players[0].tribes[TRIBE_SHADOW_LORDS]=5;
    players[0].rank=3;
    chars[0].act|=PLR_AUTOFOLLOW;
    for(int n=0;n<3;n++) {
        chars[n].hit=500;chars[n].max_hit=100000;
        chars[n].mana=100000;chars[n].position=POS_FIGHTING;
    }
    chars[0].position=POS_STANDING;
    if(mode) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(chars,hybrid_by_parents(
            chars[0].class,mode>=10?CLASS_WEREWOLF:parents[mode-1]),&state));
        assert(hybrid_player_attach(chars,&state));
    }
    if(npc) {
        add_mobile();mobile->hit=500;mobile->max_hit=100000;
        mobile->level=1;mobile->fighting=chars;mobile->position=POS_FIGHTING;
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
    for(int mode=0;mode<19;mode++)for(int gift=0;gift<3;gift++)
    for(int npc=0;npc<2;npc++)for(unsigned seed=0;seed<8;seed++) {
        setup(mode,seed,npc);
        char command[MAX_INPUT_LENGTH],row[1024];size_t used=0;
        strcpy(command,gift==0?"breathwyld":gift==1?"howlbanshee":"obedience all smile");
        interpret(actor,command);
        used+=(size_t)snprintf(row+used,sizeof row-used,"%d,%d,%d,%u",mode,gift,npc,seed);
        for(int n=0;n<3;n++)
            used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%ld,%ld,%ld",
                chars[n].hit,chars[n].mana,chars[n].wait,chars[n].position,
                chars[n].act,players[n].resist[WILL_PRESENCE]);
        uint64_t messages=UINT64_C(1469598103934665603);
        for(int n=0;n<3;n++)messages=hash_text(output[n],messages);
        used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%llu,%d\n",
            mobile?mobile->hit:0,mobile?mobile->wait:0,mobile?mobile->act:0,
            (unsigned long long)messages,number_mm());
        assert(used<sizeof row);if(dump)fputs(row,stdout);
        hash=hash_text(row,hash);++count;
    }
    assert(count==912);return hash;
}

static void make_heap_actor(void)
{
    heap_actor=malloc(sizeof *heap_actor);assert(heap_actor);
    *heap_actor=chars[0];actor=heap_actor;
    CHAR_DATA **p=&char_list;while(*p!=chars)p=&(*p)->next;*p=actor;
    p=&room.people;while(*p!=chars)p=&(*p)->next_in_room;*p=actor;
    descriptors[0].character=actor;chars[0].in_room=NULL;
}

static void callback_case(bool obedience,int action,bool single)
{
    setup(0,0,TRUE);
    if(action==1)make_heap_actor();
    if(action==4) {
        successor=calloc(1,sizeof *successor);assert(successor);
        clear_char(successor);successor->act=ACT_IS_NPC;
        successor->name=successor->short_descr="successor";
        successor->hit=successor->max_hit=10000;
        successor->position=POS_FIGHTING;successor->fighting=actor;
        successor->next=char_list;char_list=successor;
        successor->in_room=&room;successor->next_in_room=mobile->next_in_room;
        mobile->next_in_room=successor;
    }
    callback_action=action;
    if(obedience)do_obedience(actor,single?"mobile smile":"all smile");
    else do_banshee(actor,"");
    assert(callback_calls);
    if(action==1)assert(!heap_actor);
    if(action==2) {
        assert(actor->in_room==&moved_room);
        if(!obedience)assert(IS_SET(actor->act,PLR_AUTOFOLLOW));
    }
    if(action==3)assert(!mobile);
    if(action==4)assert(!successor);
    if(action==5&&obedience)assert(mobile&&!IS_SET(mobile->act,ACT_COMMANDED));
    if(action==1||action==2||action==6)assert(callback_calls==1);
    if(action==4) {
        assert(callback_calls==(obedience?1:2));
        assert(chars[1].wait==0&&chars[2].wait==0);
    }
    if(obedience&&mobile)assert(!IS_SET(mobile->act,ACT_COMMANDED));
}

static void healing_boundary(void)
{
    setup(0,0,FALSE);
    chars[1].hit=LONG_MAX-1;chars[1].max_hit=LONG_MAX;
    do_breathwyld(actor,"");
    assert(chars[1].hit==LONG_MAX);
    setup(0,0,FALSE);players[0].rank=LONG_MAX;
    do_breathwyld(actor,"");
    assert(chars[1].hit==500&&actor->wait==0);
    setup(0,0,FALSE);players[0].rank=-1;
    do_breathwyld(actor,"");
    assert(chars[1].hit==500&&actor->wait==0);
}

static void long_command(bool description)
{
    setup(0,0,TRUE);
    char *text=malloc(MAX_STRING_LENGTH*2);assert(text);
    memset(text,'x',MAX_STRING_LENGTH*2-1);text[MAX_STRING_LENGTH*2-1]='\0';
    if(description) {
        mobile->short_descr=text;
        do_obedience(actor,"mobile smile");
        mobile->short_descr="mobile";
    } else {
        memcpy(text,"all ",4);
        do_obedience(actor,text);
        assert(actor->wait==0);
    }
    free(text);
}

static void entry_guards(void)
{
    for(int gift=0;gift<3;gift++)for(int mode=0;mode<7;mode++) {
        setup(0,0,FALSE);CHAR_DATA *ch=actor;char *arg=gift==2?"all smile":"";
        if(mode==0)ch=NULL;
        if(mode==1)arg=NULL;
        if(mode==2)ch->pcdata=NULL;
        if(mode==3)ch->in_room=NULL;
        if(mode==4)ch->hit=0;
        if(mode==5)ch->position=POS_STUNNED;
        if(mode==6){char_from_room(ch);ch->in_room=&room;}
        if(gift==0)do_breathwyld(ch,arg);
        if(gift==1)do_banshee(ch,arg);
        if(gift==2)do_obedience(ch,arg);
        assert(chars[1].hit==500&&chars[1].wait==0);
    }
}

int main(int argc,char **argv)
{
    if(argc>1&&strcmp(argv[1],"--native-dump")==0){native_cases(TRUE);return 0;}
    if(argc>1&&strcmp(argv[1],"--native-hash")==0) {
        printf("%llu\n",(unsigned long long)native_cases(FALSE));return 0;
    }
    if(argc>1&&strcmp(argv[1],"--heal")==0){healing_boundary();return 0;}
    if(argc>1&&strcmp(argv[1],"--entry")==0){entry_guards();return 0;}
    if(argc>1&&strcmp(argv[1],"--long")==0){long_command(argc>2);return 0;}
    if(argc>2) {
        callback_case(!strcmp(argv[1],"--obey"),atoi(argv[2]),argc>3);
        return 0;
    }
    assert(native_cases(FALSE)==UINT64_C(16022392963781972845));
    healing_boundary();entry_guards();
    long_command(FALSE);long_command(TRUE);
    for(int action=1;action<=6;action++) {
        callback_case(FALSE,action,FALSE);
        callback_case(TRUE,action,FALSE);
        if(action!=4)callback_case(TRUE,action,TRUE);
    }
    if(mobile)remove_mobile();
    if(successor)unlink_heap(&successor);
    if(heap_actor)unlink_heap(&heap_actor);
    puts("Werewolf room handlers: native, healing and callback guards passed");
    return 0;
}
