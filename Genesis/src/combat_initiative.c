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

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"

/*
 * Determines number of pulses between Attacks.
 */
long get_speed( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    long count = 26;

    count -= char_stat(ch, STAT_DEX);

    if ((victim = ch->fighting) == NULL || !IS_STANCE(victim, STANCE_TIGER))
    {
	if (IS_STANCE(ch, STANCE_VIPER)) count -= ch->stance[STANCE_VIPER] / 100;
	else if (IS_STANCE(ch, STANCE_MANTIS)) count -= ch->stance[STANCE_MANTIS] / 100;
	else if (IS_STANCE(ch, STANCE_MONKEY)) count -= ch->stance[STANCE_MONKEY] / 100;
    }

    if (!IS_NPC(ch))
    {
	switch (ch->class)
	{
	    case CLASS_VAMPIRE:
		count -= get_celerity(ch) + 1;
		if (IS_POLYAFF(ch, POLY_SERPENT)) count -= 3;
		break;
	    case CLASS_WEREWOLF:
		if (get_tribe(ch, TRIBE_SILENT_STRIDERS) >= 1) count--;
		if (get_tribe(ch, TRIBE_SILENT_STRIDERS) >= 3) count--;
		if (get_auspice(ch, AUSPICE_AHROUN) >= 2) count--;
		if (ch->pcdata->rank >= 5) count -= 5;
		else count -= ch->pcdata->rank + 1;
		break;
	   case CLASS_MAGE:
		if (ch->spheres[MTIM] >= 1) count -= 5;
		break;
	   case CLASS_MONK:
		if (get_monk(ch, MONK_ART) >= 3) count -= 4;
		if (get_monk(ch, MONK_ART) >= 8) count -= 2;
		break;
	    case CLASS_DROW:
		if (get_drow(ch, DROW_SKIP)) count -= 3;
                count -= ch->pcdata->drow[DROW_SPEED];
		break;
	    case CLASS_HIGHLANDER:
		count -= get_highland(ch, HIGH_ATTACK);
		break;
	    case CLASS_NINJA:
		count -= get_ninja(ch, NINJA_NINGENNO);
		break;
	    case CLASS_DRAGON:
		if (IS_POLYAFF(ch, POLY_DRAGONFORM)) count -= (13 - ch->vampgen);
		break;
	    case CLASS_ELEM:
		if (ch->pcdata->powers[ELEM_WIND] >= 5) count -= ch->pcdata->powers[ELEM_WIND] / 2;
		if (ch->pcdata->powers[ELEM_TIME] >= 5) count -= ch->pcdata->powers[ELEM_TIME] / 2;
		if (ch->pcdata->powers[ELEM_EARTH] >= 5) count += ch->pcdata->powers[ELEM_EARTH] - 5;
		break;
	}
	if (ch->pcdata->pain > 0) count += 3;
    }


    if (IS_XTRA(ch, XTRA_ATONING)) count += 2;
    if (IS_XTRA(ch, XTRA_LIQUIFY)) count += 4;
    if (IS_XTRA(ch, XTRA_SLOW)) count += 1;

    if (is_affected(ch, gsn_friction_curse)) count += 2;
    if (is_affected(ch, gsn_alter_weight)) count += 3;
    if (IS_ITEMAFF(ch, ITEMA_SPEED)) count--;
    if (count < 6) count = 6;

    return count;
}
