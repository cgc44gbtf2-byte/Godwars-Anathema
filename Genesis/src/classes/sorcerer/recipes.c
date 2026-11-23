#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "classes/sorcerer/components.h"

extern char *target_name;

typedef enum { RECIPE_FOE, RECIPE_ALLY, RECIPE_SELF, RECIPE_OBJ, RECIPE_ROOM,
               RECIPE_AREAFOE, RECIPE_SPECIAL } RECIPE_MODE;
typedef enum { RECIPE_DIRECT, RECIPE_GUARDED, RECIPE_ADAPTED } RECIPE_CLASS;

typedef struct sorc_recipe
{
    long rune, glyph, sigil;
    const char *spell;
    int casts;
    RECIPE_MODE mode;
    RECIPE_CLASS kind;
    const char *note;
} SORC_RECIPE;

static const SORC_RECIPE recipes[] = {
#include "classes/sorcerer/recipe_table.h"
};
#define RECIPE_COUNT ((int)(sizeof(recipes) / sizeof(recipes[0])))

int sorc_recipe_count(void) { return RECIPE_COUNT; }

static const SORC_RECIPE *recipe_find(long rune, long glyph, long sigil)
{
    int i;
    for (i = 0; i < RECIPE_COUNT; i++)
        if (recipes[i].rune == rune && recipes[i].glyph == glyph && recipes[i].sigil == sigil)
            return &recipes[i];
    return NULL;
}

static bool recipe_known(const CHAR_DATA *ch, const SORC_RECIPE *r)
{
    return sorc_knows(ch, SORC_RUNE, r->rune) && sorc_knows(ch, SORC_GLYPH, r->glyph)
        && sorc_knows(ch, SORC_SIGIL, r->sigil);
}

static bool recipe_hostile(const SORC_RECIPE *r)
{
    return r->mode == RECIPE_FOE || r->mode == RECIPE_AREAFOE
        || !strcmp(r->spell, "earthquake") || !strcmp(r->spell, "call lightning")
        || !strcmp(r->spell, "gas breath");
}

static long recipe_mana(const CHAR_DATA *ch, const SORC_RECIPE *r, long base)
{
    long cost = base * r->casts;
    if (sorc_knows(ch, SORC_RUNE, SORC_RUNE_MASTER)) cost = cost * 3 / 4;
    return cost < 1 ? 1 : cost;
}

static bool foe_legal(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (victim == NULL || victim == ch) return FALSE;
    if (IS_IMM(victim) || IS_ITEMAFF(victim, ITEMA_REFLECT)) return FALSE;
    if (is_safe(ch, victim)) return FALSE;
    return TRUE;
}

static void cast_once(const SORC_RECIPE *r, long sn, CHAR_DATA *ch, void *vo)
{
    long level = 5 + ch->stats[STAT_INT] * 5;
    (*skill_table[sn].spell_fun)(sn, level, ch, vo);
}

static void list_recipes(CHAR_DATA *ch)
{
    char buf[MSL];
    int i, shown = 0;
    stc("Recipes you can cast: recipe <rune> <glyph> <sigil> [target]\n\r", ch);
    for (i = 0; i < RECIPE_COUNT; i++)
    {
        const SORC_RECIPE *r = &recipes[i];
        const SORC_COMPONENT *a, *b, *c;
        int n;
        if (!recipe_known(ch, r)) continue;
        for (a = sorc_component_table(SORC_RUNE, &n); a->bit != r->rune; a++) ;
        for (b = sorc_component_table(SORC_GLYPH, &n); b->bit != r->glyph; b++) ;
        for (c = sorc_component_table(SORC_SIGIL, &n); c->bit != r->sigil; c++) ;
        sprintf(buf, "  %s %s %s: %s\n\r", a->name, b->name, c->name, r->note);
        stc(buf, ch);
        shown++;
    }
    if (!shown) stc("You know no complete recipe yet. Use 'learn'.\n\r", ch);
}

