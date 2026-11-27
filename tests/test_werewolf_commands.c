/* Real interpreter and native Werewolf commands; secondary quotes use the real
 * training backend. No replacement permission, price or persistence routines. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main
#include "classes/hybrid/training.h"
#include "classes/werewolf/advancement.h"

static void wolf_fixture(long other, int secondary)
{
    fixture(secondary ? other : CLASS_WEREWOLF,
            secondary ? CLASS_WEREWOLF : other,0,-1,0);
    memset(pcs[0].breed,0,sizeof(pcs[0].breed));
    memset(pcs[0].auspice,0,sizeof(pcs[0].auspice));
    memset(pcs[0].tribes,0,sizeof(pcs[0].tribes));
    pcs[0].rank=1;pcs[0].wolf=0;
    actors[0].primal=actors[0].exp=0;
    actors[0].morph=str_dup("");
    SET_BIT(room.room_flags,ROOM_SAFE);
}

static unsigned advancement_grid(void)
{
    static const long other_parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,
        CLASS_HIGHLANDER,CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};
    unsigned count=0;
    for(int mode=0;mode<19;mode++)
    for(long rank=1;rank<4;rank++)
    for(int p=-1;p<=1;p++)for(int x=-1;x<=1;x++)for(int k=-1;k<=1;k++){
        long other=mode?other_parents[(mode-1)%9]:0;
        int secondary=mode>=10;
        wolf_fixture(other,secondary);
        int hybrid=mode!=0;
        long primal=rank*100*(hybrid?2:1);
        long experience=rank*1000000*(hybrid?2:1);
        long kills=rank*1000*(hybrid?2:1);
        actors[0].primal=primal+p;actors[0].exp=experience+x;
        actors[0].mkill=kills+k;
        pcs[0].score[SCORE_NUM_QUEST]=0;pcs[0].quest=12345;
        if(secondary)pcs[0].hybrid.secondary_progression.wolf_rank=rank;
        else pcs[0].rank=rank;
        PC_DATA before=pcs[0];
        int success=p>=0&&x>=0&&k>=0;
        if(secondary){
            HYBRID_LESSON_REQUEST request={HYBRID_LESSON_WOLF_RANK,
                CLASS_WEREWOLF,0,0,0};
            HYBRID_LESSON result;
            enum hybrid_lesson_result status=hybrid_training_prepare(actors,&request,&result);
            assert(!memcmp(&pcs[0],&before,sizeof(before)));
            assert(status==(k<0?HYBRID_LESSON_PREREQUISITE:
                           p<0||x<0?HYBRID_LESSON_NO_FUNDS:HYBRID_LESSON_OK));
            if(success){
                assert(result.state.secondary_progression.wolf_rank==rank+1);
                assert(result.primal_cost==primal&&result.experience_cost==experience);
                assert(result.primal==p&&result.experience==x&&result.quest==12345);
            }
        }else{
            char command[]="advance";interpret(actors,command);
            assert(pcs[0].rank==rank+success);
            assert(actors[0].primal==(success?p:primal+p));
            assert(actors[0].exp==(success?x:experience+x));
            before.rank=pcs[0].rank;
            assert(!memcmp(&pcs[0],&before,sizeof(before)));
        }
        assert(actors[0].mkill==kills+k);
        assert(pcs[0].score[SCORE_NUM_QUEST]==0&&pcs[0].quest==12345);
        ++count;
    }
    assert(count==1539);
    return count;
}

static void advancement_guards(void)
{
    const long invalid[]={LONG_MIN,-1,0,4,5,LONG_MAX};
    for(size_t i=0;i<sizeof(invalid)/sizeof(*invalid);i++){
        wolf_fixture(0,0);pcs[0].rank=invalid[i];
        actors[0].primal=actors[0].exp=actors[0].mkill=LONG_MAX;
        PC_DATA before=pcs[0];
        do_advance(actors,"");
        assert(!memcmp(&before,&pcs[0],sizeof before));
        assert(actors[0].primal==LONG_MAX&&actors[0].exp==LONG_MAX);
    }
    for(int bad=0;bad<4;bad++){
        wolf_fixture(CLASS_MAGE,0);
        actors[0].primal=actors[0].exp=actors[0].mkill=LONG_MAX;
        if(bad==0)pcs[0].hybrid.hybrid_id=0;
        if(bad==1)pcs[0].hybrid_present=2;
        if(bad==2)pcs[0].player_load_failed=1;
        if(bad==3)pcs[0].hybrid.primary_class=CLASS_MONK;
        PC_DATA before=pcs[0];do_advance(actors,"");
        assert(!memcmp(&before,&pcs[0],sizeof before));
        assert(actors[0].primal==LONG_MAX&&actors[0].exp==LONG_MAX);
    }
    wolf_fixture(0,0);
    actors[0].primal=100;actors[0].exp=1000000;actors[0].mkill=1000;
    do_advance(actors,"info");
    assert(pcs[0].rank==1&&actors[0].primal==100&&actors[0].exp==1000000);
    assert(strstr(outputs[0],"100 primal, 1000000 experience, 1000 total"));
    do_advance(actors,"extra");
    assert(pcs[0].rank==1&&actors[0].primal==100);
    do_advance(actors,NULL);
    assert(pcs[0].rank==2&&actors[0].primal==0&&actors[0].exp==0);
    do_advance(NULL,NULL);
    CHAR_DATA empty={0};empty.class=CLASS_WEREWOLF;do_advance(&empty,NULL);
}

static void execute(const char *text)
{
    char command[MAX_INPUT_LENGTH];
    snprintf(command,sizeof command,"%s",text);
    actors[0].wait=0;
    interpret(actors,command);
}

static void shifting(void)
{
    wolf_fixture(0,0);
    execute("shift werewolf");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN));
    assert(IS_VAMPAFF(actors,VAM_CLAWS)&&IS_VAMPAFF(actors,VAM_FANGS));
    assert(pcs[0].wolf==100&&actors[0].wait==12);
    long rage=pcs[0].wolf;
    execute("shift werewolf");assert(pcs[0].wolf==rage);
    execute("shift human"); /* No free Gaia gift and no forced reversion. */
    assert(pcs[0].wolf==rage&&strstr(outputs[0],"Children of Gaia 1"));
    pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=1;
    actors[0].position=POS_FIGHTING;
    execute("shift human");assert(pcs[0].wolf==rage);
    actors[0].position=POS_STANDING;
    execute("shift human");assert(pcs[0].wolf<rage);
    assert(!IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==0);
    assert(!IS_VAMPAFF(actors,VAM_CLAWS)&&!IS_VAMPAFF(actors,VAM_FANGS));
    wolf_fixture(0,0);execute("shift wolf");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==100);
    wolf_fixture(0,0);execute("shift crinos");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==100);
    wolf_fixture(0,0);pcs[0].auspice[AUSPICE_AHROUN]=4;
    execute("shift crinos");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==100);
    wolf_fixture(0,0);pcs[0].auspice[AUSPICE_AHROUN]=4;pcs[0].wolf=400;
    execute("shift crinos");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==100);
    wolf_fixture(0,0);pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=1;
    pcs[0].wolf=150;SET_BIT(actors[0].act,PLR_WOLFMAN);
    execute("shift homid");
    assert(!IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==0);
    wolf_fixture(0,0);pcs[0].wolf=-1;
    execute("shift werewolf");
    assert(pcs[0].wolf==-1&&!IS_SET(actors[0].act,PLR_WOLFMAN));
    wolf_fixture(0,0);pcs[0].wolf=LONG_MAX;
    execute("shift crinos");
    assert(pcs[0].wolf==LONG_MAX&&!IS_SET(actors[0].act,PLR_WOLFMAN));
    wolf_fixture(0,0);pcs[0].wolf=-1;SET_BIT(actors[0].act,PLR_WOLFMAN);
    execute("shift human");
    assert(pcs[0].wolf==-1&&IS_SET(actors[0].act,PLR_WOLFMAN));
    wolf_fixture(0,0);pcs[0].auspice[AUSPICE_AHROUN]=4;
    pcs[0].wolf=75;do_werewolf(actors,"");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==300);
    free_string(actors[0].morph);actors[0].morph=str_dup("");
    execute("shift");assert(pcs[0].wolf>=10);
    long before=pcs[0].wolf;
    execute("shift glabro");assert(pcs[0].wolf==before);
    execute("shift human extra");assert(pcs[0].wolf==before);
    execute("shift werewolf extra");assert(pcs[0].wolf==before);
    do_shift(NULL,NULL);do_rage(NULL,NULL);do_calm(NULL,NULL);
    do_werewolf(NULL,NULL);do_unwerewolf(NULL,NULL);
    wolf_fixture(CLASS_MAGE,1);execute("shift");
    assert(!IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==0);
    /* Primal Anger legitimately sets 400, beyond the usual 300/350 baseline.
     * Overflow guards must not invent a new gameplay cap. */
    wolf_fixture(0,0);pcs[0].wolf=400;
    pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=1;
    SET_BIT(actors[0].act,PLR_WOLFMAN);
    execute("shift");assert(pcs[0].wolf>=350&&pcs[0].wolf<=375);
}

