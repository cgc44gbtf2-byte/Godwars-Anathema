/* Transient combat interruption timers. Updated once per main-loop pulse,
 * before input, for every loaded character (including NPCs and link-dead PCs).
 * Never decrement these from descriptor processing or violence_update. */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "guardian.h"
#include "classes/hybrid/access.h"

/* Callback arguments are untrusted pointers until found in a live room list. */
bool combat_character_in_room(ROOM_INDEX_DATA *room, CHAR_DATA *ch)
{
    CHAR_DATA *member;
    if (room == NULL || ch == NULL) return FALSE;
    for (member = room->people; member != NULL; member = member->next_in_room)
        if (member == ch) return member->in_room == room;
    return FALSE;
}

/* Neither pointer may be dereferenced before membership is established. */
bool combat_pair_in_room(ROOM_INDEX_DATA *room, CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *member, *actor = NULL, *target = NULL;
    if (room == NULL || ch == NULL || victim == NULL) return FALSE;
    /* Neither callback argument may be dereferenced until membership is known. */
    for (member = room->people; member != NULL; member = member->next_in_room)
    {
        if (member == ch) actor = member;
        if (member == victim) target = member;
    }
    return actor != NULL && target != NULL && actor->in_room == room
        && target->in_room == room && actor->hit > 0
        && actor->position >= POS_FIGHTING && target->hit > 0
        && target->position != POS_DEAD;
}

bool combat_target_live(CHAR_DATA *ch, CHAR_DATA *victim)
{
    /* Class callers are players; generic callbacks use a cached room instead. */
    return ch != NULL && combat_pair_in_room(ch->in_room, ch, victim);
}

void combat_status_update(void)
{
    CHAR_DATA *ch;

    for (ch = char_list; ch != NULL; ch = ch->next)
    {
        guardian_reconcile(ch);
        if (ch->combat_pause > 0) --ch->combat_pause;
        if (ch->stun_immunity > 0) --ch->stun_immunity;
        if (ch->highland_guard > 0) --ch->highland_guard;
        if (ch->highland_riposte > 0) --ch->highland_riposte;
        if (ch->highland_counter_reuse > 0) --ch->highland_counter_reuse;
        if (IS_NPC(ch) || !hybrid_power_parent(ch, CLASS_HIGHLANDER)
            || ch->fighting == NULL)
            ch->highland_guard = ch->highland_riposte = 0;
    }
}
