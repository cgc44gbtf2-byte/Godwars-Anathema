/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  God Wars Mud improvements copyright (C) 1994, 1995, 1996 by Richard    *
 *  Woolcock.  This mud is NOT to be copied in whole or in part, or to be  *
 *  run without the permission of Richard Woolcock.  Nobody else has       *
 *  permission to authorise the use of this code.                          *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "merc.h"

void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *gch;
    char buf[MSL];
    long xp;
    int count = 0, connected = 0;

    if (ch == NULL || victim == NULL || ch->in_room == NULL) return;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
if (is_same_group(gch, ch)) count++;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
CHAR_DATA *wch;

if (!descriptor_is_in_game(d)) continue;
wch = (d->original != NULL) ? d->original : d->character;
if (wch == NULL) continue;
if (IS_BUI(wch)) continue;

connected++;
    }

    if (victim == ch) return;

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
if (!is_same_group(gch, ch) || IS_NPC(gch)) continue;

xp = xp_compute(victim);
if (IS_FLAG(gch, FLAG_PK)) xp *= 1.25;

if (count > 1)
{
    xp *= 2.5;
    xp /= count;
}

if (connected >= 10) xp = (xp * (100 + connected)) / 100;

if (xp == 1) strcpy(buf, "[You receive an experience point]\n\r");
else sprintf(buf, "[You receive %li experience points]\n\r", xp);
stc(buf, gch);

if (IS_SET(exp_info.bit, EXP_RANDOM)) xp = (xp * number_range(0, 400)) / 100;
else xp = (xp * exp_info.exp) / 100;

if (IS_SET(exp_info.bit, EXP_RANDOM))
{
    sprintf(buf, "You recieve %li experience points due to random xp.\n\r", xp);
    stc(buf, gch);
}
else if (exp_info.exp > 100)
{
    sprintf(buf, "You receive %li experience points due to bonus xp.\n\r", xp);
    stc(buf, gch);
}
else if (exp_info.exp < 100)
{
    sprintf(buf, "You receive %li experience points due to reduced xp.\n\r", xp );
    stc(buf, gch);
}

        gch->pcdata->score[SCORE_TOTAL_LEVEL] += victim->level;
        if (victim->level > gch->pcdata->score[SCORE_HIGH_LEVEL]) gch->pcdata->score[SCORE_HIGH_LEVEL] = victim->level;
        gch->pcdata->score[SCORE_TOTAL_XP] += xp;
        if (xp > gch->pcdata->score[SCORE_HIGH_XP]) gch->pcdata->score[SCORE_HIGH_XP] = xp;
gch->exp += xp;
    }

    return;
}

/*
 * Compute xp for a kill.
 * Edit this function to change xp computations.
 */
long xp_compute( CHAR_DATA *victim )
{
    long xp = victim->level;
    bool noexp = FALSE;

    if (IS_NPC(victim))
    {
switch (victim->pIndexData->vnum)
{
    case 29600: case 30000: case 30001: case 30006: case 30007: case 30008: case 30009:
    case 30010: case 30012: case 30013: case 30014: case 30015: case 30016:
noexp = TRUE;
break;
}
if (IS_SET(victim->act, ACT_NOEXP)) noexp = TRUE;
    }
    else noexp = TRUE;

    if (noexp) return 0;

    if (victim->level > 100) xp *= victim->level;
    else xp *= 100;
    xp *= 1.5;
    xp += victim->max_hit / 10;

    if (IS_AFFECTED(victim, AFF_SANCTUARY)) xp *= 1.25;

    return xp;
}

void dp_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
    long vnum;
    long dp = 0;
    char buf[MSL];

    if (IS_NPC(victim) && (IS_DROW(ch) || IS_DRAGON(ch)))
    {
if ((vnum = victim->pIndexData->vnum) > 29000)
{
    switch (vnum)
    {
case 29600: case 30000: case 30001: case 30006: 
        case 30007: case 30008: case 30009: case 30010: 
case 30013: case 30014: case 30015: case 30016:
    return;
    }
}
if (IS_SET(victim->act, ACT_NOEXP)) return;

dp = victim->level;
if (IS_SET(exp_info.bit, EXP_RANDOM)) dp = (dp * number_range(0, 400)) / 100;
else dp = (dp * exp_info.exp) / 100;

if (dp == 1)
{
    strcpy(buf, "[You receive a class point]\n\r");
    stc(buf, ch);
}
else if (dp > 1)
{
    sprintf(buf, "[You receive %li class points]\n\r", dp);
    stc(buf, ch);
}

ch->pcdata->classpoints[CP_CURRENT] += dp;
ch->pcdata->classpoints[CP_TOTAL] += dp;
 }
    return;
}
