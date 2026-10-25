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

/* Shared soak and avoidance. Keep RNG order and compound truncation intact. */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "classes/hybrid/passives.h"
#include "classes/hybrid/access.h"

long get_soak( CHAR_DATA *victim )
{
    CHAR_DATA *ch;
    long soak = char_stat(victim, STAT_CON) * 50;

    if (IS_ITEMAFF(victim, ITEMA_RESISTANCE)) soak += 500;

    if (!IS_NPC(victim))
    {
	switch (victim->class)
	{
	    default: break;
	    case CLASS_VAMPIRE:
		if (get_disc(victim, DISC_FORTITUDE) > 0) soak += get_disc(victim, DISC_FORTITUDE) * 75;
		if (IS_POLYAFF(victim, POLY_CHIRO)) soak += 150;
		if (IS_POLYAFF(victim, POLY_SERPENT)) soak += 500;
		if (IS_CRAFT(victim, CRAFT_FLESHSOAK)) soak += 100;
		break;
	    case CLASS_WEREWOLF:
		if (get_tribe(victim, TRIBE_BONE_GNAWERS) >= 2) soak += 30 * get_tribe(victim, TRIBE_BONE_GNAWERS);
		break;
	    case CLASS_HIGHLANDER:
		soak += get_highland(victim, HIGH_DEFENSE) * 50;
		break;
	    case CLASS_DROW:
		if (get_drow(victim, DROW_TOUGH)) soak += 300;
		break;
	    case CLASS_DRAGON:
		if (IS_POLYAFF(victim, POLY_DRAGONFORM)) soak += (13 - victim->vampgen) * 25;
		break;
	    case CLASS_ELEM:
		if (victim->pcdata->powers[ELEM_EARTH] >= 5) soak += victim->pcdata->powers[ELEM_EARTH] * 35;
		if (IS_ITEMAFF(victim, ITEMA_SUNSHIELD)) soak += 100;
		break;
	    case CLASS_MONK:
		if (get_monk(victim, MONK_ART) >= 5) soak += 200;
		break;
	}
	soak = hybrid_secondary_passive(victim, HYBRID_PASSIVE_SOAK, soak);
	if (IS_MORE(victim, MORE_FAVOUR)) soak += 100;
	if ((ch = victim->fighting) == NULL || !IS_STANCE(ch, STANCE_MANTIS))
	{
	    if (IS_STANCE(victim, STANCE_CRAB)) soak += victim->stance[STANCE_CRAB];
	    else if (IS_STANCE(victim, STANCE_DRAGON)) soak += victim->stance[STANCE_DRAGON];
	    else if (IS_STANCE(victim, STANCE_SWALLOW)) soak += victim->stance[STANCE_SWALLOW];
	}
    }
    if (is_affected(victim, gsn_brittle_bones)) soak = soak / 2;

    if (victim->affected != NULL)
    {
	AFFECT_DATA *paf;
	for (paf = victim->affected; paf != NULL; paf = paf->next)
	{
	    if (paf->type == gsn_brittle_bones)
	    {
		paf->modifier--;
		if (paf->modifier == 0)
		{
		    /* affect_strip recycles this node and replaces its next link. */
		    affect_strip(victim, gsn_brittle_bones);
		    break;
		}
	    }
	}
    }

    return soak;
}


bool check_avoid( CHAR_DATA *ch, CHAR_DATA *victim, bool weap )
{
    long av, br;

    if (victim->position != POS_FIGHTING) return FALSE;

    if (ch->beginners_luck > 0)
    {
	ch->beginners_luck--;
	return FALSE;
    }

    av = number_range(1, avoidblow(victim));
    br = number_range(1, breakavoid(ch));

    if (!weap) av *= .5;
    if (!hybrid_power_drow(victim, DROW_BFIGHT) && !can_see(victim, ch)) av *= .75;
    if (!hybrid_power_drow(ch, DROW_BFIGHT) && !can_see(ch, victim)) br *= .75;

    if (av > br) return TRUE;
    else return FALSE;
}

