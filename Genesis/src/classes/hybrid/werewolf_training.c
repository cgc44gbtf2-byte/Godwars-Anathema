/* Secondary Werewolf family focus, gift selection and paid rank training.
 * Every state change goes through the existing atomic lesson transaction. */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "access.h"
#include "hybrid.h"
#include "specialization.h"
#include "training.h"
#include "werewolf_training.h"

struct gift_name {
    const char *key;
    const char *label;
};

static const char *const family_names[]={"breed","auspice","tribe"};
static const struct gift_name breed_gifts[]={
    {"homid","Homid"},{"lupus","Lupus"},{"metis","Metis"}
};
static const struct gift_name auspice_gifts[]={
    {"ragabash","Ragabash"},{"theurge","Theurge"},{"philodox","Philodox"},
    {"galliard","Galliard"},{"ahroun","Ahroun"}
};
static const struct gift_name tribe_gifts[]={
    {"black-furies","Black Furies"},{"bone-gnawers","Bone Gnawers"},
    {"children-of-gaia","Children of Gaia"},{"fianna","Fianna"},
    {"get-of-fenris","Get of Fenris"},{"glass-walkers","Glass Walkers"},
    {"red-talons","Red Talons"},{"shadow-lords","Shadow Lords"},
    {"silent-striders","Silent Striders"},{"silver-fangs","Silver Fangs"},
    {"stargazers","Stargazers"},{"uktena","Uktena"},{"wendigos","Wendigos"}
};
static const struct gift_name *const gifts[]={
    breed_gifts,auspice_gifts,tribe_gifts
};
static const size_t gift_counts[]={
    sizeof(breed_gifts)/sizeof(breed_gifts[0]),
    sizeof(auspice_gifts)/sizeof(auspice_gifts[0]),
    sizeof(tribe_gifts)/sizeof(tribe_gifts[0])
};
static const enum hybrid_training_tree training_trees[]={
    HYBRID_TRAIN_BREED,HYBRID_TRAIN_AUSPICE,HYBRID_TRAIN_TRIBE
};
_Static_assert(sizeof(breed_gifts)/sizeof(breed_gifts[0])==BREED_MAX+1,
               "Werewolf breed catalog changed");
_Static_assert(sizeof(auspice_gifts)/sizeof(auspice_gifts[0])==AUSPICE_MAX+1,
               "Werewolf auspice catalog changed");
_Static_assert(sizeof(tribe_gifts)/sizeof(tribe_gifts[0])==TRIBE_MAX+1,
               "Werewolf tribe catalog changed");

static int word(const char **input,char out[32])
{
    const unsigned char *p=(const unsigned char *)*input;
    size_t n=0;
    while(isspace(*p))++p;
    while(*p&&!isspace(*p)){
        if(n==31)return 0;
        out[n++]=(char)tolower(*p++);
    }
    out[n]='\0';*input=(const char *)p;return 1;
}

static int family(const char *name)
{
    for(int i=0;i<3;i++)if(!strcmp(name,family_names[i]))return i;
    return -1;
}

static int gift_index(int f,const char *key)
{
    if(f<0||f>=3)return -1;
    for(size_t i=0;i<gift_counts[f];i++)
        if(!strcmp(key,gifts[f][i].key))return (int)i;
    return -1;
}

static void usage(CHAR_DATA *ch)
{
    send_to_char(
        "Use: selfbrid wolf (show secondary Werewolf training)\n\r"
        "Choose one permanent full family: selfbrid wolf focus "
        "<breed|auspice|tribe>\n\r"
        "Confirm that free choice: selfbrid wolf focus <family> confirm\n\r"
        "Choose one gift per family: selfbrid wolf gift <family> <gift-key> "
        "[confirm]\n\r"
        "Buy one gift rank: selfbrid wolf gain <family> <gift-key>\n\r"
        "The focused family uses its native cap (selected gift 5, others 4); "
        "the other families cap at 3. Rank costs follow native gifts "
        "(10 x next rank for your selected gift, otherwise 15 x next rank).\n\r"
        "Some active Werewolf commands still need hybrid-specific runtime "
        "integration; conversion remains disabled.\n\r",ch);
}

