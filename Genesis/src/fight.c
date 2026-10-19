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

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/*
 * Local functions.
 */
void	dam_message	args( ( CHAR_DATA *ch, CHAR_DATA *victim, long dam, long dt, char *message ) );

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;
    CHAR_DATA *rch;
    CHAR_DATA *rch_next;

    for (ch = first_fight; ch != NULL; ch = ch_next)
    {
	ch_next	= ch->next_fight;

        if (IS_COMBAT_PAUSED(ch)) continue;

	if ((victim = ch->fighting) == NULL || victim->in_room != ch->in_room)
	{
	    stop_fighting(ch, FALSE);
	    continue;
	}

	if (IS_AWAKE(ch) && IS_AWAKE(victim) && ch->in_room == victim->in_room)
	{
	    if (--ch->init <= 0)
	    {
		multi_hit(ch, victim);
		ch->init = get_speed(ch);
	    }
	    else continue;
	}
	else stop_fighting(ch, FALSE);

	if ((victim = ch->fighting) == NULL)
	{
	    stop_fighting(ch, FALSE);
	    continue;
	}
	if (!IS_NPC(ch) && !IS_NPC(victim)) ch->fight_timer = 30;

	/*
	 * Fun for the whole family!
	 */
	for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
	{
	    rch_next = rch->next_in_room;

	    if (IS_AWAKE(rch) && rch->fighting == NULL)
	    {
		/* Auto-assist others in group. */
		if (is_same_group(ch, rch) && !is_same_group(rch, victim))
		    multi_hit(rch, victim);
		else if (IS_NPC(ch) && IS_NPC(rch))
		{
		    /* NPC's assist NPC's of same type or 1/16 chance regardless. */
		    if (rch->pIndexData == ch->pIndexData || number_bits(4) == 0)
		    {
			CHAR_DATA *target = combat_assist_target(rch, victim);

			if (target != NULL) multi_hit(rch, target);
		    }
		}
	    }
	}
    }

    return;
}



/*
 * Do one group of attacks.
 */
void multi_hit( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *wield;
    long sn, dt, style;

    if (ch == NULL || victim == NULL || IS_COMBAT_PAUSED(ch)) return;
    if (ch->position < POS_SLEEPING || is_safe(ch, victim)) return;

    if (IS_NPC(ch)) /* Temporary I swear */
    {
	if ((wield = get_eq_char(ch, WEAR_WEAPON)) && wield->item_type == ITEM_WEAPON) dt = wield->value[3];
	else dt = TYPE_UNARMED;
	one_hit(ch, victim, dt, "attack");
	if (number_range(251, 500) <= ch->level) one_hit(ch, victim, dt, "attack");
	if (number_range(501, 1000) <= ch->level) one_hit(ch, victim, dt, "attack");
	if (number_range(1001, 2500) <= ch->level) one_hit(ch, victim, dt, "attack");
	if (number_range(2501, 5000) <= ch->level) one_hit(ch, victim, dt, "attack");
	return;
    }

    style = ch->style[0];
    if ((wield = get_eq_char(ch, WEAR_WEAPON)) != NULL)
    {
	if (style == STYLE_UNARMED || style == STYLE_NATURAL)
	{
	    stc("Erm...'Oh Shit(weapon)' bug alert.  Inform Akuma.\n\r", ch);
	    return;
	}
	dt = wield->value[3];
    }
    else
    {
	if (style == STYLE_UNARMED || style == STYLE_SHIELD) dt = TYPE_UNARMED;
	else if (style == STYLE_NATURAL) dt = TYPE_SLASH;
	else
	{
	    stc("Erm...'Oh Shit(unarmed)' bug alert.  Inform Akuma.\n\r", ch);
	    return;
	}
    }

    improve_stance(ch);
    improve_style(ch);
    improve_weapon(ch, dt);

    one_hit(ch, victim, dt, "1");
    switch(style)
    {
	default:
	    stc("Erm...'Oh Shit(style)' bug alert.  Inform Akuma.\n\r", ch);
	    return;
	case STYLE_UNARMED:
	    if (number_range(101, 200) <= get_style(ch, style)) one_hit(ch, victim, dt, "1");
	    break;
	case STYLE_NATURAL:
	    break;
	case STYLE_ONEHAND:
	    if (number_range(1, 100) <= get_style(ch, style)) one_hit(ch, victim, dt, "1");
	    if (number_range(101, 200) <= get_style(ch, style)) one_hit(ch, victim, dt, "1");
	    break;
	case STYLE_SHIELD:
	    break;
	case STYLE_TWOHAND:
	    break;
	case STYLE_TWOWEAPON:
	    if (number_range(51, 150) <= get_style(ch, style)) one_hit(ch, victim, dt, "1");
	    if (number_range(151, 250) <= get_style(ch, style)) one_hit(ch, victim, dt, "1");

	    if ((wield = get_eq_char(ch, WEAR_HOLD)) == NULL || wield->item_type != ITEM_WEAPON)
	    {
		stc("Erm...'Oh Shit(two weapon)' bug alert.  Inform Akuma.\n\r", ch);
		return;
	    }
	    dt = wield->value[3];
	    improve_weapon(ch, dt);

	    if (number_range(1, 100) <= get_style(ch, style)) one_hit(ch, victim, dt, "2");
	    if (number_range(101, 200) <= get_style(ch, style)) one_hit(ch, victim, dt, "2");
	    break;
    }

    if (victim == NULL || victim->position != POS_FIGHTING) return;
    if (wield != NULL && wield->value[0] >= 1)
    {
	sn = wield->value[0] % 1000;
	if (sn != 0 && victim->position == POS_FIGHTING) (*skill_table[sn].spell_fun) (sn, wield->level, ch, victim);
    }
    else if (style == STYLE_NATURAL && IS_WEREWOLF(ch) && get_tribe(ch, TRIBE_BLACK_FURIES) >= 3)
	spell_poison(gsn_poison, 25, ch, victim);

    if (!IS_NPC(ch) && IS_VAMPAFF(ch, VAM_FANGS) && number_range(1, 3) <= 2) one_hit(ch, victim, TYPE_PIERCE, "fangs");
    if (IS_DRAGON(ch))
    {
	if (IS_DRAGAFF(ch, DRAG_HORNSOUT) && number_range(1, 3) == 1) one_hit(ch, victim, TYPE_PIERCE, "horns");
	if (IS_DRAGAFF(ch, DRAG_WINGSOUT) && number_range(1, 3) == 1) one_hit(ch, victim, TYPE_BLUNT, "wing buffet");
    }
    if (IS_MONK(ch) && get_monk(ch, MONK_ART) >= 8 && number_range(1, 3) == 1) one_hit(ch, victim, TYPE_HIT, "wave fist");
    if (IS_MAGE(ch) && IS_XTRA(ch, XTRA_HOLY_SMOTE) && number_range(1, 2) == 1) one_hit(ch, victim, TYPE_HIT, "holy sword");
    if (IS_WEREWOLF(ch) && get_tribe(ch, TRIBE_WENDIGOS) >= 5 && IS_SET(world_affects, WORLD_RAIN) && IS_OUTSIDE(ch))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
	    vch_next = vch->next_in_room;
	    if (is_safe(ch, vch)) continue;
	    if (vch->fighting != ch) continue;
	    if (number_range(1, 2) == 1) continue;
	    one_hit(ch, vch, TYPE_HIT, "lightning blast");
	}
    }

    if (IS_NINJA(ch))
    {
	if (IS_VAMPAFF(ch, VAM_CLAWS) && number_range(1, 3) == 1) one_hit(ch, victim, TYPE_SLASH, "tsume");
	if (get_ninja(ch, NINJA_NINGENNO) >= 6 && number_range(1, 3) == 1)
	{
	    one_hit(ch, victim, TYPE_PIERCE, "shuriken");
	    if (victim == NULL || victim->position != POS_FIGHTING) return;
	    spell_poison(gsn_poison, 25, ch, victim);
	}
    }
    
    if (IS_POLYAFF(ch, POLY_SERPENT))
    {
	one_hit(ch, victim, TYPE_SLASH, "forked tongue");
	one_hit(ch, victim, TYPE_SLASH, "forked tongue");
    }

    if (IS_VAMPIRE(ch) && IS_XTRA(ch, XTRA_TENEBROUS))
    {
	one_hit(ch, victim, TYPE_SLASH, "tendril of shadow");
	one_hit(ch, victim, TYPE_BLUNT, "shadow arm");
    }

    if (IS_VAMPIRE(ch) && get_disc(ch, DISC_CHIMERSTRY) >= 2)
	one_hit(ch, victim, TYPE_SLASH, "chimerstral dagger");

    if (IS_DROW(ch) && get_drow(ch, DROW_BITE) && number_range(1, 4) == 1)
	one_hit(ch, victim, TYPE_PIERCE, "spider fangs");

    return;
}



