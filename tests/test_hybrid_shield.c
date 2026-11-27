/* Real dispatcher, independent Monk grant, shared shield and saved records. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main

static const long shield_classes[]={CLASS_MONK,CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,
    CLASS_HIGHLANDER,CLASS_MAGE,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
static const char *shield_commands[]={"shield","mindblock","shield beta"};

static void shield_fixture(long primary,long secondary,long rank,int focus,int native)
{
    fixture(primary,secondary,rank,focus,0);
    if(native==1){
        pcs[0].powers[DISC_OBFUSCATE]=6;
        pcs[0].tribes[TRIBE_SILVER_FANGS]=4;
        actors[0].spheres[MMIN]=3;
        SET_BIT(pcs[0].dragskills,DRAG_SHIELD);
    }
    if(native==2)SET_BIT(actors[0].itemaffect,ITEMA_VISION);
}

static int native_shield(long cls,long rank,int grant)
{
    if(cls==CLASS_MONK)return rank>=2;
    return grant==1&&(cls==CLASS_DRAGON||cls==CLASS_MAGE||
        cls==CLASS_VAMPIRE||cls==CLASS_WEREWOLF);
}

static void shield_run(CHAR_DATA *ch,int command,SNAPSHOT *s)
{
    PC_DATA before=*ch->pcdata;
    long cls=ch->class,primal=ch->primal,af=ch->affected_by,va=ch->vampaff;
    ROOM_INDEX_DATA *where=ch->in_room;
    char input[64];strcpy(input,shield_commands[command]);
    interpret(ch,input);
    assert(!memcmp(&before,ch->pcdata,sizeof before));
    assert(ch->class==cls&&ch->primal==primal&&ch->in_room==where);
    assert(ch->affected_by==af&&ch->vampaff==va&&ch->carrying==&relic);
    assert(actors[1].more==0&&actors[2].more==0); /* Arguments do not target others. */
    memset(s,0,sizeof *s);
    long values[]={ch->mana,ch->hit,ch->wait,ch->position,ch->more,ch->gold,
        ch->extra,ch->in_room?ch->in_room->vnum:0,actors[1].hit,actors[1].mana,
        area.nplayer,number_range(0,1000000)};
    memcpy(s->values,values,sizeof values);s->text=UINT64_C(1469598103934665603);
    for(int n=0;n<3;n++)s->text=fnv(outputs[n],descs[n].outtop,s->text);
}

static unsigned shield_native(int dump)
{
    unsigned count=0;
    uint64_t digest=UINT64_C(1469598103934665603);
    for(unsigned c=0;c<10;c++)for(long rank=0;rank<=8;rank++)
    for(int grant=0;grant<3;grant++)for(int active=0;active<2;active++)
    for(int command=0;command<3;command++){
        /* Removal after loss of eligibility is a separately tested safety fix. */
        if(active&&!native_shield(shield_classes[c],rank,grant))continue;
        shield_fixture(shield_classes[c],0,rank,-1,grant);
        if(active)SET_BIT(actors[0].more,MORE_SHIELDED);
        SNAPSHOT s;shield_run(actors,command,&s);
        digest=fnv(&s,sizeof s,digest);
        if(dump){
            printf("%u,%ld,%d,%d,%d",c,rank,grant,active,command);
            for(unsigned n=0;n<12;n++)printf(",%ld",s.values[n]);
            printf(",%016" PRIx64 "\n",s.text);
        }
        count++;
    }
    assert(count==981&&digest==UINT64_C(0xbe4caaeb550f75e7));
    return count;
}

