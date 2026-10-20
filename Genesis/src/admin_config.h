#ifndef ANATHEMA_ADMIN_CONFIG_H
#define ANATHEMA_ADMIN_CONFIG_H
/* Include merc.h first. Rejected reloads retain the previous live policy. */
bool admin_disabled_ready(void);
bool admin_bans_ready(void);
bool admin_configuration_healthy(void);
bool admin_ban_can_add(const char *name);
bool admin_disabled_can_add(const struct cmd_type *command,const char *by,long level);
#endif
