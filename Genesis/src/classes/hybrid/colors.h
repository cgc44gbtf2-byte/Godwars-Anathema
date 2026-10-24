#ifndef ANATHEMA_HYBRID_COLORS_H
#define ANATHEMA_HYBRID_COLORS_H
struct char_data;
/* Exact selfbrid colors subcommand only. Returns zero for preview/info routes.
 * Never attaches/converts a character or delegates to a native trainer. */
int hybrid_colors_command(struct char_data *ch, const char *argument);
#endif
