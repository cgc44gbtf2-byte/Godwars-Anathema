#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/state.h"
#include "classes/hybrid/specialization.h"

static CHAR_DATA actor;
static PC_DATA player;
static const struct test_tree {
    long owner;
    size_t first, count;
    enum hybrid_progression_kind kind;
    int powers;
} table[]={
    {CLASS_WEREWOLF,0,3,HYBRID_PROGRESS_BREED,0},
    {CLASS_WEREWOLF,0,5,HYBRID_PROGRESS_AUSPICE,0},
    {CLASS_WEREWOLF,0,13,HYBRID_PROGRESS_TRIBE,0},
    {CLASS_VAMPIRE,0,20,HYBRID_PROGRESS_KIND_COUNT,1},
    {CLASS_MAGE,0,9,HYBRID_PROGRESS_SPHERE,0},
    {CLASS_ELEM,1,8,HYBRID_PROGRESS_KIND_COUNT,1},
    {CLASS_SORCERER,0,6,HYBRID_PROGRESS_COLOR,0},
    {CLASS_HIGHLANDER,0,4,HYBRID_PROGRESS_HIGHLAND,0},
    {CLASS_NINJA,0,3,HYBRID_PROGRESS_NINJA,0},
    {CLASS_MONK,0,3,HYBRID_PROGRESS_MONK,0}
};
_Static_assert(sizeof(table)/sizeof(table[0])==HYBRID_TRAIN_TREE_COUNT,"test inventory");

static void fresh(long primary)
{
    memset(&actor,0,sizeof(actor)); memset(&player,0,sizeof(player));
    actor.class=primary; actor.pcdata=&player; actor.level=LEV_AWA;
    actor.clan="Brujah";
    for(size_t i=0;i<20;++i)player.powers[i]=7;
    for(size_t i=0;i<20;++i)player.tribes[i]=5;
    for(size_t i=0;i<5;++i)player.auspice[i]=5;
    for(size_t i=0;i<3;++i){
        player.breed[i]=5; player.tgift[i]=0; player.cland[i]=i;
        player.ninja[i]=10; player.monk[i]=8;
    }
    for(size_t i=0;i<6;++i)player.colors[i]=5;
    for(size_t i=0;i<4;++i)player.highland[i]=6;
    for(size_t i=0;i<9;++i)actor.spheres[i]=5;
    if(primary==CLASS_ELEM){
        player.powers[ELEM_CHOICE]=0;
        for(size_t i=1;i<=8;++i)player.powers[i]=8;
    }
}

static void setup(const HYBRID_DEFINITION *h,long primary,HYBRID_STATE *state)
{
    fresh(primary);
    assert(hybrid_state_prepare(&actor,h,state));
    if(state->secondary_class==CLASS_VAMPIRE){
        assert(hybrid_secondary_vampire_profile(state,CLASS_VAMPIRE,HYBRID_VAMPIRE_STANDARD));
        state->secondary_branches[0]=DISC_ANIMALISM;
        state->secondary_branches[1]=DISC_AUSPEX;
        state->secondary_branches[2]=DISC_THAUMATURGY;
    } else if(state->secondary_class==CLASS_MAGE){
        for(size_t i=0;i<3;++i)state->secondary_branches[i]=i;
    } else if(state->secondary_class==CLASS_WEREWOLF){
        for(size_t i=0;i<3;++i)
            assert(hybrid_secondary_progression_write(state,CLASS_WEREWOLF,
                   HYBRID_PROGRESS_SELECTED_GIFT,i,0));
    }
    assert(hybrid_state_valid(state));
}

/* Independent expected pre-focus native ceiling. */
static long normal_cap(long owner,int tree,size_t index)
{
    switch(owner){
    case CLASS_WEREWOLF:return index==0?5:4;
    case CLASS_VAMPIRE:
        if(index==DISC_THAUMATURGY)return 5;
        return index==DISC_ANIMALISM||index==DISC_AUSPEX?7:5;
    case CLASS_MAGE:return index<3?5:4;
    case CLASS_ELEM:return 8;
    case CLASS_SORCERER:return 5;
    case CLASS_HIGHLANDER:return 6;
    case CLASS_NINJA:return 10;
    case CLASS_MONK:return 8;
    default: (void)tree; assert(0); return 0;
    }
}

