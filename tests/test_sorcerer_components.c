/* Sorcerer rune/glyph/sigil learning: prerequisites, costs, duplicates, saves. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/sorcerer/components.h"

extern char *string_space,*top_string;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *,char *);
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static char output[32768];

static CHAR_DATA *fresh(long class)
{
    memset(&shell,0,sizeof(shell));memset(output,0,sizeof(output));
    assert(!load_char_obj(&shell,"Absentcomponent"));
    CHAR_DATA *ch=shell.character;
    free_string(ch->name);ch->name=str_dup("Componentfixture");
    ch->desc=&shell;shell.connected=CON_PLAYING;
    shell.outbuf=output;shell.outsize=sizeof(output);
    ch->level=LEV_SOR;ch->position=POS_STANDING;ch->class=class;
    ch->hit=ch->max_hit=50000;ch->mana=ch->max_mana=51000;
    ch->primal=100000;ch->exp=123456;
    ch->in_room=&room;ch->next=char_list;char_list=ch;
    ch->next_in_room=room.people;room.people=ch;
    return ch;
}

static void release(CHAR_DATA *ch)
{
    char_list=ch->next;room.people=ch->next_in_room;
    ch->next=ch->next_in_room=NULL;ch->desc=NULL;ch->in_room=NULL;
    free_char(ch);shell.character=NULL;
    unlink("../player/Componentfixture");unlink("../backup/Componentfixture");
}

static void command(CHAR_DATA *ch,const char *line)
{
    char buf[256];snprintf(buf,sizeof(buf),"%s",line);
    shell.outtop=0;output[0]='\0';
    interpret(ch,buf);
}

static void reload(CHAR_DATA *ch)
{
    assert(save_char_obj_checked(ch));
    for(int short_load=0;short_load<2;short_load++) {
        DESCRIPTOR_DATA d={0};
        assert(short_load?load_char_short(&d,"Componentfixture"):load_char_obj(&d,"Componentfixture"));
        CHAR_DATA *l=d.character;
        for(int i=0;i<3;i++)assert(l->pcdata->components[i]==ch->pcdata->components[i]);
        for(int i=0;i<4;i++)assert(l->pcdata->apprentice[i]==ch->pcdata->apprentice[i]);
        for(int i=0;i<6;i++)assert(l->pcdata->colors[i]==ch->pcdata->colors[i]);
        assert(l->primal==ch->primal);
        l->desc=NULL;l->in_room=NULL;free_char(l);
    }
}

extern int sorc_recipe_count(void);

static void learn_all(CHAR_DATA *ch)
{
    ch->pcdata->components[0]=2047;ch->pcdata->components[1]=1023;ch->pcdata->components[2]=15;
}

static void recipes(void)
{
    CHAR_DATA *ch=fresh(CLASS_SORCERER);
    assert(sorc_recipe_count()==53);
    ch->stats[STAT_INT]=4;
    command(ch,"recipe");assert(strstr(output,"no complete recipe"));
    command(ch,"recipe light enhancement self");assert(strstr(output,"not learned"));
    learn_all(ch);
    command(ch,"recipe");assert(strstr(output,"light enhancement self"));
    /* invalid input and unmapped combination */
    command(ch,"recipe bogus x y");assert(strstr(output,"Use: recipe"));
    command(ch,"recipe master creation self");assert(strstr(output,"Use: recipe"));
    command(ch,"recipe fire creation self");assert(strstr(output,"Nothing happens"));
    /* self recipe works, costs mana, sets a wait state */
    long mana=ch->mana;
    command(ch,"recipe light enhancement self");
    assert(strstr(output,"trace the runes")&&ch->mana<mana&&ch->wait>0);
    assert(IS_AFFECTED(ch,AFF_SANCTUARY));
    ch->wait=0;
    /* recovery, mana and target rejections leave state alone */
    ch->wait=5;mana=ch->mana;command(ch,"recipe light protection self");
    assert(strstr(output,"not recovered")&&ch->mana==mana);ch->wait=0;
    ch->mana=0;command(ch,"recipe light protection self");
    assert(strstr(output,"enough mana")&&ch->mana==0);ch->mana=51000;
    command(ch,"recipe light protection self nobody");assert(strstr(output,"only be cast on yourself"));
    command(ch,"recipe fire destruction targeting nobody");assert(strstr(output,"whom")||strstr(output,"Cast"));
    command(ch,"recipe life destruction targeting");assert(strstr(output,"whom"));
    /* hostile recipes refuse a safe room */
    room.room_flags|=ROOM_SAFE;mana=ch->mana;
    command(ch,"recipe fire destruction targeting ghost");
    assert(strstr(output,"cannot cast that here")&&ch->mana==mana);
    room.room_flags&=~ROOM_SAFE;
    /* silence */
    room.added_flags|=ROOM2_SILENCE;command(ch,"recipe light protection self");
    assert(strstr(output,"no sound"));room.added_flags&=~ROOM2_SILENCE;
    /* master rune discount */
    ch->pcdata->components[0]&=~1024;ch->wait=0;
    mana=ch->mana;command(ch,"recipe light protection self");long full=mana-ch->mana;
    ch->wait=0;ch->pcdata->components[0]|=1024;mana=ch->mana;
    command(ch,"recipe light protection self");assert(mana-ch->mana<=full);
    /* vision recipes work for a Sorcerer without the vision item flag */
    ch->wait=0;ch->mana=51000;
    assert(!IS_ITEMAFF(ch,ITEMA_VISION));
    command(ch,"recipe mind information targeting");
    assert(strstr(output,"trace the runes")&&!strstr(output,"Huh?")&&!IS_ITEMAFF(ch,ITEMA_VISION));
    ch->wait=0;
    command(ch,"recipe mind information area");
    assert(strstr(output,"trace the runes")&&!strstr(output,"Huh?")&&!IS_ITEMAFF(ch,ITEMA_VISION));
    ch->wait=0;SET_BIT(ch->itemaffect,ITEMA_VISION);
    command(ch,"recipe mind information targeting");
    assert(IS_ITEMAFF(ch,ITEMA_VISION));REMOVE_BIT(ch->itemaffect,ITEMA_VISION);
    /* adapted recipe: charges mana and gives its documented replacement effect */
    ch->wait=0;ch->mana=51000;mana=ch->mana;
    command(ch,"recipe fire enhancement object");
    assert(strstr(output,"trace the runes")&&ch->mana<mana&&ch->wait>0);
    /* soulblade validates its weapon type before charging; mana recipe is not net-free */
    ch->wait=0;mana=ch->mana;
    command(ch,"recipe spirit creation object");assert(strstr(output,"Pick slash")&&ch->mana==mana&&ch->wait==0);
    command(ch,"recipe spirit creation object sword");assert(strstr(output,"Pick slash")&&ch->mana==mana);
    {
        long lvl=5+ch->stats[STAT_INT]*5,before;
        ch->wait=0;ch->max_mana=90000;ch->mana=30000;before=ch->mana;
        command(ch,"recipe water enhancement self");
        assert(strstr(output,"trace the runes"));
        assert(ch->mana<before);
        assert(ch->mana==before-lvl*50+lvl*40);
    }
    ch->wait=0;ch->mana=51000;mana=ch->mana;
    /* summon needs a named target and no combat; unknown targets cost nothing */
    ch->wait=0;mana=ch->mana;
    command(ch,"recipe spirit summoning area");assert(strstr(output,"Summon whom")&&ch->mana==mana);
    command(ch,"recipe spirit summoning area nobodyhere");assert(strstr(output,"You failed")&&ch->mana==mana);
    ch->fighting=ch;command(ch,"recipe spirit summoning area someone");
    assert(strstr(output,"Summon whom")&&ch->mana==mana);ch->fighting=NULL;
    /* teleport is refused in combat, safe and no-teleport rooms without cost */
    ch->fighting=ch;command(ch,"recipe spirit transportation self");
    assert(strstr(output,"cannot do that here")&&ch->mana==mana);ch->fighting=NULL;
    room.room_flags|=ROOM_SAFE;command(ch,"recipe spirit transportation self");
    assert(strstr(output,"cannot do that here")&&ch->mana==mana);room.room_flags&=~ROOM_SAFE;
    room.room_flags|=ROOM_NO_TELEPORT;command(ch,"recipe spirit transportation self");
    assert(strstr(output,"cannot do that here")&&ch->mana==mana);room.room_flags&=~ROOM_NO_TELEPORT;
    /* non-sorcerer */
    ch->class=CLASS_WEREWOLF;command(ch,"recipe");assert(strstr(output,"Huh?"));ch->class=CLASS_SORCERER;
    /* existing custom-book chanting path still reachable */
    command(ch,"chant");assert(strstr(output,"hold a spellbook"));
    reload(ch);release(ch);
}

