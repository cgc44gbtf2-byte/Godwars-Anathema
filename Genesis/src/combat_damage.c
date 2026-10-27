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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/passives.h"

void dam_message( CHAR_DATA *ch, CHAR_DATA *victim, long dam, long type, char *message );

/* Retaliation is deferred until the next input-loop boundary: legacy callers
 * may still use the attacker after hurt_person returns. Never free it there. */
struct gargoyle_reflection {
    struct gargoyle_reflection *next;
    CHAR_DATA *source, *target;
    ROOM_INDEX_DATA *room;
    unsigned long source_id, target_id;
    long amount;
};
static struct gargoyle_reflection *gargoyle_reflections;
static unsigned long gargoyle_next_reflection_id = 1;
static bool gargoyle_dispatching_reflection;

static unsigned long gargoyle_reflection_id(CHAR_DATA *ch)
{
    if (ch->gargoyle_reflection_id == 0 && gargoyle_next_reflection_id != 0) {
        ch->gargoyle_reflection_id = gargoyle_next_reflection_id;
        gargoyle_next_reflection_id = gargoyle_next_reflection_id == ULONG_MAX ?
            0 : gargoyle_next_reflection_id + 1;
    }
    return ch->gargoyle_reflection_id;
}

static CHAR_DATA *gargoyle_reflection_live(CHAR_DATA *candidate, unsigned long id)
{
    CHAR_DATA *ch;
    /* Establish membership before dereferencing a possibly recycled pointer. */
    for (ch = char_list; ch != NULL; ch = ch->next)
        if (ch == candidate)
            return id != 0 && ch->gargoyle_reflection_id == id ? ch : NULL;
    return NULL;
}

static void gargoyle_reflect_later(CHAR_DATA *source, CHAR_DATA *target, long amount)
{
    struct gargoyle_reflection *entry;
    unsigned long source_id, target_id;
    if (amount <= 0 || !combat_pair_in_room(source->in_room, source, target)) return;
    source_id = gargoyle_reflection_id(source);
    target_id = gargoyle_reflection_id(target);
    if (source_id == 0 || target_id == 0) return; /* Never wrap an identity. */
    /* Coalesce hits per live pair and pulse, rather than allocate per hit. */
    for (entry = gargoyle_reflections; entry != NULL; entry = entry->next)
        if (entry->source == source && entry->target == target &&
            entry->source_id == source_id && entry->target_id == target_id &&
            entry->room == source->in_room) {
            entry->amount = entry->amount > LONG_MAX - amount ?
                LONG_MAX : entry->amount + amount;
            return;
        }
    entry = malloc(sizeof(*entry));
    if (entry == NULL) return;
    entry->source = source;
    entry->target = target;
    entry->room = source->in_room;
    entry->source_id = source_id;
    entry->target_id = target_id;
    entry->amount = amount;
    entry->next = gargoyle_reflections;
    gargoyle_reflections = entry;
}

void gargoyle_reflection_pulse(void)
{
    struct gargoyle_reflection *entry;
    if (gargoyle_dispatching_reflection) return;
    gargoyle_dispatching_reflection = TRUE;
    while ((entry = gargoyle_reflections) != NULL) {
        CHAR_DATA *source, *target;
        ROOM_INDEX_DATA *room = entry->room;
        long amount = entry->amount;
        gargoyle_reflections = entry->next;
        source = gargoyle_reflection_live(entry->source, entry->source_id);
        target = gargoyle_reflection_live(entry->target, entry->target_id);
        free(entry);
        if (source == NULL || target == NULL ||
            !combat_pair_in_room(room, source, target) ||
            !gargoyle_statue(source) || gargoyle_visceratika(source) != 5 ||
            is_safe(source, target)) continue;
        act("Your stone body reflects damage back at $N.", source, NULL, target, TO_CHAR);
        act("$n's stone body reflects damage back at you!", source, NULL, target, TO_VICT);
        /* Direct HP path retains normal mitigation/death credit, but suppresses
         * all new reflection events throughout this dispatch (no bounce loops).
         * Either actor can be extracted here: do not touch them afterwards. */
        hurt_person(source, target, amount);
    }
    gargoyle_dispatching_reflection = FALSE;
}

