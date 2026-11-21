/* Real-engine Disquiet: no replacement permission/effect/save implementations. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main
void char_update(void);

static void setup(long rank)
{
    for(int n=0;n<3;n++)
        while(actors[n].affected)affect_remove(&actors[n],actors[n].affected);
    fixture(CLASS_WEREWOLF,0,0,-1,0);
    pcs[0].breed[BREED_HOMID]=rank;
    for(int n=0;n<3;n++){
        pcs[n].flag=FLAG_PK;
        actors[n].hitroll=100;actors[n].damroll=200;
        actors[n].save_attempt_time=current_time;
    }
}

static void cast(void)
{
    char command[]="disquiet beta";
    actors[0].wait=0;
    interpret(actors,command);
}

static unsigned effects(CHAR_DATA *ch)
{
    unsigned count=0;
    for(AFFECT_DATA *a=ch->affected;a;a=a->next)count++;
    return count;
}

static void reduction(void)
{
    for(long rank=4;rank<=5;rank++){
        setup(rank);long mana=actors[0].mana,primal=actors[0].primal;
        cast();
        assert(effects(&actors[1])==2);
        assert(actors[1].hitroll==100-5*rank&&actors[1].damroll==200-5*rank);
        assert(actors[0].wait==12&&actors[0].mana==mana&&actors[0].primal==primal);
        assert(skill_lookup("disquiet")==113);
        for(AFFECT_DATA *a=actors[1].affected;a;a=a->next){
            assert(a->type==113&&a->duration==rank&&a->modifier==-5*rank);
            assert(a->bitvector==0);
        }
        cast();assert(effects(&actors[1])==2&&actors[0].wait==0);
        pcs[2].breed[BREED_HOMID]=5;actors[2].class=CLASS_WEREWOLF;
        do_disquiet(&actors[2],"beta");assert(effects(&actors[1])==2);
        affect_strip(&actors[1],113);
        assert(actors[1].hitroll==100&&actors[1].damroll==200&&!effects(&actors[1]));
    }
}

static void guards(void)
{
    for(int mode=0;mode<18;mode++){
        setup(4);
        switch(mode){
        case 0:pcs[0].breed[BREED_HOMID]=3;break;
        case 1:actors[0].class=CLASS_MAGE;break;
        case 2:SET_BIT(actors[0].act,ACT_IS_NPC);break;
        case 3:actors[0].pcdata=NULL;break;
        case 4:actors[0].in_room=NULL;break;
        case 5:room.room_flags=ROOM_SAFE;break;
        case 6:actors[1].level=LEV_ST;break;
        case 7:pcs[1].flag=FLAG_AFK|FLAG_PK;break;
        case 8:actors[1].desc=NULL;break;
        case 9:pcs[0].flag=0;break;
        case 10:pcs[1].flag=0;break;
        case 11:actors[1].pcdata=NULL;break;
        case 12:actors[1].hitroll=LONG_MIN;break;
        case 13:actors[1].damroll=LONG_MIN;break;
        case 14:SET_BIT(actors[1].itemaffect,ITEMA_PEACE);break;
        case 15:pcs[0].player_load_failed=1;break;
        case 16:SET_BIT(actors[1].affected_by,AFF_ETHEREAL);break;
        case 17:SET_BIT(actors[1].more,MORE_HEAD);break;
        }
        long hit=actors[1].hitroll,dam=actors[1].damroll;
        do_disquiet(actors,"beta");
        assert(!effects(&actors[1])&&actors[1].hitroll==hit&&actors[1].damroll==dam);
        assert(actors[0].wait==0);
    }
    setup(4);
    do_disquiet(NULL,NULL);do_disquiet(actors,NULL);do_disquiet(actors,"");
    do_disquiet(actors,"alpha");do_disquiet(actors,"missing");
    do_disquiet(actors,"beta extra");
    assert(!effects(actors)&&!effects(&actors[1])&&actors[0].wait==0);
}

static void resistance(void)
{
    setup(4);pcs[1].resist[WILL_PRESENCE]=1;
    cast();assert(!effects(&actors[1])&&actors[0].wait==12);
    setup(4);SET_BIT(pcs[1].resist[0],RES_PRESENCE);
    pcs[1].willpower[0]=10;
    cast();assert(!effects(&actors[1])&&pcs[1].willpower[0]==0);
    assert(pcs[1].resist[WILL_PRESENCE]==60);
    assert(!IS_WILLPOWER(&actors[1],RES_PRESENCE));
    setup(4);SET_BIT(pcs[1].resist[0],RES_PRESENCE|RES_TOGGLE);
    pcs[1].willpower[0]=10;cast();
    assert(!effects(&actors[1])&&IS_WILLPOWER(&actors[1],RES_PRESENCE));
    setup(4);SET_BIT(pcs[1].resist[0],RES_PRESENCE);
    pcs[1].willpower[0]=9;cast();assert(effects(&actors[1])==2);
    for(int resist=0;resist<2;resist++)for(int grant=0;grant<2;grant++){
        setup(4);SET_BIT(actors[1].more,MORE_GUARDIAN);
        if(grant){
            actors[1].class=CLASS_WEREWOLF;
            pcs[1].tribes[TRIBE_CHILDREN_OF_GAIA]=4;
        }
        pcs[1].resist[WILL_PRESENCE]=resist;
        cast();
        assert((strstr(outputs[1],"Your guardian spirit")!=NULL)==grant);
    }
}

static void numeric_and_native_targets(void)
{
    const long values[]={LONG_MIN,LONG_MIN+19,LONG_MIN+20,LONG_MIN+24,
        LONG_MIN+25,-1,0,1,LONG_MAX-25,LONG_MAX};
    for(long rank=4;rank<=5;rank++)
    for(unsigned a=0;a<sizeof(values)/sizeof(*values);a++)
    for(unsigned b=0;b<sizeof(values)/sizeof(*values);b++){
        setup(rank);actors[1].hitroll=values[a];actors[1].damroll=values[b];
        cast();
        if(values[a]<LONG_MIN+5*rank||values[b]<LONG_MIN+5*rank){
            assert(!effects(&actors[1])&&actors[0].wait==0);
        }else{
            assert(effects(&actors[1])==2);
            assert(actors[1].hitroll==values[a]-5*rank);
            assert(actors[1].damroll==values[b]-5*rank);
            affect_strip(&actors[1],113);
        }
        assert(actors[1].hitroll==values[a]&&actors[1].damroll==values[b]);
    }
    setup(4);SET_BIT(actors[1].act,ACT_IS_NPC);actors[1].pcdata=NULL;
    cast();assert(effects(&actors[1])==2);
    affect_strip(&actors[1],113);
    assert(actors[1].hitroll==100&&actors[1].damroll==200);
    setup(4);SET_BIT(actors[1].act,ACT_IS_NPC|ACT_NOKILL);
    cast();assert(!effects(&actors[1]));
    setup(4);init_mm();long next=number_range(1,100000);
    setup(4);init_mm();cast();
    assert(number_range(1,100000)==next);
}

static void original_and_secondary_access(void)
{
    const long other[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
        CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};
    for(unsigned n=0;n<9;n++)for(int order=0;order<2;order++){
        setup(4);HYBRID_STATE state;
        long first=order?other[n]:CLASS_WEREWOLF;
        long second=order?CLASS_WEREWOLF:other[n];
        actors[0].class=first;
        assert(hybrid_secondary_initialize(actors,hybrid_by_parents(first,second),&state));
        assert(hybrid_player_attach(actors,&state));
        cast();assert(effects(&actors[1])==(order?0:2));
    }
}

static void expiry(void)
{
    for(int rank=4;rank<=5;rank++){
        setup(rank);cast();assert(effects(&actors[1])==2);
        for(int n=0;n<rank;n++){
            char_update();
            assert(effects(&actors[1])==2);
        }
        char_update();
        assert(!effects(&actors[1]));
        assert(actors[1].hitroll==100&&actors[1].damroll==200);
    }
}

static void persistence(void)
{
    char cwd[4096],root[]="/tmp/anathema-disquiet-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    for(int rank=4;rank<=5;rank++){
        setup(rank);DESCRIPTOR_DATA d={0};
        assert(!load_char_obj(&d,"Disquietfixture"));
        CHAR_DATA *saved=d.character;
        saved->level=LEV_AWA;saved->class=CLASS_NONE;
        saved->hit=saved->max_hit=50000;saved->mana=saved->max_mana=50000;
        saved->hitroll=100;saved->damroll=200;saved->position=POS_STANDING;
        saved->pcdata->flag=FLAG_PK;saved->in_room=&room;saved->desc=&d;
        saved->next_in_room=room.people;room.people=saved;
        do_disquiet(actors,"Disquietfixture");
        assert(effects(saved)==2&&save_char_obj_checked(saved));
        saved->next_in_room=NULL;saved->desc=NULL;saved->in_room=NULL;free_char(saved);
        room.people=actors;
        for(int brief=0;brief<2;brief++){
            memset(&d,0,sizeof d);
            assert(brief?load_char_short(&d,"Disquietfixture"):load_char_obj(&d,"Disquietfixture"));
            saved=d.character;assert(!saved->pcdata->player_load_failed);
            assert(saved->hitroll==100-5*rank&&saved->damroll==200-5*rank);
            assert(effects(saved)==2);
            for(AFFECT_DATA *a=saved->affected;a;a=a->next)
                assert(a->type==113&&a->duration==rank&&a->modifier==-5*rank);
            affect_strip(saved,113);
            assert(saved->hitroll==100&&saved->damroll==200&&!effects(saved));
            saved->desc=NULL;saved->in_room=NULL;free_char(saved);
        }
        assert(!unlink("../player/Disquietfixture"));
        (void)unlink("../backup/Disquietfixture");
    }
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("run")&&!rmdir("player")&&!rmdir("backup"));
    assert(!chdir(cwd)&&!rmdir(root));
}

int main(void)
{
    reduction();guards();resistance();numeric_and_native_targets();
    original_and_secondary_access();expiry();persistence();
    for(int n=0;n<3;n++)
        while(actors[n].affected)affect_remove(&actors[n],actors[n].affected);
    puts("Disquiet: 200 numeric pairs, access, non-stacking, resistance, real expiry and four full/short reloads passed");
    return 0;
}
