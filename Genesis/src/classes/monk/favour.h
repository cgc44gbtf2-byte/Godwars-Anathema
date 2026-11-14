#ifndef ANATHEMA_MONK_FAVOUR_H
#define ANATHEMA_MONK_FAVOUR_H
/* Game-calendar hours, not wall-clock seconds or a new class power. */
bool monk_favour_clock_valid(const TIME_INFO_DATA *clock);
bool monk_favour_grant(CHAR_DATA *ch, const TIME_INFO_DATA *clock);
bool monk_favour_active(const CHAR_DATA *ch, const TIME_INFO_DATA *clock);
void monk_favour_clear(CHAR_DATA *ch);
long monk_favour_saved_more(const CHAR_DATA *ch, const TIME_INFO_DATA *clock);
void monk_favour_write(FILE *fp, const CHAR_DATA *ch, const TIME_INFO_DATA *clock);
void monk_favour_read(CHAR_DATA *ch, FILE *fp);
void monk_favour_reconcile(CHAR_DATA *ch, const TIME_INFO_DATA *clock);
#endif
