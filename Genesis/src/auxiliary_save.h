#ifndef ANATHEMA_AUXILIARY_SAVE_H
#define ANATHEMA_AUXILIARY_SAVE_H

/* Called on the game pulse; these are no-ops unless dirty and retry is due. */
void save_notes(void);
void retry_leaderboard_save(void);

#endif
