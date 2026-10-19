/* Real native getters/combat/powers; no permissive rank or damage mocks. */
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/passives.h"
#include "classes/hybrid/player.h"
#include "classes/hybrid/build.h"

extern void init_mm(void);
extern char *string_space,*top_string;
static const long parents[]={CLASS_VAMPIRE,CLASS_WEREWOLF,CLASS_SORCERER,
    CLASS_DROW,CLASS_HIGHLANDER,CLASS_MAGE,CLASS_DRAGON,CLASS_NINJA,
    CLASS_MONK,CLASS_ELEM};
static CHAR_DATA actors[2];
static PC_DATA pcs[2];
static DESCRIPTOR_DATA descs[2];
static char output[2][32768];
static ROOM_INDEX_DATA room;
static AREA_DATA area;

static void fixture(long parent,int trained)
{
    memset(pcs,0,sizeof(pcs));memset(descs,0,sizeof(descs));
    memset(output,0,sizeof(output));memset(&room,0,sizeof(room));
    memset(&area,0,sizeof(area));room.area=&area;room.light=1;
    first_fight=last_fight=NULL;char_list=room.people=actors;
    descriptor_list=NULL;
    for(int n=0;n<2;++n) {
        CHAR_DATA *ch=&actors[n];clear_char(ch);
        ch->name=ch->short_descr=n?"beta":"alpha";ch->contingency="";
        ch->pcdata=&pcs[n];ch->desc=&descs[n];
        ch->class=n?CLASS_SORCERER:parent;ch->level=LEV_AWA;ch->status=100;
        ch->position=POS_STANDING;ch->hit=ch->max_hit=100000;
        ch->mana=ch->max_mana=10000;ch->in_room=&room;
        ch->next=ch->next_in_room=n?NULL:&actors[1];
        ch->style[0]=STYLE_ONEHAND;ch->style[STYLE_ONEHAND]=100;
        for(int i=0;i<5;++i)ch->stats[i]=3;
        pcs[n].love=pcs[n].conding="";pcs[n].flag=FLAG_PK;
        descs[n].character=ch;descs[n].connected=CON_PLAYING;
        descs[n].outbuf=output[n];descs[n].outsize=sizeof(output[n]);
    }
    if(trained) {
        CHAR_DATA *ch=actors;PC_DATA *p=pcs;
        for(int i=0;i<20;++i)p->powers[i]=p->tribes[i]=3;
        for(int i=0;i<9;++i)ch->spheres[i]=3;
        for(int i=0;i<6;++i)p->colors[i]=3;
        for(int i=0;i<5;++i)p->auspice[i]=3;
        for(int i=0;i<4;++i)p->highland[i]=3;
        for(int i=0;i<3;++i)p->breed[i]=p->monk[i]=p->ninja[i]=3;
        p->drow[DROW_POWERS]=DROW_SKIP|DROW_TOUGH|DROW_EYES|DROW_MIGHT|DROW_BITE;
        p->drow[DROW_SPEED]=1;p->rank=4;p->celerity=10;
        p->wolf=100;ch->beast=30;ch->vampgen=5;
        ch->polyaff=POLY_DRAGONFORM|POLY_CHIMERA|POLY_SERPENT|POLY_CHIRO;
        SET_BIT(ch->more,MORE_FAVOUR);
    }
}
static void metrics_for(CHAR_DATA *ch,long *v)
{
    v[0]=get_speed(ch);v[1]=get_soak(ch);
    v[2]=avoidblow(ch);v[3]=breakavoid(ch);v[4]=get_damcap(ch);
    v[5]=char_hitroll(ch);v[6]=char_damroll(ch);v[7]=char_ac(ch);
}
#define metrics(v) metrics_for(actors,v)
static void attach(long secondary)
{
    HYBRID_STATE s;
    assert(hybrid_secondary_initialize(actors,
        hybrid_by_parents(actors[0].class,secondary),&s));
    assert(hybrid_player_attach(actors,&s));
}
static void configure(long parent)
{
    HYBRID_STATE *s=&pcs[0].hybrid;
    if(parent==CLASS_VAMPIRE)assert(hybrid_secondary_build_choose(s,1));
    if(parent==CLASS_MAGE)assert(hybrid_secondary_build_choose(s,101));
    if(parent==CLASS_DRAGON)assert(hybrid_secondary_build_choose(s,201));
}
static void baseline_dump(void)
{
    for(size_t p=0;p<10;++p)for(int trained=0;trained<2;++trained) {
        long v[8];fixture(parents[p],trained);metrics(v);
        printf("{%ld,%ld,%ld,%ld,%ld},\n",v[0],v[1],v[2],v[3],v[4]);
        printf("raw {%ld,%ld,%ld},\n",char_hitroll(actors),char_damroll(actors),char_ac(actors));
    }
}
/* Captured BEFORE adding their combat hooks, on d94048d's unchanged
 * native functions with this exact fixture. No active fields are copied
 * from the primary into a secondary contribution. */
