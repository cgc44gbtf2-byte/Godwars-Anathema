/* Real interpreter, healing, ranks and messages; no permission stubs. */
#define main restored_fixture_main
#include "test_restored_spells.c"
#undef main
#include <stdint.h>
#include "classes/hybrid/player.h"

static const long parents[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE};

static void setup(int mode,long rank)
{
    if(mobile)remove_mobile();
    reset();
    chars[0].class=mode>=10?parents[mode-10]:CLASS_WEREWOLF;
    players[0].auspice[AUSPICE_THEURGE]=rank;
    players[0].obeah=0;
    if(mode) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(chars,hybrid_by_parents(
            chars[0].class,mode>=10?CLASS_WEREWOLF:parents[mode-1]),&state));
        assert(hybrid_player_attach(chars,&state));
    }
}

static uint64_t hash_text(const char *s,uint64_t hash)
{
    for(const unsigned char *p=(const unsigned char *)s;*p;p++)
        hash=(hash^*p)*UINT64_C(1099511628211);
    return hash;
}

static void native_cases(bool dump)
{
    uint64_t hash=UINT64_C(1469598103934665603);unsigned count=0;
    for(int mode=0;mode<10;mode++)for(long rank=1;rank<=5;rank++)
    for(int path=0;path<8;path++)for(int seed=0;seed<2;seed++) {
        setup(mode,rank);init_mm();for(int i=0;i<seed;i++)(void)number_mm();
        CHAR_DATA *target=path==1?chars:&chars[1];
        target->max_hit=10001;target->hit=path==2?10000:path==3?10001:path==4?-5:111;
        if(path==5)players[0].obeah=1;
        if(path==6)players[0].obeah=LONG_MAX;
        if(path==7){add_mobile();target=mobile;target->max_hit=10001;target->hit=111;}
        char command[64];strcpy(command,path==0?"mother":path==1?"mother caster":path==7?"mother mobile":"mother target");
        interpret(chars,command);
        uint64_t messages=UINT64_C(1469598103934665603);
        for(int i=0;i<3;i++)messages=hash_text(output[i],messages);
        char row[256];
        int length=snprintf(row,sizeof row,"%d,%ld,%d,%d,%ld,%ld,%ld,%ld,%llu,%d\n",mode,rank,path,seed,
            target->hit,target->position,players[0].obeah,chars[0].wait,
            (unsigned long long)messages,number_mm());
        assert(length>0&&(size_t)length<sizeof row);
        hash=hash_text(row,hash);++count;if(dump)fputs(row,stdout);
    }
    if(mobile)remove_mobile();
    assert(count==800&&hash==UINT64_C(1651741104360414461));
}

static void extreme_healing(void)
{
    for(int mode=0;mode<10;mode++)for(long rank=1;rank<=5;rank++) {
        setup(mode,rank);
        chars[1].hit=LONG_MAX-1;chars[1].max_hit=LONG_MAX;
        do_mother(chars,"target");
        assert(chars[1].hit==LONG_MAX&&players[0].obeah==15&&chars[0].wait==12);
        setup(mode,rank);
        chars[1].hit=LONG_MIN;chars[1].max_hit=LONG_MAX;
        do_mother(chars,"target");
        assert(chars[1].hit>LONG_MIN&&chars[1].hit<=chars[1].max_hit);
    }
}

static void access_and_invalid_hp(void)
{
    for(int mode=10;mode<19;mode++) {
        setup(mode,5);long before=chars[1].hit;
        do_mother(chars,"target");
        assert(chars[1].hit==before&&players[0].obeah==0&&chars[0].wait==0);
    }
    for(int max=0;max>=-1;max--) {
        setup(0,5);chars[1].hit=-10;chars[1].max_hit=max;
        do_mother(chars,"target");
        assert(chars[1].hit==-10&&players[0].obeah==0&&chars[0].wait==0);
    }
    setup(0,0);do_mother(chars,"target");assert(players[0].obeah==0);
    setup(0,-1);do_mother(chars,"target");assert(players[0].obeah==0);
    setup(0,LONG_MAX);chars[1].hit=LONG_MAX-1;chars[1].max_hit=LONG_MAX;
    do_mother(chars,"target");assert(chars[1].hit==LONG_MAX);
}

static void oversized_input(void)
{
    setup(0,5);
    char *input=malloc(MAX_STRING_LENGTH*2);assert(input);
    memset(input,'a',MAX_STRING_LENGTH*2-1);input[MAX_STRING_LENGTH*2-1]='\0';
    do_mother(chars,input);free(input);
    assert(players[0].obeah==0&&chars[0].wait==0);
}

int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native_cases(TRUE);return 0;}
    if(argc>1&&!strcmp(argv[1],"--extreme")){extreme_healing();return 0;}
    if(argc>1&&!strcmp(argv[1],"--long-input")){oversized_input();return 0;}
    native_cases(FALSE);extreme_healing();oversized_input();access_and_invalid_hp();
    puts("Mother's Touch: healing bounds and target input passed");
    return 0;
}
