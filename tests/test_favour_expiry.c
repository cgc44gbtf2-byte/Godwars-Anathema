/* Pure lifetime/parser tests; real full/short loaders are tested separately. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "classes/monk/favour.h"

static TIME_INFO_DATA advance(TIME_INFO_DATA t, int hours)
{
    while(hours--){
        if(++t.hour==24){t.hour=0;if(++t.day==35){
            t.day=0;if(++t.month==17){t.month=0;t.year++;}}}
    }
    return t;
}

static void calendars(void)
{
    unsigned casts=0;
    const long years[]={0,1,1300};
    for(unsigned y=0;y<3;y++)for(long m=0;m<17;m++)
    for(long d=0;d<35;d++)for(long h=0;h<24;h++){
        TIME_INFO_DATA t={h,d,m,years[y]};
        CHAR_DATA ch;memset(&ch,0,sizeof(ch));
        ch.more=MORE_CLOAK|MORE_GUARDIAN;
        assert(monk_favour_grant(&ch,&t));
        long until=ch.favour_until,since=ch.favour_since;
        assert(until>since&&until-since<=24&&until%24==6);
        for(int elapsed=0;elapsed<=24;elapsed++){
            TIME_INFO_DATA later=advance(t,elapsed);
            assert(monk_favour_active(&ch,&later)==(elapsed<until-since));
            long expected=elapsed<until-since?ch.more:ch.more&~MORE_FAVOUR;
            assert(monk_favour_saved_more(&ch,&later)==expected);
            assert(ch.favour_since==since&&ch.favour_until==until&&IS_MORE(&ch,MORE_FAVOUR));
        }
        TIME_INFO_DATA dawn=advance(t,(int)(until-since));
        monk_favour_reconcile(&ch,&dawn);
        assert(ch.more==(MORE_CLOAK|MORE_GUARDIAN)&&!ch.favour_until&&!ch.favour_since);
        casts++;
    }
    assert(casts==42840);
}

static void parse(const char *tail,bool valid)
{
    CHAR_DATA ch;memset(&ch,0,sizeof(ch));ch.more=MORE_FAVOUR|MORE_CLOAK;
    TIME_INFO_DATA t={5,0,0,0};
    FILE *fp=tmpfile();assert(fp);fputs(tail,fp);fputs("NEXT\n",fp);rewind(fp);
    monk_favour_read(&ch,fp);
    char next[8];assert(fgets(next,sizeof(next),fp)&&!strcmp(next,"NEXT\n"));
    fclose(fp);assert(monk_favour_active(&ch,&t)==valid);
    monk_favour_reconcile(&ch,&t);
    assert(IS_MORE(&ch,MORE_CLOAK)&&!!IS_MORE(&ch,MORE_FAVOUR)==valid);
}

static void records(void)
{
    parse(" 1 5 6\n",TRUE);parse(" 1 0 6\r\n",TRUE);
    const char *bad[]={" \n"," 1 5\n"," 2 5 6\n"," 1 -1 6\n"," 1 +5 6\n",
        " 1 5 30\n"," 1 6 6\n"," 1 6 30\n"," 1 0 7\n"," 1 5 6 x\n",
        " 1 5 6 0\n"," 1 99999999999999999999999999999999 6\n"," 1 0x5 6\n"};
    for(unsigned n=0;n<sizeof(bad)/sizeof(*bad);n++)parse(bad[n],FALSE);
    char huge[512];memset(huge,' ',sizeof(huge));huge[510]='\n';huge[511]=0;
    parse(huge,FALSE);
    CHAR_DATA ch;memset(&ch,0,sizeof(ch));ch.more=MORE_FAVOUR|MORE_CLOAK;
    FILE *fp=tmpfile();assert(fp);fputs(" 1 5 6\n 1 5 6\n",fp);rewind(fp);
    monk_favour_read(&ch,fp);monk_favour_read(&ch,fp);fclose(fp);
    TIME_INFO_DATA t={5,0,0,0};assert(!monk_favour_active(&ch,&t));
    monk_favour_reconcile(&ch,&t);assert(ch.more==MORE_CLOAK);
    memset(&ch,0,sizeof(ch));ch.more=MORE_FAVOUR|MORE_CLOAK;
    fp=tmpfile();assert(fp);fputs(" 1 5 6",fp);rewind(fp);
    monk_favour_read(&ch,fp);fclose(fp);assert(!monk_favour_active(&ch,&t));
    monk_favour_reconcile(&ch,&t);assert(ch.more==MORE_CLOAK);
}

static void guards(void)
{
    TIME_INFO_DATA valid={5,34,16,1300};
    CHAR_DATA ch;memset(&ch,0,sizeof(ch));ch.more=MORE_GUARDIAN;
    assert(monk_favour_grant(&ch,&valid));
    TIME_INFO_DATA backward=valid;backward.hour=4;
    assert(!monk_favour_active(&ch,&backward));
    for(int n=0;n<8;n++){
        TIME_INFO_DATA t=valid;
        switch(n){
        case 0:t.hour=-1;break;case 1:t.hour=24;break;
        case 2:t.day=-1;break;case 3:t.day=35;break;
        case 4:t.month=-1;break;case 5:t.month=17;break;
        case 6:t.year=-1;break;case 7:t.year=LONG_MAX;break;
        }
        CHAR_DATA before=ch;assert(!monk_favour_clock_valid(&t));
        assert(!monk_favour_grant(&ch,&t)&&!memcmp(&ch,&before,sizeof(ch)));
        assert(monk_favour_saved_more(&ch,&t)==MORE_GUARDIAN);
    }
    assert(!monk_favour_grant(NULL,&valid)&&!monk_favour_clock_valid(NULL));
    monk_favour_clear(NULL);monk_favour_read(NULL,NULL);
    monk_favour_reconcile(NULL,&valid);
    FILE *fp=tmpfile();assert(fp);monk_favour_write(fp,&ch,&valid);rewind(fp);
    char line[128];assert(fgets(line,sizeof(line),fp)&&!strncmp(line,"*AnathemaFavor 1 ",16));
    fclose(fp);
    ch.more=MORE_FAVOUR|MORE_CLOAK;ch.favour_since=ch.favour_until=0;
    monk_favour_reconcile(&ch,&valid);assert(ch.more==MORE_CLOAK);
}

int main(void)
{
    calendars();records();guards();
    puts("Favor expiry:42840 calendar grants,1071000 lifetime checks,"
         "bounded records,duplicates,legacy flags and clock guards passed.");
    return 0;
}