void damage( CHAR_DATA *ch, CHAR_DATA *victim, long dam, long dt, char *message )
{
    ROOM_INDEX_DATA *combat_room;
    if (ch == NULL || victim == NULL || victim->position == POS_DEAD) return;
    combat_room = ch->in_room;

    if (victim != ch)
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */
	if (is_safe(ch, victim)) return;

	if (victim->position > POS_STUNNED)
	{
	    if (victim->fighting == NULL) {
                set_fighting(victim, ch);
                if (!combat_pair_in_room(combat_room, ch, victim)) return;
            }
	    victim->position = POS_FIGHTING;
	    if (ch->fighting == NULL) {
                set_fighting(ch, victim);
                if (!combat_pair_in_room(combat_room, ch, victim)) return;
            }
	}

	/*
	 * More charm stuff.
	 */
	if ( victim->master == ch )
	    stop_follower( victim );

	/*
	 * Damage modifiers.
	 */
	if ( IS_AFFECTED(ch, AFF_HIDE) )
	{
	    if (!can_see(victim,ch)) dam *= 1.5;
	    send_to_char("You leap from your concealment.\n\r",ch);
	    REMOVE_BIT( ch->affected_by, AFF_HIDE );
	    act( "$n leaps from $s concealment.", ch, NULL, NULL, TO_ROOM );
	}
        if (!IS_NPC(ch) && IS_NINJA(ch) && IS_AFFECTED(ch, AFF_KAKUSU))
        {
            if (!can_see(victim, ch)) dam *= 1.5;
	    stc("You leap from the shadows.\n\r",ch);
	    REMOVE_BIT(ch->affected_by, AFF_KAKUSU);
	    act("$n leaps from the shadows.", ch, NULL, NULL, TO_ROOM);
        }

	if (is_affected(ch, gsn_mage_invis))
	{
	    affect_strip(ch, gsn_mage_invis);
	    act("$n slowly fades into your awareness.",ch,NULL,NULL,TO_ROOM);
	}
	if (IS_ITEMAFF(ch,ITEMA_BLISSFUL))
	{
	    REMOVE_BIT(ch->itemaffect,ITEMA_BLISSFUL);
	    act("$n slowly fades into your awareness.",ch,NULL,NULL,TO_ROOM);
	}
	if (!IS_NPC(ch) && IS_VAMPAFF(ch, VAM_OBFUSCATE))
	{
	    send_to_char("You drop out of Obfuscate!\n\r",ch);
	    if (!can_see(victim,ch)) dam *= 1.5;
	    REMOVE_BIT( ch->vampaff, VAM_OBFUSCATE );
	    act( "$n fades into existance.", ch, NULL, NULL, TO_ROOM );
	}

	if (IS_WEREWOLF(victim) && get_tribe(victim, TRIBE_STARGAZERS) >= 5 && number_range(1, 8) == 1)
	{
	    CHAR_DATA *temp;
	    stc("Your attack is redirected back at you!\n\r", ch);
	    act("You redirect $n's attack back at $m!", ch, NULL, victim, TO_VICT);
	    temp = ch;
	    ch = victim;
	    victim = temp;
	}
	if (IS_XTRA(ch, XTRA_MUTATE_EPHEMERA) && number_range(1, 6) == 1)
        {
            CHAR_DATA *temp;
            stc("Fate twists your attack back against you!\n\r", ch);
            act("Fate twists $ns attack back at $m!", ch, NULL, victim, TO_VICT);
            temp = ch;
            ch = victim;
            victim = temp;
        }

	dam_message(ch, victim, gargoyle_damage(victim, dam), dt, message);
	if (victim == NULL || victim->position == POS_DEAD) return;
    }
#ifdef COMBAT_CALLBACK_TESTING
    /* The dedicated regression build can free an NPC at this callback.
     * Production and the other full-engine suites call hurt_person directly. */
    {
        extern void combat_test_hurt_person(CHAR_DATA *, CHAR_DATA *, long);
        combat_test_hurt_person(ch, victim, dam);
    }
