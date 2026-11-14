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

/* Explicit-command and automatic combat safety intentionally differ:
 * preserve each policy and its messages rather than merging predicates. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"

bool is_safe( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if (ch == NULL || victim == NULL ||
	ch->in_room == NULL || victim->in_room == NULL) return TRUE;
    if (ch->in_room != victim->in_room) return TRUE;
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_NOKILL))
    {
	stc("That mobile cannot be attacked.\n\r", ch);
	return TRUE;
    }

    if (IS_AFFECTED(ch, AFF_ETHEREAL) && !IS_AFFECTED(victim, AFF_ETHEREAL))
    {
	stc("You cannot while ethereal.\n\r", ch);
	return TRUE;
    }
    if (!IS_AFFECTED(ch, AFF_ETHEREAL) && IS_AFFECTED(victim, AFF_ETHEREAL))
    {
	stc("You cannot fight an ethereal person.\n\r", ch);
	return TRUE;
    }
    if (IS_POLYAFF(ch, POLY_MIST) || IS_POLYAFF(victim, POLY_MIST))
    {
	stc("People in mist form may not enter combat.\n\r", ch);
	return TRUE;
    }
    if (IS_POLYAFF(ch, POLY_SHADOW) || IS_POLYAFF(victim, POLY_SHADOW))
    {
	stc("People in smoke form may not enter combat.\n\r", ch);
	return TRUE;
    }
    if (IS_AFFECTED(ch, AFF_DEEPUMBRA) && !IS_AFFECTED(victim, AFF_DEEPUMBRA))
    {
	act("You are too insubstantial!", ch, NULL, victim, TO_CHAR );
	return TRUE;
    }
    if (!IS_AFFECTED(ch, AFF_DEEPUMBRA) && IS_AFFECTED(victim, AFF_DEEPUMBRA))
    {
	act( "$E is too insubstantial!", ch, NULL, victim, TO_CHAR );
	return TRUE;
    }
    if (IS_AFFECTED(ch, AFF_SHADOWPLANE) && !IS_AFFECTED(victim, AFF_SHADOWPLANE))
    {
	act("You are too insubstantial!", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }
    if (!IS_AFFECTED(ch, AFF_SHADOWPLANE) && IS_AFFECTED(victim, AFF_SHADOWPLANE))
    {
	if (!IS_MAGE(ch) || !IS_SET(ch->xtra, XTRA_MAGE_CARESS))
	{
	    act("$E is too insubstantial!", ch, NULL, victim, TO_CHAR);
	    return TRUE;
	}
    }
    if (!IS_NPC(victim) && IS_FLAG(victim, FLAG_AFK))
    {
        stc("They are AFK!\n\r", ch);
        return TRUE;
    }
    if (!IS_NPC(ch) && IS_FLAG(ch, FLAG_AFK))
    {
	stc("Not while afk!\n\r", ch);
	return TRUE;
    }
    if (!IS_NPC(victim) && victim->desc == NULL && victim->position != POS_FIGHTING)
    {
	stc("They are currently LINK-DEAD!\n\r", ch);
	return TRUE;
    }
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE) && ch->status < 5)
    {
        stc("Something in the air prevents you from attacking.\n\r", ch);
        return TRUE;
    }
    if (IS_SET(victim->in_room->room_flags, ROOM_SAFE) && victim->status < 5)
    {
	stc("Something in the air prevents you from attacking.\n\r", ch);
	return TRUE;
    }
    if (IS_MORE(ch, MORE_HEAD) || IS_EXTRA(ch, EXTRA_OSWITCH))
    {
	stc("Objects cannot fight!\n\r", ch);
	return TRUE;
    }
    if (IS_MORE(victim, MORE_HEAD) || IS_EXTRA(victim, EXTRA_OSWITCH))
    {
	stc("You cannot attack an object.\n\r", ch);
	return TRUE;
    }
    if (IS_VAMPAFF(ch, VAM_EARTHMELDED) || IS_VAMPAFF(victim, VAM_EARTHMELDED))
    {
	stc("Earthmelded players can't enter combat.\n\r", ch);
	return TRUE;
    }

    if (victim->fighting == ch) return FALSE;
    if (victim->fight_timer > 0) return FALSE;

    if (IS_NPC(ch) && (!IS_NPC(victim) && !CAN_PK(victim)) && IS_SET(ch->act, ACT_COMMANDED))
    {
	stc("You may not attack people without a PK flag.\n\r", ch);
	return TRUE;
    }
    if (IS_NPC(victim) && !CAN_PK(ch) && IS_SET(victim->act, ACT_COMMANDED))
    {
	stc("You need a PK flag to attack player controlled mobs.\n\r", ch);
	return TRUE;
    }
    if (IS_NPC(ch) && ch->level < 700 && IS_EXTRA(victim, EXTRA_AWE) && (ch->position != POS_FIGHTING))
    {
	stc("You like them too much to attack them.\n\r", ch);
	return TRUE;
    }
    if (IS_ITEMAFF(ch, ITEMA_PEACE))
    {
	send_to_char( "You are unable to attack them.\n\r", ch );
	return TRUE;
    }
    if (IS_ITEMAFF(victim, ITEMA_PEACE))
    {
	stc("They look FAR too imposing to attack!\n\r", ch);
	return TRUE;
    }

    if (IS_NPC(ch) || IS_NPC(victim)) return FALSE;

    if (ch->desc == NULL || victim->desc == NULL)
    {
	if (victim->position != POS_FIGHTING && victim->position > POS_STUNNED)
	{
	    stc("They are currently link dead.\n\r", ch);
	    return TRUE;
	}
    }
    if (IS_MORE(ch, MORE_LOYAL) && strlen(ch->pcdata->conding) > 1 && !str_cmp(ch->pcdata->conding, victim->name))
    {
	stc("You refuse to attack your master!\n\r", ch);
	return TRUE;
    }
    else if (strlen(ch->pcdata->love) > 1 && !str_cmp(ch->pcdata->love, victim->name))
    {
	stc("How could you attack someone you love?!\n\r", ch);
	return TRUE;
    }

    return FALSE;
}

bool auto_safe( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if (ch == NULL || victim == NULL ||
	ch->in_room == NULL || victim->in_room == NULL) return TRUE;
    if (ch->in_room != victim->in_room) return TRUE;
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_NOKILL)) return TRUE;
    if (IS_AFFECTED(ch, AFF_ETHEREAL) && !IS_AFFECTED(victim, AFF_ETHEREAL)) return TRUE;
    if (!IS_AFFECTED(ch, AFF_ETHEREAL) && IS_AFFECTED(victim, AFF_ETHEREAL)) return TRUE;
    if (IS_POLYAFF(ch, POLY_MIST) || IS_POLYAFF(victim, POLY_MIST)) return TRUE;
    if (IS_POLYAFF(ch, POLY_SHADOW) || IS_POLYAFF(victim, POLY_SHADOW)) return TRUE;
    if (IS_AFFECTED(ch, AFF_DEEPUMBRA) && !IS_AFFECTED(victim, AFF_DEEPUMBRA)) return TRUE;
    if (!IS_AFFECTED(ch, AFF_DEEPUMBRA) && IS_AFFECTED(victim, AFF_DEEPUMBRA)) return TRUE;
    if (IS_AFFECTED(ch, AFF_SHADOWPLANE) && !IS_AFFECTED(victim, AFF_SHADOWPLANE)) return TRUE;
    if (!IS_AFFECTED(ch, AFF_SHADOWPLANE) && IS_AFFECTED(victim, AFF_SHADOWPLANE)) return TRUE;
    if (IS_SET(ch->in_room->room_flags, ROOM_SAFE)) return TRUE;
    if (IS_MORE(ch, MORE_HEAD) || IS_EXTRA(ch, EXTRA_OSWITCH)) return TRUE;
    if (IS_MORE(victim, MORE_HEAD) || IS_EXTRA(victim, EXTRA_OSWITCH)) return TRUE;
    if (IS_VAMPAFF(ch, VAM_EARTHMELDED) || IS_VAMPAFF(victim, VAM_EARTHMELDED)) return TRUE;

    if (victim->fighting == ch) return FALSE;

    if (IS_NPC(ch) && !CAN_PK(victim) && IS_SET(ch->act, ACT_COMMANDED)) return TRUE;
    if (IS_NPC(victim) && !CAN_PK(ch) && IS_SET(victim->act, ACT_COMMANDED)) return TRUE;
    if (IS_NPC(ch) && ch->level < 700 && IS_EXTRA(victim, EXTRA_AWE)) return TRUE;
    if (IS_ITEMAFF(ch, ITEMA_PEACE)) return TRUE;
    if (IS_ITEMAFF(victim, ITEMA_PEACE)) return TRUE;

    if (IS_NPC(ch) || IS_NPC(victim)) return FALSE;

    if ((ch->desc == NULL || victim->desc == NULL) && victim->position != POS_FIGHTING && victim->position > POS_STUNNED) return TRUE;

    if (IS_MORE(ch, MORE_LOYAL) && strlen(ch->pcdata->conding) > 1 && !str_cmp(ch->pcdata->conding, victim->name)) return TRUE;
    if (strlen(ch->pcdata->love) > 1 && !str_cmp(ch->pcdata->love, victim->name)) return TRUE;

    return FALSE;
}
