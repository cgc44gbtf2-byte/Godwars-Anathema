/* Original Anathema registry. This does not activate multi-class permissions. */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../class_ids.h"
#include "hybrid.h"

static const HYBRID_DEFINITION roster[] = {
    { 1, "Umbral Drake",       {CLASS_DRAGON, CLASS_DROW}},
    { 2, "Primal Tempest",     {CLASS_DRAGON, CLASS_ELEM}},
    { 3, "Wyrmblade",          {CLASS_DRAGON, CLASS_HIGHLANDER}},
    { 4, "Astral Dragon",      {CLASS_DRAGON, CLASS_MAGE}},
    { 5, "Draconic Ascetic",   {CLASS_DRAGON, CLASS_MONK}},
    { 6, "Phantom Wyrm",       {CLASS_DRAGON, CLASS_NINJA}},
    { 7, "Runebound Dragon",   {CLASS_DRAGON, CLASS_SORCERER}},
    { 8, "Bloodwyrm",          {CLASS_DRAGON, CLASS_VAMPIRE}},
    { 9, "Dire Wyrm",          {CLASS_DRAGON, CLASS_WEREWOLF}},
    {10, "Venomweaver",        {CLASS_DROW, CLASS_ELEM}},
    {11, "Gloomblade",         {CLASS_DROW, CLASS_HIGHLANDER}},
    {12, "Shadow Arcanist",    {CLASS_DROW, CLASS_MAGE}},
    {13, "Spider Sage",        {CLASS_DROW, CLASS_MONK}},
    {14, "Nightfang",          {CLASS_DROW, CLASS_NINJA}},
    {15, "Web Hexer",          {CLASS_DROW, CLASS_SORCERER}},
    {16, "Bloodshade",         {CLASS_DROW, CLASS_VAMPIRE}},
    {17, "Moonstalker",        {CLASS_DROW, CLASS_WEREWOLF}},
    {18, "Stormforged",        {CLASS_ELEM, CLASS_HIGHLANDER}},
    {19, "Primal Arcanist",    {CLASS_ELEM, CLASS_MAGE}},
    {20, "Elemental Fist",     {CLASS_ELEM, CLASS_MONK}},
    {21, "Gale Shadow",        {CLASS_ELEM, CLASS_NINJA}},
    {22, "Chaos Conduit",     {CLASS_ELEM, CLASS_SORCERER}},
    {23, "Crimson Tempest",    {CLASS_ELEM, CLASS_VAMPIRE}},
    {24, "Stormfang",          {CLASS_ELEM, CLASS_WEREWOLF}},
    {25, "Spellblade",         {CLASS_HIGHLANDER, CLASS_MAGE}},
    {26, "Iron Disciple",      {CLASS_HIGHLANDER, CLASS_MONK}},
    {27, "Silent Blade",       {CLASS_HIGHLANDER, CLASS_NINJA}},
    {28, "Runebreaker",        {CLASS_HIGHLANDER, CLASS_SORCERER}},
    {29, "Crimson Duelist",    {CLASS_HIGHLANDER, CLASS_VAMPIRE}},
    {30, "Fangblade",          {CLASS_HIGHLANDER, CLASS_WEREWOLF}},
    {31, "Astral Adept",       {CLASS_MAGE, CLASS_MONK}},
    {32, "Riftstalker",        {CLASS_MAGE, CLASS_NINJA}},
    {33, "Grand Thaumaturge",  {CLASS_MAGE, CLASS_SORCERER}},
    {34, "Night Arcanist",     {CLASS_MAGE, CLASS_VAMPIRE}},
    {35, "Lunar Invoker",      {CLASS_MAGE, CLASS_WEREWOLF}},
    {36, "Shadow Disciple",    {CLASS_MONK, CLASS_NINJA}},
    {37, "Mystic Channeler",   {CLASS_MONK, CLASS_SORCERER}},
    {38, "Blood Ascetic",      {CLASS_MONK, CLASS_VAMPIRE}},
    {39, "Spirit Claw",        {CLASS_MONK, CLASS_WEREWOLF}},
    {40, "Spellshade",         {CLASS_NINJA, CLASS_SORCERER}},
    {41, "Night Reaver",       {CLASS_NINJA, CLASS_VAMPIRE}},
    {42, "Ghostfang",          {CLASS_NINJA, CLASS_WEREWOLF}},
    {43, "Blood Thaumaturge",  {CLASS_SORCERER, CLASS_VAMPIRE}},
    {44, "Hexfang",            {CLASS_SORCERER, CLASS_WEREWOLF}},
    {45, "Bloodmoon Revenant", {CLASS_VAMPIRE, CLASS_WEREWOLF}}
};