static CHAR_DATA *second(const char *name,long class)
{
    CHAR_DATA *o=alloc_perm(sizeof(CHAR_DATA));
    DESCRIPTOR_DATA *d=calloc(1,sizeof(*d));
    (void)o;(void)d;(void)name;(void)class;return NULL;
}

static void ready_apprentice(CHAR_DATA *a)
{
    a->pcdata->apprentice[0]=APP_APPRENTICE;
    a->pcdata->components[0]=1|2|4;a->pcdata->components[1]=1;a->pcdata->components[2]=1;
    a->pcdata->colors[0]=3;
}

static void apprenticeship(void)
{
    CHAR_DATA *a=fresh(CLASS_SORCERER);
    CHAR_DATA *m=NULL;
    long level=a->level,trust=get_trust(a);
    (void)second;
    ready_apprentice(a);
    command(a,"apprentice");assert(strstr(output,"0/5 lessons"));
    /* NPC route: no trainer, wrong name, then valid trainer */
    command(a,"apprentice train");assert(strstr(output,"no trainer"));
    CHAR_DATA npc;memset(&npc,0,sizeof npc);npc.act=ACT_IS_NPC;
    npc.name="guard";npc.short_descr="the guard";npc.position=POS_STANDING;npc.in_room=&room;
    npc.next_in_room=room.people;room.people=&npc;
    command(a,"apprentice train");assert(strstr(output,"no trainer"));
    npc.name="sorctrainer master";
    a->primal=9;command(a,"apprentice train");assert(strstr(output,"costs 10")&&a->pcdata->apprentice[1]==0&&a->primal==9);
    a->primal=100;
    command(a,"apprentice train");assert(a->pcdata->apprentice[1]==1&&a->primal==90&&a->pcdata->apprentice[2]==current_time);
    command(a,"apprentice train");assert(strstr(output,"absorb")&&a->pcdata->apprentice[1]==1&&a->primal==90);
    npc.position=POS_SLEEPING;current_time+=601;
    command(a,"apprentice train");assert(strstr(output,"no trainer"));
    npc.position=POS_STANDING;npc.fighting=&npc;command(a,"apprentice train");assert(strstr(output,"no trainer"));npc.fighting=NULL;
    reload(a);
    for(int i=0;i<2;i++){current_time+=601;command(a,"apprentice train");}
    assert(a->pcdata->apprentice[1]==3);
    /* graduation blocked until requirements are met */
    command(a,"apprentice graduate");assert(strstr(output,"do not meet")&&a->pcdata->apprentice[0]==APP_APPRENTICE);
    /* a non-teacher cannot advance an apprentice; offers need consent */
    command(a,"apprentice accept");assert(strstr(output,"Nobody is offering"));
    command(a,"apprentice teach Componentfixture");assert(strstr(output,"Only a graduated"));
    npc.name="sorctrainer master";
    room.people=npc.next_in_room;
    /* player route */
    m=fresh(CLASS_SORCERER);
    free_string(m->name);m->name=str_dup("Mentorfixture");
    m->pcdata->apprentice[0]=APP_NONE; /* legacy Sorcerer */
    command(m,"apprentice teach Componentfixture");
    assert(strstr(output,"offer")||strstr(output,"cannot be taught"));
    /* apprentice now holds an offer; a bystander cannot accept it */
    current_time+=601;a->primal=100;
    long lessons=a->pcdata->apprentice[1];
    command(m,"apprentice teach Mentorfixture");assert(strstr(output,"cannot be taught"));
    command(m,"apprentice teach Componentfixture");
    command(a,"apprentice decline");assert(strstr(output,"decline")&&a->pcdata->apprentice[1]==lessons);
    command(a,"apprentice accept");assert(strstr(output,"Nobody is offering"));
    command(m,"apprentice teach Componentfixture");
    command(a,"apprentice accept");
    assert(a->pcdata->apprentice[1]==lessons+1&&a->primal==95);
    command(a,"apprentice accept");assert(a->pcdata->apprentice[1]==lessons+1);
    current_time+=601;
    command(m,"apprentice teach Componentfixture");
    m->fighting=a;command(a,"apprentice accept");m->fighting=NULL;
    assert(strstr(output,"no longer able")&&a->pcdata->apprentice[1]==lessons+1);
    command(m,"apprentice teach Componentfixture");
    command(a,"apprentice accept");
    /* fifth lesson meets the requirements: graduate once, reward once */
    assert(a->pcdata->apprentice[0]==APP_GRADUATED&&a->pcdata->apprentice[1]==5);
    assert(a->primal==95+0-5+100&&a->pcdata->apprentice[3]==1);
    long primal=a->primal;
    command(a,"apprentice graduate");assert(strstr(output,"not an apprentice")&&a->primal==primal);
    command(a,"apprentice train");current_time+=601;command(a,"apprentice train");
    assert(a->primal==primal&&a->pcdata->apprentice[1]==5);
    /* a mentor does not become an apprentice, and no privilege changes occur */
    command(m,"apprentice");assert(strstr(output,"full Sorcerer"));
    assert(a->level==level&&get_trust(a)==trust&&m->level==level&&get_trust(m)==trust&&!IS_IMM(a));
    reload(a);
    /* graduated apprentices keep full component access, no limits */
    assert(sorc_learn(a,SORC_RUNE,"water")==SORC_LEARN_OK);
    /* reward flag prevents a second payout even if stage is forced back */
    a->pcdata->apprentice[0]=APP_APPRENTICE;primal=a->primal;
    command(a,"apprentice graduate");assert(a->pcdata->apprentice[0]==APP_GRADUATED&&a->primal==primal);
    release(m);release(a);
    /* a legacy Sorcerer is never forced into apprenticeship */
    CHAR_DATA *legacy=fresh(CLASS_SORCERER);
    command(legacy,"apprentice train");assert(strstr(output,"not an apprentice")||strstr(output,"no trainer"));
    assert(legacy->pcdata->apprentice[0]==APP_NONE&&!sorc_is_apprentice(legacy));
    command(legacy,"apprentice bogus");assert(strstr(output,"Use: apprentice"));
    reload(legacy);release(legacy);
}

