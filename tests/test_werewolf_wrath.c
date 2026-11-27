/* Real command dispatch, target policy, fear affects, expiry and saved reloads. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main
void char_update(void);

static unsigned effects(CHAR_DATA *ch)
{
    unsigned count=0;
    for(AFFECT_DATA *af=ch->affected;af;af=af->next)count++;
    return count;
}

static void setup(long rank)
{
    for(int n=0;n<3;n++)
        while(actors[n].affected)affect_remove(&actors[n],actors[n].affected);
    fixture(CLASS_WEREWOLF,0,0,-1,0);
    pcs[0].tribes[TRIBE_SILVER_FANGS]=rank;
    actors[0].beast=0;
    actors[0].mana=0;
    actors[1].class=CLASS_VAMPIRE;
    actors[1].beast=1;
    weather_info.sunlight=SUN_DARK;time_info.hour=0;
    for(int n=0;n<3;n++){
        pcs[n].flag=FLAG_PK;
        actors[n].hitroll=100;actors[n].damroll=200;
        actors[n].blood[BLOOD_CURRENT]=actors[n].blood[BLOOD_POOL]=100000;
        actors[n].save_attempt_time=current_time;
    }
}

static void cast(void)
{
    char command[]="wrath";
    interpret(actors,command);
}

static void guards(void)
{
    do_wrath(NULL,NULL);
    for(int mode=0;mode<45;mode++){
        setup(3);
        switch(mode){
        case 0:pcs[0].tribes[TRIBE_SILVER_FANGS]=2;break;
        case 1:actors[0].class=CLASS_MAGE;break;
        case 2:SET_BIT(actors[0].act,ACT_IS_NPC);break;
        case 3:actors[0].pcdata=NULL;break;
        case 4:actors[0].in_room=NULL;break;
        case 5:actors[0].hit=0;break;
        case 6:actors[0].position=POS_SLEEPING;break;
        case 7:actors[0].wait=1;break;
        case 8:actors[0].combat_pause=1;break;
        case 9:pcs[0].player_load_failed=1;break;
        case 10:actors[0].level=LEV_SLE;break;
        case 11:SET_BIT(room.room_flags,ROOM_SAFE);break;
        case 12:pcs[0].flag=0;break;
        case 13:pcs[0].flag|=FLAG_AFK;break;
        case 14:actors[1].class=CLASS_NONE;break;
        case 15:SET_BIT(actors[1].act,ACT_IS_NPC);break;
        case 16:actors[1].pcdata=NULL;break;
        case 17:pcs[1].player_load_failed=1;break;
        case 18:actors[1].beast=0;break;
        case 19:actors[1].beast=LONG_MIN;break;
        case 20:actors[1].hit=0;break;
        case 21:actors[1].position=POS_SLEEPING;break;
        case 22:actors[1].level=LEV_SLE;break;
        case 23:actors[1].level=LEV_IMM;break;
        case 24:pcs[1].flag=0;break;
        case 25:pcs[1].flag|=FLAG_AFK;break;
        case 26:actors[1].desc=NULL;break;
        case 27:descs[1].connected=CON_GET_NAME;break;
        case 28:actors[1].leader=actors;break;
        case 29:SET_BIT(actors[1].affected_by,AFF_SHADOWPLANE);break;
        case 30:SET_BIT(actors[1].affected_by,AFF_DEEPUMBRA);break;
        case 31:SET_BIT(actors[1].affected_by,AFF_ETHEREAL);break;
        case 32:SET_BIT(actors[1].itemaffect,ITEMA_PEACE);break;
        case 33:SET_BIT(actors[1].vampaff,VAM_EARTHMELDED);break;
        case 34:SET_BIT(actors[1].polyaff,POLY_MIST);break;
        case 35:actors[1].hitroll=LONG_MIN;break;
        case 36:actors[1].damroll=LONG_MIN;break;
        case 37:actors[1].in_room=NULL;break;
        case 38:SET_BIT(actors[0].affected_by,AFF_CHARM);
                actors[0].master=&actors[1];break;
        case 39:SET_BIT(actors[1].affected_by,AFF_FEAR);break;
        case 40:actors[0].in_room->people=&actors[1];break;
        case 41:actors[1].leader=actors;actors[0].fighting=&actors[1];
                actors[1].fighting=actors;break;
        case 42:SET_BIT(actors[1].more,MORE_HEAD);break;
        case 43:SET_BIT(actors[1].itemaffect,ITEMA_PEACE);
                actors[1].fight_timer=1;break;
        case 44:SET_BIT(actors[0].itemaffect,ITEMA_PEACE);
                actors[1].fight_timer=1;break;
        }
        long hit=actors[1].hitroll,dam=actors[1].damroll,wait=actors[0].wait;
        long flags=actors[1].affected_by;
        init_mm();long next=number_mm();init_mm();
        do_wrath(actors,"");
        assert(!effects(&actors[1])&&actors[1].hitroll==hit&&actors[1].damroll==dam);
        assert(actors[1].affected_by==flags);
        assert(actors[0].mana==0&&actors[0].wait==wait&&number_mm()==next);
    }
}

static void ordinary_and_refresh(void)
{
    unsigned durations=0;
    for(long rank=3;rank<=5;rank++)for(int seed=0;seed<64;seed++){
        setup(rank);init_mm();
        for(int i=0;i<seed;i++)(void)number_mm();
        long primal=actors[0].primal,hp=actors[1].hit;
        cast();
        assert(effects(&actors[1])==2&&IS_AFFECTED(&actors[1],AFF_FEAR));
        assert(actors[1].hitroll==100-rank&&actors[1].damroll==200-rank);
        assert(actors[0].wait==20&&actors[0].mana==0&&actors[0].primal==primal);
        assert(actors[0].beast==0&&actors[1].beast==1&&actors[1].hit==hp);
        assert(!actors[0].combat_pause&&!actors[1].combat_pause);
        assert(!actors[0].stun_immunity&&!actors[1].stun_immunity);
        AFFECT_DATA *af=actors[1].affected;
        long duration=af->duration;
        assert(duration>=1&&duration<=2&&af->next->duration==duration);
        durations|=1u<<duration;
        for(;af;af=af->next)
            assert(af->type==skill_lookup("fear")&&af->modifier==-rank&&af->bitvector==AFF_FEAR);
        actors[0].wait=0;cast();
        assert(effects(&actors[1])==2&&actors[0].wait==0);
        assert(actors[1].affected->duration==duration);
        pcs[2].tribes[TRIBE_SILVER_FANGS]=5;actors[2].class=CLASS_WEREWOLF;
        do_wrath(&actors[2],"");
        assert(effects(&actors[1])==2&&actors[2].wait==0);
        affect_strip(&actors[1],skill_lookup("fear"));
        assert(!effects(&actors[1])&&!IS_AFFECTED(&actors[1],AFF_FEAR));
        assert(actors[1].hitroll==100&&actors[1].damroll==200);
    }
    assert(durations==6);

    setup(3);actors[2].class=CLASS_VAMPIRE;actors[2].beast=1;
    cast();assert(effects(&actors[1])==2&&effects(&actors[2])==2&&actors[0].wait==20);
    setup(3);actors[2].class=CLASS_VAMPIRE;actors[2].beast=1;
    pcs[1].flag=FLAG_AFK|FLAG_PK;
    cast();assert(!effects(&actors[1])&&effects(&actors[2])==2&&actors[0].wait==20);
    setup(3);AFFECT_DATA af={0};
    af.type=skill_lookup("fear");af.duration=2;af.location=APPLY_NONE;
    affect_to_char(&actors[1],&af);
    cast();assert(effects(&actors[1])==1&&actors[0].wait==0);
}

static void numeric_boundaries(void)
{
    for(long rank=3;rank<=5;rank++){
        const long values[]={LONG_MIN,LONG_MIN+rank-1,LONG_MIN+rank,
            -1,0,1,LONG_MAX-1,LONG_MAX};
        for(unsigned a=0;a<8;a++)for(unsigned b=0;b<8;b++){
            setup(rank);actors[1].hitroll=values[a];actors[1].damroll=values[b];
            cast();
            bool success=values[a]>=LONG_MIN+rank&&values[b]>=LONG_MIN+rank;
            assert(effects(&actors[1])==(success?2u:0u));
            assert(actors[0].wait==(success?20:0)&&actors[0].mana==0);
            if(success){
                assert(actors[1].hitroll==values[a]-rank&&actors[1].damroll==values[b]-rank);
                affect_strip(&actors[1],skill_lookup("fear"));
            }
            assert(actors[1].hitroll==values[a]&&actors[1].damroll==values[b]);
        }
    }
}

static void expiry_and_access(void)
{
    for(int seed=0;seed<8;seed++){
        setup(3);init_mm();for(int i=0;i<seed;i++)(void)number_mm();
        cast();long duration=actors[1].affected->duration;
        for(int i=0;i<duration;i++){
            char_update();assert(effects(&actors[1])==2);
        }
        char_update();
        assert(!effects(&actors[1])&&!IS_AFFECTED(&actors[1],AFF_FEAR));
        assert(actors[1].hitroll==100&&actors[1].damroll==200);
    }
    for(unsigned i=0;i<sizeof(parents)/sizeof(*parents);i++)for(int order=0;order<2;order++){
        if(parents[i]==CLASS_WEREWOLF)continue;
        setup(3);HYBRID_STATE state;
        long primary=order?parents[i]:CLASS_WEREWOLF;
        long secondary=order?CLASS_WEREWOLF:parents[i];
        actors[0].class=primary;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(primary,secondary),&state));
        assert(hybrid_player_attach(actors,&state));
        cast();assert(effects(&actors[1])==(order?0u:2u));
    }
}

static void persistence(void)
{
    char cwd[4096],root[]="/tmp/anathema-wrath-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    setup(3);DESCRIPTOR_DATA d={0};
    assert(!load_char_obj(&d,"Wrathfixture"));
    CHAR_DATA *saved=d.character;
    saved->level=LEV_AWA;saved->class=CLASS_VAMPIRE;saved->beast=1;
    saved->hit=saved->max_hit=50000;saved->mana=saved->max_mana=50000;
    saved->hitroll=100;saved->damroll=200;saved->position=POS_STANDING;
    saved->pcdata->flag=FLAG_PK;saved->in_room=&room;saved->desc=&d;d.connected=CON_PLAYING;
    saved->next_in_room=room.people;room.people=saved;
    do_wrath(actors,"");
    long duration=saved->affected->duration;
    assert(effects(saved)==2&&save_char_obj_checked(saved));
    room.people=saved->next_in_room;
    saved->next_in_room=NULL;saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    for(int brief=0;brief<2;brief++){
        memset(&d,0,sizeof d);
        assert(brief?load_char_short(&d,"Wrathfixture"):load_char_obj(&d,"Wrathfixture"));
        saved=d.character;assert(!saved->pcdata->player_load_failed);
        assert(saved->hitroll==97&&saved->damroll==197&&effects(saved)==2);
        assert(IS_AFFECTED(saved,AFF_FEAR)&&saved->affected->duration==duration);
        affect_strip(saved,skill_lookup("fear"));
        assert(saved->hitroll==100&&saved->damroll==200&&!effects(saved));
        assert(!IS_AFFECTED(saved,AFF_FEAR));
        saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    }
    assert(!unlink("../player/Wrathfixture"));(void)unlink("../backup/Wrathfixture");
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("run")&&!rmdir("player")&&!rmdir("backup"));
    assert(!chdir(cwd)&&!rmdir(root));
}

int main(void)
{
    guards();ordinary_and_refresh();numeric_boundaries();expiry_and_access();persistence();
    for(int n=0;n<3;n++)
        while(actors[n].affected)affect_remove(&actors[n],actors[n].affected);
    puts("Wrath:45 rejection guards,192 ordinary casts,192 numeric boundaries,"
         "room filtering, non-refresh, target Beast, zero mana, successful-only "
         "20-pulse recovery, original-parent access, real expiry and full/short reloads passed.");
    return 0;
}
