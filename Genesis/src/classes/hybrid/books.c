#include <sys/types.h>
#include "../../merc.h"
#include "books.h"
#include "player.h"
#include "access.h"

long hybrid_book_chain_limit(const CHAR_DATA *ch)
{
    if (!ch) return 0;
    if (IS_NPC(ch)) return 10; /* No hybrid player record belongs to an NPC. */
    if (!hybrid_player_save_valid(ch)) return 0;
    if (!ch->pcdata->hybrid_present) return 10;
    long limit = hybrid_book_chain_policy(
        hybrid_by_id(ch->pcdata->hybrid.hybrid_id));
    return limit ? limit : 10;
}

long hybrid_book_color_rank(CHAR_DATA *ch, long color)
{
    if (color < 0 || color > 5 || !hybrid_player_save_valid(ch)) return 0;
    return hybrid_power_rank(ch, CLASS_SORCERER, HYBRID_TRAIN_COLOR,
                             (size_t)color);
}
