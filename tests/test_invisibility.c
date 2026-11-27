#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main
void char_update(void);
static OBJ_DATA items[2];
static OBJ_INDEX_DATA prototypes[2];
static AFFECT_DATA item_affects[2],*spell_affects[2];

static void setup(void)
{
    for(int n=0;n<3;n++)
        while(actors[n].affected)affect_remove(&actors[n],actors[n].affected);
    fixture(CLASS_WEREWOLF,0,0,-1,0);
    actors[0].carrying=NULL;
    memset(items,0,sizeof items);memset(prototypes,0,sizeof prototypes);
    memset(item_affects,0,sizeof item_affects);
    memset(spell_affects,0,sizeof spell_affects);
    gsn_invis=skill_lookup("invis");gsn_mass_invis=skill_lookup("mass invis");
    assert(gsn_invis>=0&&gsn_mass_invis>=0);
    weather_info.sunlight=SUN_DARK;time_info.hour=0;
    for(int n=0;n<3;n++){
        actors[n].blood[BLOOD_CURRENT]=actors[n].blood[BLOOD_POOL]=100000;
        actors[n].save_attempt_time=current_time;
    }
}

static void add_source(int slot,int kind)
{
    if(kind<2){
        AFFECT_DATA af={0};
        af.type=kind?gsn_mass_invis:gsn_invis;af.duration=24;
        af.location=APPLY_NONE;af.bitvector=AFF_INVISIBLE;
        affect_to_char(actors,&af);spell_affects[slot]=actors->affected;return;
    }
    OBJ_DATA *obj=&items[slot];
    obj->pIndexData=&prototypes[slot];obj->name="veil";
    obj->short_descr="a test veil";obj->weight=1;obj->level=1;
    obj->item_type=ITEM_ARMOR;obj->wear_flags=ITEM_TAKE;
    obj->wear_loc=WEAR_NONE;obj->carried_by=actors;
    obj->next_content=actors->carrying;actors->carrying=obj;
    if(kind==2)obj->value[3]=5;
    if(kind==3)obj->value[3]=46;
    if(kind==4){obj->item_type=ITEM_WEAPON;obj->value[0]=5000;}
    if(kind>=5){
        item_affects[slot].location=APPLY_NONE;
        item_affects[slot].bitvector=AFF_INVISIBLE;
        if(kind==5)prototypes[slot].affected=&item_affects[slot];
        else obj->affected=&item_affects[slot];
    }
    equip_char(actors,obj,slot?WEAR_ABOUT:WEAR_BODY);
    assert(obj->wear_loc==(slot?WEAR_ABOUT:WEAR_BODY));
}

static void remove_source(int slot,int kind)
{
    if(kind<2)affect_remove(actors,spell_affects[slot]);
    else unequip_char(actors,&items[slot]);
}

static void ownership(void)
{
    unsigned cases=0;
    for(int a=0;a<7;a++)for(int b=0;b<7;b++)for(int first=0;first<2;first++){
        setup();SET_BIT(actors->affected_by,AFF_INFRARED);
        add_source(0,a);add_source(1,b);
        assert(IS_AFFECTED(actors,AFF_INVISIBLE));
        remove_source(first,first?b:a);
        assert(IS_AFFECTED(actors,AFF_INVISIBLE));
        assert(!can_see(actors+1,actors));
        remove_source(!first,first?a:b);
        assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
        assert(IS_AFFECTED(actors,AFF_INFRARED));
        assert(can_see(actors+1,actors));cases++;
    }
    assert(cases==98);
    setup();add_source(0,0);add_source(1,3);
    for(int i=0;i<25;i++){
        for(int n=0;n<3;n++)actors[n].timer=0;
        char_update();
        assert(IS_AFFECTED(actors,AFF_INVISIBLE));
    }
    assert(!is_affected(actors,gsn_invis));
    remove_source(1,3);assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    setup();add_source(0,0);do_visible(actors,"");
    assert(!is_affected(actors,gsn_invis)&&!IS_AFFECTED(actors,AFF_INVISIBLE));
    setup();add_source(0,0);add_source(1,2);do_visible(actors,"");
    assert(!is_affected(actors,gsn_invis)&&!IS_AFFECTED(actors,AFF_INVISIBLE));
    assert(items[1].wear_loc==WEAR_ABOUT);
    refresh_equipment_state(actors);assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    setup();add_source(0,5);add_source(1,6);do_visible(actors,"");
    assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    remove_source(0,5);assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    refresh_equipment_state(actors);assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    remove_source(1,6);assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    setup();add_source(0,5);add_source(1,2);
    prototypes[1].affected=prototypes[0].affected;
    items[1].value[3]=0;
    remove_source(0,5);assert(IS_AFFECTED(actors,AFF_INVISIBLE));
    remove_source(1,5);assert(!IS_AFFECTED(actors,AFF_INVISIBLE));
    puts("Ownership:98 two-source/removal-order cases;real expiry and explicit reveal passed.");
}