static void catalogue(void)
{
    int n;
    sorc_component_table(SORC_RUNE,&n);assert(n==11);
    sorc_component_table(SORC_GLYPH,&n);assert(n==10);
    sorc_component_table(SORC_SIGIL,&n);assert(n==4);
    assert(sorc_component_find(SORC_RUNE,"master")->bit==1024);
    assert(sorc_component_find(SORC_RUNE,"Fire")->bit==1);
    assert(!sorc_component_find(SORC_RUNE,"")&&!sorc_component_find(SORC_RUNE,"fir"));
    assert(!sorc_component_find(SORC_GLYPH,"fire")&&!sorc_component_find(3,"fire"));
}

static void learning(void)
{
    CHAR_DATA *ch=fresh(CLASS_SORCERER);
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=i;
    long primal=ch->primal;
    /* prerequisites */
    assert(sorc_learn(ch,SORC_GLYPH,"creation")==SORC_LEARN_PREREQ);
    assert(sorc_learn(ch,SORC_SIGIL,"self")==SORC_LEARN_PREREQ);
    assert(sorc_learn(ch,SORC_RUNE,"master")==SORC_LEARN_PREREQ);
    assert(ch->primal==primal&&!ch->pcdata->components[0]);
    /* invalid input */
    assert(sorc_learn(ch,SORC_RUNE,"bogus")==SORC_LEARN_BAD_NAME);
    assert(sorc_learn(ch,SORC_RUNE,"")==SORC_LEARN_BAD_NAME);
    assert(sorc_learn(ch,SORC_RUNE,NULL)==SORC_LEARN_BAD_NAME);
    assert(sorc_learn(ch,9,"fire")==SORC_LEARN_BAD_NAME);
    command(ch,"learn rune");assert(strstr(output,"no such rune"));
    command(ch,"learn weird fire");assert(strstr(output,"Learn: learn"));
    /* costs: 20,40,... */
    command(ch,"learn rune fire");assert(strstr(output,"You learn the fire rune")&&ch->primal==primal-20);
    command(ch,"learn rune fire");assert(strstr(output,"already know")&&ch->primal==primal-20);
    command(ch,"learn rune air");assert(ch->primal==primal-60);
    command(ch,"learn glyph destruction");assert(ch->primal==primal-70);
    command(ch,"learn sigil targeting");assert(ch->primal==primal-100);
    assert(sorc_knows(ch,SORC_RUNE,1)&&sorc_knows(ch,SORC_GLYPH,2)&&sorc_knows(ch,SORC_SIGIL,2));
    for(int i=0;i<6;i++)assert(ch->pcdata->colors[i]==i);
    reload(ch);
    /* resource boundary */
    ch->primal=59;assert(sorc_learn(ch,SORC_RUNE,"earth")==SORC_LEARN_COST&&ch->primal==59);
    ch->primal=60;assert(sorc_learn(ch,SORC_RUNE,"earth")==SORC_LEARN_OK&&ch->primal==0);
    /* master needs ten element runes */
    ch->primal=100000;
    const char *el[]={"water","dark","light","life","death","mind","spirit"};
    assert(sorc_learn(ch,SORC_RUNE,"master")==SORC_LEARN_PREREQ);
    for(int i=0;i<7;i++)assert(sorc_learn(ch,SORC_RUNE,el[i])==SORC_LEARN_OK);
    assert(sorc_component_count(ch,SORC_RUNE)==10);
    assert(sorc_learn(ch,SORC_RUNE,"master")==SORC_LEARN_OK);
    assert(sorc_component_count(ch,SORC_RUNE)==11);
    /* glyph cost capped at 50, sigil flat 30 */
    const char *gl[]={"creation","summoning","transformation","transportation","enhancement","reduction"};
    for(int i=0;i<6;i++)assert(sorc_learn(ch,SORC_GLYPH,gl[i])==SORC_LEARN_OK);
    assert(sorc_component_cost(ch,SORC_GLYPH)==50&&sorc_component_cost(ch,SORC_SIGIL)==30);
    reload(ch);release(ch);
}

