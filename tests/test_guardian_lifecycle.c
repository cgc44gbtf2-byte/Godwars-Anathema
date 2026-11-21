/* Reuse the production-API fixtures, not permissive access/codec stubs. */
#define ATOMIC_SAVE_TESTING
#include "atomic_save.h"
#define main guardian_command_suite_main
#include "test_hybrid_guardian.c"
#undef main
#include "guardian.h"

static void disk_boundary(int mode)
{
    char root[]="/tmp/anathema-guardian-life-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    fixture(CLASS_DROW,0,0,-1,0);
    const long keep=MORE_BRISTLES,stale=keep|MORE_GUARDIAN;
    DESCRIPTOR_DATA saved;memset(&saved,0,sizeof(saved));
    if(mode==2){
        assert(!load_char_obj(&saved,"Guardexpiry"));
        CHAR_DATA *ch=saved.character;ch->desc=NULL;ch->class=CLASS_DROW;
        ch->level=LEV_AWA;ch->in_room=&room;ch->more=stale;
        assert(save_char_obj_checked(ch));assert(ch->more==stale);
        atomic_save_test_fault=SAVE_FAULT_RENAME;
        assert(!save_char_obj_checked(ch)&&ch->more==stale);
        atomic_save_test_fault=SAVE_FAULT_NONE;
        ch->in_room=NULL;free_char(ch);saved.character=NULL;
        FILE *fp=fopen("../player/Guardexpiry","r");assert(fp);
        char line[4096];int found=0;
        while(fgets(line,sizeof(line),fp)){
            long more;
            if(sscanf(line,"More %ld",&more)==1){assert(more==keep);found++;}
        }
        assert(found==1&&!fclose(fp));
    }else{
        FILE *fp=fopen("../player/Guardexpiry","w");assert(fp);
        assert(fprintf(fp,"#PLAYER\nName Guardexpiry~\nClass %ld\nLevel 2\n"
            "Room 902\nMore %ld\nEnd\n#END\n",(long)CLASS_DROW,stale)>0&&!fclose(fp));
        assert(mode==3?load_char_obj(&saved,"Guardexpiry"):
                      load_char_short(&saved,"Guardexpiry"));
        CHAR_DATA *ch=saved.character;assert(!ch->pcdata->player_load_failed);
        assert(ch->more==keep);ch->desc=NULL;ch->in_room=NULL;free_char(ch);
    }
    assert(!unlink("../player/Guardexpiry")&&!fclose(fpReserve));fpReserve=NULL;
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("player")&&!rmdir("backup")&&!rmdir("run"));
    assert(!chdir(cwd)&&!rmdir(root));
}

static void lifecycle_boundary(int mode)
{
    if(mode!=1){disk_boundary(mode);return;}
    fixture(CLASS_DROW,CLASS_MONK,6,MONK_MANTRA,0);
    do_spiritguardian(actors,"");assert(IS_MORE(actors,MORE_GUARDIAN));
    pcs[0].hybrid.secondary_progression.full_branch=MONK_ART;
    actors[0].desc=NULL; /* Link-dead originals must not evade revocation. */
    combat_status_update();
    assert(!IS_MORE(actors,MORE_GUARDIAN)&&IS_MORE(actors,MORE_BRISTLES));
}

static unsigned eligibility_cases,pulse_cases,projection_cases,load_cases;
static void check_eligibility(int expected)
{
    CHAR_DATA *ch=actors;PC_DATA before=pcs[0];
    const long more=ch->more,mana=ch->mana,hit=ch->hit;
    assert(!!guardian_eligible(ch)==expected);
    do_spiritguardian(ch,"");
    assert(!!IS_MORE(ch,MORE_GUARDIAN)==expected);
    assert(ch->mana==mana&&ch->hit==hit&&!memcmp(&before,&pcs[0],sizeof(before)));
    const long masks[]={0,MORE_BRISTLES,MORE_GUARDIAN,
        MORE_BRISTLES|MORE_GUARDIAN,LONG_MAX,LONG_MIN,-1};
    for(unsigned i=0;i<sizeof(masks)/sizeof(*masks);i++){
        const long live=ch->more;
        assert(guardian_saved_more(ch,masks[i])==
            (expected?masks[i]:(masks[i]&~MORE_GUARDIAN)));
        assert(ch->more==live&&!memcmp(&before,&pcs[0],sizeof(before)));
        projection_cases++;
    }
    for(int connection=0;connection<4;connection++){
        ch->more=more|MORE_GUARDIAN;
        ch->combat_pause=4;ch->stun_immunity=3;
        if(connection==1)ch->desc=NULL;
        if(connection==2)descs[0].connected=CON_NOTE_TEXT;
        if(connection==3){
            ch->desc=NULL;descs[0].original=ch;
            descs[0].character=&actors[1];actors[1].desc=&descs[0];
            SET_BIT(actors[1].act,ACT_IS_NPC);
        }
        combat_status_update();
        assert(!!IS_MORE(ch,MORE_GUARDIAN)==expected);
        assert(IS_MORE(ch,MORE_BRISTLES)&&ch->combat_pause==3&&ch->stun_immunity==2);
        assert(ch->mana==mana&&ch->hit==hit&&!memcmp(&before,&pcs[0],sizeof(before)));
        ch->more=more;guardian_reconcile(ch);assert(ch->more==more);
        ch->desc=&descs[0];descs[0].connected=CON_PLAYING;
        descs[0].character=ch;descs[0].original=NULL;
        actors[1].act=0;actors[1].desc=&descs[1];pulse_cases++;
    }
    eligibility_cases++;
}

