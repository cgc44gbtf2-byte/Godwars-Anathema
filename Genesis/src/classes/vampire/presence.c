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

/* Vampire-only Presence powers extracted from clan.c without rule changes. */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"

void do_majesty( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if (IS_NPC(ch)) return;

    if (!IS_VAMPIRE(ch))
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
    if (get_disc(ch,DISC_PRESENCE) < 5)
    {
	send_to_char("You require level 5 Presence to use Majesty.\n\r",ch);
	return;
    }

    if (IS_ITEMAFF(ch,ITEMA_PEACE) )
    {
	send_to_char("You lower your Majesty.\n\r",ch);
	act("$n no longer seems so imposing.", ch, NULL, NULL, TO_ROOM);
	REMOVE_BIT(ch->itemaffect, ITEMA_PEACE);
	ch->pcdata->majesty = 120;
	return;
    }
    if (ch->pcdata->majesty > 0)
    {
	if (ch->pcdata->majesty > 1)
	    sprintf(buf,"You cannot raise your Majesty for another %li seconds.\n\r",ch->pcdata->majesty);
	else
	    sprintf(buf,"You cannot raise your Majesty for another 1 second.\n\r");
	send_to_char(buf,ch);
	return;
    }
    send_to_char("Your raise your Majesty.\n\r",ch);
    act("$n suddenly appears very imposing.", ch, NULL, NULL, TO_ROOM);
    SET_BIT(ch->itemaffect, ITEMA_PEACE);
    return;
}

void do_awe( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch)) return;

    if (!IS_VAMPIRE(ch))
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
    if (get_disc(ch,DISC_PRESENCE) < 1)
    {
	send_to_char("You require level 1 Presence to use Awe.\n\r",ch);
	return;
    }

    if (IS_EXTRA(ch,EXTRA_AWE) )
    {
	send_to_char("You are no longer awe-inspiring.\n\r",ch);
	act("$n no longer appears so awe-inspiring.", ch, NULL, NULL, TO_ROOM);
	REMOVE_BIT(ch->extra, EXTRA_AWE);
	return;
    }
    send_to_char("You are now awe-inspiring.\n\r",ch);
    act("$n suddenly appears very awe-inspiring.", ch, NULL, NULL, TO_ROOM);
    SET_BIT(ch->extra, EXTRA_AWE);
    return;
}