#else
    hurt_person(ch, victim, dam);
#endif
    if (!combat_pair_in_room(combat_room, ch, victim)) return;
    if (IS_ELEM(victim) && IS_POLYAFF(victim, POLY_PHOENIXFORM) && victim != ch)
    {
	act("Your burning form singes $n!", ch, NULL, victim, TO_VICT);
	act("$N's burning form singes you!", ch, NULL, victim, TO_CHAR);
	act("$N's burning form singes $n!", ch, NULL, victim, TO_NOTVICT);
	hurt_person(victim, ch, number_range(200, 250));
        if (!combat_pair_in_room(combat_room, ch, victim)) return;
    }
    if (IS_VAMPIRE(victim) && IS_CRAFT(victim, CRAFT_BONEQUILLS) && victim != ch && number_range(1, 6) == 1)
    {
	act("Your bone quills slice into $n!", ch, NULL, victim, TO_VICT);
	act("$N's bone quills slice into you!", ch, NULL, victim, TO_CHAR);
	one_hit(victim, ch, TYPE_PIERCE, "bone quills");
    }
    return;
}

bool hurt_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if (ch == NULL || victim == NULL || victim->position == POS_DEAD) return FALSE;
    ROOM_INDEX_DATA *combat_room = ch->in_room;

    if (victim != ch)
    {
	if (auto_safe(ch, victim)) return FALSE;

	if (victim->position > POS_STUNNED)
	{
	    if (victim->fighting == NULL) {
                set_fighting(victim, ch);
                if (!combat_pair_in_room(combat_room, ch, victim)) return FALSE;
            }
	    victim->position = POS_FIGHTING;
	    if (ch->fighting == NULL) {
                set_fighting(ch, victim);
                if (!combat_pair_in_room(combat_room, ch, victim)) return FALSE;
            }

	    /*
	     * If victim is charmed, ch might attack victim's master.
	     */
	    if (IS_NPC(ch) && IS_NPC(victim) && IS_AFFECTED(victim, AFF_CHARM)
	    && victim->master != NULL && victim->master->in_room == ch->in_room && number_bits(3) == 0)
	    {
		stop_fighting(ch, FALSE);
		multi_hit(ch, victim->master);
		return TRUE;
	    }
	}
	if (victim->master == ch) stop_follower(victim);
    }
    return TRUE;
}

