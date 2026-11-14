/* Secondary Sorcerer UI; all writes use the real paid atomic lesson backend. */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "access.h"
#include "books.h"
#include "colors.h"
#include "training.h"

static const char *const names[]={"purple","red","blue","green","yellow","black"};
_Static_assert(PURPLE_MAGIC==0&&RED_MAGIC==1&&BLUE_MAGIC==2&&GREEN_MAGIC==3&&
               YELLOW_MAGIC==4&&BLACK_MAGIC==5,"Native color-name mapping changed");

/* No one_argument-sized destination assumptions for arbitrary command input. */
static int word(const char **input,char out[16])
{
    const unsigned char *p=(const unsigned char *)*input;
    size_t n=0;
    while(isspace(*p))++p;
    while(*p&&!isspace(*p)) {
        if(n==15)return 0;
        out[n++]=(char)tolower(*p++);
    }
    out[n]='\0';*input=(const char *)p;
    return 1;
}

static int color(const char *name)
{
    for(int i=0;i<6;i++)if(!strcmp(name,names[i]))return i;
    return -1;
}

static void usage(CHAR_DATA *ch)
{
    send_to_char("Use: selfbrid colors | selfbrid colors <color>\n\r"
        "Choose one permanent full color: selfbrid colors focus <color>\n\r"
        "Confirm that free choice with: selfbrid colors focus <color> confirm\n\r"
        "Focus grants no ranks. Chosen cap5, other caps3; each rank costs "
        "10 x next rank primal. Conversion remains disabled.\n\r",ch);
}

static void result(CHAR_DATA *ch,enum hybrid_lesson_result r)
{
    switch(r) {
    case HYBRID_LESSON_OK:
        send_to_char("Secondary color lesson completed and saved.\n\r",ch);break;
    case HYBRID_LESSON_NO_FUNDS:
        send_to_char("Not enough primal for the next color rank. Nothing changed.\n\r",ch);break;
    case HYBRID_LESSON_AT_CAP:
        send_to_char("That color is already at its secondary cap. Nothing changed.\n\r",ch);break;
    case HYBRID_LESSON_SAVE_PENDING:
        send_to_char("Primary save recovery is pending. Wait before another lesson.\n\r",ch);break;
    case HYBRID_LESSON_SAVE_FAILED:
        send_to_char("The save failed. No lesson or payment was applied. "
                     "Wait for primary save recovery before retrying.\n\r",ch);break;
    case HYBRID_LESSON_SAVE_UNCERTAIN:
        send_to_char("Lesson/payment applied; save durability is pending. "
                     "Do not repeat; wait for primary save recovery.\n\r",ch);break;
    case HYBRID_LESSON_UNCHANGED:
        send_to_char("That choice is already selected. Nothing changed.\n\r",ch);break;
    default:
        send_to_char("Lesson refused. Be connected in your own body, standing, "
                     "out of combat, with valid training data. Nothing changed.\n\r",ch);break;
    }
}

int hybrid_colors_command(CHAR_DATA *ch,const char *argument)
{
    char args[4][16],tail[16],line[160];
    const char *p=argument?argument:"";
    if(!word(&p,args[0])||strcmp(args[0],"colors"))return 0;
    if(!ch||IS_NPC(ch)||!ch->pcdata)return 1;
    for(int i=1;i<4;i++)if(!word(&p,args[i])){usage(ch);return 1;}
    if(!word(&p,tail)||*tail){usage(ch);return 1;}
    if(ch->class==CLASS_SORCERER) {
        send_to_char("Sorcerer is your original parent. Use the unchanged native colors command.\n\r",ch);
        return 1;
    }
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if(!s||s->secondary_class!=CLASS_SORCERER) {
        send_to_char("You do not have a valid secondary Sorcerer record. "
                     "No conversion or training was performed.\n\r",ch);return 1;
    }
    if(get_trust(ch)<LEV_SOR) {
        send_to_char("Secondary colors require the same Sorcerer-tier access as native colors.\n\r",ch);
        return 1;
    }
    long focus=s->secondary_progression.full_branch;
    if(!*args[1]) {
        snprintf(line,sizeof(line),"Secondary Sorcerer colors; full color: %s.\n\r",
                 focus>=0&&focus<6?names[focus]:"not selected");
        send_to_char(line,ch);
        for(int i=0;i<6;i++) {
            long cap=0;
            if(!hybrid_parent_training_cap(ch,s,CLASS_SORCERER,HYBRID_TRAIN_COLOR,i,&cap))
                return 1;
            snprintf(line,sizeof(line),"%s: %ld/%ld (stored %ld).\n\r",names[i],
                     hybrid_book_color_rank(ch,i),cap,s->secondary_progression.colors[i]);
            send_to_char(line,ch);
        }
        if(!hybrid_secondary_ranks_valid(s))
            send_to_char("Stored training exceeds policy caps; staff review is required before lessons.\n\r",ch);
        usage(ch);return 1;
    }
    if(!hybrid_secondary_ranks_valid(s)) {
        send_to_char("Stored training needs staff review; nothing was normalized or charged.\n\r",ch);
        return 1;
    }
    HYBRID_LESSON_REQUEST request={0};
    request.parent=CLASS_SORCERER;request.tree=HYBRID_TRAIN_COLOR;
    if(!strcmp(args[1],"focus")) {
        int selected=color(args[2]);
        if(selected<0||(*args[3]&&strcmp(args[3],"confirm"))) {usage(ch);return 1;}
        if(focus!=-1) {
            send_to_char("Your full color is already chosen and cannot be changed here.\n\r",ch);
            return 1;
        }
        if(!*args[3]) {
            snprintf(line,sizeof(line),"Permanent choice: %s cap5, others3. Free; no ranks granted. "
                     "Repeat with confirm to save.\n\r",names[selected]);
            send_to_char(line,ch);return 1;
        }
        request.action=HYBRID_LESSON_SPECIALIZE;request.value=selected;
    } else {
        int selected=color(args[1]);
        if(selected<0||*args[2]||*args[3]){usage(ch);return 1;}
        if(focus==-1){send_to_char("Choose and confirm a full color before training.\n\r",ch);return 1;}
        request.action=HYBRID_LESSON_RANK;request.index=(size_t)selected;
    }
    result(ch,hybrid_training_commit(ch,&request));
    return 1;
}