static void reach_fixture(void)
{
    wolf_fixture(0,0);
    pcs[0].tribes[TRIBE_SILENT_STRIDERS]=5;
    pcs[0].flag=pcs[1].flag=FLAG_PK;
    actors[0].level=actors[1].level=LEV_AWA;
    actors[0].mana=4321;
    SET_BIT(actors[0].affected_by,AFF_SHADOWSIGHT);
    SET_BIT(actors[1].affected_by,AFF_SHADOWPLANE);
    REMOVE_BIT(room.room_flags,ROOM_SAFE);
}

static void reach_guards(void)
{
    do_reach(NULL,NULL);
    for(int mode=0;mode<31;mode++){
        reach_fixture();
        const char *command="beta";
        switch(mode){
        case 0:command="";break;
        case 1:command="beta extra";break;
        case 2:command="missing";break;
        case 3:SET_BIT(room.room_flags,ROOM_SAFE);break;
        case 4:pcs[0].tribes[TRIBE_SILENT_STRIDERS]=4;break;
        case 5:actors[0].class=CLASS_MAGE;break;
        case 6:SET_BIT(actors[0].act,ACT_IS_NPC);break;
        case 7:actors[0].pcdata=NULL;break;
        case 8:actors[0].in_room=NULL;break;
        case 9:actors[0].hit=0;break;
        case 10:actors[0].position=POS_SLEEPING;break;
        case 11:actors[0].wait=1;break;
        case 12:actors[0].fight_timer=1;break;
        case 13:actors[0].combat_pause=1;break;
        case 14:SET_BIT(actors[0].affected_by,AFF_SHADOWPLANE);break;
        case 15:pcs[0].flag=0;break;
        case 16:REMOVE_BIT(actors[1].affected_by,AFF_SHADOWPLANE);break;
        case 17:SET_BIT(actors[1].affected_by,AFF_DEEPUMBRA);break;
        case 18:actors[1].position=POS_FIGHTING;actors[1].fighting=&actors[2];break;
        case 19:actors[1].fight_timer=1;break;
        case 20:pcs[1].flag|=FLAG_AFK;break;
        case 21:actors[1].desc=NULL;break;
        case 22:actors[0].fighting=&actors[2];break;
        case 23:actors[0].level=LEV_SLE;break;
        case 24:SET_BIT(actors[0].affected_by,AFF_CHARM);
                actors[0].master=&actors[1];break;
        case 25:actors[1].hit=0;break;
        case 26:actors[1].position=POS_STUNNED;break;
        case 27:pcs[0].player_load_failed=1;break;
        case 28:command=NULL;break;
        case 29:command="alpha";break;
        case 30:REMOVE_BIT(actors[0].affected_by,AFF_SHADOWSIGHT);break;
        }
        long mana=actors[0].mana, wait=actors[0].wait;
        long caster_plane=actors[0].affected_by&AFF_SHADOWPLANE;
        long victim_affects=actors[1].affected_by;
        do_reach(actors,(char *)command);
        assert(actors[0].mana==mana&&actors[0].wait==wait);
        assert((actors[0].affected_by&AFF_SHADOWPLANE)==caster_plane);
        assert(actors[1].affected_by==victim_affects);
    }

    for(int mode=0;mode<6;mode++){
        reach_fixture();
        switch(mode){
        case 0:pcs[1].flag=0;break;
        case 1:actors[1].level=LEV_IMM;break;
        case 2:pcs[1].player_load_failed=1;break;
        case 3:SET_BIT(actors[1].itemaffect,ITEMA_PEACE);break;
        case 4:
            SET_BIT(actors[1].act,ACT_IS_NPC|ACT_NOKILL);
            actors[1].pcdata=NULL;actors[1].desc=NULL;
            break;
        case 5:SET_BIT(actors[1].affected_by,AFF_ETHEREAL);break;
        }
        long mana=actors[0].mana, wait=actors[0].wait;
        long caster_plane=actors[0].affected_by&AFF_SHADOWPLANE;
        long victim_affects=actors[1].affected_by;
        do_reach(actors,"beta");
        assert(actors[0].mana==mana&&actors[0].wait==wait);
        assert((actors[0].affected_by&AFF_SHADOWPLANE)==caster_plane);
        assert(actors[1].affected_by==victim_affects);
    }
}

