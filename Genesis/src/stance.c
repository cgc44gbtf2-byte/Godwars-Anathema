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


void improve_stance( CHAR_DATA *ch )
{
    char buf[MIL];
    char bufskill[50];
    char skillname[50];
    long max = 200, skill = ch->stance[0];

    if (IS_NPC(ch) || skill < 1 || skill > 10) return;

    if (ch->stance[skill] >= max)
    {
	ch->stance[skill] = max;
	return;
    }

    if ((ch->stance[skill] <= max * .25 && number_range(0, 1) == 0)
     || (ch->stance[skill] <= max * .75 && number_range(0, 3) == 0)
     || number_range(0, 7) == 0)
	ch->stance[skill]++;
    else return;

    if      (ch->stance[skill] == 1)	sprintf(bufskill, "an apprentice");
    else if (ch->stance[skill] == 50)	sprintf(bufskill, "a student");
    else if (ch->stance[skill] == 100)	sprintf(bufskill, "an expert");
    else if (ch->stance[skill] == 150)	sprintf(bufskill, "a master");
    else if (ch->stance[skill] == 200)	sprintf(bufskill, "a grand master");
    else return;
    if      (skill == STANCE_VIPER)	sprintf(skillname, "viper");
    else if (skill == STANCE_CRANE)	sprintf(skillname, "crane");
    else if (skill == STANCE_CRAB)	sprintf(skillname, "crab");
    else if (skill == STANCE_MONGOOSE)	sprintf(skillname, "mongoose");
    else if (skill == STANCE_BULL)	sprintf(skillname, "bull");
    else if (skill == STANCE_MANTIS)	sprintf(skillname, "mantis");
    else if (skill == STANCE_DRAGON)	sprintf(skillname, "dragon");
    else if (skill == STANCE_TIGER)	sprintf(skillname, "tiger");
    else if (skill == STANCE_MONKEY)	sprintf(skillname, "monkey");
    else if (skill == STANCE_SWALLOW)	sprintf(skillname, "swallow");
    else return;

    sprintf(buf, "#7You are now %s of the %s stance.#n\n\r", bufskill, skillname);
    stc(buf, ch);
    return;
}

