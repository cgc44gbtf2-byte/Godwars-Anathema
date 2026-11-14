/* Reuse real-engine object/viewer fixtures; no inspection command stubs. */
#define main identify_fixture_main
#include "test_identify.c"
#undef main
void obj_score(CHAR_DATA *ch,OBJ_DATA *obj);
static OBJ_DATA other;
static void comparison_fixture(void)
{
    fixture();other=obj;other.name="other";other.short_descr="another sword";
    obj.next_content=&other;other.next_content=NULL;
    actor.carrying=&obj;obj.carried_by=other.carried_by=&actor;
    obj.wear_loc=other.wear_loc=WEAR_NONE;
    obj.wear_flags=other.wear_flags=ITEM_TAKE|ITEM_WEAR_HOLD;
}
static void score_native(int dump)
{
    const long types[]={ITEM_WEAPON,ITEM_ARMOR,ITEM_POTION,ITEM_SCROLL,
        ITEM_WAND,ITEM_STAFF,ITEM_QUEST,ITEM_PAGE,ITEM_TREASURE};
    const long powers[]={0,1,4,30,34,37,48,53,2,1001,2004,24048};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int t=0;t<9;t++)for(int p=0;p<12;p++)for(int owner=0;owner<4;owner++){
        fixture();obj.item_type=types[t];obj.value[0]=powers[p];obj.value[3]=p;
        obj.level=p*5;if(!(owner&1))obj.questmaker="";if(!(owner&2))obj.questowner="";
        obj_score(&actor,&obj);
        uint64_t row=hash(UINT64_C(1469598103934665603),output,desc.outtop);
        total=hash(total,&row,sizeof row);rows++;
        if(dump)printf("score,%d,%d,%d,%" PRIu64 "\n",t,p,owner,row);
    }
    assert(rows==432);fprintf(stderr,"SCORE_NATIVE_HASH=%" PRIu64 "\n",total);
    if(!dump)assert(total==UINT64_C(13185047630302027763));
}
static void compare_native(int dump)
{
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int type=0;type<3;type++)for(int a=0;a<5;a++)for(int b=0;b<5;b++)
    for(int mode=0;mode<4;mode++){
        comparison_fixture();obj.item_type=other.item_type=type==0?ITEM_WEAPON:type==1?ITEM_ARMOR:ITEM_TREASURE;
        obj.value[0]=a;other.value[0]=b;obj.value[1]=a;obj.value[2]=a+3;
        other.value[1]=b;other.value[2]=b+3;
        if(mode==1)other.wear_loc=WEAR_HOLD;
        if(mode==3)other.item_type=ITEM_TRASH;
        char arg[40];strcpy(arg,mode==1?"silver":mode==2?"silver silver":"silver other");
        do_compare(&actor,arg);
        uint64_t row=hash(UINT64_C(1469598103934665603),output,desc.outtop);
        total=hash(total,&row,sizeof row);rows++;
        if(dump)printf("compare,%d,%d,%d,%d,%" PRIu64 "\n",type,a,b,mode,row);
    }
    assert(rows==300);fprintf(stderr,"COMPARE_NATIVE_HASH=%" PRIu64 "\n",total);
    if(!dump)assert(total==UINT64_C(2747055546945648736));
}
static void edge(int n)
{
    fixture();memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
    if(n==0)obj.short_descr=large;
    if(n==1)obj.questmaker=obj.questowner=large;
    if(n==2)obj.short_descr=NULL;
    if(n==3)obj.value[0]=MAX_SKILL+100;
    if(n==4)obj.value[1]=obj.value[2]=LONG_MAX;
    if(n==5)obj.pIndexData=NULL;
    if(n==6){obj_score(&actor,NULL);assert(!desc.outtop);return;}
    if(n==7){obj_score(NULL,&obj);assert(!desc.outtop);return;}
    if(n==8){comparison_fixture();obj.value[1]=obj.value[2]=LONG_MAX;
        other.value[1]=other.value[2]=LONG_MIN;char arg[]="silver other";
        do_compare(&actor,arg);assert(strstr(output,"better"));return;}
    obj_score(&actor,&obj);assert(desc.outtop>0);
}
static void exact_comparisons(void)
{
    const long v[]={LONG_MIN,LONG_MIN+1,-5,-4,-3,-1,0,1,3,4,5,LONG_MAX-1,LONG_MAX};
    for(unsigned a=0;a<13;a++)for(unsigned b=0;b<13;b++)
    for(unsigned c=0;c<13;c++)for(unsigned d=0;d<13;d++){
        comparison_fixture();obj.value[1]=v[a];obj.value[2]=v[b];
        other.value[1]=v[c];other.value[2]=v[d];
        __int128 x=(__int128)v[a]+v[b],y=(__int128)v[c]+v[d];
        char arg[]="silver other";do_compare(&actor,arg);
        assert(strstr(output,x==y?"about the same":x>y?"better":"worse"));
    }
    /* Score averages use the same truncation rule as identify. */
    for(unsigned a=0;a<13;a++)for(unsigned b=0;b<13;b++){
        fixture();obj.value[1]=v[a];obj.value[2]=v[b];char wanted[80];
        snprintf(wanted,sizeof wanted,"(average %li).",(long)(((__int128)v[a]+v[b])/2));
        obj_score(&actor,&obj);assert(strstr(output,wanted));
    }
}
static void contexts_and_entry(void)
{
    fixture();obj_score(&actor,&obj);size_t size=(size_t)desc.outtop;
    char expected[MSL*2];assert(size<sizeof expected);memcpy(expected,output,size);
    memset(output,0,sizeof output);desc.outtop=0;
    pc.chobj=&obj;SET_BIT(actor.extra,EXTRA_OSWITCH);
    char empty[]="";do_score(&actor,empty);
    assert(desc.outtop==(long)size&&!memcmp(output,expected,size));
    fixture();SET_BIT(actor.extra,EXTRA_OSWITCH);pc.chobj=NULL;
    do_score(&actor,empty);assert(!desc.outtop);
    fixture();actor.pcdata=NULL;obj_score(&actor,&obj);assert(!desc.outtop);
    char command[]="silver other";
    do_compare(&actor,command);assert(!desc.outtop);
    fixture();do_compare(NULL,command);do_compare(&actor,NULL);assert(!desc.outtop);
    for(int ansi=0;ansi<2;ansi++)for(int hashes=1;hashes<4;hashes++){
        fixture();if(ansi)SET_BIT(actor.act,PLR_ANSI);
        memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
        memset(large+MSL-3-8-hashes,'#',(size_t)hashes);obj.short_descr=large;
        OBJ_DATA before=obj;obj_score(&actor,&obj);
        char *line=strstr(output,"\n\r");
        assert(line&&line-output<MSL&&line-output>MSL-10);
        assert(strstr(line,"Type ")&&!memcmp(&before,&obj,sizeof obj));
    }
}
int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){score_native(1);compare_native(1);return 0;}
    if(argc>2&&!strcmp(argv[1],"--boundary")){edge(atoi(argv[2]));return 0;}
    score_native(0);compare_native(0);for(int n=0;n<9;n++)edge(n);
    exact_comparisons();contexts_and_entry();
    puts("Item inspection:732 native snapshots,9 boundaries,28561 exact comparisons and169 averages passed.");
    return 0;
}
