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
    assert(pcs[0].wolf>=10&&pcs[0].wolf<=20);
    assert(!IS_SET(actors[0].act,PLR_WOLFMAN));
    assert(strstr(outputs[0],"Rage:"));
    for(int n=0;n<10&&!IS_SET(actors[0].act,PLR_WOLFMAN);n++)
        execute("shift werewolf");
    assert(IS_SET(actors[0].act,PLR_WOLFMAN));
    assert(IS_VAMPAFF(actors,VAM_CLAWS)&&IS_VAMPAFF(actors,VAM_FANGS));
    assert(pcs[0].wolf>=100&&pcs[0].wolf<=144);
    long rage=pcs[0].wolf;
    execute("shift werewolf");assert(pcs[0].wolf==rage);
    execute("shift human"); /* No free Gaia gift and no forced reversion. */
    assert(pcs[0].wolf==rage&&strstr(outputs[0],"Children of Gaia 1"));
    pcs[0].tribes[TRIBE_CHILDREN_OF_GAIA]=1;
    actors[0].position=POS_FIGHTING;
    execute("shift human");assert(pcs[0].wolf==rage);
    actors[0].position=POS_STANDING;
    execute("shift human");assert(pcs[0].wolf<rage);
    for(int n=0;n<12&&IS_SET(actors[0].act,PLR_WOLFMAN);n++)
        execute("shift human");
    assert(!IS_SET(actors[0].act,PLR_WOLFMAN)&&pcs[0].wolf==0);
    assert(!IS_VAMPAFF(actors,VAM_CLAWS)&&!IS_VAMPAFF(actors,VAM_FANGS));
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
    execute("shift human");assert(pcs[0].wolf>=350&&pcs[0].wolf<=375);
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
    advancement_guards();shifting();menus();
    for(int i=0;i<6;i++)safety_case(i);
    puts("Werewolf commands:1539 advancement boundary cases across19 native/"
         "ordered-hybrid contexts; doubled primal/XP/cumulative kills, zero quest "
         "requirements, real shift/Rage/Calm and menu checks;6 safety cases passed.");
    return prices==1539?0:1;
}
