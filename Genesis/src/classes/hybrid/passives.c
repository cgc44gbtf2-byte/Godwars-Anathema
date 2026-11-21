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
/* Native learned formulas adapted to owned secondary banks; original native
 * class calculations remain in place. No copied activation state. */
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include "../../merc.h"
#include "access.h"
#include "passives.h"

static long add(long base,long delta)
{
    if(delta>0 && base>LONG_MAX-delta)return LONG_MAX;
    if(delta<0 && base<LONG_MIN-delta)return LONG_MIN;
    return base+delta;
}

long hybrid_secondary_passive(CHAR_DATA *ch,
                             enum hybrid_passive_metric m, long value)
{
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if (!s) return value;
    long initial=value;
    value=0; /* All deltas below are bounded by validated native ranks/bits. */
    long parent=s->secondary_class;
#define R(tree,index) hybrid_power_rank(ch,parent,HYBRID_TRAIN_##tree,index)
    long a,b,c,d;
    switch (parent) {
    case CLASS_VAMPIRE:
        a=R(DISCIPLINE,DISC_FORTITUDE);b=R(DISCIPLINE,DISC_POTENCE);
        if (m==HYBRID_PASSIVE_SOAK) value+=a*75;
        if (m==HYBRID_PASSIVE_ARMOR) value+=a*100;
        if (m==HYBRID_PASSIVE_DAMROLL) value+=b*20;
        if (m==HYBRID_PASSIVE_DAMCAP) {
            value+=b*145;
            if (b>=7) value+=50;
        }
        break;
    case CLASS_WEREWOLF:
        if (m==HYBRID_PASSIVE_SPEED) {
            a=R(TRIBE,TRIBE_SILENT_STRIDERS);
            if (a>=1) --value;
            if (a>=3) --value;
            if (R(AUSPICE,AUSPICE_AHROUN)>=2) --value;
        } else if (m==HYBRID_PASSIVE_SOAK) {
            a=R(TRIBE,TRIBE_BONE_GNAWERS);
            if (a>=2) value+=30*a;
        } else if (m==HYBRID_PASSIVE_AVOID) {
            if (R(BREED,BREED_LUPUS)>=3) value+=3;
            if (R(TRIBE,TRIBE_CHILDREN_OF_GAIA)>=2) value+=3;
            if (R(TRIBE,TRIBE_STARGAZERS)>=4) value+=3;
        } else if (m==HYBRID_PASSIVE_BREAK) {
            if (R(AUSPICE,AUSPICE_PHILODOX)>=3) value+=15;
        } else if (m==HYBRID_PASSIVE_DAMCAP) {
            if (R(TRIBE,TRIBE_SHADOW_LORDS)>=1) value+=100;
            if (R(TRIBE,TRIBE_GET_OF_FENRIS)>=1) value+=100;
            if (R(AUSPICE,AUSPICE_PHILODOX)>=3) value+=100;
        } else if (m==HYBRID_PASSIVE_ARMOR) {
            if (R(TRIBE,TRIBE_CHILDREN_OF_GAIA)>=2) value+=250;
        } else if (m==HYBRID_PASSIVE_HITROLL || m==HYBRID_PASSIVE_DAMROLL) {
            a=R(TRIBE,TRIBE_STARGAZERS);
            if (a>=2) value+=a*10;
        }
        break;
    case CLASS_MAGE:
        a=R(SPHERE,MTIM);b=R(SPHERE,MENT);
        if (m==HYBRID_PASSIVE_SPEED && a>=1) value-=5;
        if (m==HYBRID_PASSIVE_AVOID && a>=2) value+=15;
        if (m==HYBRID_PASSIVE_BREAK && b>=1) value+=8;
        if (m==HYBRID_PASSIVE_DAMCAP && b>=1) value+=400;
        break;
    case CLASS_HIGHLANDER:
        if (m==HYBRID_PASSIVE_SPEED) value-=R(HIGHLAND,HIGH_ATTACK);
        if (m==HYBRID_PASSIVE_SOAK) value+=R(HIGHLAND,HIGH_DEFENSE)*50;
        if (m==HYBRID_PASSIVE_AVOID || m==HYBRID_PASSIVE_BREAK)
            value+=R(HIGHLAND,HIGH_WEAPON)*5;
        if (m==HYBRID_PASSIVE_DAMCAP) value+=R(HIGHLAND,HIGH_ATTACK)*300;
        if (m==HYBRID_PASSIVE_HITROLL || m==HYBRID_PASSIVE_DAMROLL)
            value+=R(HIGHLAND,HIGH_ATTACK)*50;
        if (m==HYBRID_PASSIVE_ARMOR) value+=R(HIGHLAND,HIGH_DEFENSE)*100;
        break;
    case CLASS_NINJA:
        a=R(NINJA,NINJA_NINGENNO);b=R(NINJA,NINJA_CHIKYU);
        if (m==HYBRID_PASSIVE_SPEED) value-=a;
        if (m==HYBRID_PASSIVE_AVOID) value+=b;
        if (m==HYBRID_PASSIVE_BREAK) value+=a*4;
        if (m==HYBRID_PASSIVE_DAMCAP) value+=b*50;
        break;
    case CLASS_MONK:
        a=R(MONK,MONK_ART);
        if (m==HYBRID_PASSIVE_SPEED) {
            if (a>=3) value-=4;
            if (a>=8) value-=2;
        }
        if (m==HYBRID_PASSIVE_SOAK && a>=5) value+=200;
        if (m==HYBRID_PASSIVE_AVOID && a>=4) value+=15;
        if (m==HYBRID_PASSIVE_BREAK && a>=2) value+=10;
        if (m==HYBRID_PASSIVE_DAMCAP) {
            if (a>=1) value+=100;
            if (a>=7) value+=100;
        }
        if (m==HYBRID_PASSIVE_HITROLL && a>=2) value+=200;
        if (m==HYBRID_PASSIVE_ARMOR && a>=4) value+=375;
        if (m==HYBRID_PASSIVE_DAMROLL) {
            if (a>=1) value+=100;
            if (a>=7) value+=100;
        }
        break;
    case CLASS_DROW:
        a=s->secondary_progression.drow[DROW_SPEED];
        if (m==HYBRID_PASSIVE_DAMROLL) value+=a*50;
        if (m==HYBRID_PASSIVE_SPEED) {
            if (hybrid_power_drow(ch,DROW_SKIP)) value-=3;
            value-=a;
        } else if (m==HYBRID_PASSIVE_SOAK) {
            if (hybrid_power_drow(ch,DROW_TOUGH)) value+=300;
        } else if (m==HYBRID_PASSIVE_AVOID) {
            if (hybrid_power_drow(ch,DROW_SKIP)) value+=10;
            value+=(a*5)/2;
        } else if (m==HYBRID_PASSIVE_BREAK) {
            if (hybrid_power_drow(ch,DROW_EYES)) value+=10;
            value+=a*5;
        } else if (m==HYBRID_PASSIVE_DAMCAP) {
            if (hybrid_power_drow(ch,DROW_MIGHT)) value+=600;
            if (hybrid_power_drow(ch,DROW_BITE)) value+=200;
            value+=a*100;
        }
        break;
    case CLASS_ELEM:
        a=R(ELEMENT,ELEM_WIND);b=R(ELEMENT,ELEM_TIME);
        c=R(ELEMENT,ELEM_EARTH);d=R(ELEMENT,ELEM_FIRE);
        if (m==HYBRID_PASSIVE_SPEED) {
            if (a>=5) value-=a/2;
            if (b>=5) value-=b/2;
            if (c>=5) value+=c-5;
        } else if (m==HYBRID_PASSIVE_SOAK) {
            if (c>=5) value+=c*35;
        } else if (m==HYBRID_PASSIVE_AVOID) {
            if (a>4) value+=a;
            if (b>4) value+=b;
            long moon=R(ELEMENT,ELEM_MOON);
            if (moon>4) value+=moon;
            if (c>5) value-=c;
        } else if (m==HYBRID_PASSIVE_BREAK) {
            if (b>2) value+=b;
            if (a>4) value+=a;
        } else if (m==HYBRID_PASSIVE_DAMCAP) {
            if (d>=5) value+=d*60;
            long infinity=R(ELEMENT,ELEM_INFIN);
            if (infinity>=8) value+=infinity*30;
        }
        break;
    default:
        /* Sorcerer has no learned-only contribution in these five paths;
         * Dragon bonuses require its owned form/lifecycle adapter. */
        break;
    }
#undef R
    long result=add(initial,value);
    /* Native += speed*2.5 truncates toward zero after the entire sum.
     * Integer arithmetic preserves that rule without double->long overflow. */
    if(parent==CLASS_DROW && m==HYBRID_PASSIVE_AVOID &&
       (s->secondary_progression.drow[DROW_SPEED]&1) && result<0)
        result=add(result,1);
    return result;
}

int hybrid_secondary_unwounded(CHAR_DATA *ch)
{
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    return s && s->secondary_class==CLASS_WEREWOLF &&
        hybrid_power_rank(ch,CLASS_WEREWOLF,HYBRID_TRAIN_AUSPICE,
                          AUSPICE_PHILODOX)>=1;
}
