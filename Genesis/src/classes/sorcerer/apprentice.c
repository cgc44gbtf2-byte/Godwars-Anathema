#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "classes/sorcerer/components.h"

#define LESSONS_TO_GRADUATE 5
#define LESSON_COOLDOWN 600
#define OFFER_LIFETIME 120
#define PLAYER_LESSON_COST 5
#define NPC_LESSON_COST 10
#define GRADUATION_REWARD 100

static struct
{
    char mentor[MIL];
    char apprentice[MIL];
    time_t when;
} offer;

static bool is_mentor(const CHAR_DATA *ch)
{
    return sorc_can_use_components(ch) && !sorc_is_apprentice(ch);
}

static long color_total(const CHAR_DATA *ch)
{
    long total = 0;
    int i;
    for (i = 0; i < 6; i++) total += ch->pcdata->colors[i];
    return total;
}

static bool requirements_met(const CHAR_DATA *ch)
{
    return ch->pcdata->apprentice[1] >= LESSONS_TO_GRADUATE
        && sorc_component_count(ch, SORC_RUNE) >= 3
        && sorc_component_count(ch, SORC_GLYPH) >= 1
        && sorc_component_count(ch, SORC_SIGIL) >= 1
        && color_total(ch) >= 3;
}

/* Returns TRUE only on the single transition to graduated. */
static bool try_graduate(CHAR_DATA *ch)
{
    if (!sorc_is_apprentice(ch) || !requirements_met(ch)) return FALSE;
    ch->pcdata->apprentice[0] = APP_GRADUATED;
    if (!ch->pcdata->apprentice[3])
    {
        ch->pcdata->apprentice[3] = 1;
        ch->primal += GRADUATION_REWARD;
    }
    stc("You have completed your apprenticeship and graduated as a full Sorcerer!\n\r", ch);
    return TRUE;
}

static void clear_offer(void)
{
    memset(&offer, 0, sizeof offer);
}

static bool lesson(CHAR_DATA *apprentice, long cost, const char *teacher)
{
    char buf[MSL];

    if (!sorc_can_use_components(apprentice) || !sorc_is_apprentice(apprentice))
    {
        stc("You are not an apprentice.\n\r", apprentice);
        return FALSE;
    }
    if (apprentice->pcdata->apprentice[2] != 0
     && current_time - apprentice->pcdata->apprentice[2] < LESSON_COOLDOWN)
    {
        stc("You need time to absorb your last lesson.\n\r", apprentice);
        return FALSE;
    }
    if (apprentice->pcdata->apprentice[1] >= LESSONS_TO_GRADUATE)
    {
        stc("You have no more lessons to take; learn the rest and use 'apprentice graduate'.\n\r", apprentice);
        return FALSE;
    }
    if (apprentice->primal < cost)
    {
        sprintf(buf, "A lesson costs %li primal.\n\r", cost);
        stc(buf, apprentice);
        return FALSE;
    }
    apprentice->primal -= cost;
    apprentice->pcdata->apprentice[1]++;
    apprentice->pcdata->apprentice[2] = current_time;
    sprintf(buf, "%s teaches you a lesson of the arcane (%li of %d).\n\r", teacher,
            apprentice->pcdata->apprentice[1], LESSONS_TO_GRADUATE);
    stc(buf, apprentice);
    try_graduate(apprentice);
    return TRUE;
}

static void status(CHAR_DATA *ch)
{
    char buf[MSL];
    if (sorc_is_apprentice(ch))
    {
        sprintf(buf, "You are an apprentice: %li/%d lessons, %d runes (3), %d glyphs (1), %d sigils (1), "
                "color ranks %li (3).\n\r", ch->pcdata->apprentice[1], LESSONS_TO_GRADUATE,
                sorc_component_count(ch, SORC_RUNE), sorc_component_count(ch, SORC_GLYPH),
                sorc_component_count(ch, SORC_SIGIL), color_total(ch));
        stc(buf, ch);
        stc("Find a Sorcerer mentor ('apprentice accept' after they offer) or a trainer ('apprentice train'). "
            "Use 'apprentice graduate' when ready.\n\r", ch);
    }
    else stc("You are a full Sorcerer. You may offer lessons with 'apprentice teach <player>'.\n\r", ch);
}

