/* Secondary-only specialization policy. Not connected to live class gates. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../merc.h"
#include "state.h"
#include "specialization.h"

struct tree_spec {
    long owner;
    size_t first, count;
    enum hybrid_progression_kind progression;
    int powers;
};
static const struct tree_spec trees[] = {
    {CLASS_WEREWOLF,0,3,HYBRID_PROGRESS_BREED,0},
    {CLASS_WEREWOLF,0,5,HYBRID_PROGRESS_AUSPICE,0},
    {CLASS_WEREWOLF,0,TRIBE_WENDIGOS+1,HYBRID_PROGRESS_TRIBE,0},
    {CLASS_VAMPIRE,0,DISC_MAX+1,HYBRID_PROGRESS_KIND_COUNT,1},
    {CLASS_MAGE,0,9,HYBRID_PROGRESS_SPHERE,0},
    {CLASS_ELEM,ELEM_FIRE,8,HYBRID_PROGRESS_KIND_COUNT,1},
    {CLASS_SORCERER,0,6,HYBRID_PROGRESS_COLOR,0},
    {CLASS_HIGHLANDER,0,4,HYBRID_PROGRESS_HIGHLAND,0},
    {CLASS_NINJA,0,3,HYBRID_PROGRESS_NINJA,0},
    {CLASS_MONK,0,3,HYBRID_PROGRESS_MONK,0}
};
_Static_assert(sizeof(trees)/sizeof(trees[0]) == HYBRID_TRAIN_TREE_COUNT,
               "training tree inventory");

size_t hybrid_specialization_count(long parent)
{
    switch (parent) {
    case CLASS_WEREWOLF: case CLASS_NINJA: case CLASS_MONK: return 3;
    case CLASS_VAMPIRE: return DISC_MAX+1;
    case CLASS_MAGE: return 9;
    case CLASS_ELEM: return 8;
    case CLASS_SORCERER: return 6;
    case CLASS_HIGHLANDER: return 4;
    default: return 0;
    }
}

int hybrid_specialization_branch_valid(long parent, long branch)
{
    long first = parent == CLASS_ELEM ? ELEM_FIRE : 0;
    size_t count = hybrid_specialization_count(parent);
    return count && branch >= first && branch - first < (long)count;
}

static const struct tree_spec *find_tree(long parent, enum hybrid_training_tree tree,
                                         size_t index)
{
    if ((unsigned int)tree >= HYBRID_TRAIN_TREE_COUNT) return NULL;
    const struct tree_spec *t = &trees[tree];
    if (t->owner != parent || index < t->first || index - t->first >= t->count)
        return NULL;
    return t;
}

static int actor_matches(const CHAR_DATA *ch, const HYBRID_STATE *state, long parent)
{
    return ch && !IS_NPC(ch) && ch->pcdata && hybrid_state_valid(state) &&
           ch->class == state->primary_class &&
           (parent == state->primary_class || parent == state->secondary_class);
}

static long opposite_element(long element)
{
    switch (element) {
    case ELEM_FIRE: return ELEM_WATER;
    case ELEM_WATER: return ELEM_FIRE;
    case ELEM_EARTH: return ELEM_WIND;
    case ELEM_WIND: return ELEM_EARTH;
    case ELEM_SUN: return ELEM_MOON;
    case ELEM_MOON: return ELEM_SUN;
    case ELEM_TIME: return ELEM_INFIN;
    case ELEM_INFIN: return ELEM_TIME;
    default: return 0;
    }
}

static int native_cap(const CHAR_DATA *ch, const HYBRID_STATE *state, long parent,
                      enum hybrid_training_tree tree, size_t index, long *cap)
{
    int primary = parent == state->primary_class;
    const HYBRID_PROGRESSION *p = &state->secondary_progression;
    switch (tree) {
    case HYBRID_TRAIN_BREED: case HYBRID_TRAIN_AUSPICE: case HYBRID_TRAIN_TRIBE:
        *cap = (primary ? ch->pcdata->tgift[tree] : p->selected_gifts[tree]) ==
               (long)index ? 5 : 4;
        break;
    case HYBRID_TRAIN_DISCIPLINE: {
        const long *branches = primary ? ch->pcdata->cland : state->secondary_branches;
        long profile = p->vampire_profile;
        if (primary) profile = ch->clan &&
            (!str_cmp(ch->clan,"Caitiff") || !str_cmp(ch->clan,"Panders")) ?
            HYBRID_VAMPIRE_CLANLESS : HYBRID_VAMPIRE_STANDARD;
        if (profile == HYBRID_VAMPIRE_UNINITIALIZED) return 0;
        *cap = profile == HYBRID_VAMPIRE_CLANLESS ? 6 : 5;
        for (size_t i=0;i<3;++i)
            if (branches[i] == (long)index && profile != HYBRID_VAMPIRE_CLANLESS)
                *cap = 7;
        if (index == DISC_THAUMATURGY) *cap = 5;
        break;
    }
    case HYBRID_TRAIN_SPHERE: {
        const long *branches = primary ? ch->pcdata->cland : state->secondary_branches;
        *cap = 4;
        for (size_t i=0;i<3;++i) if (branches[i] == (long)index) *cap = 5;
        break;
    }
    case HYBRID_TRAIN_ELEMENT: {
        long choice = primary ? ch->pcdata->powers[ELEM_CHOICE] :
                                state->secondary_powers[ELEM_CHOICE];
        if (choice && !hybrid_specialization_branch_valid(CLASS_ELEM,choice)) return 0;
        *cap = choice == (long)index ? 10 :
               (choice && opposite_element(choice) == (long)index ? 6 : 8);
        break;
    }
    case HYBRID_TRAIN_COLOR: *cap=5; break;
    case HYBRID_TRAIN_HIGHLAND: *cap=6; break;
    case HYBRID_TRAIN_NINJA: *cap=10; break;
    case HYBRID_TRAIN_MONK: *cap=8; break;
    default: return 0;
    }
    return 1;
}

static int secondary_cap(const HYBRID_STATE *state, enum hybrid_training_tree tree,
                         size_t index, long *destination)
{
    long native, parent = state->secondary_class;
    if (!find_tree(parent,tree,index) ||
        !native_cap(NULL,state,parent,tree,index,&native)) return 0;
    if (parent == CLASS_ELEM && state->secondary_powers[ELEM_CHOICE] &&
        state->secondary_progression.full_branch != state->secondary_powers[ELEM_CHOICE])
        return 0;
    long branch = parent == CLASS_WEREWOLF ? (long)tree : (long)index;
    /* WW's selected FAMILY has advertised cap 5; its individual nonselected
     * gifts still respect their lower native ceiling 4. Other tree paths
     * use their own clan/affinity-sensitive native ceiling. */
    long advertised = parent == CLASS_WEREWOLF ? 5 : native;
    long ceiling = state->secondary_progression.full_branch == branch ?
                   advertised : advertised/2 + advertised%2;
    *destination = UMIN(native,ceiling);
    return 1;
}

