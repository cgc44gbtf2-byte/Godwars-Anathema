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

/* Attack pulse, attack groups and targeting; public entry points stay stable. */
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

/* A cached next fighter may have been removed by the preceding attack.
 * Compare addresses in the owned live list, never read the cached node first. */
static bool active_fighter(CHAR_DATA *candidate)
{
    CHAR_DATA *fighter;
    for (fighter = first_fight; fighter != NULL; fighter = fighter->next_fight)
        if (fighter == candidate) return TRUE;
    return FALSE;
}

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
    ROOM_INDEX_DATA *combat_room;

    for (ch = first_fight; ch != NULL; ch = ch_next)
    {
        /* An interrupted traversal resumes from the live list next pulse. */
        if (!active_fighter(ch)) break;
	ch_next	= ch->next_fight;
        combat_room = ch->in_room;

        if (IS_COMBAT_PAUSED(ch)) continue;

	if (!combat_character_in_room(combat_room, victim = ch->fighting))
	{
	    stop_fighting(ch, FALSE);
	    continue;
	}

	if (IS_AWAKE(ch) && IS_AWAKE(victim) && ch->in_room == victim->in_room)
	{
	    if (--ch->init <= 0)
	    {
		multi_hit(ch, victim);
                if (!combat_character_in_room(combat_room, ch)) continue;
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
            if (!combat_pair_in_room(combat_room, ch, victim) ||
                !combat_character_in_room(combat_room, rch)) break;
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
    ROOM_INDEX_DATA *combat_room;

    if (ch == NULL || victim == NULL || IS_COMBAT_PAUSED(ch) ||
        gargoyle_statue(ch)) return;
    if (ch->position < POS_SLEEPING || is_safe(ch, victim)) return;
    combat_room = ch->in_room;

    /* Keep damage/RNG order for uninterrupted attacks. Every strike can kill,
     * extract, recall or interrupt either participant before the next read. */
#define ATTACK_ONCE(target, type, label) do { \
    one_hit(ch, (target), (type), (label)); \
    if (!combat_pair_in_room(combat_room, ch, victim) || IS_COMBAT_PAUSED(ch)) return; \
} while (0)

    if (IS_NPC(ch)) /* Temporary I swear */
    {
	if ((wield = get_eq_char(ch, WEAR_WEAPON)) && wield->item_type == ITEM_WEAPON) dt = wield->value[3];
	else dt = TYPE_UNARMED;
	ATTACK_ONCE(victim, dt, "attack");
	if (number_range(251, 500) <= ch->level) ATTACK_ONCE(victim, dt, "attack");
	if (number_range(501, 1000) <= ch->level) ATTACK_ONCE(victim, dt, "attack");
	if (number_range(1001, 2500) <= ch->level) ATTACK_ONCE(victim, dt, "attack");
	if (number_range(2501, 5000) <= ch->level) ATTACK_ONCE(victim, dt, "attack");
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

    ATTACK_ONCE(victim, dt, "1");
    switch(style)
    {
	default:
	    stc("Erm...'Oh Shit(style)' bug alert.  Inform Akuma.\n\r", ch);
	    return;
	case STYLE_UNARMED:
	    if (number_range(101, 200) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "1");
	    break;
	case STYLE_NATURAL:
	    break;
	case STYLE_ONEHAND:
	    if (number_range(1, 100) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "1");
	    if (number_range(101, 200) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "1");
	    break;
	case STYLE_SHIELD:
	    break;
	case STYLE_TWOHAND:
	    break;
	case STYLE_TWOWEAPON:
	    if (number_range(51, 150) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "1");
	    if (number_range(151, 250) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "1");

	    if ((wield = get_eq_char(ch, WEAR_HOLD)) == NULL || wield->item_type != ITEM_WEAPON)
	    {
		stc("Erm...'Oh Shit(two weapon)' bug alert.  Inform Akuma.\n\r", ch);
		return;
	    }
	    dt = wield->value[3];
	    improve_weapon(ch, dt);

	    if (number_range(1, 100) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "2");
	    if (number_range(101, 200) <= get_style(ch, style)) ATTACK_ONCE(victim, dt, "2");
	    break;
    }

    if (victim == NULL || victim->position != POS_FIGHTING) return;
    if (wield != NULL &&
        get_eq_char(ch, style == STYLE_TWOWEAPON ? WEAR_HOLD : WEAR_WEAPON) != wield)
        return;
    if (wield != NULL && wield->value[0] >= 1)
    {
	sn = wield->value[0] % 1000;
	/* Saved/OLC weapon values are not trusted skill-table indexes.
	 * Item-only callbacks cannot consume a character target. TAR_DEATH
	 * remains valid here: those legacy damage spells take CHAR_DATA. */
	if (sn > 0 && sn < MAX_SKILL && skill_table[sn].spell_fun != NULL
	    && skill_table[sn].target != TAR_OBJ_INV && victim->position == POS_FIGHTING)
	    (*skill_table[sn].spell_fun) (sn, wield->level, ch, victim);
    }
    else if (style == STYLE_NATURAL && IS_WEREWOLF(ch) && get_tribe(ch, TRIBE_BLACK_FURIES) >= 3)
	spell_poison(gsn_poison, 25, ch, victim);
    if (!combat_pair_in_room(combat_room, ch, victim) || IS_COMBAT_PAUSED(ch)) return;

    if (!IS_NPC(ch) && IS_VAMPAFF(ch, VAM_FANGS) && number_range(1, 3) <= 2) ATTACK_ONCE(victim, TYPE_PIERCE, "fangs");
    if (IS_DRAGON(ch))
    {
	if (IS_DRAGAFF(ch, DRAG_HORNSOUT) && number_range(1, 3) == 1) ATTACK_ONCE(victim, TYPE_PIERCE, "horns");
	if (IS_DRAGAFF(ch, DRAG_WINGSOUT) && number_range(1, 3) == 1) ATTACK_ONCE(victim, TYPE_BLUNT, "wing buffet");
    }
    if (IS_MONK(ch) && get_monk(ch, MONK_ART) >= 8 && number_range(1, 3) == 1) ATTACK_ONCE(victim, TYPE_HIT, "wave fist");
    if (IS_MAGE(ch) && IS_XTRA(ch, XTRA_HOLY_SMOTE) && number_range(1, 2) == 1) ATTACK_ONCE(victim, TYPE_HIT, "holy sword");
    if (IS_WEREWOLF(ch) && get_tribe(ch, TRIBE_WENDIGOS) >= 5 && IS_SET(world_affects, WORLD_RAIN) && IS_OUTSIDE(ch))
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;
	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
            if (!combat_character_in_room(combat_room, vch)) break;
	    vch_next = vch->next_in_room;
	    if (is_safe(ch, vch)) continue;
	    if (vch->fighting != ch) continue;
	    if (number_range(1, 2) == 1) continue;
	    ATTACK_ONCE(vch, TYPE_HIT, "lightning blast");
	}
    }

    if (IS_NINJA(ch))
    {
	if (IS_VAMPAFF(ch, VAM_CLAWS) && number_range(1, 3) == 1) ATTACK_ONCE(victim, TYPE_SLASH, "tsume");
	if (get_ninja(ch, NINJA_NINGENNO) >= 6 && number_range(1, 3) == 1)
	{
	    ATTACK_ONCE(victim, TYPE_PIERCE, "shuriken");
	    if (victim == NULL || victim->position != POS_FIGHTING) return;
	    spell_poison(gsn_poison, 25, ch, victim);
            if (!combat_pair_in_room(combat_room, ch, victim) || IS_COMBAT_PAUSED(ch)) return;
	}
    }
    
    if (IS_POLYAFF(ch, POLY_SERPENT))
    {
	ATTACK_ONCE(victim, TYPE_SLASH, "forked tongue");
	ATTACK_ONCE(victim, TYPE_SLASH, "forked tongue");
    }

    if (IS_VAMPIRE(ch) && IS_XTRA(ch, XTRA_TENEBROUS))
    {
	ATTACK_ONCE(victim, TYPE_SLASH, "tendril of shadow");
	ATTACK_ONCE(victim, TYPE_BLUNT, "shadow arm");
    }

    if (IS_VAMPIRE(ch) && get_disc(ch, DISC_CHIMERSTRY) >= 2)
	ATTACK_ONCE(victim, TYPE_SLASH, "chimerstral dagger");

    if (IS_DROW(ch) && get_drow(ch, DROW_BITE) && number_range(1, 4) == 1)
	ATTACK_ONCE(victim, TYPE_PIERCE, "spider fangs");

#undef ATTACK_ONCE
    return;
}