static void result(CHAR_DATA *ch,enum hybrid_lesson_result r)
{
    switch(r){
    case HYBRID_LESSON_OK:
        send_to_char("Secondary Werewolf lesson completed and saved.\n\r",ch);break;
    case HYBRID_LESSON_NO_FUNDS:
        send_to_char("Not enough primal for that next gift rank. Nothing changed.\n\r",ch);break;
    case HYBRID_LESSON_AT_CAP:
        send_to_char("That gift is at its secondary cap. Nothing changed.\n\r",ch);break;
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

static void display_family(CHAR_DATA *ch,const HYBRID_STATE *s,int f)
{
    enum hybrid_training_tree tree=training_trees[f];
    const long *stored=f==0?s->secondary_progression.breed:
        f==1?s->secondary_progression.auspice:s->secondary_progression.tribes;
    long selected=s->secondary_progression.selected_gifts[f];
    char line[192];
    snprintf(line,sizeof(line),"%s gifts (chosen native gift: %s):\n\r",
        family_names[f],selected>=0&&(size_t)selected<gift_counts[f]?
        gifts[f][selected].label:"none");
    send_to_char(line,ch);
    for(size_t i=0;i<gift_counts[f];i++){
        long cap=0;
        if(!hybrid_parent_training_cap(ch,s,CLASS_WEREWOLF,tree,i,&cap)){
            send_to_char("  Stored family data needs staff review.\n\r",ch);
            return;
        }
        long rank=stored[i];
        long multiplier=selected==(long)i?10:15;
        if(rank<cap)
            snprintf(line,sizeof(line),"  %-20s %ld/%ld; next rank: %ld primal\n\r",
                gifts[f][i].label,rank,cap,(rank<1?1:rank+1)*multiplier);
        else
            snprintf(line,sizeof(line),"  %-20s %ld/%ld; at cap\n\r",
                gifts[f][i].label,rank,cap);
        send_to_char(line,ch);
    }
}

static void show(CHAR_DATA *ch,const HYBRID_STATE *s)
{
    long focus=s->secondary_progression.full_branch;
    char line[128];
    snprintf(line,sizeof(line),"Secondary Werewolf full family: %s.\n\r",
        focus>=0&&focus<3?family_names[focus]:"not selected");
    send_to_char(line,ch);
    for(int f=0;f<3;f++)display_family(ch,s,f);
    if(!hybrid_secondary_ranks_valid(s))
        send_to_char("Stored gift ranks exceed the policy cap; staff review is required "
                     "before any lesson.\n\r",ch);
    usage(ch);
}

int hybrid_werewolf_training_command(CHAR_DATA *ch,const char *argument)
{
    char args[5][32],tail[32];
    const char *p=argument?argument:"";
    if(!word(&p,args[0]) ||
       (strcmp(args[0],"wolf")&&strcmp(args[0],"werewolf")))return 0;
    if(!ch||IS_NPC(ch)||!ch->pcdata)return 1;
    for(int i=1;i<5;i++)if(!word(&p,args[i])){usage(ch);return 1;}
    if(!word(&p,tail)||*tail){usage(ch);return 1;}
    if(ch->class==CLASS_WEREWOLF){
        send_to_char("Werewolf is your original parent. Use the unchanged native "
                     "gifts command.\n\r",ch);return 1;
    }
    const HYBRID_STATE *s=hybrid_live_secondary(ch);
    if(!s||s->secondary_class!=CLASS_WEREWOLF){
        send_to_char("You do not have a valid secondary Werewolf record. "
                     "No conversion or training was performed.\n\r",ch);return 1;
    }
    if(!*args[1]){
        if(*args[2]||*args[3]||*args[4]){usage(ch);return 1;}
        show(ch,s);return 1;
    }
    if(!hybrid_secondary_ranks_valid(s)){
        send_to_char("Stored training needs staff review; nothing was changed or charged.\n\r",ch);
        return 1;
    }

    HYBRID_LESSON_REQUEST request={0};
    request.parent=CLASS_WEREWOLF;
    long focus=s->secondary_progression.full_branch;
    if(!strcmp(args[1],"focus")){
        int selected=family(args[2]);
        if(selected<0||(*args[3]&&strcmp(args[3],"confirm"))||*args[4]){
            usage(ch);return 1;
        }
        if(focus!=-1){
            send_to_char("Your full Werewolf family is already chosen and cannot be "
                         "changed here.\n\r",ch);return 1;
        }
        if(!*args[3]){
            char line[160];
            snprintf(line,sizeof(line),"Permanent choice: %s is the full family; "
                "the other two cap at 3. Free, no ranks granted. Repeat with "
                "confirm to save.\n\r",family_names[selected]);
            send_to_char(line,ch);return 1;
        }
        request.action=HYBRID_LESSON_SPECIALIZE;
        request.value=selected;
    }else if(!strcmp(args[1],"gift")){
        int f=family(args[2]),index=gift_index(f,args[3]);
        if(f<0||index<0||(*args[4]&&strcmp(args[4],"confirm"))){
            usage(ch);return 1;
        }
        long selected=s->secondary_progression.selected_gifts[f];
        if(selected!=-1){
            send_to_char(selected==index?
                "That gift is already selected for this family.\n\r":
                "Your gift for this family is already chosen and cannot be changed here.\n\r",ch);
            return 1;
        }
        if(!*args[4]){
            char line[160];
            snprintf(line,sizeof(line),"Permanent gift choice: %s for %s; free, no ranks "
                "granted. Repeat with confirm to save.\n\r",
                gifts[f][index].label,family_names[f]);
            send_to_char(line,ch);return 1;
        }
        request.action=HYBRID_LESSON_GIFT_SELECT;
        request.index=(size_t)f;request.value=index;
    }else if(!strcmp(args[1],"gain")){
        int f=family(args[2]),index=gift_index(f,args[3]);
        if(f<0||index<0||*args[4]){usage(ch);return 1;}
        if(focus<0){
            send_to_char("Choose and confirm a full family before training.\n\r",ch);
            return 1;
        }
        request.action=HYBRID_LESSON_RANK;
        request.tree=training_trees[f];request.index=(size_t)index;
    }else{
        usage(ch);return 1;
    }
    result(ch,hybrid_training_commit(ch,&request));
    return 1;
}
