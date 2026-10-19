/* Read-only first stage. No class assignment, currency charge, staff command
 * dispatch or save mutation is permitted until parent-state migration lands. */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "hybrid.h"

void do_selfbrid(CHAR_DATA *ch, char *argument)
{
    const HYBRID_DEFINITION *hybrid;
    const char *name;
    int list = 0;
    char line[256];
    if (!ch || IS_NPC(ch) || !ch->pcdata) return;
    if (!argument) argument = "";
    while (isspace((unsigned char)*argument)) ++argument;
    if (!str_prefix("list", argument)) {
        name = argument + 4;
        while (isspace((unsigned char)*name)) ++name;
        list = !*name;
    }
    send_to_char("Hybrid preview only: conversion is not enabled.\n\r", ch);
    if (!*argument || list) {
        send_to_char(" ID  Hybrid                Parents\n\r", ch);
        for (size_t i = 0; i < hybrid_count(); ++i) {
            hybrid = hybrid_at(i);
            snprintf(line, sizeof(line), "%3ld  %-21s %s + %s\n\r",
                     hybrid->id, hybrid->name,
                     hybrid_parent_name(hybrid->parents[0]),
                     hybrid_parent_name(hybrid->parents[1]));
            send_to_char(line, ch);
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
    if (!hybrid) {
        send_to_char("Unknown hybrid. Use selfbrid list or "
                     "selfbrid info <full hybrid name>.\n\r", ch);
        return;
    }
    snprintf(line, sizeof(line), "[%ld] %s: %s + %s\n\r",
             hybrid->id, hybrid->name,
             hybrid_parent_name(hybrid->parents[0]),
             hybrid_parent_name(hybrid->parents[1]));
    send_to_char(line, ch);
    send_to_char(hybrid_has_parent(hybrid, ch->class)
        ? "Parent requirement: your current class matches one parent.\n\r"
        : "Parent requirement: your current class does not match either parent.\n\r",
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
                 "Other conversion prerequisites are not configured.\n\r"
                 "No equipment, training, currency or character data was changed.\n\r",
                 ch);
}
