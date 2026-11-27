/* Optional, legacy-comment-compatible lifetime metadata for Almighty Favor. */
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include "../../merc.h"
#include "favour.h"

static bool hour_stamp(const TIME_INFO_DATA *clock, long *stamp)
{
    if (!clock || !stamp || clock->year<0 || clock->month<0 ||
        clock->month>=17 || clock->day<0 || clock->day>=35 ||
        clock->hour<0 || clock->hour>=24) return FALSE;
    const long factor[]={17,35,24};
    const long add[]={clock->month,clock->day,clock->hour};
    long value=clock->year;
    for (int n=0;n<3;n++) {
        if (value>(LONG_MAX-add[n])/factor[n]) return FALSE;
        value=value*factor[n]+add[n];
    }
    *stamp=value;
    return TRUE;
}

static bool next_dawn(long now, long *until)
{
    long delta=(6-now%24+24)%24;
    if (!delta) delta=24;
    if (now<0 || now>LONG_MAX-delta) return FALSE;
    *until=now+delta;
    return TRUE;
}

bool monk_favour_clock_valid(const TIME_INFO_DATA *clock)
{
    long now,until;
    return hour_stamp(clock,&now) && next_dawn(now,&until);
}

bool monk_favour_grant(CHAR_DATA *ch, const TIME_INFO_DATA *clock)
{
    long now,until;
    if (!ch || !hour_stamp(clock,&now) || !next_dawn(now,&until)) return FALSE;
    ch->favour_since=now;ch->favour_until=until;
    SET_BIT(ch->more,MORE_FAVOUR);
    return TRUE;
}

bool monk_favour_active(const CHAR_DATA *ch, const TIME_INFO_DATA *clock)
{
    long now,expected;
    return ch && IS_MORE(ch,MORE_FAVOUR) && ch->favour_since>=0 &&
        ch->favour_until>0 && hour_stamp(clock,&now) &&
        next_dawn(ch->favour_since,&expected) && expected==ch->favour_until &&
        now>=ch->favour_since && now<ch->favour_until;
}

void monk_favour_clear(CHAR_DATA *ch)
{
    if (!ch) return;
    REMOVE_BIT(ch->more,MORE_FAVOUR);
    ch->favour_since=ch->favour_until=0;
}

long monk_favour_saved_more(const CHAR_DATA *ch, const TIME_INFO_DATA *clock)
{
    if (!ch) return 0;
    return monk_favour_active(ch,clock)?ch->more:ch->more&~MORE_FAVOUR;
}

void monk_favour_write(FILE *fp, const CHAR_DATA *ch, const TIME_INFO_DATA *clock)
{
    if (fp && monk_favour_active(ch,clock))
        fprintf(fp,"*AnathemaFavor 1 %ld %ld\n",ch->favour_since,ch->favour_until);
}

static bool decimal(const char **text, long *value)
{
    const unsigned char *p=(const unsigned char *)*text;
    while (*p && isspace(*p)) p++;
    if (*p<'0' || *p>'9') return FALSE;
    long n=0;
    do {
        long digit=*p-'0';
        if (n>(LONG_MAX-digit)/10) return FALSE;
        n=n*10+digit;p++;
    } while (*p>='0' && *p<='9');
    if (*p && !isspace(*p)) return FALSE;
    *text=(const char *)p;*value=n;
    return TRUE;
}

void monk_favour_read(CHAR_DATA *ch, FILE *fp)
{
    char line[128];size_t used=0;int c;
    bool overflow=FALSE;
    if (!ch || !fp) return;
    /* Drain exactly this comment line even on overflow; never parse the next
     * player field as part of a truncated or malformed metadata record. */
    while ((c=fgetc(fp))!=EOF && c!='\n') {
        if (!c || used+1>=sizeof(line)) overflow=TRUE;
        else line[used++]=(char)c;
    }
    line[used]='\0';
    bool duplicate=ch->favour_since!=0 || ch->favour_until!=0;
    ch->favour_since=0;ch->favour_until=-1;
    const char *p=line;long version,since,until,expected;
    if (duplicate || overflow || c!='\n' || !decimal(&p,&version) ||
        !decimal(&p,&since) || !decimal(&p,&until)) return;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p || version!=1 || !next_dawn(since,&expected) || expected!=until) return;
    ch->favour_since=since;ch->favour_until=until;
}

void monk_favour_reconcile(CHAR_DATA *ch, const TIME_INFO_DATA *clock)
{
    if (!monk_favour_active(ch,clock)) monk_favour_clear(ch);
}