void do_skill( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MIL];
    char buf[MSL];
    char bufskill[50];
    char skillname[50];
    long skill = 0;

    one_argument(argument, arg);

    if (IS_NPC(ch)) return;

    if (arg[0] == '\0') sprintf(arg, ch->name);

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
	stc("They aren't here.\n\r", ch);
	return;
    }

    skill = victim->style[0];

    if (skill < STYLE_UNARMED || skill > STYLE_TWOWEAPON)
    {
	stc(ch == victim ? "You are not using a fighting style.\n\r"
	                 : "They are not using a fighting style.\n\r", ch);
    }
    else
    {
    if      (victim->style[skill] < 1)		sprintf(bufskill, "totally unskilled");
    else if (victim->style[skill] < 50)		sprintf(bufskill, "slightly skilled");
    else if (victim->style[skill] < 100)	sprintf(bufskill, "fairly competent");
    else if (victim->style[skill] < 150)	sprintf(bufskill, "an expert");
    else if (victim->style[skill] < 200)	sprintf(bufskill, "a master");
    else if (victim->style[skill] >= 200)	sprintf(bufskill, "a grand master");
    else return;
    if      (skill == STYLE_UNARMED)	sprintf(skillname, "unarmed combat");
    else if (skill == STYLE_NATURAL)	sprintf(skillname, "natural weapons");
    else if (skill == STYLE_ONEHAND)	sprintf(skillname, "one-handed combat");
    else if (skill == STYLE_SHIELD)	sprintf(skillname, "shielded fighting");
    else if (skill == STYLE_TWOHAND)	sprintf(skillname, "two-handed combat");
    else if (skill == STYLE_TWOWEAPON)	sprintf(skillname, "two weapon fighting");
    else return;

    if (ch == victim) sprintf(buf, "You are a %s at %s.", bufskill, skillname);
    else sprintf(buf, "$N is a %s at %s.", bufskill, skillname);

    act(buf, ch, NULL, victim, TO_CHAR);
    }

    skill = victim->stance[0];

    /* Legacy records can use -1 for no stance; never index with it. */
    if (skill < STANCE_VIPER || skill > STANCE_SWALLOW)
    {
	stc(ch == victim ? "You are not in a fighting stance.\n\r"
	                 : "They are not in a fighting stance.\n\r", ch);
	return;
    }

    if      (victim->stance[skill] < 0)		sprintf(bufskill, "completely unskilled in");
    else if (victim->stance[skill] < 50)	sprintf(bufskill, "an apprentice of");
    else if (victim->stance[skill] < 100)	sprintf(bufskill, "a student of");
    else if (victim->stance[skill] < 150)	sprintf(bufskill, "an expert of");
    else if (victim->stance[skill] < 200)	sprintf(bufskill, "a master of");
    else if (victim->stance[skill] >= 200)	sprintf(bufskill, "a grand master of");
    else return;
    if      (skill == STANCE_VIPER)	sprintf(skillname, "viper");
    else if (skill == STANCE_CRANE)	sprintf(skillname, "crane");
    else if (skill == STANCE_CRAB)	sprintf(skillname, "crab");
    else if (skill == STANCE_MONGOOSE)	sprintf(skillname, "mongoose");
    else if (skill == STANCE_BULL)	sprintf(skillname, "bull");
    else if (skill == STANCE_MANTIS)	sprintf(skillname, "mantis");
    else if (skill == STANCE_DRAGON)	sprintf(skillname, "dragon");
    else if (skill == STANCE_TIGER)	sprintf(skillname, "tiger");
    else if (skill == STANCE_MONKEY)	sprintf(skillname, "monkey");
    else if (skill == STANCE_SWALLOW)	sprintf(skillname, "swallow");
    else return;

    if (ch == victim) sprintf(buf, "You are a %s at %s.", bufskill, skillname);
    else sprintf(buf, "$N is a %s at %s.", bufskill, skillname);

    act(buf, ch, NULL, victim, TO_CHAR);

    return;
}

/* Keep stance damage math with the stance commands.  The defender's
 * protection must apply even when the attacker has no active stance. */
long dambonus( CHAR_DATA *ch, CHAR_DATA *victim, long dam )
{
    if (dam < 1) return 1;

    if (IS_STANCE(ch, STANCE_BULL)) dam *= 1 + (ch->stance[STANCE_BULL] * 0.01);
    else if (IS_STANCE(ch, STANCE_DRAGON)) dam *= 1 + (ch->stance[STANCE_DRAGON] * 0.01);
    else if (IS_STANCE(ch, STANCE_TIGER)) dam *= 1 + (ch->stance[STANCE_TIGER] * 0.01);

    if (IS_STANCE(victim, STANCE_CRAB)) dam /= 1 + (victim->stance[STANCE_CRAB] * 0.01);
    else if (IS_STANCE(victim, STANCE_DRAGON)) dam /= 1 + (victim->stance[STANCE_DRAGON] * 0.01);
    else if (IS_STANCE(victim, STANCE_SWALLOW)) dam /= 1 + (victim->stance[STANCE_SWALLOW] * 0.01);
    return dam;
}

