#ifndef ANATHEMA_WEREWOLF_ADVANCEMENT_H
#define ANATHEMA_WEREWOLF_ADVANCEMENT_H
#include <stddef.h>

/* One pricing policy for original and secondary Werewolves. Kills are a
 * cumulative prerequisite, not a currency. Rank 4 remains the solo cap. */
static inline int werewolf_advancement_price(long rank, int hybrid,
    long *primal, long *experience, long *kills)
{
    long multiplier;
    if (rank < 1 || rank >= 4 || (hybrid != 0 && hybrid != 1) ||
        primal == NULL || experience == NULL || kills == NULL)
        return 0;
    multiplier = hybrid ? 2 : 1;
    *primal = rank * 100 * multiplier;
    *experience = rank * 1000000 * multiplier;
    *kills = rank * 1000 * multiplier;
    return 1;
}

#endif
