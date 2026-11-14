/* Real commands and state; only calls made by the Auspice module can inject
 * controlled movement/extraction at a transformation callback boundary. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static CHAR_DATA *actor,*target,*heap_actor,*heap_target,*heap_next;
static ROOM_INDEX_DATA elsewhere;
static int inject_kind=-1,inject_action;
static unsigned calls[6];
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

static bool callback(int kind)
{
    ++calls[kind];
    if(inject_kind!=kind)return FALSE;
    if(inject_action==1)unlink_heap(&heap_actor);
    else if(inject_action==2)unlink_heap(&heap_target);
    else if(inject_action==3){char_from_room(actor);char_to_room(actor,&elsewhere);}
    else if(inject_action==4){char_from_room(target);char_to_room(target,&elsewhere);}
    else if(inject_action==5)unlink_heap(&heap_next);
    else if(inject_action==6){actor->hit=0;actor->position=POS_STUNNED;}
    return TRUE;
}

#define WRAP(name,kind) \
void forms_##name(CHAR_DATA *ch,char *arg) \
{if(!callback(kind))do_##name(ch,arg);}
WRAP(majesty,0)
WRAP(shadowbody,1)
WRAP(change,2)
WRAP(werewolf,3)
WRAP(beastlike,4)
WRAP(berserk,5)
#undef WRAP

static void setup(int mode,int kind,unsigned seed)
{
    if(heap_actor)unlink_heap(&heap_actor);
    if(heap_target)unlink_heap(&heap_target);
    if(heap_next)unlink_heap(&heap_next);
    if(mobile)remove_mobile();
    for(int i=0;i<3;i++)if(chars[i].morph) {
        free_string(chars[i].morph);chars[i].morph=NULL;
    }
    reset();actor=chars;target=&chars[1];inject_kind=-1;
    memset(calls,0,sizeof calls);memset(&elsewhere,0,sizeof elsewhere);
    elsewhere.area=&area;elsewhere.light=1;
    actor->class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].auspice[AUSPICE_GALLIARD]=5;
    if(mode) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actor,hybrid_by_parents(
            actor->class,mode>=10?CLASS_WEREWOLF:parents[mode-1]),&state));
        assert(hybrid_player_attach(actor,&state));
    }
    target->class=CLASS_VAMPIRE;target->beast=100;
    target->vampgen=9;target->blood[0]=target->blood[1]=100;
    players[1].powers[DISC_PRESENCE]=5;
    players[1].powers[DISC_PROTEAN]=4;
    players[1].powers[DISC_OBTENEBRATION]=3;
    players[1].wolf=25;
    for(int i=0;i<3;i++) {
        chars[i].hit=chars[i].max_hit=1000000;
        chars[i].mana=chars[i].max_mana=1000000;
        chars[i].fighting=NULL;chars[i].stance[0]=-1;
        chars[i].morph=str_dup("");
        /* Login descriptors make form autosaves ineligible, not mocked. */
        descriptors[i].connected=CON_GET_NAME;
    }
    if(kind==0)SET_BIT(target->itemaffect,ITEMA_PEACE);
    if(kind==1){SET_BIT(target->polyaff,POLY_SHADOW);
        SET_BIT(target->affected_by,AFF_POLYMORPH|AFF_ETHEREAL);}
    if(kind==2){SET_BIT(target->polyaff,POLY_MIST);
        SET_BIT(target->affected_by,AFF_POLYMORPH|AFF_ETHEREAL);}
    if(kind==3){target->class=CLASS_WEREWOLF;players[1].auspice[AUSPICE_AHROUN]=2;}
    if(kind==4)target->beast=50;
    if(kind==5)target->class=CLASS_MONK;
    if(kind==6) {
        target->class=CLASS_DROW;
        SET_BIT(players[1].drow[DROW_POWERS],DROW_GLOBE);
    }
    if(kind==7){target->class=CLASS_VAMPIRE;target->beast=0;}
    init_mm();while(seed--)(void)number_mm();
}

static uint64_t hash_text(const char *s,uint64_t hash)
{
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        hash=(hash^*p)*UINT64_C(1099511628211);
    return hash;
}

