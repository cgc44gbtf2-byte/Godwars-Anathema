#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "classes/hybrid/access.h"

/* Keep Sorcerer grants separate from legacy/provider flags. In particular,
 * snapshots of old flags cannot protect a grant added while vision is active.
 * Existing Extra serialization carries these bits through both player loaders
 * and real copyover without projecting class identity or changing a codec. */
static long owned_sight(const CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata ||
        !(ch->extra & EXTRA_SORC_VISION)) return 0;
    return hybrid_power_parent(ch, CLASS_SORCERER)
        ? ch->extra & EXTRA_SORC_VISION : 0;
}

int effective_holylight(const CHAR_DATA *ch)
{
    return ch && (IS_SET(ch->act, PLR_HOLYLIGHT) ||
                  (owned_sight(ch) & EXTRA_SORC_HOLY));
}

int effective_shadowsight(const CHAR_DATA *ch)
{
    return ch && (IS_AFFECTED(ch, AFF_SHADOWSIGHT) ||
                  (owned_sight(ch) & EXTRA_SORC_SHADOW));
}

int effective_nightsight(const CHAR_DATA *ch)
{
    return ch && (IS_VAMPAFF(ch, VAM_NIGHTSIGHT) ||
                  (owned_sight(ch) & EXTRA_SORC_NIGHT));
}

void sorcerer_vision_toggle(CHAR_DATA *ch)
{
    if (!hybrid_power_parent(ch, CLASS_SORCERER)) {
        if (ch) stc("Huh?\n\r", ch);
        return;
    }
    if (ch->extra & EXTRA_SORC_HOLY) {
        REMOVE_BIT(ch->extra, EXTRA_SORC_VISION);
        stc("You allow your magically enhanced vision to fade.\n\r", ch);
    } else {
        SET_BIT(ch->extra, EXTRA_SORC_VISION);
        stc("You manipulate magical fields about yourself to enhance your vision.\n\r", ch);
    }
}

/* Hostile holy-sight removal still removes all holy grants, but must not erase
 * the remaining shadowsight/nightsight parts or independent provider flags. */
void strip_holylight(CHAR_DATA *ch)
{
    if (!ch) return;
    REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
    REMOVE_BIT(ch->extra, EXTRA_SORC_HOLY);
}

/* Broad affect resets (death/staff clearing) must also expire owned shadow
 * sight. Holy/night sight historically live outside affected_by and remain. */
void strip_shadowsight(CHAR_DATA *ch)
{
    if (!ch) return;
    REMOVE_BIT(ch->affected_by, AFF_SHADOWSIGHT);
    REMOVE_BIT(ch->extra, EXTRA_SORC_SHADOW);
}