void do_recipe(CHAR_DATA *ch, char *argument)
{
    char a1[MIL], a2[MIL], a3[MIL], buf[MSL];
    const SORC_COMPONENT *ru, *gl, *si;
    const SORC_RECIPE *r;
    CHAR_DATA *victim = NULL;
    OBJ_DATA *obj = NULL;
    void *vo = NULL;
    ROOM_INDEX_DATA *room;
    long sn = -1, mana;
    int n;

    if (!sorc_can_use_components(ch))
    {
        stc("Huh?\n\r", ch);
        return;
    }
    argument = one_argument(argument, a1);
    if (a1[0] == '\0')
    {
        list_recipes(ch);
        return;
    }
    argument = one_argument(argument, a2);
    argument = one_argument(argument, a3);
    ru = sorc_component_find(SORC_RUNE, a1);
    gl = sorc_component_find(SORC_GLYPH, a2);
    si = sorc_component_find(SORC_SIGIL, a3);
    if (ru == NULL || gl == NULL || si == NULL || ru->bit == SORC_RUNE_MASTER)
    {
        stc("Use: recipe <rune> <glyph> <sigil> [target]\n\r", ch);
        return;
    }
    if (!sorc_knows(ch, SORC_RUNE, ru->bit) || !sorc_knows(ch, SORC_GLYPH, gl->bit)
     || !sorc_knows(ch, SORC_SIGIL, si->bit))
    {
        stc("You have not learned all three components.\n\r", ch);
        return;
    }
    if ((r = recipe_find(ru->bit, gl->bit, si->bit)) == NULL)
    {
        stc("Nothing happens.\n\r", ch);
        return;
    }
    room = ch->in_room;
    if (room == NULL) return;
    if (IS_SET(room->added_flags, ROOM2_SILENCE) && !IS_BUI(ch))
    {
        stc("Your words make no sound!\n\r", ch);
        return;
    }
    if (ch->wait > 0 || ch->combat_pause > 0)
    {
        stc("You have not recovered enough to cast again.\n\r", ch);
        return;
    }
    if (recipe_hostile(r) && IS_SET(room->room_flags, ROOM_SAFE))
    {
        stc("You cannot cast that here.\n\r", ch);
        return;
    }

    if (r->mode != RECIPE_SPECIAL || strcmp(r->spell, "readaura") * strcmp(r->spell, "scry") * strcmp(r->spell, "preserve") != 0)
    {
        sn = skill_lookup(r->spell);
        if (sn < 0 || skill_table[sn].spell_fun == NULL)
        {
            stc("That recipe is unavailable.\n\r", ch);
            return;
        }
    }
    if (sn >= 0) mana = recipe_mana(ch, r, skill_table[sn].min_mana);
    else mana = recipe_mana(ch, r, 20);

    switch (r->mode)
    {
    case RECIPE_FOE:
        if (argument[0] == '\0') victim = ch->fighting;
        else victim = get_char_room(ch, argument);
        if (victim == NULL)
        {
            stc("Cast the recipe on whom?\n\r", ch);
            return;
        }
        if (!foe_legal(ch, victim)) return;
        vo = victim;
        break;
    case RECIPE_ALLY:
        victim = argument[0] == '\0' ? ch : get_char_room(ch, argument);
        if (victim == NULL)
        {
            stc("They aren't here.\n\r", ch);
            return;
        }
        vo = victim;
        break;
    case RECIPE_SELF:
        if (argument[0] != '\0' && !is_name(argument, ch->name))
        {
            stc("This recipe can only be cast on yourself.\n\r", ch);
            return;
        }
        vo = ch;
        break;
    case RECIPE_OBJ:
        if (argument[0] == '\0' || (obj = get_obj_carry(ch, argument)) == NULL)
        {
            stc("You are not carrying that.\n\r", ch);
            return;
        }
        vo = obj;
        break;
    case RECIPE_ROOM:
    case RECIPE_AREAFOE:
        break;
    case RECIPE_SPECIAL:
        if (!strcmp(r->spell, "preserve"))
        {
            if (argument[0] == '\0' || (obj = get_obj_carry(ch, argument)) == NULL)
            {
                stc("You are not carrying that.\n\r", ch);
                return;
            }
        }
        else if (!strcmp(r->spell, "summon"))
        {
            if (argument[0] == '\0' || ch->fighting != NULL)
            {
                stc("Summon whom, and not while fighting.\n\r", ch);
                return;
            }
            if ((victim = get_char_world(ch, argument)) == NULL || IS_IMM(victim))
            {
                stc("You failed.\n\r", ch);
                return;
            }
        }
        break;
    }
    if (!strcmp(r->spell, "teleport")
     && (ch->fighting != NULL || IS_SET(room->room_flags, ROOM_SAFE)
      || IS_SET(room->room_flags, ROOM_NO_TELEPORT)))
    {
        stc("You cannot do that here.\n\r", ch);
        return;
    }
    if (ch->mana < mana)
    {
        stc("You don't have enough mana.\n\r", ch);
        return;
    }

    ch->mana -= mana;
    if (sn >= 0) WAIT_STATE(ch, skill_table[sn].beats);
    else WAIT_STATE(ch, 12);
    act("You trace the runes of a recipe.", ch, NULL, NULL, TO_CHAR);
    act("$n traces glowing runes in the air.", ch, NULL, NULL, TO_ROOM);
    target_name = argument;

    if (r->mode == RECIPE_SPECIAL)
    {
        if (!strcmp(r->spell, "readaura")) do_readaura(ch, argument);
        else if (!strcmp(r->spell, "scry")) do_scry(ch, argument);
        else if (!strcmp(r->spell, "preserve"))
        {
            obj->timer = obj->timer > 0 ? UMIN(200, obj->timer + 24) : 0;
            act("$p is wrapped in preserving magic.", ch, obj, NULL, TO_CHAR);
        }
        else cast_once(r, sn, ch, NULL);
        return;
    }

    for (n = 0; n < r->casts; n++)
    {
        if (r->mode == RECIPE_AREAFOE)
        {
            CHAR_DATA *v, *next;
            for (v = room->people; v != NULL; v = next)
            {
                next = v->next_in_room;
                if (!foe_legal(ch, v) || v->fighting != ch) continue;
                cast_once(r, sn, ch, v);
            }
            continue;
        }
        if (victim != NULL && !combat_pair_in_room(room, ch, victim)) break;
        cast_once(r, sn, ch, vo);
    }
    if (r->mode == RECIPE_FOE && victim != NULL && combat_pair_in_room(room, ch, victim)
     && victim->fighting == NULL && IS_AWAKE(victim))
        multi_hit(victim, ch);
    (void)buf;
}
