/* Reuse the real-engine actors, records and deterministic RNG fixture.
 * No permission/rank/RNG/spell implementation is substituted. */
#define main acid_fixture_main
#include "test_hybrid_acidbolt.c"
#undef main

extern void __real_spell_web(long,long,CHAR_DATA *,void *);
static int web_calls,web_callback;
static const char *web_commands[]={"web beta","web absent","web","web",
    "web beta","web beta","web beta","web beta","web beta","web beta",
    "web beta","web beta","web alpha","web beta"};
typedef struct web_snapshot { long values[16];uint64_t text; } WEB_SNAPSHOT;

void __wrap_spell_web(long sn,long level,CHAR_DATA *ch,void *vo)
{
    ++web_calls;
    __real_spell_web(sn,level,ch,vo);
    if(web_callback) {
        int action=web_callback;web_callback=0;
        stop_fighting(ch,TRUE);char_from_room(ch);
        if(action==2) {
            assert(char_list==ch);char_list=ch->next;
            descs[0].character=NULL;free(ch);
        }
    }
}

static void web_fixture(long primary,long secondary,int learned,int ninja,long ki,int npc)
{
    fixture(primary,secondary,0,npc);
    pcs[0].drow[DROW_POWERS]=primary==CLASS_DROW&&learned?DROW_WEB:0;
    pcs[0].ninja[NINJA_CHIKYU]=ninja;
    pcs[0].ninja[NINJA_KI]=ki;
    if(secondary==CLASS_DROW)
        pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=learned?DROW_WEB:0;
    gsn_web=skill_lookup("web");assert(gsn_web>=0);
    web_calls=web_callback=0;
}

static void web_run(int mode,WEB_SNAPSHOT *s)
{
    char command[64];strcpy(command,web_commands[mode]);
    switch(mode) {
    case 3:set_fighting(actors,&actors[1]);break;
    case 4:room.room_flags=ROOM_SAFE;actors[0].status=actors[1].status=0;break;
    case 5:actors[1].itemaffect=ITEMA_PEACE;break;
    case 6:pcs[1].flag|=FLAG_AFK;break;
    case 7:actors[1].desc=NULL;break;
    case 8:actors[1].act|=ACT_NOKILL;break;
    case 9:actors[0].combat_pause=1;break;
    case 10:actors[0].polyaff=POLY_DRAGONFORM;break;
    case 11:actors[0].polyaff=POLY_CHIMERA;break;
    case 13:SET_BIT(actors[1].affected_by,AFF_WEBBED);break;
    }
    PC_DATA before=pcs[0];
    interpret(actors,command);
    long ki=pcs[0].ninja[NINJA_KI];pcs[0].ninja[NINJA_KI]=before.ninja[NINJA_KI];
    assert(!memcmp(&before,&pcs[0],sizeof before));pcs[0].ninja[NINJA_KI]=ki;
    memset(s,0,sizeof *s);
    long values[]={actors[0].mana,actors[0].wait,ki,actors[0].hit,
        actors[1].hit,actors[1].position,actors[1].affected_by,
        web_calls,actors[0].affected_by,actors[0].position,
        actors[0].primal,actors[0].exp,number_range(1,1000000),0,0,0};
    if(actors[1].affected) {
        values[13]=actors[1].affected->duration;
        values[14]=actors[1].affected->location;
        values[15]=actors[1].affected->modifier;
    }
    memcpy(s->values,values,sizeof values);
    s->text=UINT64_C(1469598103934665603);
    for(int i=0;i<2;i++)for(int n=0;n<descs[i].outtop;n++)
        s->text=(s->text^(unsigned char)output[i][n])*UINT64_C(1099511628211);
}

static uint64_t web_native(int dump)
{
    uint64_t hash=UINT64_C(1469598103934665603);unsigned count=0;
    for(int cls=0;cls<2;cls++)for(int trained=0;trained<2;trained++)
    for(int npc=0;npc<2;npc++)for(int mode=0;mode<14;mode++)for(long ki=9;ki<=10;ki++) {
        web_fixture(cls?CLASS_NINJA:CLASS_DROW,0,trained,trained?8:7,ki,npc);
        WEB_SNAPSHOT s;web_run(mode,&s);char row[1024];
        int used=snprintf(row,sizeof row,"%d,%d,%d,%d,%ld",cls,trained,npc,mode,ki);
        for(int n=0;n<16;n++)used+=snprintf(row+used,sizeof row-(size_t)used,",%ld",s.values[n]);
        used+=snprintf(row+used,sizeof row-(size_t)used,",%" PRIu64 "\n",s.text);
        assert(used>0&&(size_t)used<sizeof row);if(dump)fputs(row,stdout);
        for(int n=0;n<used;n++)hash=(hash^(unsigned char)row[n])*UINT64_C(1099511628211);
        count++;
    }
    assert(count==224);return hash;
}

