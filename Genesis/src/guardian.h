/* Eligibility and persistence of the shared native warning toggle. */
#ifndef ANATHEMA_GUARDIAN_H
#define ANATHEMA_GUARDIAN_H
bool guardian_eligible(CHAR_DATA *ch);
long guardian_saved_more(CHAR_DATA *ch, long more);
void guardian_reconcile(CHAR_DATA *ch);
#endif