static void visibility(void)
{
    const long classes[]={CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,CLASS_DROW,
        CLASS_HIGHLANDER,CLASS_MAGE,CLASS_DRAGON,CLASS_NINJA,CLASS_MONK,CLASS_ELEM};
    unsigned cases=0;
    for(unsigned c=0;c<10;c++)for(int mode=0;mode<15;mode++){
        setup();CHAR_DATA *viewer=actors+1;viewer->class=classes[c];
        bool expected=FALSE;
        switch(mode){
        case 1:SET_BIT(viewer->affected_by,AFF_DETECT_INVIS);expected=TRUE;break;
        case 2:SET_BIT(viewer->act,PLR_HOLYLIGHT);expected=TRUE;break;
        case 3:SET_BIT(viewer->itemaffect,ITEMA_VISION);expected=TRUE;break;
        case 4:viewer->level=LEV_BUI;expected=TRUE;break;
        case 5:viewer=actors;expected=TRUE;break;
        case 6:SET_BIT(viewer->act,ACT_IS_NPC);break;
        case 7:SET_BIT(viewer->act,ACT_IS_NPC);
               SET_BIT(viewer->affected_by,AFF_DETECT_INVIS);expected=TRUE;break;
        case 8:SET_BIT(viewer->affected_by,AFF_DETECT_INVIS|AFF_BLIND);break;
        case 9:SET_BIT(viewer->act,PLR_HOLYLIGHT);
               SET_BIT(viewer->affected_by,AFF_BLIND);expected=TRUE;break;
        case 10:SET_BIT(viewer->affected_by,AFF_DETECT_INVIS|AFF_SHADOWPLANE);break;
        case 11:SET_BIT(viewer->affected_by,AFF_DETECT_INVIS);
                SET_BIT(actors->affected_by,AFF_KAKUSU);expected=classes[c]==CLASS_NINJA;break;
        case 12:SET_BIT(viewer->affected_by,AFF_DETECT_INVIS);
                SET_BIT(actors->itemaffect,ITEMA_BLISSFUL);break;
        case 13:SET_BIT(viewer->affected_by,AFF_DETECT_INVIS);
                SET_BIT(actors->vampaff,VAM_EARTHMELDED);break;
        case 14:SET_BIT(viewer->extra,BLINDFOLDED);
                SET_BIT(viewer->itemaffect,ITEMA_VISION);break;
        }
        SET_BIT(actors->affected_by,AFF_INVISIBLE);
        assert(can_see(viewer,actors)==expected);cases++;
    }
    assert(cases==150);
    setup();add_source(0,0);
    assert(get_char_room(actors+1,"alpha")==NULL);
    SET_BIT(actors[1].affected_by,AFF_DETECT_INVIS);
    assert(get_char_room(actors+1,"alpha")==actors);
    setup();SET_BIT(room.added_flags,ROOM2_DARKNESS);
    pcs[1].tribes[TRIBE_UKTENA]=1;actors[1].class=CLASS_WEREWOLF;
    SET_BIT(actors->affected_by,AFF_INVISIBLE);
    assert(!can_see(actors+1,actors));
    SET_BIT(actors[1].affected_by,AFF_DETECT_INVIS);
    assert(can_see(actors+1,actors));
    setup();actors[0].class=CLASS_VAMPIRE;SET_BIT(actors->vampaff,VAM_OBFUSCATE);
    pcs[0].powers[DISC_OBFUSCATE]=3;
    assert(!can_see(actors+1,actors));
    actors[1].class=CLASS_VAMPIRE;
    pcs[1].powers[DISC_AUSPEX]=3;assert(can_see(actors+1,actors));
    SET_BIT(actors->affected_by,AFF_INVISIBLE);
    assert(!can_see(actors+1,actors));
    SET_BIT(actors[1].affected_by,AFF_DETECT_INVIS);assert(can_see(actors+1,actors));
    puts("Visibility:150 class/sight cases;lookup,dark-room and independent concealment checks passed.");
}

