/***************************************************************************
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,           *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.      *
 * Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael             *
 * Chastain, Michael Quan, and Mitchell Tse.                               *
 * In order to use any part of this Merc Diku Mud, you must comply with    *
 * both the original Diku license in 'license.doc' as well the Merc        *
 * license in 'license.txt'. In particular, you may not remove either of   *
 * these copyright notices.                                               *
 ***************************************************************************/
/***************************************************************************
 * God Wars Mud improvements copyright (C) 1994, 1995, 1996 by Richard       *
 * Woolcock. This mud is NOT to be copied in whole or in part, or to be     *
 * run without the permission of Richard Woolcock. Nobody else has         *
 * permission to authorise the use of this code.                           *
 ***************************************************************************/
#include <sys/types.h>
#include <stdio.h>
#include "merc.h"

/* Attack-loop targeting lives here; public combat entry points stay stable. */
CHAR_DATA *combat_assist_target(CHAR_DATA *helper, CHAR_DATA *victim)
{
    CHAR_DATA *vch, *target = NULL;
    long number = 0;

    if (helper == NULL || victim == NULL || helper->in_room == NULL ||
        helper->in_room != victim->in_room)
        return NULL;

    for (vch = helper->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->in_room != helper->in_room)
            continue;
        if (can_see(helper, vch) && is_same_group(vch, victim) &&
            number_range(0, number) == 0)
        {
            target = vch;
            number++;
        }
    }
    return target;
}
