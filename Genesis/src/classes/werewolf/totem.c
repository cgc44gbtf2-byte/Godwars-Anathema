#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/player.h"

#define METIS_TOTEM_MANA 5000
#define METIS_TOTEM_ABSORPTION 10000
#define METIS_TOTEM_DURATION 10
#define METIS_TOTEM_RECOVERY 12

static const char *const tribe_totem_names[TRIBE_MAX + 1] =
{
    "Pegasus",
    "Rat",
    "Unicorn",
    "Stag",
    "Fenris",
    "Cockroach",
    "Griffin",
    "Grandfather Thunder",
    "Owl",
    "Falcon",
    "Chimera",
    "Uktena",
    "Wendigo"
};

static unsigned long next_totem_id = 1;

static OBJ_DATA *metis_totem_object(const CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if (ch == NULL || ch->metis_totem_id == 0)
        return NULL;

    for (obj = object_list; obj != NULL; obj = obj->next)
        if (obj->metis_totem_id == ch->metis_totem_id)
            return obj;

    return NULL;
}

void werewolf_totem_clear(CHAR_DATA *ch)
{
    OBJ_DATA *obj;
    OBJ_DATA *next;
    unsigned long id;

    if (ch == NULL)
        return;

    id = ch->metis_totem_id;
    ch->metis_totem_id = 0;
    ch->metis_totem_timer = 0;
    ch->metis_totem_absorb = 0;
    ch->metis_totem_room = NULL;

    if (id == 0)
        return;

    for (obj = object_list; obj != NULL; obj = next)
    {
        next = obj->next;
        if (obj->metis_totem_id == id)
            extract_obj(obj);
    }
}

void werewolf_totem_update(CHAR_DATA *ch)
{
    OBJ_DATA *obj;

    if (ch == NULL || ch->metis_totem_id == 0)
        return;

    obj = metis_totem_object(ch);
    if (!hybrid_player_save_valid(ch) || !IS_WEREWOLF(ch) ||
        get_breed(ch, BREED_METIS) < 5 ||
        ch->in_room == NULL || ch->in_room != ch->metis_totem_room ||
        obj == NULL || obj->in_room != ch->in_room ||
        ch->position == POS_DEAD)
    {
        werewolf_totem_clear(ch);
        return;
    }

    if (ch->metis_totem_timer <= 1)
    {
        werewolf_totem_clear(ch);
        stc("The tribal totem's barrier fades away.\n\r", ch);
        return;
    }

    --ch->metis_totem_timer;
}

long werewolf_totem_absorb(CHAR_DATA *ch, long damage)
{
    OBJ_DATA *obj;
    long absorbed;

    if (ch == NULL || damage <= 0 || ch->metis_totem_id == 0)
        return damage;

    obj = metis_totem_object(ch);
    if (!hybrid_player_save_valid(ch) || !IS_WEREWOLF(ch) ||
        get_breed(ch, BREED_METIS) < 5 ||
        ch->in_room == NULL || ch->in_room != ch->metis_totem_room ||
        obj == NULL || obj->in_room != ch->in_room ||
        ch->position == POS_DEAD || ch->metis_totem_timer <= 0 ||
        ch->metis_totem_absorb <= 0)
    {
        werewolf_totem_clear(ch);
        return damage;
    }

    absorbed = damage < ch->metis_totem_absorb
        ? damage : ch->metis_totem_absorb;
    ch->metis_totem_absorb -= absorbed;
    damage -= absorbed;

    if (ch->metis_totem_absorb == 0)
    {
        werewolf_totem_clear(ch);
        stc("Your tribal totem absorbs the last of the blow, then shatters.\n\r", ch);
    }
    else
        stc("Your tribal totem flares, absorbing part of the blow.\n\r", ch);

    return damage;
}

