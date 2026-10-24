/* Real native entry/revert and combat. Only clan.c's multi-hit boundary can
 * inject extraction, relocation, death or reversion to check caller ownership. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static CHAR_DATA *actor,*target,*heap_actor,*heap_target,*heap_next;
static ROOM_INDEX_DATA elsewhere;
static int action,trigger,attacks;
static bool triggered;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};

static void unlink_actor(CHAR_DATA **slot)
{
    CHAR_DATA *ch=*slot,**link=&char_list;
    assert(ch);stop_fighting(ch,TRUE);
    while(*link&&*link!=ch)link=&(*link)->next;
    assert(*link==ch);*link=ch->next;
    char_from_room(ch);
    if(ch->desc)ch->desc->character=NULL;
    free_string(ch->morph);free(ch);*slot=NULL;
}

void lifecycle_multi_hit(CHAR_DATA *ch,CHAR_DATA *victim)
{
    ++attacks;
    if(!action||triggered||attacks!=trigger){multi_hit(ch,victim);return;}
    triggered=TRUE;
    switch(action) {
    case 1:unlink_actor(&heap_actor);break;
    case 2:unlink_actor(&heap_target);break;
    case 3:unlink_actor(&heap_next);break;
    case 4:char_from_room(ch);char_to_room(ch,&elsewhere);break;
    case 5:char_from_room(victim);char_to_room(victim,&elsewhere);break;
    case 6:ch->hit=0;ch->position=POS_DEAD;stop_fighting(ch,TRUE);break;
    case 7:victim->hit=0;victim->position=POS_DEAD;stop_fighting(victim,TRUE);break;
    case 8:do_unwerewolf(ch,"");break;
    case 9:ch->position=POS_RESTING;break;
    case 10:ch->combat_pause=4;break;
    default:assert(FALSE);
    }
}

static void setup(int mode,int ahroun,int metis,int path,unsigned seed)
{
    if(heap_actor)unlink_actor(&heap_actor);
    if(heap_target)unlink_actor(&heap_target);
    if(heap_next)unlink_actor(&heap_next);
    if(mobile)remove_mobile();
    for(int i=0;i<3;i++) {
        while(chars[i].affected)affect_remove(&chars[i],chars[i].affected);
        free_string(chars[i].morph);chars[i].morph=NULL;
    }
    reset();actor=chars;target=&chars[1];action=0;trigger=1;attacks=0;triggered=FALSE;
    memset(&elsewhere,0,sizeof elsewhere);elsewhere.area=&area;elsewhere.light=1;
    actor->class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].auspice[AUSPICE_AHROUN]=ahroun;
    players[0].breed[BREED_METIS]=metis;
    players[0].wolf=100;
    if(mode) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(actor,hybrid_by_parents(
            actor->class,mode>=10?CLASS_WEREWOLF:parents[mode-1]),&state));
        assert(hybrid_player_attach(actor,&state));
    }
    for(int i=0;i<3;i++) {
        chars[i].hit=chars[i].max_hit=1000000;
        chars[i].mana=chars[i].max_mana=1000000;
        chars[i].fighting=NULL;chars[i].stance[0]=-1;
        chars[i].morph=str_dup("");
        descriptors[i].connected=CON_PLAYING;
    }
    if(path==1)room.room_flags=ROOM_SAFE;
    if(path==2){char_from_room(&chars[2]);char_to_room(&chars[2],&elsewhere);}
    if(path==3)players[2].flag=0;
    init_mm();while(seed--)(void)number_mm();
}

static uint64_t text_hash(const char *s,uint64_t h)
{
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        h=(h^*p)*UINT64_C(1099511628211);
    return h;
}

static uint64_t snapshot(int mode,int ahroun,int metis,int path,
                         unsigned seed,int phase,uint64_t hash,bool dump)
{
    char row[4096];size_t used=(size_t)snprintf(row,sizeof row,
        "%d,%d,%d,%d,%u,%d,%d",mode,ahroun,metis,path,seed,phase,attacks);
    uint64_t messages=UINT64_C(1469598103934665603);
    for(int i=0;i<3;i++) {
        used+=(size_t)snprintf(row+used,sizeof row-used,
            ",%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld",
            chars[i].hit,chars[i].wait,chars[i].position,chars[i].act,
            chars[i].polyaff,chars[i].affected_by,chars[i].vampaff,
            players[i].wolf,chars[i].style[0],chars[i].hitroll,
            chars[i].damroll,chars[i].armor);
        messages=text_hash(output[i],messages);
        messages=text_hash(chars[i].morph,messages);
    }
    used+=(size_t)snprintf(row+used,sizeof row-used,",%llu,%d\n",
        (unsigned long long)messages,number_mm());
    assert(used<sizeof row);if(dump)fputs(row,stdout);
    return text_hash(row,hash);
}

static void native_cases(bool dump)
{
    uint64_t hash=UINT64_C(1469598103934665603);unsigned count=0;
    const int metis[]={0,2,3,5};
    for(int mode=0;mode<19;mode++)for(int ahroun=1;ahroun<=5;ahroun++)
    for(int rank=0;rank<4;rank++)for(int path=0;path<4;path++)
    for(unsigned seed=0;seed<2;seed++) {
        setup(mode,ahroun,metis[rank],path,seed);
        do_werewolf(actor,"");
        hash=snapshot(mode,ahroun,metis[rank],path,seed,0,hash,dump);++count;
        do_unwerewolf(actor,"");
        hash=snapshot(mode,ahroun,metis[rank],path,seed,1,hash,dump);++count;
    }
    assert(count==6080);
    assert(hash==UINT64_C(179437905898800419));
    if(!dump)printf("native Werewolf lifecycle rows=%u hash=%llu\n",
                    count,(unsigned long long)hash);
}

static CHAR_DATA *heap_copy(CHAR_DATA *old)
{
    CHAR_DATA *copy=malloc(sizeof *copy);assert(copy);*copy=*old;
    old->morph=NULL;
    CHAR_DATA **link=&char_list;
    while(*link&&*link!=old)link=&(*link)->next;
    assert(*link==old);*link=copy;
    link=&room.people;
    while(*link&&*link!=old)link=&(*link)->next_in_room;
    assert(*link==old);*link=copy;
    copy->desc->character=copy;return copy;
}

static void callback_case(int mode,int kind,int when)
{
    setup(mode,3,3,0,0);
    if(kind==1)actor=heap_actor=heap_copy(actor);
    if(kind==2)target=heap_target=heap_copy(target);
    if(kind==3)heap_next=heap_copy(&chars[2]);
    action=kind;trigger=when;
    do_werewolf(actor,"");
    assert(triggered);
    if(kind==1||kind==4||kind==6||kind==8||kind==9||kind==10)
        assert(attacks==when);
    if(kind==2||kind==5||kind==7) {
        /* The first target's remaining strikes stop; a later live target can
         * still receive its ordinary three attack groups. */
        assert(attacks==when+3);
    }
    if(kind==3)assert(attacks==3);
    if(kind==8)assert(!IS_SET(actor->act,PLR_WOLFMAN)&&players[0].wolf==0);
}

int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native_cases(TRUE);return 0;}
    if(argc>3&&!strcmp(argv[1],"--callback")) {
        callback_case(0,atoi(argv[2]),atoi(argv[3]));return 0;
    }
    native_cases(FALSE);
    for(int mode=0;mode<10;mode++)for(int kind=1;kind<=10;kind++)
    for(int when=1;when<=3;when++)callback_case(mode,kind,when);
    puts("Werewolf lifecycle: native entry/revert and 300 callback paths passed");
    return 0;
}
