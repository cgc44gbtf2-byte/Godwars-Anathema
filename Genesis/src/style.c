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

/* Gear-driven combat styles, grip and weapon proficiency.
 * Stance commands and stance effects live in stance.c. */
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

long get_style( CHAR_DATA *ch, long style )
{
    if (style < STYLE_UNARMED || style > STYLE_TWOWEAPON)
    {
bug("Get_style: out of style range", 0);
return 0;
    }

    if (ch->style[style] < 0) ch->style[style] = 0;
    if (ch->style[style] > 200) ch->style[style] = 200;

    return ch->style[style];
}

void update_style( CHAR_DATA *ch )
{
    OBJ_DATA *weap;
    OBJ_DATA *hold;
    long style;

    weap = get_eq_char(ch, WEAR_WEAPON);
    hold = get_eq_char(ch, WEAR_HOLD);

    if (weap == NULL)
    {
	if (hold == NULL) style = STYLE_UNARMED;
	else if (hold->item_type == ITEM_ARMOR) style = STYLE_SHIELD;
	else if (hold->item_type == ITEM_WEAPON)
	{
	    hold->wear_loc = WEAR_WEAPON;
	    /* The old left/right silver flags were calculated for the old slot. */
	    refresh_equipment_state(ch);
	    style = STYLE_ONEHAND;
	    act("$n shifts $p into $s main hand.", ch, hold, NULL, TO_ROOM);
	    act("You shift $p into your main hand.", ch, hold, NULL, TO_CHAR);
	}
	else style = STYLE_UNARMED;
    }
    else if (hold == NULL) style = STYLE_ONEHAND;
    else
    {
	if (hold->item_type == ITEM_ARMOR) style = STYLE_SHIELD;
	else if (hold->item_type == ITEM_WEAPON) style = STYLE_TWOWEAPON;
	else style = STYLE_ONEHAND;
    }

    if (style == STYLE_UNARMED && (IS_DRAGON(ch) || IS_VAMPIRE(ch) || IS_WEREWOLF(ch)) && IS_VAMPAFF(ch, VAM_CLAWS))
	style = STYLE_NATURAL;

    if (style == STYLE_ONEHAND && ch->style[0] == STYLE_TWOHAND) return;
    if (style == ch->style[0]) return;

    ch->style[0] = style;
    stc("You change weapon styles.\n\r", ch);
    return;
}

void do_grip( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;

    if ((obj = get_eq_char(ch, WEAR_WEAPON)) == NULL)
    {
	stc("You aren't wearing a weapon to grip in both hands.\n\r", ch);
	return;
    }

    if (ch->style[0] == STYLE_TWOHAND)
    {
	ch->style[0] = STYLE_ONEHAND;
	act("You shift $p into one hand.", ch, obj, NULL, TO_CHAR);
	act("$n shifts $p into one hand.", ch, obj, NULL, TO_ROOM);
	return;
    }

    if (get_eq_char(ch, WEAR_HOLD) != NULL)
    {
	stc("You've already got something in your off hand.\n\r", ch);
	return;
    }

    ch->style[0] = STYLE_TWOHAND;
    act("You grip $p in both hands.", ch, obj, NULL, TO_CHAR);
    act("$n grips $p in both hands.", ch, obj, NULL, TO_ROOM);
    return;
}

