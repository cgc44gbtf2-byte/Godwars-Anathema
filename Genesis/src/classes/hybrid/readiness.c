/* Read-only physical prerequisites. No conversion or training mutation. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "hybrid.h"
#include "readiness.h"

_Static_assert(2 + STAT_WIS - STAT_STR + 1 +
    STANCE_SWALLOW - STANCE_VIPER + 1 + TYPE_BLUNT - TYPE_UNARMED + 1 +
    STYLE_TWOWEAPON - STYLE_UNARMED + 1 == HYBRID_READINESS_ITEMS,
    "Update readiness capacity when native skill slots change");
_Static_assert(STAT_STR == 0 && STAT_WIS == 4 && STANCE_VIPER == 1 &&
    STANCE_SWALLOW == 10 && TYPE_UNARMED == 1 && TYPE_BLUNT == 4 &&
    STYLE_UNARMED == 1 && STYLE_TWOWEAPON == 6,
    "Update readiness label mapping when native IDs change");

/* Mirror native improve_style/improve_weapon, NOT get_style's generic200
 * normalization. Real-engine tests compare every class/slot against native
 * improvement at an over-cap rank; policy drift fails the regression suite.
 * Do not change native combat/training to make these prerequisites pass. */
static long style_cap(const CHAR_DATA *ch, long style)
{
    if (style == STYLE_UNARMED) {
        if (IS_MONK(ch)) return 200;
        if (IS_NINJA(ch)) return 150;
    }
    if (style == STYLE_NATURAL && IS_DRAGON(ch)) return 150;
    if (style == STYLE_ONEHAND) {
        if (IS_HIGHLANDER(ch)) return 200;
        if (IS_SORCERER(ch)) return 150;
    }
    if (style == STYLE_SHIELD) {
        if (IS_ELEM(ch)) return 150;
        if (IS_HIGHLANDER(ch)) return 200;
    }
    if (style == STYLE_TWOHAND && IS_HIGHLANDER(ch)) return 200;
    if (style == STYLE_TWOWEAPON && IS_DROW(ch)) return 150;
    return 100;
}

static long weapon_cap(const CHAR_DATA *ch, long weapon)
{
    if (weapon == TYPE_UNARMED && IS_MONK(ch)) return 200;
    if (weapon == TYPE_SLASH) {
        if (IS_DROW(ch)) return 200;
        if (IS_HIGHLANDER(ch) || IS_NINJA(ch) || IS_WEREWOLF(ch)) return 150;
    }
    if (weapon == TYPE_PIERCE && (IS_NINJA(ch) || IS_VAMPIRE(ch))) return 150;
    return 100;
}

static void add(HYBRID_READINESS *r, enum hybrid_readiness_kind kind,
                long index, long current, long required, int exempt)
{
    HYBRID_READINESS_ITEM *item = &r->items[r->count++];
    item->kind = kind;
    item->index = index;
    item->current = current;
    item->required = required;
    if (exempt) { item->status = HYBRID_READY_EXEMPT; return; }
    r->applicable++;
    if (current < 0 || (kind != HYBRID_READY_HP && kind != HYBRID_READY_MANA &&
                       current > required)) {
        item->status = HYBRID_READY_REVIEW;
        r->review++;
    } else if (current >= required) {
        item->status = HYBRID_READY_MET;
        r->met++;
    } else item->status = HYBRID_READY_MISSING;
}

int hybrid_readiness_read(const CHAR_DATA *ch, HYBRID_READINESS *report)
{
    HYBRID_READINESS result = {0};
    if (!report) return 0;
    /* On failure do not leave an older successful report in the destination. */
    memset(report, 0, sizeof(*report));
    if (!ch || IS_NPC(ch) || !ch->pcdata || !hybrid_parent_name(ch->class)) return 0;
    add(&result, HYBRID_READY_HP, 0, ch->max_hit, 50000, 0);
    add(&result, HYBRID_READY_MANA, 0, ch->max_mana, 50000, 0);
    for (long i = STAT_STR; i <= STAT_WIS; i++)
        add(&result, HYBRID_READY_STAT, i, ch->stats[i],
            stat_max((CHAR_DATA *)ch, i), 0);
    for (long i = STANCE_VIPER; i <= STANCE_SWALLOW; i++)
        add(&result, HYBRID_READY_STANCE, i, ch->stance[i], 200, 0);
    for (long i = TYPE_UNARMED; i <= TYPE_BLUNT; i++)
        add(&result, HYBRID_READY_WEAPON, i, ch->weapon[i], weapon_cap(ch, i), 0);
    for (long i = STYLE_UNARMED; i <= STYLE_TWOWEAPON; i++) {
        /* Equipment selection permits natural only for these claw-capable
         * original classes. Do not let retracting claws skip mastery; do not
         * demand an unreachable natural style of the other seven classes. */
        int exempt = i == STYLE_NATURAL &&
            !(IS_DRAGON(ch) || IS_VAMPIRE(ch) || IS_WEREWOLF(ch));
        add(&result, HYBRID_READY_STYLE, i, ch->style[i], style_cap(ch, i), exempt);
    }
    *report = result;
    return 1;
}

static const char *label(const HYBRID_READINESS_ITEM *item)
{
    static const char *stats[] = {"STR", "DEX", "CON", "INT", "WIS"};
    static const char *stances[] = {"none", "Viper", "Crane", "Crab",
        "Mongoose", "Bull", "Mantis", "Dragon", "Tiger", "Monkey", "Swallow"};
    static const char *weapons[] = {"none", "Unarmed weapon", "Slash", "Pierce", "Blunt"};
    static const char *styles[] = {"none", "Unarmed style", "Natural style",
        "One-handed", "Shielded", "Two-handed", "Two weapon"};
    switch (item->kind) {
    case HYBRID_READY_HP: return "Max HP";
    case HYBRID_READY_MANA: return "Max mana";
    case HYBRID_READY_STAT: return stats[item->index];
    case HYBRID_READY_STANCE: return stances[item->index];
    case HYBRID_READY_WEAPON: return weapons[item->index];
    case HYBRID_READY_STYLE: return styles[item->index];
    }
    return "Unknown";
}

void hybrid_readiness_show(CHAR_DATA *ch)
{
    HYBRID_READINESS report;
    char line[160];
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    if (!hybrid_readiness_read(ch, &report)) {
        send_to_char("Numeric readiness unavailable: select a supported base class.\n\r", ch);
        return;
    }
    snprintf(line, sizeof(line), "Numeric prerequisites: %zu/%zu met "
             "(not conversion eligibility).\n\r", report.met, report.applicable);
    send_to_char(line, ch);
    for (size_t i = 0; i < report.count; i++) {
        const HYBRID_READINESS_ITEM *item = &report.items[i];
        if (item->status == HYBRID_READY_MET) continue;
        if (item->status == HYBRID_READY_EXEMPT)
            snprintf(line, sizeof(line), "  %s: unavailable to this class; exempt.\n\r",
                     label(item));
        else
            snprintf(line, sizeof(line), "  %s: %ld/%ld%s\n\r", label(item),
                item->current, item->required,
                item->status == HYBRID_READY_REVIEW ? " (stored value needs review)" : "");
        send_to_char(line, ch);
    }
    send_to_char("Base stats exclude gear/form bonuses. Rank values above native caps "
                 "need review; nothing was normalized.\n\r"
                 "Class/build power mastery and safe conversion are still pending. "
                 "Conversion remains disabled.\n\r", ch);
}
