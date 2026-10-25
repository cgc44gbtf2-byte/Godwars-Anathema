/* Registry-backed help fallback and class cross-links. No world files or
 * existing HELP_DATA are modified; file-backed custom topics take priority. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "hybrid.h"

static const struct {
    long parent;
    const char *keyword;
} parent_topics[] = {
    {CLASS_DRAGON, "Dragons"}, {CLASS_DROW, "Drow"},
    {CLASS_ELEM, "Elementalists"}, {CLASS_HIGHLANDER, "Highlanders"},
    {CLASS_MAGE, "Mages"}, {CLASS_MONK, "Monks"}, {CLASS_NINJA, "Ninja"},
    {CLASS_SORCERER, "Sorcerers"}, {CLASS_VAMPIRE, "Vampires"},
    {CLASS_WEREWOLF, "Werewolf"}
};

static void parent_rows(CHAR_DATA *ch, long parent)
{
    char display[128], line[256];
    for (size_t i = 0; i < hybrid_count(); i++) {
        const HYBRID_DEFINITION *h = hybrid_at(i);
        if (!hybrid_has_parent(h, parent)) continue;
        if (!hybrid_display_name_for_parent(h, parent, display, sizeof(display))) return;
        long other = h->parents[0] == parent ? h->parents[1] : h->parents[0];
        snprintf(line, sizeof(line), "  help %s  (Parent 2: %s%s#n)\n\r",
                 display, hybrid_parent_color(other), hybrid_parent_name(other));
        send_to_char(line, ch);
    }
}

void hybrid_help_parent_links(CHAR_DATA *ch, const char *keywords)
{
    if (!ch || !keywords) return;
    for (size_t i = 0; i < sizeof(parent_topics)/sizeof(parent_topics[0]); i++) {
        long parent = parent_topics[i].parent;
        if (!is_name(parent_topics[i].keyword, (char *)keywords) &&
            !is_name(hybrid_parent_name(parent), (char *)keywords)) continue;
        send_to_char("\n\rHybrid help topics for this original parent "
                     "(preview only):\n\r", ch);
        parent_rows(ch, parent);
        send_to_char("See also: help hybrid / help selfbrid. "
                     "Conversion remains disabled.\n\r", ch);
        return;
    }
}

int hybrid_help_topic(CHAR_DATA *ch, const char *argument)
{
    char line[256], display[128];
    if (!ch || !argument) return 0;
    if (!str_cmp(argument, "hybrid") || !str_cmp(argument, "hybrids") ||
        !str_cmp(argument, "selfbrid")) {
        snprintf(line, sizeof(line), "Hybrid catalog: %zu planned hybrids\n\r",
                 hybrid_count());
        send_to_char(line, ch);
        send_to_char("Parent 1 is your original class. Each hybrid appears under "
                     "both possible original parents.\n\r"
                     "This HELP catalog shows all pairs; the SELFBRID command "
                     "offers only your nine parent-compatible choices.\n\r"
                      "Use: selfbrid list / selfbrid info <listed name>.\n\r"
                      "The catalog itself is read-only; conversion is disabled. "
                      "Separate training controls affect only already attached "
                      "secondary-parent records.\n\r", ch);
        for (size_t i = 0; i < sizeof(parent_topics)/sizeof(parent_topics[0]); i++) {
            long parent = parent_topics[i].parent;
            snprintf(line, sizeof(line), "\n\rParent 1: %s%s#n\n\r",
                hybrid_parent_color(parent), hybrid_parent_name(parent));
            send_to_char(line, ch);
            parent_rows(ch, parent);
        }
        send_to_char("\n\rPlanned conversion: 50,000 quest points; original training "
                     "retained, second parent untrained.\n\r"
                     "Required: max original class/build powers, base stats, stances, "
                     "weapons and trainable styles; at least50k max HP/mana.\n\r"
                     "Numeric and original-class mastery previews exist; safe conversion remains "
                     "pending. Secondary ranked trees: one full branch, others rounded-up "
                     "half, within native caps. Branchless parents keep normal ability "
                     "rosters, not free learning.\n\r"
                     "Sorcerer chain limits: Mage6; Drow/Elementalist/Vampire4; "
                     "Dragon/Highlander/Monk/Ninja/Werewolf3. Both parent orders; "
                     "standalone Sorcerer10. Enforced for valid hybrid records; "
                     "Learned secondary colors affect book damage/affect pages; "
                     "native command gates remain. Conversion is disabled.\n\r", ch);
        send_to_char("Existing secondary Sorcerers: selfbrid colors shows ranks/caps. "
                     "Confirm one permanent full color before paid rank training; "
                     "this does not create a hybrid.\n\r",ch);
        send_to_char("Existing secondary Drow: selfbrid drow lists the currently "
                     "integrated powers. Attunement costs 1,000 DP; Faerie "
                     "Fire and Acid Bolt cost 5,000 DP each. Learn these through "
                     "selfbrid drow <power> learn. Other Drow powers are not "
                     "yet offered by this trainer.\n\r",ch);
        send_to_char("Existing secondary Werewolves: selfbrid wolf lists Breed, "
                     "Auspice and Tribe gifts with effective ranks/caps. Choose "
                     "one permanent full family (the others cap at3), then "
                     "choose one gift per family and buy ranks with native "
                     "primal pricing. This never changes a primary Werewolf's "
                     "native gifts. Some active Werewolf commands still require "
                     "hybrid runtime integration; conversion remains disabled.\n\r",ch);
        send_to_char("Existing secondary Sorcerers can sorccreate book/page/pen: "
                     "book or pen: 10 primal; page: free. With a held pen, write "
                     "titles and spell-page lines, then the native end.<color>. "
                     "Writing grants no color ranks. Enchant only gear claimed "
                     "in your own name: +5 hitroll/+5 damroll, once, no fee; "
                     "artifacts excluded. Vision is free and source-owned: "
                     "toggling it preserves other sight grants; legacy raw "
                     "flags require review before migration.\n\r",ch);
        return 1;
    }
    const HYBRID_DEFINITION *h = hybrid_by_name(argument);
    if (!h) return 0;
    long parent = hybrid_has_parent(h, ch->class) ? ch->class : h->parents[0];
    if (!hybrid_display_name_for_parent(h, parent, display, sizeof(display))) return 0;
    snprintf(line, sizeof(line), "%s - planned hybrid [%ld]\n\r", display, h->id);
    send_to_char(line, ch);
    snprintf(line, sizeof(line), "Parents: %s%s#n + %s%s#n. Either can be Parent 1.\n\r",
        hybrid_parent_color(h->parents[0]), hybrid_parent_name(h->parents[0]),
        hybrid_parent_color(h->parents[1]), hybrid_parent_name(h->parents[1]));
    send_to_char(line, ch);
    send_to_char("Original training is retained; the second parent starts untrained. "
                 "There is no playable conversion yet.\n\r", ch);
    if (hybrid_has_parent(h, CLASS_WEREWOLF))
        send_to_char("For an existing secondary Werewolf record: selfbrid wolf "
                     "lists gift ranks/caps; confirm one full family, then "
                     "select one gift per family and use selfbrid wolf gain "
                     "<family> <gift-key> to buy a rank. The other two families "
                     "cap at3. Not all active Werewolf commands are hybrid-ready.\n\r",ch);
    if (hybrid_has_parent(h, CLASS_MONK))
    {
        send_to_char("Existing secondary Monks: selfbrid monk shows paths; "
                     "selfbrid monk focus mantra confirm chooses a permanent full "
                     "path; selfbrid monk mantra gain buys one rank for "
                     "10 x next rank primal. Chosen cap8, others4. Native "
                     "monklearn is unchanged; conversion/forms remain disabled.\n\r", ch);
        send_to_char("Learned secondary Mantra2 enables readaura (Mantra2; free), "
                     "including carried-object identification. The Monk grant "
                     "does not require the original class's aura-reading power. "
                     "It also enables scry (Mantra2; free): survey players or "
                     "scry a target while retaining native shields, staff privacy "
                     "and guardian warnings. Mantra2 also enables shield/mindblock "
                     "(free, one shared self-only aura shield; no stacking). "
                     "Using either command again dismisses it even after training loss.\n\r", ch);
        send_to_char("Mantra1 enables heighten (Heightened Senses / Eyes of God): "
                     "free, self-only, six-pulse activation delay. Dismissal is free "
                     "even after training loss; independent Sorcerer vision remains. "
                     "Conversion and live hybrid forms are still disabled.\n\r", ch);
        send_to_char("Existing records: secondary Monk Stun Fist needs Tech 7 on the full Tech path; "
                     "other paths cap Tech at4. Costs5000 mana, with1-second interruption, "
                     "2-second caster recovery and shared6-second target immunity. "
                     "Original Monks keep native access; no free ranks or conversion.\n\r", ch);
        send_to_char("Secondary Monk Tech: repeat (1), aurashot (2), blitz (3); "
                     "cripple (5) and darkblaze (6) need the full Tech path. "
                     "Native costs and timing apply. Forms remain native-only.\n\r", ch);
        send_to_char("Secondary Monk Mantra: almighty (5; 5000 mana single / "
                     "20000 group), lifecloak (8; full HP/mana, spends all mana). "
                     "Both need the full Mantra path; other paths cap Mantra at4. "
                     "Cloak rescues once. Favor dawn cleanup covers loaded characters. "
                     "Favor deadlines are also checked on reload; legacy Favor needs recasting.\n\r", ch);
        send_to_char("Secondary Monk travel: celestial (7; 1500 mana) needs full "
                     "Mantra and learned rank. Native destination restrictions and "
                     "player summon consent remain. Conversion/forms are not enabled.\n\r", ch);
        send_to_char("Secondary Monk spiritguardian (6; no mana cost) needs full "
                     "Mantra and learned rank. One shared warning toggle, not a pet; "
                     "native Vampire/Werewolf access does not stack. Active guardians "
                     "can be dismissed after training loss, and expire on the next "
                     "pulse or load when no supported grant remains. Regaining "
                     "training does not reactivate them.\n\r", ch);
    }
    long pages = hybrid_book_chain_policy(h);
    if (pages) {
        snprintf(line, sizeof(line), "Approved book-chain policy: %ld pages in either "
                 "parent order. Enforced for valid hybrid records; "
                 "Learned secondary colors affect book damage/affect pages; "
                 "training/crafting commands are unchanged.\n\r", pages);
        send_to_char(line, ch);
    }
    snprintf(line, sizeof(line), "Related help: %s, %s, HYBRID, SELFBRID.\n\r",
             hybrid_parent_name(h->parents[0]), hybrid_parent_name(h->parents[1]));
    send_to_char(line, ch);
    return 1;
}
