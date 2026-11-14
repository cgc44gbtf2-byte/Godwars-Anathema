/* Godwars: Anathema owner bootstrap. Original Diku/Merc/God Wars notices
 * remain in the engine. This deliberately implements the operator-requested
 * NAME-BASED policy: control of the Luto account is sufficient authority.
 * It is not proof of real-world server ownership or a secret-based gate. */
#include <stdio.h>
#include <string.h>
#include "merc.h"

void do_authorize(CHAR_DATA *ch, char *argument)
{
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->name == NULL)
        return;
    DESCRIPTOR_DATA *d = ch->desc;
    if (str_cmp(ch->name, "Luto") || d == NULL || d->character != ch ||
        d->original != NULL || d->connected != CON_PLAYING || !d->fcommand ||
        ch->in_room == NULL || ch->pcdata->pwd == NULL ||
        ch->pcdata->pwd[0] == '\0' || IS_MORE(ch, MORE_HEAD) ||
        IS_EXTRA(ch, EXTRA_OSWITCH) || IS_AFFECTED(ch, AFF_CHARM)) {
        send_to_char("Authorization is only available to Luto's own active session.\n\r", ch);
        return;
    }
    if (argument == NULL || argument[0] != '\0') {
        send_to_char("Syntax: authorize (no arguments).\n\r", ch);
        return;
    }
    if (ch->level == LEV_ST) {
        send_to_char("You already have the highest staff level.\n\r", ch);
        return;
    }
    long old_level = ch->level;
    ch->level = LEV_ST;
    if (!(player_save_state(ch, PLAYER_SAVE_PRIMARY) & PLAYER_SAVE_PRIMARY)) {
        ch->level = old_level;
        send_to_char("Authorization save was not confirmed. Runtime rank unchanged; resolve storage errors before retrying.\n\r", ch);
        log_string("Owner authorization primary save was not confirmed.");
        return;
    }
    bool backup = (player_save_state(ch, PLAYER_SAVE_BACKUP) & PLAYER_SAVE_BACKUP) != 0;
    send_to_char(backup ?
        "Luto authorized at the highest staff level (7). HP, mana and class stats unchanged.\n\r" :
        "Luto authorized at the highest staff level (7); primary saved, backup retry pending.\n\r", ch);
    log_string("Luto used name-based owner authorization.");
}