void do_stance( CHAR_DATA *ch, char *argument )
{
    char arg[MIL];
    char buf[MSL];
    long selection;
    bool got_adv = FALSE;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	if (ch->stance[0] != 0)
	{
	    ch->stance[0] = 0;
	    stc("You relax from your fighting stance.\n\r", ch);
	    act("$n relaxes from $s fighting stance.", ch, NULL, NULL, TO_ROOM);
	}
	else stc("You aren't in a fighting stance.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "viper")) {selection = STANCE_VIPER;
	stc("You arch your body into the viper fighting stance.\n\r", ch);
	act("$n arches $s body into the viper fighting stance.", ch, NULL, NULL, TO_ROOM);}
    else if (!str_cmp(arg,"crane")) {selection = STANCE_CRANE;
	send_to_char("You swing your body into the crane fighting stance.\n\r", ch);
	act("$n swings $s body into the crane fighting stance.", ch, NULL, NULL, TO_ROOM);}
    else if (!str_cmp(arg,"crab")) {selection = STANCE_CRAB;
	send_to_char("You squat down into the crab fighting stance.\n\r", ch);
	act("$n squats down into the crab fighting stance. ", ch, NULL, NULL, TO_ROOM);}
    else if (!str_cmp(arg,"mongoose")) {selection = STANCE_MONGOOSE;
	send_to_char("You twist into the mongoose fighting stance.\n\r", ch);
	act("$n twists into the mongoose fighting stance. ", ch, NULL, NULL, TO_ROOM);}
    else if (!str_cmp(arg,"bull")) {selection = STANCE_BULL;
	send_to_char("You hunch down into the bull fighting stance.\n\r", ch);
	act("$n hunches down into the bull fighting stance. ", ch, NULL, NULL, TO_ROOM);}
    else
    {
	strcpy(buf, "Advanced stances:");
	if (ch->stance[STANCE_BULL] >= 200 && ch->stance[STANCE_CRANE] >= 200)
	{ strcat(buf, " Tiger"); got_adv = TRUE; }
	if (ch->stance[STANCE_BULL] >= 200 && ch->stance[STANCE_CRAB] >= 200)
	{ strcat(buf, " Dragon"); got_adv = TRUE; }
	if (ch->stance[STANCE_CRAB] >= 200 && ch->stance[STANCE_MONGOOSE] >= 200)
	{ strcat(buf, " Swallow"); got_adv = TRUE; }
	if (ch->stance[STANCE_MONGOOSE] >= 200 && ch->stance[STANCE_VIPER] >= 200)
	{ strcat(buf, " Mantis"); got_adv = TRUE; }
	if (ch->stance[STANCE_CRANE] >= 200 && ch->stance[STANCE_VIPER] >= 200)
	{ strcat(buf, " Monkey"); got_adv = TRUE; }
	strcat(buf, ".\n\r");

	if (!str_cmp(arg,"mantis") && ch->stance[STANCE_MONGOOSE] >= 200 && ch->stance[STANCE_VIPER] >= 200)
	{
	    selection = STANCE_MANTIS;
	    send_to_char("You spin your body into the mantis fighting stance.\n\r",ch);
	    act("$n spins $s body into the mantis fighting stance.",ch,NULL,NULL,TO_ROOM);
	}
	else if (!str_cmp(arg,"dragon") && ch->stance[STANCE_BULL] >= 200 && ch->stance[STANCE_CRAB] >= 200)
	{
	    selection = STANCE_DRAGON;
	    send_to_char("You coil your body into the dragon fighting stance.\n\r",ch);
	    act("$n coils $s body into the dragon fighting stance.",ch,NULL,NULL,TO_ROOM);
	}
	else if (!str_cmp(arg,"tiger") && ch->stance[STANCE_BULL] >= 200 && ch->stance[STANCE_CRANE] >= 200)
	{
	    selection = STANCE_TIGER;
	    send_to_char("You lunge into the tiger fighting stance.\n\r",ch);
	    act("$n lunges into the tiger fighting stance.",ch,NULL,NULL,TO_ROOM);
	}
	else if (!str_cmp(arg,"monkey") && ch->stance[STANCE_CRANE] >= 200 && ch->stance[STANCE_VIPER] >= 200)
	{
	    selection = STANCE_MONKEY;
	    send_to_char("You rotate your body into the monkey fighting stance.\n\r",ch);
	    act("$n rotates $s body into the monkey fighting stance.",ch,NULL,NULL,TO_ROOM);
	}
	else if (!str_cmp(arg,"swallow") && ch->stance[STANCE_CRAB] >= 200 && ch->stance[STANCE_MONGOOSE] >= 200)
	{
	    selection = STANCE_SWALLOW;
	    send_to_char("You slide into the swallow fighting stance.\n\r",ch);
	    act("$n slides into the swallow fighting stance.",ch,NULL,NULL,TO_ROOM);
	}
	else
	{
	    send_to_char("Syntax is: stance <style>.\n\r",ch);
	    send_to_char("Basic stances: Viper Crane Crab Mongoose Bull.\n\r",ch);
	    if (got_adv) send_to_char(buf, ch);
	    return;
	}
    }
    ch->stance[0] = selection;
    WAIT_STATE(ch, 6);
    return;
}


