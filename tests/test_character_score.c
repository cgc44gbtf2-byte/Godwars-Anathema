/* Whole-engine character score, with stable time and real metric getters. */
#define main identify_fixture_main
#include "test_identify.c"
#undef main
void obj_score(CHAR_DATA *ch,OBJ_DATA *object);
static void score_fixture(void)
{
    fixture();current_time=43200;actor.logon=36000;actor.played=7200;
    actor.createtime="Mon Jan  1 00:00:00 2024";pc.title=" the traveller";
    actor.hit=actor.max_hit=50000;actor.mana=actor.max_mana=50000;
    for(int n=0;n<5;n++)actor.stats[n]=5;
}
static void score_native(int dump)
{
    static const long classes[]={0,CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,
        CLASS_DROW,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_DRAGON,CLASS_NINJA,CLASS_MONK,CLASS_ELEM};
    static const long beasts[]={0,5,10,15,20,30,40,60,75,90,99,100};
    static const long status[]={0,4,5,9,10,14,15,19,20,24,25,0,0,0,0,0};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int c=0;c<11;c++)for(int rank=0;rank<16;rank++)
    for(int state=0;state<12;state++)for(int ansi=0;ansi<2;ansi++){
        score_fixture();actor.class=classes[c];actor.status=status[rank];
        if(rank==11)actor.level=LEV_BUI;if(rank==12)actor.level=LEV_IMM;
        if(rank==13)actor.level=LEV_IMP;if(rank==14)actor.level=LEV_ST;
        if(rank==15)actor.level=LEV_SLE;
        if(ansi)SET_BIT(actor.act,PLR_ANSI);
        actor.beast=beasts[state];actor.pkill=state%3==2?100:state%3;
        actor.pdeath=state%3==2?10:state%3;actor.mkill=actor.pkill;actor.mdeath=actor.pdeath;
        if(state&1){pc.wolf=10;SET_BIT(actor.act,PLR_WOLFMAN);
            SET_BIT(actor.affected_by,AFF_HIDE);SET_BIT(actor.more,MORE_FAVOUR);}
        if(state&2){actor.xtra=XTRA_BETTER_STR|XTRA_BETTER_DEX|XTRA_BETTER_CON|
                XTRA_BETTER_INT|XTRA_BETTER_WIS;actor.better_body_timer=7;pc.atm=20;}
        pc.ninja[NINJA_KI]=100;pc.ninja[NINJA_HARAKIRI]=3;
        pc.classpoints[CP_CURRENT]=600;pc.classpoints[CP_TOTAL]=1200;
        actor.blood[0]=100;actor.blood[1]=200;actor.fight_timer=state;
        actor.paradox=3;actor.quint[1]=12;actor.arete=5;
        CHAR_DATA before=actor;PC_DATA saved=pc;
        char empty[]="";do_score(&actor,empty);
        assert(!memcmp(&before,&actor,sizeof actor)&&!memcmp(&saved,&pc,sizeof pc));
        uint64_t row=hash(UINT64_C(1469598103934665603),output,desc.outtop);
        total=hash(total,&row,sizeof row);rows++;
        if(dump)printf("%d,%d,%d,%d,%" PRIu64 "\n",c,rank,state,ansi,row);
    }
    assert(rows==4224);fprintf(stderr,"CHARACTER_SCORE_NATIVE_HASH=%" PRIu64 "\n",total);
    if(!dump)assert(total==UINT64_C(4978974497349168729));
}
static void score_edge(int n)
{
    score_fixture();memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
    char empty[]="";
    if(n==0)actor.name=large;
    if(n==1)pc.title=large;
    if(n==2)actor.createtime=large;
    if(n==3){do_score(NULL,empty);return;}
    if(n==4)actor.pcdata=NULL;
    if(n==5){actor.name=NULL;pc.title=NULL;actor.createtime=NULL;}
    if(n==6)SET_BIT(actor.act,ACT_IS_NPC);
    if(n==7)SET_BIT(actor.extra,EXTRA_OSWITCH);
    if(n==8){actor.pkill=LONG_MAX;actor.pdeath=0;}
    if(n==9){actor.pkill=LONG_MAX;actor.pdeath=1;}
    if(n==10){actor.pkill=1;actor.pdeath=-1;}
    if(n==11)actor.played=LONG_MAX;
    if(n==12){current_time=LONG_MAX;actor.logon=LONG_MIN;}
    do_score(&actor,empty);
    assert(desc.outtop<(long)sizeof output);
    if(n==4||n==6||n==7)assert(desc.outtop==0);
    if(n==5)assert(strstr(output,"Someone")&&strstr(output,"Character Created: unknown"));
    if(n>=8&&n<=10)assert(strstr(output,"PK Score: unavailable"));
    if(n>=11)assert(strstr(output,"Playing time unavailable"));
}
static int fits_long(__int128 value)
{return value>=LONG_MIN&&value<=LONG_MAX;}
static void score_arithmetic(void)
{
    static const long values[]={LONG_MIN,LONG_MIN+1,-1000000000,-1000,-1,0,1,
        2,3,10,1000,1000000000,LONG_MAX-1,LONG_MAX};
    unsigned pk_cases=0,hour_cases=0;
    for(unsigned a=0;a<14;a++)for(unsigned b=0;b<14;b++){
        score_fixture();actor.pkill=values[a];actor.pdeath=values[b];
        __int128 numerator=(__int128)100*actor.pkill,result=numerator;
        int valid=fits_long(numerator);
        if(actor.pdeath!=0){
            __int128 sum=(__int128)actor.pkill+actor.pdeath;
            valid=valid&&fits_long(sum)&&sum!=0;
            if(valid){
                __int128 denominator=sum*sum; numerator*=actor.pkill;
                valid=fits_long(denominator)&&fits_long(numerator);
                if(valid){result=(__int128)actor.pkill*(numerator/denominator);valid=fits_long(result);}
            }
        }
        CHAR_DATA before=actor;PC_DATA saved=pc;
        char empty[]="",wanted[100];do_score(&actor,empty);
        if(valid)snprintf(wanted,sizeof wanted,"PK Score: %li\n\r",(long)result);
        else strcpy(wanted,"PK Score: unavailable\n\r");
        assert(strstr(output,wanted));
        assert(!memcmp(&actor,&before,sizeof actor)&&!memcmp(&pc,&saved,sizeof pc));pk_cases++;
    }
    for(unsigned a=0;a<14;a++)for(unsigned b=0;b<14;b++)for(unsigned c=0;c<14;c++){
        score_fixture();actor.played=values[a];current_time=values[b];actor.logon=values[c];
        __int128 delta=(__int128)current_time-actor.logon;
        __int128 sum=(__int128)actor.played+delta;
        char empty[]="",wanted[100];do_score(&actor,empty);
        if(fits_long(delta)&&fits_long(sum))
            snprintf(wanted,sizeof wanted,"You have been playing for %li hours.",(long)sum/3600);
        else strcpy(wanted,"Playing time unavailable");
        assert(strstr(output,wanted));hour_cases++;
    }
    assert(pk_cases==196&&hour_cases==2744);
}
static void score_text_and_delegation(void)
{
    for(int ansi=0;ansi<2;ansi++)for(int field=0;field<3;field++)
    for(int hashes=1;hashes<=3;hashes++){
        score_fixture();if(ansi)SET_BIT(actor.act,PLR_ANSI);
        memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
        size_t prefix=field==0?0:field==1?strlen(actor.name):strlen("Character Created: ");
        memset(large+MSL-3-prefix-hashes,'#',(size_t)hashes);
        if(field==0)actor.name=large;if(field==1)pc.title=large;
        if(field==2)actor.createtime=large;
        char empty[]="";do_score(&actor,empty);
        assert(desc.outtop>0&&desc.outtop<MSL*4&&strstr(output,"Hit Points:"));
        assert(strstr(output,"Status:"));
    }
    for(int head=0;head<2;head++){
        score_fixture();pc.chobj=&obj;
        if(head)SET_BIT(actor.more,MORE_HEAD);else SET_BIT(actor.extra,EXTRA_OSWITCH);
        char empty[]="";do_score(&actor,empty);
        char expected[MSL*4];assert(desc.outtop<(long)sizeof expected);
        long length=desc.outtop;memcpy(expected,output,(size_t)length);
        desc.outtop=0;output[0]=0;obj_score(&actor,&obj);
        assert(length==desc.outtop&&!memcmp(expected,output,(size_t)length));
    }
}
int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){score_native(1);return 0;}
    if(argc>2&&!strcmp(argv[1],"--boundary")){score_edge(atoi(argv[2]));return 0;}
    score_native(0);for(int n=0;n<13;n++)score_edge(n);
    score_arithmetic();score_text_and_delegation();
    puts("Character score:4224 native snapshots,196 PK/2744 time cases,13 boundaries,18 colour cases and object delegation passed.");
    return 0;
}
