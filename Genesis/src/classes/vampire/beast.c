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

/* Vampire Beast callbacks retain their shared command-helper calls. */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"

void do_humanity( CHAR_DATA *ch, char *argument )
{
    char       arg [MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if (!IS_NPC(ch) && IS_VAMPIRE(ch) && ch->beast > 0
	&& ch->beast < 100 && number_range(1, 500) <= ch->beast)
    {
	if (ch->beast == 1)
	{
	    send_to_char("You have attained Golconda!\n\r", ch);
	}
	else
	    send_to_char("You feel slightly more in control of your beast.\n\r", ch);
	ch->beast -= 1;
    }
    return;
}

void do_beastlike( CHAR_DATA *ch, char *argument )
{
    char       arg [MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if (IS_VAMPIRE(ch) && ch->beast < 100 && number_range(1, 200) <= ch->beast)
    {
	if (ch->beast < 99)
	    send_to_char("You feel your beast take more control over your actions.\n\r", ch);
	else
	    send_to_char("Your beast has fully taken over control of your actions!\n\r", ch);
	ch->beast += 1;
	if (get_disc(ch,DISC_PROTEAN) > 0 && 
	    !IS_VAMPAFF(ch, VAM_NIGHTSIGHT) ) do_nightsight(ch,"");
	if (!IS_VAMPAFF(ch, VAM_FANGS)) do_fangs(ch,"");
	if (get_disc(ch,DISC_PROTEAN) > 1 && 
	    !IS_VAMPAFF(ch, VAM_CLAWS) ) do_claws(ch,"");
    }
    return;
}

void do_beastwithin( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );

    if ( IS_NPC(ch) )
	return;

    if (!IS_VAMPIRE(ch))
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    if (get_disc(ch,DISC_ANIMALISM) < 4)
    {
	send_to_char("You require level 4 Animalism to use Beast Within.\n\r",ch);
	return;
    }

    if ( arg[0] == '\0' ) victim = ch;
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (!IS_VAMPIRE(victim) && !IS_WEREWOLF(victim))
    {
	send_to_char("But they have no beast to call forth!\n\r",ch);
	return;
    }

    if (IS_WEREWOLF(victim) && IS_SET(victim->act, PLR_WOLFMAN))
    {
	send_to_char("Their beast is already controlling them.\n\r",ch);
	return;
    }

    if ( !IS_WEREWOLF(victim) && victim->pcdata->wolf > 0 )
    {
	if (ch == victim)
	    send_to_char("Your beast is already controlling you.\n\r",ch);
	else
	    send_to_char("Their beast is already controlling them.\n\r",ch);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Beast Within on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if (!IS_WEREWOLF(victim) && (victim->beast < 20 ))
    {
	if (ch == victim)
	    send_to_char("You have too much control over your beast to use Beast Within.\n\r",ch);
	else
	    send_to_char("They have too much control over their beast to use Beast Within on.\n\r",ch);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Beast Within on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if (victim->pcdata->resist[WILL_ANIMALISM] > 0 && ch!=victim)
    {
	send_to_char("You failed.\n\r",ch);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Beast Within on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if (IS_WILLPOWER(victim, RES_ANIMALISM) && ch!= victim&& 
	victim->pcdata->willpower[0] >= 10)
    {
	victim->pcdata->willpower[0] -= 10;
	victim->pcdata->resist[WILL_ANIMALISM] = 60;
	if (!IS_WILLPOWER(victim, RES_TOGGLE))
	    REMOVE_BIT(victim->pcdata->resist[0], RES_ANIMALISM);
	send_to_char("You failed.\n\r",ch);
	send_to_char("You burn some willpower to resist Beast Within.\n\r",victim);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Beast Within on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if (IS_VAMPIRE(victim) && get_disc(victim,DISC_PROTEAN) > 0 && 
	!IS_VAMPAFF(victim, VAM_NIGHTSIGHT) ) do_nightsight(victim,"");
    if (IS_VAMPIRE(victim) && !IS_VAMPAFF(victim, VAM_FANGS)) do_fangs(victim,"");
    if (IS_VAMPIRE(victim) && get_disc(victim,DISC_PROTEAN) > 1 && 
	!IS_VAMPAFF(victim, VAM_CLAWS) ) do_claws(victim,"");
    if (IS_WEREWOLF(victim))
    {
	victim->pcdata->wolf = number_range(150,200);
	do_werewolf(victim,"");
    }
    else
    {
	if (victim->beast > 0) do_beastlike(victim,"");
	send_to_char("You bare your fangs and growl as your inner beast consumes you.\n\r",victim);
	act("$n bares $s fangs and growls as $s inner beast consumes $m.",victim,NULL,NULL,TO_ROOM);
 	victim->pcdata->wolf += number_range(10,20);
    }
    if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
    {
	guardian_message( victim );
	act("$n just used Beast Within on you.",ch,NULL,victim,TO_VICT);
    }
    WAIT_STATE(ch,12);
    return;
}