/* Autostance - Kai */
void do_autostance( CHAR_DATA *ch, char *argument )
{
    char arg[MIL];
    char buf[MIL];

    one_argument(argument, arg);

    if (IS_NPC(ch)) return;

    if (!str_cmp(arg, "none")) ch->stance[11] = STANCE_NONE;
    else if (!str_cmp(arg, "viper")) ch->stance[11] = STANCE_VIPER;
    else if (!str_cmp(arg, "crane")) ch->stance[11] = STANCE_CRANE;
    else if (!str_cmp(arg, "crab")) ch->stance[11] = STANCE_CRAB;
    else if (!str_cmp(arg, "mongoose")) ch->stance[11] = STANCE_MONGOOSE;
    else if (!str_cmp(arg, "bull")) ch->stance[11] = STANCE_BULL;
    else if (!str_cmp(arg, "mantis") && ch->stance[STANCE_MONGOOSE] >= 200 && ch->stance[STANCE_VIPER] >= 200)
	ch->stance[11] = STANCE_MANTIS;
    else if (!str_cmp(arg, "dragon") && ch->stance[STANCE_BULL] >= 200 && ch->stance[STANCE_CRAB] >= 200)
	ch->stance[11] = STANCE_DRAGON;
    else if (!str_cmp(arg, "tiger") && ch->stance[STANCE_BULL] >= 200 && ch->stance[STANCE_CRANE] >= 200)
	ch->stance[11] = STANCE_TIGER;
    else if (!str_cmp(arg, "monkey") && ch->stance[STANCE_CRANE] >= 200 && ch->stance[STANCE_VIPER] >= 200)
	ch->stance[11] = STANCE_MONKEY;
    else if (!str_cmp(arg, "swallow") && ch->stance[STANCE_CRAB] >= 200 && ch->stance[STANCE_MONGOOSE] >= 200)
	ch->stance[11] = STANCE_SWALLOW;
    else
    {
	stc("Syntax is: autostance <style>.\n\r", ch);
	stc("Basic Stances: None, Viper, Crane, Crab, Mongoose, Bull.\n\r", ch);
	stc("Advanced Stances: Mantis, Dragon, Tiger, Monkey, Swallow.\n\r", ch);
	return;
    }

    sprintf(buf, "Autostance set to %s.\n\r", arg);
    stc(buf, ch);
    return;
}

void autostance( CHAR_DATA *ch )
{
    if (IS_NPC(ch)) return;
    if (ch->stance[11] == STANCE_NONE) return;
    if (ch->stance[0] > 0) return;
    if (ch->position <= POS_SLEEPING) return;

    if (ch->stance[11] == STANCE_VIPER) do_stance(ch, "viper");
    else if (ch->stance[11] == STANCE_CRANE) do_stance(ch, "crane");
    else if (ch->stance[11] == STANCE_CRAB) do_stance(ch, "crab");
    else if (ch->stance[11] == STANCE_MONGOOSE) do_stance(ch, "mongoose");
    else if (ch->stance[11] == STANCE_BULL) do_stance(ch, "bull");
    else if (ch->stance[11] == STANCE_MANTIS) do_stance(ch, "mantis");
    else if (ch->stance[11] == STANCE_DRAGON) do_stance(ch, "dragon");
    else if (ch->stance[11] == STANCE_TIGER) do_stance(ch, "tiger");
    else if (ch->stance[11] == STANCE_MONKEY) do_stance(ch, "monkey");
    else if (ch->stance[11] == STANCE_SWALLOW) do_stance(ch, "swallow");
    else stc("Autostance bug, inform Akuma.\n\r", ch);

    return;
}