static void compatibility(void)
{
    CHAR_DATA *ch=fresh(CLASS_SORCERER);
    for(int i=0;i<6;i++)ch->pcdata->colors[i]=5;
    reload(ch); /* pre-feature save: zero components, still a Sorcerer with colors */
    assert(!ch->pcdata->components[0]&&ch->pcdata->apprentice[0]==APP_NONE&&!sorc_is_apprentice(ch));
    release(ch);
    CHAR_DATA *other=fresh(CLASS_WEREWOLF);
    long primal=other->primal;
    assert(sorc_learn(other,SORC_RUNE,"fire")==SORC_LEARN_NOT_SORCERER&&other->primal==primal);
    command(other,"learn");assert(strstr(output,"Huh?"));
    release(other);
    CHAR_DATA *app=fresh(CLASS_SORCERER);
    app->pcdata->apprentice[0]=APP_APPRENTICE;
    const char *r[]={"fire","air","earth","water"};
    for(int i=0;i<3;i++)assert(sorc_learn(app,SORC_RUNE,r[i])==SORC_LEARN_OK);
    assert(sorc_learn(app,SORC_RUNE,r[3])==SORC_LEARN_LIMIT);
    assert(sorc_learn(app,SORC_GLYPH,"creation")==SORC_LEARN_OK);
    assert(sorc_learn(app,SORC_GLYPH,"control")==SORC_LEARN_OK);
    assert(sorc_learn(app,SORC_GLYPH,"summoning")==SORC_LEARN_LIMIT);
    reload(app);release(app);
}

int main(void)
{
    char cwd[4096],root[]="/tmp/anathema-sorccomp-XXXXXX";
    assert(getcwd(cwd,sizeof(cwd))&&mkdtemp(root)&&chdir(root)==0);
    assert(mkdir("player",0700)==0&&mkdir("backup",0700)==0&&mkdir("run",0700)==0&&chdir("run")==0);
    string_space=malloc(4*1024*1024);assert(string_space);top_string=string_space;
    current_time=123456;init_mm();fpReserve=fopen(NULL_FILE,"r");assert(fpReserve);
    room.vnum=900;room.area=&area;room.name="Private fixture";room.description="";
    room_index_hash[900%MAX_KEY_HASH]=&room;
    catalogue();learning();compatibility();recipes();apprenticeship();
    assert(!char_list&&!room.people&&fclose(fpReserve)==0&&chdir("..")==0);
    assert(rmdir("player")==0&&rmdir("backup")==0&&rmdir("run")==0&&chdir(cwd)==0&&rmdir(root)==0);
    puts("sorcerer components: 11 runes/10 glyphs/4 sigils, prerequisites, costs, duplicates, apprentice limits, both loaders, legacy saves, recipes, chant compatibility and apprenticeship lifecycle passed");
    return 0;
}
