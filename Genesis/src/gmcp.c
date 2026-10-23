/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.
 *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael
 *  Chastain, Michael Quan, and Mitchell Tse.
 *
 *  In order to use any part of this Merc Diku Mud, you must comply with
 *  both the original Diku license in 'license.doc' as well the Merc
 *  license in 'license.txt'. In particular, you may not remove either of
 *  these copyright notices.
 *
 *  Much time and thought has gone into this software and you are
 *  benefitting. We hope that you share your changes too. What goes
 *  around, comes around.
 ***************************************************************************/
/***************************************************************************
 *  God Wars Mud improvements copyright (C) 1994, 1995, 1996 by Richard
 *  Woolcock. This mud is NOT to be copied in whole or in part, or to be
 *  run without the permission of Richard Woolcock. Nobody else has
 *  permission to authorise the use of this code.
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "telnet_input.h"
#include "gmcp.h"

_Static_assert(sizeof(((DESCRIPTOR_DATA *)0)->gmcp_cache) == GMCP_CACHE_SIZE,
               "GMCP descriptor cache size mismatch");

bool gmcp_offer(DESCRIPTOR_DATA *d)
{
    static const unsigned char offer[] = {255, 251, GMCP_OPTION};
    if (d == NULL || d->gmcp_state > GMCP_DISABLED) return FALSE;
    if (d->gmcp_state != GMCP_NEW) return TRUE;
    if (!telnet_write(d, offer, sizeof(offer))) return FALSE;
    d->gmcp_state = GMCP_OFFERED;
    return TRUE;
}

void gmcp_negotiate(DESCRIPTOR_DATA *d, bool enable)
{
    /* DO is an acknowledgement of our WILL, not another offer. Repeated
     * acknowledgements never elicit a reply or force another snapshot.
     * Honor unsolicited DO/DONT too, as the GMCP specification requires. */
    if (d == NULL) return;
    unsigned char next = enable ? GMCP_ACTIVE : GMCP_DISABLED;
    if (d->gmcp_state != next) d->gmcp_cache[0] = '\0';
    d->gmcp_state = next;
}

bool gmcp_recover(DESCRIPTOR_DATA *d)
{
    /* Reset the inherited client before re-offering. No GMCP state is stored
     * in the v2 handoff; even an older executable's session starts cleanly.
     * Failed exec never reaches this path and leaves negotiation unchanged. */
    static const unsigned char reset[] = {255, 252, GMCP_OPTION,
                                         255, 251, GMCP_OPTION};
    if (d == NULL || !telnet_write(d, reset, sizeof(reset))) return FALSE;
    d->gmcp_state = GMCP_OFFERED;
    d->gmcp_cache[0] = '\0';
    return TRUE;
}

static const char *class_name(long class)
{
    switch (class) {
    case CLASS_NONE: return "none";
    case CLASS_VAMPIRE: return "vampire";
    case CLASS_WEREWOLF: return "werewolf";
    case CLASS_SORCERER: return "sorcerer";
    case CLASS_HIGHLANDER: return "highlander";
    case CLASS_DROW: return "drow";
    case CLASS_NINJA: return "ninja";
    case CLASS_DRAGON: return "dragon";
    case CLASS_MAGE: return "mage";
    case CLASS_MONK: return "monk";
    case CLASS_ELEM: return "elemental";
    default: return "unknown";
    }
}

/* Normal player names are ASCII letters, at most 12 bytes. Bound legacy or
 * damaged names to 64 bytes and escape every non-ASCII/control byte as JSON,
 * including IAC. No raw player text may become JSON or Telnet framing. */
static void identity_name(char out[385], const char *name)
{
    static const char hex[] = "0123456789abcdef";
    size_t used = 0;
    for (size_t i = 0; name != NULL && i < 64 && name[i] != '\0'; ++i) {
        unsigned char c = (unsigned char)name[i];
        if (c < 32 || c >= 127) {
            out[used++] = '\\'; out[used++] = 'u';
            out[used++] = '0'; out[used++] = '0';
            out[used++] = hex[c >> 4]; out[used++] = hex[c & 15];
        } else {
            if (c == '"' || c == '\\') out[used++] = '\\';
            out[used++] = (char)c;
        }
    }
    out[used] = '\0';
}