void do_totemgift(CHAR_DATA *ch, char *argument)
{
    OBJ_INDEX_DATA *prototype;
    OBJ_DATA *totem;
    char arg[MAX_INPUT_LENGTH];
    char extra[MAX_INPUT_LENGTH];
    char name[MAX_STRING_LENGTH];
    char short_descr[MAX_STRING_LENGTH];
    char description[MAX_STRING_LENGTH];
    long tribe;
    unsigned long id;

    if (ch == NULL || IS_NPC(ch) || !IS_WEREWOLF(ch) ||
        !hybrid_player_save_valid(ch) || ch->in_room == NULL ||
        ch->hit <= 0 || get_breed(ch, BREED_METIS) < 5)
    {
        if (ch != NULL)
            stc("Only a native Metis Werewolf with rank-five training can call this gift.\n\r", ch);
        return;
    }

    argument = one_argument(argument != NULL ? argument : "", arg);
    one_argument(argument, extra);
    if (arg[0] != '\0' || extra[0] != '\0')
    {
        stc("Use Totemgift without a target.\n\r", ch);
        return;
    }

    if (ch->position < POS_FIGHTING || ch->wait > 0)
    {
        stc("You must be ready to call your totem.\n\r", ch);
        return;
    }

    tribe = ch->pcdata->tgift[2];
    if (tribe < 0 || tribe > TRIBE_MAX || get_tribe(ch, tribe) < 1)
    {
        stc("Choose and train a primary tribe before calling its totem.\n\r", ch);
        return;
    }

    if (ch->metis_totem_id != 0)
    {
        if (metis_totem_object(ch) != NULL)
        {
            stc("Your tribal totem is already protecting you.\n\r", ch);
            return;
        }
        werewolf_totem_clear(ch);
    }

    if (ch->mana < METIS_TOTEM_MANA)
    {
        stc("You need 5000 mana to call your tribal totem.\n\r", ch);
        return;
    }
    if (next_totem_id == 0)
    {
        stc("The spirit world cannot answer your call right now.\n\r", ch);
        return;
    }

    prototype = get_obj_index(OBJ_VNUM_LIGHT_BALL);
    if (prototype == NULL || prototype->item_type != ITEM_LIGHT)
    {
        stc("The spirit world cannot answer your call right now.\n\r", ch);
        return;
    }

    id = next_totem_id;
    next_totem_id = id == ULONG_MAX ? 0 : id + 1;

    totem = create_object(prototype, ch->level);
    snprintf(name, sizeof(name), "tribal totem %s", tribe_totem_names[tribe]);
    snprintf(short_descr, sizeof(short_descr), "a %s tribal totem",
             tribe_totem_names[tribe]);
    snprintf(description, sizeof(description),
             "A %s tribal totem stands here, its spirit ward ready to shield its summoner.",
             tribe_totem_names[tribe]);
    free_string(totem->name);
    free_string(totem->short_descr);
    free_string(totem->description);
    totem->name = str_dup(name);
    totem->short_descr = str_dup(short_descr);
    totem->description = str_dup(description);
    totem->item_type = ITEM_TRASH;
    totem->wear_flags = 0;
    totem->weight = 0;
    totem->cost = 0;
    totem->points = 0;
    totem->timer = 0;
    SET_BIT(totem->extra_flags, ITEM_VANISH | ITEM_NODROP);
    totem->metis_totem_id = id;
    obj_to_room(totem, ch->in_room);

    ch->metis_totem_id = id;
    ch->metis_totem_room = ch->in_room;
    ch->metis_totem_timer = METIS_TOTEM_DURATION;
    ch->metis_totem_absorb = METIS_TOTEM_ABSORPTION;
    ch->mana -= METIS_TOTEM_MANA;
    WAIT_STATE(ch, METIS_TOTEM_RECOVERY);

    stc("You call your tribe's totem. It stands watch and can absorb up to 10000 damage for you.\n\r", ch);
    act("$n calls a spirit totem, which takes a stationary place nearby.", ch, NULL, NULL, TO_ROOM);
}