long avoidblow( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    long chance = 0, style = ch->style[0];

    if (IS_NPC(ch))
    {
	if (ch->level < 500) return ch->level/10;
	else return 50 + (ch->level - 750)/100;
    }

    chance += char_ac(ch)/250;
    chance += char_stat(ch, STAT_DEX);/*10*/

    chance += ch->style[STYLE_UNARMED] * .05;

    if (style == STYLE_UNARMED) chance += ch->style[style] * .2;
    else if (style == STYLE_ONEHAND || style == STYLE_SHIELD) chance += ch->style[style] * .1;
    else if (style == STYLE_TWOHAND || style == STYLE_NATURAL) chance += ch->style[style] * .05;

    if ((victim = ch->fighting) == NULL || !IS_STANCE(victim, STANCE_MONKEY))
    {
	if (IS_STANCE(ch, STANCE_MONGOOSE)) chance += ch->stance[STANCE_MONGOOSE] * 0.05;
	else if (IS_STANCE(ch, STANCE_SWALLOW)) chance += ch->stance[STANCE_SWALLOW] * 0.05;
	else if (IS_STANCE(ch, STANCE_MANTIS)) chance += ch->stance[STANCE_MANTIS] * 0.05;
    }

    switch(ch->class)
    {
	case CLASS_VAMPIRE:/*55/60*/
	    chance += get_celerity(ch) * 5;
	    if(IS_XTRA(ch, XTRA_TENEBROUS)) chance += 10;
	    break;
	case CLASS_WEREWOLF:/*50*/
	    if (get_breed(ch,BREED_LUPUS) >= 3) chance += 3;
	    if (get_tribe(ch,TRIBE_CHILDREN_OF_GAIA) >= 2) chance += 3;
	    if (get_tribe(ch,TRIBE_STARGAZERS) >= 4) chance += 3;
	    chance += get_celerity(ch) * 3;
	    break;
	case CLASS_MONK:/*70*/
            if (get_monk(ch, MONK_ART) >= 4) chance += 15;
            if (IS_SET(ch->more, MORE_FAVOUR)) chance += 15;
	    if (IS_POLYAFF(ch, POLY_CHIMERA)) chance += 10;
            break;
	case CLASS_MAGE:/*50*/
	    if (ch->spheres[MTIM] >= 2) chance += 15;
	    break;
	case CLASS_DROW:/*50*/
	    if (get_drow(ch, DROW_SKIP)) chance += 10;
	    chance += ch->pcdata->drow[DROW_SPEED] * 2.5;
	    break;
	case CLASS_HIGHLANDER:/*65*/
	    if (ch->pcdata->celerity > 0) chance += 5;
	    chance += get_highland(ch, HIGH_WEAPON) * 5;
	    break;
	case CLASS_NINJA:/*40*/
	    chance += get_ninja(ch, NINJA_CHIKYU);
	    break;
	case CLASS_DRAGON:/*55*/
	    if (IS_POLYAFF(ch, POLY_DRAGONFORM)) chance += (13 - ch->vampgen) * 2;
	    break;
	case CLASS_ELEM:
	    if (ch->pcdata->powers[ELEM_WIND]>4) chance += (ch->pcdata->powers[ELEM_WIND]);
	    if (ch->pcdata->powers[ELEM_TIME]>4) chance += (ch->pcdata->powers[ELEM_TIME]);
	    if (ch->pcdata->powers[ELEM_MOON]>4) chance += (ch->pcdata->powers[ELEM_MOON]); 
	    if (ch->pcdata->powers[ELEM_EARTH]>5) chance -=(ch->pcdata->powers[ELEM_EARTH]);
	    if (IS_ITEMAFF(ch, ITEMA_AQUABARRIER)) chance += (ch->pcdata->powers[ELEM_WATER]);
	    break;
    }
    return hybrid_secondary_passive(ch, HYBRID_PASSIVE_AVOID, chance);
}

long breakavoid( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    long chance = 0, style = ch->style[0];

    if (IS_NPC(ch))
    {
	if (ch->level < 750) return ch->level/10;
	else return 75 + (ch->level - 750)/100;
    }

    /* A partial or corrupt save must not become an array index. */
    if (style < STYLE_UNARMED || style > STYLE_TWOWEAPON)
	style = STYLE_UNARMED;

    chance += char_hitroll(ch)/150;
    chance += char_stat(ch, STAT_DEX);/*10*/

    if (style == STYLE_TWOHAND) chance += ch->style[style] * .2;
    else if (style != STYLE_UNARMED) chance += ch->style[style] * .1;
    else chance += ch->style[style] * .05;

    if ((victim = ch->fighting) == NULL || !IS_STANCE(victim, STANCE_DRAGON))
    {
	if (IS_STANCE(ch, STANCE_CRANE)) chance += ch->stance[STANCE_CRANE] * 0.05;
	else if (IS_STANCE(ch, STANCE_TIGER)) chance += ch->stance[STANCE_TIGER] * 0.05;
	else if (IS_STANCE(ch, STANCE_MONKEY)) chance += ch->stance[STANCE_MONKEY] * 0.05;
    }

    if (ch->pcdata->pain > 0) chance -= 10;

    switch(ch->class)
    {
	case CLASS_VAMPIRE:/*50/65*/
	    chance += get_celerity(ch) * 5;
	    if (get_disc(ch, DISC_FORTITUDE) > 5)
	    chance += get_celerity(ch) * 2;
	    break;
	case CLASS_WEREWOLF:/*60*/
	    if (get_auspice(ch,AUSPICE_PHILODOX) >= 3) chance += 15;
	    chance += get_celerity(ch) * 3;
	    break;
	case CLASS_MONK:/*45*/
            if (get_monk(ch, MONK_ART) >= 2) chance += 10;
	    if (IS_POLYAFF(ch, POLY_CHIMERA)) chance += 5;
            break;
	case CLASS_MAGE: /*30*/
	    if (ch->spheres[MENT] >= 1) chance += 8;
	    break;
	case CLASS_DROW:/*65*/
	    if (get_drow(ch, DROW_EYES)) chance += 10;
	    chance += ch->pcdata->drow[DROW_SPEED] * 5;
	    break;
	case CLASS_HIGHLANDER:/*60*/
	    chance += get_highland(ch, HIGH_WEAPON) * 5;
	    break;
	case CLASS_NINJA:/*70*/
	    chance += get_ninja(ch, NINJA_NINGENNO) * 4;
	    break;
	case CLASS_DRAGON:/*55*/
	    if (IS_POLYAFF(ch,POLY_DRAGONFORM)) chance += (13 - ch->vampgen) * 2;
	    break;
	case CLASS_ELEM:
	    if (ch->pcdata->powers[ELEM_TIME]>2) chance += (ch->pcdata->powers[ELEM_TIME]);
	    if (ch->pcdata->powers[ELEM_WIND]>4) chance += (ch->pcdata->powers[ELEM_WIND]);
	   // if (ch->pcdata->powers[ELEM_EARTH]>4) chance -= (ch->pcdata->powers[ELEM_EARTH]);
	    if (IS_ITEMAFF(ch, ITEMA_AURA)) chance += (ch->pcdata->powers[ELEM_SUN]);
	    break;
    }
    return hybrid_secondary_passive(ch, HYBRID_PASSIVE_BREAK, chance);
}
