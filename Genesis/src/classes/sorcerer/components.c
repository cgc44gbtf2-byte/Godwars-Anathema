#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/sorcerer/components.h"

static const SORC_COMPONENT runes[SORC_RUNES] = {
    {"fire", 1}, {"air", 2}, {"earth", 4}, {"water", 8}, {"dark", 16},
    {"light", 32}, {"life", 64}, {"death", 128}, {"mind", 256},
    {"spirit", 512}, {"master", SORC_RUNE_MASTER}
};
static const SORC_COMPONENT glyphs[SORC_GLYPHS] = {
    {"creation", 1}, {"destruction", 2}, {"summoning", 4},
    {"transformation", 8}, {"transportation", 16}, {"enhancement", 32},
    {"reduction", 64}, {"control", 128}, {"protection", 256},
    {"information", 512}
};
static const SORC_COMPONENT sigils[SORC_SIGILS] = {
    {"self", 1}, {"targeting", 2}, {"area", 4}, {"object", 8}
};
static const char *kind_names[3] = {"rune", "glyph", "sigil"};

const SORC_COMPONENT *sorc_component_table(int kind, int *count)
{
    switch (kind)
    {
    case SORC_RUNE: if (count) *count = SORC_RUNES; return runes;
    case SORC_GLYPH: if (count) *count = SORC_GLYPHS; return glyphs;
    case SORC_SIGIL: if (count) *count = SORC_SIGILS; return sigils;
    }
    if (count) *count = 0;
    return NULL;
}

const SORC_COMPONENT *sorc_component_find(int kind, const char *name)
{
    int n, i;
    const SORC_COMPONENT *t = sorc_component_table(kind, &n);
    if (t == NULL || name == NULL || name[0] == '\0') return NULL;
    for (i = 0; i < n; i++)
        if (!str_cmp(name, t[i].name)) return &t[i];
    return NULL;
}

bool sorc_can_use_components(const CHAR_DATA *ch)
{
    return ch != NULL && !IS_NPC(ch) && ch->pcdata != NULL
        && hybrid_power_parent(ch, CLASS_SORCERER);
}

bool sorc_is_apprentice(const CHAR_DATA *ch)
{
    return ch != NULL && !IS_NPC(ch) && ch->pcdata != NULL
        && ch->pcdata->apprentice[0] == APP_APPRENTICE;
}

bool sorc_knows(const CHAR_DATA *ch, int kind, long bit)
{
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || kind < 0 || kind > 2)
        return FALSE;
    return (ch->pcdata->components[kind] & bit) != 0;
}

int sorc_component_count(const CHAR_DATA *ch, int kind)
{
    int n, i, c = 0;
    const SORC_COMPONENT *t = sorc_component_table(kind, &n);
    if (t == NULL || ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return 0;
    for (i = 0; i < n; i++)
        if (ch->pcdata->components[kind] & t[i].bit) c++;
    return c;
}

long sorc_component_cost(const CHAR_DATA *ch, int kind)
{
    int known = sorc_component_count(ch, kind);
    long cost;
    switch (kind)
    {
    case SORC_RUNE: return 20L * (known + 1);
    case SORC_GLYPH: cost = 10L * (known + 1); return cost > 50 ? 50 : cost;
    case SORC_SIGIL: return 30;
    }
    return 0;
}

static int apprentice_limit(int kind)
{
    return kind == SORC_RUNE ? 3 : 2;
}

SORC_LEARN_RESULT sorc_learn(CHAR_DATA *ch, int kind, const char *name)
{
    const SORC_COMPONENT *c;
    long cost;

    if (!sorc_can_use_components(ch)) return SORC_LEARN_NOT_SORCERER;
    if ((c = sorc_component_find(kind, name)) == NULL) return SORC_LEARN_BAD_NAME;
    if (ch->pcdata->components[kind] & c->bit) return SORC_LEARN_KNOWN;

    if (kind == SORC_GLYPH && sorc_component_count(ch, SORC_RUNE) < 1)
        return SORC_LEARN_PREREQ;
    if (kind == SORC_SIGIL && sorc_component_count(ch, SORC_GLYPH) < 1)
        return SORC_LEARN_PREREQ;
    if (kind == SORC_RUNE && c->bit == SORC_RUNE_MASTER
     && sorc_component_count(ch, SORC_RUNE) < SORC_RUNES - 1)
        return SORC_LEARN_PREREQ;
    if (sorc_is_apprentice(ch)
     && sorc_component_count(ch, kind) >= apprentice_limit(kind))
        return SORC_LEARN_LIMIT;

    cost = sorc_component_cost(ch, kind);
    if (ch->primal < cost) return SORC_LEARN_COST;

    ch->primal -= cost;
    ch->pcdata->components[kind] |= c->bit;
    return SORC_LEARN_OK;
}

static void list_kind(CHAR_DATA *ch, int kind)
{
    char buf[MSL];
    int n, i;
    const SORC_COMPONENT *t = sorc_component_table(kind, &n);
    sprintf(buf, "%ss:", kind_names[kind]);
    buf[0] = UPPER(buf[0]);
    for (i = 0; i < n; i++)
    {
        strcat(buf, " ");
        strcat(buf, t[i].name);
        strcat(buf, sorc_knows(ch, kind, t[i].bit) ? "(known)" : "");
    }
    sprintf(buf + strlen(buf), "  [next cost %li primal]\n\r", sorc_component_cost(ch, kind));
    stc(buf, ch);
}

void do_learn(CHAR_DATA *ch, char *argument)
{
    char arg1[MIL], arg2[MIL], buf[MSL];
    int kind;

    if (!sorc_can_use_components(ch))
    {
        stc("Huh?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
        list_kind(ch, SORC_RUNE);
        list_kind(ch, SORC_GLYPH);
        list_kind(ch, SORC_SIGIL);
        stc("Learn: learn <rune|glyph|sigil> <name>. Glyphs need a rune, sigils need a glyph, "
            "Master needs all ten element runes.\n\r", ch);
        return;
    }

    if (!str_cmp(arg1, "rune")) kind = SORC_RUNE;
    else if (!str_cmp(arg1, "glyph")) kind = SORC_GLYPH;
    else if (!str_cmp(arg1, "sigil")) kind = SORC_SIGIL;
    else
    {
        do_learn(ch, "");
        return;
    }

    switch (sorc_learn(ch, kind, arg2))
    {
    case SORC_LEARN_OK:
        sprintf(buf, "You learn the %s %s.\n\r", arg2, kind_names[kind]);
        stc(buf, ch);
        break;
    case SORC_LEARN_BAD_NAME:
        sprintf(buf, "There is no such %s. Use 'learn' to list them.\n\r", kind_names[kind]);
        stc(buf, ch);
        break;
    case SORC_LEARN_KNOWN:
        stc("You already know that.\n\r", ch);
        break;
    case SORC_LEARN_PREREQ:
        stc("You have not met the prerequisite for that component.\n\r", ch);
        break;
    case SORC_LEARN_LIMIT:
        stc("As an apprentice you cannot learn more of that kind until you graduate.\n\r", ch);
        break;
    case SORC_LEARN_COST:
        sprintf(buf, "You need %li primal to learn that.\n\r", sorc_component_cost(ch, kind));
        stc(buf, ch);
        break;
    default:
        stc("Huh?\n\r", ch);
    }
}