static void shield_pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int focus=-1;focus<3;focus++)for(long rank=0;rank<=8;rank++)
    for(int native=0;native<3;native++)for(int active=0;active<2;active++)
    for(int command=0;command<3;command++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        shield_fixture(primary,secondary,rank,focus,native);
        long effective=hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA);
        assert(effective==UMIN(rank,order||focus==MONK_MANTRA?8:4));
        if(active)SET_BIT(actors[0].more,MORE_SHIELDED);
        SNAPSHOT a,b;shield_run(actors,command,&a);
        shield_fixture(effective>=2?CLASS_MONK:primary,0,effective,-1,native);
        if(active)SET_BIT(actors[0].more,MORE_SHIELDED);
        shield_run(actors,command,&b);
        assert(!memcmp(&a,&b,sizeof a));count++;
    }
    assert(count==11664);
}

static void shield_rejection_and_cleanup(void)
{
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int bad=0;bad<6;bad++)for(int native=0;native<2;native++)
    for(int active=0;active<2;active++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        shield_fixture(primary,secondary,2,MONK_MANTRA,native);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.secondary_progression.monk[MONK_MANTRA]=LONG_MAX;break;
        case 4:pcs[0].hybrid.hybrid_id=0;break;
        case 5:pcs[0].hybrid.primary_class=CLASS_NONE;break;
        }
        if(active)SET_BIT(actors[0].more,MORE_SHIELDED);
        SNAPSHOT a,b;shield_run(actors,0,&a);
        shield_fixture(primary,0,2,-1,native);
        if(active)SET_BIT(actors[0].more,MORE_SHIELDED);
        shield_run(actors,0,&b);
        assert(!memcmp(&a,&b,sizeof a));
    }
    for(unsigned c=0;c<10;c++){
        shield_fixture(shield_classes[c],0,0,-1,0);
        SET_BIT(actors[0].more,MORE_SHIELDED);
        do_shield(actors,"");
        assert(!IS_MORE(actors,MORE_SHIELDED)&&strstr(outputs[0],"stop shielding"));
        do_shield(actors,"");
        assert(!IS_MORE(actors,MORE_SHIELDED)); /* Cleanup is not an activation grant. */
    }
}

static void shield_boundaries(int which)
{
    shield_fixture(CLASS_MONK,0,2,-1,0);
    switch(which){
    case 0:do_shield(NULL,"");return;
    case 1:actors[0].pcdata=NULL;do_shield(actors,"");return;
    case 2:do_shield(actors,NULL);assert(IS_MORE(actors,MORE_SHIELDED));return;
    case 3:SET_BIT(actors[0].act,ACT_IS_NPC);do_shield(actors,NULL);
        assert(!actors[0].more&&!descs[0].outtop);return;
    case 4:actors[0].in_room=NULL;do_shield(actors,"");
        assert(IS_MORE(actors,MORE_SHIELDED));return;
    case 5:actors[0].mana=LONG_MIN;do_shield(actors,"");
        assert(IS_MORE(actors,MORE_SHIELDED)&&actors[0].mana==LONG_MIN);return;
    case 6:actors[0].mana=LONG_MAX;do_shield(actors,"");
        assert(IS_MORE(actors,MORE_SHIELDED)&&actors[0].mana==LONG_MAX);return;
    }
    assert(0);
}

static void shield_lost_rank(int which)
{
    assert(which>=0&&which<10);
    shield_fixture(shield_classes[which],0,0,-1,0);
    SET_BIT(actors[0].more,MORE_SHIELDED);
    do_shield(actors,"");
    assert(!IS_MORE(actors,MORE_SHIELDED)&&strstr(outputs[0],"stop shielding"));
}

static void shield_consumers(void)
{
    for(int command=0;command<2;command++)for(int vision=0;vision<2;vision++){
        shield_fixture(CLASS_MONK,0,2,-1,0);
        actors[1].class=CLASS_MONK;pcs[1].monk[MONK_MANTRA]=2;
        char toggle[]="shield";interpret(&actors[1],toggle);
        assert(IS_MORE(&actors[1],MORE_SHIELDED));
        if(vision)SET_BIT(actors[0].itemaffect,ITEMA_VISION);
        char request[32];strcpy(request,command?"scry beta":"readaura beta");
        interpret(actors,request);
        if(command&&!vision)assert(strstr(outputs[0],"unable to locate"));
        if(command&&vision)assert(strstr(outputs[0],"Private aura fixture"));
        if(!command)assert(strstr(outputs[0],"Hp:")); /* Native Read Aura is not blocked. */
        assert(IS_MORE(&actors[1],MORE_SHIELDED));
    }
}

