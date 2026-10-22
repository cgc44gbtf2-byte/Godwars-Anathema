/* Real class reset, ownership teardown and atomic player files.
 * Only clearclass.c's free_string calls are counted; real frees still run. */
#define main ownership_fixture_main
#include "test_lifecycle_ownership.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static char *tracked[9];
static unsigned freed[9];
static bool counting;
static const long classes[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};

void class_reset_free_string(char *text)
{
    if(counting)for(unsigned i=0;i<9;i++)if(text==tracked[i])++freed[i];
    free_string(text);
}
static uint64_t hash_text(const char *s,uint64_t h)
{
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        h=(h^*p)*UINT64_C(1099511628211);
    return h;
}
static void strings(CHAR_DATA *ch,bool shared)
{
    char **slots[]={&ch->pcdata->conding,&ch->pcdata->love,
        &ch->pcdata->covenant,&ch->pcdata->cov_time,&ch->pcdata->leader,
        &ch->pcdata->vleader,&ch->lord,&ch->clan,&ch->morph};
    for(unsigned i=0;i<9;i++) {
        free_string(*slots[i]);
        char text[128];snprintf(text,sizeof text,"owned reset value %u",i);
        /* Native snapshots use an empty label; governance is independent. */
        if(i==7)snprintf(text,sizeof text,"");
        *slots[i]=shared?pooled(text):str_dup(text);
        tracked[i]=*slots[i];freed[i]=0;
    }
}
static CHAR_DATA *target(long cls,int level,int married,bool shared)
{
    CHAR_DATA *victim=player("Target");
    victim->class=cls;victim->level=level;victim->gold=1234;victim->primal=4321;
    victim->exp=1000000;victim->pcdata->quest=50000;
    if(married)SET_BIT(victim->extra,EXTRA_MARRIED);
    SET_BIT(victim->act,PLR_AUTOEXIT);
    strings(victim,shared);
    return victim;
}
static void load_reset(void)
{
    DESCRIPTOR_DATA shell={0};
    assert(load_char_obj(&shell,"Target"));
    CHAR_DATA *loaded=shell.character;
    assert(loaded&&!loaded->pcdata->player_load_failed);
    assert(loaded->class==0&&!loaded->pcdata->hybrid_present);
    loaded->desc=NULL;loaded->next=char_list;char_list=loaded;
    char_to_room(loaded,&home);
}
static void hybrid_case(unsigned id,int order,bool disk_only)
{
    const HYBRID_DEFINITION *definition=hybrid_by_id(id);
    assert(definition);
    CHAR_DATA *staff=player("Staff");staff->level=LEV_IMP;
    CHAR_DATA *victim=target(definition->parents[order],LEV_AWA,0,FALSE);
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(victim,definition,&state));
    assert(hybrid_player_attach(victim,&state));
    assert(save_char_obj_checked(victim));
    do_clearclass(staff,"Target");
    assert(victim->class==0&&victim->gold==1234&&victim->primal==4321);
    if(!disk_only) {
        assert(!victim->pcdata->hybrid_present);
        const unsigned char *bytes=(const unsigned char *)&victim->pcdata->hybrid;
        for(size_t i=0;i<sizeof victim->pcdata->hybrid;i++)assert(bytes[i]==0);
        assert(hybrid_player_save_valid(victim));
        assert(save_char_obj_checked(victim));
    }
    load_reset();cleanup();
}
static void string_case(bool shared)
{
    CHAR_DATA *staff=player("Staff");staff->level=LEV_IMP;
    CHAR_DATA *victim=target(CLASS_MONK,LEV_AWA,0,shared);
    free_string(victim->clan);
    victim->clan=shared?pooled("legacy label"):str_dup("legacy label");
    tracked[7]=victim->clan;
    counting=TRUE;do_clearclass(staff,"Target");counting=FALSE;
    for(unsigned i=0;i<9;i++)assert(freed[i]==1);
    assert(!*victim->morph&&!strcmp(victim->pcdata->cov_time,"None"));
    assert(IS_SET(victim->act,PLR_AUTOEXIT));
    cleanup();
}
static void effects_case(void)
{
    CHAR_DATA *staff=player("Staff");staff->level=LEV_IMP;
    CHAR_DATA *victim=target(CLASS_MAGE,LEV_AWA,0,FALSE);
    AFFECT_DATA effect={0};
    effect.type=0;effect.duration=10;effect.location=APPLY_HITROLL;
    effect.modifier=7;effect.bitvector=AFF_SANCTUARY;
    affect_to_char(victim,&effect);
    assert(victim->affected&&victim->hitroll==7);
    object_index.vnum=91001;object_index.name=object_index.short_descr=pooled("test armor");
    object_index.description=str_empty;object_index.item_type=ITEM_ARMOR;
    object_index.wear_flags=ITEM_TAKE|ITEM_WEAR_BODY;
    obj_index_hash[object_index.vnum%MAX_KEY_HASH]=&object_index;
    OBJ_DATA *gear=create_object(&object_index,0);
    obj_to_char(gear,victim);equip_char(victim,gear,WEAR_BODY);
    assert(get_eq_char(victim,WEAR_BODY)==gear);
    do_clearclass(staff,"Target");
    assert(!victim->affected&&victim->hitroll==0&&victim->armor==0);
    assert(gear->carried_by==victim&&gear->wear_loc==WEAR_NONE);
    cleanup();
}
static void invalid_record(int failure)
{
    CHAR_DATA *staff=player("Staff");staff->level=LEV_IMP;
    CHAR_DATA *victim=target(CLASS_WEREWOLF,LEV_AWA,0,FALSE);
    if(failure==0)victim->pcdata->hybrid_present=2;
    else victim->pcdata->player_load_failed=1;
    long saved_hit=victim->max_hit,saved_exp=victim->exp;
    char *saved_morph=victim->morph;
    do_clearclass(staff,"Target");
    assert(victim->class==CLASS_WEREWOLF&&victim->max_hit==saved_hit&&
           victim->exp==saved_exp&&victim->morph==saved_morph);
    /* Restore synthetic invalid metadata before ordinary fixture teardown. */
    victim->pcdata->hybrid_present=0;victim->pcdata->player_load_failed=0;
    cleanup();
}
static void native_cases(bool dump)
{
    const int levels[]={LEV_AWA,LEV_ST,LEV_IMP};
    uint64_t h=UINT64_C(1469598103934665603);unsigned count=0;
    for(int cls=0;cls<10;cls++)for(int lev=0;lev<3;lev++)
    for(int married=0;married<2;married++)for(int shared=0;shared<2;shared++) {
        CHAR_DATA *staff=player("Staff");staff->level=LEV_IMP;
        CHAR_DATA *victim=target(classes[cls],levels[lev],married,shared);
        for(int cycle=0;cycle<2;cycle++) {
            do_clearclass(staff,"Target");
            char row[1024];uint64_t text=UINT64_C(1469598103934665603);
            const char *values[]={victim->pcdata->conding,victim->pcdata->love,
                victim->pcdata->covenant,victim->pcdata->cov_time,
                victim->pcdata->leader,victim->pcdata->vleader,
                victim->lord,victim->clan,victim->morph};
            for(unsigned i=0;i<9;i++)text=hash_text(values[i],text);
            int n=snprintf(row,sizeof row,
                "%d,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%llu\n",
                cls,lev,married,shared,cycle,victim->class,victim->level,
                victim->max_hit,victim->max_mana,victim->exp,victim->pcdata->quest,
                victim->gold,victim->primal,victim->act,victim->extra,
                victim->affected_by,(unsigned long long)text);
            assert(n>0&&(size_t)n<sizeof row);
            if(dump)fputs(row,stdout);
            h=hash_text(row,h);++count;
        }
        cleanup();
    }
    assert(count==240);
    assert(h==UINT64_C(2039951304154796781));
    if(!dump)printf("native class-reset rows=%u hash=%llu\n",count,(unsigned long long)h);
}
int main(int argc,char **argv)
{
    char root[]="/tmp/anathema-class-reset-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root));
    assert(chdir(root)==0&&mkdir("player",0700)==0&&mkdir("backup",0700)==0&&
           mkdir("run",0700)==0&&chdir("run")==0);
    string_space=malloc(4*1024*1024);assert(string_space);
    top_string=string_space;current_time=123456;init_mm();
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    leader_board.bestpk_number=leader_board.tt_number=LONG_MAX;
    leader_board.worstpk_number=LONG_MIN;
    home.vnum=900;home.area=&area;home.name="Private room";home.description="";home.light=1;
    room_index_hash[home.vnum%MAX_KEY_HASH]=&home;
    if(argc==2&&!strcmp(argv[1],"--native-dump"))native_cases(TRUE);
    else if(argc>2&&!strcmp(argv[1],"--hybrid"))hybrid_case(atoi(argv[2]),
        argc>3?atoi(argv[3]):0,argc>4?atoi(argv[4]):0);
    else if(argc==3&&!strcmp(argv[1],"--strings"))string_case(atoi(argv[2]));
    else if(argc==3&&!strcmp(argv[1],"--invalid"))invalid_record(atoi(argv[2]));
    else {
        native_cases(FALSE);
        for(unsigned id=1;id<=hybrid_count();id++)for(int order=0;order<2;order++)
            hybrid_case(id,order,FALSE);
        string_case(FALSE);string_case(TRUE);invalid_record(0);invalid_record(1);
        effects_case();do_clearclass(NULL,NULL);
        puts("class reset: 90 hybrid clears/reloads, nine owned/pooled strings, guards, real effects/equipment passed");
    }
    assert(fclose(fpReserve)==0&&chdir("..")==0);
    remove_saves("player");remove_saves("backup");
    assert(rmdir("run")==0&&chdir(cwd)==0&&rmdir(root)==0);
    return 0;
}
