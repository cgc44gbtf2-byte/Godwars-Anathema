/* Secondary Drow UI exposes only lessons with audited secondary behavior. */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "access.h"
#include "drow_training.h"
#include "training.h"

struct drow_utility {
    const char *name;
    long flag;
    long cost;
};

static const struct drow_utility utilities[] = {
    {"attunement",DROW_ATTUNE,1000},
    {"faeriefire",DROW_FIRE,5000},
    {"acidbolt",DROW_ACID,5000},
    {"llothsweb",DROW_WEB,8000},
    {"darkness",DROW_DARK,9500},
    {"readaura",DROW_READ,6000},
    {"heighten",DROW_SIGHT,20000},
    {"spidereyes",DROW_EYES,7500},
    {"poisonimmunity",DROW_PIMM,5000},
    {"blindfight",DROW_BFIGHT,5000},
    {"toughness",DROW_TOUGH,8000}
};

static int word(const char **input,char out[32])
{
    const unsigned char *p=(const unsigned char *)*input;
    size_t n=0;
    while(isspace(*p))++p;
    while(*p&&!isspace(*p)) {
        if(n==31)return 0;
        out[n++]=(char)tolower(*p++);
    }
    out[n]='\0';
    *input=(const char *)p;
    return 1;
}

static const struct drow_utility *find_utility(const char *name)
{
    for(size_t i=0;i<sizeof(utilities)/sizeof(utilities[0]);++i)
        if(!strcmp(name,utilities[i].name))return &utilities[i];
    return NULL;
}

static void usage(CHAR_DATA *ch)
{
    send_to_char("Use: selfbrid drow | selfbrid drow <attunement|faeriefire|acidbolt|llothsweb|darkness|readaura|heighten|spidereyes|poisonimmunity|blindfight|toughness|speed>\n\r"
        "Learn an integrated power: selfbrid drow <power> learn; Speed: selfbrid drow speed learn\n\r"
        "Globe of Darkness: globedarkness (500 DP to activate, 150 DP per upkeep tick).\n\r"
        "Other secondary Drow powers are not available yet; conversion remains disabled.\n\r",
        ch);
}

static void result(CHAR_DATA *ch,enum hybrid_lesson_result r)
{
    switch(r) {
    case HYBRID_LESSON_OK:
        send_to_char("Secondary Drow power learned and saved.\n\r",ch);break;
    case HYBRID_LESSON_NO_FUNDS:
        send_to_char("Not enough secondary Drow points. Nothing changed.\n\r",ch);break;
    case HYBRID_LESSON_AT_CAP:
        send_to_char("That Drow power is already learned. Nothing changed.\n\r",ch);break;
    case HYBRID_LESSON_SAVE_PENDING:
        send_to_char("Primary save recovery is pending. Wait before another lesson.\n\r",ch);break;
    case HYBRID_LESSON_SAVE_FAILED:
        send_to_char("The save failed. No lesson or points were applied. "
                     "Wait for primary save recovery before retrying.\n\r",ch);break;
    case HYBRID_LESSON_SAVE_UNCERTAIN:
        send_to_char("Lesson applied; save durability is uncertain. Do not repeat; "
                     "wait for primary save recovery.\n\r",ch);break;
    default:
        send_to_char("Lesson refused. Use your own connected, standing, out-of-combat "
                     "character with a valid secondary Drow record.\n\r",ch);break;
    }
}

static void show(CHAR_DATA *ch,const struct drow_utility *u)
{
    char line[192];
    int learned=hybrid_power_drow(ch,u->flag);
    if(u->flag==DROW_DARK)
        snprintf(line,sizeof(line),"%s: %s; %ld DP to learn. "
            "globedarkness: 500 DP activation, 150 DP upkeep per tick; "
            "free to deactivate.\n\r",u->name,learned?"learned":"available",u->cost);
    else
        snprintf(line,sizeof(line),"%s: %s; %ld DP to learn. Activation has no "
            "additional charge.\n\r",u->name,learned?"learned":"available",u->cost);
    send_to_char(line,ch);
}

static void show_speed(CHAR_DATA *ch,const HYBRID_STATE *s)
{
    char line[192];
    long rank=s->secondary_progression.drow[DROW_SPEED];
    if(rank>=4)
        snprintf(line,sizeof(line),"Speed: rank %ld/4; maximum reached.\n\r",rank);
    else
        snprintf(line,sizeof(line),"Speed: rank %ld/4; next rank costs 10000 DP. "
            "Each rank improves your existing Drow combat-speed bonuses.\n\r",rank);
    send_to_char(line,ch);
}

int hybrid_drow_training_command(CHAR_DATA *ch,const char *argument)
{
    char args[3][32],tail[32];
    const char *p=argument?argument:"";
    if(!word(&p,args[0])||strcmp(args[0],"drow"))return 0;
    if(!ch||IS_NPC(ch)||!ch->pcdata)return 1;
    for(size_t i=1;i<3;++i)if(!word(&p,args[i])){usage(ch);return 1;}
    if(!word(&p,tail)||*tail){usage(ch);return 1;}
    if(ch->class==CLASS_DROW) {
        send_to_char("Drow is your original class; use the unchanged native prayer command.\n\r",ch);
        return 1;
    }
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if(!s||s->secondary_class!=CLASS_DROW) {
        send_to_char("You do not have a valid secondary Drow record. "
                     "No training or payment was performed.\n\r",ch);
        return 1;
    }
    if(!*args[1]) {
        char points[128];
        snprintf(points,sizeof(points),"Secondary Drow points: %ld%s.\n\r",
            hybrid_drow_points_current(ch),
            hybrid_drow_globe_active(ch)?"; Globe of Darkness is active":"");
        send_to_char(points,ch);
        for(size_t i=0;i<sizeof(utilities)/sizeof(utilities[0]);++i)
            show(ch,&utilities[i]);
        show_speed(ch,s);
        if(!hybrid_secondary_ranks_valid(s))
            send_to_char("Stored Drow training needs staff review; no lesson is allowed.\n\r",ch);
        usage(ch);
        return 1;
    }
    if(!strcmp(args[1],"speed")) {
        if(!hybrid_secondary_ranks_valid(s)) {
            send_to_char("Stored Drow training needs staff review; no lesson is allowed.\n\r",ch);
            return 1;
        }
        if(!*args[2]) {
            show_speed(ch,s);
            return 1;
        }
        if(strcmp(args[2],"learn")) {
            usage(ch);
            return 1;
        }
        HYBRID_LESSON_REQUEST request={0};
        request.action=HYBRID_LESSON_DROW_SPEED;
        request.parent=CLASS_DROW;
        result(ch,hybrid_training_commit(ch,&request));
        return 1;
    }
    const struct drow_utility *u=find_utility(args[1]);
    if(!u) {
        send_to_char("That Drow power is not offered by this secondary trainer yet.\n\r",ch);
        usage(ch);
        return 1;
    }
    if(!*args[2]) {
        show(ch,u);
        if(!hybrid_secondary_ranks_valid(s))
            send_to_char("Stored Drow training needs staff review; no lesson is allowed.\n\r",ch);
        usage(ch);
        return 1;
    }
    if(strcmp(args[2],"learn")||!hybrid_secondary_ranks_valid(s)) {
        usage(ch);
        return 1;
    }
    HYBRID_LESSON_REQUEST request={0};
    request.action=HYBRID_LESSON_DROW_POWER;
    request.parent=CLASS_DROW;
    request.index=(size_t)u->flag;
    result(ch,hybrid_training_commit(ch,&request));
    return 1;
}