size_t hybrid_count(void)
{
    return sizeof(roster) / sizeof(roster[0]);
}

const HYBRID_DEFINITION *hybrid_at(size_t index)
{
    return index < hybrid_count() ? &roster[index] : NULL;
}

const HYBRID_DEFINITION *hybrid_by_id(long id)
{
    /* Never narrow a player-controlled long before checking its bounds. */
    if (id < 1 || id > (long)hybrid_count()) return NULL;
    return &roster[id - 1];
}

const char *hybrid_parent_name(long parent)
{
    switch (parent) {
    case CLASS_DRAGON: return "Dragon";
    case CLASS_DROW: return "Drow";
    case CLASS_ELEM: return "Elementalist";
    case CLASS_HIGHLANDER: return "Highlander";
    case CLASS_MAGE: return "Mage";
    case CLASS_MONK: return "Monk";
    case CLASS_NINJA: return "Ninja";
    case CLASS_SORCERER: return "Sorcerer";
    case CLASS_VAMPIRE: return "Vampire";
    case CLASS_WEREWOLF: return "Werewolf";
    default: return NULL;
    }
}

const char *hybrid_parent_color(long parent)
{
    switch (parent) {
    case CLASS_DRAGON: return "#n";
    case CLASS_DROW: return "#P";
    case CLASS_ELEM: return "#c";
    case CLASS_HIGHLANDER: return "#C";
    case CLASS_MAGE: return "#p";
    case CLASS_MONK: return "#7";
    case CLASS_NINJA: return "#0";
    case CLASS_SORCERER: return "#L";
    case CLASS_VAMPIRE: return "#r";
    case CLASS_WEREWOLF: return "#o";
    default: return NULL;
    }
}

/* Fixed, distinct letter motifs; 0 = original parent, 1 = second parent.
 * Spaces retain the preceding color and do not advance the motif. */
static const char *const color_patterns[] = {
    "00000001", /* 1: Umbral Drake */
    "01000001", /* 2: Primal Tempest */
    "00100001", /* 3: Wyrmblade */
    "01100001", /* 4: Astral Dragon */
    "00010001", /* 5: Draconic Ascetic */
    "01010001", /* 6: Phantom Wyrm */
    "00110001", /* 7: Runebound Dragon */
    "01110001", /* 8: Bloodwyrm */
    "00001001", /* 9: Dire Wyrm */
    "01001001", /* 10: Venomweaver */
    "00101001", /* 11: Gloomblade */
    "01101001", /* 12: Shadow Arcanist */
    "00011001", /* 13: Spider Sage */
    "01011001", /* 14: Nightfang */
    "00111001", /* 15: Web Hexer */
    "01111001", /* 16: Bloodshade */
    "00000101", /* 17: Moonstalker */
    "01000101", /* 18: Stormforged */
    "00100101", /* 19: Primal Arcanist */
    "01100101", /* 20: Elemental Fist */
    "00010101", /* 21: Gale Shadow */
    "01010101", /* 22: Chaos Conduit */
    "00110101", /* 23: Crimson Tempest */
    "01110101", /* 24: Stormfang */
    "00001101", /* 25: Spellblade */
    "01001101", /* 26: Iron Disciple */
    "00101101", /* 27: Silent Blade */
    "01101101", /* 28: Runebreaker */
    "00011101", /* 29: Crimson Duelist */
    "01011101", /* 30: Fangblade */
    "00111101", /* 31: Astral Adept */
    "01111101", /* 32: Riftstalker */
    "00000011", /* 33: Grand Thaumaturge */
    "01000011", /* 34: Night Arcanist */
    "00100011", /* 35: Lunar Invoker */
    "01100011", /* 36: Shadow Disciple */
    "00010011", /* 37: Mystic Channeler */
    "01010011", /* 38: Blood Ascetic */
    "00110011", /* 39: Spirit Claw */
    "01110011", /* 40: Spellshade */
    "00001011", /* 41: Night Reaver */
    "01001011", /* 42: Ghostfang */
    "00101011", /* 43: Blood Thaumaturge */
    "01101011", /* 44: Hexfang */
    "00011011", /* 45: Bloodmoon Revenant */
};
_Static_assert(sizeof(color_patterns)/sizeof(color_patterns[0]) ==
               sizeof(roster)/sizeof(roster[0]), "Every hybrid needs a color motif");

