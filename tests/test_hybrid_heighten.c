/* Real dispatcher, native Heightened Senses and independent Monk Mantra. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main

static const long ht_classes[]={CLASS_MONK,CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,
    CLASS_HIGHLANDER,CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};

static void ht_fixture(long primary,long secondary,long rank,int focus,int grant)
{
    fixture(primary,secondary,rank,focus,0);
    if(grant==1){
        pcs[0].powers[DISC_AUSPEX]=1;pcs[0].breed[BREED_LUPUS]=1;
        pcs[0].ninja[NINJA_SORA]=4;pcs[0].highland[HIGH_ABILITY]=1;
        actors[0].spheres[MMIN]=1;
        SET_BIT(pcs[0].drow[DROW_POWERS],DROW_SIGHT);
        SET_BIT(pcs[0].dragskills,DRAG_TRUESIGHT);
    }
    if(grant==2)SET_BIT(actors[0].itemaffect,ITEMA_VISION);
}

static int ht_native_grant(long cls,long rank,int grant)
{
    if(cls==CLASS_MONK)return rank>=1;
    return grant==1&&(cls==CLASS_DRAGON||cls==CLASS_DROW||cls==CLASS_HIGHLANDER||
        cls==CLASS_MAGE||cls==CLASS_NINJA||cls==CLASS_VAMPIRE||cls==CLASS_WEREWOLF);
}

static void ht_run(CHAR_DATA *ch,int argument,SNAPSHOT *s)
{
    PC_DATA before=*ch->pcdata;long cls=ch->class,primal=ch->primal;
    long extra=ch->extra,va=ch->vampaff,af=ch->affected_by;
    ROOM_INDEX_DATA *where=ch->in_room;
    char input[64];strcpy(input,argument?"heighten beta":"heighten");
    interpret(ch,input);
    assert(!memcmp(&before,ch->pcdata,sizeof before));
    assert(ch->class==cls&&ch->primal==primal&&ch->in_room==where);
    assert(ch->extra==extra&&ch->vampaff==va&&ch->affected_by==af);
    assert(!IS_SET(actors[1].act,PLR_HOLYLIGHT)&&!IS_SET(actors[2].act,PLR_HOLYLIGHT));
    memset(s,0,sizeof *s);
    long values[]={ch->mana,ch->hit,ch->wait,ch->position,ch->act,ch->gold,
        ch->extra,ch->in_room?ch->in_room->vnum:0,actors[1].hit,actors[1].mana,
        area.nplayer,number_range(0,1000000)};
    memcpy(s->values,values,sizeof values);s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<3;n++)s->text=fnv(outputs[n],descs[n].outtop,s->text);
}

static unsigned ht_native(int dump)
{
    unsigned count=0;uint64_t digest=UINT64_C(1469598103934665603);
    for(unsigned c=0;c<10;c++)for(long rank=0;rank<=8;rank++)
    for(int grant=0;grant<3;grant++)for(int active=0;active<2;active++)
    for(int vision=0;vision<2;vision++)for(int argument=0;argument<2;argument++){
        /* Newly safe dismissal after training loss is characterized separately. */
        if(active&&!ht_native_grant(ht_classes[c],rank,grant))continue;
        ht_fixture(ht_classes[c],0,rank,-1,grant);
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        if(vision)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        SNAPSHOT s;ht_run(actors,argument,&s);digest=fnv(&s,sizeof s,digest);
        if(dump){
            printf("%u,%ld,%d,%d,%d,%d",c,rank,grant,active,vision,argument);
            for(unsigned n=0;n<12;n++)printf(",%ld",s.values[n]);
            printf(",%016" PRIx64 "\n",s.text);
        }
        count++;
    }
    assert(count==1428&&digest==UINT64_C(0x4055552fe329d8c3));
    return count;
}

static void ht_pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int focus=-1;focus<3;focus++)for(long rank=0;rank<=8;rank++)
    for(int grant=0;grant<3;grant++)for(int active=0;active<2;active++)
    for(int vision=0;vision<2;vision++)for(int argument=0;argument<2;argument++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        ht_fixture(primary,secondary,rank,focus,grant);
        long effective=hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA);
        assert(effective==UMIN(rank,order||focus==MONK_MANTRA?8:4));
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        if(vision)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        SNAPSHOT a,b;ht_run(actors,argument,&a);
        ht_fixture(effective>=1?CLASS_MONK:primary,0,effective,-1,grant);
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        if(vision)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        ht_run(actors,argument,&b);
        assert(!memcmp(&a,&b,sizeof a));count++;
    }
    assert(count==15552);
}

static void ht_rejections(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int bad=0;bad<6;bad++)for(int native=0;native<2;native++)
    for(int active=0;active<2;active++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        ht_fixture(primary,secondary,1,MONK_MANTRA,native);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.secondary_progression.monk[MONK_MANTRA]=LONG_MAX;break;
        case 4:pcs[0].hybrid.hybrid_id=0;break;
        case 5:pcs[0].hybrid.primary_class=CLASS_NONE;break;
        }
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        SNAPSHOT a,b;ht_run(actors,0,&a);
        ht_fixture(primary,0,1,-1,native);
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        ht_run(actors,0,&b);assert(!memcmp(&a,&b,sizeof a));count++;
    }
    assert(count==432);
}

