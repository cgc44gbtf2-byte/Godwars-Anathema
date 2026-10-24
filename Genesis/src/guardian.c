/* One shared warning toggle. Never infer a grant from the stored flag itself. */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "guardian.h"
#include "classes/hybrid/access.h"

bool guardian_eligible(CHAR_DATA *ch)
{
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL) return FALSE;
    /* Original-class grants survive an unavailable/rejected secondary bank. */
    if (IS_VAMPIRE(ch) && get_disc(ch, DISC_NECROMANCY) >= 4) return TRUE;
    if (IS_WEREWOLF(ch) && get_tribe(ch, TRIBE_CHILDREN_OF_GAIA) >= 4) return TRUE;
    return hybrid_power_parent(ch, CLASS_MONK) &&
        hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) >= 6;
}

long guardian_saved_more(CHAR_DATA *ch, long more)
{
    /* Pure save projection: never mutate the live actor during a disk attempt. */
    if (IS_SET(more, MORE_GUARDIAN) && !guardian_eligible(ch))
        return more & ~MORE_GUARDIAN;
    return more;
}

void guardian_reconcile(CHAR_DATA *ch)
{
    /* NPC prototype flags are outside the player guardian lifecycle. */
    if (ch == NULL || IS_NPC(ch) || !IS_MORE(ch, MORE_GUARDIAN)) return;
    ch->more = guardian_saved_more(ch, ch->more);
}