static void reach(void)
{
    reach_guards();
    reach_fixture();
    long mana=actors[0].mana;
    long other_affects=actors[1].affected_by&~AFF_SHADOWPLANE;
    PC_DATA before=pcs[1];
    execute("reach beta");
    assert(actors[0].mana==mana&&actors[0].wait==0);
    assert(!IS_AFFECTED(&actors[0],AFF_SHADOWPLANE));
    assert(!IS_AFFECTED(&actors[1],AFF_SHADOWPLANE));
    assert((actors[1].affected_by&~AFF_SHADOWPLANE)==other_affects);
    assert(actors[1].in_room==&room&&!memcmp(&pcs[1],&before,sizeof before));
    assert(strstr(outputs[0],"out of the Umbra"));
    assert(strstr(outputs[1],"pulls you out of the Umbra"));

    reach_fixture();
    SET_BIT(actors[1].act,ACT_IS_NPC);
    actors[1].pcdata=NULL;
    actors[1].desc=NULL;
    execute("reach beta");
    assert(!IS_AFFECTED(&actors[1],AFF_SHADOWPLANE));
    assert(actors[0].mana==4321&&actors[0].wait==0);
}

static void menus(void)
{
    for(int rank=1;rank<=5;rank++){
        wolf_fixture(0,0);
        pcs[0].tribes[TRIBE_BLACK_FURIES]=rank;
        pcs[0].tribes[TRIBE_BONE_GNAWERS]=rank;
        do_wwpowers(actors,"");
        assert((strstr(outputs[0],"Aeolus")!=NULL)==(rank>=2));
        assert((strstr(outputs[0],"Bodywrack")!=NULL)==(rank>=5));
        assert((strstr(outputs[0],"Friend")!=NULL)==(rank>=3));
        assert((strstr(outputs[0],"Infest")!=NULL)==(rank>=4));
    }
    wolf_fixture(0,0);pcs[0].tribes[TRIBE_BLACK_FURIES]=1;
    pcs[0].tribes[TRIBE_BONE_GNAWERS]=1;
    for(int i=0;i<5;i++)pcs[0].auspice[i]=5;
    do_wwpowers(actors,"");
    assert(!strstr(outputs[0],"Aeolus")&&!strstr(outputs[0],"Bodywrack"));
    assert(!strstr(outputs[0],"Friend")&&!strstr(outputs[0],"Infest"));
    wolf_fixture(0,0);pcs[0].breed[BREED_HOMID]=5;
    pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=4;
    do_wwpowers(actors,"");
    assert(strstr(outputs[0],"Spiritward")&&strstr(outputs[0],"Spiritguardian"));
    assert(strstr(outputs[0],"Viewing gifts is free"));
    execute("gifts");
    assert(strstr(outputs[0],"Your selected breed/auspice/tribe gifts cost 10"));
    assert(strstr(outputs[0],"other gifts cost 15"));
    assert(strstr(outputs[0],"Each purchase trains one rank immediately"));
}

