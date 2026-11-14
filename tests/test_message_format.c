/* Local, real-engine regression coverage for the three shared formatters. */
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
typedef void (*FORMATTER)(const char *,CHAR_DATA *,const void *,const void *,long);
static FORMATTER funcs[]={act,act2,kavitem};
static CHAR_DATA people[4];
static PC_DATA pcs[4];
static DESCRIPTOR_DATA descs[4];
static ROOM_INDEX_DATA room,remote;
static AREA_DATA area;
static OBJ_DATA objects[2],container;
static char output[4][32768],large[MSL*3];
static const long audiences[]={TO_CHAR,TO_VICT,TO_ROOM,TO_NOTVICT};

static void fixture(void)
{
    memset(people,0,sizeof people);memset(pcs,0,sizeof pcs);
    memset(descs,0,sizeof descs);memset(objects,0,sizeof objects);
    memset(output,0,sizeof output);memset(&room,0,sizeof room);
    memset(&remote,0,sizeof remote);memset(&area,0,sizeof area);memset(&container,0,sizeof container);
    room.vnum=901;room.light=1;room.area=&area;room.people=people;
    remote.vnum=902;remote.light=1;remote.area=&area;remote.people=people+3;
    char_list=people;descriptor_list=descs;world_affects=0;
    time_info.hour=12;weather_info.sunlight=SUN_LIGHT;
    for(int n=0;n<4;n++){
        CHAR_DATA *ch=people+n;clear_char(ch);ch->pcdata=pcs+n;
        ch->name=ch->short_descr=n==0?"alpha":n==1?"beta":n==2?"gamma":"wizard";
        ch->morph="a masked person";ch->level=LEV_AWA;ch->position=POS_STANDING;
        ch->in_room=n==3?&remote:&room;ch->desc=descs+n;ch->vampgen=4;
        ch->hit=ch->max_hit=ch->mana=ch->max_mana=50000;
        ch->next=n<3?people+n+1:NULL;ch->next_in_room=n<2?people+n+1:NULL;
        descs[n].character=ch;descs[n].connected=CON_PLAYING;
        descs[n].outbuf=output[n];descs[n].outsize=sizeof output[n];
        descs[n].fcommand=TRUE;descs[n].next=n<3?descs+n+1:NULL;
    }
    objects[0].name="relic";objects[0].short_descr="a silver relic";
    objects[1].name="shield";objects[1].short_descr="a round shield";
    objects[0].item_type=objects[1].item_type=ITEM_TREASURE;
}
static uint64_t hash(uint64_t h,const void *v,size_t len)
{const unsigned char *p=v;while(len--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
static uint64_t native(int dump)
{
    static const char *formats[]={"a quiet room.","$n greets $N.","$e $m $s; $E $M $S.",
        "text $t.","other $T.","item $p.","item $P.","door $d.","$n lifts $p.",
        "owner $o.","$n watches.","$N nods."};
    uint64_t sum=UINT64_C(1469598103934665603);unsigned rows=0;
    for(int f=0;f<3;f++)for(int a=0;a<4;a++)for(int sex=0;sex<3;sex++)
    for(int visibility=0;visibility<3;visibility++)for(int k=0;k<12;k++){
        /* TO_VICT's arg2 is a character, never an object/text pointer. */
        if(a==1&&(k==4||k==6||k==7))continue;
        fixture();people[0].sex=sex;people[1].sex=2-sex;
        if(visibility==1){SET_BIT(people[0].affected_by,AFF_HIDE);objects[0].chobj=people+2;}
        if(visibility==2){SET_BIT(people[1].affected_by,AFF_POLYMORPH);people[2].position=POS_SLEEPING;}
        const void *one=k==3?"a whispered word":(const void *)(objects);
        const void *two=(k==4||k==7)?(const void *)"iron gate":
                        k==6?(const void *)(objects+1):(const void *)(people+1);
        funcs[f](formats[k],people,one,two,audiences[a]);
        uint64_t row=UINT64_C(1469598103934665603);
        for(int n=0;n<4;n++){
            row=hash(row,output[n],descs[n].outtop);
            row=hash(row,&descs[n].outtop,sizeof descs[n].outtop);
        }
        sum=hash(sum,&row,sizeof row);rows++;
        if(dump)printf("%d,%d,%d,%d,%d,%" PRIu64 "\n",f,a,sex,visibility,k,row);
    }
    assert(rows==1215);
    if(!dump)assert(sum==UINT64_C(17448779909195589922));
    fprintf(stderr,"FORMAT_NATIVE_HASH=%" PRIu64 "\n",sum);return sum;
}
static void boundaries(int f,int n)
{
    fixture();memset(large,'z',sizeof large-1);large[sizeof large-1]=0;
    const char *fmt="$n says hello.";const void *one=objects,*two=people+1;
    switch(n){
    case 0:fmt=large;break;
    case 1:people[0].short_descr=large;SET_BIT(people[0].act,ACT_IS_NPC);break;
    case 2:objects[0].short_descr=large;fmt=f==2?"$o":"$p";break;
    case 3:fmt="unfinished$";break;
    case 4:fmt="$p";one=NULL;break;
    case 5:people[0].short_descr=NULL;SET_BIT(people[0].act,ACT_IS_NPC);break;
    case 6:fmt=f==0?"$t":"$p";one=f==0?NULL:objects;objects[0].short_descr=NULL;break;
    case 7:people[0].in_room=NULL;break;
    }
    funcs[f](fmt,people,one,two,TO_ROOM);
    if(n==7)assert(!descs[1].outtop&&!descs[2].outtop);
    else{
        /* write_to_buffer appends its seven-byte normal-colour reset. */
        assert(descs[1].outtop>=9&&descs[1].outtop<=MSL+6);
        assert(output[1][descs[1].outtop-9]=='\n'&&output[1][descs[1].outtop-8]=='\r');
        if(n==3)assert(!strncmp(output[1],"Unfinished$\n\r",13));
    }
}
static void familiars(void)
{
    for(int had=0;had<2;had++)for(int awake=0;awake<2;awake++){
        fixture();people[2].desc=NULL;SET_BIT(people[2].act,ACT_IS_NPC);
        people[2].wizard=people+3;pcs[3].familiar=people+2;
        people[3].position=awake?POS_STANDING:POS_SLEEPING;
        if(had)SET_BIT(people[3].extra,EXTRA_CAN_SEE);
        long before=people[3].extra;
        act("$n lifts $p.",people,objects,NULL,TO_ROOM);
        assert(people[3].extra==before);
        assert((descs[3].outtop>0)==awake);
        if(awake)assert(!strncmp(output[3],"[ ",2)&&strstr(output[3],"Alpha lifts"));
    }
    fixture();people[2].desc=NULL;SET_BIT(people[2].act,ACT_IS_NPC);
    people[2].wizard=people+3;people[3].pcdata=NULL;
    act("$n nods.",people,NULL,NULL,TO_ROOM);
    assert(!descs[3].outtop);
}
static void contexts_and_colours(void)
{
    for(int f=0;f<3;f++){
        fixture();people[0].pcdata=NULL;funcs[f]("x",people,NULL,NULL,TO_ROOM);
        assert(!descs[1].outtop);
        fixture();people[1].pcdata=NULL;funcs[f]("$n nods.",people,NULL,NULL,TO_ROOM);
        assert(!descs[1].outtop&&descs[2].outtop>0);
        fixture();people[1].name=NULL;funcs[f]("$N",people,NULL,people+1,TO_ROOM);
        /* kavitem does not support $N; the other dialects use the fallback. */
        if(f!=2)assert(strstr(output[2],"Someone"));
        fixture();funcs[f]("x",people,NULL,NULL,TO_VICT);assert(!descs[1].outtop);
        fixture();people[1].in_room=NULL;
        funcs[f]("x",people,NULL,people+1,TO_VICT);assert(!descs[1].outtop);
        for(int same=0;same<2;same++){
            fixture();room.vnum=ROOM_VNUM_IN_OBJECT;
            pcs[0].chobj=objects;pcs[1].chobj=objects+1;
            objects[0].in_obj=&container;objects[1].in_obj=same?&container:NULL;
            funcs[f]("$n nods.",people,NULL,NULL,TO_ROOM);
            assert((descs[1].outtop>0)==same&&!descs[2].outtop);
        }
        for(int ansi=0;ansi<2;ansi++)for(int hashes=1;hashes<4;hashes++){
            fixture();if(ansi)SET_BIT(people[1].act,PLR_ANSI);
            memset(large,'a',sizeof large-1);large[sizeof large-1]=0;
            memset(large+MSL-3-hashes,'#',(size_t)hashes);
            funcs[f](large,people,NULL,NULL,TO_ROOM);
            long n=descs[1].outtop;
            assert(n>9&&output[1][n-9]=='\n'&&output[1][n-8]=='\r');
        }
    }
    /* Familiar redirection rejected by object-room isolation must restore
     * a pre-existing sight grant as well as a temporary one. */
    for(int had=0;had<2;had++){
        fixture();room.vnum=ROOM_VNUM_IN_OBJECT;
        people[2].desc=NULL;SET_BIT(people[2].act,ACT_IS_NPC);
        people[2].wizard=people+3;pcs[3].familiar=people+2;
        if(had)SET_BIT(people[3].extra,EXTRA_CAN_SEE);
        long before=people[3].extra;act("$n nods.",people,NULL,NULL,TO_ROOM);
        assert(before==people[3].extra&&!descs[3].outtop);
    }
}
int main(int argc,char **argv)
{
    if(argc>1&&!strcmp(argv[1],"--native-dump")){native(1);return 0;}
    if(argc>1&&!strcmp(argv[1],"--familiar")){familiars();return 0;}
    if(argc>3&&!strcmp(argv[1],"--boundary")){boundaries(atoi(argv[2]),atoi(argv[3]));return 0;}
    native(0);
    for(int f=0;f<3;f++)for(int n=0;n<8;n++)boundaries(f,n);
    for(int f=0;f<3;f++){fixture();funcs[f]("x",NULL,NULL,NULL,TO_ROOM);}
    familiars();contexts_and_colours();
    puts("Message formatters:1215 native snapshots,24 bounded/null/parser cases and familiar visibility restoration passed.");
    return 0;
}
