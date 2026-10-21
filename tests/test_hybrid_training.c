/* Real engine and real atomic player files; no permissive training/save mocks. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/hybrid/build.h"
#include "classes/hybrid/player.h"
#include "classes/hybrid/training.h"

extern char *string_space, *top_string;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *, char *);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static OBJ_INDEX_DATA token;
static size_t quotes, commits, native_comparisons, modeled_steps;

static CHAR_DATA *fresh(long secondary)
{
    memset(&shell,0,sizeof(shell));
    assert(!load_char_obj(&shell,"Unwrittenfixture"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name); ch->name=str_dup("Trainfixture");
    ch->desc=&shell; shell.connected=CON_PLAYING;
    ch->level=LEV_AWA; ch->position=POS_STANDING;
    ch->class=secondary==CLASS_HIGHLANDER?CLASS_NINJA:CLASS_HIGHLANDER;
    ch->hit=ch->max_hit=50000; ch->mana=ch->max_mana=51000;
    ch->primal=100000; ch->exp=100000000; ch->pcdata->quest=50000;
    ch->vampgen=4; ch->pcdata->powers[2]=7;
    ch->pcdata->classpoints[0]=123; ch->pcdata->ninja[0]=9;
    free_string(ch->clan); ch->clan=str_dup("Original Clan");
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,hybrid_by_parents(ch->class,secondary),&state));
    assert(hybrid_player_attach(ch,&state));
    ch->in_room=&room; ch->next=char_list; char_list=ch;
    ch->next_in_room=room.people; room.people=ch;
    obj_to_char(create_object(&token,1),ch);
    return ch;
}
static void release(CHAR_DATA *ch)
{
    CHAR_DATA **link=&char_list;
    while(*link&&*link!=ch)link=&(*link)->next;
    assert(*link==ch);*link=ch->next;
    link=&room.people;
    while(*link&&*link!=ch)link=&(*link)->next_in_room;
    assert(*link==ch);*link=ch->next_in_room;
    ch->next=ch->next_in_room=NULL; ch->desc=NULL; ch->in_room=NULL;
    free_char(ch); shell.character=NULL;
    unlink("../player/Trainfixture");unlink("../backup/Trainfixture");
}
static HYBRID_LESSON_REQUEST request(CHAR_DATA *ch,enum hybrid_lesson_action a,
                                    enum hybrid_training_tree tree,size_t index,long value)
{
    HYBRID_LESSON_REQUEST r={a,ch->pcdata->hybrid.secondary_class,tree,index,value};
    return r;
}
static enum hybrid_lesson_result quote(CHAR_DATA *ch,HYBRID_LESSON_REQUEST r,
                                       HYBRID_LESSON *out)
{
    CHAR_DATA before=*ch; PC_DATA pc=*ch->pcdata;
    HYBRID_LESSON sentinel; memset(&sentinel,0x5a,sizeof(sentinel)); *out=sentinel;
    enum hybrid_lesson_result result=hybrid_training_prepare(ch,&r,out);
    assert(!memcmp(&before,ch,sizeof(before))&&!memcmp(&pc,ch->pcdata,sizeof(pc)));
    if(result!=HYBRID_LESSON_OK)assert(!memcmp(&sentinel,out,sizeof(sentinel)));
    ++quotes;return result;
}
static void primary_same(CHAR_DATA *ch,const CHAR_DATA *before,const PC_DATA *pc)
{
    assert(ch->class==before->class&&ch->vampgen==before->vampgen&&
           ch->clan==before->clan&&ch->hit==before->hit&&ch->max_hit==before->max_hit&&
           ch->mana==before->mana&&ch->max_mana==before->max_mana&&
           !memcmp(ch->spheres,before->spheres,sizeof(ch->spheres)));
    assert(!memcmp(pc->powers,ch->pcdata->powers,sizeof(pc->powers))&&
           !memcmp(pc->classpoints,ch->pcdata->classpoints,sizeof(pc->classpoints))&&
           !memcmp(pc->cland,ch->pcdata->cland,sizeof(pc->cland))&&
           !memcmp(pc->breed,ch->pcdata->breed,sizeof(pc->breed))&&
           !memcmp(pc->auspice,ch->pcdata->auspice,sizeof(pc->auspice))&&
           !memcmp(pc->tribes,ch->pcdata->tribes,sizeof(pc->tribes))&&
           !memcmp(pc->ninja,ch->pcdata->ninja,sizeof(pc->ninja))&&
           !memcmp(pc->monk,ch->pcdata->monk,sizeof(pc->monk))&&
           !memcmp(pc->highland,ch->pcdata->highland,sizeof(pc->highland)));
}
static void commit(CHAR_DATA *ch,HYBRID_LESSON_REQUEST r)
{
    HYBRID_LESSON q;
    assert(quote(ch,r,&q)==HYBRID_LESSON_OK);
    CHAR_DATA before=*ch; PC_DATA pc=*ch->pcdata;
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_OK);
    assert(!memcmp(&q.state,&ch->pcdata->hybrid,sizeof(q.state)));
    assert(ch->primal==q.primal&&ch->exp==q.experience&&ch->pcdata->quest==q.quest);
    primary_same(ch,&before,&pc);
    assert(!(ch->save_pending&PLAYER_SAVE_PRIMARY));
    ++commits;
}
static void reload(CHAR_DATA *ch)
{
    for(int short_load=0;short_load<2;++short_load) {
        DESCRIPTOR_DATA d={0};
        assert(short_load?load_char_short(&d,"Trainfixture"):load_char_obj(&d,"Trainfixture"));
        CHAR_DATA *loaded=d.character;
        assert(!loaded->pcdata->player_load_failed&&loaded->pcdata->hybrid_present);
        assert(!memcmp(&ch->pcdata->hybrid,&loaded->pcdata->hybrid,sizeof(HYBRID_STATE)));
        assert(ch->primal==loaded->primal&&ch->exp==loaded->exp&&
               ch->pcdata->quest==loaded->pcdata->quest&&ch->class==loaded->class);
        if(!short_load)assert(loaded->carrying&&!loaded->carrying->next_content&&
                              loaded->carrying->pIndexData==&token);
        loaded->desc=NULL;loaded->in_room=NULL;free_char(loaded);
    }
}
static void builds_and_choices(void)
{
    assert(hybrid_build_count()==25&&!hybrid_build_at(25)&&!hybrid_build_by_id(-1));
    for(size_t i=0;i<hybrid_build_count();++i) {
        const HYBRID_BUILD_DEFINITION *b=hybrid_build_at(i);
        CHAR_DATA *ch=fresh(b->parent);
        HYBRID_LESSON_REQUEST r=request(ch,HYBRID_LESSON_BUILD,0,0,b->id);
        commit(ch,r); reload(ch);
        assert(ch->pcdata->hybrid.secondary_build==b->id);
        HYBRID_LESSON q;
        assert(quote(ch,r,&q)==HYBRID_LESSON_UNCHANGED);
        r.value=99999;assert(quote(ch,r,&q)==HYBRID_LESSON_INVALID);
        if(b->parent==CLASS_MAGE) {
            if(b->id!=110) {
                r=request(ch,HYBRID_LESSON_SPHERE_SELECT,0,0,SPHERE_MIND);
                assert(quote(ch,r,&q)==HYBRID_LESSON_INVALID);
            } else commit(ch,request(ch,HYBRID_LESSON_SPHERE_SELECT,0,0,SPHERE_MIND));
            long first=ch->pcdata->hybrid.secondary_branches[0];
            r=request(ch,HYBRID_LESSON_SPHERE_SELECT,0,1,first);
            assert(quote(ch,r,&q)==HYBRID_LESSON_INVALID);
            commit(ch,request(ch,HYBRID_LESSON_SPHERE_SELECT,0,1,(first+1)%9));
            commit(ch,request(ch,HYBRID_LESSON_SPHERE_SELECT,0,2,(first+2)%9));
            reload(ch);
        }
        release(ch);
    }
    CHAR_DATA *ch=fresh(CLASS_WEREWOLF);
    for(size_t f=0;f<3;++f) {
        commit(ch,request(ch,HYBRID_LESSON_GIFT_SELECT,0,f,0));
        HYBRID_LESSON q;
        assert(quote(ch,request(ch,HYBRID_LESSON_GIFT_SELECT,0,f,1),&q)==HYBRID_LESSON_INVALID);
    }
    reload(ch);release(ch);
}
static const char *rank_command(enum hybrid_training_tree tree,size_t index)
{
    static const char *breed[]={"homid","lupus","metis"};
    static const char *aus[]={"ragabash","theurge","philodox","galliard","ahroun"};
    static const char *sphere[]={"correspondence learn","life learn","prime learn","entropy learn",
        "mind learn","spirit learn","forces learn","matter learn","time learn"};
    static const char *color[]={"purple","red","blue","green","yellow","black"};
    static const char *high[]={"weaponry","attack","defense","powers"};
    static const char *ninja[]={"sora improve","chikyu improve","ningenno improve"};
    static const char *monk[]={"tech gain","mantra gain","art gain"};
    switch(tree) {
    case HYBRID_TRAIN_BREED:return breed[index];
    case HYBRID_TRAIN_AUSPICE:return aus[index];
    case HYBRID_TRAIN_TRIBE:return "black furies";
    case HYBRID_TRAIN_DISCIPLINE:return "celerity";
    case HYBRID_TRAIN_SPHERE:return sphere[index];
    case HYBRID_TRAIN_ELEMENT:return "fire improve";
    case HYBRID_TRAIN_COLOR:return color[index];
    case HYBRID_TRAIN_HIGHLAND:return high[index];
    case HYBRID_TRAIN_NINJA:return ninja[index];
    case HYBRID_TRAIN_MONK:return monk[index];
    default:assert(0);return "";
    }
}
static void compare_native(CHAR_DATA *ch,HYBRID_LESSON_REQUEST r,const HYBRID_LESSON *q)
{
    /* A separate native actor, never the hybrid or a temporarily swapped
     * primary. Compare actual native handler price/increment at each step. */
    CHAR_DATA native=*ch; PC_DATA pc=*ch->pcdata; native.pcdata=&pc;
    native.desc=NULL;native.class=r.parent;pc.hybrid_present=0;
    memcpy(pc.cland,ch->pcdata->hybrid.secondary_branches,sizeof(pc.cland));
    memcpy(pc.tgift,ch->pcdata->hybrid.secondary_progression.selected_gifts,sizeof(pc.tgift));
    long raw;
    switch(r.tree) {
    case HYBRID_TRAIN_BREED:memcpy(pc.breed,ch->pcdata->hybrid.secondary_progression.breed,sizeof(pc.breed));raw=pc.breed[r.index];break;
    case HYBRID_TRAIN_AUSPICE:memcpy(pc.auspice,ch->pcdata->hybrid.secondary_progression.auspice,sizeof(pc.auspice));raw=pc.auspice[r.index];break;
    case HYBRID_TRAIN_TRIBE:memcpy(pc.tribes,ch->pcdata->hybrid.secondary_progression.tribes,sizeof(pc.tribes));raw=pc.tribes[r.index];break;
    case HYBRID_TRAIN_DISCIPLINE:memcpy(pc.powers,ch->pcdata->hybrid.secondary_powers,sizeof(pc.powers));raw=pc.powers[r.index];break;
    case HYBRID_TRAIN_SPHERE:memcpy(native.spheres,ch->pcdata->hybrid.secondary_progression.spheres,sizeof(native.spheres));raw=native.spheres[r.index];break;
    case HYBRID_TRAIN_ELEMENT:memcpy(pc.powers,ch->pcdata->hybrid.secondary_powers,sizeof(pc.powers));raw=pc.powers[r.index];break;
    case HYBRID_TRAIN_COLOR:memcpy(pc.colors,ch->pcdata->hybrid.secondary_progression.colors,sizeof(pc.colors));raw=pc.colors[r.index];break;
    case HYBRID_TRAIN_HIGHLAND:memcpy(pc.highland,ch->pcdata->hybrid.secondary_progression.highland,sizeof(pc.highland));raw=pc.highland[r.index];break;
    case HYBRID_TRAIN_NINJA:memcpy(pc.ninja,ch->pcdata->hybrid.secondary_progression.ninja,sizeof(pc.ninja));raw=pc.ninja[r.index];break;
    case HYBRID_TRAIN_MONK:memcpy(pc.monk,ch->pcdata->hybrid.secondary_progression.monk,sizeof(pc.monk));raw=pc.monk[r.index];break;
    default:assert(0);return;
    }
    char command[80];snprintf(command,sizeof(command),"%s",rank_command(r.tree,r.index));
    switch(r.tree) {
    case HYBRID_TRAIN_BREED:case HYBRID_TRAIN_AUSPICE:case HYBRID_TRAIN_TRIBE:do_gifts(&native,command);raw=(r.tree==0?pc.breed[r.index]:r.tree==1?pc.auspice[r.index]:pc.tribes[r.index]);break;
    case HYBRID_TRAIN_DISCIPLINE:do_discipline(&native,command);raw=pc.powers[r.index];break;
    case HYBRID_TRAIN_SPHERE:do_spheres(&native,command);raw=native.spheres[r.index];break;
    case HYBRID_TRAIN_ELEMENT:do_element(&native,command);raw=pc.powers[r.index];break;
    case HYBRID_TRAIN_COLOR:do_colors(&native,command);raw=pc.colors[r.index];break;
    case HYBRID_TRAIN_HIGHLAND:do_increase(&native,command);raw=pc.highland[r.index];break;
    case HYBRID_TRAIN_NINJA:do_principles(&native,command);raw=pc.ninja[r.index];break;
    case HYBRID_TRAIN_MONK:do_monklearn(&native,command);raw=pc.monk[r.index];break;
    default:assert(0);
    }
    long next;
    assert(hybrid_parent_rank_read(ch,&q->state,r.parent,r.tree,r.index,&next));
    if(native.primal!=q->primal||raw!=next)
        fprintf(stderr,"native comparison tree=%d index=%zu funds=%ld/%ld rank=%ld/%ld command=%s\n",
            r.tree,r.index,native.primal,q->primal,raw,next,command);
    assert(native.primal==q->primal&&raw==next);
    ++native_comparisons;
}
static void all_rank_caps(void)
{
    static const long parents[]={CLASS_WEREWOLF,CLASS_WEREWOLF,CLASS_WEREWOLF,
        CLASS_VAMPIRE,CLASS_MAGE,CLASS_ELEM,CLASS_SORCERER,CLASS_HIGHLANDER,
        CLASS_NINJA,CLASS_MONK};
    for(int t=0;t<HYBRID_TRAIN_TREE_COUNT;++t) {
        CHAR_DATA *ch=fresh(parents[t]);size_t index=0;
        if(t==HYBRID_TRAIN_DISCIPLINE){commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,3));index=DISC_CELERITY;}
        if(t==HYBRID_TRAIN_SPHERE)commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,109));
        if(t==HYBRID_TRAIN_ELEMENT)index=ELEM_FIRE;
        commit(ch,request(ch,HYBRID_LESSON_SPECIALIZE,0,0,
            t<=HYBRID_TRAIN_TRIBE?t:(long)index));
        if(t<=HYBRID_TRAIN_TRIBE)
            commit(ch,request(ch,HYBRID_LESSON_GIFT_SELECT,0,(size_t)t,(long)index));
        HYBRID_LESSON_REQUEST r=request(ch,HYBRID_LESSON_RANK,(enum hybrid_training_tree)t,index,0);
        HYBRID_LESSON q;long cap;
        assert(hybrid_parent_training_cap(ch,&ch->pcdata->hybrid,r.parent,r.tree,index,&cap));
        for(long n=0;n<cap;++n) {
            assert(quote(ch,r,&q)==HYBRID_LESSON_OK);
            compare_native(ch,r,&q);commit(ch,r);
        }
        assert(quote(ch,r,&q)==HYBRID_LESSON_AT_CAP);
        reload(ch);release(ch);
    }
}
static void funds_and_faults(void)
{
    CHAR_DATA *ch=fresh(CLASS_NINJA);
    HYBRID_LESSON_REQUEST r=request(ch,HYBRID_LESSON_RANK,HYBRID_TRAIN_NINJA,0,0);
    HYBRID_LESSON q;
    ch->primal=9;assert(quote(ch,r,&q)==HYBRID_LESSON_NO_FUNDS);
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_NO_FUNDS&&ch->primal==9);
    ch->primal=100;
    HYBRID_LESSON_REQUEST bad=r;bad.parent=ch->class;
    assert(quote(ch,bad,&q)==HYBRID_LESSON_PRIMARY);
    bad=r;bad.index=SIZE_MAX;assert(quote(ch,bad,&q)==HYBRID_LESSON_INVALID);
    bad=r;bad.tree=(enum hybrid_training_tree)-1;assert(quote(ch,bad,&q)==HYBRID_LESSON_INVALID);
    bad=r;bad.action=(enum hybrid_lesson_action)-1;assert(quote(ch,bad,&q)==HYBRID_LESSON_INVALID);
    assert(save_char_obj_checked(ch));
    static const int faults[]={SAVE_FAULT_CREATE,SAVE_FAULT_FLUSH,SAVE_FAULT_SYNC,
        SAVE_FAULT_CLOSE,SAVE_FAULT_RENAME};
    for(size_t i=0;i<sizeof(faults)/sizeof(faults[0]);++i) {
        HYBRID_STATE old=ch->pcdata->hybrid;long primal=ch->primal;
        atomic_save_test_fault=faults[i];
        assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_SAVE_FAILED);
        assert(ch->primal==primal&&!memcmp(&old,&ch->pcdata->hybrid,sizeof(old)));
        atomic_save_test_fault=SAVE_FAULT_NONE;
        reload(ch);assert(save_char_obj_checked(ch));
    }
    atomic_save_test_fault=SAVE_FAULT_DIRSYNC;
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_SAVE_UNCERTAIN);
    assert(ch->primal==90&&ch->pcdata->hybrid.secondary_progression.ninja[0]==1);
    assert(ch->save_pending&PLAYER_SAVE_PRIMARY);reload(ch);
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_SAVE_PENDING);
    atomic_save_test_fault=SAVE_FAULT_NONE;assert(save_char_obj_checked(ch));
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_OK&&ch->primal==70);
    reload(ch);
    ch->fighting=ch;assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_INVALID);ch->fighting=NULL;
    shell.original=ch;assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_INVALID);shell.original=NULL;
    CHAR_DATA *head=char_list;char_list=ch->next;
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_INVALID);char_list=head;
    shell.connected=CON_GET_NAME;
    assert(hybrid_training_commit(ch,&r)==HYBRID_LESSON_INVALID);shell.connected=CON_PLAYING;
    ch->pcdata->player_load_failed=1;assert(quote(ch,r,&q)==HYBRID_LESSON_INVALID);
    ch->pcdata->player_load_failed=0;release(ch);
}
static void compare_special(CHAR_DATA *ch,HYBRID_LESSON_REQUEST r,const char *command)
{
    HYBRID_LESSON q;assert(quote(ch,r,&q)==HYBRID_LESSON_OK);
    CHAR_DATA native=*ch;PC_DATA pc=*ch->pcdata;
    native.pcdata=&pc;native.desc=NULL;native.class=r.parent;
    native.name="Nativetrain";pc.hybrid_present=0;
    const HYBRID_STATE *s=&ch->pcdata->hybrid;
    memcpy(pc.classpoints,s->secondary_classpoints,sizeof(pc.classpoints));
    pc.rank=s->secondary_progression.wolf_rank;
    native.arete=s->secondary_progression.arete;
    memcpy(pc.drow,s->secondary_progression.drow,sizeof(pc.drow));
    pc.dragskills=s->secondary_progression.dragon_skills;
    native.vampgen=s->secondary_generation;
    char arg[80];snprintf(arg,sizeof(arg),"%s",command);
    switch(r.action) {
    case HYBRID_LESSON_ARETE:do_arete(&native,arg);
        assert(native.arete==q.state.secondary_progression.arete);break;
    case HYBRID_LESSON_WOLF_RANK:do_advance(&native,arg);
        assert(pc.rank==q.state.secondary_progression.wolf_rank);
        /* This comparison actor is a nonhybrid: the actual secondary pays
         * double native primal/XP and needs double native cumulative kills. */
        assert(ch->primal-native.primal==q.primal_cost/2);
        assert(ch->exp-native.exp==q.experience_cost/2);
        native.primal=q.primal;native.exp=q.experience;
        break;
    case HYBRID_LESSON_DROW_POWER:case HYBRID_LESSON_DROW_SPEED:
        do_prayer(&native,arg);
        assert(!memcmp(pc.drow,q.state.secondary_progression.drow,sizeof(pc.drow)));break;
    case HYBRID_LESSON_DRAGON_AGE:case HYBRID_LESSON_DRAGON_SKILL:
        do_dragonskill(&native,arg);
        assert(native.vampgen==q.state.secondary_generation&&
               pc.dragskills==q.state.secondary_progression.dragon_skills);break;
    default:assert(0);
    }
    assert(native.primal==q.primal&&native.exp==q.experience&&pc.quest==q.quest&&
           pc.classpoints[CP_CURRENT]==q.state.secondary_classpoints[CP_CURRENT]);
    unlink("../player/Nativetrain");unlink("../backup/Nativetrain");
    ++native_comparisons;
}
static void special_lessons(void)
{
    HYBRID_LESSON q;
    CHAR_DATA *ch=fresh(CLASS_VAMPIRE);
    assert(quote(ch,request(ch,HYBRID_LESSON_RANK,HYBRID_TRAIN_DISCIPLINE,DISC_PROTEAN,0),&q)==HYBRID_LESSON_CHOICE_REQUIRED);
    commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,3));
    assert(quote(ch,request(ch,HYBRID_LESSON_RANK,HYBRID_TRAIN_DISCIPLINE,DISC_PROTEAN,0),&q)==HYBRID_LESSON_NEEDS_TEACHER);
    assert(quote(ch,request(ch,HYBRID_LESSON_SPECIALIZE,0,0,4),&q)==HYBRID_LESSON_INVALID);
    release(ch);
    ch=fresh(CLASS_MAGE);commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,110));
    HYBRID_LESSON_REQUEST r=request(ch,HYBRID_LESSON_ARETE,0,0,0);
    assert(quote(ch,r,&q)==HYBRID_LESSON_OK&&q.experience_cost==1000000);
    ch->exp=999999;assert(quote(ch,r,&q)==HYBRID_LESSON_NO_FUNDS);ch->exp=100000000;
    compare_special(ch,r,"");
    commit(ch,r);reload(ch);release(ch);
    ch=fresh(CLASS_WEREWOLF);r=request(ch,HYBRID_LESSON_WOLF_RANK,0,0,0);
    assert(quote(ch,r,&q)==HYBRID_LESSON_PREREQUISITE);
    ch->mkill=6000;ch->pcdata->score[SCORE_NUM_QUEST]=0;
    for(int n=1;n<4;++n){compare_special(ch,r,"");commit(ch,r);}
    assert(quote(ch,r,&q)==HYBRID_LESSON_AT_CAP);release(ch);
    ch=fresh(CLASS_DROW);
    r=request(ch,HYBRID_LESSON_DROW_POWER,0,DROW_ACID,0);
    assert(quote(ch,r,&q)==HYBRID_LESSON_NO_FUNDS);
    ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]=1000000;
    assert(quote(ch,r,&q)==HYBRID_LESSON_OK&&q.secondary_dp_cost==5000);
    compare_special(ch,r,"self acidbolt");
    commit(ch,r);assert(quote(ch,r,&q)==HYBRID_LESSON_AT_CAP);
    r.index=DROW_ACID|DROW_WEB;assert(quote(ch,r,&q)==HYBRID_LESSON_INVALID);
    r=request(ch,HYBRID_LESSON_DROW_SPEED,0,0,0);
    for(int n=0;n<4;++n){compare_special(ch,r,"self speed");commit(ch,r);}
    assert(quote(ch,r,&q)==HYBRID_LESSON_AT_CAP);
    static const struct {long bit;const char *name;} drow[]={
        {DROW_WEB,"self llothsweb"},{DROW_DARK,"self darkness"},{DROW_FIRE,"self faeriefire"},
        {DROW_BITE,"self spiderbite"},{DROW_DUALWIELD,"self dualwield"},{DROW_BFIGHT,"self blindfight"},
        {DROW_SPIDER,"self summonspider"},{DROW_EYES,"self spidereyes"},{DROW_READ,"self readaura"},
        {DROW_NIGHT,"self darkvision"},{DROW_SIGHT,"self heighten"},{DROW_PIMM,"self poisonimmunity"},
        {DROW_SKIP,"self skipdisc"},{DROW_ATTUNE,"self attunement"},{DROW_MIGHT,"self might"},
        {DROW_TOUGH,"self toughness"}};
    for(size_t i=0;i<sizeof(drow)/sizeof(drow[0]);++i) {
        r=request(ch,HYBRID_LESSON_DROW_POWER,0,(size_t)drow[i].bit,0);
        compare_special(ch,r,drow[i].name);commit(ch,r);
    }
    reload(ch);release(ch);
    ch=fresh(CLASS_DRAGON);commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,201));
    ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]=1000000;
    r=request(ch,HYBRID_LESSON_DRAGON_SKILL,0,DRAG_FANGS,0);
    assert(quote(ch,r,&q)==HYBRID_LESSON_PREREQUISITE);
    r=request(ch,HYBRID_LESSON_DRAGON_AGE,0,0,0);
    long paid=0;
    for(int age=12;age>=3;--age) {
        if(age==AGE_YADULT) {
            ch->pcdata->quest=499;
            assert(quote(ch,r,&q)==HYBRID_LESSON_NO_FUNDS);
            ch->pcdata->quest=50000;
        }
        assert(quote(ch,r,&q)==HYBRID_LESSON_OK);
        paid+=q.secondary_dp_cost;compare_special(ch,r,"age");commit(ch,r);
    }
    assert(paid==320000&&ch->pcdata->quest==39500);
    assert(quote(ch,r,&q)==HYBRID_LESSON_AT_CAP);
    static const struct {long bit;const char *name;} dragon[]={
        {DRAG_FANGS,"fangs"},{DRAG_NIGHT,"nightsight"},{DRAG_CLAWS,"claws"},
        {DRAG_HORNS,"horns"},{DRAG_WINGS,"wings"},{DRAG_BREATH,"breath"},{DRAG_TAIL,"tail"},
        {DRAG_TRUESIGHT,"truesight"},{DRAG_DUST,"dust"},{DRAG_MAGIC,"magic"},
        {DRAG_SCRY,"scry"},{DRAG_READAURA,"readaura"},{DRAG_SHIELD,"shield"}};
    for(size_t i=0;i<sizeof(dragon)/sizeof(dragon[0]);++i) {
        r=request(ch,HYBRID_LESSON_DRAGON_SKILL,0,(size_t)dragon[i].bit,0);
        compare_special(ch,r,dragon[i].name);commit(ch,r);
    }
    reload(ch);release(ch);
}
static void v3_migration(void)
{
    CHAR_DATA *ch=fresh(CLASS_MAGE);HYBRID_STATE loaded;
    FILE *fp=tmpfile();assert(fp&&hybrid_state_write(fp,&ch->pcdata->hybrid));
    rewind(fp);char line[4096];assert(fgets(line,sizeof(line),fp));
    line[strlen("AnathemaHybrid ")]='3';
    char *last=strrchr(line,' ');assert(last);*last='\n';last[1]='\0';
    rewind(fp);assert(ftruncate(fileno(fp),0)==0&&fputs(line,fp)>=0);rewind(fp);
    assert(hybrid_state_read(fp,&loaded)&&loaded.secondary_build==0);
    /* No automatic clan inference/reset on older trained Vampire records. */
    fclose(fp);release(ch);
    ch=fresh(CLASS_VAMPIRE);commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,3));
    commit(ch,request(ch,HYBRID_LESSON_RANK,HYBRID_TRAIN_DISCIPLINE,DISC_CELERITY,0));
    fp=tmpfile();assert(fp&&hybrid_state_write(fp,&ch->pcdata->hybrid));
    rewind(fp);assert(fgets(line,sizeof(line),fp));
    line[strlen("AnathemaHybrid ")]='3';last=strrchr(line,' ');
    assert(last);*last='\n';last[1]='\0';
    rewind(fp);assert(ftruncate(fileno(fp),0)==0&&fputs(line,fp)>=0);rewind(fp);
    assert(hybrid_state_read(fp,&loaded)&&loaded.secondary_build==0&&
           loaded.secondary_powers[DISC_CELERITY]==1);
    ch->pcdata->hybrid=loaded;
    HYBRID_LESSON q;
    assert(quote(ch,request(ch,HYBRID_LESSON_RANK,HYBRID_TRAIN_DISCIPLINE,DISC_CELERITY,0),&q)==HYBRID_LESSON_CHOICE_REQUIRED);
    assert(quote(ch,request(ch,HYBRID_LESSON_BUILD,0,0,3),&q)==HYBRID_LESSON_INVALID);
    fclose(fp);release(ch);
}
static void all_pairs_model(void)
{
    /* Exercise every applicable ranked slot in all 90 parent orientations.
     * Candidate steps are modeled in memory here; real disk transactions and
     * native-handler comparisons are independently covered above. */
    static const long owners[]={CLASS_WEREWOLF,CLASS_WEREWOLF,CLASS_WEREWOLF,
        CLASS_VAMPIRE,CLASS_MAGE,CLASS_ELEM,CLASS_SORCERER,CLASS_HIGHLANDER,
        CLASS_NINJA,CLASS_MONK};
    static const size_t counts[]={3,5,TRIBE_WENDIGOS+1,20,9,8,6,4,3,3};
    size_t orientations=0;
    for(size_t n=0;n<hybrid_count();++n)for(int order=0;order<2;++order) {
        const HYBRID_DEFINITION *def=hybrid_at(n);
        long parent=def->parents[1-order];
        CHAR_DATA *ch=fresh(parent);
        ch->class=def->parents[order];
        HYBRID_STATE initialized;
        assert(hybrid_secondary_initialize(ch,def,&initialized));
        ch->pcdata->hybrid=initialized;
        long build=parent==CLASS_VAMPIRE?1:parent==CLASS_MAGE?101:parent==CLASS_DRAGON?201:0;
        if(build)commit(ch,request(ch,HYBRID_LESSON_BUILD,0,0,build));
        if(hybrid_specialization_count(parent)) {
            long full=parent==CLASS_ELEM?ELEM_FIRE:parent==CLASS_VAMPIRE?DISC_DOMINATE:
                parent==CLASS_MAGE?SPHERE_MIND:0;
            commit(ch,request(ch,HYBRID_LESSON_SPECIALIZE,0,0,full));
        }
        if(parent==CLASS_WEREWOLF)for(size_t f=0;f<3;++f)
            commit(ch,request(ch,HYBRID_LESSON_GIFT_SELECT,0,f,0));
        CHAR_DATA before=*ch;PC_DATA pc=*ch->pcdata;
        for(int t=0;t<HYBRID_TRAIN_TREE_COUNT;++t)if(owners[t]==parent)
            for(size_t k=0;k<counts[t];++k) {
                size_t index=k+(parent==CLASS_ELEM?ELEM_FIRE:0);
                HYBRID_LESSON_REQUEST r=request(ch,HYBRID_LESSON_RANK,
                    (enum hybrid_training_tree)t,index,0);
                HYBRID_LESSON q;long cap;
                enum hybrid_lesson_result result=quote(ch,r,&q);
                if(parent==CLASS_VAMPIRE&&(index==4||index==8||index==16||index==19)) {
                    assert(result==HYBRID_LESSON_INVALID);continue;
                }
                if(result==HYBRID_LESSON_NEEDS_TEACHER)continue;
                assert(hybrid_parent_training_cap(ch,&ch->pcdata->hybrid,parent,r.tree,index,&cap));
                long expected=parent==CLASS_WEREWOLF?(t==0?(index==0?5:4):3):
                    parent==CLASS_VAMPIRE?(index==DISC_DOMINATE?7:
                        (index==DISC_FORTITUDE||index==DISC_PRESENCE?4:3)):
                    parent==CLASS_MAGE?(index==SPHERE_MIND?5:2):
                    parent==CLASS_ELEM?(index==ELEM_FIRE?8:4):
                    parent==CLASS_SORCERER?(index==0?5:3):
                    parent==CLASS_HIGHLANDER?(index==0?6:3):
                    parent==CLASS_NINJA?(index==0?10:5):(index==0?8:4);
                assert(cap==expected);
                for(long rank=0;rank<cap;++rank) {
                    assert(quote(ch,r,&q)==HYBRID_LESSON_OK);
                    ch->pcdata->hybrid=q.state;ch->primal=q.primal;
                    ch->exp=q.experience;ch->pcdata->quest=q.quest;
                    ++modeled_steps;
                }
                assert(quote(ch,r,&q)==HYBRID_LESSON_AT_CAP);
            }
        primary_same(ch,&before,&pc);
        assert(save_char_obj_checked(ch));reload(ch);
        release(ch);++orientations;
    }
    assert(orientations==90);
}
int main(void)
{
    char root[]="/tmp/anathema-hybrid-training-XXXXXX",cwd[4096];
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(mkdir("player",0700)==0&&mkdir("backup",0700)==0&&mkdir("run",0700)==0&&chdir("run")==0);
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    current_time=123456;init_mm();fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    /* Prototypes normally own boot-time string-pool strings, not literals or
     * independently freed heap strings shared by multiple object instances. */
    token.vnum=5001;token.name=top_string;strcpy(top_string,"training token");
    top_string+=strlen(top_string)+1;
    token.short_descr=top_string;strcpy(top_string,"a training token");
    top_string+=strlen(top_string)+1;
    token.description=top_string;strcpy(top_string,"A training token is here.");
    top_string+=strlen(top_string)+1;token.item_type=ITEM_TREASURE;
    token.wear_flags=ITEM_TAKE;token.weight=1;
    obj_index_hash[5001%MAX_KEY_HASH]=&token;
    builds_and_choices();all_rank_caps();funds_and_faults();special_lessons();v3_migration();all_pairs_model();
    assert(!char_list&&!room.people&&fclose(fpReserve)==0&&chdir("..")==0);
    assert(rmdir("player")==0&&rmdir("backup")==0&&rmdir("run")==0&&chdir(cwd)==0&&rmdir(root)==0);
    printf("hybrid training: %zu pure quotes, %zu durable commits, %zu native-handler comparisons, %zu modeled rank steps; "
        "25 builds, choices/caps/costs, real reloads and atomic failure/uncertainty passed\n",
        quotes,commits,native_comparisons,modeled_steps);
    return 0;
}
