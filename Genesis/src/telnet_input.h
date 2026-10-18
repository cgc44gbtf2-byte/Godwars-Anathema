#ifndef ANATHEMA_TELNET_INPUT_H
#define ANATHEMA_TELNET_INPUT_H
#include <stddef.h>

/* Include merc.h first. These states are private transport state, not skills. */
enum {
    TELNET_DATA,
    TELNET_COMMAND,
    TELNET_OPTION,
    TELNET_SUBNEGOTIATION,
    TELNET_SUBNEGOTIATION_COMMAND
};

bool telnet_state_valid(unsigned int state, unsigned int verb);
bool telnet_input(DESCRIPTOR_DATA *d, const unsigned char *bytes, size_t count);
#endif
