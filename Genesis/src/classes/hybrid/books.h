#ifndef ANATHEMA_HYBRID_BOOKS_H
#define ANATHEMA_HYBRID_BOOKS_H
struct char_data;
/* Chain budget only, NOT casting permission or an effective color rank.
 * Identity-based even if secondary training/build is unconfigured.
 * Zero rejects an invalid player record; ordinary/native actors keep10. */
long hybrid_book_chain_limit(const struct char_data *ch);
#endif