static void web_pairs(void)
{
    unsigned count=0;
    for(int p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int trained=0;trained<2;trained++)for(int ninja=7;ninja<=8;ninja++)
    for(int npc=0;npc<2;npc++)for(int mode=0;mode<14;mode++)for(long ki=9;ki<=10;ki++) {
        long primary=order?CLASS_DROW:parents[p],secondary=order?parents[p]:CLASS_DROW;
        web_fixture(primary,secondary,trained,ninja,ki,npc);
        WEB_SNAPSHOT actual,expected;web_run(mode,&actual);
        web_fixture(trained||primary!=CLASS_NINJA?CLASS_DROW:primary,0,trained,ninja,ki,npc);
        web_run(mode,&expected);
        if(memcmp(&actual,&expected,sizeof actual)) {
            fprintf(stderr,"web mismatch p=%d order=%d trained=%d ninja=%d npc=%d mode=%d ki=%ld\n",
                p,order,trained,ninja,npc,mode,ki);
            for(int n=0;n<16;n++)if(actual.values[n]!=expected.values[n])
                fprintf(stderr,"field %d actual=%ld expected=%ld\n",n,actual.values[n],expected.values[n]);
            fprintf(stderr,"text actual=%" PRIu64 " expected=%" PRIu64 "\n",actual.text,expected.text);
        }
        assert(!memcmp(&actual,&expected,sizeof actual));count++;
    }
    assert(count==4032);
}

static void web_refusals(void)
{
    for(int bad=0;bad<7;bad++) {
        web_fixture(CLASS_MAGE,CLASS_DROW,1,0,10,1);
        switch(bad) {
        case 0:pcs[0].hybrid_present=2;break;
        case 1:pcs[0].hybrid.secondary_class=CLASS_MONK;break;
        case 2:pcs[0].hybrid.primary_class=CLASS_VAMPIRE;break;
        case 3:pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=DROW_FIRE;break;
        case 4:pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
        case 5:actors[0].act=ACT_IS_NPC;actors[0].pIndexData=&mob;break;
        case 6:pcs[0].hybrid_present=0;pcs[0].drow[DROW_POWERS]=DROW_WEB;break;
        }
        do_web(actors,"beta");assert(!web_calls&&!actors[0].wait);
    }
}

static void web_boundaries(int mode)
{
    web_fixture(mode>=6?CLASS_NINJA:CLASS_DROW,0,1,8,10,1);
    if(mode==1||mode==2||mode==6||mode==7) {
        int action=mode>=6?mode-5:mode;
        CHAR_DATA *ch=actors;
        if(action==2) {
            ch=malloc(sizeof *ch);assert(ch);*ch=actors[0];
            char_list=room.people=ch;descs[0].character=ch;
            actors[0].next=actors[0].next_in_room=NULL;
        }
        web_callback=action;do_web(ch,"beta");
        assert(web_calls==1&&!actors[0].wait);
        if(action==2)assert(!descs[0].character);
        else assert(pcs[0].ninja[NINJA_KI]==10);
    }else if(mode==3) {
        actors[0].in_room=NULL;do_web(actors,"beta");
        assert(!web_calls);actors[0].in_room=&room;
    }else if(mode==4) {
        actors[0].pcdata=NULL;do_web(actors,"beta");
        assert(!web_calls);actors[0].pcdata=pcs;
    }else {
        ROOM_INDEX_DATA elsewhere={0};
        room.people=actors;actors[0].next_in_room=NULL;
        actors[1].in_room=&elsewhere;actors[1].next_in_room=NULL;
        set_fighting(actors,&actors[1]);do_web(actors,"absent");
        assert(!web_calls&&!actors[0].wait);
    }
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){web_native(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--boundary")){web_boundaries(atoi(argv[2]));return 0;}
    if(argc==2&&!strcmp(argv[1],"--secondary-repro")){
        web_fixture(CLASS_MAGE,CLASS_DROW,1,0,10,1);
        do_web(actors,"beta");assert(web_calls==1);return 0;
    }
    /* Fixed against the pre-adapter/native source before any runtime edits. */
    assert(web_native(0)==UINT64_C(13384253252868165408));
    web_pairs();web_refusals();
    for(int mode=1;mode<=7;mode++)web_boundaries(mode);
    do_web(NULL,NULL);
    puts("Web: 224 native snapshots, 4032 paired commands, 7 refusals and 8 context/callback checks passed.");
    return 0;
}
