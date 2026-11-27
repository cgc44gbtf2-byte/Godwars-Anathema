/* Terminal/restart checkpoints for already accepted auxiliary updates.
 * This deliberately does not post editor drafts or repair rejected files. */
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "auxiliary_save.h"

bool auxiliary_save_checkpoint(void)
{
    /* Do not short-circuit: one failing destination must not suppress another
     * category's last opportunity to commit its latest accepted data. */
    bool notes=flush_notes();
    bool leaderboard=flush_leaderboard();
    if(!notes)log_string("WARNING: pending board saves were not confirmed durable.");
    if(!leaderboard)log_string("WARNING: pending leaderboard save was not confirmed durable.");
    return notes&&leaderboard;
}
