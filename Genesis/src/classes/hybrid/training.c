/* Parent-aware native training candidates and atomic single-player commits.
 * No ancestry masks, primary-field swapping or free rank learning. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "build.h"
#include "player.h"
#include "training.h"
#include "../werewolf/advancement.h"

struct unlock { long flag, cost, age; };
static int valid_discipline(long value)
{
    switch(value) {
    case DISC_ANIMALISM:case DISC_AUSPEX:case DISC_CELERITY:case DISC_CHIMERSTRY:
    case DISC_DOMINATE:case DISC_FORTITUDE:case DISC_NECROMANCY:
    case DISC_OBFUSCATE:case DISC_OBTENEBRATION:case DISC_POTENCE:
    case DISC_PRESENCE:case DISC_PROTEAN:case DISC_QUIETUS:case DISC_SERPENTIS:
    case DISC_THAUMATURGY:case DISC_VICISSITUDE:return 1;
    default:return 0;
    }
}
static const struct unlock drow_powers[] = {
    {DROW_ACID,5000,0},{DROW_WEB,8000,0},{DROW_DARK,9500,0},
    {DROW_FIRE,5000,0},{DROW_BITE,7500,0},{DROW_DUALWIELD,10000,0},
    {DROW_BFIGHT,5000,0},{DROW_SPIDER,8000,0},{DROW_EYES,7500,0},
    {DROW_READ,6000,0},{DROW_NIGHT,5000,0},{DROW_SIGHT,20000,0},
    {DROW_PIMM,5000,0},{DROW_SKIP,5000,0},{DROW_ATTUNE,1000,0},
    {DROW_MIGHT,8000,0},{DROW_TOUGH,8000,0}
};
static const struct unlock dragon_skills[] = {
    {DRAG_FANGS,1000,AGE_VYOUNG},{DRAG_NIGHT,1000,AGE_VYOUNG},
    {DRAG_CLAWS,2000,AGE_YOUNG},{DRAG_HORNS,2000,AGE_YOUNG},
    {DRAG_WINGS,3000,AGE_JUVENILE},{DRAG_BREATH,3000,AGE_JUVENILE},
    {DRAG_TAIL,4000,AGE_YADULT},{DRAG_TRUESIGHT,5000,AGE_ADULT},
    {DRAG_DUST,6000,AGE_MATURE},{DRAG_MAGIC,7000,AGE_OLD},
    {DRAG_SCRY,7000,AGE_OLD},{DRAG_READAURA,8000,AGE_VOLD},
    {DRAG_SHIELD,9000,AGE_ANCIENT}
};

static long raw_rank(const HYBRID_STATE *s,enum hybrid_training_tree tree,size_t i)
{
    const HYBRID_PROGRESSION *p=&s->secondary_progression;
    switch(tree) {
    case HYBRID_TRAIN_BREED:return p->breed[i];
    case HYBRID_TRAIN_AUSPICE:return p->auspice[i];
    case HYBRID_TRAIN_TRIBE:return p->tribes[i];
    case HYBRID_TRAIN_DISCIPLINE:case HYBRID_TRAIN_ELEMENT:return s->secondary_powers[i];
    case HYBRID_TRAIN_SPHERE:return p->spheres[i];
    case HYBRID_TRAIN_COLOR:return p->colors[i];
    case HYBRID_TRAIN_HIGHLAND:return p->highland[i];
    case HYBRID_TRAIN_NINJA:return p->ninja[i];
    case HYBRID_TRAIN_MONK:return p->monk[i];
    default:return -2;
    }
}

static enum hybrid_lesson_result ranked(const CHAR_DATA *ch,
    const HYBRID_LESSON_REQUEST *r,HYBRID_LESSON *lesson)
{
    long cap, rank, multiplier=10;
    HYBRID_STATE *s=&lesson->state;
    if((s->secondary_class==CLASS_VAMPIRE||s->secondary_class==CLASS_MAGE) &&
       !s->secondary_build)return HYBRID_LESSON_CHOICE_REQUIRED;
    /* Validate class/tree/index BEFORE any direct array access or arithmetic. */
    if(!hybrid_parent_training_cap(ch,s,r->parent,r->tree,r->index,&cap))
        return HYBRID_LESSON_INVALID;
    if(r->tree==HYBRID_TRAIN_DISCIPLINE &&
       !valid_discipline((long)r->index))return HYBRID_LESSON_INVALID;
    rank=raw_rank(s,r->tree,r->index);
    if(rank>=cap)return HYBRID_LESSON_AT_CAP;
    if(s->secondary_class==CLASS_ELEM && s->secondary_powers[ELEM_CHOICE])
        return HYBRID_LESSON_AT_CAP; /* Native focus ends basic training. */
    if(r->tree==HYBRID_TRAIN_DISCIPLINE) {
        /* Native zero means unavailable until a legitimate teaching route
         * grants learning. Unknown -1 is the normal first paid rank1. */
        if(rank==0)return HYBRID_LESSON_NEEDS_TEACHER;
        multiplier=15;
        for(size_t i=0;i<3;++i)
            if(s->secondary_branches[i]==(long)r->index)multiplier=5;
    } else if(r->tree<=HYBRID_TRAIN_TRIBE) {
        multiplier=s->secondary_progression.selected_gifts[r->tree]==(long)r->index?10:15;
    } else if(r->tree==HYBRID_TRAIN_HIGHLAND)multiplier=15;
    if(rank < (r->tree==HYBRID_TRAIN_DISCIPLINE?-1:0))
        return HYBRID_LESSON_INVALID;
    long next=rank<1?1:rank+1; /* Cap is bounded by the native tree descriptor. */
    lesson->primal_cost=next*multiplier;
    if(!hybrid_secondary_rank_write(s,r->parent,r->tree,r->index,next))
        return HYBRID_LESSON_INVALID;
    return HYBRID_LESSON_OK;
}

