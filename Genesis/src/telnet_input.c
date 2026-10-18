/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
/***************************************************************************
 *  God Wars Mud improvements copyright (C) 1994, 1995, 1996 by Richard    *
 *  Woolcock.  This mud is NOT to be copied in whole or in part, or to be  *
 *  run without the permission of Richard Woolcock.  Nobody else has       *
 *  permission to authorise the use of this code.                          *
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "telnet_input.h"

enum { TN_SE = 240, TN_SB = 250, TN_WILL, TN_WONT, TN_DO, TN_DONT, TN_IAC };

bool telnet_state_valid(unsigned int state, unsigned int verb)
{
    if (state > TELNET_SUBNEGOTIATION_COMMAND) return FALSE;
    if (state == TELNET_OPTION) return verb >= TN_WILL && verb <= TN_DONT;
    return verb == 0;
}

/* Binary control data must bypass color expansion and command/prompt spacing.
 * Buffer it normally; process_output owns nonblocking writes and closure. */
static bool reply(DESCRIPTOR_DATA *d, unsigned char verb, unsigned char option)
{
    if (d->outbuf == NULL || d->outsize < 4 || d->outsize > 128000 ||
        d->outtop < 0 || d->outtop >= d->outsize) return FALSE;
    while (d->outtop + 3 >= d->outsize) {
        long size;
        char *buffer;
        if (d->outsize == 128000) return FALSE;
        size = d->outsize > 64000 ? 128000 : 2 * d->outsize;
        buffer = alloc_mem(size);
        memcpy(buffer, d->outbuf, (size_t)d->outtop);
        free_mem(d->outbuf, d->outsize);
        d->outbuf = buffer;
        d->outsize = size;
    }
    d->outbuf[d->outtop++] = (char)TN_IAC;
    d->outbuf[d->outtop++] = (char)verb;
    d->outbuf[d->outtop++] = (char)option;
    d->outbuf[d->outtop] = '\0';
    return TRUE;
}

/* Incremental RFC 854 framing, including binary/NUL-containing subnegotiation.
 * All unsupported options are declined; GMCP/MSDP/MXP are not implemented.
 * The socket reader separately bounds total raw bytes processed per tick. */
bool telnet_input(DESCRIPTOR_DATA *d, const unsigned char *bytes, size_t count)
{
    size_t used = 0;
    if (d == NULL || (bytes == NULL && count != 0) ||
        !telnet_state_valid(d->telnet_state, d->telnet_verb)) return FALSE;
    while (used < sizeof(d->inbuf) && d->inbuf[used] != '\0') ++used;
    if (used >= sizeof(d->inbuf) - 10) return FALSE;

    for (size_t i = 0; i < count; ++i) {
        unsigned char c = bytes[i];
        switch (d->telnet_state) {
        case TELNET_DATA:
            if (c == TN_IAC) { d->telnet_state = TELNET_COMMAND; continue; }
            if (c == 0) continue; /* NVT CR NUL; never truncate a C buffer. */
            break;
        case TELNET_COMMAND:
            d->telnet_state = TELNET_DATA;
            if (c >= TN_WILL && c <= TN_DONT) {
                d->telnet_state = TELNET_OPTION;
                d->telnet_verb = c;
                continue;
            }
            if (c == TN_SB) d->telnet_state = TELNET_SUBNEGOTIATION;
            if (c != TN_IAC) continue; /* Includes GA, NOP and bare SE. */
            break; /* Escaped IAC data; legacy ASCII canonicalization applies. */
        case TELNET_OPTION: {
            unsigned char verb = d->telnet_verb;
            d->telnet_state = TELNET_DATA;
            d->telnet_verb = 0;
            /* DO/DONT ECHO acknowledge existing server WILL/WONT ECHO.
             * Do not answer DO ECHO with WONT and re-enable password echo. */
            if (verb == TN_WILL && !reply(d, TN_DONT, c)) return FALSE;
            if (verb == TN_DO && c != 1 && !reply(d, TN_WONT, c)) return FALSE;
            continue; /* Negative acknowledgements never start reply loops. */
        }
        case TELNET_SUBNEGOTIATION:
            if (c == TN_IAC) d->telnet_state = TELNET_SUBNEGOTIATION_COMMAND;
            continue;
        case TELNET_SUBNEGOTIATION_COMMAND:
            d->telnet_state = c == TN_SE ? TELNET_DATA : TELNET_SUBNEGOTIATION;
            continue;
        default:
            return FALSE;
        }
        if (used >= sizeof(d->inbuf) - 10) return FALSE;
        d->inbuf[used++] = (char)c;
        d->inbuf[used] = '\0';
    }
    return TRUE;
}
