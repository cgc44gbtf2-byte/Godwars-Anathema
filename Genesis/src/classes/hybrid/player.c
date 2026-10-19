/* Optional player-file adapter. No conversion command or ancestry class gates. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "player.h"

int hybrid_secondary_initialize(const CHAR_DATA *ch,
                                const HYBRID_DEFINITION *definition,
                                HYBRID_STATE *destination)
{
    HYBRID_STATE candidate;
    if (!destination || !hybrid_state_prepare(ch, definition, &candidate)) return 0;
    switch (candidate.secondary_class) {
    case CLASS_VAMPIRE:
        /* Native autoclass: generation nine, basic disciplines start at -1.
         * -1 permits first learning; zero is unavailable until teaching.
         * Clan/profile/selections remain unchosen, so no effective access. */
        candidate.secondary_generation = 9;
        for (size_t i = 0; i < 20; ++i) candidate.secondary_powers[i] = -1;
        /* Native update_gen(9) capacity; do not modify primary willpower/blood. */
        candidate.secondary_blood[BLOOD_POOL] = 300;
        break;
    case CLASS_DRAGON:
        candidate.secondary_generation = AGE_HATCHLING;
        break;
    case CLASS_WEREWOLF:
        candidate.secondary_progression.wolf_rank = 1;
        break;
    case CLASS_MAGE:
        candidate.secondary_progression.arete = 1;
        break;
    default:
        break;
    }
    if (!hybrid_state_valid(&candidate)) return 0;
    *destination = candidate;
    return 1;
}

int hybrid_player_save_valid(const CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata || ch->pcdata->player_load_failed)
        return 0;
    return !ch->pcdata->hybrid_present ||
        (ch->pcdata->hybrid_present == 1 &&
         hybrid_state_valid(&ch->pcdata->hybrid) &&
         ch->class == ch->pcdata->hybrid.primary_class);
}

int hybrid_player_attach(CHAR_DATA *ch, const HYBRID_STATE *state)
{
    if (!hybrid_player_save_valid(ch) || ch->pcdata->hybrid_present ||
        !hybrid_state_valid(state) || ch->class != state->primary_class) return 0;
    ch->pcdata->hybrid = *state;
    ch->pcdata->hybrid_present = 1;
    return 1;
}

int hybrid_player_read_tail(CHAR_DATA *ch, FILE *stream)
{
    HYBRID_STATE candidate;
    if (!ch || !ch->pcdata) return 0;
    if (ch->pcdata->player_load_failed || ch->pcdata->hybrid_present ||
        !hybrid_state_read_tail(stream, &candidate)) {
        ch->pcdata->player_load_failed = 1;
        return 0;
    }
    /* Class may appear later in a legacy player record. Validate ownership at
     * explicit End, before equipment repair, authentication or any save. */
    ch->pcdata->hybrid = candidate;
    ch->pcdata->hybrid_present = 1;
    return 1;
}

int hybrid_player_load_finish(CHAR_DATA *ch)
{
    if (!hybrid_player_save_valid(ch)) {
        if (ch && ch->pcdata) ch->pcdata->player_load_failed = 1;
        return 0;
    }
    return 1;
}
