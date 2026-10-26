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
#include <string.h>
#include "merc.h"
#include "classes/hybrid/access.h"

/* Death templates accept names (%s, at most two) and literal percent (%%).
 * Never give a persisted format string to printf as executable formatting. */
static bool format_death_names(char *out, size_t size, const char *pattern,
                               const char *first, const char *second)
{
    size_t used = 0;
    int names = 0;
    if (out == NULL || size == 0 || pattern == NULL) return FALSE;
    out[0] = '\0';
    for (; *pattern != '\0'; ++pattern) {
        char literal[2] = {*pattern, '\0'};
        const char *text = literal;
        if (*pattern == '%') {
            ++pattern;
            if (*pattern == '%') text = "%";
            else if (*pattern == 's' && names < 2) {
                text = names++ == 0 ? first : second;
                if (text == NULL) { out[0] = '\0'; return FALSE; }
            } else { out[0] = '\0'; return FALSE; }
        }
        while (*text != '\0' && used < size - 1) out[used++] = *text++;
        out[used] = '\0';
    }
    return TRUE;
}

/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
    if (victim == NULL) return;
    if ( victim->hit > 0 )
    {
    	if ( victim->position <= POS_STUNNED )
	{
	    victim->position = POS_STANDING;
	    act( "$n flips back up to $s feet.", victim, NULL, NULL, TO_ROOM);
	    act( "You flip back up to your feet.", victim, NULL, NULL, TO_CHAR);
	}
	return;
    }

    if (victim->hit <= 0 && monk_cloak_can_save(victim))
    {
	stc("Your Cloak of Life saves you from death!\n\r", victim);
	act("A flash of light bursts outward from $n, saving him from certain death!", victim, NULL, NULL, TO_ROOM);
	REMOVE_BIT(victim->more, MORE_CLOAK);
victim->hit = UMAX(1, victim->max_hit / 5);
    }

    if (victim->hit <= 0 && hybrid_drow_globe_active(victim))
    {
	act("Darkness fades from the room.", victim, NULL, NULL, TO_ROOM);
hybrid_drow_globe_set(victim,0);
    }

    if (!IS_NPC(victim) && victim->hit <= -10)
    {
	victim->hit = -10;
	if (victim->position == POS_FIGHTING) stop_fighting(victim,TRUE);
    }
    else if (IS_NPC(victim) && victim->hit <= 0)
    {
	victim->position = POS_DEAD;
	return;
    }

    if (victim->hit <= -6) victim->position = POS_MORTAL;
    else if (victim->hit <= -3) victim->position = POS_INCAP;
    else victim->position = POS_STUNNED;

    return;
}

