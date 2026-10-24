/* Real interpreter, native Scry, learned Monk records and remote room views. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main

static ROOM_INDEX_DATA remote_room;
static AREA_DATA remote_area;
static MOB_INDEX_DATA remote_mob;
static const long scry_classes[]={CLASS_MONK,CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,
    CLASS_HIGHLANDER,CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
static const char *scry_commands[]={"scry beta","scry","scry alpha","scry missing",
    "scry beta","scry","scry beta","scry beta","scry beta","scry beta",
    "scry beta","scry beta"};

static void scry_fixture(long primary,long secondary,long rank,int focus,int native)
{
    fixture(primary,secondary,rank,focus,0);
    memset(&remote_room,0,sizeof remote_room);memset(&remote_area,0,sizeof remote_area);
    memset(&remote_mob,0,sizeof remote_mob);
    remote_room.vnum=902;remote_room.light=1;remote_room.area=&remote_area;
    remote_room.name="Remote scry fixture";remote_room.description="An empty stone chamber.\n\r";
    remote_room.people=&actors[1];remote_area.nplayer=1;area.nplayer=2;
    room_index_hash[902%MAX_KEY_HASH]=&remote_room;
    actors[1].in_room=&remote_room;actors[1].next_in_room=NULL;
    actors[0].next_in_room=&actors[2];
    if(native==1||native==3){
        pcs[0].powers[DISC_AUSPEX]=native==3?6:3;
        pcs[0].breed[BREED_LUPUS]=native==3?5:2;
        pcs[0].breed[BREED_HOMID]=native==3?5:0;
        pcs[0].ninja[NINJA_SORA]=4;actors[0].spheres[MMIN]=2;
        pcs[0].drow[DROW_POWERS]|=DROW_EYES;pcs[0].dragskills|=DRAG_SCRY;
    }
    if(native==2)SET_BIT(actors[0].itemaffect,ITEMA_VISION);
}
static void scry_run(CHAR_DATA *ch,int mode,SNAPSHOT *s)
{
    if(mode==4||mode==5){
        SET_BIT(actors[1].more,MORE_SHIELDED|MORE_GUARDIAN);
        actors[1].class=CLASS_MONK;pcs[1].monk[MONK_MANTRA]=6;
    }
    if(mode==6)actors[1].level=LEV_BUI;
    if(mode==7||mode==9)SET_BIT(ch->affected_by,AFF_SHADOWPLANE);
    if(mode==8||mode==9)SET_BIT(actors[1].affected_by,AFF_SHADOWPLANE);
    if(mode==9||mode==10)SET_BIT(ch->pcdata->drow[DROW_POWERS],DROW_GLOBE);
    if(mode==11){
        SET_BIT(actors[1].act,ACT_IS_NPC);actors[1].pIndexData=&remote_mob;
        remote_mob.vnum=902;remote_mob.player_name="beta";
    }
    PC_DATA before=*ch->pcdata;
    ROOM_INDEX_DATA *where=ch->in_room;long af=ch->affected_by,cls=ch->class,primal=ch->primal;
    char command[64];strcpy(command,scry_commands[mode]);interpret(ch,command);
    assert(!memcmp(&before,ch->pcdata,sizeof before));
    assert(ch->in_room==where&&ch->affected_by==af&&ch->class==cls&&ch->primal==primal);
    assert(room.people==ch&&remote_room.people==&actors[1]);
    assert(ch->next_in_room==&actors[2]&&area.nplayer==2&&remote_area.nplayer==1);
    memset(s,0,sizeof *s);
    long values[]={ch->mana,ch->hit,ch->wait,ch->position,ch->more,ch->gold,
        ch->extra,ch->in_room?ch->in_room->vnum:0,actors[1].hit,actors[1].mana,
        area.nplayer+remote_area.nplayer,number_range(0,1000000)};
    memcpy(s->values,values,sizeof values);s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<3;n++)s->text=fnv(outputs[n],descs[n].outtop,s->text);
}
static unsigned scry_native(int dump)
{
    unsigned count=0;
    for(unsigned p=0;p<10;p++)for(int native=0;native<4;native++)
    for(int mode=0;mode<12;mode++)for(int m=0;m<3;m++){
        scry_fixture(scry_classes[p],0,native==1||native==3?2:0,-1,native);
        actors[0].mana=mana_values[m];SNAPSHOT s;scry_run(actors,mode,&s);count++;
        if(dump){
            printf("%u,%d,%d,%d",p,native,mode,m);
            for(unsigned i=0;i<12;i++)printf(",%ld",s.values[i]);
            printf(",%016" PRIx64 "\n",s.text);
        }
    }
    assert(count==1440);return count;
}
static void scry_pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int focus=-1;focus<3;focus++)for(long rank=0;rank<=8;rank++)
    for(int mode=0;mode<12;mode++)for(int m=0;m<3;m++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        scry_fixture(primary,secondary,rank,focus,0);actors[0].mana=mana_values[m];
        long effective=hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA);
        assert(effective==UMIN(rank,order||focus==MONK_MANTRA?8:4));
        SNAPSHOT actual,expected;scry_run(actors,mode,&actual);
        scry_fixture(effective>=2?CLASS_MONK:primary,0,effective,-1,0);
        actors[0].mana=mana_values[m];scry_run(actors,mode,&expected);
        assert(!memcmp(&actual,&expected,sizeof actual));count++;
    }
    assert(count==23328);
}
static void scry_bad_records(void)
{
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)for(int bad=0;bad<5;bad++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        scry_fixture(primary,secondary,2,MONK_MANTRA,0);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.secondary_progression.monk[MONK_MANTRA]=LONG_MAX;break;
        case 4:pcs[0].hybrid.hybrid_id=0;break;
        }
        SNAPSHOT a,b;scry_run(actors,0,&a);
        scry_fixture(primary,0,2,-1,0);scry_run(actors,0,&b);
        assert(!memcmp(&a,&b,sizeof a));
    }
    for(unsigned p=0;p<9;p++){
        scry_fixture(parents[p],CLASS_MONK,0,MONK_TECH,1);
        SNAPSHOT a,b;scry_run(actors,0,&a);
        scry_fixture(parents[p],0,0,-1,1);scry_run(actors,0,&b);
        assert(!memcmp(&a,&b,sizeof a));
    }
    scry_fixture(CLASS_MAGE,CLASS_MONK,2,MONK_ART,0);
    actors[0].mana=LONG_MIN;SNAPSHOT s;scry_run(actors,0,&s);
    assert(strstr(outputs[0],"Remote scry fixture")&&actors[0].mana==LONG_MIN);
    scry_fixture(CLASS_WEREWOLF,CLASS_MONK,2,MONK_ART,3);
    scry_run(actors,4,&s);assert(!strstr(outputs[1],"Scry"));
    scry_fixture(CLASS_MAGE,CLASS_MONK,2,MONK_ART,0);
    scry_run(actors,4,&s);assert(strstr(outputs[0],"unable to locate"));
    assert(strstr(outputs[1],"failed to use Scry"));
    scry_fixture(CLASS_SORCERER,CLASS_MONK,2,MONK_ART,0);
    scry_run(actors,6,&s);assert(strstr(outputs[0],"may not scry the Gods"));
}
static void scry_safety(int which)
{
    scry_fixture(CLASS_MONK,0,2,-1,0);
    if(which==0){do_scry(NULL,"beta");return;}
    if(which==1){actors[0].pcdata=NULL;do_scry(actors,"beta");return;}
    if(which==2){actors[0].in_room=NULL;do_scry(actors,"beta");return;}
    if(which==3){do_scry(actors,NULL);assert(strstr(outputs[0],"mentally survey"));return;}
    char huge[2*MAX_STRING_LENGTH+1];memset(huge,'x',sizeof huge-1);huge[sizeof huge-1]='\0';
    if(which==4){actors[1].morph=huge;SET_BIT(actors[1].affected_by,AFF_POLYMORPH);}
    if(which==5)remote_room.name=huge;
    if(which==6){actors[1].morph=NULL;SET_BIT(actors[1].affected_by,AFF_POLYMORPH);}
    if(which==7){
        actors[1].in_room=NULL;do_scry(actors,"beta");assert(actors[0].in_room==&room);
        assert(strstr(outputs[0],"unable to locate"));return;
    }
    if(which==8)remote_room.name=NULL;
    do_scry(actors,"");
    assert(actors[0].in_room==&room);
    if(which==4||which==6)assert(strstr(outputs[0],": "));
    if(which==5||which==8)assert(strstr(outputs[0],".\n\r"));
    if(which==6)assert(strstr(outputs[0],"Someone: "));
    if(which==8)assert(strstr(outputs[0],"(unknown)."));
}
static void scry_loaders(void)
{
    char cwd[4096],root[]="/tmp/anathema-scry-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)for(int full=0;full<2;full++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        scry_fixture(primary,secondary,2,full?MONK_MANTRA:MONK_TECH,0);
        HYBRID_STATE state=pcs[0].hybrid;DESCRIPTOR_DATA d={0};
        assert(!load_char_obj(&d,"Scryfixture"));CHAR_DATA *ch=d.character;
        ch->class=primary;ch->level=LEV_AWA;ch->position=POS_STANDING;
        ch->hit=ch->max_hit=ch->mana=ch->max_mana=50000;ch->vampgen=4;ch->in_room=&room;
        ch->pcdata->monk[MONK_MANTRA]=order?2:8;ch->pcdata->quest=50000;
        assert(hybrid_player_attach(ch,&state)&&save_char_obj_checked(ch));
        ch->desc=NULL;ch->in_room=NULL;free_char(ch);
        for(int brief=0;brief<2;brief++){
            scry_fixture(primary,secondary,2,full?MONK_MANTRA:MONK_TECH,0);memset(&d,0,sizeof d);
            assert(brief?load_char_short(&d,"Scryfixture"):load_char_obj(&d,"Scryfixture"));
            ch=d.character;assert(!ch->pcdata->player_load_failed&&ch->class==primary);
            ch->in_room=&room;ch->next=&actors[1];ch->next_in_room=&actors[2];
            char_list=room.people=ch;ch->desc=&descs[0];descs[0].character=ch;
            long mana=ch->mana;SNAPSHOT s;scry_run(ch,0,&s);
            assert(strstr(outputs[0],"Remote scry fixture")&&ch->mana==mana);
            ch->next=ch->next_in_room=NULL;ch->desc=NULL;ch->in_room=NULL;
            free_char(ch);char_list=NULL;room.people=NULL;descriptor_list=NULL;count++;
        }
        unlink("../player/Scryfixture");unlink("../backup/Scryfixture");
    }
    assert(count==72&&!fclose(fpReserve)&&!chdir(".."));
    assert(!rmdir("player")&&!rmdir("backup")&&!rmdir("run")&&!chdir(cwd)&&!rmdir(root));
}
int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){scry_native(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--safety")){scry_safety(atoi(argv[2]));return 0;}
    scry_native(0);scry_pairs();scry_bad_records();
    for(int i=0;i<9;i++)scry_safety(i);
    scry_loaders();
    puts("Scry:1440 native snapshots,23328 paired commands,90 rejected-record cases,"
         "9 independent native grants,72 full/short loads and9 safety boundaries passed.");
    return 0;
}
