/* Real entry/revert and class reset. No command, rank or save permission stubs. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};
static uint64_t hash_text(const char *s,uint64_t h)
{
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        h=(h^*p)*UINT64_C(1099511628211);
    return h;
}
static void setup(int mode,int metis,int ahroun,int features)
{
    for(int i=0;i<3;i++)free_string(chars[i].morph);
    reset();room.room_flags=ROOM_SAFE;
    chars[0].class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].breed[BREED_METIS]=metis;
    players[0].auspice[AUSPICE_AHROUN]=ahroun;
    players[0].wolf=100;
    if(mode) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(chars,hybrid_by_parents(
            chars[0].class,mode>=10?CLASS_WEREWOLF:parents[mode-1]),&state));
        assert(hybrid_player_attach(chars,&state));
    }
    for(int i=0;i<3;i++) {
        chars[i].morph=str_dup("");
        chars[i].stance[0]=-1;
        descriptors[i].connected=CON_GET_NAME;
    }
    if(features)SET_BIT(chars[0].vampaff,VAM_CLAWS|VAM_FANGS);
    if(features==2&&metis>=3)SET_BIT(chars[0].vampaff,VAM_NIGHTSIGHT);
    init_mm();
}
static uint64_t snapshot(int mode,int metis,int ahroun,int features,int dead,
                         int cycle,int phase,uint64_t h,bool dump)
{
    char row[2048];
    uint64_t messages=UINT64_C(1469598103934665603);
    for(int i=0;i<3;i++)messages=hash_text(output[i],messages);
    messages=hash_text(chars[0].morph,messages);
    int n=snprintf(row,sizeof row,"%d,%d,%d,%d,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%llu,%d\n",
        mode,metis,ahroun,features,dead,cycle,phase,chars[0].act,
        chars[0].polyaff,chars[0].vampaff,chars[0].affected_by,
        players[0].wolf,chars[0].style[0],chars[0].position,
        (unsigned long long)messages,number_mm());
    assert(n>0&&(size_t)n<sizeof row);if(dump)fputs(row,stdout);
    return hash_text(row,h);
}
static void native_cases(bool dump)
{
    const int metis[]={0,2,3,5},ahroun[]={0,2,3,4,5};
    unsigned count=0;uint64_t h=UINT64_C(1469598103934665603);
    for(int mode=0;mode<19;mode++)for(int m=0;m<4;m++)for(int a=0;a<5;a++)
    for(int features=0;features<3;features++)for(int dead=0;dead<2;dead++) {
        setup(mode,metis[m],ahroun[a],features);
        for(int cycle=0;cycle<2;cycle++) {
            chars[0].hit=1000;chars[0].position=POS_STANDING;
            players[0].wolf=100;
            do_werewolf(chars,"");
            h=snapshot(mode,metis[m],ahroun[a],features,dead,cycle,0,h,dump);++count;
            if(dead){chars[0].hit=0;chars[0].position=POS_DEAD;}
            do_unwerewolf(chars,"");
            h=snapshot(mode,metis[m],ahroun[a],features,dead,cycle,1,h,dump);++count;
        }
    }
    assert(count==9120);
    assert(h==UINT64_C(3967719075077048123));
    if(!dump)printf("native Werewolf reversion rows=%u hash=%llu\n",
                   count,(unsigned long long)h);
}
static void rank_loss(int mode,int rank,int dead)
{
    setup(mode,5,4,0);
    do_werewolf(chars,"");
    assert(IS_SET(chars[0].act,PLR_WOLFMAN));
    assert(IS_VAMPAFF(chars,VAM_NIGHTSIGHT));
    players[0].breed[BREED_METIS]=rank;
    if(dead){chars[0].hit=0;chars[0].position=POS_DEAD;}
    do_unwerewolf(chars,NULL);
    assert(!IS_SET(chars[0].act,PLR_WOLFMAN));
    assert(!IS_VAMPAFF(chars,VAM_NIGHTSIGHT|VAM_CLAWS|VAM_FANGS));
    assert(!IS_AFFECTED(chars,AFF_POLYMORPH));
    assert(!IS_VAMPAFF(chars,VAM_DISGUISED));
    assert(!*chars[0].morph&&players[0].wolf==0);
    do_unwerewolf(chars,NULL); /* Cleanup is idempotent. */
    assert(!IS_VAMPAFF(chars,VAM_NIGHTSIGHT));
}
static void clear_form(int mode)
{
    setup(mode,5,4,0);do_werewolf(chars,"");
    SET_BIT(chars[0].act,PLR_AUTOEXIT);
    chars[1].level=LEV_IMP;
    do_clearclass(&chars[1],"caster");
    assert(chars[0].class==0);
    assert(!IS_SET(chars[0].act,PLR_WOLFMAN));
    assert(IS_SET(chars[0].act,PLR_AUTOEXIT));
    assert(chars[0].vampaff==0&&chars[0].affected_by==0);
    assert(players[0].wolf==0&&!*chars[0].morph);
}
int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_cases(TRUE);return 0;}
    if(argc>2&&!strcmp(argv[1],"--rank-loss")) {
        rank_loss(atoi(argv[2]),argc>3?atoi(argv[3]):0,argc>4?atoi(argv[4]):0);
        return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--clear")){clear_form(atoi(argv[2]));return 0;}
    native_cases(FALSE);
    for(int mode=0;mode<10;mode++) {
        for(int rank=0;rank<6;rank++)for(int dead=0;dead<2;dead++)
            rank_loss(mode,rank,dead);
        clear_form(mode);
    }
    do_unwerewolf(NULL,NULL);
    printf("Werewolf reversion: 120 rank-loss/retention cases, 10 real class resets passed.\n");
    return 0;
}
