/* Side-effect-free original-class mastery inventory, not a conversion gate. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "hybrid.h"
#include "mastery.h"

_Static_assert(HYBRID_MASTERY_ITEMS>=32,"Update mastery report capacity");
_Static_assert(DISC_MAX==19&&DISC_ANIMALISM==0&&DISC_VICISSITUDE==18&&
    BREED_METIS==2&&AUSPICE_AHROUN==4&&TRIBE_WENDIGOS==12&&
    SPHERE_CORRESPONDENCE==0&&SPHERE_LIFE==1&&SPHERE_PRIME==2&&
    SPHERE_ENTROPY==3&&SPHERE_MIND==4&&SPHERE_SPIRIT==5&&
    SPHERE_FORCES==6&&SPHERE_MATTER==7&&SPHERE_TIME==8&&
    HIGH_WEAPON==0&&HIGH_ABILITY==3&&MONK_TECH==0&&MONK_ART==2&&
    NINJA_SORA==0&&NINJA_NINGENNO==2&&PURPLE_MAGIC==0&&BLACK_MAGIC==5&&
    DROW_MAX==65536&&DRAG_SHIELD==4096&&AGE_WYRM==2&&AGE_HATCHLING==12,
    "Update mastery mapping when native training IDs change");

static void item(HYBRID_MASTERY *r, enum hybrid_mastery_kind kind, long index,
                 const char *name, long current, long required, long minimum,
                 int exempt)
{
    if(r->count>=HYBRID_MASTERY_ITEMS){r->review++;return;}
    HYBRID_MASTERY_ITEM *v=&r->items[r->count++];
    *v=(HYBRID_MASTERY_ITEM){kind,index,current,required,name,HYBRID_READY_MISSING};
    if(exempt){v->status=HYBRID_READY_EXEMPT;return;}
    r->applicable++;
    if(current<minimum || current>required){v->status=HYBRID_READY_REVIEW;r->review++;}
    else if(current==required){v->status=HYBRID_READY_MET;r->met++;}
}

static void choice(HYBRID_MASTERY *r,long index,const char *name,long value,
                   long first,long last,int unselected)
{
    /* A valid choice counts as complete. Each native field defines its own
     * unselected sentinel: -1 for gifts/branches, 0 for elemental focus, none
     * for a Dragon color (0 is a valid Bronze Dragon).
     * Corrupt selections must not be used as array subscripts. */
    item(r,HYBRID_MASTERY_CHOICE,index,name,
         value>=first&&value<=last ? 1 : unselected ? 0 : 2,1,0,0);
}
static int selected(const long *choices,long index)
{
    return choices[0]==index||choices[1]==index||choices[2]==index;
}
static void branches(HYBRID_MASTERY *r,const long *values,long last)
{
    static const char *names[]={"Primary branch","Second branch","Third branch"};
    for(long i=0;i<3;i++)choice(r,i,names[i],values[i],0,last,values[i]==-1);
    for(long i=0;i<3;i++)for(long j=i+1;j<3;j++)
        if(values[i]>=0&&values[i]==values[j])
            item(r,HYBRID_MASTERY_RESERVED,i*3+j,"Duplicate selected branches",1,0,0,0);
}
static void array(HYBRID_MASTERY *r,enum hybrid_mastery_kind kind,
                  const long *values,const char *const *names,size_t count,long cap)
{
    for(size_t i=0;i<count;i++)item(r,kind,(long)i,names[i],values[i],cap,0,0);
}
static void unlocks(HYBRID_MASTERY *r,enum hybrid_mastery_kind kind,
                    long values,const char *const *names,size_t count,long transient)
{
    long mask=(1L<<count)-1;
    item(r,HYBRID_MASTERY_RESERVED,kind,"Unsupported unlock bits",
         values&~(mask|transient),0,0,0);
    for(size_t i=0;i<count;i++)
        item(r,kind,1L<<i,names[i],(values&(1L<<i))!=0,1,0,0);
}
static int discipline_known(long i)
{
    return i>=0&&i<=DISC_MAX&&i!=4&&i!=8&&i!=16&&i!=19;
}
static void vampire(const CHAR_DATA *ch,HYBRID_MASTERY *r)
{
    static const char *names[]={"Animalism","Auspex","Celerity","Chimerstry",
        "Reserved discipline 4","Dominate","Fortitude","Necromancy",
        "Reserved discipline 8","Obfuscate","Obtenebration","Potence","Presence",
        "Protean","Quietus","Serpentis","Reserved discipline 16","Thaumaturgy",
        "Vicissitude","Reserved discipline 19"};
    const PC_DATA *pc=ch->pcdata;
    int clanless=ch->clan&&(!str_cmp(ch->clan,"Caitiff")||!str_cmp(ch->clan,"Panders"));
    item(r,HYBRID_MASTERY_CHOICE,3,"Vampire clan/profile",
         ch->clan&&*ch->clan?1:0,1,0,0);
    if(!clanless){
        branches(r,pc->cland,DISC_MAX);
        for(long i=0;i<3;i++)if(pc->cland[i]>=0&&!discipline_known(pc->cland[i]))
            item(r,HYBRID_MASTERY_RESERVED,i,"Unavailable clan discipline",1,0,0,0);
    }
    for(long i=0;i<=DISC_MAX;i++){
        long rank=pc->powers[i];
        if(!discipline_known(i)){
            item(r,HYBRID_MASTERY_RESERVED,i,names[i],rank,0,0,rank==0);
            continue;
        }
        /* Basic and selected clan disciplines are required even if corruptly
         * zeroed. Other locked rank0 disciplines require teaching first, so
         * they are exempt until unlocked (-1) or actually learned (>0). */
        int basic=i==DISC_ANIMALISM||i==DISC_AUSPEX||i==DISC_CELERITY||
            i==DISC_DOMINATE||i==DISC_FORTITUDE||i==DISC_OBFUSCATE||
            i==DISC_POTENCE||i==DISC_PRESENCE;
        long cap=clanless?6:selected(pc->cland,i)?7:5;
        if(i==DISC_THAUMATURGY)cap=5;
        item(r,HYBRID_MASTERY_DISCIPLINE,i,names[i],rank,cap,-1,
             rank==0&&!basic&&(clanless||!selected(pc->cland,i)));
    }
}

