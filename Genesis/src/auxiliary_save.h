#ifndef ANATHEMA_AUXILIARY_SAVE_H
#define ANATHEMA_AUXILIARY_SAVE_H

/* Called on the game pulse; these are no-ops unless dirty and retry is due. */
void save_notes(void);
void retry_leaderboard_save(void);

/* Include merc.h first. One final pass ignores periodic retry backoff.
 * FALSE includes a visible replacement whose directory sync was uncertain.
 * These functions run only in normal execution, never signal handlers. */
bool flush_notes(void);
bool flush_leaderboard(void);
bool auxiliary_save_checkpoint(void);

#endif