static void eligibility_matrix(void)
{
    const long classes[]={CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
        CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
    for(unsigned p=0;p<10;p++)for(long rank=0;rank<=8;rank++)for(long native=0;native<=5;native++){
        fixture(classes[p],0,rank,-1,native);
        check_eligibility((classes[p]==CLASS_MONK&&rank>=6)||
            ((classes[p]==CLASS_VAMPIRE||classes[p]==CLASS_WEREWOLF)&&native>=4));
    }
    for(unsigned p=0;p<9;p++)for(long focus=-1;focus<=2;focus++)
    for(long rank=0;rank<=8;rank++)for(long native=0;native<=5;native++){
        fixture(parents[p],CLASS_MONK,rank,focus,native);
        check_eligibility((focus==MONK_MANTRA&&rank>=6)||
            ((parents[p]==CLASS_VAMPIRE||parents[p]==CLASS_WEREWOLF)&&native>=4));
    }
    for(unsigned p=0;p<9;p++)for(long rank=0;rank<=8;rank++)for(long native=0;native<=5;native++){
        fixture(CLASS_MONK,parents[p],rank,-1,native);check_eligibility(rank>=6);
    }
    assert(eligibility_cases==2970&&pulse_cases==11880&&projection_cases==20790);
}

static void corrupt_records(void)
{
    for(int native=0;native<4;native++)for(int bad=0;bad<8;bad++){
        long primary=native==1?CLASS_VAMPIRE:native==2?CLASS_WEREWOLF:
            native==3?CLASS_MONK:CLASS_DROW;
        fixture(primary,native==3?CLASS_MAGE:CLASS_MONK,8,MONK_MANTRA,native?4:0);
        HYBRID_STATE *s=&pcs[0].hybrid;
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:s->hybrid_id=LONG_MAX;break;
        case 4:s->primary_class=CLASS_NINJA;break;
        case 5:s->secondary_class=CLASS_NINJA;break;
        case 6:s->secondary_progression.monk[MONK_MANTRA]=9;break;
        case 7:s->secondary_progression.full_branch=LONG_MAX;break;
        }
        SET_BIT(actors[0].more,MORE_GUARDIAN);
        PC_DATA before=pcs[0];guardian_reconcile(actors);
        assert(!!IS_MORE(actors,MORE_GUARDIAN)==!!native);
        assert(IS_MORE(actors,MORE_BRISTLES)&&!memcmp(&before,&pcs[0],sizeof(before)));
    }
    assert(!guardian_eligible(NULL));
    assert(guardian_saved_more(NULL,MORE_GUARDIAN|MORE_BRISTLES)==MORE_BRISTLES);
    guardian_reconcile(NULL);
    fixture(CLASS_MONK,0,8,-1,0);actors[0].pcdata=NULL;
    SET_BIT(actors[0].more,MORE_GUARDIAN);guardian_reconcile(actors);
    assert(!guardian_eligible(actors)&&!IS_MORE(actors,MORE_GUARDIAN));
    fixture(CLASS_MONK,0,8,-1,0);SET_BIT(actors[0].act,ACT_IS_NPC);
    SET_BIT(actors[0].more,MORE_GUARDIAN);guardian_reconcile(actors);
    assert(!guardian_eligible(actors)&&IS_MORE(actors,MORE_GUARDIAN));
}