/* Identity and vitals share the authenticated-self privacy gate and cache.
 * Class is the existing primary class, never inferred hybrid ancestry. */
bool gmcp_update(DESCRIPTOR_DATA *d)
{
    char frame[GMCP_CACHE_SIZE];
    char name[385];
    char blood[96] = "null", ki[96] = "null", quint[96] = "null";
    char paradox[96] = "null", points[96] = "null", will[96] = "null";
    CHAR_DATA *ch;
    long hp = 0, maxhp = 0, mp = 0, maxmp = 0;
    if (d == NULL || d->gmcp_state > GMCP_DISABLED) return FALSE;
    if (d->gmcp_state != GMCP_ACTIVE) return TRUE;
    if (!descriptor_is_in_game(d)) {
        d->gmcp_cache[0] = '\0';
        return TRUE; /* No authentication-shell telemetry. */
    }
    ch = d->character;
    if (d->original != NULL || ch == NULL || IS_NPC(ch) ||
        ch->pcdata == NULL || ch->desc != d ||
        IS_MORE(ch, MORE_HEAD) || IS_EXTRA(ch, EXTRA_OSWITCH)) ch = NULL;
    if (ch == NULL && d->gmcp_cache[0] == '\0') return TRUE;
    if (ch != NULL) {
        hp = ch->hit; maxhp = ch->max_hit;
        mp = ch->mana; maxmp = ch->max_mana;
        if (IS_VAMPIRE(ch))
            snprintf(blood, sizeof(blood), "{\"current\":%ld,\"max\":%ld}",
                     ch->blood[BLOOD_CURRENT], ch->blood[BLOOD_POOL]);
        if (IS_NINJA(ch))
            snprintf(ki, sizeof(ki), "{\"current\":%ld,\"max\":300}",
                     ch->pcdata->ninja[NINJA_KI]);
        if (IS_MAGE(ch)) {
            snprintf(quint, sizeof(quint), "{\"current\":%ld,\"max\":null}",
                     ch->quint[BODY]);
            snprintf(paradox, sizeof(paradox), "{\"current\":%ld,\"max\":null}",
                     ch->paradox);
        }
        if (IS_DROW(ch) || IS_DRAGON(ch))
            snprintf(points, sizeof(points), "{\"current\":%ld,\"max\":null}",
                     ch->pcdata->classpoints[CP_CURRENT]);
        if (IS_WEREWOLF(ch))
            snprintf(will, sizeof(will), "{\"current\":%ld,\"max\":%ld}",
                     ch->pcdata->willpower[0], ch->pcdata->willpower[1]);
    }
    identity_name(name, ch ? ch->name : NULL);
    int count = snprintf(frame, sizeof(frame),
        "%c%c%cChar.Base {\"name\":\"%s\",\"level\":%ld,\"class\":\"%s\"}%c%c"
        "%c%c%cChar.Vitals {\"hp\":%ld,\"maxhp\":%ld,\"mp\":%ld,\"maxmp\":%ld,"
        "\"mana\":%ld,\"maxmana\":%ld}%c%c"
        "%c%c%cAnathema.Vitals {\"available\":%s,\"class\":\"%s\","
        "\"blood\":%s,\"ki\":%s,\"quintessence\":%s,\"paradox\":%s,"
        "\"class_points\":%s,\"willpower\":%s}%c%c",
        255,250,GMCP_OPTION,name,ch ? ch->level : 0L,
        ch ? class_name(ch->class) : "none",255,240,
        255,250,GMCP_OPTION,hp,maxhp,mp,maxmp,mp,maxmp,255,240,
        255,250,GMCP_OPTION,ch ? "true" : "false",
        ch ? class_name(ch->class) : "none",blood,ki,quint,paradox,points,will,255,240);
    if (count < 0 || (size_t)count >= sizeof(frame)) return FALSE;
    if (!strcmp(frame, d->gmcp_cache)) return TRUE;
    if (!telnet_write(d, (const unsigned char *)frame, (size_t)count)) return FALSE;
    memcpy(d->gmcp_cache, frame, (size_t)count + 1);
    return TRUE;
}