/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch, CHAR_DATA *maker )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;

    if ( IS_NPC(ch) )
    {
	name		= ch->short_descr;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
	corpse->timer	= number_range( 60, 120 );
	corpse->value[2]= ch->pIndexData->vnum;
	if ( ch->gold > 0 )
	{
	    obj_to_obj( create_money( ch->gold ), corpse );
	    ch->gold = 0;
	}
    }
    else
    {
	name		= ch->name;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer	= number_range( 600, 1200 );
	/* Why should players keep their gold? */
	if ( ch->gold > 0 )
	{
	    obj = create_money( ch->gold );
	    if (IS_AFFECTED(ch,AFF_SHADOWPLANE) &&
	        (!IS_SET(obj->extra_flags, ITEM_SHADOWPLANE) ) )
	        SET_BIT(obj->extra_flags, ITEM_SHADOWPLANE);
	    obj_to_obj( obj, corpse );
	    ch->gold = 0;
	}
    }

    free_string(corpse->questmaker);
    if (IS_NPC(maker)) corpse->questmaker = str_dup(maker->short_descr);
    else corpse->questmaker = str_dup(maker->name);

    free_string(corpse->questowner);
    if (IS_NPC(ch)) corpse->questowner = str_dup(ch->short_descr);
    else corpse->questowner = str_dup(ch->name);

    if (ch->in_room != NULL) corpse->specpower = ch->in_room->vnum;

    if (!format_death_names(buf, sizeof(buf), corpse->short_descr, name, NULL))
	snprintf(buf, sizeof(buf), "the corpse of %s", name);
    free_string( corpse->short_descr );
    corpse->short_descr = str_dup( buf );

    if (!format_death_names(buf, sizeof(buf), corpse->description, name, NULL))
	snprintf(buf, sizeof(buf), "The corpse of %s is lying here.", name);
    free_string( corpse->description );
    corpse->description = str_dup( buf );

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;

	obj_from_char( obj );
	if ( IS_SET( obj->extra_flags, ITEM_VANISH ) )
	    extract_obj( obj );
	else
	{
	    if (IS_AFFECTED(ch,AFF_SHADOWPLANE) &&
	        (!IS_SET(obj->extra_flags, ITEM_SHADOWPLANE) ) )
	        SET_BIT(obj->extra_flags, ITEM_SHADOWPLANE);
	    obj_to_obj( obj, corpse );
	}
    }

    /* If you die in the shadowplane, your corpse should stay there */
    if (IS_AFFECTED(ch, AFF_SHADOWPLANE))
	SET_BIT(corpse->extra_flags, ITEM_SHADOWPLANE);

    obj_to_room( corpse, ch->in_room );
    return;
}

void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    long door;

    if ( IS_NPC(ch) )
	msg = "You hear something's death cry.";
    else
	msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( door = 0; door <= 5; door++ )
    {
	EXIT_DATA *pexit;

	if ( ( pexit = was_in_room->exit[door] ) != NULL
	&&   pexit->to_room != NULL
	&&   pexit->to_room != was_in_room )
	{
	    ch->in_room = pexit->to_room;
	    act( msg, ch, NULL, NULL, TO_ROOM );
	}
    }
    ch->in_room = was_in_room;
    return;
}

void make_head( CHAR_DATA *ch )
{
    OBJ_DATA *obj;

    if (IS_NPC(ch)) return;

    obj = create_object(get_obj_index(OBJ_VNUM_SEVERED_HEAD), 0);
    ch->pcdata->chobj = obj;
    obj->chobj = ch;
    obj->timer = 5;

    return;
}

void raw_kill( CHAR_DATA *victim, CHAR_DATA *ch )
{
    if (!IS_NPC(victim)) death_message(ch, victim);
    stop_fighting(victim, TRUE);

    if (ch == victim) ch->mdeath++;
    else if (IS_NPC(victim) && !IS_NPC(ch))
    {
	ch->mkill++;
	if (ch->mkill >= 5 && ch->level <= LEV_SLE)
	{
	    char buf[MSL];
	    sprintf(buf,"%s has awakened!",ch->name);
	    do_info(ch, buf);
	    ch->level = LEV_AWA;
	    do_help(ch, "newbie");
	}
    }
    else if (!IS_NPC(victim) && IS_NPC(ch)) victim->mdeath++;
    else if (!IS_NPC(victim) && !IS_NPC(ch) && ch != victim)
    {
	ch->pkill++;
	victim->pdeath++;
    }

    if (IS_NPC(victim) && (IS_EXTRA(victim, EXTRA_ILLUSION) || IS_SPIRIT(victim)))
	act("$n slowly fades away to nothing.", victim, NULL, NULL, TO_ROOM);
    else if (!IS_NPC(victim) || !IS_EXTRA(victim, EXTRA_ILLUSION))
    {
	death_cry(victim);
	make_corpse(victim, ch);
    }

    victim->position = POS_DEAD;

    if (IS_NPC(victim))
    {
	char_from_room(victim);
	char_to_room(victim, get_room_index(ROOM_VNUM_DISCONNECTION));
	extract_char(victim, TRUE);
	return;
    }
     	   
    victim->hit = 1;
    extract_char(victim, FALSE);
    while (victim->affected)
	affect_remove(victim, victim->affected);

    REMOVE_BIT(victim->extra, TIED_UP);
    REMOVE_BIT(victim->extra, GAGGED);
    REMOVE_BIT(victim->extra, BLINDFOLDED);
    REMOVE_BIT(victim->more, MORE_THINBLOOD);

    victim->itemaffect	 = 0;
    victim->blood[0]	 = victim->blood[1];
    victim->affected_by	 = 0;
    strip_shadowsight(victim);
    victim->armor	 = 0;
    victim->position	 = POS_RESTING;
    victim->hit		 = UMAX(1, victim->hit);
    victim->mana	 = UMAX(1, victim->mana);
    victim->hitroll	 = 0;
    victim->damroll	 = 0;
    victim->saving_throw = 0;
    victim->carry_weight = 0;
    victim->carry_number = 0;
    victim->pcdata->atm  = 0;
    do_call(victim, "all");
    do_autosave(victim, "");
    return;
}