static void uktena(void)
{
    unsigned guards=0;
    do_invisibility(NULL,NULL);
    for(int mode=0;mode<19;mode++){
        setup();pcs[0].tribes[TRIBE_UKTENA]=4;
        switch(mode){
        case 0:pcs[0].tribes[TRIBE_UKTENA]=3;break;
        case 1:actors->class=CLASS_MAGE;break;
        case 2:SET_BIT(actors->act,ACT_IS_NPC);break;
        case 3:actors->pcdata=NULL;break;
        case 4:actors->in_room=NULL;break;
        case 5:room.people=actors+1;break;
        case 6:actors->hit=0;break;
        case 7:actors->position=POS_SLEEPING;break;
        case 8:actors->position=POS_FIGHTING;break;
        case 9:actors->position=POS_RESTING;break;
        case 10:actors->wait=1;break;
        case 11:actors->combat_pause=1;break;
        case 12:SET_BIT(pcs[0].flag,FLAG_AFK);break;
        case 13:pcs[0].player_load_failed=1;break;
        case 14:actors->level=LEV_SLE;break;
        case 15:SET_BIT(actors->affected_by,AFF_INVISIBLE);break;
        case 16:add_source(0,0);break;
        case 17:add_source(0,0);REMOVE_BIT(actors->affected_by,AFF_INVISIBLE);break;
        case 18:add_source(0,1);REMOVE_BIT(actors->affected_by,AFF_INVISIBLE);break;
        }
        AFFECT_DATA *before=actors->affected;
        long mana=actors->mana,wait=actors->wait,primal=actors->primal;
        do_invisibility(actors,"");
        assert(actors->affected==before&&actors->mana==mana&&actors->wait==wait);
        assert(actors->primal==primal);guards++;
    }
    for(int rank=4;rank<=5;rank++)for(int safe=0;safe<2;safe++){
        setup();pcs[0].tribes[TRIBE_UKTENA]=rank;
        if(safe)SET_BIT(room.room_flags,ROOM_SAFE);
        actors->mana=0;long primal=actors->primal,dp=pcs[0].classpoints[CP_CURRENT];
        char command[]="invisibility";interpret(actors,command);
        assert(actors->affected&&actors->affected->type==gsn_invis);
        assert(actors->affected->duration==24&&IS_AFFECTED(actors,AFF_INVISIBLE));
        assert(actors->mana==0&&actors->primal==primal&&pcs[0].classpoints[CP_CURRENT]==dp);
        assert(actors->wait==12&&!can_see(actors+1,actors));
        AFFECT_DATA *before=actors->affected;actors->wait=0;
        interpret(actors,command);
        assert(actors->affected==before&&actors->affected->next==NULL&&actors->wait==0);
    }
    for(unsigned p=0;p<sizeof(parents)/sizeof(*parents);p++)for(int order=0;order<2;order++){
        if(parents[p]==CLASS_WEREWOLF)continue;
        setup();pcs[0].tribes[TRIBE_UKTENA]=4;
        long primary=order?parents[p]:CLASS_WEREWOLF;
        long secondary=order?CLASS_WEREWOLF:parents[p];
        HYBRID_STATE state;actors->class=primary;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        assert(hybrid_player_attach(actors,&state));
        char command[]="invisibility";interpret(actors,command);
        assert((actors->affected!=NULL)==!order);
    }
    assert(guards==19);
    puts("Uktena:19 rejection guards;rank4/5,safe-room,zero-cost,recovery,non-refresh and original-parent access passed.");
}

static void persistence(void)
{
    char cwd[4096],root[]="/tmp/anathema-invis-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    setup();DESCRIPTOR_DATA d={0};assert(!load_char_obj(&d,"Invisfixture"));
    CHAR_DATA *saved=d.character;
    saved->level=LEV_AWA;saved->class=CLASS_WEREWOLF;
    saved->hit=saved->max_hit=50000;saved->position=POS_STANDING;
    saved->in_room=&room;saved->desc=&d;d.connected=CON_PLAYING;
    saved->next_in_room=room.people;room.people=saved;
    saved->pcdata->tribes[TRIBE_UKTENA]=4;
    saved->mana=0;do_invisibility(saved,"");
    assert(saved->affected&&saved->affected->type==gsn_invis&&saved->affected->duration==24);
    assert(saved->mana==0&&saved->wait==12);
    AFFECT_DATA af={0};af.location=APPLY_NONE;af.bitvector=AFF_INVISIBLE;
    af.type=gsn_mass_invis;af.duration=7;affect_to_char(saved,&af);
    assert(save_char_obj_checked(saved));
    room.people=saved->next_in_room;saved->next_in_room=NULL;
    saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    for(int brief=0;brief<2;brief++){
        memset(&d,0,sizeof d);
        assert(brief?load_char_short(&d,"Invisfixture"):load_char_obj(&d,"Invisfixture"));
        saved=d.character;
        assert(!saved->pcdata->player_load_failed&&IS_AFFECTED(saved,AFF_INVISIBLE));
        for(AFFECT_DATA *a=saved->affected;a;a=a->next)
            assert(a->duration==(a->type==gsn_invis?24:7));
        affect_strip(saved,gsn_invis);
        assert(IS_AFFECTED(saved,AFF_INVISIBLE)&&is_affected(saved,gsn_mass_invis));
        affect_strip(saved,gsn_mass_invis);assert(!IS_AFFECTED(saved,AFF_INVISIBLE));
        saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    }
    assert(!unlink("../player/Invisfixture"));(void)unlink("../backup/Invisfixture");
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("run")&&!rmdir("player")&&!rmdir("backup"));
    assert(!chdir(cwd)&&!rmdir(root));
    puts("Persistence:full/short reloads retain two durations and independent cleanup passed.");
}

int main(void)
{
    visibility();ownership();uktena();persistence();setup();
    puts("Shared invisibility and guarded Uktena passed;Mastery remains disabled.");
    return 0;
}
