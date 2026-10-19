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
        if (!hybrid_display_name(h, display, sizeof(display))) return;
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
                     "Preview only: no conversion, payment or free training.\n\r", ch);
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
                     "Numeric checks exist; power mastery and safe conversion remain "
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
        send_to_char("Existing secondary Sorcerers can sorccreate book/page/pen: "
                     "book or pen: 10 primal; page: free. With a held pen, write "
                     "titles and spell-page lines, then the native end.<color>. "
                     "Writing grants no color ranks. Enchanting/vision remain "
                     "unavailable through this adapter.\n\r",ch);
        return 1;
    }
    const HYBRID_DEFINITION *h = hybrid_by_name(argument);
    if (!h) return 0;
    if (!hybrid_display_name(h, display, sizeof(display))) return 0;
    snprintf(line, sizeof(line), "%s - planned hybrid [%ld]\n\r", display, h->id);
    send_to_char(line, ch);
    snprintf(line, sizeof(line), "Parents: %s%s#n + %s%s#n. Either can be Parent 1.\n\r",
        hybrid_parent_color(h->parents[0]), hybrid_parent_name(h->parents[0]),
        hybrid_parent_color(h->parents[1]), hybrid_parent_name(h->parents[1]));
    send_to_char(line, ch);
    send_to_char("Original training is retained; the second parent starts untrained. "
                 "There is no playable conversion yet.\n\r", ch);
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