static void ht_safety(int which)
{
    ht_fixture(CLASS_MONK,0,1,-1,0);
    switch(which){
    case 0:do_truesight(NULL,"");return;
    case 1:actors[0].pcdata=NULL;do_truesight(actors,"");return;
    case 2:do_truesight(actors,NULL);assert(IS_SET(actors[0].act,PLR_HOLYLIGHT));return;
    case 3:SET_BIT(actors[0].act,ACT_IS_NPC);do_truesight(actors,NULL);
        assert(!IS_SET(actors[0].act,PLR_HOLYLIGHT)&&!descs[0].outtop);return;
    case 4:actors[0].in_room=NULL;do_truesight(actors,"");
        assert(IS_SET(actors[0].act,PLR_HOLYLIGHT));return;
    case 5:actors[0].mana=LONG_MIN;do_truesight(actors,"");
        assert(IS_SET(actors[0].act,PLR_HOLYLIGHT)&&actors[0].mana==LONG_MIN);return;
    case 6:actors[0].mana=LONG_MAX;do_truesight(actors,"");
        assert(IS_SET(actors[0].act,PLR_HOLYLIGHT)&&actors[0].mana==LONG_MAX);return;
    }
    assert(0);
}

static void ht_lost(int which)
{
    ht_fixture(ht_classes[which],0,0,-1,0);
    SET_BIT(actors[0].act,PLR_HOLYLIGHT);
    SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
    do_truesight(actors,"");
    assert(!IS_SET(actors[0].act,PLR_HOLYLIGHT)&&strstr(outputs[0],"return to normal"));
    assert(IS_SET(actors[0].extra,EXTRA_SORC_HOLY)&&actors[0].wait==0);
    do_truesight(actors,"");
    assert(!IS_SET(actors[0].act,PLR_HOLYLIGHT));
}

static void ht_timing_and_sources(void)
{
    for(int source=0;source<2;source++){
        ht_fixture(CLASS_SORCERER,CLASS_MONK,1,MONK_ART,0);
        if(source)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        SNAPSHOT s;ht_run(actors,0,&s);
        assert(IS_SET(actors[0].act,PLR_HOLYLIGHT)&&actors[0].wait==6);
        assert(effective_holylight(actors));
        actors[0].wait=0;ht_run(actors,0,&s);
        assert(!IS_SET(actors[0].act,PLR_HOLYLIGHT)&&actors[0].wait==0);
        assert(!!IS_SET(actors[0].extra,EXTRA_SORC_HOLY)==source);
        assert(!!effective_holylight(actors)==source);
        assert(!!effective_shadowsight(actors)==source);
        assert(!!effective_nightsight(actors)==source);
    }
}

static void ht_loaders(void)
{
    char cwd[4096],root[]="/tmp/anathema-heighten-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int focus=0;focus<2;focus++)for(int active=0;active<2;active++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        ht_fixture(primary,secondary,1,focus?MONK_MANTRA:MONK_ART,0);
        HYBRID_STATE state=pcs[0].hybrid;DESCRIPTOR_DATA d={0};
        assert(!load_char_obj(&d,"Heightfixture"));CHAR_DATA *ch=d.character;
        ch->class=primary;ch->level=LEV_AWA;ch->position=POS_STANDING;ch->vampgen=4;
        ch->hit=ch->max_hit=ch->mana=ch->max_mana=50000;ch->in_room=&room;
        ch->primal=1000;ch->pcdata->quest=50000;ch->pcdata->monk[MONK_MANTRA]=order?1:8;
        SET_BIT(ch->extra,EXTRA_SORC_VISION);
        if(active)SET_BIT(ch->act,PLR_HOLYLIGHT);
        assert(hybrid_player_attach(ch,&state)&&save_char_obj_checked(ch));
        ch->desc=NULL;ch->in_room=NULL;free_char(ch);
        for(int brief=0;brief<2;brief++){
            ht_fixture(primary,secondary,1,focus?MONK_MANTRA:MONK_ART,0);
            memset(&d,0,sizeof d);
            assert(brief?load_char_short(&d,"Heightfixture"):load_char_obj(&d,"Heightfixture"));
            ch=d.character;assert(!ch->pcdata->player_load_failed&&ch->class==primary);
            assert(!!IS_SET(ch->act,PLR_HOLYLIGHT)==active);
            ch->in_room=&room;ch->next=ch->next_in_room=&actors[1];
            room.people=char_list=ch;ch->desc=&descs[0];descs[0].character=ch;
            ch->carrying=&relic;relic.carried_by=ch;
            SNAPSHOT s;ht_run(ch,0,&s);
            assert(!!IS_SET(ch->act,PLR_HOLYLIGHT)==!active&&ch->mana==50000);
            assert(IS_SET(ch->extra,EXTRA_SORC_HOLY));
            actors[0].wait=ch->wait=0;ht_run(ch,0,&s);
            assert(!!IS_SET(ch->act,PLR_HOLYLIGHT)==active);
            ch->carrying=NULL;ch->next=ch->next_in_room=NULL;
            ch->desc=NULL;ch->in_room=NULL;free_char(ch);count++;
            char_list=NULL;room.people=NULL;descriptor_list=NULL;
        }
        unlink("../player/Heightfixture");unlink("../backup/Heightfixture");
    }
    assert(count==144&&!fclose(fpReserve)&&!chdir(".."));
    assert(!rmdir("player")&&!rmdir("backup")&&!rmdir("run")&&!chdir(cwd)&&!rmdir(root));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){ht_native(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--safety")){ht_safety(atoi(argv[2]));return 0;}
    if(argc==3&&!strcmp(argv[1],"--lost-rank")){ht_lost(atoi(argv[2]));return 0;}
    ht_native(0);ht_pairs();ht_rejections();
    for(int n=0;n<7;n++)ht_safety(n);
    for(int n=0;n<10;n++)ht_lost(n);
    ht_timing_and_sources();ht_loaders();
    puts("Heighten:1428 native snapshots,15552 paired commands,432 rejected-record cases,10 safe dismissals,"
         "7 context boundaries,2 timing/source cases,144 full/short loads passed.");
    return 0;
}