static enum hybrid_lesson_result unlock_lesson(HYBRID_LESSON *lesson,
    const HYBRID_LESSON_REQUEST *r,int dragon)
{
    HYBRID_STATE *s=&lesson->state;
    const struct unlock *list=dragon?dragon_skills:drow_powers;
    size_t count=dragon?sizeof(dragon_skills)/sizeof(dragon_skills[0]):
        sizeof(drow_powers)/sizeof(drow_powers[0]);
    const struct unlock *u=NULL;
    if(s->secondary_class!=(dragon?CLASS_DRAGON:CLASS_DROW))return HYBRID_LESSON_INVALID;
    for(size_t i=0;i<count;++i)if(r->index==(size_t)list[i].flag)u=&list[i];
    if(!u)return HYBRID_LESSON_INVALID; /* Reject combined and transient bits. */
    if(dragon&&!s->secondary_build)return HYBRID_LESSON_CHOICE_REQUIRED;
    if(dragon&&s->secondary_generation>u->age)return HYBRID_LESSON_PREREQUISITE;
    long *bits=dragon?&s->secondary_progression.dragon_skills:
        &s->secondary_progression.drow[DROW_POWERS];
    if(*bits&u->flag)return HYBRID_LESSON_AT_CAP;
    *bits|=u->flag;
    lesson->secondary_dp_cost=u->cost;
    return HYBRID_LESSON_OK;
}

