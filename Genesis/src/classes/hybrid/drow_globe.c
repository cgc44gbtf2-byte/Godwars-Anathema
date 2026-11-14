/* Globe lifecycle mutations; effective accessors remain read-only. */
#include <sys/types.h>
#include <stdio.h>
#include "../../merc.h"
#include "access.h"

int hybrid_drow_globe_active(const CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata) return 0;
    if (IS_DROW(ch))
        return (ch->pcdata->drow[DROW_POWERS]&DROW_GLOBE)!=0;
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    return s && s->secondary_class==CLASS_DROW &&
        (s->secondary_progression.drow[DROW_POWERS]&DROW_GLOBE)!=0;
}

long hybrid_drow_points_current(const CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata) return 0;
    if (IS_DROW(ch)) return ch->pcdata->classpoints[CP_CURRENT];
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if (!s || s->secondary_class!=CLASS_DROW) return 0;
    return s->secondary_classpoints[CP_CURRENT];
}

static long *drow_globe_bits(CHAR_DATA *ch)
{
    if (!ch || IS_NPC(ch) || !ch->pcdata) return NULL;
    if (IS_DROW(ch)) return &ch->pcdata->drow[DROW_POWERS];
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if (!s || s->secondary_class!=CLASS_DROW) return NULL;
    return &ch->pcdata->hybrid.secondary_progression.drow[DROW_POWERS];
}

int hybrid_drow_globe_set(CHAR_DATA *ch, int active)
{
    long *bits=drow_globe_bits(ch);
    if (!bits) return 0;
    if (active) SET_BIT(*bits,DROW_GLOBE);
    else REMOVE_BIT(*bits,DROW_GLOBE);
    return 1;
}

int hybrid_drow_points_charge(CHAR_DATA *ch, long amount)
{
    if (amount<0 || !ch || IS_NPC(ch) || !ch->pcdata) return 0;
    if (IS_DROW(ch)) {
        if (ch->pcdata->classpoints[CP_CURRENT]<amount) return 0;
        ch->pcdata->classpoints[CP_CURRENT]-=amount;
        return 1;
    }
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if (!s || s->secondary_class!=CLASS_DROW ||
        s->secondary_classpoints[CP_CURRENT]<amount) return 0;
    ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]-=amount;
    return 1;
}

int hybrid_drow_points_set_current(CHAR_DATA *ch, long amount)
{
    if (amount<0 || !ch || IS_NPC(ch) || !ch->pcdata) return 0;
    if (IS_DROW(ch)) {
        ch->pcdata->classpoints[CP_CURRENT]=amount;
        return 1;
    }
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if (!s || s->secondary_class!=CLASS_DROW) return 0;
    ch->pcdata->hybrid.secondary_classpoints[CP_CURRENT]=amount;
    return 1;
}

int hybrid_drow_globe_upkeep(CHAR_DATA *ch, long amount)
{
    if (!ch || IS_DROW(ch) || !hybrid_power_parent(ch,CLASS_DROW) ||
        !hybrid_drow_globe_active(ch) || amount<0) return -1;
    long points=hybrid_drow_points_current(ch);
    if (points<=amount) {
        if (!hybrid_drow_points_set_current(ch,0) ||
            !hybrid_drow_globe_set(ch,0)) return -1;
        return 0;
    }
    return hybrid_drow_points_charge(ch,amount)?1:-1;
}