void do_summoning( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg [MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if (IS_NPC(ch)) return;

    if (!IS_VAMPIRE(ch))
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    if (get_disc(ch,DISC_PRESENCE) < 4)
    {
	send_to_char("You require level 4 Presence to use Summoning.\n\r",ch);
	return;
    }

    act( "A look of concentration crosses $n's face.", ch, NULL, NULL, TO_ROOM );

    if ( ( victim = get_char_world( ch, arg ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   ch->in_room == NULL
    ||   victim->level > (get_disc(ch,DISC_PRESENCE) * 140))
    {
	send_to_char( "Nothing happens!\n\r", ch );
	return;
    }

    if ( IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   victim->fighting != NULL
    ||   (!IS_NPC(victim) && !IS_EXTRA(victim, EXTRA_SUMMON) ))
    {
	send_to_char( "Nothing happens.\n\r", ch );
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Summoning on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( "$N has summoned you!", victim, NULL, ch,   TO_CHAR );
    do_look( victim, "auto" );
    WAIT_STATE( ch, 12 );
    if (IS_NPC(victim)) SET_BIT(victim->act, ACT_NOEXP);
    if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
    {
	guardian_message( victim );
	act("$n just used Summoning on you.",ch,NULL,victim,TO_VICT);
    }
    return;
}

void do_entrancement( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg [MIL];
    long sn, level;

    argument = one_argument(argument, arg);

    if (!IS_VAMPIRE(ch))
    {
	stc("Huh?\n\r", ch);
	return;
    }

    if (get_disc(ch, DISC_PRESENCE) < 3)
    {
	stc("You require level 3 Presence to use Entrancement.\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
	stc("They aren't here.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim)) return;

    if (IS_NPC(victim) && ch->pcdata->followers > 4)
    {
	stc("Nothing happens.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM))
    {
	stc("Nothing happens.\n\r", ch);
	return;
    }

    act("You stare deeply into $N's eyes.", ch, NULL, victim, TO_CHAR);
    act("$n stares deeply into $N's eyes.", ch, NULL, victim, TO_NOTVICT);
    act("$n stares deeply into your eyes.", ch, NULL, victim, TO_VICT);

    if ((sn = skill_lookup("charm person")) < 0) return;
    level = get_disc(ch,DISC_PRESENCE) * 60;

    if (IS_NPC(victim) && victim->level > level)
    {
	act("$N shrugs off your attempt to entrance $M and attacks you!", ch, NULL, victim, TO_CHAR);
	act("$N shrugs off $n's attempt to entrance $M and attacks $m.", ch, NULL, victim, TO_NOTVICT);
	multi_hit(victim, ch);
	return;
    }

    if (IS_VAMPIRE(victim) && get_disc(victim, DISC_PRESENCE) >= get_disc(ch, DISC_PRESENCE))
    {
	act("$N shrugs off your attempt to entrance $M.", ch, NULL, victim, TO_CHAR);
	act("$N shrugs off $n's attempt to entrance $M.", ch, NULL, victim, TO_NOTVICT);
	act("You shrug off $n's attempt to entrance you.", ch, NULL, victim, TO_VICT);
	return;
    }

    if (!IS_NPC(victim) && victim->pcdata->resist[WILL_PRESENCE] > 0)
    {
	act("$N shrugs off your attempt to entrance $M.",ch,NULL,victim,TO_CHAR);
	act("$N shrugs off $n's attempt to entrance $M.",ch,NULL,victim,TO_NOTVICT);
	act("You shrug off $n's attempt to entrance you.",ch,NULL,victim,TO_VICT);
	return;
    }

    if (!IS_NPC(victim) && IS_WILLPOWER(victim, RES_PRESENCE) && victim->pcdata->willpower[0] >= 10)
    {
	victim->pcdata->willpower[0] -= 10;
	victim->pcdata->resist[WILL_PRESENCE] = 60;
	if (!IS_WILLPOWER(victim, RES_TOGGLE))
	    REMOVE_BIT(victim->pcdata->resist[0], RES_PRESENCE);
	send_to_char("You burn some willpower to resist Entrancement.\n\r",victim);
	act("$N shrugs off your attempt to entrance $M.",ch,NULL,victim,TO_CHAR);
	act("$N shrugs off $n's attempt to entrance $M.",ch,NULL,victim,TO_NOTVICT);
	act("You shrug off $n's attempt to entrance you.",ch,NULL,victim,TO_VICT);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Entrancement on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    (*skill_table[sn].spell_fun) ( sn, level, ch, victim );
    WAIT_STATE( ch, 12 );

    if ( !IS_AFFECTED(victim, AFF_CHARM) )
    {
	act("$N shrugs off your attempt to entrance $M.",ch,NULL,victim,TO_CHAR);
	act("$N shrugs off $n's attempt to entrance $M.",ch,NULL,victim,TO_NOTVICT);
	act("You shrug off $n's attempt to entrance you.",ch,NULL,victim,TO_VICT);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Entrancement on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    act("$N gazes lovingly back at $n.",ch,NULL,victim,TO_NOTVICT);
    if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
    {
	guardian_message( victim );
	act("$n just used Entrancement on you.",ch,NULL,victim,TO_VICT);
    }

    if (IS_NPC(victim))
    {
	free_string(victim->lord);
	victim->lord = str_dup(ch->name);
	ch->pcdata->followers++;
    }
    return;
}

void do_dreadgaze( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg [MAX_INPUT_LENGTH];
    long sn;
    long level;

    argument = one_argument( argument, arg );

    if (IS_NPC(ch)) return;
    if (!IS_VAMPIRE(ch))
    {
	send_to_char( "Huh?\n\r", ch );
	return;
    }

    if (get_disc(ch,DISC_PRESENCE) < 2)
    {
	send_to_char("You require level 2 Presence to use Dread Gaze.\n\r",ch);
	return;
    }

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");

    if ( arg[0] == '\0' )
    {
	send_to_char( "Who do you wish to use Dread Gaze on?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (ch == victim)
    {
	send_to_char( "You're pretty scared of yourself!\n\r", ch );
	return;
    }

    if (is_safe(ch,victim)) return;

    if ( IS_AFFECTED(victim, AFF_FEAR) )
    {
	send_to_char( "They are already pretty scared.\n\r", ch );
	return;
    }

    act("You bare your fangs and snarl at $N.",ch,NULL,victim,TO_CHAR);
    act("$n bares $s fangs and snarls at $N.",ch,NULL,victim,TO_NOTVICT);
    act("$n bares $s fangs and snarls at you.",ch,NULL,victim,TO_VICT);


    if (IS_VAMPIRE(victim) && get_disc(victim,DISC_PRESENCE) >= get_disc(ch,DISC_PRESENCE))
    {
	send_to_char( "Your Dread Gaze has no effect on them.\n\r", ch );
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Dread Gaze on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if (!IS_NPC(victim) && victim->pcdata->resist[WILL_PRESENCE] > 0)
    {
	send_to_char( "Your Dread Gaze has no effect on them.\n\r", ch );
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Dread Gaze on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if (!IS_NPC(victim) && IS_WILLPOWER(victim, RES_PRESENCE) && 
	victim->pcdata->willpower[0] >= 10)
    {
	victim->pcdata->willpower[0] -= 10;
	victim->pcdata->resist[WILL_PRESENCE] = 60;
	if (!IS_WILLPOWER(victim, RES_TOGGLE))
	    REMOVE_BIT(victim->pcdata->resist[0], RES_PRESENCE);
	send_to_char( "Your Dread Gaze has no effect on them.\n\r", ch );
	send_to_char("You burn some willpower to resist Dread Gaze.\n\r",victim);
	if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
	{
	    guardian_message( victim );
	    act("$n just failed to use Dread Gaze on you.",ch,NULL,victim,TO_VICT);
	}
	return;
    }

    if ( ( sn = skill_lookup( "fear" ) ) < 0 ) return;
    level = get_disc(ch,DISC_PRESENCE) * 5;
    (*skill_table[sn].spell_fun) ( sn, level, ch, victim );
    WAIT_STATE( ch, 12 );
    if (!IS_NPC(victim) && ch != victim && IS_MORE(victim, MORE_GUARDIAN))
    {
	guardian_message( victim );
	act("$n just used Dread Gaze on you.",ch,NULL,victim,TO_VICT);
    }
    return;
}
