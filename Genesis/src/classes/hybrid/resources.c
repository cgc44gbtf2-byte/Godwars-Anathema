/***************************************************************************
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer, Michael
 * Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.
 * Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael Chastain,
 * Michael Quan, and Mitchell Tse.
 * In order to use any part of this Merc Diku Mud, you must comply with both
 * the original Diku license in 'license.doc' as well the Merc license in
 * 'license.txt'. In particular, you may not remove either copyright notice.
 * God Wars Mud improvements copyright (C) 1994, 1995, 1996 by Richard
 * Woolcock. This mud is NOT to be copied in whole or in part, or to be run
 * without the permission of Richard Woolcock. Nobody else has permission
 * to authorise the use of this code.
 ***************************************************************************/
/* Owned runtime accounting. Original native handlers remain unchanged. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "../../merc.h"
#include "access.h"
#include "resources.h"

static long owner(enum hybrid_resource resource)
{
    switch(resource) {
    case HYBRID_RESOURCE_BLOOD:return CLASS_VAMPIRE;
    case HYBRID_RESOURCE_DROW_DP:return CLASS_DROW;
    case HYBRID_RESOURCE_DRAGON_DP:return CLASS_DRAGON;
    case HYBRID_RESOURCE_KI:return CLASS_NINJA;
    case HYBRID_RESOURCE_BODY_QUINT:return CLASS_MAGE;
    default:return CLASS_NONE;
    }
}

static long *secondary_slot(HYBRID_STATE *s,enum hybrid_resource resource)
{
    switch(resource) {
    case HYBRID_RESOURCE_BLOOD:return &s->secondary_blood[BLOOD_CURRENT];
    case HYBRID_RESOURCE_DROW_DP:
    case HYBRID_RESOURCE_DRAGON_DP:return &s->secondary_classpoints[CP_CURRENT];
    case HYBRID_RESOURCE_KI:return &s->secondary_progression.ninja[NINJA_KI];
    case HYBRID_RESOURCE_BODY_QUINT:return &s->secondary_progression.quint[BODY];
    default:return NULL;
    }
}

int hybrid_resource_read(const CHAR_DATA *ch,long parent,
                         enum hybrid_resource resource,long *destination)
{
    long value;
    if(!ch || IS_NPC(ch) || !ch->pcdata || !destination ||
       owner(resource)==CLASS_NONE || parent!=owner(resource))return 0;
    if(ch->class==parent) {
        switch(resource) {
        case HYBRID_RESOURCE_BLOOD:value=ch->blood[BLOOD_CURRENT];break;
        case HYBRID_RESOURCE_DROW_DP:
        case HYBRID_RESOURCE_DRAGON_DP:value=ch->pcdata->classpoints[CP_CURRENT];break;
        case HYBRID_RESOURCE_KI:value=ch->pcdata->ninja[NINJA_KI];break;
        case HYBRID_RESOURCE_BODY_QUINT:value=ch->quint[BODY];break;
        default:return 0;
        }
    } else {
        const HYBRID_STATE *s=hybrid_live_secondary(ch);
        if(!s || parent!=s->secondary_class)return 0;
        switch(resource) {
        case HYBRID_RESOURCE_BLOOD:value=s->secondary_blood[BLOOD_CURRENT];break;
        case HYBRID_RESOURCE_DROW_DP:
        case HYBRID_RESOURCE_DRAGON_DP:value=s->secondary_classpoints[CP_CURRENT];break;
        case HYBRID_RESOURCE_KI:value=s->secondary_progression.ninja[NINJA_KI];break;
        case HYBRID_RESOURCE_BODY_QUINT:value=s->secondary_progression.quint[BODY];break;
        default:return 0;
        }
    }
    if(value<0)return 0;
    *destination=value;
    return 1;
}

static int apply(CHAR_DATA *ch,long parent,enum hybrid_resource resource,
                 long amount,int gain)
{
    if(amount<0 || owner(resource)==CLASS_NONE || parent!=owner(resource))return 0;
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if(!s || s->secondary_class!=parent)return 0;
    HYBRID_STATE candidate;
    memcpy(&candidate,s,sizeof(candidate));
    long *slot=secondary_slot(&candidate,resource);
    if(!slot)return 0;
    if(!gain) {
        if(amount>*slot)return 0;
        *slot-=amount;
    } else if(amount) {
        long capacity=LONG_MAX;
        if(resource==HYBRID_RESOURCE_BLOOD)capacity=s->secondary_blood[BLOOD_POOL];
        if(resource==HYBRID_RESOURCE_KI)capacity=300;
        if(resource==HYBRID_RESOURCE_BODY_QUINT)capacity=s->secondary_progression.arete;
        /* Preserve migrated over-cap body quint, but never add more to it. */
        if(*slot>capacity || amount>capacity-*slot)return 0;
        if(resource==HYBRID_RESOURCE_DROW_DP || resource==HYBRID_RESOURCE_DRAGON_DP) {
            long *total=&candidate.secondary_classpoints[CP_TOTAL];
            if(amount>LONG_MAX-*total)return 0;
            *total+=amount;
        }
        *slot+=amount;
    }
    if(!hybrid_state_valid(&candidate))return 0;
    memcpy(&ch->pcdata->hybrid,&candidate,sizeof(candidate));
    return 1;
}

int hybrid_secondary_resource_spend(CHAR_DATA *ch,long parent,
                                    enum hybrid_resource resource,long amount)
{
    return apply(ch,parent,resource,amount,0);
}

int hybrid_secondary_resource_gain(CHAR_DATA *ch,long parent,
                                   enum hybrid_resource resource,long amount)
{
    return apply(ch,parent,resource,amount,1);
}

void hybrid_secondary_mobile_update(CHAR_DATA *ch)
{
    if(!ch || !ch->in_room || !ch->desc || !IS_PLAYING(ch->desc))return;
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if(!s || s->secondary_class!=CLASS_NINJA)return;
    HYBRID_STATE candidate;
    memcpy(&candidate,s,sizeof(candidate));
    HYBRID_PROGRESSION *p=&candidate.secondary_progression;
    /* Conservatively suspend regeneration under ambiguous Kakusu; do not
     * clear an original-parent flag, even when fighting. Its owning secondary
     * activation/flag lifecycle is not implemented yet. */
    if(p->ninja[NINJA_KI]<300 && !IS_AFFECTED(ch,AFF_KAKUSU)) {
        long gain=ch->position==POS_MEDITATING?5:2;
        if(gain>300-p->ninja[NINJA_KI])gain=300-p->ninja[NINJA_KI];
        p->ninja[NINJA_KI]+=gain;
    }
    if(p->ninja[NINJA_HARAKIRI]>0)--p->ninja[NINJA_HARAKIRI];
    if(candidate.secondary_rage>0) {
        long cap=hybrid_power_rank(ch,CLASS_NINJA,HYBRID_TRAIN_NINJA,NINJA_SORA)*25;
        if(ch->position!=POS_FIGHTING)--candidate.secondary_rage;
        else if(candidate.secondary_rage<cap)++candidate.secondary_rage;
    }
    if(hybrid_state_valid(&candidate))
        memcpy(&ch->pcdata->hybrid,&candidate,sizeof(candidate));
}