static void quicksand_fixture(void)
{
    wolf_fixture(0,0);
    pcs[0].tribes[TRIBE_RED_TALONS]=4;
    for(int n=0;n<3;n++)pcs[n].flag=FLAG_PK;
    actors[0].mana=1;
    REMOVE_BIT(room.room_flags,ROOM_SAFE);
}

static void quicksand_guards(void)
{
    do_quicksand(NULL,NULL);
    for(int mode=0;mode<26;mode++){
        quicksand_fixture();
        const char *command="beta";
        switch(mode){
        case 0:actors[0].mana=0;break;
        case 1:command="alpha";break;
        case 2:command="missing";break;
        case 3:command="";break;
        case 4:command="beta extra";break;
        case 5:SET_BIT(room.room_flags,ROOM_SAFE);break;
        case 6:pcs[0].tribes[TRIBE_RED_TALONS]=3;break;
        case 7:actors[0].class=CLASS_MAGE;break;
        case 8:SET_BIT(actors[0].act,ACT_IS_NPC);break;
        case 9:actors[0].pcdata=NULL;break;
        case 10:actors[0].in_room=NULL;break;
        case 11:actors[0].position=POS_SLEEPING;break;
        case 12:actors[0].wait=1;break;
        case 13:actors[0].combat_pause=1;break;
        case 14:actors[1].desc=NULL;break;
        case 15:pcs[1].flag|=FLAG_AFK;break;
        case 16:SET_BIT(actors[1].affected_by,AFF_ETHEREAL);break;
        case 17:actors[1].level=LEV_ST;break;
        case 18:pcs[1].flag=0;break;
        case 19:actors[1].pcdata=NULL;break;
        case 20:actors[1].hit=0;break;
        case 21:actors[1].position=POS_STUNNED;break;
        case 22:SET_BIT(actors[1].act,ACT_IS_NPC|ACT_NOKILL);break;
        case 23:SET_BIT(actors[1].itemaffect,ITEMA_PEACE);break;
        case 24:pcs[0].player_load_failed=1;break;
        case 25:SET_BIT(actors[0].affected_by,AFF_CHARM);
                actors[0].master=&actors[1];break;
        }
        CHAR_DATA before[3];PC_DATA pc_before[3];
        memcpy(before,actors,sizeof before);memcpy(pc_before,pcs,sizeof pc_before);
        do_quicksand(actors,(char *)command);
        assert(actors[0].mana==before[0].mana&&actors[0].wait==before[0].wait);
        assert(actors[0].quicksand_timer==0&&actors[1].quicksand_timer==0);
        assert(actors[0].fighting==before[0].fighting);
        assert(!memcmp(pc_before,pcs,sizeof pc_before));
    }
}