int hybrid_secondary_specialize(HYBRID_STATE *state, long parent, long branch)
{
    HYBRID_STATE candidate;
    if (!hybrid_state_valid(state) || state->secondary_class != parent ||
        !hybrid_specialization_branch_valid(parent,branch)) return 0;
    if (parent == CLASS_ELEM && state->secondary_powers[ELEM_CHOICE] &&
        branch != state->secondary_powers[ELEM_CHOICE]) return 0;
    if (state->secondary_progression.full_branch != -1)
        return state->secondary_progression.full_branch == branch;
    memcpy(&candidate,state,sizeof(candidate));
    candidate.secondary_progression.full_branch=branch;
    if (!hybrid_state_valid(&candidate)) return 0;
    memcpy(state,&candidate,sizeof(*state));
    return 1;
}

int hybrid_secondary_vampire_profile(HYBRID_STATE *state,long parent,
                                    enum hybrid_vampire_profile profile)
{
    HYBRID_STATE candidate;
    if (!hybrid_state_valid(state) || parent != CLASS_VAMPIRE ||
        state->secondary_class != parent ||
        (profile != HYBRID_VAMPIRE_STANDARD && profile != HYBRID_VAMPIRE_CLANLESS))
        return 0;
    if (state->secondary_progression.vampire_profile)
        return state->secondary_progression.vampire_profile == (long)profile;
    /* Initial setup only. Nonempty legacy banks require an explicit migration,
     * not a silent reclassification or destruction of their stored training. */
    for(size_t i=0;i<20;++i) if(state->secondary_powers[i]) return 0;
    memcpy(&candidate,state,sizeof(candidate));
    candidate.secondary_progression.vampire_profile=profile;
    if(!hybrid_state_valid(&candidate))return 0;
    memcpy(state,&candidate,sizeof(*state));
    return 1;
}

int hybrid_parent_training_cap(const CHAR_DATA *ch,const HYBRID_STATE *state,
                              long parent,enum hybrid_training_tree tree,
                              size_t index,long *destination)
{
    long cap;
    if(!destination || !find_tree(parent,tree,index) ||
       !actor_matches(ch,state,parent))return 0;
    int ok = parent == state->primary_class ?
        native_cap(ch,state,parent,tree,index,&cap) :
        secondary_cap(state,tree,index,&cap);
    if(!ok)return 0;
    *destination=cap;
    return 1;
}