void improve_style( CHAR_DATA *ch )
{
    char buf[MIL];
    char bufskill[50];
    char skillname[50];
    long max = 100, skill = ch->style[0];

    if (IS_NPC(ch)) return;

    if (skill == STYLE_UNARMED)
    {
	if (IS_MONK(ch)) max = 200;
	else if (IS_NINJA(ch)) max = 150;
    }
    else if (skill == STYLE_NATURAL)
    {
	if (IS_DRAGON(ch)) max = 150;
    }
    else if (skill == STYLE_ONEHAND)
    {
	if (IS_HIGHLANDER(ch)) max = 200;
	else if (IS_SORCERER(ch)) max = 150;
    }
    else if (skill == STYLE_SHIELD)
    {
	if (IS_HIGHLANDER(ch)) max = 200;
	if (IS_ELEM(ch)) max = 150;
    }
    else if (skill == STYLE_TWOHAND)
    {
	if (IS_HIGHLANDER(ch)) max = 200;
    }
    else if (skill == STYLE_TWOWEAPON)
    {
	if (IS_DROW(ch)) max = 150;
    }
    else
    {
	stc("Bad style.  Inform Akuma.\n\r", ch);
	return;
    }

    if (ch->style[skill] >= max)
    {
	ch->style[skill] = max;
	return;
    }

    if ((ch->style[skill] <= max * .25 && number_range(0, 1) == 0)
     || (ch->style[skill] <= max * .75 && number_range(0, 3) == 0)
     || number_range(0, 7) == 0)
	ch->style[skill]++;
    else return;

    if      (ch->style[skill] == 1)	sprintf(bufskill, "slightly skilled");
    else if (ch->style[skill] == 50)	sprintf(bufskill, "fairly competent");
    else if (ch->style[skill] == 100)	sprintf(bufskill, "an expert");
    else if (ch->style[skill] == 150)	sprintf(bufskill, "a master");
    else if (ch->style[skill] == 200)	sprintf(bufskill, "a grand master");
    else return;
    if      (skill == STYLE_UNARMED)	sprintf(skillname, "unarmed combat");
    else if (skill == STYLE_NATURAL)	sprintf(skillname, "natural weapons");
    else if (skill == STYLE_ONEHAND)	sprintf(skillname, "one-handed combat");
    else if (skill == STYLE_SHIELD)	sprintf(skillname, "shielded fighting");
    else if (skill == STYLE_TWOHAND)	sprintf(skillname, "two-handed combat");
    else if (skill == STYLE_TWOWEAPON)	sprintf(skillname, "two weapon fighting");
    else return;

    sprintf(buf, "#7You are now %s in %s.#n\n\r", bufskill, skillname);
    stc(buf, ch);
    return;
}

void improve_weapon( CHAR_DATA *ch, long skill )
{
    char buf[MIL];
    char bufskill[50];
    char skillname[50];
    long max = 100;

    if (IS_NPC(ch) || skill < 1 || skill > 4) return;

    if (skill == TYPE_UNARMED)
    {
	if (IS_MONK(ch)) max = 200;
    }
    else if (skill == TYPE_SLASH)
    {
	if (IS_DROW(ch)) max = 200;
	else if (IS_HIGHLANDER(ch) || IS_NINJA(ch) || IS_WEREWOLF(ch)) max = 150;
    }
    else if (skill == TYPE_PIERCE)
    {
	if (IS_NINJA(ch) || IS_VAMPIRE(ch)) max = 150;
    }

    if (ch->weapon[skill] >= max)
    {
	ch->weapon[skill] = max;
	return;
    }

    if ((ch->weapon[skill] <= max * .25 && number_range(0, 1) == 0)
     || (ch->weapon[skill] <= max * .75 && number_range(0, 3) == 0)
     || number_range(0, 7) == 0)
	ch->weapon[skill]++;
    else return;

    if      (ch->weapon[skill] == 1)	sprintf(bufskill, "slightly skilled");
    else if (ch->weapon[skill] == 50)	sprintf(bufskill, "fairly competent");
    else if (ch->weapon[skill] == 100)	sprintf(bufskill, "an expert");
    else if (ch->weapon[skill] == 150)	sprintf(bufskill, "a master");
    else if (ch->weapon[skill] == 200)	sprintf(bufskill, "a grand master");
    else return;
    if      (skill == TYPE_UNARMED)	sprintf(skillname, "no");
    else if (skill == TYPE_SLASH)	sprintf(skillname, "slashing");
    else if (skill == TYPE_PIERCE)	sprintf(skillname, "piercing");
    else if (skill == TYPE_BLUNT)	sprintf(skillname, "bludgeoning");
    else return;

    sprintf(buf, "#7You are now %s of using %s weapons.#n\n\r", bufskill, skillname);
    stc(buf, ch);
    return;
}