static void quicksand(void)
{
    quicksand_guards();
    quicksand_fixture();
    ROOM_INDEX_DATA destination={0};
    EXIT_DATA exit={0};
    destination.area=&area;destination.name="Snare destination";
    destination.description="A private destination.\n\r";destination.light=1;
    exit.to_room=&destination;room.exit[DIR_NORTH]=&exit;

    execute("quicksand beta");
    assert(actors[0].mana==0&&actors[1].quicksand_timer==20);
    assert(actors[1].class==CLASS_NONE&&actors[1].morph[0]=='\0');
    assert(actors[0].wait==12);

    actors[0].mana=1;
    execute("quicksand beta");
    assert(actors[0].mana==1&&actors[0].wait==0&&actors[1].quicksand_timer==20);
    move_char(&actors[1],DIR_NORTH);
    assert(actors[1].in_room==&room&&strstr(outputs[1],"quicksand holds you fast"));

    actors[1].desc=NULL;
    for(int pulse=0;pulse<19;pulse++)combat_status_update();
    assert(actors[1].quicksand_timer==1);
    actors[1].desc=&descs[1];
    combat_status_update();
    assert(actors[1].quicksand_timer==0);
    move_char(&actors[1],DIR_NORTH);
    assert(actors[1].in_room==&destination);
    char_from_room(&actors[1]);char_to_room(&actors[1],&room);

    actors[0].mana=1;
    execute("quicksand beta");
    assert(actors[1].quicksand_timer==20);
    char_from_room(&actors[1]);
    assert(actors[1].quicksand_timer==0);
    char_to_room(&actors[1],&room);

    quicksand_fixture();
    SET_BIT(actors[1].act,ACT_IS_NPC);actors[1].pcdata=NULL;
    actors[1].desc=NULL;
    do_quicksand(actors,"beta");
    assert(actors[1].quicksand_timer==20&&actors[0].mana==0);
    for(int pulse=0;pulse<20;pulse++)combat_status_update();
    assert(actors[1].quicksand_timer==0);

    quicksand_fixture();
    pcs[2].tribes[TRIBE_RED_TALONS]=4;actors[2].class=CLASS_WEREWOLF;
    do_quicksand(actors,"beta");combat_status_update();
    long mana=actors[2].mana;
    do_quicksand(&actors[2],"beta");
    assert(actors[1].quicksand_timer==19&&actors[2].mana==mana&&actors[2].wait==0);
}

static void lycaon_fixture(void)
{
    wolf_fixture(0,0);
    pcs[0].tribes[TRIBE_RED_TALONS]=5;
    for(int n=0;n<3;n++)pcs[n].flag=FLAG_PK;
    actors[0].mana=2000;
    REMOVE_BIT(room.room_flags,ROOM_SAFE);
}

static void lycaon_guards(void)
{
    for(int mode=0;mode<27;mode++){
        lycaon_fixture();
        const char *command="beta";
        char oversized[MAX_INPUT_LENGTH+16];
        memset(oversized,'x',sizeof oversized-1);oversized[sizeof oversized-1]='\0';
        switch(mode){
        case 0:actors[0].mana=999;break;
        case 1:command="alpha";break;
        case 2:command="missing";break;
        case 3:command="";break;
        case 4:command="beta extra";break;
        case 5:SET_BIT(room.room_flags,ROOM_SAFE);break;
        case 6:pcs[0].tribes[TRIBE_RED_TALONS]=4;break;
        case 7:actors[0].class=CLASS_MAGE;break;
        case 8:SET_BIT(actors[0].act,ACT_IS_NPC);break;
        case 9:actors[0].pcdata=NULL;break;
        case 10:actors[0].in_room=NULL;break;
        case 11:actors[0].position=POS_SLEEPING;break;
        case 12:actors[0].wait=1;break;
        case 13:actors[0].combat_pause=1;break;
        case 14:actors[1].desc=NULL;break;
        case 15:pcs[1].flag|=FLAG_AFK;break;
        case 16:SET_BIT(actors[1].affected_by,AFF_ETHEREAL);break;
        case 17:actors[1].level=LEV_ST;break;
        case 18:pcs[1].flag=0;break;
        case 19:SET_BIT(actors[1].act,ACT_IS_NPC);actors[1].pcdata=NULL;break;
        case 20:
            SET_BIT(actors[1].affected_by,AFF_POLYMORPH);
            actors[1].morph=str_dup("A masked person");break;
        case 21:
            actors[1].lycaon_timer=12;
            actors[1].morph=str_dup("A wolf");
            SET_BIT(actors[1].affected_by,AFF_POLYMORPH);break;
        case 22:actors[1].morph=str_dup("a shadow");break;
        case 23:SET_BIT(actors[1].act,PLR_WOLFMAN);break;
        case 24:pcs[0].flag=0;break;
        case 25:pcs[1].player_load_failed=1;break;
        case 26:command=oversized;break;
        }
        CHAR_DATA before[3];PC_DATA pc_before[3];
        memcpy(before,actors,sizeof before);memcpy(pc_before,pcs,sizeof pc_before);
        do_lycaon(actors,(char *)command);
        assert(actors[0].mana==before[0].mana&&actors[0].wait==before[0].wait);
        assert(actors[1].lycaon_timer==before[1].lycaon_timer);
        assert(actors[1].affected_by==before[1].affected_by);
        assert(actors[1].morph==before[1].morph);
        assert(!memcmp(pc_before,pcs,sizeof pc_before));
    }
}