int hybrid_mastery_read(const CHAR_DATA *ch,HYBRID_MASTERY *report)
{
    if(!report)return 0;
    memset(report,0,sizeof(*report));
    if(!ch||IS_NPC(ch)||!ch->pcdata||!hybrid_parent_name(ch->class))return 0;
    const PC_DATA *p=ch->pcdata;
    HYBRID_MASTERY r={0};
    switch(ch->class){
    case CLASS_VAMPIRE:vampire(ch,&r);break;
    case CLASS_WEREWOLF:{
        static const char *breed[]={"Homid","Lupus","Metis"};
        static const char *aus[]={"Ragabash","Theurge","Philodox","Galliard","Ahroun"};
        static const char *tribes[]={"Black Furies","Bone Gnawers","Children of Gaia",
            "Fianna","Get of Fenris","Glass Walkers","Red Talons","Shadow Lords",
            "Silent Striders","Silver Fangs","Stargazers","Uktena","Wendigos"};
        const long *values[]={p->breed,p->auspice,p->tribes};
        const char *const *names[]={breed,aus,tribes};
        const long sizes[]={3,5,13};
        static const char *choices[]={"Selected breed","Selected auspice","Selected tribe"};
        for(long f=0;f<3;f++){
            choice(&r,f,choices[f],p->tgift[f],0,sizes[f]-1,p->tgift[f]==-1);
            for(long i=0;i<sizes[f];i++)
                item(&r,(enum hybrid_mastery_kind)(HYBRID_MASTERY_BREED+f),i,
                     names[f][i],values[f][i],p->tgift[f]==i?5:4,0,0);
        }
        item(&r,HYBRID_MASTERY_WOLF_RANK,0,"Wolf advancement rank",p->rank,4,0,0);
        break;
    }
    case CLASS_MAGE:{
        static const char *names[]={"Correspondence","Life","Prime","Entropy",
            "Mind","Spirit","Forces","Matter","Time"};
        branches(&r,p->cland,SPHERE_TIME);
        for(long i=0;i<9;i++)item(&r,HYBRID_MASTERY_SPHERE,i,names[i],
            ch->spheres[i],selected(p->cland,i)?5:4,0,0);
        item(&r,HYBRID_MASTERY_ARETE,0,"Arete",ch->arete,10,0,0);
        break;
    }
    case CLASS_ELEM:{
        static const char *names[]={"Fire","Water","Earth","Wind","Sun","Moon","Time","Infinity"};
        static const long opposite[]={0,ELEM_WATER,ELEM_FIRE,ELEM_WIND,ELEM_EARTH,
            ELEM_MOON,ELEM_SUN,ELEM_INFIN,ELEM_TIME};
        long focus=p->powers[ELEM_CHOICE];
        choice(&r,0,"Element focus",focus,ELEM_FIRE,ELEM_INFIN,focus==0);
        for(long i=ELEM_FIRE;i<=ELEM_INFIN;i++){
            long cap=8;
            if(focus>=ELEM_FIRE&&focus<=ELEM_INFIN)
                cap=focus==i?10:opposite[focus]==i?6:8;
            item(&r,HYBRID_MASTERY_ELEMENT,i,names[i-ELEM_FIRE],p->powers[i],cap,0,0);
        }
        break;
    }
    case CLASS_SORCERER:{
        static const char *names[]={"Purple","Red","Blue","Green","Yellow","Black"};
        array(&r,HYBRID_MASTERY_COLOR,p->colors,names,6,5);break;
    }
    case CLASS_HIGHLANDER:{
        static const char *names[]={"Weaponry","Attack","Defense","Powers"};
        array(&r,HYBRID_MASTERY_HIGHLAND,p->highland,names,4,6);break;
    }
    case CLASS_NINJA:{
        static const char *names[]={"Sora","Chikyu","Ningenno"};
        array(&r,HYBRID_MASTERY_NINJA,p->ninja,names,3,10);break;
    }
    case CLASS_MONK:{
        static const char *names[]={"Techniques","Mantras","Arts"};
        array(&r,HYBRID_MASTERY_MONK,p->monk,names,3,8);break;
    }
    case CLASS_DROW:{
        static const char *names[]={"Acid bolt","Lloth's web","Darkness","Faerie fire",
            "Spider bite","Dual wield","Blind fighting","Summon spider","Spider eyes",
            "Read aura","Darkvision","Heighten","Poison immunity","Skip disc",
            "Attunement","Might","Toughness"};
        unlocks(&r,HYBRID_MASTERY_DROW,p->drow[DROW_POWERS],names,17,DROW_GLOBE);
        item(&r,HYBRID_MASTERY_DROW_SPEED,0,"Drow speed",p->drow[DROW_SPEED],4,0,0);
        break;
    }
    case CLASS_DRAGON:{
        static const char *names[]={"Fangs","Nightsight","Horns","Claws","Wings",
            "Breath","Tail","Truesight","Dust","Scry","Magic","Read aura","Shield"};
        choice(&r,0,"Dragon color",p->dragcol,PURPLE_MAGIC,BLACK_MAGIC,0);
        unlocks(&r,HYBRID_MASTERY_DRAGON,p->dragskills,names,13,DRAG_WINGSOUT|DRAG_HORNSOUT);
        /* Native age training stops at WYRM, not the staff-only AWYRM.
         * Descending age values are reported in their original representation. */
        item(&r,HYBRID_MASTERY_DRAGON_AGE,0,"Dragon age (Wyrm = 2)",
             ch->vampgen,AGE_HATCHLING,AGE_WYRM,0);
        HYBRID_MASTERY_ITEM *age=&r.items[r.count-1];
        if(age->status==HYBRID_READY_MET)r.met--;
        if(age->status!=HYBRID_READY_REVIEW){
            age->status=ch->vampgen==AGE_WYRM?HYBRID_READY_MET:HYBRID_READY_MISSING;
            if(age->status==HYBRID_READY_MET)r.met++;
        }
        age->required=AGE_WYRM;break;
    }
    default:return 0;
    }
    *report=r;return 1;
}