static void shield_loaders(void)
{
    char cwd[4096],root[]="/tmp/anathema-shield-XXXXXX";
    assert(getcwd(cwd,sizeof cwd)&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int full=0;full<2;full++)for(int active=0;active<2;active++){
        long primary=order?CLASS_MONK:parents[p],secondary=order?parents[p]:CLASS_MONK;
        shield_fixture(primary,secondary,2,full?MONK_MANTRA:MONK_TECH,0);
        HYBRID_STATE state=pcs[0].hybrid;DESCRIPTOR_DATA d={0};
        assert(!load_char_obj(&d,"Shieldfixture"));CHAR_DATA *ch=d.character;
        ch->class=primary;ch->level=LEV_AWA;ch->position=POS_STANDING;ch->vampgen=4;
        ch->hit=ch->max_hit=ch->mana=ch->max_mana=50000;ch->in_room=&room;
        ch->primal=1000;ch->pcdata->quest=50000;
        ch->pcdata->monk[MONK_MANTRA]=order?2:8;
        if(active)SET_BIT(ch->more,MORE_SHIELDED);
        assert(hybrid_player_attach(ch,&state)&&save_char_obj_checked(ch));
        ch->desc=NULL;ch->in_room=NULL;free_char(ch);
        for(int brief=0;brief<2;brief++){
            shield_fixture(primary,secondary,2,full?MONK_MANTRA:MONK_TECH,0);
            memset(&d,0,sizeof d);
            assert(brief?load_char_short(&d,"Shieldfixture"):load_char_obj(&d,"Shieldfixture"));
            ch=d.character;assert(!ch->pcdata->player_load_failed&&ch->class==primary);
            assert(!!IS_MORE(ch,MORE_SHIELDED)==active);
            ch->in_room=&room;ch->next=ch->next_in_room=&actors[1];
            room.people=char_list=ch;ch->desc=&descs[0];descs[0].character=ch;
            ch->carrying=&relic;relic.carried_by=ch;
            SNAPSHOT s;shield_run(ch,0,&s);
            assert(!!IS_MORE(ch,MORE_SHIELDED)==!active&&ch->mana==50000);
            shield_run(ch,1,&s);assert(!!IS_MORE(ch,MORE_SHIELDED)==active);
            ch->carrying=NULL;ch->next=ch->next_in_room=NULL;
            ch->desc=NULL;ch->in_room=NULL;free_char(ch);count++;
            char_list=NULL;room.people=NULL;descriptor_list=NULL;
        }
        unlink("../player/Shieldfixture");unlink("../backup/Shieldfixture");
    }
    assert(count==144&&!fclose(fpReserve)&&!chdir(".."));
    assert(!rmdir("player")&&!rmdir("backup")&&!rmdir("run")&&!chdir(cwd)&&!rmdir(root));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){shield_native(1);return 0;}
    if(argc==3&&!strcmp(argv[1],"--safety")){shield_boundaries(atoi(argv[2]));return 0;}
    if(argc==3&&!strcmp(argv[1],"--lost-rank")){shield_lost_rank(atoi(argv[2]));return 0;}
    if(argc==2&&!strcmp(argv[1],"--cleanup")){shield_rejection_and_cleanup();return 0;}
    shield_native(0);shield_pairs();shield_rejection_and_cleanup();
    for(int n=0;n<7;n++)shield_boundaries(n);
    shield_consumers();shield_loaders();
    puts("Shield:981 native snapshots,11664 paired commands,432 rejected-record cases,"
         "10 safe dismissals,7 context boundaries,4 consumer cases,144 full/short loads passed.");
    return 0;
}