static void legacy_bit(void)
{
    FILE *in=fopen("../player/Guardround","r"),*copy=tmpfile();assert(in&&copy);
    char line[4096];int found=0;
    while(fgets(line,sizeof(line),in)){
        if(!strncmp(line,"More ",5)){
            assert(fprintf(copy,"More %ld\n",(long)(MORE_GUARDIAN|MORE_BRISTLES))>0);found++;
        }else assert(fputs(line,copy)>=0);
    }
    assert(found==1&&!ferror(in)&&!fclose(in));rewind(copy);
    FILE *out=fopen("../player/Guardround","w");assert(out);size_t n;
    while((n=fread(line,1,sizeof(line),copy)))assert(fwrite(line,1,n,out)==n);
    assert(!ferror(copy)&&!fclose(copy)&&!fclose(out));
}

static void reload_case(int expected,int brief)
{
    DESCRIPTOR_DATA saved;memset(&saved,0,sizeof(saved));
    assert(!load_char_obj(&saved,"Guardround"));
    CHAR_DATA *ch=saved.character;ch->desc=NULL;ch->class=actors[0].class;
    ch->level=LEV_AWA;ch->in_room=&room;ch->more=MORE_BRISTLES|MORE_GUARDIAN;
    memcpy(ch->pcdata->monk,pcs[0].monk,sizeof(pcs[0].monk));
    memcpy(ch->pcdata->powers,pcs[0].powers,sizeof(pcs[0].powers));
    memcpy(ch->pcdata->tribes,pcs[0].tribes,sizeof(pcs[0].tribes));
    if(pcs[0].hybrid_present)assert(hybrid_player_attach(ch,&pcs[0].hybrid));
    assert(save_char_obj_checked(ch)&&IS_MORE(ch,MORE_GUARDIAN));
    ch->in_room=NULL;free_char(ch);saved.character=NULL;
    legacy_bit(); /* Model an older binary/staff edit retaining an invalid flag. */
    assert(brief?load_char_short(&saved,"Guardround"):load_char_obj(&saved,"Guardround"));
    ch=saved.character;assert(!ch->pcdata->player_load_failed);
    assert(!!IS_MORE(ch,MORE_GUARDIAN)==expected&&IS_MORE(ch,MORE_BRISTLES));
    assert(!!guardian_eligible(ch)==expected);
    assert(!memcmp(ch->pcdata->monk,pcs[0].monk,sizeof(pcs[0].monk)));
    if(pcs[0].hybrid_present)assert(!memcmp(&ch->pcdata->hybrid,&pcs[0].hybrid,sizeof(HYBRID_STATE)));
    ch->desc=NULL;ch->in_room=NULL;free_char(ch);assert(!unlink("../player/Guardround"));load_cases++;
}

static void lifecycle_loaders(void)
{
    char root[]="/tmp/anathema-guardian-reloads-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&!chdir(root));
    assert(!mkdir("player",0700)&&!mkdir("backup",0700)&&!mkdir("run",0700)&&!chdir("run"));
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);const long ranks[]={0,5,6,8};
    for(unsigned p=0;p<9;p++)for(long focus=-1;focus<=2;focus++)
    for(unsigned r=0;r<4;r++)for(int native=0;native<2;native++)for(int brief=0;brief<2;brief++){
        fixture(parents[p],CLASS_MONK,ranks[r],focus,native?4:0);
        reload_case((focus==MONK_MANTRA&&ranks[r]>=6)||
            (native&&(parents[p]==CLASS_VAMPIRE||parents[p]==CLASS_WEREWOLF)),brief);
    }
    for(unsigned p=0;p<9;p++)for(long rank=5;rank<=6;rank++)for(int brief=0;brief<2;brief++){
        fixture(CLASS_MONK,parents[p],rank,-1,0);reload_case(rank>=6,brief);
    }
    const long natives[]={CLASS_MONK,CLASS_VAMPIRE,CLASS_WEREWOLF};
    for(unsigned p=0;p<3;p++)for(int trained=0;trained<3;trained++)for(int brief=0;brief<2;brief++){
        fixture(natives[p],0,trained+4,-1,trained+2);reload_case(trained==2,brief);
    }
    assert(load_cases==630&&!fclose(fpReserve));fpReserve=NULL;
    free(string_space);string_space=top_string=NULL;
    assert(!chdir("..")&&!rmdir("player")&&!rmdir("backup")&&!rmdir("run"));
    assert(!chdir(cwd)&&!rmdir(root));
}

int main(int argc,char **argv)
{
    if(argc==3&&!strcmp(argv[1],"--boundary")){
        lifecycle_boundary(atoi(argv[2]));return 0;
    }
    for(int mode=1;mode<=4;mode++)lifecycle_boundary(mode);
    eligibility_matrix();corrupt_records();lifecycle_loaders();
    puts("Guardian lifecycle: 4 old boundaries,2970 command-equivalence cases,"
         "11880 pulses,20790 pure projections,32 corrupt records,630 full/short loads passed.");
    return 0;
}