int hybrid_prerequisites_read(const CHAR_DATA *ch,HYBRID_PREREQUISITES *report)
{
    if(!report)return 0;
    memset(report,0,sizeof(*report));
    HYBRID_PREREQUISITES r={0};
    if(!hybrid_readiness_read(ch,&r.numeric)||!hybrid_mastery_read(ch,&r.mastery))return 0;
    r.quest=ch->pcdata->quest;r.quest_required=HYBRID_DEFAULT_QUEST_COST;
    r.existing_hybrid=ch->pcdata->hybrid_present!=0;
    r.rejected_player=ch->pcdata->player_load_failed!=0;
    r.requirements_met=!r.existing_hybrid&&!r.rejected_player&&
        r.quest>=r.quest_required&&!r.numeric.review&&!r.mastery.review&&
        r.numeric.met==r.numeric.applicable&&r.mastery.met==r.mastery.applicable;
    *report=r;return 1;
}

void hybrid_mastery_show(CHAR_DATA *ch)
{
    HYBRID_PREREQUISITES r;
    char line[240];
    if(!hybrid_prerequisites_read(ch,&r)){
        if(ch&&!IS_NPC(ch)&&ch->pcdata)send_to_char("Class mastery preview unavailable.\n\r",ch);
        return;
    }
    snprintf(line,sizeof line,"Original-class mastery: %zu/%zu met.\n\r",
             r.mastery.met,r.mastery.applicable);
    send_to_char(line,ch);
    for(size_t i=0;i<r.mastery.count;i++){
        const HYBRID_MASTERY_ITEM *v=&r.mastery.items[i];
        if(v->status==HYBRID_READY_MET||v->status==HYBRID_READY_EXEMPT)continue;
        snprintf(line,sizeof line,"  %s: %ld/%ld%s\n\r",v->name,v->current,v->required,
                 v->status==HYBRID_READY_REVIEW?" (stored data needs review)":"");
        send_to_char(line,ch);
    }
    if(r.existing_hybrid)send_to_char("Already has hybrid data; new conversion is blocked.\n\r",ch);
    if(r.rejected_player)send_to_char("Player data was rejected; staff review is required.\n\r",ch);
    snprintf(line,sizeof line,"Prerequisite preview: %s; planned price %ld QP (nothing charged).\n\r",
             r.requirements_met?"requirements met":"requirements not met",r.quest_required);
    send_to_char(line,ch);
    send_to_char("This is not conversion authorization. No training, gear or currency changed. "
                 "Conversion remains disabled.\n\r",ch);
}
