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

int hybrid_display_name(const HYBRID_DEFINITION *hybrid, char *out, size_t size)
{
    if (!out || !size) return 0;
    out[0] = '\0';
    if (!hybrid || hybrid_by_id(hybrid->id) != hybrid) return 0;
    size_t length = strlen(hybrid->name);
    if (length < 2) return 0;
    uint32_t hash = UINT32_C(2166136261) ^ (uint32_t)hybrid->id;
    for (size_t i = 0; i < length; i++)
        hash = (hash ^ (unsigned char)hybrid->name[i]) * UINT32_C(16777619);
    size_t split = 1 + hash % (length - 1);
    const char *first = hybrid_parent_color(hybrid->parents[hash & 1]);
    const char *second = hybrid_parent_color(hybrid->parents[(hash & 1) ^ 1]);
    int n = snprintf(out, size, "%s%.*s%s%s#n", first, (int)split,
                     hybrid->name, second, hybrid->name + split);
    if (n < 0 || (size_t)n >= size) { out[0] = '\0'; return 0; }
    return 1;
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