void do_apprentice(CHAR_DATA *ch, char *argument)
{
    char arg[MIL], arg2[MIL];
    CHAR_DATA *other;

    if (!sorc_can_use_components(ch))
    {
        stc("Huh?\n\r", ch);
        return;
    }
    argument = one_argument(argument, arg);
    one_argument(argument, arg2);

    if (arg[0] == '\0')
    {
        status(ch);
        return;
    }
    if (!str_cmp(arg, "teach"))
    {
        if (!is_mentor(ch))
        {
            stc("Only a graduated Sorcerer can teach.\n\r", ch);
            return;
        }
        if (arg2[0] == '\0' || (other = get_char_room(ch, arg2)) == NULL)
        {
            stc("Teach whom?\n\r", ch);
            return;
        }
        if (other == ch || IS_NPC(other) || !sorc_is_apprentice(other) || other->fighting != NULL
         || ch->fighting != NULL)
        {
            stc("They cannot be taught by you right now.\n\r", ch);
            return;
        }
        clear_offer();
        strncpy(offer.mentor, ch->name, sizeof(offer.mentor) - 1);
        strncpy(offer.apprentice, other->name, sizeof(offer.apprentice) - 1);
        offer.when = current_time;
        act("You offer $N a lesson.", ch, NULL, other, TO_CHAR);
        act("$n offers you a lesson. Use 'apprentice accept' or 'apprentice decline'.", ch, NULL, other, TO_VICT);
        return;
    }
    if (!str_cmp(arg, "accept") || !str_cmp(arg, "decline"))
    {
        CHAR_DATA *mentor = NULL;
        if (!sorc_is_apprentice(ch) || offer.mentor[0] == '\0' || str_cmp(offer.apprentice, ch->name)
         || current_time - offer.when > OFFER_LIFETIME)
        {
            clear_offer();
            stc("Nobody is offering you a lesson.\n\r", ch);
            return;
        }
        for (other = ch->in_room != NULL ? ch->in_room->people : NULL; other != NULL; other = other->next_in_room)
            if (!IS_NPC(other) && !str_cmp(other->name, offer.mentor)) mentor = other;
        clear_offer();
        if (!str_cmp(arg, "decline"))
        {
            stc("You decline the lesson.\n\r", ch);
            return;
        }
        if (mentor == NULL || mentor == ch || !is_mentor(mentor) || mentor->fighting != NULL)
        {
            stc("Your mentor is no longer able to teach you.\n\r", ch);
            return;
        }
        if (lesson(ch, PLAYER_LESSON_COST, mentor->name))
            act("You teach $N.", mentor, NULL, ch, TO_CHAR);
        return;
    }
    if (!str_cmp(arg, "train"))
    {
        for (other = ch->in_room != NULL ? ch->in_room->people : NULL; other != NULL; other = other->next_in_room)
            if (IS_NPC(other) && other->name != NULL && is_name("sorctrainer", other->name)
             && IS_AWAKE(other) && other->fighting == NULL) break;
        if (other == NULL)
        {
            stc("There is no trainer here.\n\r", ch);
            return;
        }
        if (ch->fighting != NULL)
        {
            stc("Not while fighting!\n\r", ch);
            return;
        }
        lesson(ch, NPC_LESSON_COST, other->short_descr != NULL ? other->short_descr : "The trainer");
        return;
    }
    if (!str_cmp(arg, "graduate"))
    {
        if (!sorc_is_apprentice(ch))
        {
            stc("You are not an apprentice.\n\r", ch);
            return;
        }
        if (!try_graduate(ch)) stc("You do not meet the graduation requirements yet.\n\r", ch);
        return;
    }
    stc("Use: apprentice [teach <player>|accept|decline|train|graduate]\n\r", ch);
}
