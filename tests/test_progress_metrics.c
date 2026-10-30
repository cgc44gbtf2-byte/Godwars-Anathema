/* Real shared helpers/callers; leaderboard writes go only to a private temp dir. */
#define main identify_fixture_main
#include "test_identify.c"
#undef main
#include <unistd.h>
#include <sys/stat.h>
#include "progress_metrics.h"
extern char *string_space, *top_string;
static char old_cwd[PATH_MAX],sandbox[PATH_MAX];
static void sandbox_begin(void)
{
    assert(getcwd(old_cwd,sizeof old_cwd));char pattern[]="/tmp/anathema-metrics-XXXXXX";
    char *path=mkdtemp(pattern);assert(path);strcpy(sandbox,path);assert(chdir(path)==0);
    assert(mkdir("area",0700)==0&&mkdir("run",0700)==0&&chdir("run")==0);
    string_space=calloc(1,2*1024*1024);assert(string_space);top_string=string_space;
}
static void sandbox_end(void)
{
    unlink("../area/leader.txt");assert(chdir("..")==0);
    assert(rmdir("area")==0&&rmdir("run")==0&&chdir(old_cwd)==0&&rmdir(sandbox)==0);
    free(string_space);string_space=top_string=NULL;
}
static void board_names(int create)
{
    char **names[]={&leader_board.bestpk_name,&leader_board.worstpk_name,
        &leader_board.pk_name,&leader_board.pd_name,&leader_board.mk_name,
        &leader_board.md_name,&leader_board.tt_name,&leader_board.qc_name};
    for(unsigned n=0;n<8;n++){if(*names[n])free_string(*names[n]);*names[n]=create?str_dup("Noone"):NULL;}
}
static void metrics_fixture(void)
{
    fixture();current_time=43200;actor.logon=current_time;actor.played=30*3600;
    actor.hit=actor.max_hit=50000;actor.mana=actor.max_mana=50000;
    actor.long_descr="A traveller waits here.\n\r";
    for(int n=0;n<5;n++)actor.stats[n]=5;
    pc.covenant="TestCovenant";pc.cov_time="Epoch";pc.vleader="owner";
    board_names(0);memset(&leader_board,0,sizeof leader_board);board_names(1);
    leader_board.worstpk_number=1000000;
    leader_board.pk_number=leader_board.pd_number=leader_board.mk_number=
        leader_board.md_number=leader_board.qc_number=1000000;
    unlink("../area/leader.txt");
}
static int apply_login_age(void)
{
    return grant_age_pk(&actor);
}
static void metrics_native(int dump)
{
    static const long counts[]={0,1,2,3,10,100,1000,10000};
    static const long hours[]={0,29,30,31,50};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int a=0;a<8;a++)for(int b=0;b<8;b++)for(int h=0;h<5;h++)
    for(int caller=0;caller<5;caller++)for(int ansi=0;ansi<2;ansi++){
        metrics_fixture();actor.pkill=counts[a];actor.pdeath=counts[b];actor.played=hours[h]*3600;
        if(ansi)SET_BIT(actor.act,PLR_ANSI);
        char command[40]="";
        if(caller==0)snprintf(output,sizeof output,"%li,%li",get_hours(&actor),get_ratio(&actor));
        if(caller==1){strcpy(command,"examiner");do_mstat(&actor,command);}
        if(caller==2){strcpy(command,"info");do_covenant(&actor,command);}
        if(caller==3){check_leaderboard(&actor,command);
            snprintf(output,sizeof output,"%li:%s,%li:%s,%li:%s",
                leader_board.bestpk_number,leader_board.bestpk_name,
                leader_board.worstpk_number,leader_board.worstpk_name,
                leader_board.tt_number,leader_board.tt_name);}
        if(caller==4){int granted=apply_login_age();snprintf(output,sizeof output,"%d,%d",granted,!!IS_SET(pc.flag,FLAG_PK));}
        uint64_t row=hash(UINT64_C(1469598103934665603),output,strlen(output));
        total=hash(total,&row,sizeof row);rows++;
        if(dump)printf("%d,%d,%d,%d,%d,%" PRIu64 "\n",a,b,h,caller,ansi,row);
    }
    assert(rows==3200);fprintf(stderr,"PROGRESS_NATIVE_HASH=%" PRIu64 "\n",total);
    if(!dump)assert(total==UINT64_C(1863285937676416231));
}
static void metrics_edge(int n)
{
    metrics_fixture();char command[40]="";
    if(n==0){actor.played=LONG_MAX;actor.logon--; (void)get_hours(&actor);}
    if(n==1){current_time=LONG_MAX;actor.logon=LONG_MIN;(void)get_hours(&actor);}
    if(n==2){actor.pkill=LONG_MAX;(void)get_ratio(&actor);}
    if(n==3){actor.pkill=LONG_MAX;actor.pdeath=1;(void)get_ratio(&actor);}
    if(n==4){actor.pkill=1;actor.pdeath=-1;(void)get_ratio(&actor);}
    if(n==5){actor.pkill=1000000000;actor.pdeath=1;(void)get_ratio(&actor);}
    if(n==6){actor.played=LONG_MAX;actor.logon--;check_leaderboard(&actor,command);}
    if(n==7){actor.pkill=1;actor.pdeath=-1;check_leaderboard(&actor,command);}
    if(n==8){actor.played=LONG_MAX;actor.logon--;strcpy(command,"examiner");do_mstat(&actor,command);}
    if(n==9){actor.pkill=1;actor.pdeath=-1;strcpy(command,"info");do_covenant(&actor,command);}
    if(n==10)(void)get_hours(NULL);
    if(n==11)(void)get_ratio(NULL);
    if(n==12)check_leaderboard(NULL,command);
    if(n==13){actor.pcdata=NULL;check_leaderboard(&actor,command);}
    if(n==8||n==9)assert(strstr(output,"unavailable"));
}
static int fits_long(__int128 value)
{return value>=LONG_MIN&&value<=LONG_MAX;}
static void metrics_oracles(void)
{
    static const long values[]={LONG_MIN,LONG_MIN+1,-1000000000,-1000,-1,0,1,
        2,3,10,1000,1000000000,LONG_MAX-1,LONG_MAX};
    for(unsigned a=0;a<14;a++)for(unsigned b=0;b<14;b++){
        metrics_fixture();actor.pkill=values[a];actor.pdeath=values[b];
        __int128 numerator=(__int128)100*actor.pkill,result=numerator;
        int valid=fits_long(numerator);
        if(actor.pdeath!=0){
            __int128 sum=(__int128)actor.pkill+actor.pdeath;
            valid=valid&&fits_long(sum)&&sum!=0;
            if(valid){
                __int128 denominator=sum*sum;numerator*=actor.pkill;
                valid=fits_long(denominator)&&fits_long(numerator);
                if(valid){result=(__int128)actor.pkill*(numerator/denominator);valid=fits_long(result);}
            }
        }
        CHAR_DATA before=actor;long actual=314159;
        assert(get_ratio_checked(&actor,&actual)==valid);
        assert(actual==(valid?(long)result:314159));
        assert(get_ratio(&actor)==(valid?(long)result:0));
        assert(!memcmp(&actor,&before,sizeof actor));
    }
    for(unsigned a=0;a<14;a++)for(unsigned b=0;b<14;b++)for(unsigned c=0;c<14;c++){
        metrics_fixture();actor.played=values[a];current_time=values[b];actor.logon=values[c];
        __int128 delta=(__int128)current_time-actor.logon,sum=(__int128)actor.played+delta;
        int valid=fits_long(delta)&&fits_long(sum);long actual=314159;
        assert(get_hours_checked(&actor,&actual)==valid);
        assert(actual==(valid?(long)sum/3600:314159));
        assert(get_hours(&actor)==(valid?(long)sum/3600:0));
        int eligible=valid&&(long)sum/3600>=AGE_PK;
        assert(grant_age_pk(&actor)==eligible);
        assert(!!IS_SET(pc.flag,FLAG_PK)==eligible);
        assert(!grant_age_pk(&actor)); /* No repeated grant/event. */
        SET_BIT(pc.flag,FLAG_PK);
        assert(!grant_age_pk(&actor)&&IS_SET(pc.flag,FLAG_PK)); /* Never revoke. */
    }
    metrics_fixture();long value=314159;
    assert(!get_hours_checked(NULL,&value)&&value==314159);
    assert(!get_ratio_checked(NULL,&value)&&value==314159);
    assert(!get_hours_checked(&actor,NULL)&&!get_ratio_checked(&actor,NULL));
    assert(!grant_age_pk(NULL));actor.pcdata=NULL;assert(!grant_age_pk(&actor));
    SET_BIT(actor.act,ACT_IS_NPC);actor.pkill=LONG_MAX;actor.pdeath=LONG_MAX;
    assert(get_ratio_checked(&actor,&value)&&value==0&&!grant_age_pk(&actor));
}
static void leaderboard_failures_and_reload(void)
{
    metrics_fixture();actor.pkill=1;actor.pdeath=-1;actor.played=LONG_MAX;actor.logon--;
    LEADER_BOARD before=leader_board;char empty[]="";
    check_leaderboard(&actor,empty);
    assert(!memcmp(&leader_board,&before,sizeof before)&&access("../area/leader.txt",F_OK)!=0);
    actor.played=31*3600;actor.logon=current_time;
    check_leaderboard(&actor,empty);
    assert(leader_board.bestpk_number==before.bestpk_number&&
           leader_board.worstpk_number==before.worstpk_number&&leader_board.tt_number==31);
    board_names(0);load_leaderboard();
    assert(leader_board.tt_number==31&&!strcmp(leader_board.tt_name,actor.name));
    assert(leader_board.bestpk_number==before.bestpk_number&&
           leader_board.worstpk_number==before.worstpk_number);
    /* A bad derived ratio must not block unrelated valid counters. */
    metrics_fixture();actor.pkill=1;actor.pdeath=-1;actor.played=LONG_MAX;actor.logon--;
    leader_board.mk_number=0;actor.mkill=10;
    check_leaderboard(&actor,empty);assert(leader_board.mk_number==10);
    assert(leader_board.bestpk_number==0&&leader_board.worstpk_number==1000000&&leader_board.tt_number==0);
    board_names(0);load_leaderboard();assert(leader_board.mk_number==10);
    metrics_fixture();actor.pkill=2;actor.pdeath=1;actor.played=LONG_MAX;actor.logon--;
    check_leaderboard(&actor,empty);
    assert(leader_board.bestpk_number==88&&leader_board.worstpk_number==88&&leader_board.tt_number==0);
    board_names(0);load_leaderboard();
    assert(leader_board.bestpk_number==88&&leader_board.worstpk_number==88&&leader_board.tt_number==0);
    for(int npc=0;npc<2;npc++){
        metrics_fixture();if(npc)SET_BIT(actor.act,ACT_IS_NPC);else actor.level=LEV_BUI;
        LEADER_BOARD copy=leader_board;check_leaderboard(&actor,empty);
        assert(!memcmp(&copy,&leader_board,sizeof copy)&&access("../area/leader.txt",F_OK)!=0);
    }
}
#ifndef PROGRESS_METRICS_FIXTURE_ONLY
int main(int argc,char **argv)
{
    sandbox_begin();
    if(argc>1&&!strcmp(argv[1],"--native-dump"))metrics_native(1);
    else if(argc>2&&!strcmp(argv[1],"--boundary"))metrics_edge(atoi(argv[2]));
    else {metrics_native(0);for(int n=0;n<14;n++)metrics_edge(n);
        metrics_oracles();leaderboard_failures_and_reload();
        puts("Progress metrics:3200 native snapshots,196 PK/2744 timing oracles,14 boundaries,PK eligibility and leaderboard isolation/reloads passed.");}
    board_names(0);sandbox_end();return 0;
}
#endif
