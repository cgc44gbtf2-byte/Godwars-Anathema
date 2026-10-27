#ifndef ANATHEMA_CLASS_FACTIONS_H
#define ANATHEMA_CLASS_FACTIONS_H

struct char_data;

/* Managed faction/clan roles: 0 none/unavailable, 1 member, 2 officer, 3 leader.
 * Name is borrowed registry storage, not an online-character pointer; do not
 * free it or retain it across registry updates. NULL means no managed leader.
 */
int clan_member_role(const struct char_data *ch);
const char *clan_leader_name(const struct char_data *ch);
/* Display status, not an authorization check. Registry errors are reported as
 * Unavailable rather than fabricating Loner/Exiled membership history. */
const char *clan_affiliation(const struct char_data *ch);

#endif
