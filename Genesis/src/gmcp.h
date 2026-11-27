#ifndef ANATHEMA_GMCP_H
#define ANATHEMA_GMCP_H
/* Include merc.h first. Descriptor-local state, never player-save data. */
#define GMCP_OPTION 201
#define GMCP_CACHE_SIZE 2048
enum { GMCP_NEW, GMCP_OFFERED, GMCP_ACTIVE, GMCP_DISABLED };
bool gmcp_offer(DESCRIPTOR_DATA *d);
void gmcp_negotiate(DESCRIPTOR_DATA *d, bool enable);
bool gmcp_update(DESCRIPTOR_DATA *d);
bool gmcp_recover(DESCRIPTOR_DATA *d);
#endif