int hybrid_display_name_for_parent(const HYBRID_DEFINITION *hybrid, long parent,
                                   char *out, size_t size)
{
    if (!out || !size) return 0;
    out[0] = '\0';
    if (!hybrid || hybrid_by_id(hybrid->id) != hybrid ||
        !hybrid_has_parent(hybrid, parent)) return 0;
    size_t length = strlen(hybrid->name);
    if (length < 2 || length > (SIZE_MAX-5)/3) return 0;
    const char *pattern = color_patterns[hybrid->id-1];
    const char *colors[] = {hybrid_parent_color(parent),
        hybrid_parent_color(hybrid->parents[0] == parent ?
                            hybrid->parents[1] : hybrid->parents[0])};
    /* All HELP CLASS color codes are two bytes. Preflight before any write. */
    size_t letters = 0, needed = length+5;
    char previous = '0';
    for (size_t i = 0; i < length; i++) {
        if (!isalpha((unsigned char)hybrid->name[i])) continue;
        char color = pattern[letters++ % 8];
        if (color != previous) needed += 2;
        previous = color;
    }
    if (size < needed) return 0;
    size_t used = 2;
    memcpy(out, colors[0], 2);
    letters = 0;
    previous = '0';
    for (size_t i = 0; i < length; i++) {
        if (isalpha((unsigned char)hybrid->name[i])) {
            char color = pattern[letters++ % 8];
            if (color != previous) {
                memcpy(out+used, colors[color-'0'], 2);
                used += 2;
            }
            previous = color;
        }
        out[used++] = hybrid->name[i];
    }
    memcpy(out+used, "#n", 3);
    return 1;
}

int hybrid_display_name(const HYBRID_DEFINITION *hybrid, char *out, size_t size)
{
    return hybrid_display_name_for_parent(hybrid, hybrid ? hybrid->parents[0] : 0,
                                          out, size);
}

int hybrid_has_parent(const HYBRID_DEFINITION *hybrid, long parent)
{
    return hybrid && hybrid_parent_name(parent) &&
        (hybrid->parents[0] == parent || hybrid->parents[1] == parent);
}

long hybrid_book_chain_policy(const HYBRID_DEFINITION *hybrid)
{
    if (!hybrid || hybrid_by_id(hybrid->id) != hybrid ||
        !hybrid_has_parent(hybrid, CLASS_SORCERER)) return 0;
    if (hybrid_has_parent(hybrid, CLASS_MAGE)) return 6;
    if (hybrid_has_parent(hybrid, CLASS_DROW) ||
        hybrid_has_parent(hybrid, CLASS_ELEM) ||
        hybrid_has_parent(hybrid, CLASS_VAMPIRE)) return 4;
    return 3;
}

const HYBRID_DEFINITION *hybrid_by_parents(long first, long second)
{
    if (first == second || !hybrid_parent_name(first) ||
        !hybrid_parent_name(second)) return NULL;
    for (size_t i = 0; i < hybrid_count(); ++i)
        if (hybrid_has_parent(&roster[i], first) &&
            hybrid_has_parent(&roster[i], second)) return &roster[i];
    return NULL;
}

const HYBRID_DEFINITION *hybrid_by_name(const char *name)
{
    const char *end;
    size_t length;
    if (!name) return NULL;
    while (isspace((unsigned char)*name)) ++name;
    end = name + strlen(name);
    while (end > name && isspace((unsigned char)end[-1])) --end;
    if (end - name >= 2 && ((*name == '"' && end[-1] == '"') ||
                            (*name == '\'' && end[-1] == '\''))) {
        ++name;
        --end;
    }
    length = (size_t)(end - name);
    if (!length) return NULL;
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const char *candidate = roster[i].name;
        size_t j;
        if (strlen(candidate) != length) continue;
        for (j = 0; j < length; ++j)
            if (tolower((unsigned char)name[j]) !=
                tolower((unsigned char)candidate[j])) break;
        if (j == length) return &roster[i];
    }
    return NULL;
}
