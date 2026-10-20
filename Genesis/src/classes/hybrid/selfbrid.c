/* List/info remain read-only. An explicit colors subcommand can train only an
 * existing secondary record; class conversion/mastery transactions pending. */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "hybrid.h"
#include "readiness.h"
#include "colors.h"

void do_selfbrid(CHAR_DATA *ch, char *argument)
{
    const HYBRID_DEFINITION *hybrid;
    const char *name;
    int list = 0;
    char line[256];
    char display[128];
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    if (!argument) argument = "";
    while (isspace((unsigned char)*argument)) ++argument;
    if (hybrid_colors_command(ch, argument)) return;
    if (!str_prefix("list", argument)) {
        name = argument + 4;
        while (isspace((unsigned char)*name)) ++name;
        list = !*name;
    }
    send_to_char("Hybrid preview only: conversion is not enabled.\n\r", ch);
    if (!*argument || list) {
        size_t choices = 0;
        send_to_char(" ID  Hybrid                Parents\n\r", ch);
        for (size_t i = 0; i < hybrid_count(); ++i) {
            hybrid = hybrid_at(i);
            if (!hybrid_has_parent(hybrid, ch->class)) continue;
            choices++;
            if (!hybrid_display_name_for_parent(hybrid, ch->class, display, sizeof(display))) return;
            int padding = strlen(hybrid->name) < 21 ? 21 - (int)strlen(hybrid->name) : 0;
            snprintf(line, sizeof(line), "%3ld  %s%*s %s%s#n + %s%s#n\n\r",
                     hybrid->id, display, padding, "",
                     hybrid_parent_color(hybrid->parents[0]),
                     hybrid_parent_name(hybrid->parents[0]),
                     hybrid_parent_color(hybrid->parents[1]),
                     hybrid_parent_name(hybrid->parents[1]));
            send_to_char(line, ch);
        }
        if (!choices) {
            send_to_char("No hybrid choices: select a supported base class first.\n\r", ch);
            return;
        }
        send_to_char("Use: selfbrid info <full hybrid name>\n\r", ch);
        return;
    }
    name = argument;
    if (!str_prefix("info", argument) &&
        isspace((unsigned char)argument[4])) {
        name = argument + 4;
        while (isspace((unsigned char)*name)) ++name;
    }
    hybrid = hybrid_by_name(name);
    if (!hybrid || !hybrid_has_parent(hybrid, ch->class)) {
        send_to_char("Unknown hybrid or unavailable to your current base class. "
                     "Use selfbrid list.\n\r", ch);
        return;
    }
    if (!hybrid_display_name_for_parent(hybrid, ch->class, display, sizeof(display))) return;
    snprintf(line, sizeof(line), "[%ld] %s: %s%s#n + %s%s#n\n\r",
             hybrid->id, display,
             hybrid_parent_color(hybrid->parents[0]),
             hybrid_parent_name(hybrid->parents[0]),
             hybrid_parent_color(hybrid->parents[1]),
             hybrid_parent_name(hybrid->parents[1]));
    send_to_char(line, ch);
    send_to_char("Parent requirement: your current class matches one parent.\n\r",
        ch);
    snprintf(line, sizeof(line), "Planned conversion price: %ld quest points "
             "(nothing charged).\n\r",
             HYBRID_DEFAULT_QUEST_COST);
    send_to_char(line, ch);
    send_to_char(ch->pcdata->quest >= HYBRID_DEFAULT_QUEST_COST
        ? "Quest balance check: enough for the planned price.\n\r"
        : "Quest balance check: not enough for the planned price.\n\r", ch);
    send_to_char("Planned training: keep the original parent's earned training; "
                 "the second parent starts untrained.\n\r"
                 "Hybrids will not qualify for base-class Master/Elite rankings.\n\r"
                 "This is not a final eligibility check.\n\r"
                 "No equipment, training, currency or character data was changed.\n\r",
                 ch);
    hybrid_readiness_show(ch);
}
