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

/* Fight-list ownership.  General output stays in core.c; timers and
 * action gating stay in combat_status.c. */
#include <sys/types.h>
#include <stdio.h>
#include "merc.h"

void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if (auto_safe(ch, victim)) return;

    if (ch->fighting != NULL)
    {
	bug( "Set_fighting: already fighting", 0 );
	return;
    }

    if (!IS_NPC(ch) && !IS_NPC(victim))
    {
	ch->fight_timer = 30;
	victim->fight_timer = 30;
    }
    
    if ( ch->next_fight != NULL || ch->prev_fight != NULL )
    {
	bug( "Set_fighting: new fight code", 0 );
	return;
    }

    autostance(ch);
    autostance(victim);
    if (IS_AFFECTED(ch, AFF_SLEEP))
	affect_strip(ch, gsn_sleep);

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    if (first_fight == NULL)
	first_fight = ch;

    if (last_fight != NULL)
    {
	last_fight->next_fight = ch;
	ch->prev_fight = last_fight;
    }
    last_fight = ch;

    if (IS_MAGE(victim) && str_cmp(victim->contingency, "")) do_contingency(victim, "");
    if (IS_MAGE(ch) && str_cmp(ch->contingency, "")) do_contingency(ch, "");

    return;
}

void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;

    for (fch = first_fight; fch != NULL; fch = fch_next)
    {
	fch_next = fch->next_fight;

	if (fch == ch || (fBoth && fch->fighting == ch))
	{
	    fch->fighting	= NULL;
	    fch->position	= POS_STANDING;

	    if (fch->prev_fight != NULL && fch->next_fight != NULL)
	    {
		fch->prev_fight->next_fight = fch->next_fight;
		fch->next_fight->prev_fight = fch->prev_fight;
	    }
	    else if (fch->prev_fight != NULL)
	    {
		fch->prev_fight->next_fight = NULL;
		last_fight = fch->prev_fight;
	    }
	    else if (fch->next_fight != NULL)
	    {
		fch->next_fight->prev_fight = NULL;
		first_fight = fch->next_fight;
	    }
	    else
	    {
		first_fight = NULL;
		last_fight = NULL;
	    }

	    fch->prev_fight = NULL;
	    fch->next_fight = NULL;

	    update_pos( fch );
	}
    }

    return;
}