static HYBRID_STATE roundtrip(const HYBRID_STATE *state)
{
    HYBRID_STATE loaded;
    long before[HYBRID_PROGRESSION_WORDS],after[HYBRID_PROGRESSION_WORDS];
    FILE *fp=tmpfile();
    assert(fp&&hybrid_state_write(fp,state)&&fflush(fp)==0);
    rewind(fp);
    assert(hybrid_state_read(fp,&loaded)&&fclose(fp)==0);
    assert(hybrid_progression_export(&state->secondary_progression,before,HYBRID_PROGRESSION_WORDS));
    assert(hybrid_progression_export(&loaded.secondary_progression,after,HYBRID_PROGRESSION_WORDS));
    assert(!memcmp(before,after,sizeof(before)));
    assert(!memcmp(state->secondary_powers,loaded.secondary_powers,sizeof(state->secondary_powers)));
    return loaded;
}

static int raw_store(HYBRID_STATE *state,int tree,size_t index,long value)
{
    return table[tree].powers ?
        hybrid_secondary_value_write(state,state->secondary_class,HYBRID_VALUE_POWER,index,value):
        hybrid_secondary_progression_write(state,state->secondary_class,table[tree].kind,index,value);
}

static void every_pair_every_choice(void)
{
    size_t choices=0,checked=0;
    for(size_t pair=0;pair<hybrid_count();++pair){
        const HYBRID_DEFINITION *h=hybrid_at(pair);
        for(size_t order=0;order<2;++order){
            long parent=h->parents[1-order];
            size_t count=hybrid_specialization_count(parent);
            long first=parent==CLASS_ELEM?1:0;
            if(!count){
                HYBRID_STATE state;
                setup(h,h->parents[order],&state);
                assert(!hybrid_secondary_specialize(&state,parent,0));
                assert(hybrid_secondary_ranks_valid(&state));
                continue;
            }
            for(long branch=first;branch<first+(long)count;++branch){
                HYBRID_STATE state,before;
                setup(h,h->parents[order],&state);
                unsigned char a[sizeof(actor)],p[sizeof(player)];
                memcpy(a,&actor,sizeof(a));memcpy(p,&player,sizeof(p));
                assert(state.secondary_progression.full_branch==-1);
                for(int t=0;t<HYBRID_TRAIN_TREE_COUNT;++t){
                    if(table[t].owner!=parent)continue;
                    long cap;
                    long native=normal_cap(parent,t,table[t].first);
                    long ceiling=parent==CLASS_WEREWOLF?3:native/2+native%2;
                    assert(hybrid_parent_training_cap(&actor,&state,parent,
                           (enum hybrid_training_tree)t,table[t].first,&cap));
                    assert(cap==UMIN(native,ceiling));
                }
                assert(hybrid_secondary_specialize(&state,parent,branch));
                assert(hybrid_secondary_specialize(&state,parent,branch));
                memcpy(&before,&state,sizeof(state));
                long different=branch==first?first+1:first;
                assert(!hybrid_secondary_specialize(&state,parent,different));
                assert(!hybrid_secondary_specialize(&state,parent,LONG_MAX));
                assert(!memcmp(&before,&state,sizeof(state)));
                int clipped=0;
                for(int t=0;t<HYBRID_TRAIN_TREE_COUNT;++t){
                    if(table[t].owner!=parent)continue;
                    for(size_t i=table[t].first;i<table[t].first+table[t].count;++i){
                        long native=normal_cap(parent,t,i);
                        long id=parent==CLASS_WEREWOLF?t:(long)i;
                        long base=parent==CLASS_WEREWOLF?5:native;
                        long cap=id==branch?native:UMIN(native,base/2+base%2);
                        long value=-99;
                        assert(hybrid_parent_training_cap(&actor,&state,parent,
                               (enum hybrid_training_tree)t,i,&value)&&value==cap);
                        assert(hybrid_parent_rank_read(&actor,&state,parent,
                               (enum hybrid_training_tree)t,i,&value)&&value==0);
                        assert(hybrid_secondary_rank_write(&state,parent,
                               (enum hybrid_training_tree)t,i,cap));
                        memcpy(&before,&state,sizeof(state));
                        assert(!hybrid_secondary_rank_write(&state,parent,
                               (enum hybrid_training_tree)t,i,cap+1));
                        assert(!hybrid_secondary_rank_write(&state,parent,
                               (enum hybrid_training_tree)t,i,-1));
                        assert(!memcmp(&before,&state,sizeof(state)));
                        assert(hybrid_parent_rank_read(&actor,&state,parent,
                               (enum hybrid_training_tree)t,i,&value)&&value==cap);
                        if(!clipped&&native>cap){
                            assert(raw_store(&state,t,i,native));
                            assert(!hybrid_secondary_ranks_valid(&state));
                            HYBRID_STATE loaded=roundtrip(&state);
                            assert(hybrid_parent_rank_read(&actor,&loaded,parent,
                                   (enum hybrid_training_tree)t,i,&value)&&value==cap);
                            assert(hybrid_secondary_rank_write(&state,parent,
                                   (enum hybrid_training_tree)t,i,cap));
                            clipped=1;
                        }
                        ++checked;
                    }
                }
                assert(clipped&&hybrid_secondary_ranks_valid(&state));
                state=roundtrip(&state);
                assert(hybrid_secondary_ranks_valid(&state));
                assert(!memcmp(a,&actor,sizeof(a))&&!memcmp(p,&player,sizeof(p)));
                ++choices;
            }
            /* The original parent's normal effective access is never halved. */
            HYBRID_STATE state;
            setup(h,h->parents[order],&state);
            for(int t=0;t<HYBRID_TRAIN_TREE_COUNT;++t){
                if(table[t].owner!=actor.class)continue;
                for(size_t i=table[t].first;i<table[t].first+table[t].count;++i){
                    long expected= actor.class==CLASS_WEREWOLF?5 :
                        actor.class==CLASS_VAMPIRE?7 : actor.class==CLASS_MAGE?5 :
                        actor.class==CLASS_ELEM?8 : actor.class==CLASS_SORCERER?5 :
                        actor.class==CLASS_HIGHLANDER?6 : actor.class==CLASS_NINJA?10:8;
                    long rank=-99;
                    assert(hybrid_parent_rank_read(&actor,&state,actor.class,
                           (enum hybrid_training_tree)t,i,&rank)&&rank==expected);
                }
            }
        }
    }
    printf("specialization matrix: %zu choices, %zu checked capped ranks\n",choices,checked);
}