static void lycaon(void)
{
    lycaon_guards();
    lycaon_fixture();
    ROOM_INDEX_DATA destination={0};
    destination.area=&area;destination.name="Lycaon destination";
    destination.description="A private destination.\n\r";destination.light=1;
    PC_DATA before_target=pcs[1];
    long polyaff=actors[1].polyaff, class_id=actors[1].class;
    execute("lycaon beta");
    assert(actors[0].mana==1000&&actors[0].wait==12&&actors[1].lycaon_timer==30);
    assert(!strcmp(actors[1].morph,"A wolf"));
    assert(IS_AFFECTED(&actors[1],AFF_POLYMORPH));
    assert(actors[1].class==class_id&&actors[1].polyaff==polyaff);
    assert(!memcmp(&pcs[1],&before_target,sizeof before_target));
    assert(!IS_SET(actors[1].act,PLR_WOLFMAN));

    actors[0].mana=2000;
    execute("lycaon beta");
    assert(actors[0].mana==2000&&actors[0].wait==0&&actors[1].lycaon_timer==30);

    char_from_room(&actors[1]);
    assert(actors[1].lycaon_timer==30&&!strcmp(actors[1].morph,"A wolf"));
    char_to_room(&actors[1],&destination);
    for(int pulse=0;pulse<29;pulse++)combat_status_update();
    assert(actors[1].lycaon_timer==1&&IS_AFFECTED(&actors[1],AFF_POLYMORPH));
    char_from_room(&actors[1]);
    assert(actors[1].lycaon_timer==1&&!strcmp(actors[1].morph,"A wolf"));
    char_to_room(&actors[1],&room);
    combat_status_update();
    assert(actors[1].lycaon_timer==0&&actors[1].morph[0]=='\0');
    assert(!IS_AFFECTED(&actors[1],AFF_POLYMORPH));
    assert(strstr(outputs[1],"natural appearance returns"));
}

static void lycaon_persistence(void)
{
    char cwd[4096],root[]="/tmp/anathema-lycaon-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700));
    assert(!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    lycaon_fixture();
    DESCRIPTOR_DATA d={0};
    assert(!load_char_obj(&d,"Lycaonfixture"));
    CHAR_DATA *saved=d.character;
    saved->level=LEV_AWA;saved->class=CLASS_NONE;
    saved->hit=saved->max_hit=50000;saved->mana=saved->max_mana=50000;
    saved->pcdata->flag=FLAG_PK;saved->in_room=&room;saved->desc=&d;
    saved->next_in_room=room.people;room.people=saved;
    do_lycaon(actors,"Lycaonfixture");
    assert(saved->lycaon_timer==30&&IS_AFFECTED(saved,AFF_POLYMORPH));
    assert(save_char_obj_checked(saved));
    assert(saved->lycaon_timer==30&&IS_AFFECTED(saved,AFF_POLYMORPH));
    FILE *fp=fopen("../player/Lycaonfixture","rb");assert(fp);
    char line[2048];bool morph_empty=FALSE,affected_clean=FALSE;
    while(fgets(line,sizeof line,fp)){
        if(!strncmp(line,"Morph",5))morph_empty=!strcmp(line,"Morph        ~\n");
        if(!strncmp(line,"AffectedBy",10)){
            long bits=-1;assert(sscanf(line,"AffectedBy %li",&bits)==1);
            affected_clean=!IS_SET(bits,AFF_POLYMORPH);
        }
        assert(strstr(line,"lycaon_timer")==NULL);
    }
    assert(!fclose(fp)&&morph_empty&&affected_clean);
    room.people=saved->next_in_room;
    saved->next_in_room=NULL;saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    for(int brief=0;brief<2;brief++){
        memset(&d,0,sizeof d);
        assert(brief?load_char_short(&d,"Lycaonfixture"):load_char_obj(&d,"Lycaonfixture"));
        saved=d.character;
        assert(!saved->pcdata->player_load_failed&&saved->lycaon_timer==0);
        assert(saved->class==CLASS_NONE&&saved->morph[0]=='\0');
        assert(!IS_AFFECTED(saved,AFF_POLYMORPH));
        saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    }
    assert(!unlink("../player/Lycaonfixture"));(void)unlink("../backup/Lycaonfixture");
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("run")&&!rmdir("player")&&!rmdir("backup"));
    assert(!chdir(cwd)&&!rmdir(root));
}

