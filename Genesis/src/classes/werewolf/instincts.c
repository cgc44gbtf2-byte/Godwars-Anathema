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

/* Werewolf-only entry points; transformation and Calm remain shared code. */
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include "merc.h"

void do_shadowsight( CHAR_DATA *ch, char *argument )
{
    char      arg [MAX_INPUT_LENGTH];

    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return;
    argument = one_argument( argument != NULL ? argument : "", arg );

    if (IS_NPC(ch)) return;
    if (!IS_WEREWOLF(ch))
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
    if (IS_AFFECTED(ch,AFF_SHADOWSIGHT) )
    {
	send_to_char("You can no longer see between planes.\n\r",ch);
	REMOVE_BIT(ch->affected_by, AFF_SHADOWSIGHT);
	return;
    }
    send_to_char("You can now see between planes.\n\r",ch);
    SET_BIT(ch->affected_by, AFF_SHADOWSIGHT);
    return;
}

void do_rage( CHAR_DATA *ch, char *argument )
{
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL ||
        ch->in_room == NULL) return;

    if (!IS_WEREWOLF(ch))
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (ch->pcdata->wolf < 0 || ch->pcdata->wolf > LONG_MAX - 50)
    {
        send_to_char("Your rage needs staff review.\n\r", ch);
        return;
    }
    if (!IS_SET(ch->act, PLR_WOLFMAN) || ch->pcdata->wolf < 300)
    {
	send_to_char("You start snarling angrilly.\n\r",ch);
	act("$n starts snarling angrilly.",ch,NULL,NULL,TO_ROOM);
	ch->pcdata->wolf += number_range(10,20);
	if (ch->pcdata->wolf >= 100) do_werewolf(ch,"");
	WAIT_STATE(ch,12);
	return;
    }
    send_to_char("But you are already in a rage!\n\r",ch);
    return;
}

/* Compatibility command, not a second transformation system. Rage may need
 * several uses before reaching 100 and still has its native hostile effects.
 * Returning human uses Calm, including Gaia1, posture and recovery rules. */
void do_shift( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
    int human;

    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL ||
        ch->in_room == NULL) return;
    if (!IS_WEREWOLF(ch))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    argument = one_argument(argument != NULL ? argument : "", arg);
    one_argument(argument, extra);
    if (extra[0] ||
        (arg[0] && str_cmp(arg,"human") && str_cmp(arg,"homid") &&
         str_cmp(arg,"werewolf") && str_cmp(arg,"wolf") && str_cmp(arg,"crinos")))
    {
        send_to_char("Syntax: shift [werewolf|human]. Uses Rage/Calm, not new forms.\n\r",ch);
        return;
    }
    human = arg[0] ? (!str_cmp(arg,"human") || !str_cmp(arg,"homid"))
                  : IS_SET(ch->act,PLR_WOLFMAN);
    if (human)
    {
        if (!IS_SET(ch->act,PLR_WOLFMAN) && ch->pcdata->wolf == 0)
            send_to_char("You are already human.\n\r",ch);
        else if (ch->position < POS_STANDING)
            send_to_char("You must stop fighting and stand before using Calm.\n\r",ch);
        else
            do_calm(ch,"");
    }
    else if (IS_SET(ch->act,PLR_WOLFMAN))
        send_to_char("You are already in werewolf form.\n\r",ch);
    else
    {
        do_rage(ch,"");
        if (!IS_SET(ch->act,PLR_WOLFMAN) &&
            ch->pcdata->wolf >= 0 && ch->pcdata->wolf < 100)
        {
            snprintf(buf,sizeof(buf),
                "Rage: %ld/100. Use shift werewolf again to build toward transformation.\n\r",
                ch->pcdata->wolf);
            send_to_char(buf,ch);
        }
    }
}
