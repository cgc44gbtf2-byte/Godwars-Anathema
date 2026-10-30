/* Include merc.h first. Checked metrics never overwrite output on failure. */
#ifndef ANATHEMA_PROGRESS_METRICS_H
#define ANATHEMA_PROGRESS_METRICS_H
bool get_hours_checked(CHAR_DATA *ch, long *hours);
bool get_ratio_checked(CHAR_DATA *ch, long *ratio);
/* True only when a valid age newly enables the existing PK flag. */
bool grant_age_pk(CHAR_DATA *ch);
#endif