/*
 * Hit one guy once.
 * Makes a single attack, check avoid, damage limited by damcap.
 */
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, long dt, char *message )
{
    OBJ_DATA *wield;
    long dam, max_dam;
    bool weap = TRUE;

    if (ch == NULL || IS_COMBAT_PAUSED(ch)) return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if (victim == NULL || victim->position == POS_DEAD || ch->in_room != victim->in_room) return;

    bool melee = highlander_melee_attack(dt, message);

    /*
     * Check if it is a weapon attack.
     */
    if (IS_NPC(ch)) wield = get_eq_char(ch, WEAR_WEAPON);
    else if (message[0] == '1')
    {
	wield = get_eq_char(ch, WEAR_WEAPON);
	if (wield == NULL || wield->item_type != ITEM_WEAPON)
	{
	    wield = NULL;
	    message = str_dup("");
	}
	else message = str_dup(wield->short_descr);
    }
    else if (message[0] == '2')
    {
	wield = get_eq_char(ch, WEAR_HOLD);
	if (wield == NULL || wield->item_type != ITEM_WEAPON)
	{
	    wield = NULL;
	    message = str_dup("");
	}
	else message = str_dup(wield->short_descr);
    }
    else
    {
	wield = NULL;
	weap = FALSE;
    }

    if (message[0] == '\0')
    {
	if (IS_NPC(ch)) message = str_dup("fist");
	else if (IS_VAMPAFF(ch, VAM_CLAWS) && IS_MORE(ch, MORE_HELIOS)) message = str_dup("flaming claws");
	else if (IS_CRAFT(ch, CRAFT_BONETALONS)) message = str_dup("bone talons");
	else if (IS_VAMPAFF(ch, VAM_CLAWS)) message = str_dup("claws");
	else if (IS_MORE(ch, MORE_HELIOS)) message = str_dup("flaming fist");
	else if (IS_MONK(ch) && get_monk(ch, MONK_ART) >= 6) message = str_dup("adamantium fist");
	else  message = str_dup("fist");
    }
    if (!str_cmp(message, "backstab") || !str_cmp(message, "circle")) weap = TRUE;

    /* A riposte changes one ordinary weapon strike, not a nested attack.
     * It cannot start another counter; all native damage/cap/safety paths
     * below still apply. Natural/contact attacks qualify; magic and ranged
     * attacks are excluded by the original-label classifier above. */
    bool riposte = highlander_counter_strike(ch, victim, wield, dt);
    if (melee && !riposte && highlander_counter_parry(ch, victim, wield, dt)) return;
    if (!riposte && check_avoid(ch, victim, weap))
    {
	if (IS_NPC(ch) || !IS_SET(ch->act, PLR_BRIEF)) act("$N avoids your blow.", ch, NULL, victim, TO_CHAR);
	if (IS_NPC(victim) || !IS_SET(victim->act, PLR_BRIEF)) act("You avoid $n's blow.", ch, NULL, victim, TO_VICT);
	hurt_fighting(ch, victim);
	return;
    }

    /*
     * Calc damage.
     */
    if (IS_NPC(ch))
    {
	dam = mod_damcap(ch, victim);
	dam = number_range(dam / 2, 3 * dam / 2);
	if (wield != NULL) dam *= 1.5;
    }
    else
    {
	if (wield != NULL)
	{
	    dam = number_range(wield->value[1] * 5, wield->value[2] * 5);
	}
	else if (dt == TYPE_UNARMED)
	{
	    if (IS_MONK(ch) && get_monk(ch, MONK_ART) >= 6) dam = number_range(50, 100);
	    else if (IS_MORE(ch, MORE_HELIOS)) dam = number_range(50, 100);
	    else dam = number_range(15, 25);
	}
	else
	{
	    dam = number_range(25, 50);
	}

	dam += char_damroll(ch);

	if (IS_MAGE(ch) && ch->spheres[MENT] >= 1) dam *= 2;
	if (IS_MONK(ch))
	{
	    if (get_monk(ch, MONK_ART) >= 7) dam *= 2.5;
	    else if (get_monk(ch, MONK_ART) >= 1) dam *= 1.5;
	}
	if (IS_VAMPIRE(ch))     if (get_disc(ch, DISC_POTENCE) > 0) dam *= 1 + (get_disc(ch, DISC_POTENCE) * .4);
	if (IS_ELEM(ch))	if (ch->pcdata->powers[ELEM_FIRE] >= 5) dam *= ch->pcdata->powers[ELEM_FIRE] * .3;
	if (IS_DRAGON(ch))      if (IS_POLYAFF(ch, POLY_DRAGONFORM)) dam *= 1 + ((13 - ch->vampgen) * .2);
	if (IS_DROW(ch))	if (get_drow(ch, DROW_MIGHT)) dam *= 2.5;
	if (IS_NINJA(ch))       dam *= 1 + (get_ninja(ch, NINJA_CHIKYU) * .2);
	if (IS_HIGHLANDER(ch))	dam *= 1 + (get_highland(ch, HIGH_WEAPON) * .3);
	dam *= 1 + (.5 * char_stat(ch, STAT_STR));
    }

    dam = dambonus(ch, victim, dam);

    max_dam = mod_damcap(ch, victim);
    if (!IS_NPC(ch) && dam > max_dam) dam = max_dam;

    if (!IS_NPC(ch))
    {
	if (ch->style[0] == STYLE_NATURAL && weap) dam *= 1 + (ch->style[STYLE_NATURAL] * .0025);
	if (ch->style[0] == STYLE_TWOHAND && weap) dam *= 1 + (ch->style[STYLE_TWOHAND] * .01);
    }
    if (!IS_NPC(victim))
    {
	if (victim->style[0] == STYLE_SHIELD) dam /= 1 + (victim->style[STYLE_SHIELD] * .005);
    }

    if (!str_cmp(message, "backstab")) dam *= 2;

    if (dt >= TYPE_UNARMED && dt <= TYPE_BLUNT && IS_AFFECTED(victim, AFF_SANCTUARY)) dam *= .7;

    if (is_affected(ch, gsn_friction_curse)) //friction curse turns some of your movement into damage
 	hurt_person(ch, ch, 100);


    damage(ch, victim, dam, dt, message);
    tail_chain();
    return;
}



