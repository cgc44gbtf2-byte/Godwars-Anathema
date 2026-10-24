/* Whole-engine identification output: native goldens and local edge cases. */
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
static CHAR_DATA actor;
static PC_DATA pc;
static DESCRIPTOR_DATA desc;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static OBJ_DATA obj;
static OBJ_INDEX_DATA index_data;
static AFFECT_DATA effects[2];
static char output[MSL*32],large[MSL*3];
static void fixture(void)
{
    memset(&actor,0,sizeof actor);memset(&pc,0,sizeof pc);
    memset(&desc,0,sizeof desc);memset(&obj,0,sizeof obj);
    memset(&room,0,sizeof room);memset(&area,0,sizeof area);
    memset(&index_data,0,sizeof index_data);memset(effects,0,sizeof effects);
    memset(output,0,sizeof output);clear_char(&actor);
    actor.pcdata=&pc;actor.name="examiner";actor.short_descr="examiner";
    actor.morph="examiner";actor.level=LEV_AWA;actor.position=POS_STANDING;
    actor.desc=&desc;actor.in_room=&room;actor.vampgen=4;
    room.vnum=901;room.light=1;room.people=&actor;room.area=&area;
    time_info.hour=12;weather_info.sunlight=SUN_LIGHT;world_affects=0;
    char_list=&actor;descriptor_list=&desc;
    desc.character=&actor;desc.connected=CON_PLAYING;
    desc.outbuf=output;desc.outsize=sizeof output;desc.fcommand=TRUE;
    obj.name="silver sword";obj.short_descr="a SILVER Sword";
    obj.item_type=ITEM_WEAPON;obj.pIndexData=&index_data;
    obj.questmaker="Maker";obj.questowner="Owner";
    obj.weight=7;obj.cost=31;obj.points=12;obj.level=30;
    obj.value[0]=48;obj.value[1]=11;obj.value[2]=20;
    effects[0].location=APPLY_HITROLL;effects[0].modifier=5;
    effects[1].location=APPLY_DAMROLL;effects[1].modifier=-7;
    index_data.affected=effects;obj.affected=effects+1;
    effects[0].next=NULL;
}
static uint64_t hash(uint64_t h,const void *v,size_t len)
{const unsigned char *p=v;while(len--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
static void native(int dump)
{
    static const long types[]={ITEM_WEAPON,ITEM_ARMOR,ITEM_POTION,ITEM_PILL,
        ITEM_SCROLL,ITEM_WAND,ITEM_STAFF,ITEM_QUEST,ITEM_QUESTCARD,ITEM_PAGE,
        ITEM_TREASURE,ITEM_CORPSE_NPC,ITEM_CORPSE_PC};
    static const long powers[]={0,1,4,30,34,37,48,53,2,1001,2004,24048};
    uint64_t total=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int t=0;t<13;t++)for(int p=0;p<12;p++)for(int owner=0;owner<4;owner++)
    for(int flags=0;flags<2;flags++){
        fixture();obj.item_type=types[t];obj.value[0]=powers[p];
        obj.value[3]=p<9?p:24;obj.level=p*5;
        if(!(owner&1))obj.questmaker="";if(!(owner&2))obj.questowner="";
        if(flags)obj.quest=QUEST_ENCHANTED|QUEST_AGGRAVATED;
        spell_identify(0,50,&actor,&obj);
        uint64_t row=hash(UINT64_C(1469598103934665603),output,desc.outtop);
        total=hash(total,&row,sizeof row);rows++;
        if(dump)printf("%d,%d,%d,%d,%" PRIu64 "\n",t,p,owner,flags,row);
    }
    assert(rows==1248);
    fprintf(stderr,"IDENTIFY_NATIVE_HASH=%" PRIu64 "\n",total);
    if(!dump)assert(total==UINT64_C(1470866982027351726));
}
static void boundary(int n)
{
    fixture();memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
    switch(n){
    case 0:obj.name=large;break;
    case 1:obj.questmaker=large;obj.questowner=large;break;
    case 2:obj.short_descr=large;break;
    case 3:obj.name=NULL;obj.short_descr=NULL;break;
    case 4:obj.value[0]=MAX_SKILL+100;break;
    case 5:obj.value[1]=LONG_MAX;obj.value[2]=LONG_MAX;break;
    case 6:obj.value[1]=LONG_MIN;obj.value[2]=LONG_MIN;break;
    case 7:obj.pIndexData=NULL;break;
    case 8:spell_identify(0,50,&actor,NULL);assert(!desc.outtop);return;
    case 9:spell_identify(0,50,NULL,&obj);assert(!desc.outtop);return;
    case 10:obj.value[0]=LONG_MAX;break;
    case 11:obj.value[1]=3;obj.value[2]=-4;break;
    }
    spell_identify(0,50,&actor,&obj);
    assert(desc.outtop>0&&desc.outtop<(long)sizeof output);
    if(n==4)assert(strstr(output,"unknown spell"));
    if(n==5)assert(strstr(output,"average 9223372036854775807"));
    if(n==6)assert(strstr(output,"average -9223372036854775808"));
    if(n==11)assert(strstr(output,"average 0"));
}
static void numeric_and_contexts(void)
{
    const long values[]={LONG_MIN,LONG_MIN+1,-5,-4,-3,-1,0,1,3,4,5,LONG_MAX-1,LONG_MAX};
    for(unsigned a=0;a<sizeof values/sizeof *values;a++)
    for(unsigned b=0;b<sizeof values/sizeof *values;b++){
        fixture();obj.value[1]=values[a];obj.value[2]=values[b];
        OBJ_DATA before=obj;
        spell_identify(0,50,&actor,&obj);
        char wanted[100];
        long average=(long)(((__int128)values[a]+values[b])/2);
        snprintf(wanted,sizeof wanted,"(average %li).",average);
        assert(strstr(output,wanted)&&!memcmp(&obj,&before,sizeof obj));
    }
    for(int ansi=0;ansi<2;ansi++)for(int hashes=1;hashes<4;hashes++){
        fixture();if(ansi)SET_BIT(actor.act,PLR_ANSI);
        memset(large,'x',sizeof large-1);large[sizeof large-1]=0;
        /* Header prefix Object ' consumes eight bytes before the name. */
        memset(large+MSL-3-8-hashes,'#',(size_t)hashes);obj.name=large;
        spell_identify(0,50,&actor,&obj);
        char *weight=strstr(output,"Weight");
        assert(!weight); /* Deliberate bounded header truncation, later lines remain. */
        assert(strstr(output,"\n\r")&&strstr(output,"Quest point value"));
    }
    fixture();actor.pcdata=NULL;spell_identify(0,50,&actor,&obj);assert(!desc.outtop);
    fixture();actor.in_room=NULL;spell_identify(0,50,&actor,&obj);
    assert(strstr(output,"Object 'silver sword'"));
    fixture();obj.value[0]=-1;obj.value[1]=-5;obj.value[2]=9;
    spell_identify(0,50,&actor,&obj);assert(strstr(output,"average 2"));
    fixture();obj.item_type=ITEM_POTION;
    obj.value[1]=LONG_MIN;obj.value[2]=LONG_MAX;obj.value[3]=MAX_SKILL;
    spell_identify(0,50,&actor,&obj);assert(strstr(output,"spells of:"));
}
int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native(1);return 0;}
    if(argc>2&&!strcmp(argv[1],"--boundary")){boundary(atoi(argv[2]));return 0;}
    native(0);for(int n=0;n<12;n++)boundary(n);
    numeric_and_contexts();
    puts("Identify:1248 native snapshots,12 boundaries,169 numeric pairs and context/colour cases passed.");
    return 0;
}