void hurt_person( CHAR_DATA *ch, CHAR_DATA *victim, long dam )
{
    ROOM_INDEX_DATA *actor_room = ch != NULL ? ch->in_room : NULL;
    long hit_before;
    if (ch == NULL || victim == NULL) return;
    hit_before = victim->hit > 0 ? victim->hit : 0;
    /* Shared HP boundary covers melee and direct book/spell damage once.
     * Never discount signed healing or self-inflicted costs. */
    if (ch != victim) dam = gargoyle_damage(victim, dam);
    /* Preserve legacy signed healing without overflowing the HP subtraction. */
    if (dam > 0 && victim->hit < LONG_MIN + dam) victim->hit = LONG_MIN;
    else if (dam < 0 && victim->hit > LONG_MAX + dam) victim->hit = LONG_MAX;
    else victim->hit -= dam;

    if (IS_IMM(victim) && victim->hit < 1) victim->hit = victim->max_hit;

    update_pos(victim);

    if (!gargoyle_dispatching_reflection && ch != victim && dam > 0 &&
        gargoyle_statue(victim) && gargoyle_visceratika(victim) == 5 &&
        victim->hit > 0 && victim->hit < hit_before)
        gargoyle_reflect_later(victim, ch, (hit_before - victim->hit) / 4);

    switch(victim->position)
    {
	case POS_MORTAL:
	    if (IS_VAMPIRE(victim))
	    {
		act("$n is mortally wounded, and spraying blood everywhere.",victim, NULL, NULL, TO_ROOM);
		stc("You have been torpored.\n\r",victim);
	    }
	    else
	    {
		act("$n is mortally wounded, and spraying blood everywhere.",victim, NULL, NULL, TO_ROOM);
		stc("You have been mortally wounded.\n\r",victim);
	    }
	    break;
	case POS_INCAP:
	    act("$n is incapacitated, and bleeding badly.",victim, NULL, NULL, TO_ROOM);
	    stc("You are incapacitated, and bleeding badly.\n\r",victim);
	    break;
	case POS_STUNNED:
	    act( "$n is stunned, but will soon recover.",victim, NULL, NULL, TO_ROOM );
	    stc("You are stunned, but will soon recover.\n\r",victim);
	    break;
	case POS_DEAD:
	    act("$n is DEAD!!", victim, 0, 0, TO_ROOM);
	    stc("You have been KILLED!!\n\r",victim);
	    break;
	default:
	    if (dam > victim->max_hit * 0.25) stc("That really did HURT!\n\r", victim);
	    if (victim->hit < victim->max_hit * 0.25 && dam > 0)
	    {
		if (!IS_NPC(victim) && IS_VAMPIRE(victim) && number_percent() < victim->beast) vamp_rage(victim);
		else stc("You sure are BLEEDING!\n\r", victim);
	    }
	    break;
    }

    /*
     * Payoff for killing things.
     */
    if (victim->position == POS_DEAD)
    {
	group_gain(ch, victim);

	if (!IS_NPC(victim))
	{
	    snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "%s killed by %s at %li",victim->name,(IS_NPC(ch) ? ch->short_descr : ch->name),victim->in_room != NULL ? victim->in_room->vnum : 0);
	    log_string(log_buf);
	    if (!CAN_PK(victim))
	    {
		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "#G%s doesn't have a PK flag!#n", victim->name);
		log_string(log_buf);
	    }
	    if (!CAN_PK(ch))
	    {
		snprintf(log_buf, 2 * MAX_INPUT_LENGTH, "#G%s doesn't have a PK flag!#n", ch->name);
		log_string(log_buf);
	    }
	}
        dp_gain(ch, victim);
    	if (!IS_NPC(victim) && IS_NPC(ch)) victim->mdeath++;
#ifdef COMBAT_CALLBACK_TESTING
    {
        extern void combat_test_raw_kill(CHAR_DATA *, CHAR_DATA *);
        combat_test_raw_kill(victim, ch);
    }
#else
	raw_kill(victim, ch);
#endif

/* Death can remove the attacker too (notably NPC self-damage or linked
 * body teardown). A non-NULL pooled pointer is not a live character. */
if (combat_character_in_room(actor_room, ch) && IS_SET(ch->act, PLR_AUTOSEARCH))
    do_look(ch, "in corpse");
	return;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if (!IS_AWAKE(victim)) stop_fighting(victim, TRUE);

    if (victim == ch)
	return;

    /*
     * Take care of link dead people.
     */
    if (!IS_NPC(victim) && victim->desc == NULL)
    {
	do_recall(victim, "");
	return;
    }

    if (IS_AFFECTED(victim, AFF_FEAR)) do_flee(victim, "");

    tail_chain( );
    return;
}

long mod_damcap( CHAR_DATA *ch, CHAR_DATA *victim )
{
    long max_dam, soak;

    if (ch == NULL || victim == NULL) return 0;

    max_dam = get_damcap(ch);
    soak = get_soak(victim);

    if (IS_VAMPIRE(victim) && IS_ITEMAFF(ch, ITEMA_SUNSHIELD)) max_dam += 500;
    if (IS_VAMPIRE(victim) && get_disc(victim, DISC_FORTITUDE) > 6) soak += max_dam / 4;
    if (IS_WEREWOLF(victim) && get_auspice(victim, AUSPICE_RAGABASH) < 5)
    {
	if (IS_WEREWOLF(ch) && get_auspice(ch, AUSPICE_AHROUN) >= 3 && IS_VAMPAFF(ch, VAM_CLAWS)) max_dam += 500;
	else
	{
	    if (IS_ITEMAFF(ch, ITEMA_RIGHT_SILVER)) max_dam += 250;
	    if (IS_ITEMAFF(ch, ITEMA_LEFT_SILVER)) max_dam += 250;
	}
    }

    if (IS_VAMPIRE(ch) && IS_VAMPAFF(ch, VAM_CLAWS) && get_disc(ch, DISC_PROTEAN) >= 1) soak -= 400;

    if (IS_NPC(victim)) soak /= 2;

    max_dam -= soak;
    if (max_dam < 500) max_dam = 500;
    return max_dam;
}