void dam_message( CHAR_DATA *ch, CHAR_DATA *victim, long dam, long type, char *mess )
{
    CHAR_DATA *rch;
    char buf1a[512], buf1b[512], buf2[512], buf3[512];
    const char *verb;
    const char *vs;
    const char *dammess = mess != NULL ? mess : "attack";
    long vrand = number_range(0, 2);

    /* Gathering wording compressed into Genesis's unchanged seven bands. */
    if (dam <= 50) vs = " like a wimp.";
    else if (dam <= 250) vs = " lightly.";
    else if (dam <= 500) vs = " hard.";
    else if (dam <= 1000) vs = " incredibly hard!";
    else if (dam <= 2500) vs = " with ULTIMATE power!";
    else if (dam <= 5000) vs = " with UNGODLY vengeance!";
    else vs = " with UNMERCIFUL strength!!";

    switch(type)
    {
	default:		verb = "BUGGED";
				break;
	case TYPE_HIT:		if (vrand == 0) verb = "sucks";
				else if (vrand == 1) verb = "blows";
				else verb = "swallows";
				break;
	case TYPE_UNARMED:	if (vrand == 0) verb = "hits";
				else if (vrand == 1) verb = "strikes";
				else verb = "punishes";
				break;
	case TYPE_SLASH:	if (vrand == 0) verb = "cuts";
				else if (vrand == 1) verb = "slashes";
				else verb = "slices";
				break;
	case TYPE_PIERCE:	if (vrand == 0) verb = "stabs";
				else if (vrand == 1) verb = "pierces";
				else verb = "impales";
				break;
	case TYPE_BLUNT:	if (vrand == 0) verb = "pounds";
				else if (vrand == 1) verb = "crushes";
				else verb = "smashes";
				break;
	case TYPE_SPELL:	if (vrand == 0) verb = "blasts";
				else if (vrand == 1) verb = "incinerates";
				else verb = "explodes";
				break;
    }

    snprintf(buf1a, sizeof(buf1a), "$n's %s %s $t%s", dammess, verb, vs);
    snprintf(buf1b, sizeof(buf1b), "$n's %s %s $t%s [#0%li#n]", dammess, verb, vs, dam);
    snprintf(buf2, sizeof(buf2), "Your %s %s $N%s [#g%li#n]", dammess, verb, vs, dam);
    snprintf(buf3, sizeof(buf3), "$n's %s %s you%s [#r%li#n]", dammess, verb, vs, dam);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
	if(rch == ch || rch == victim) continue;
	if (IS_NPC(rch) || !IS_SET(rch->act, PLR_SDAM))
	{
	    if (!can_see(rch, victim)) act(buf1a, ch, "someone", rch, TO_VICT);
	    else if (!IS_NPC(victim))
	    {
		if (IS_AFFECTED(victim, AFF_POLYMORPH))
		    act(buf1a, ch, victim->morph, rch, TO_VICT);
		else act(buf1a, ch, victim->name, rch, TO_VICT);
	    }
	    else act(buf1a, ch, victim->short_descr, rch, TO_VICT);
	}
	else
	{
	    if (!can_see(rch, victim)) act(buf1b, ch, "someone", rch, TO_VICT);
	    else if (!IS_NPC(victim))
	    {
		if (IS_AFFECTED(victim, AFF_POLYMORPH))
		    act(buf1b, ch, victim->morph, rch, TO_VICT);
		else act(buf1b, ch, victim->name, rch, TO_VICT);
	    }
	    else act(buf1b, ch, victim->short_descr, rch, TO_VICT);
	}
    }
    act(buf2, ch, NULL, victim, TO_CHAR);
    act(buf3, ch, NULL, victim, TO_VICT);
    return;
}