void behead( CHAR_DATA *victim, CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *location;
    char buf[MSL];

    if (IS_NPC(victim)) return;

    location = victim->in_room;

    stop_fighting(victim, TRUE);
    make_corpse(victim, ch);
    extract_char(victim, FALSE);

    char_from_room(victim);
    char_to_room(victim, location);

    if (victim == NULL)
    {
	bug("Behead: Victim no longer exists.", 0);
	return;
    }

    make_head(victim);

    while (victim->affected)
	affect_remove(victim, victim->affected);

    REMOVE_BIT(victim->extra, TIED_UP);
    REMOVE_BIT(victim->extra, GAGGED);
    REMOVE_BIT(victim->extra, BLINDFOLDED);
    victim->itemaffect	 = 0;
    victim->blood[0]     = victim->blood[1];
    victim->affected_by	 = 0;
    strip_shadowsight(victim);
    victim->armor	 = 0;
    victim->position	 = POS_STANDING;
    victim->hit		 = 1;
    victim->mana	 = 1;
    victim->hitroll	 = 0;
    victim->damroll	 = 0;
    victim->saving_throw = 0;
    victim->carry_weight = 0;
    victim->carry_number = 0;

    SET_BIT(victim->more, MORE_HEAD);
    SET_BIT(victim->affected_by, AFF_POLYMORPH);
    snprintf(buf, sizeof(buf), "the severed head of %s", victim->name);
    free_string(victim->morph);
    victim->morph = str_dup(buf);
    do_call(victim, "all");
    do_autosave(victim, "");
    return;
}

void death_message( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MSL];
    char *chsex;
    const char *killer, *target;
    if (ch == NULL || victim == NULL) return;
    chsex = ch->sex == SEX_FEMALE ? "her" : "his";
    killer = IS_NPC(ch) ? ch->short_descr : ch->name;
    if (killer == NULL) killer = "someone";
    target = victim->name != NULL ? victim->name : "someone";

    if (!IS_BUI(ch) && ch == victim) snprintf(buf, sizeof(buf), "#r%s#n has just died!", killer);
    else if (!IS_NPC(ch) && ch->pcdata != NULL && ch->pcdata->cdest != NULL &&
             strlen(ch->pcdata->cdest) >= 2 &&
             format_death_names(buf, sizeof(buf), ch->pcdata->cdest, killer, target)) {
        /* Safely rendered custom text is already in buf. */
    }
    else if (IS_IMM(ch)) snprintf(buf, sizeof(buf), "#r%s#n has slain #r%s#n in cold blood!", killer, target);
    else snprintf(buf, sizeof(buf), "#r%s#n has summoned forth %s hatred to destroy #r%s#n!", killer, chsex, target);

    buf[0] = UPPER(buf[0]);
    do_fatality(ch, buf);
    return;
}
