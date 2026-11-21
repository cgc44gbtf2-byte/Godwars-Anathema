/* Real interpreter, travel, world lookup and output. Only the Auspice module's
 * lookup is counted; a separate forced-miss case exercises bounded fallback. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

extern bool fBootDb;
static ROOM_INDEX_DATA fallback_rooms[160], destination;
static unsigned long lookups;
static bool miss_random;
static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};

ROOM_INDEX_DATA *werewolf_galliard_room(long vnum)
{
    ++lookups;
    if(miss_random&&lookups<=4096)return NULL;
    return get_room_index(vnum);
}

static void setup(int mode,unsigned seed)
{
    if(mobile)remove_mobile();
    reset();lookups=0;miss_random=FALSE;
    memset(room_index_hash,0,sizeof room_index_hash);
    memset(fallback_rooms,0,sizeof fallback_rooms);
    memset(&destination,0,sizeof destination);
    fBootDb=FALSE;
    room.vnum=99;room.name="the starting room";room.description="The beginning.\n\r";
    area.nplayer=3;
    destination.vnum=20000;destination.area=&area;destination.light=1;
    destination.name="the destination";destination.description="Moonlight.\n\r";
    for(int i=0;i<160;i++) {
        ROOM_INDEX_DATA *r=&fallback_rooms[i];
        r->vnum=100*(i+1);r->area=&area;r->light=1;
        r->name="a fallback room";r->description="A quiet clearing.\n\r";
        r->next=room_index_hash[r->vnum%MAX_KEY_HASH];
        room_index_hash[r->vnum%MAX_KEY_HASH]=r;
    }
    chars[0].class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].auspice[AUSPICE_GALLIARD]=5;
    players[0].auspice[AUSPICE_RAGABASH]=5;
    if(mode) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(chars,hybrid_by_parents(
            chars[0].class,mode>=10?CLASS_WEREWOLF:parents[mode-1]),&state));
        assert(hybrid_player_attach(chars,&state));
    }
    chars[1].class=CLASS_WEREWOLF;
    chars[2].level=LEV_BUI;
    for(int i=0;i<3;i++) {
        chars[i].fighting=NULL;
        descriptors[i].next=i<2?&descriptors[i+1]:NULL;
    }
    descriptor_list=descriptors;
    SET_BIT(chars[1].extra,EXTRA_SUMMON);
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
    for(int mode=0;mode<10;mode++)for(int gift=0;gift<2;gift++)
    for(int path=0;path<5;path++)for(unsigned seed=0;seed<8;seed++) {
        setup(mode,seed);
        char command[MAX_INPUT_LENGTH],row[1024];size_t used=0;
        if(!gift) {
            strcpy(command,"wyld");
            if(path==0||path==4)chars[0].fighting=&chars[1];
            if(path==1){chars[0].hit=0;chars[0].position=POS_MORTAL;}
            if(path==3){add_mobile();chars[0].fighting=mobile;}
            if(path==4)descriptors[1].connected=CON_GET_NAME;
        } else {
            char_from_room(&chars[1]);char_to_room(&chars[1],&destination);
            strcpy(command,path==1?"bridgewalk missing":path==4?"bridgewalk":"bridgewalk target");
            if(path==2){char_from_room(&chars[1]);char_to_room(&chars[1],&room);}
            if(path==3)destination.room_flags=ROOM_PRIVATE;
            SET_BIT(chars[0].itemaffect,ITEMA_BLISSFUL);
        }
        interpret(chars,command);
        used+=(size_t)snprintf(row+used,sizeof row-used,"%d,%d,%d,%u",mode,gift,path,seed);
        for(int n=0;n<3;n++)
            used+=(size_t)snprintf(row+used,sizeof row-used,",%ld,%ld,%ld,%ld,%ld,%ld",
                chars[n].hit,chars[n].mana,chars[n].wait,chars[n].position,
                chars[n].itemaffect,chars[n].in_room->vnum);
        uint64_t messages=UINT64_C(1469598103934665603);
        for(int n=0;n<3;n++)messages=hash_text(output[n],messages);
        used+=(size_t)snprintf(row+used,sizeof row-used,",%lu,%llu,%d\n",
            lookups,(unsigned long long)messages,number_mm());
        assert(used<sizeof row);if(dump)fputs(row,stdout);
        hash=hash_text(row,hash);++count;
    }
    if(mobile)remove_mobile();
    assert(count==800);return hash;
}

static void bounded_search(bool empty)
{
    setup(0,0);SET_BIT(chars[0].itemaffect,ITEMA_BLISSFUL);
    if(empty)memset(room_index_hash,0,sizeof room_index_hash);
    else miss_random=TRUE;
    do_bridgewalk(chars,"missing");
    assert(lookups<=4096+15901);
    if(empty) {
        assert(chars[0].in_room==&room&&chars[0].wait==0);
        assert(IS_ITEMAFF(chars,ITEMA_BLISSFUL));
        assert(strstr(output[0],"No moon bridge"));
    } else {
        assert(chars[0].in_room==fallback_rooms);
        assert(lookups==4097&&chars[0].wait==8);
    }
}

static void restricted_worlds(void)
{
    for(int kind=0;kind<4;kind++) {
        setup(0,0);
        for(int i=0;i<160;i++)fallback_rooms[i].room_flags=
            kind==0?ROOM_PRIVATE:kind==1?ROOM_SOLITARY:ROOM_NO_TELEPORT;
        if(kind==3){fallback_rooms[159].room_flags=0;miss_random=TRUE;}
        do_bridgewalk(chars,"missing");
        assert(lookups<=4096+15901);
        assert(chars[0].in_room==(kind==3?&fallback_rooms[159]:&room));
    }
    setup(0,0);room.name=NULL;chars[0].hit=0;
    do_wyld(chars,"");assert(strstr(output[1],"somewhere"));
    setup(0,0);add_mobile();mobile->short_descr=NULL;chars[0].fighting=mobile;
    do_wyld(chars,"");assert(strstr(output[1],"someone"));remove_mobile();
}

static void long_report(bool enemy)
{
    setup(0,0);add_mobile();chars[0].fighting=mobile;
    char *text=malloc(MAX_STRING_LENGTH*2);assert(text);
    memset(text,'x',MAX_STRING_LENGTH*2-1);text[MAX_STRING_LENGTH*2-1]='\0';
    if(enemy)mobile->short_descr=text;
    else room.name=text;
    do_wyld(chars,"");
    assert(descriptors[1].outtop>0&&descriptors[1].outtop<MAX_STRING_LENGTH+512);
    room.name="the starting room";mobile->short_descr="mobile";
    free(text);remove_mobile();
}

static void stale_opponent(void)
{
    setup(0,0);
    CHAR_DATA *old=malloc(sizeof *old);assert(old);memset(old,0,sizeof *old);
    chars[0].fighting=old;free(old);
    do_wyld(chars,"");
    assert(!strstr(output[1],"under attack"));
    chars[0].fighting=NULL;
}

static void entry_guards(void)
{
    for(int gift=0;gift<2;gift++)for(int mode=0;mode<5;mode++) {
        setup(0,0);CHAR_DATA *ch=chars;char *argument=gift?"missing":"";
        if(mode==0)ch=NULL;
        if(mode==1)argument=NULL;
        if(mode==2)ch->pcdata=NULL;
        if(mode==3)ch->in_room=NULL;
        if(mode==4){char_from_room(ch);ch->in_room=&room;}
        if(gift)do_bridgewalk(ch,argument);else do_wyld(ch,argument);
        assert(lookups==0&&descriptors[1].outtop==0);
    }
    setup(0,0);chars[0].hit=0;chars[0].position=POS_MORTAL;
    do_wyld(chars,"");assert(strstr(output[1],"badly wounded"));
    setup(0,0);chars[0].position=POS_STUNNED;
    do_bridgewalk(chars,"missing");assert(lookups==0);
}

static void permissions_and_rank(void)
{
    for(int mode=10;mode<19;mode++)for(int gift=0;gift<2;gift++) {
        setup(mode,0);chars[0].fighting=&chars[1];
        int expected=number_mm();init_mm();
        if(gift)do_bridgewalk(chars,"target");else do_wyld(chars,"");
        assert(chars[0].in_room==&room&&lookups==0&&chars[0].wait==0);
        assert(descriptors[1].outtop==0&&number_mm()==expected);
    }
    setup(0,0);players[0].auspice[AUSPICE_GALLIARD]=4;
    do_bridgewalk(chars,"target");
    assert(strstr(output[0],"Galliard 5")&&!strstr(output[0],"Ragabash 5"));
}

static void oversized_name(void)
{
    setup(0,0);char *text=malloc(MAX_STRING_LENGTH*2);assert(text);
    memset(text,'x',MAX_STRING_LENGTH*2-1);text[MAX_STRING_LENGTH*2-1]='\0';
    do_bridgewalk(chars,text);assert(lookups==0&&chars[0].in_room==&room);
    free(text);
}

int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native_cases(TRUE);return 0;}
    if(argc>1&&!strcmp(argv[1],"--native-hash")) {
        printf("%llu\n",(unsigned long long)native_cases(FALSE));return 0;
    }
    if(argc>1&&!strcmp(argv[1],"--empty")){bounded_search(TRUE);return 0;}
    if(argc>1&&!strcmp(argv[1],"--long")){long_report(argc>2);return 0;}
    if(argc>1&&!strcmp(argv[1],"--stale")){stale_opponent();return 0;}
    if(argc>1&&!strcmp(argv[1],"--entry")){entry_guards();return 0;}
    if(argc>1&&!strcmp(argv[1],"--name")){oversized_name();return 0;}
    if(argc>1&&!strcmp(argv[1],"--rank")){permissions_and_rank();return 0;}
    assert(native_cases(FALSE)==UINT64_C(819415021862684503));
    bounded_search(TRUE);bounded_search(FALSE);restricted_worlds();
    long_report(FALSE);long_report(TRUE);stale_opponent();
    entry_guards();permissions_and_rank();oversized_name();
    puts("Galliard utilities: native behavior, bounded travel and reports passed");
    return 0;
}
