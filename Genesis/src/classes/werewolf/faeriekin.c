#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <string.h>
#include "merc.h"
#include "classes/werewolf/faeriekin.h"

void do_faeriekin(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *spirit;
    MOB_INDEX_DATA *prototype;
    AFFECT_DATA af;
    long sn;

    (void)argument;
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL)
        return;

    if (!IS_WEREWOLF(ch))
    {
        stc("Huh?\n\r", ch);
        return;
    }

    if (get_tribe(ch, TRIBE_FIANNA) < 3)
    {
        stc("You require Fianna 3 to use Faerie Kin.\n\r", ch);
        return;
    }

    if (ch->pcdata->followers >= 1)
    {
        stc("Nothing happens.\n\r", ch);
        return;
    }

    prototype = get_mob_index(MOB_VNUM_FAERIE_SPIRIT);
    sn = skill_lookup("guardian");
    if (prototype == NULL || sn < 0)
    {
        stc("The Faerie Kin spirit is unavailable; please tell staff.\n\r", ch);
        return;
    }

    spirit = create_mobile(prototype);
    free_string(spirit->lord);
    spirit->lord = str_dup(ch->name);
    free_string(spirit->name);
    spirit->name = str_dup("faerie spirit");
    free_string(spirit->short_descr);
    spirit->short_descr = str_dup("a faerie spirit");
    free_string(spirit->long_descr);
    spirit->long_descr = str_dup("A faerie spirit shimmers here, ready to help.\n\r");
    free_string(spirit->description);
    spirit->description = str_dup("A small spirit of the Wyld, bright-eyed and watchful.\n\r");
    spirit->level = 30;
    spirit->hit = 2500;
    spirit->max_hit = 2500;
    spirit->mana = 0;
    spirit->max_mana = 0;
    spirit->hitroll = 30;
    spirit->damroll = 20;
    spirit->armor = 50;
    char_to_room(spirit, ch->in_room);

    ch->pcdata->followers++;
    add_follower(spirit, ch);
    af.type = sn;
    af.duration = FAERIE_KIN_DURATION;
    af.location = APPLY_NONE;
    af.modifier = MOB_VNUM_FAERIE_SPIRIT;
    af.bitvector = AFF_CHARM;
    affect_to_char(spirit, &af);
    stc("You call a faerie spirit from the Wyld to aid you.\n\r", ch);
    act("$n calls a faerie spirit from the Wyld.", ch, NULL, NULL, TO_ROOM);
}