static void screamgaia_fixture(void)
{
    wolf_fixture(0,0);
    pcs[0].tribes[TRIBE_GET_OF_FENRIS]=4;
    pcs[0].flag=pcs[1].flag=FLAG_PK;
    pcs[2].flag=0;
    actors[0].mana=STUN_FIST_COST;
    REMOVE_BIT(room.room_flags,ROOM_SAFE);
}

static void screamgaia(void)
{
    screamgaia_fixture();
    actors[2].leader=&actors[0];
    SET_BIT(actors[2].act,ACT_IS_NPC);
    actors[2].pcdata=NULL;
    actors[2].desc=NULL;
    actors[0].hit=actors[1].hit=actors[2].hit=50000;
    actors[1].position=actors[2].position=POS_FIGHTING;
    long target_hit=actors[1].hit, mobile_hit=actors[2].hit;
    execute("screamgaia");
    assert(actors[0].mana==0&&actors[0].combat_pause==STUN_FIST_RECOVERY);
    assert(actors[1].combat_pause==STUN_FIST_DURATION);
    assert(actors[1].stun_immunity==STUN_FIST_IMMUNITY);
    assert(actors[2].combat_pause==0&&actors[2].stun_immunity==0);
    assert(actors[1].hit==target_hit&&actors[2].hit==mobile_hit);
    assert(actors[1].position==POS_FIGHTING&&actors[1].in_room==&room);
    assert(strstr(outputs[0],"stunning Scream of Gaia"));
    assert(strstr(outputs[1],"interrupting your actions"));
    assert(!strstr(outputs[2],"stagger"));

    actors[0].mana=STUN_FIST_COST;
    actors[0].combat_pause=0;
    execute("screamgaia");
    assert(actors[0].mana==STUN_FIST_COST);
    assert(actors[1].combat_pause==STUN_FIST_DURATION);

    for(int pulse=0;pulse<STUN_FIST_IMMUNITY;pulse++)
        combat_status_update();
    assert(actors[1].combat_pause==0&&actors[1].stun_immunity==0);
    actors[0].combat_pause=0;
    execute("screamgaia");
    assert(actors[0].mana==0&&actors[1].combat_pause==STUN_FIST_DURATION);

    screamgaia_fixture();
    SET_BIT(actors[1].act,ACT_IS_NPC);
    actors[1].pcdata=NULL;
    actors[1].desc=NULL;
    execute("screamgaia");
    assert(actors[0].mana==0&&actors[1].combat_pause==STUN_FIST_DURATION);
}

static void screamgaia_rejections(void)
{
    for(int mode=0;mode<10;mode++){
        screamgaia_fixture();
        switch(mode){
        case 0: actors[0].mana=STUN_FIST_COST-1;break;
        case 1: SET_BIT(room.room_flags,ROOM_SAFE);break;
        case 2: pcs[0].tribes[TRIBE_GET_OF_FENRIS]=3;break;
        case 3: actors[0].class=CLASS_MAGE;break;
        case 4: SET_BIT(actors[1].act,ACT_IS_NPC);actors[1].pcdata=NULL;actors[1].desc=NULL;SET_BIT(actors[1].act,ACT_NOKILL);break;
        case 5: pcs[1].flag=0;break;
        case 6: actors[1].leader=&actors[0];actors[2].leader=&actors[0];break;
        case 7: actors[1].combat_pause=1;break;
        case 8: actors[1].stun_immunity=1;break;
        case 9: actors[1].position=POS_SLEEPING;break;
        }
        do_screamgaia(actors,"ignored");
        assert(actors[0].mana==STUN_FIST_COST-(mode==0?1:0));
        assert(actors[0].combat_pause==0);
        assert(actors[1].combat_pause==(mode==7?1:0));
        assert(actors[1].stun_immunity==(mode==8?1:0));
    }
}