static const long baseline[20][8]={
    {22,150,13,13,2000,0,0,100},{16,1125,30,29,2565,240,240,600},
    {22,150,16,16,1900,0,0,100},{15,340,36,44,2700,180,180,550},
    {23,150,13,13,1800,0,0,100},{23,250,14,14,1900,150,150,300},
    {23,150,13,13,1800,0,0,100},{19,550,26,29,2800,150,200,300},
    {23,150,13,13,1800,0,0,100},{18,400,37,32,2800,300,300,600},
    {23,150,13,13,1800,0,0,100},{18,250,29,22,2300,150,150,300},
    {23,150,13,13,1800,0,0,100},{15,450,31,30,2700,150,150,600},
    {23,150,13,13,1800,0,0,100},{20,250,17,26,2450,250,250,300},
    {23,150,13,13,1800,0,0,100},{19,250,40,31,2200,550,450,675},
    {23,150,13,13,1800,0,0,100},{23,250,14,17,1900,150,150,300}
};
static void unchanged_native(void)
{
    for(size_t p=0;p<10;++p)for(int trained=0;trained<2;++trained) {
        long v[8];fixture(parents[p],trained);metrics(v);
        assert(!memcmp(v,baseline[p*2+trained],sizeof(v)));
        for(size_t q=0;q<10;++q)if(p!=q) {
            fixture(parents[p],trained);attach(parents[q]);configure(parents[q]);
            CHAR_DATA before=actors[0];PC_DATA pc=pcs[0];metrics(v);
            assert(!memcmp(v,baseline[p*2+trained],sizeof(v)));
            assert(!memcmp(&before,actors,sizeof(before)));
            assert(!memcmp(&pc,pcs,sizeof(pc)));
        }
    }
}
static void train_fixture(void)
{
    HYBRID_STATE *s=&pcs[0].hybrid;
    long parent=s->secondary_class,full=0;
    configure(parent);
    if(parent==CLASS_MONK)full=MONK_ART;
    if(parent==CLASS_HIGHLANDER)full=HIGH_ATTACK;
    if(parent==CLASS_NINJA)full=NINJA_CHIKYU;
    if(parent==CLASS_VAMPIRE)full=DISC_FORTITUDE;
    if(parent==CLASS_ELEM)full=ELEM_FIRE;
    if(parent==CLASS_WEREWOLF) {
        full=HYBRID_GIFT_TRIBE;
        for(size_t f=0;f<3;++f)assert(hybrid_secondary_gift_select(s,f,0));
    }
    if(hybrid_specialization_count(parent))assert(hybrid_secondary_specialize(s,parent,full));
    for(int tree=0;tree<HYBRID_TRAIN_TREE_COUNT;++tree)for(size_t i=0;i<20;++i) {
        long cap;
        if(!hybrid_parent_training_cap(actors,s,parent,tree,i,&cap))continue;
        if(tree==HYBRID_TRAIN_DISCIPLINE &&
           !hybrid_power_rank(actors,parent,tree,i) &&
           (i==4||i==8||i==16||i==19))continue;
        assert(hybrid_secondary_rank_write(s,parent,tree,i,cap));
    }
    if(parent==CLASS_DROW) {
        s->secondary_progression.drow[DROW_POWERS]=
            DROW_SKIP|DROW_TOUGH|DROW_EYES|DROW_MIGHT|DROW_BITE;
        s->secondary_progression.drow[DROW_SPEED]=1;
    }
    assert(hybrid_state_valid(s));
}
static void native_counterpart(CHAR_DATA *native,PC_DATA *pc)
{
    *native=actors[0];*pc=pcs[0];native->pcdata=pc;
    HYBRID_STATE *s=&pcs[0].hybrid;HYBRID_PROGRESSION *p=&s->secondary_progression;
    native->class=s->secondary_class;pc->hybrid_present=0;
    memcpy(pc->powers,s->secondary_powers,sizeof(pc->powers));
    memcpy(pc->breed,p->breed,sizeof(p->breed));
    memcpy(pc->auspice,p->auspice,sizeof(p->auspice));
    memcpy(pc->tribes,p->tribes,sizeof(p->tribes));
    memcpy(pc->highland,p->highland,sizeof(p->highland));
    memcpy(pc->ninja,p->ninja,sizeof(p->ninja));
    memcpy(pc->monk,p->monk,sizeof(p->monk));
    memcpy(pc->drow,p->drow,sizeof(p->drow));
    memcpy(pc->colors,p->colors,sizeof(p->colors));
    memcpy(native->spheres,p->spheres,sizeof(p->spheres));
    /* Deliberately not copied: form flags, wolf/rage/Ki/rank, celerity,
     * resource counters or blood. No owned lifecycle exists for these yet. */
}
static void learned_contributions(void)
{
    for(size_t p=0;p<10;++p)for(size_t q=0;q<10;++q)if(p!=q) {
        fixture(parents[p],0);attach(parents[q]);train_fixture();
        assert(hybrid_power_parent(actors,parents[p]));
        assert(hybrid_power_parent(actors,parents[q]));
        assert(!hybrid_power_parent(actors,parents[p]|parents[q]));
        CHAR_DATA before=actors[0],native;PC_DATA pc=pcs[0],np;
        native_counterpart(&native,&np);
        long v[8],nv[8];metrics_for(&native,nv);metrics(v);
        for(int m=0;m<8;++m) {
            long expected=baseline[p*2][m]+nv[m]-baseline[q*2][m];
            if(m==0&&expected<6)expected=6;
            if(v[m]!=expected)
                fprintf(stderr,"parents %ld/%ld metric%d got%ld expected%ld\n",
                    parents[p],parents[q],m,v[m],expected);
            assert(v[m]==expected);
        }
        assert(!memcmp(&before,actors,sizeof(before))&&!memcmp(&pc,pcs,sizeof(pc)));
        long contribution[8];
        for(int m=0;m<8;++m)
            contribution[m]=hybrid_secondary_passive(actors,m,1000);
        /* Another parent's active fields must not trigger a second copy. */
        pcs[0].wolf=222;pcs[0].celerity=99;pcs[0].rank=5;
        actors[0].vampgen=2;actors[0].beast=200;
        actors[0].polyaff=POLY_DRAGONFORM|POLY_CHIMERA|POLY_SERPENT|POLY_CHIRO;
        SET_BIT(actors[0].more,MORE_FAVOUR);
        actors[0].itemaffect=ITEMA_AURA|ITEMA_AQUABARRIER|ITEMA_SUNSHIELD;
        for(int m=0;m<8;++m)
            assert(hybrid_secondary_passive(actors,m,1000)==contribution[m]);
    }
}
static void rejection_and_conditions(void)
{
    fixture(CLASS_NINJA,0);attach(CLASS_HIGHLANDER);train_fixture();
    assert(hybrid_power_rank(actors,CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,HIGH_ATTACK)==6);
    assert(!hybrid_power_rank(actors,CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,SIZE_MAX));
    assert(!hybrid_power_rank(actors,CLASS_HIGHLANDER,HYBRID_TRAIN_MONK,0));
    assert(!hybrid_power_rank(actors,CLASS_VAMPIRE,HYBRID_TRAIN_DISCIPLINE,0));
    assert(!hybrid_power_rank(NULL,CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,0));
    assert(!hybrid_power_rank(actors,CLASS_HIGHLANDER,-1,0));
    pcs[0].hybrid.secondary_progression.highland[HIGH_DEFENSE]=4; /* cap3 */
    assert(hybrid_live_secondary(actors)); /* old raw ranks are retained */
    assert(hybrid_power_rank(actors,CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,HIGH_DEFENSE)==3);
    assert(hybrid_secondary_passive(actors,HYBRID_PASSIVE_SOAK,100)==250);
    pcs[0].hybrid.secondary_progression.highland[HIGH_DEFENSE]=7; /* native cap6 */
    assert(!hybrid_live_secondary(actors));
    assert(!hybrid_power_rank(actors,CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,HIGH_ATTACK));
    assert(hybrid_secondary_passive(actors,HYBRID_PASSIVE_DAMCAP,100)==100);
    pcs[0].hybrid.secondary_progression.highland[HIGH_DEFENSE]=3;
    pcs[0].player_load_failed=1;assert(!hybrid_live_secondary(actors));
    pcs[0].player_load_failed=0;pcs[0].hybrid.primary_class=CLASS_DROW;
    assert(!hybrid_live_secondary(actors));
    fixture(CLASS_NINJA,1);attach(CLASS_HIGHLANDER);
    pcs[0].hybrid.hybrid_id=999;
    assert(hybrid_power_rank(actors,CLASS_NINJA,HYBRID_TRAIN_NINJA,NINJA_CHIKYU)==3);
    SET_BIT(actors[0].act,ACT_IS_NPC);assert(!hybrid_live_secondary(actors));
    assert(!hybrid_power_rank(actors,CLASS_NINJA,HYBRID_TRAIN_NINJA,0));

    fixture(CLASS_SORCERER,0);attach(CLASS_VAMPIRE);
    pcs[0].hybrid.secondary_powers[DISC_FORTITUDE]=3;
    assert(!hybrid_live_secondary(actors)); /* migrated identity0 */
    fixture(CLASS_SORCERER,0);attach(CLASS_VAMPIRE);configure(CLASS_VAMPIRE);
    pcs[0].hybrid.secondary_generation=0;assert(!hybrid_live_secondary(actors));
    fixture(CLASS_SORCERER,0);attach(CLASS_DROW);
    pcs[0].hybrid.secondary_progression.drow[DROW_SPEED]=1;
    assert(hybrid_secondary_passive(actors,HYBRID_PASSIVE_AVOID,-3)==0);
    assert(hybrid_secondary_passive(actors,HYBRID_PASSIVE_AVOID,LONG_MAX)==LONG_MAX);
    assert(!hybrid_power_drow(actors,DROW_GLOBE));
    assert(!hybrid_power_drow(actors,DROW_SKIP|DROW_TOUGH));
    assert(!hybrid_power_drow(actors,-1));
    fixture(CLASS_SORCERER,0);attach(CLASS_DRAGON);configure(CLASS_DRAGON);
    pcs[0].hybrid.secondary_generation=AGE_YOUNG;
    pcs[0].hybrid.secondary_progression.dragon_skills=DRAG_CLAWS;
    assert(hybrid_power_dragon(actors,DRAG_CLAWS));
    assert(!hybrid_power_dragon(actors,DRAG_CLAWS|DRAG_FANGS));
    assert(!hybrid_power_dragon(actors,-1));

    fixture(CLASS_SORCERER,0);attach(CLASS_HIGHLANDER);train_fixture();
    assert(hybrid_secondary_passive(actors,HYBRID_PASSIVE_DAMCAP,LONG_MAX)==LONG_MAX);
    assert(hybrid_secondary_passive(actors,HYBRID_PASSIVE_SPEED,LONG_MIN)==LONG_MIN);
    AFFECT_DATA af={0};af.type=gsn_brittle_bones;af.modifier=2;af.duration=5;
    affect_to_char(actors,&af);
    assert(get_soak(actors)==150); /* (150 native+150 secondary)/2 */
    affect_strip(actors,gsn_brittle_bones);
    fixture(CLASS_SORCERER,0);attach(CLASS_ELEM);
    assert(hybrid_secondary_specialize(&pcs[0].hybrid,CLASS_ELEM,ELEM_EARTH));
    assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_ELEM,
                                      HYBRID_TRAIN_ELEMENT,ELEM_EARTH,8));
    actors[0].stance[0]=STANCE_VIPER;actors[0].stance[STANCE_VIPER]=5000;
    assert(get_speed(actors)==6); /* floor AFTER the Earth slowdown */
    fixture(CLASS_SORCERER,0);attach(CLASS_WEREWOLF);
    assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_WEREWOLF,
        HYBRID_TRAIN_TRIBE,TRIBE_STARGAZERS,2));
    actors[0].hitroll=actors[0].damroll=400;actors[0].armor=400;
    long hr=char_hitroll(actors),dr=char_damroll(actors),ac=char_ac(actors);
    actors[0].hit=50000;
    assert(char_hitroll(actors)<hr&&char_damroll(actors)<dr&&char_ac(actors)<ac);
    assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_WEREWOLF,
        HYBRID_TRAIN_AUSPICE,AUSPICE_PHILODOX,1));
    assert(char_hitroll(actors)==hr&&char_damroll(actors)==dr&&char_ac(actors)==ac);
    fixture(CLASS_NINJA,0);attach(CLASS_HIGHLANDER);train_fixture();
    current_time=123;init_mm();long expected=number_range(1,100000);
    init_mm();long v[8];metrics(v);
    assert(number_range(1,100000)==expected);
}
static void highlander_powers(void)
{
    fixture(CLASS_MAGE,0);attach(CLASS_HIGHLANDER);
    HYBRID_STATE *s=&pcs[0].hybrid;
    assert(hybrid_secondary_rank_write(s,CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,HIGH_ABILITY,2));
    interpret(actors,"bflash beta");assert(IS_AFFECTED(&actors[1],AFF_BLIND));
    assert(actors[0].fighting==&actors[1]&&actors[0].wait==24);
    affect_strip(&actors[1],skill_lookup("blind"));
    stop_fighting(actors,TRUE);
    fixture(CLASS_MAGE,0);attach(CLASS_HIGHLANDER);
    do_bladeflash(actors,"beta");assert(!actors[1].affected&&!actors[0].fighting);
    assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_HIGHLANDER,
        HYBRID_TRAIN_HIGHLAND,HIGH_ABILITY,2));
    SET_BIT(actors[1].itemaffect,ITEMA_PEACE);
    do_bladeflash(actors,"beta");assert(!actors[1].affected&&!actors[0].wait);
    REMOVE_BIT(actors[1].itemaffect,ITEMA_PEACE);
    do_bladeflash(actors,"alpha");assert(!actors[0].affected&&!actors[0].wait);

    fixture(CLASS_MAGE,0);attach(CLASS_HIGHLANDER);
    assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_HIGHLANDER,
        HYBRID_TRAIN_HIGHLAND,HIGH_ABILITY,3));
    for(int seed=1;seed<=20&&actors[1].hit==100000;++seed) {
        current_time=seed;init_mm();interpret(actors,"bash beta");
    }
    assert(actors[1].hit<100000&&actors[1].wait==10&&actors[0].wait==20);
    assert(actors[0].class==CLASS_MAGE&&pcs[0].highland[HIGH_ABILITY]==0);
    stop_fighting(actors,TRUE);
    fixture(CLASS_MAGE,0);attach(CLASS_HIGHLANDER);
    assert(hybrid_secondary_specialize(&pcs[0].hybrid,CLASS_HIGHLANDER,HIGH_ABILITY));
    assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_HIGHLANDER,
        HYBRID_TRAIN_HIGHLAND,HIGH_ABILITY,5));
    interpret(actors,"judge beta");
    assert(strstr(output[0],"beta is Awakened.")&&strstr(output[0],"Health:"));
    /* Counter needs BOTH native rank4 paths. One full tree never grants it. */
    for(long full=0;full<4;++full) {
        fixture(CLASS_MAGE,0);attach(CLASS_HIGHLANDER);
        assert(hybrid_secondary_specialize(&pcs[0].hybrid,CLASS_HIGHLANDER,full));
        for(size_t i=0;i<4;++i) {
            long cap;assert(hybrid_parent_training_cap(actors,&pcs[0].hybrid,
                CLASS_HIGHLANDER,HYBRID_TRAIN_HIGHLAND,i,&cap));
            assert(hybrid_secondary_rank_write(&pcs[0].hybrid,CLASS_HIGHLANDER,
                HYBRID_TRAIN_HIGHLAND,i,cap));
        }
        do_counter(actors,"");
        assert(!actors[0].highland_guard&&actors[0].mana==10000&&!actors[0].wait);
        pcs[0].hybrid.secondary_progression.highland[HIGH_DEFENSE]=4;
        pcs[0].hybrid.secondary_progression.highland[HIGH_ABILITY]=4;
        do_counter(actors,"");
        assert(!actors[0].highland_guard&&actors[0].mana==10000&&!actors[0].wait);
    }
}
int main(int argc,char **argv)
{
    string_space=malloc(1024*1024);assert(string_space);
    top_string=string_space;*top_string='\0';init_mm();
    if(argc==2&&!strcmp(argv[1],"--baseline")) {baseline_dump();return 0;}
    unchanged_native();learned_contributions();
    rejection_and_conditions();highlander_powers();
    puts("hybrid access: 20 native snapshots/180 untrained pairs,90 trained pairs;"
         " owned ranks,passives,conditions,RNG and live Highlander powers passed");
    return 0;
}