int hybrid_parent_rank_read(const CHAR_DATA *ch,const HYBRID_STATE *state,
                           long parent,enum hybrid_training_tree tree,
                           size_t index,long *destination)
{
    const struct tree_spec *t=find_tree(parent,tree,index);
    long rank, cap;
    if(!destination || !t || !actor_matches(ch,state,parent))return 0;
    if(parent == state->primary_class){
        /* Delegate to the real native effective getter, not training caps.
         * Original legacy over-cap training remains exactly as before. */
        CHAR_DATA *native=(CHAR_DATA *)ch;
        switch(tree){
        case HYBRID_TRAIN_BREED: rank=get_breed(native,index);break;
        case HYBRID_TRAIN_AUSPICE: rank=get_auspice(native,index);break;
        case HYBRID_TRAIN_TRIBE: rank=get_tribe(native,index);break;
        case HYBRID_TRAIN_DISCIPLINE: rank=get_disc(native,index);break;
        case HYBRID_TRAIN_SPHERE: rank=ch->spheres[index];break;
        case HYBRID_TRAIN_ELEMENT: rank=ch->pcdata->powers[index];break;
        case HYBRID_TRAIN_COLOR: rank=get_color(native,index);break;
        case HYBRID_TRAIN_HIGHLAND: rank=get_highland(native,index);break;
        case HYBRID_TRAIN_NINJA: rank=get_ninja(native,index);break;
        case HYBRID_TRAIN_MONK: rank=get_monk(native,index);break;
        default:return 0;
        }
    } else{
        if(!secondary_cap(state,tree,index,&cap))return 0;
        if(t->powers) rank=state->secondary_powers[index];
        else if(!hybrid_parent_progression_read(ch,state,parent,t->progression,index,&rank))
            return 0;
        rank=UMIN(rank,cap);
    }
    *destination=rank;
    return 1;
}

int hybrid_secondary_rank_write(HYBRID_STATE *state,long parent,
                                enum hybrid_training_tree tree,size_t index,long value)
{
    long cap;
    const struct tree_spec *t=find_tree(parent,tree,index);
    if(!t || !hybrid_state_valid(state) || state->secondary_class != parent ||
       value < 0 || !secondary_cap(state,tree,index,&cap) || value > cap)return 0;
    if(t->powers)
        return hybrid_secondary_value_write(state,parent,HYBRID_VALUE_POWER,index,value);
    return hybrid_secondary_progression_write(state,parent,t->progression,index,value);
}

int hybrid_secondary_ranks_valid(const HYBRID_STATE *state)
{
    if(!hybrid_state_valid(state))return 0;
    for(int tree=0;tree<HYBRID_TRAIN_TREE_COUNT;++tree){
        const struct tree_spec *t=&trees[tree];
        if(t->owner != state->secondary_class)continue;
        for(size_t i=t->first;i<t->first+t->count;++i){
            long rank,cap;
            if(!secondary_cap(state,(enum hybrid_training_tree)tree,i,&cap))return 0;
            if(t->powers)rank=state->secondary_powers[i];
            else if(!hybrid_progression_record_read(state->secondary_class,
                &state->secondary_progression,t->progression,i,&rank))return 0;
            if(rank < 0 || rank > cap)return 0;
        }
    }
    return 1;
}

int hybrid_secondary_element_focus(HYBRID_STATE *state,long element)
{
    HYBRID_STATE candidate;
    if(!hybrid_state_valid(state) || state->secondary_class != CLASS_ELEM ||
       !hybrid_specialization_branch_valid(CLASS_ELEM,element) ||
       state->secondary_powers[ELEM_CHOICE] != 0 ||
       state->secondary_progression.full_branch != element)return 0;
    for(size_t i=ELEM_FIRE;i<=ELEM_INFIN;++i){
        long cap;
        if(!secondary_cap(state,HYBRID_TRAIN_ELEMENT,i,&cap) ||
           state->secondary_powers[i] != cap)return 0;
    }
    memcpy(&candidate,state,sizeof(candidate));
    candidate.secondary_powers[ELEM_CHOICE]=element;
    candidate.secondary_powers[element]=10;
    candidate.secondary_powers[opposite_element(element)]=3;
    if(!hybrid_state_valid(&candidate) || !hybrid_secondary_ranks_valid(&candidate))return 0;
    memcpy(state,&candidate,sizeof(*state));
    return 1;
}