static void native_cases(bool dump)
{
    uint64_t hash=UINT64_C(1469598103934665603);unsigned count=0;
    for(int mode=0;mode<19;mode++)for(int kind=0;kind<8;kind++)
    for(int path=0;path<4;path++)for(unsigned seed=0;seed<4;seed++) {
        setup(mode,kind,seed);
        if(path==1)room.room_flags=ROOM_SAFE;
        if(path==2){players[1].powers[DISC_PRESENCE]=0;
            players[1].powers[DISC_OBTENEBRATION]=0;players[1].powers[DISC_PROTEAN]=0;}
        if(path==3)target->fight_timer=5;
        char command[64];strcpy(command,kind<3||kind==6?"wyrmcall":"songofrage target");
        interpret(actor,command);
        char row[1024];size_t used=(size_t)snprintf(row,sizeof row,"%d,%d,%d,%u",
            mode,kind,path,seed);
        for(int i=0;i<3;i++)
            used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld",
                chars[i].hit,chars[i].wait,chars[i].position,chars[i].polyaff,
                chars[i].affected_by,chars[i].vampaff,chars[i].itemaffect,
                players[i].wolf,players[i].majesty,chars[i].beast,players[i].drow[DROW_POWERS]);
        uint64_t messages=UINT64_C(1469598103934665603);
        for(int i=0;i<3;i++) {
            messages=hash_text(output[i],messages);
            messages=hash_text(chars[i].morph,messages);
        }
        used+=(size_t)snprintf(row+used,sizeof row-used,",%llu,%d\n",
            (unsigned long long)messages,number_mm());
        assert(used<sizeof row);hash=hash_text(row,hash);++count;
        if(dump)fputs(row,stdout);
    }
    assert(count==2432);
    assert(hash==UINT64_C(1761630710869323801));
    if(!dump)printf("native forms cases=%u hash=%llu\n",count,(unsigned long long)hash);
}

static void overflow_case(void)
{
    setup(0,4,0);players[1].wolf=LONG_MAX;
    target->beast=100;
    do_songrage(actor,"target");
    assert(players[1].wolf==LONG_MAX);
}

static CHAR_DATA *heap_copy(CHAR_DATA *ch)
{
    CHAR_DATA *copy=malloc(sizeof *copy);assert(copy);*copy=*ch;
    CHAR_DATA **link=&char_list;
    while(*link&&*link!=ch)link=&(*link)->next;
    assert(*link==ch);*link=copy;
    link=&room.people;
    while(*link&&*link!=ch)link=&(*link)->next_in_room;
    assert(*link==ch);*link=copy;
    /* Previous list nodes can refer to the replaced node. */
    for(CHAR_DATA *p=char_list;p;p=p->next) {
        if(p!=copy&&p->next==ch)p->next=copy;
        if(p!=copy&&p->next_in_room==ch)p->next_in_room=copy;
    }
    copy->desc->character=copy;
    return copy;
}

static void callback_case(int kind,int action)
{
    setup(0,kind,0);
    if(action==1)actor=heap_actor=heap_copy(actor);
    if(action==2)target=heap_target=heap_copy(target);
    if(action==5) {
        /* Place cached next after target, so the first callback removes it. */
        heap_next=heap_copy(&chars[2]);
    }
    inject_kind=kind;inject_action=action;
    if(kind<3)do_wyrmcall(actor,"");else do_songrage(actor,"target");
    assert(calls[kind]==1);
    if(action==2&&kind==4)assert(players[1].wolf==25);
    if(kind==4&&(action==3||action==4||action==6))assert(players[1].wolf==25);
    if(action==1)assert(heap_actor==NULL);
    if(action==2)assert(heap_target==NULL);
    if(action==3||action==6)assert(actor->wait==0);
}

static void entry_guards(void)
{
    do_wyrmcall(NULL,"");do_songrage(NULL,"target");
    setup(0,0,0);char_from_room(actor);
    do_wyrmcall(actor,"");do_songrage(actor,"target");
    assert(actor->wait==0);
    setup(0,0,0);actor->hit=0;actor->position=POS_STUNNED;
    do_wyrmcall(actor,"");do_songrage(actor,"target");assert(actor->wait==0);
    setup(0,4,0);target->hit=0;target->position=POS_DEAD;
    do_songrage(actor,"target");assert(players[1].wolf==25&&actor->wait==0);
    setup(0,0,0);do_songrage(actor,NULL);assert(actor->wait==0);
}

int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native_cases(TRUE);return 0;}
    if(argc>1&&!strcmp(argv[1],"--overflow")){overflow_case();return 0;}
    if(argc>3&&!strcmp(argv[1],"--callback")) {
        callback_case(atoi(argv[2]),atoi(argv[3]));return 0;
    }
    native_cases(FALSE);overflow_case();
    for(int kind=0;kind<6;kind++)for(int action=1;action<=6;action++)
        callback_case(kind,action);
    entry_guards();
    puts("Werewolf transformation callers: ordinary, rage bounds and callback contracts passed");
    return 0;
}