static void profiles_and_rejects(void)
{
    HYBRID_STATE state,before;
    fresh(CLASS_WEREWOLF);
    assert(hybrid_state_prepare(&actor,hybrid_by_parents(CLASS_WEREWOLF,CLASS_VAMPIRE),&state));
    long value=-99;
    assert(!hybrid_secondary_ranks_valid(&state));
    assert(!hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    assert(!hybrid_parent_rank_read(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    assert(!hybrid_secondary_rank_write(&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,1));
    assert(value==-99);
    assert(hybrid_secondary_vampire_profile(&state,CLASS_VAMPIRE,HYBRID_VAMPIRE_CLANLESS));
    assert(hybrid_secondary_specialize(&state,CLASS_VAMPIRE,DISC_THAUMATURGY));
    for(size_t i=0;i<20;++i){
        assert(hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,
               HYBRID_TRAIN_DISCIPLINE,i,&value)&&value==(i==DISC_THAUMATURGY?5:3));
    }
    assert(hybrid_secondary_vampire_profile(&state,CLASS_VAMPIRE,HYBRID_VAMPIRE_CLANLESS));
    memcpy(&before,&state,sizeof(state));
    assert(!hybrid_secondary_vampire_profile(&state,CLASS_VAMPIRE,HYBRID_VAMPIRE_STANDARD));
    assert(!hybrid_secondary_vampire_profile(&state,CLASS_MAGE,HYBRID_VAMPIRE_CLANLESS));
    assert(!memcmp(&before,&state,sizeof(state)));
    state=roundtrip(&state);
    value=-99;
    assert(!hybrid_parent_rank_read(NULL,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    assert(!hybrid_parent_rank_read(&actor,NULL,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    for(int t=-1;t<=HYBRID_TRAIN_TREE_COUNT;++t){
        assert(!hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,
               (enum hybrid_training_tree)t,(size_t)-1,&value));
        assert(!hybrid_secondary_rank_write(&state,CLASS_VAMPIRE,
               (enum hybrid_training_tree)t,(size_t)-1,1));
    }
    assert(value==-99&&!memcmp(&before,&state,sizeof(state)));
    actor.class=CLASS_MAGE;
    assert(!hybrid_parent_rank_read(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    actor.class=state.primary_class;
    SET_BIT(actor.act,ACT_IS_NPC);
    assert(!hybrid_parent_rank_read(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    REMOVE_BIT(actor.act,ACT_IS_NPC);
    actor.pcdata=NULL;
    assert(!hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0,&value));
    actor.pcdata=&player;
    assert(value==-99);
    fresh(CLASS_WEREWOLF);
    assert(hybrid_state_prepare(&actor,hybrid_by_parents(CLASS_WEREWOLF,CLASS_VAMPIRE),&state));
    state.secondary_powers[0]=1;
    memcpy(&before,&state,sizeof(state));
    assert(!hybrid_secondary_vampire_profile(&state,CLASS_VAMPIRE,HYBRID_VAMPIRE_STANDARD));
    assert(!memcmp(&before,&state,sizeof(state)));
    /* Native Caitiff/Panders and Thaumaturgy caps remain unchanged for primary. */
    fresh(CLASS_VAMPIRE);
    assert(hybrid_state_prepare(&actor,hybrid_by_parents(CLASS_WEREWOLF,CLASS_VAMPIRE),&state));
    actor.clan="Caitiff";
    assert(hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,
           DISC_ANIMALISM,&value)&&value==6);
    assert(hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,
           DISC_THAUMATURGY,&value)&&value==5);
    actor.clan="Panders";
    assert(hybrid_parent_training_cap(&actor,&state,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,
           DISC_ANIMALISM,&value)&&value==6);
}

static void elemental_focus(void)
{
    const long opposites[]={0,2,1,4,3,6,5,8,7};
    for(size_t pair=0;pair<hybrid_count();++pair){
        const HYBRID_DEFINITION *h=hybrid_at(pair);
        if(!hybrid_has_parent(h,CLASS_ELEM))continue;
        long other=h->parents[h->parents[0]==CLASS_ELEM?1:0];
        for(long chosen=1;chosen<=8;++chosen){
            HYBRID_STATE state,before;
            setup(h,other,&state);
            assert(hybrid_secondary_specialize(&state,CLASS_ELEM,chosen));
            memcpy(&before,&state,sizeof(state));
            assert(!hybrid_secondary_element_focus(&state,chosen));
            assert(!memcmp(&before,&state,sizeof(state)));
            for(size_t i=1;i<=8;++i)
                assert(hybrid_secondary_rank_write(&state,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,
                       i,i==(size_t)chosen?8:4));
            assert(hybrid_secondary_rank_write(&state,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,chosen,7));
            memcpy(&before,&state,sizeof(state));
            assert(!hybrid_secondary_element_focus(&state,chosen));
            assert(!memcmp(&before,&state,sizeof(state)));
            assert(hybrid_secondary_rank_write(&state,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,chosen,8));
            assert(hybrid_secondary_element_focus(&state,chosen));
            assert(state.secondary_powers[ELEM_CHOICE]==chosen);
            for(size_t i=1;i<=8;++i){
                long expected=i==(size_t)chosen?10:(i==(size_t)opposites[chosen]?3:4);
                long value;
                assert(hybrid_parent_training_cap(&actor,&state,CLASS_ELEM,
                       HYBRID_TRAIN_ELEMENT,i,&value)&&value==expected);
                assert(hybrid_parent_rank_read(&actor,&state,CLASS_ELEM,
                       HYBRID_TRAIN_ELEMENT,i,&value)&&value==expected);
            }
            assert(hybrid_secondary_ranks_valid(&state));
            state=roundtrip(&state);
            HYBRID_STATE inconsistent=state;
            inconsistent.secondary_progression.full_branch=chosen==1?2:1;
            long blocked=-99;
            assert(hybrid_state_valid(&inconsistent));
            assert(!hybrid_secondary_ranks_valid(&inconsistent));
            assert(!hybrid_parent_rank_read(&actor,&inconsistent,CLASS_ELEM,
                   HYBRID_TRAIN_ELEMENT,chosen,&blocked)&&blocked==-99);
            memcpy(&before,&state,sizeof(state));
            assert(!hybrid_secondary_element_focus(&state,chosen));
            assert(!hybrid_secondary_element_focus(&state,0));
            assert(!hybrid_secondary_element_focus(&state,9));
            assert(!memcmp(&before,&state,sizeof(state)));
        }
    }
}

int main(void)
{
    every_pair_every_choice();
    profiles_and_rejects();
    elemental_focus();
    puts("hybrid specialization: eight branched classes, native primary access, "
         "legacy raw-rank clipping, profile guards and 72 elemental focus flows passed");
    return 0;
}