static void quicksand_persistence(void)
{
    char cwd[4096],root[]="/tmp/anathema-quicksand-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700));
    assert(!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    quicksand_fixture();
    DESCRIPTOR_DATA d={0};
    assert(!load_char_obj(&d,"Snarefixture"));
    CHAR_DATA *saved=d.character;
    saved->level=LEV_AWA;saved->class=CLASS_NONE;
    saved->hit=saved->max_hit=50000;saved->mana=saved->max_mana=50000;
    saved->pcdata->flag=FLAG_PK;saved->in_room=&room;saved->desc=&d;
    saved->next_in_room=room.people;room.people=saved;
    do_quicksand(actors,"Snarefixture");
    assert(saved->quicksand_timer==20&&save_char_obj_checked(saved));
    assert(saved->quicksand_timer==20);
    room.people=saved->next_in_room;
    saved->next_in_room=NULL;saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    for(int brief=0;brief<2;brief++){
        memset(&d,0,sizeof d);
        assert(brief?load_char_short(&d,"Snarefixture"):load_char_obj(&d,"Snarefixture"));
        saved=d.character;
        assert(!saved->pcdata->player_load_failed&&saved->quicksand_timer==0);
        assert(saved->class==CLASS_NONE&&saved->morph[0]=='\0');
        saved->desc=NULL;saved->in_room=NULL;free_char(saved);
    }
    assert(!unlink("../player/Snarefixture"));(void)unlink("../backup/Snarefixture");
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("run")&&!rmdir("player")&&!rmdir("backup"));
    assert(!chdir(cwd)&&!rmdir(root));
}

/* Deliberately malformed internal calls reproduce old failures separately;
 * actual player command regression tests above always use the interpreter. */
static void safety_case(int mode)
{
    wolf_fixture(0,0);
    if(mode==0){
        char huge[MAX_STRING_LENGTH*4];memset(huge,'X',sizeof huge-1);huge[sizeof huge-1]=0;
        PC_DATA before=pcs[0];do_tribegifts(actors,huge);
        assert(!memcmp(&before,&pcs[0],sizeof before));
    }else if(mode==1){
        pcs[0].breed[BREED_HOMID]=LONG_MIN;actors[0].primal=1000;
        do_gifts(actors,"homid");
        assert(pcs[0].breed[BREED_HOMID]==LONG_MIN&&actors[0].primal==1000);
    }else if(mode==2){
        pcs[0].wolf=LONG_MAX;do_rage(actors,"");assert(pcs[0].wolf==LONG_MAX);
    }else if(mode==3){
        char huge[MAX_STRING_LENGTH*4];memset(huge,'X',sizeof huge-1);huge[sizeof huge-1]=0;
        actors[0].name=huge;do_werewolf(actors,"");
        assert(IS_SET(actors[0].act,PLR_WOLFMAN));
        assert(strlen(actors[0].morph)<MAX_INPUT_LENGTH);
        free_string(actors[0].morph);actors[0].morph=str_dup("");
    }else if(mode==4){
        pcs[0].wolf=LONG_MIN;pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=1;
        SET_BIT(actors[0].act,PLR_WOLFMAN);do_calm(actors,"");
        assert(pcs[0].wolf==LONG_MIN);
    }else if(mode==5){
        actors[0].pcdata=NULL;do_advance(actors,"");actors[0].pcdata=&pcs[0];
    }else assert(0);
}

int main(int argc,char **argv)
{
    if(argc==3&&!strcmp(argv[1],"--safety")){
        safety_case(atoi(argv[2]));return 0;
    }
    unsigned prices=advancement_grid();
    advancement_guards();shifting();reach();menus();quicksand();lycaon();
    screamgaia_rejections();screamgaia();
    for(int i=0;i<6;i++)safety_case(i);
    quicksand_persistence();lycaon_persistence();
    puts("Werewolf commands:1539 advancement boundary cases across19 native/"
         "ordered-hybrid contexts; doubled primal/XP/cumulative kills, zero quest "
         "requirements, real shift/Rage/Calm and menu checks; guarded rank-5 Reach "
         "with safe cross-plane target handling and no cost/recovery;6 safety cases; "
         "Quicksand 26 rejection guards, movement, 20-pulse NPC/link-dead expiry, "
         "non-refresh, relocation cleanup and full/short reloads; Lycaon 27 rejection "
         "guards, one-thousand-mana cost, appearance-only 30-pulse expiry and save "
         "masking; Scream of Gaia ten rejection cases, room target filtering, "
         "one-pulse stun and immunity recovery passed.");
    return prices==1539?0:1;
}