long get_damcap( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    long max_dam;
    if (ch == NULL) return 0;
    max_dam = 500 + (char_stat(ch, STAT_STR) * 100);

    if (IS_ITEMAFF(ch, ITEMA_ARTIFACT)) max_dam += 100;
    if (IS_ITEMAFF(ch, ITEMA_VORPAL)) max_dam += 500;

    if ((victim = ch->fighting) == NULL || !IS_STANCE(victim, STANCE_SWALLOW))
    {
	if (IS_STANCE(ch, STANCE_BULL)) max_dam += ch->stance[STANCE_BULL];
	else if (IS_STANCE(ch, STANCE_DRAGON)) max_dam += ch->stance[STANCE_DRAGON];
	else if (IS_STANCE(ch, STANCE_TIGER)) max_dam += ch->stance[STANCE_TIGER];
    }

    if (!IS_NPC(ch))
    {
	if (IS_FLAG(ch, FLAG_PK) && ch->status > 0) max_dam += 10 * ch->status;
	if (IS_MORE(ch, MORE_FAVOUR)) max_dam += 100;

	switch (ch->class)
	{
	    default: break;
	    case CLASS_VAMPIRE:
		max_dam += 200;
		if (ch->pcdata->wolf > 0) max_dam += ch->beast;
		if (get_disc(ch, DISC_POTENCE) > 0) max_dam += get_disc(ch,DISC_POTENCE) * 145;
		if (get_disc(ch, DISC_POTENCE) >= 7) max_dam += 50;
		break;
	    case CLASS_WEREWOLF:
		if (get_tribe(ch, TRIBE_SHADOW_LORDS) >= 1) max_dam += 100;
		if (get_tribe(ch, TRIBE_GET_OF_FENRIS) >= 1) max_dam += 100;
		if (get_auspice(ch, AUSPICE_PHILODOX) >= 3) max_dam += 100;
		if (IS_SET(ch->act, PLR_WOLFMAN)) max_dam += ch->pcdata->wolf * 1.5;
		if (ch->pcdata->rank >= 5) max_dam += 500;
		else max_dam += (ch->pcdata->rank + 1) * 100;
		break;
	    case CLASS_MAGE:
		if (ch->spheres[MENT] >= 1) max_dam += 400;
		break;
	    case CLASS_HIGHLANDER:
		max_dam += get_highland(ch, HIGH_ATTACK) * 300;
		break;
	    case CLASS_MONK:
		if (get_monk(ch, MONK_ART) >= 1) max_dam += 100;
		if (get_monk(ch, MONK_ART) >= 7) max_dam += 100;
		if (IS_POLYAFF(ch, POLY_CHIMERA)) max_dam += 200;
		break;
	    case CLASS_DROW:
		if (get_drow(ch, DROW_MIGHT)) max_dam += 600;
		if (get_drow(ch, DROW_BITE)) max_dam += 200;
		max_dam += get_drow_speed(ch) * 100;
		break;
	    case CLASS_NINJA:
		max_dam += ch->pcdata->wolf * 4;
		max_dam += get_ninja(ch, NINJA_CHIKYU) * 50;
		break;
	    case CLASS_DRAGON:
		if (IS_POLYAFF(ch, POLY_DRAGONFORM)) max_dam += (13 - ch->vampgen) * 100;
		break;
	    case CLASS_ELEM:
		if (ch->pcdata->powers[ELEM_FIRE] >= 5) max_dam += ch->pcdata->powers[ELEM_FIRE] * 60;
		if (ch->pcdata->powers[ELEM_INFIN] >= 8) max_dam += ch->pcdata->powers[ELEM_INFIN] * 30;
		break;
	}
        max_dam = hybrid_secondary_passive(ch, HYBRID_PASSIVE_DAMCAP, max_dam);
    }

    return max_dam;
}