/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf [MAX_STRING_LENGTH];

    if (IS_AFFECTED(victim,AFF_FLYING) || (IS_VAMPIRE(victim) && IS_VAMPAFF(victim,VAM_FLYING))) return;

    if (IS_NPC(ch) && victim->level > 2 && number_percent() > 5) return;

    if ( victim->wait == 0)
    {
    	sprintf(buf,"$n trips you and you go down!");
    	ADD_COLOUR(ch,buf,WHITE);
    	act( buf, ch, NULL, victim, TO_VICT    );
    	sprintf(buf,"You trip $N and $E goes down!");
    	ADD_COLOUR(ch,buf,WHITE);
    	act( buf,  ch, NULL, victim, TO_CHAR    );
    	sprintf(buf,"$n trips $N and $E goes down!");
    	ADD_COLOUR(ch,buf,WHITE);
    	act( buf,  ch, NULL, victim, TO_NOTVICT );

	WAIT_STATE( ch,     2 * PULSE_VIOLENCE );
	WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
	victim->position = POS_RESTING;
    }

    return;
}

void do_kill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");
    if ( arg[0] == '\0' )
    {
	send_to_char( "Kill whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "You cannot kill yourself!\n\r", ch );
	return;
    }

    if (IS_AFFECTED(ch, AFF_FEAR))
    {
	send_to_char( "No way, you're too scared!\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
	return;

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
	act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( ch->position == POS_FIGHTING )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    autostance(ch);
    autostance(victim);

    WAIT_STATE(ch, PULSE_VIOLENCE);

    if (!IS_NPC(ch) && IS_WEREWOLF(ch) && get_tribe(ch, TRIBE_BLACK_FURIES) >= 4 && victim->position == POS_STANDING)
    {
	act("You charge into $N, knocking $M from $S feet.",ch,NULL,victim,TO_CHAR);
	act("$n charge into $N, knocking $M from $S feet.",ch,NULL,victim,TO_NOTVICT);
	act("$n charge into you, knocking you from your feet.",ch,NULL,victim,TO_VICT);
	multi_hit(ch, victim);
	multi_hit(ch, victim);
	return;
    }
    else multi_hit(ch, victim);
    return;
}

void do_redirect( CHAR_DATA *ch, char *argument )
{
    char arg[MIL];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (ch->position != POS_FIGHTING)
    {
	stc("You must be fighting redirect your attacks\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
	stc("Redirect your attacks toward whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
	stc("That person is not here.\n\r", ch);
	return;
    }

    if (ch->fighting == victim)
    {
	act("You are already fighting $m.\n\r", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (is_safe(ch, victim)) return;

    if (victim == ch)
    {
	stc("Killing yourself is ill advised.\n\r",ch);
 	return;
    }

    ch->fighting = victim;
    act("You redirect your attacks toward $N!", ch, NULL, victim, TO_CHAR);
    act("$n redirects $s attacks toward $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n redirects $s attacks toward you!", ch, NULL, victim, TO_VICT);

    WAIT_STATE(ch, 12);
    return;
}

void do_flee( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim;
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    long attempt;

    if ((victim = ch->fighting) == NULL)
    {
	if (ch->position == POS_FIGHTING)
	    ch->position = POS_STANDING;
	stc("You aren't fighting anyone.\n\r", ch);
	return;
    }

    if (ch->enrage_timer > 0)
    {
	stc("You snarl with rage and continue fighting.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_WEBBED))
    {
	send_to_char("You are unable to move with all this sticky webbing on.\n\r",ch);
	return;
    }
    else if (IS_AFFECTED(ch, AFF_SHADOW))
    {
	send_to_char("You are unable to move with these tendrils of darkness entrapping you.\n\r",ch);
	return;
    }
    if (!IS_NPC(ch) && ch->pcdata->wolf >= 0)
    {
	if (IS_VAMPIRE(ch) && number_percent() <= ch->beast && (number_range(1, 20) > 10))
	{
	    send_to_char("Your inner beast refuses to let you run!\n\r",ch);
	    WAIT_STATE(ch, 18);
	    return;
	}
	else if (IS_WEREWOLF(ch) && number_percent() <= ch->pcdata->wolf / 4)
	{
	    send_to_char("Your rage is too great!\n\r",ch);
	    WAIT_STATE(ch, 18);
	    return;
	}
    }
    if (IS_WEREWOLF(victim) && number_percent() < get_tribe(victim, TRIBE_GET_OF_FENRIS) * 10)
    {
	stc("You find yourself moving too slow to flee!.\n\r",ch);
	stc("You Halt the Cowards Flight.\n\r",victim);
	WAIT_STATE(ch, 18);
	return;
    }

    was_in = ch->in_room;
    for (attempt = 0; attempt < 6; attempt++)
    {
	EXIT_DATA *pexit;
	long door;

	door = number_door( );
	if ((pexit = was_in->exit[door]) == 0
	 || pexit->to_room == NULL || IS_SET(pexit->exit_info, EX_CLOSED)
	 || IS_SET(pexit->to_room->room_flags, ROOM_SAFE)
	 || (IS_NPC(ch) && IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB)))
	    continue;

	move_char(ch, door);
	if ((now_in = ch->in_room) == was_in) continue;

	ch->in_room = was_in;
	act("$n has fled $t!", ch, dir_name[door], NULL, TO_ROOM);
	act("You flee to the $t!", ch, dir_name[door], NULL, TO_CHAR);
	ch->in_room = now_in;

	stc("You flee from combat!\n\r", ch);

	for (wch = was_in->people; wch != NULL; wch = wch_next)
	{
	    wch_next = wch->next_in_room;

	    if (IS_NPC(wch)) continue;
	    if (wch->fighting != ch) continue;
	    if (!IS_SET(wch->act, PLR_AUTOFOLLOW)) continue;
	    if (number_percent() < 50) {stop_fighting(wch, TRUE); continue;}

	    stc("You flee, but your enemy follows you!\n\r", ch);
	    stc("Your opponent tries to flee but you follow them!\n\r", wch);
	    move_char(wch, door);
	}

	WAIT_STATE(ch, 12);
	return;
    }

    stc("You were unable to escape!\n\r", ch);
    WAIT_STATE(ch, 18);
    return;
}

void do_rescue( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    char arg[MAX_INPUT_LENGTH];
    bool found = FALSE;

    one_argument( argument, arg );

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");

    if ( arg[0] == '\0' )
    {
	send_to_char( "Rescue whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "What about fleeing instead?\n\r", ch );
	return;
    }

    if ( !IS_NPC(ch) && IS_NPC(victim) )
    {
	send_to_char( "They don't need your help!\n\r", ch );
	return;
    }

    if ( ch->fighting == victim )
    {
	send_to_char( "Too late.\n\r", ch );
	return;
    }

    if (victim->fighting == NULL)
    {
	send_to_char( "That person is not fighting right now.\n\r", ch );
	return;
    }

    for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
    {
	fch_next = fch->next_in_room;

	if (fch == ch || fch == victim) continue;
	if (fch->fighting != victim) continue;
	if (auto_safe(ch, fch)) continue;

	found = TRUE;

	act( "You rescue $N!",  ch, NULL, victim, TO_CHAR    );
	act( "$n rescues you!", ch, NULL, victim, TO_VICT    );
	act( "$n rescues $N!",  ch, NULL, victim, TO_NOTVICT );

	stop_fighting( ch, FALSE );
	stop_fighting( fch, FALSE );
	stop_fighting( victim, FALSE );

	set_fighting( ch, fch );
	set_fighting( fch, ch );
	break;
    }

    if (!found) stc("They aren't being attacked.\n\r", ch);
    WAIT_STATE(ch, 8);

    return;
}

void do_kick( CHAR_DATA *ch, char *argument )
{
    char arg[MIL];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");

    if (arg[0] == '\0' && ch->fighting == NULL)
    {
	send_to_char( "Who do you wish to kick?\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}
    }
    if ( ch == victim )
    {
	send_to_char( "Kick YOURSELF?\n\r", ch );
	return;
    }

    WAIT_STATE( ch, 12);
    one_hit(ch, victim, TYPE_UNARMED, "kick");
    return;
}

void do_punch( CHAR_DATA *ch, char *argument )
{
    char arg [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    long dam;

    one_argument(argument,arg);

    if ( IS_NPC(ch) ) return;

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "Nobody here by that name.\n\r", ch );
	return;
    }

    if ( ch == victim )
    {
	send_to_char( "You cannot punch yourself!\n\r", ch );
	return;
    }

    if ( victim->hit < victim->max_hit )
    {
	send_to_char( "They are hurt and suspicious.\n\r", ch );
	return;
    }

    if ( victim->position < POS_FIGHTING )
    {
	send_to_char( "You can only punch someone who is standing.\n\r", ch );
	return;
    }

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");

    if (is_safe(ch, victim)) return;

    act("You draw your fist back and aim a punch at $N.",ch,NULL,victim,TO_CHAR);
    act("$n draws $s fist back and aims a punch at you.",ch,NULL,victim,TO_VICT);
    act("$n draws $s fist back and aims a punch at $N.",ch,NULL,victim,TO_NOTVICT);
    WAIT_STATE(ch, 12);

    dam = number_range(1, 4);
    dam += char_damroll(ch);
    if (!IS_AWAKE(victim)) dam *= 2;
    if (dam <= 0) dam = 1;

    if ((IS_WEREWOLF(victim) && get_tribe(victim, TRIBE_GET_OF_FENRIS) >= 5) 
     || (IS_VAMPIRE(victim) && get_disc(victim, DISC_FORTITUDE) >= 7))
	dam = 0;

    if (highlander_counter_parry(ch, victim, NULL, TYPE_BLUNT)) return;
    damage(ch, victim, dam, TYPE_BLUNT, "punch");
    if (victim == NULL || victim->position == POS_DEAD || dam < 1) return;
    if (victim->position == POS_FIGHTING) stop_fighting(victim, TRUE);
    act("You fall to the ground stunned!", victim, NULL, NULL, TO_CHAR);
    act("$n falls to the ground stunned!", victim, NULL, NULL, TO_ROOM);
    victim->position = POS_STUNNED;
    return;
}

void do_berserk( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *wield;
    char arg[MIL];
    long number_hit = 0, style = ch->style[0], dt = TYPE_HIT;

    argument = one_argument(argument, arg);

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch, "");

    WAIT_STATE(ch, 18);

    act("You go BERSERK!", ch, NULL, NULL, TO_CHAR);
    act("$n goes BERSERK!", ch, NULL, NULL, TO_ROOM);

    if ((wield = get_eq_char(ch, WEAR_WEAPON)) == NULL || wield->item_type != ITEM_WEAPON)
    {
	if (style == STYLE_UNARMED || style == STYLE_SHIELD) dt = TYPE_UNARMED;
	if (style == STYLE_NATURAL) dt = TYPE_SLASH;
    }
    else dt = wield->value[3];

    if (dt == TYPE_HIT)
    {
	stc("Style Bug(berserk).  Inform Akuma.\n\r", ch);
	return;
    }

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
	vch_next = vch->next_in_room;
	if (number_hit >= 5) continue;
	if (!IS_NPC(vch) && vch->pcdata->chobj != NULL) continue;
	if (ch == vch) continue;
	if ((IS_NPC(ch) && IS_NPC(vch)) || (!IS_NPC(ch) && !IS_NPC(vch))) continue;
	if (!can_see(ch, vch)) continue;

	one_hit(ch, vch, dt, "1");	/* Multi_hit is too much */
	number_hit++;
    }
    return;
}



/* Hurl skill by KaVir */
void do_hurl( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA       *victim;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA       *pexit;
    EXIT_DATA       *pexit_rev;
    char            buf       [MAX_INPUT_LENGTH];
    char            direction [MAX_INPUT_LENGTH];
    char            arg1      [MAX_INPUT_LENGTH];
    char            arg2      [MAX_INPUT_LENGTH];
    long             door;
    long             rev_dir;
    long             dam;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch,"");

    if ( arg1[0] == '\0' )
    {
	send_to_char("Who do you wish to hurl?\n\r", ch);
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    {
	send_to_char("They are not here.\n\r", ch);
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "How can you hurl yourself?\n\r", ch );
	return;
    }

    if (victim->hit < victim->max_hit || (victim->position == POS_FIGHTING && victim->fighting != ch))
    {
	act( "$N is hurt and suspicious, and you are unable to approach $M.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( is_safe( ch, victim ) )
      return;

    if (!IS_NPC(victim) && IS_WEREWOLF(victim) && get_tribe(victim, TRIBE_GET_OF_FENRIS) >= 5)
    {
	send_to_char("You are unable to get their feet of the ground.\n\r",ch);
	return;
    }

    if ( IS_NPC(victim) && victim->level > 900 )
    {
	send_to_char("You are unable to get their feet of the ground.\n\r",ch);
	return;
    }

    WAIT_STATE( ch, 9);

    rev_dir = 0;

    if ( arg2[0] == '\0' )
	door = number_range(0,3);
    else
    {
    	if      ( !str_cmp( arg2, "n" ) || !str_cmp( arg2, "north" ) )
	    door = 0;
    	else if ( !str_cmp( arg2, "e" ) || !str_cmp( arg2, "east" ) )
	    door = 1;
    	else if ( !str_cmp( arg2, "s" ) || !str_cmp( arg2, "south" ) )
	    door = 2;
    	else if ( !str_cmp( arg2, "w" ) || !str_cmp( arg2, "west" ) )
	    door = 3;
    	else
    	{
	    send_to_char("You can only hurl people north, south, east or west.\n\r", ch);
	    return;
    	}
    }

    if (door == 0) {sprintf(direction,"north");rev_dir = 2;}
    if (door == 1) {sprintf(direction,"east");rev_dir = 3;}
    if (door == 2) {sprintf(direction,"south");rev_dir = 0;}
    if (door == 3) {sprintf(direction,"west");rev_dir = 1;}

    if (( pexit = ch->in_room->exit[door]) == NULL || (to_room = pexit->to_room) == NULL)
    {
	sprintf(buf,"$n hurls $N into the %s wall.", direction);
	act(buf,ch,NULL,victim,TO_NOTVICT);
	sprintf(buf,"You hurl $N into the %s wall.", direction);
	act(buf,ch,NULL,victim,TO_CHAR);
	sprintf(buf,"$n hurls you into the %s wall.", direction);
	act(buf,ch,NULL,victim,TO_VICT);
	dam = number_range(ch->level, (ch->level * 4));
	victim->hit = victim->hit - dam;
	update_pos(victim);
    	if (IS_NPC(victim) && !IS_NPC(ch)) ch->mkill = ch->mkill + 1;
    	if (!IS_NPC(victim) && IS_NPC(ch)) victim->mdeath = victim->mdeath + 1;
	if (victim->position == POS_DEAD) 
	{
	    raw_kill(victim, ch);
	    return;
	}
	return;
    }

    pexit = victim->in_room->exit[door];
    if (IS_SET(pexit->exit_info, EX_CLOSED) && 
	!IS_AFFECTED(victim,AFF_PASS_DOOR ) &&
	!IS_AFFECTED(victim,AFF_ETHEREAL  ) )
    {
	if (IS_SET(pexit->exit_info, EX_LOCKED))
	    REMOVE_BIT(pexit->exit_info, EX_LOCKED);
	if (IS_SET(pexit->exit_info, EX_CLOSED))
	    REMOVE_BIT(pexit->exit_info, EX_CLOSED);
	sprintf(buf,"$n hoists $N in the air and hurls $M %s.", direction);
	act(buf,ch,NULL,victim,TO_NOTVICT);
	sprintf(buf,"You hoist $N in the air and hurl $M %s.", direction);
	act(buf,ch,NULL,victim,TO_CHAR);
	sprintf(buf,"$n hurls you %s, smashing you through the %s.", direction,pexit->keyword);
	act(buf,ch,NULL,victim,TO_VICT);
	sprintf(buf,"There is a loud crash as $n smashes through the $d.");
	act(buf,victim,NULL,pexit->keyword,TO_ROOM);

	if ( ( to_room   = pexit->to_room               ) != NULL
	&&   ( pexit_rev = to_room->exit[rev_dir] ) != NULL
	&&   pexit_rev->to_room == ch->in_room
	&&   pexit_rev->keyword != NULL )
	{
	    if (IS_SET(pexit_rev->exit_info, EX_LOCKED))
		REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	    if (IS_SET(pexit_rev->exit_info, EX_CLOSED))
		REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
	    if (door == 0) sprintf(direction,"south");
	    if (door == 1) sprintf(direction,"west");
	    if (door == 2) sprintf(direction,"north");
	    if (door == 3) sprintf(direction,"east");
	    char_from_room(victim);
	    char_to_room(victim,to_room);
	    sprintf(buf,"$n comes smashing in through the %s $d.", direction);
	    act(buf,victim,NULL,pexit->keyword,TO_ROOM);
	    dam = number_range(ch->level, (ch->level * 6));
	    victim->hit = victim->hit - dam;
	    update_pos(victim);
    	    if (IS_NPC(victim) && !IS_NPC(ch)) ch->mkill = ch->mkill + 1;
    	    if (!IS_NPC(victim) && IS_NPC(ch)) victim->mdeath = victim->mdeath + 1;
	    if (victim->position == POS_DEAD) 
	    {
		raw_kill(victim, ch);
		return;
	    }
	}
    }
    else
    {
	sprintf(buf,"$n hurls $N %s.", direction);
	act(buf,ch,NULL,victim,TO_NOTVICT);
	sprintf(buf,"You hurl $N %s.", direction);
	act(buf,ch,NULL,victim,TO_CHAR);
	sprintf(buf,"$n hurls you %s.", direction);
	act(buf,ch,NULL,victim,TO_VICT);
	if (door == 0) sprintf(direction,"south");
	if (door == 1) sprintf(direction,"west");
	if (door == 2) sprintf(direction,"north");
	if (door == 3) sprintf(direction,"east");
	char_from_room(victim);
	char_to_room(victim,to_room);
	sprintf(buf,"$n comes flying in from the %s.", direction);
	act(buf,victim,NULL,NULL,TO_ROOM);
	dam = number_range(ch->level, (ch->level * 2));
	victim->hit = victim->hit - dam;
	update_pos(victim);
    	if (IS_NPC(victim) && !IS_NPC(ch)) ch->mkill = ch->mkill + 1;
    	if (!IS_NPC(victim) && IS_NPC(ch)) victim->mdeath = victim->mdeath + 1;
	if (victim->position == POS_DEAD) 
	{
	    raw_kill(victim, ch);
	    return;
	}
    }
    return;
}



void do_disarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    long dex = number_range(1, 20);

    if ((victim = ch->fighting) == NULL)
    {
	stc("You aren't fighting anyone.\n\r", ch);
	return;
    }

    if ((obj = get_eq_char(victim, WEAR_WEAPON)) == NULL)
    {
	stc("Your opponent is not wielding a weapon.\n\r", ch);
	return;
    }

    dex += char_stat(ch, STAT_DEX);
    dex -= char_stat(victim, STAT_DEX);

    if (dex > 10)
    {
	obj_from_char(obj);
	obj_to_char(obj, victim);

	act("You disarm $N!", ch, NULL, victim, TO_CHAR);
	act("$n disarms you!", ch, NULL, victim, TO_VICT);
	act("$n disarms $N!", ch, NULL, victim, TO_NOTVICT);
    }
    else
    {
	act("You fail on your attempt to disarm $N!", ch, NULL, victim, TO_CHAR);
	act("$n fails on $s attempt to disarm you!", ch, NULL, victim, TO_VICT);
	act("$n fails on $s attempt to disarm $N!", ch, NULL, victim, TO_NOTVICT);
    }

    WAIT_STATE(ch, 15);
    return;
}


void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	stc("Slay whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
        stc("They are not here.\n\r", ch);
        return;
    }

    if (!IS_NPC(victim) && !IS_BUI(victim) && !IS_ST(ch))
    {
	stc("You may not slay mortals.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim) && victim->desc->connected != CON_PLAYING)
    {
	stc("They are busy right now.\n\r", ch);
	return;
    }

    if (!IS_NPC(victim) && victim->level > ch->level)
    {
	sprintf(buf, "%s just failed to slay %s!", ch->name, victim->name);
	do_info(ch, buf);
	victim = ch;
    }

    if (!IS_NPC(victim) && IS_FLAG(victim, FLAG_AFK))  REMOVE_BIT(ch->pcdata->flag, FLAG_AFK);

    act("You slay $M in cold blood!", ch, NULL, victim, TO_CHAR);
    act("$n slays you in cold blood!", ch, NULL, victim, TO_VICT);
    act("$n slays $N in cold blood!", ch, NULL, victim, TO_NOTVICT);
    raw_kill(victim, ch);
    return;
}

/* Had problems with people not dying when POS_DEAD...KaVir */
void killperson( CHAR_DATA *ch, CHAR_DATA *victim )
{
    stc("You have been KILLED!!\n\r", victim);
    raw_kill(victim, ch);
    return;
}

/* For decapitating players - KaVir */
void do_decapitate( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    bool dox = FALSE;

    one_argument(argument, arg);

    if (IS_NPC(ch)) return;

    if (IS_ITEMAFF(ch, ITEMA_PEACE)) do_majesty(ch, "");

    if (arg[0] == '\0' || (victim = get_char_room(ch, arg)) == NULL)
    {
	stc("Decapitate whom?\n\r", ch);
	return;
    }

    if (IS_NPC(victim) || ch == victim)
    {
	stc("You may only decapitate other players.\n\r", ch);
	return;
    }

    if (!CAN_PK(ch))
    {
	stc("You must have PK flag to decapitate.\n\r", ch);
	return;
    }
    if (!CAN_PK(victim))
    {
	stc("They are not PK, so you cannot decapitate them,\n\r",ch);
	return;
    }
    if (victim->position > POS_STUNNED || victim->hit > 0)
    {
	stc("You can only do this to mortally wounded players.\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
    {
	stc("They are safe from you.\n\r", ch);
	return;
    }

    act("You bring your weapon down upon $N's neck!", ch, NULL, victim, TO_CHAR);
    stc("Your head is sliced from your shoulders!\n\r", victim);
    act("$n swings $s weapon down towards $N's neck!", ch, NULL, victim, TO_NOTVICT);
    act("$n's head is sliced from $s shoulders!", victim, NULL, NULL, TO_ROOM);
    ch->fight_timer = 20;
    if (strlen(victim->pcdata->love) > 1 && !str_cmp(ch->name, victim->pcdata->love))
    {
	free_string(victim->pcdata->love);
	victim->pcdata->love = str_dup("");
    }
    if ((vch = victim->embrace ) != NULL)
    {
	vch->embrace = NULL;
	vch->embraced = ARE_NONE;
	victim->embrace = NULL;
	victim->embraced = ARE_NONE;
    }

    act("A misty white vapour pours from $N's corpse into your body.", ch, NULL, victim, TO_CHAR);
    act("A misty white vapour pours from $N's corpse into $n's body.", ch, NULL, victim, TO_NOTVICT);
    act("You double over in agony as raw energy pulses through your veins!", ch, NULL, NULL, TO_CHAR);
    act("$n doubles over in agony as sparks of energy crackle around $m!", ch, NULL, NULL, TO_NOTVICT);

    if (IS_WEREWOLF(victim)) if (IS_SET(victim->act, PLR_WOLFMAN)) do_unwerewolf(victim,"");
    if (IS_VAMPAFF(victim, VAM_NIGHTSIGHT)) do_nightsight(victim,"");
    if (IS_VAMPAFF(victim, VAM_FANGS)) do_fangs(victim,"");
    if (IS_VAMPAFF(victim, VAM_CLAWS)) do_claws(victim,"");
    mortal_vamp(victim);

    behead(victim, ch);
    ch->pkill++;
    victim->pdeath++;
    victim->pcdata->wolf = 0;

    if (((victim->status <= 0 && ch->status > 0) || (victim->status + 5) < ch->status) && (victim->name != ch->pcdata->decap_char))
    {
	dox = TRUE;
	sprintf(buf, "#r%s#n has been decapitated by #r%s#n for no status.", victim->name, ch->name);
	sprintf(log_buf, "%s decapitated by %s at %li for no status.", victim->name, ch->name, victim->in_room->vnum);
    }
    else
    {
	ch->status++;
	if (victim->status > 0) victim->status--;
	sprintf(buf, "#r%s#n has been decapitated by #r%s#n.", victim->name, ch->name);
	sprintf(log_buf, "%s decapitated by %s at %li.", victim->name, ch->name, victim->in_room->vnum);
    }
    if ((victim->status - ch->status) > 5)
    {
	victim->pcdata->decap_char = ch->name;
	stc("You now have an opportunity to retaliate.\n\r", victim);
    }
    if (victim->name == ch->pcdata->decap_char)
    {
	ch->pcdata->decap_char = "";
    }
    do_fatality(ch, buf);
    log_string(log_buf);
    if (dox) paradox(ch);
    return;
}

void do_destroy( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MIL];
    char logbuf[MIL];

    one_argument(argument, arg);

    if (arg[0] == '\0' || (victim = get_char_room(ch, arg)) == NULL)
    {
	stc("Destroy whom?\n\r", ch);
	return;
    }

    if (!CAN_PK(ch) || !CAN_PK(victim))
    {
	stc("Both players need a PK flag.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim)) return;

    if (victim->position > POS_STUNNED || victim->hit > 0)
    {
	stc("You may only destroy the mortally wounded.\n\r",ch);
	return;
    }

    act("You summon forth your hatred to destroy $N",  ch, NULL, victim, TO_CHAR);
    act("$n summons forth $s hatred to destroy you", ch, NULL, victim, TO_VICT);
    act("$n summons forth $s hatred to destroy $N",  ch, NULL, victim, TO_NOTVICT);

    sprintf(logbuf, "%s destroyed %s at %li", ch->name, victim->name, victim->in_room->vnum);

    log_string(logbuf);
    raw_kill(victim, ch);
    return;
}

void do_statcheck(CHAR_DATA* ch, char* argument)
{
    CHAR_DATA *victim;
    char arg[MSL], buf[MSL];
    long av, br;

    one_argument(argument, arg);

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	stc("They aren't here.\n\r", ch);
	return;
    }

    av = avoidblow(victim);
    br = breakavoid(victim);

    sprintf(buf, "Statcheck %s.\n\r", victim->name);
    stc(buf, ch);
    sprintf(buf, "Avoid: %li.  Break: %li.  Speed: %li.\n\r", av, br, get_speed(victim));
    stc(buf, ch);

    return;
}