enum hybrid_lesson_result hybrid_training_prepare(const CHAR_DATA *ch,
    const HYBRID_LESSON_REQUEST *r,HYBRID_LESSON *destination)
{
    if(!r||!destination||!hybrid_player_save_valid(ch)||
       !ch->pcdata->hybrid_present)return HYBRID_LESSON_INVALID;
    if(r->parent==ch->class)return HYBRID_LESSON_PRIMARY;
    if(r->parent!=ch->pcdata->hybrid.secondary_class)return HYBRID_LESSON_INVALID;
    HYBRID_LESSON lesson={0};
    memcpy(&lesson.state,&ch->pcdata->hybrid,sizeof(lesson.state));
    HYBRID_STATE *s=&lesson.state;
    HYBRID_PROGRESSION *p=&s->secondary_progression;
    enum hybrid_lesson_result result=HYBRID_LESSON_OK;
    int selected=0;
    switch(r->action) {
    case HYBRID_LESSON_BUILD:
        selected=hybrid_secondary_build_choose(s,r->value);break;
    case HYBRID_LESSON_GIFT_SELECT:
        selected=hybrid_secondary_gift_select(s,r->index,r->value);break;
    case HYBRID_LESSON_SPHERE_SELECT:
        selected=hybrid_secondary_sphere_select(s,r->index,r->value);break;
    case HYBRID_LESSON_SPECIALIZE:
        if(r->parent==CLASS_VAMPIRE&&!valid_discipline(r->value))
            return HYBRID_LESSON_INVALID;
        selected=hybrid_secondary_specialize(s,r->parent,r->value);break;
    case HYBRID_LESSON_ELEMENT_FOCUS:
        selected=hybrid_secondary_element_focus(s,r->value);break;
    case HYBRID_LESSON_RANK:
        result=ranked(ch,r,&lesson);break;
    case HYBRID_LESSON_ARETE:
        if(r->parent!=CLASS_MAGE)return HYBRID_LESSON_INVALID;
        if(!s->secondary_build)return HYBRID_LESSON_CHOICE_REQUIRED;
        if(p->arete>=10)return HYBRID_LESSON_AT_CAP;
        if(p->arete<1)return HYBRID_LESSON_PREREQUISITE;
        lesson.experience_cost=(p->arete+1)*500000;
        ++p->arete;break;
    case HYBRID_LESSON_WOLF_RANK:
        if(r->parent!=CLASS_WEREWOLF)return HYBRID_LESSON_INVALID;
        if(p->wolf_rank>=4)return HYBRID_LESSON_AT_CAP;
        {
            long kills;
            if(!werewolf_advancement_price(p->wolf_rank,1,
                &lesson.primal_cost,&lesson.experience_cost,&kills) ||
                ch->mkill<kills)
                return HYBRID_LESSON_PREREQUISITE;
        }
        ++p->wolf_rank;break;
    case HYBRID_LESSON_DROW_SPEED:
        if(r->parent!=CLASS_DROW)return HYBRID_LESSON_INVALID;
        if(p->drow[DROW_SPEED]>=4)return HYBRID_LESSON_AT_CAP;
        ++p->drow[DROW_SPEED];lesson.secondary_dp_cost=10000;break;
    case HYBRID_LESSON_DROW_POWER:result=unlock_lesson(&lesson,r,0);break;
    case HYBRID_LESSON_DRAGON_SKILL:result=unlock_lesson(&lesson,r,1);break;
    case HYBRID_LESSON_DRAGON_AGE: {
        static const long dp[]={2500,5000,7500,10000,15000,25000,35000,50000,70000,100000};
        static const long qp[]={0,0,0,0,500,1000,1500,2000,2500,3000};
        if(r->parent!=CLASS_DRAGON)return HYBRID_LESSON_INVALID;
        if(!s->secondary_build)return HYBRID_LESSON_CHOICE_REQUIRED;
        long age=s->secondary_generation;
        if(age<=AGE_WYRM)return HYBRID_LESSON_AT_CAP;
        if(age>AGE_HATCHLING)return HYBRID_LESSON_PREREQUISITE;
        size_t i=(size_t)(AGE_HATCHLING-age);
        lesson.secondary_dp_cost=dp[i];lesson.quest_cost=qp[i];
        --s->secondary_generation;break;
    }
    default:return HYBRID_LESSON_INVALID;
    }
    if(r->action<=HYBRID_LESSON_ELEMENT_FOCUS&&!selected)return HYBRID_LESSON_INVALID;
    if(result!=HYBRID_LESSON_OK)return result;
    if(!hybrid_state_valid(s))return HYBRID_LESSON_INVALID;
    if(!memcmp(s,&ch->pcdata->hybrid,sizeof(*s)))return HYBRID_LESSON_UNCHANGED;
    if(ch->primal<lesson.primal_cost||ch->exp<lesson.experience_cost||
       ch->pcdata->quest<lesson.quest_cost||
       s->secondary_classpoints[CP_CURRENT]<lesson.secondary_dp_cost)
        return HYBRID_LESSON_NO_FUNDS;
    lesson.primal=ch->primal-lesson.primal_cost;
    lesson.experience=ch->exp-lesson.experience_cost;
    lesson.quest=ch->pcdata->quest-lesson.quest_cost;
    s->secondary_classpoints[CP_CURRENT]-=lesson.secondary_dp_cost;
    if(!hybrid_state_valid(s))return HYBRID_LESSON_INVALID;
    *destination=lesson;
    return HYBRID_LESSON_OK;
}

enum hybrid_lesson_result hybrid_training_commit(CHAR_DATA *ch,
    const HYBRID_LESSON_REQUEST *r)
{
    if(!ch||IS_NPC(ch)||!ch->pcdata||!ch->desc||ch->desc->character!=ch||
       ch->desc->original||!descriptor_is_in_game(ch->desc)||!ch->in_room||
       ch->level<2||ch->fighting||ch->position<POS_STANDING)
        return HYBRID_LESSON_INVALID;
    CHAR_DATA *live=char_list;
    while(live&&live!=ch)live=live->next;
    if(!live)return HYBRID_LESSON_INVALID;
    if(ch->save_pending&PLAYER_SAVE_PRIMARY)return HYBRID_LESSON_SAVE_PENDING;
    HYBRID_LESSON lesson;
    enum hybrid_lesson_result result=hybrid_training_prepare(ch,r,&lesson);
    if(result!=HYBRID_LESSON_OK)return result;
    enum atomic_save_result saved=hybrid_player_commit(ch,&lesson.state,
        lesson.primal,lesson.experience,lesson.quest);
    if(saved==ATOMIC_SAVE_FAILED)return HYBRID_LESSON_SAVE_FAILED;
    if(saved==ATOMIC_SAVE_UNCERTAIN)return HYBRID_LESSON_SAVE_UNCERTAIN;
    return HYBRID_LESSON_OK;
}
