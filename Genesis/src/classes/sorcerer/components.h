#ifndef SORCERER_COMPONENTS_H
#define SORCERER_COMPONENTS_H

/* requires merc.h to be included first */

#define SORC_RUNE   0
#define SORC_GLYPH  1
#define SORC_SIGIL  2

#define SORC_RUNES  11
#define SORC_GLYPHS 10
#define SORC_SIGILS 4
#define SORC_RUNE_MASTER 1024

#define APP_NONE      0
#define APP_APPRENTICE 1
#define APP_GRADUATED  2

typedef struct sorc_component
{
    const char *name;
    long bit;
} SORC_COMPONENT;

typedef enum
{
    SORC_LEARN_OK,
    SORC_LEARN_NOT_SORCERER,
    SORC_LEARN_BAD_NAME,
    SORC_LEARN_KNOWN,
    SORC_LEARN_PREREQ,
    SORC_LEARN_LIMIT,
    SORC_LEARN_COST
} SORC_LEARN_RESULT;

const SORC_COMPONENT *sorc_component_table(int kind, int *count);
const SORC_COMPONENT *sorc_component_find(int kind, const char *name);
bool sorc_can_use_components(const CHAR_DATA *ch);
int sorc_component_count(const CHAR_DATA *ch, int kind);
bool sorc_knows(const CHAR_DATA *ch, int kind, long bit);
long sorc_component_cost(const CHAR_DATA *ch, int kind);
SORC_LEARN_RESULT sorc_learn(CHAR_DATA *ch, int kind, const char *name);
bool sorc_is_apprentice(const CHAR_DATA *ch);

#endif
