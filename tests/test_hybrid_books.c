/* Real native interpreter/pages/RNG/cost/effects: no casting/getter stubs. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/books.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"

extern void init_mm(void);
extern long chainspell;
extern void adv_spell_damage(CHAR_DATA *,OBJ_DATA *,OBJ_DATA *,char *,int);
extern void adv_spell_affect(CHAR_DATA *,OBJ_DATA *,OBJ_DATA *,char *,int);
extern void adv_spell_action(CHAR_DATA *,OBJ_DATA *,OBJ_DATA *,char *,int);
static const long parents[] = {CLASS_DRAGON,CLASS_DROW,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};
static CHAR_DATA actor;
static PC_DATA pc;
static DESCRIPTOR_DATA desc;
static ROOM_INDEX_DATA room;
static AREA_DATA area;
static OBJ_DATA book,pages[12];
static char output[32768];

static void fixture(long parent,long secondary,int count,int start,int damage_only)
{
    while (actor.affected) affect_remove(&actor,actor.affected);
    clear_char(&actor);
    memset(&pc,0,sizeof(pc));memset(&desc,0,sizeof(desc));
    memset(&room,0,sizeof(room));memset(&area,0,sizeof(area));
    memset(&book,0,sizeof(book));memset(pages,0,sizeof(pages));
    memset(output,0,sizeof(output));
    current_time=123;first_fight=last_fight=NULL;descriptor_list=NULL;
    char_list=room.people=&actor;room.area=&area;room.light=1;room.vnum=1000;
    actor.name=actor.short_descr="alpha";actor.contingency="";
    actor.pcdata=&pc;actor.desc=&desc;actor.class=parent;actor.level=LEV_SOR;
    actor.status=100;actor.position=POS_STANDING;actor.vampgen=9;
    actor.in_room=&room;actor.hit=1;actor.max_hit=1000000;
    actor.mana=actor.max_mana=1000000;
    actor.style[0]=STYLE_UNARMED;
    for(int i=0;i<5;i++)actor.stats[i]=3;
    for(int i=0;i<6;i++)pc.colors[i]=2;
    pc.love=pc.conding="";pc.flag=FLAG_PK;
    desc.character=&actor;desc.connected=CON_PLAYING;
    desc.outbuf=output;desc.outsize=sizeof(output);
    book.item_type=ITEM_BOOK;book.name="testbook";book.short_descr="a testbook";
    book.wear_loc=WEAR_HOLD;book.carried_by=&actor;book.value[2]=1;
    actor.carrying=&book;book.contains=count?pages:NULL;
    for(int i=0;i<count;i++) {
        OBJ_DATA *p=&pages[i];int kind=damage_only?0:(i+start)%3;
        p->item_type=ITEM_PAGE;p->name="page";p->short_descr="a page";
        p->value[0]=i+1;p->level=1;p->in_obj=&book;
        p->next_content=i+1<count?&pages[i+1]:NULL;
        p->spectype=ADV_FINISHED|(kind==0?ADV_DAMAGE:kind==1?ADV_AFFECT:ADV_ACTION);
        p->toughness=kind==0?BLUE_MAGIC:kind==1?i/3:GREEN_MAGIC;
        p->chpoweroff=p->victpoweron=p->victpoweroff="A glyph glows.";
        if(i+1<count){p->spectype|=ADV_NEXT_PAGE;p->specpower=i+2;}
    }
    if(secondary) {
        HYBRID_STATE state;
        assert(hybrid_secondary_initialize(&actor,hybrid_by_parents(parent,secondary),&state));
        assert(hybrid_player_attach(&actor,&state));
    }
}

static void cast(long result[7])
{
    unsigned char before[sizeof(pc)];memcpy(before,&pc,sizeof(pc));
    init_mm();interpret(&actor,"chant alpha");
    result[0]=actor.hit;result[1]=actor.mana;result[2]=actor.wait;
    result[3]=actor.position;result[4]=number_range(1,1000000);
    result[5]=result[6]=0;
    for(AFFECT_DATA *a=actor.affected;a;a=a->next) {
        ++result[5];result[6]+=a->type+a->duration+a->modifier+a->location+a->bitvector;
    }
    assert(!memcmp(before,&pc,sizeof(pc)));
}

static uint64_t native_dump(int emit)
{
    uint64_t hash=UINT64_C(14695981039346656037);
    const int lengths[]={0,1,3,4,6,10,12};
    for(size_t p=0;p<10;p++)for(int start=0;start<3;start++)
        for(size_t n=0;n<sizeof(lengths)/sizeof(lengths[0]);n++) {
            fixture(parents[p],0,lengths[n],start,0);
            long v[7];cast(v);
            char line[256];
            int size=snprintf(line,sizeof(line),
                "%ld,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld\n",
                parents[p],start,lengths[n],v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
            assert(size>0&&(size_t)size<sizeof(line));
            for(int j=0;j<size;j++)
                hash=(hash^(unsigned char)line[j])*UINT64_C(1099511628211);
            if(emit)fputs(line,stdout);
        }
    return hash;
}

static long expected(long first,long second)
{
    if(first!=CLASS_SORCERER&&second!=CLASS_SORCERER)return 10;
    long other=first==CLASS_SORCERER?second:first;
    if(other==CLASS_MAGE)return 6;
    if(other==CLASS_DROW||other==CLASS_ELEM||other==CLASS_VAMPIRE)return 4;
    return 3;
}

static void learned_colors(int focus,int rank,int secondary)
{
    if(secondary) {
        assert(hybrid_secondary_specialize(&pc.hybrid,CLASS_SORCERER,focus));
        for(int i=0;i<6;i++)
            assert(hybrid_secondary_rank_write(&pc.hybrid,CLASS_SORCERER,
                    HYBRID_TRAIN_COLOR,i,i==focus?rank:(rank>3?3:rank)));
    } else {
        for(int i=0;i<6;i++)pc.colors[i]=rank;
    }
}

static void paired(void)
{
    int cases=0;
    for(size_t p=0;p<10;p++)for(size_t q=0;q<10;q++)if(p!=q)
        for(int start=0;start<3;start++)for(int n=1;n<=12;n++) {
            long cap=expected(parents[p],parents[q]),native[7],hybrid[7];
            fixture(parents[p],0,n<cap?n:(int)cap,start,0);cast(native);
            char text[sizeof(output)];memcpy(text,output,sizeof(text));
            fixture(parents[p],parents[q],n,start,0);
            assert(hybrid_book_chain_limit(&actor)==cap);
            cast(hybrid);
            assert(!memcmp(native,hybrid,sizeof(native)));
            assert(!strcmp(text,output));
            assert(chainspell==(n<cap?n:cap));
            ++cases;
        }
    printf("hybrid books: %d real mixed damage/affect/action prefix replays passed\n",cases);
}

static void trained_replays(void)
{
    const int ranks[]={0,1,3,5};
    int cases=0;
    for(size_t p=0;p<10;p++)if(parents[p]!=CLASS_SORCERER)
        for(int primary=0;primary<2;primary++)for(int focus=0;focus<6;focus++)
        for(size_t r=0;r<4;r++)for(int start=0;start<3;start++)
        for(int n=1;n<=12;n++) {
            long cap=expected(CLASS_SORCERER,parents[p]),native[7],hybrid[7];
            fixture(CLASS_SORCERER,0,n<cap?n:(int)cap,start,0);
            learned_colors(focus,ranks[r],0);
            if(!primary)for(int i=0;i<6;i++)
                if(i!=focus&&pc.colors[i]>3)pc.colors[i]=3;
            cast(native);
            char text[sizeof(output)];memcpy(text,output,sizeof(text));
            fixture(primary?CLASS_SORCERER:parents[p],
                    primary?parents[p]:CLASS_SORCERER,n,start,0);
            learned_colors(focus,ranks[r],!primary);
            for(int i=0;i<6;i++) {
                long want=!primary&&i!=focus&&ranks[r]>3?3:ranks[r];
                assert(hybrid_book_color_rank(&actor,i)==want);
                if(!primary)assert(get_color(&actor,i)==0);
            }
            cast(hybrid);
            if(memcmp(native,hybrid,sizeof(native))||strcmp(text,output)) {
                fprintf(stderr,"Trained replay mismatch parent%ld primary%d focus%d rank%d start%d n%d\n",
                        parents[p],primary,focus,ranks[r],start,n);
                assert(0);
            }
            assert(chainspell==(n<cap?n:cap));
            ++cases;
        }
    printf("hybrid book colors: %d learned-bank/native-prefix replays passed\n",cases);
}

static void color_boundaries(void)
{
    long v[7];
    fixture(CLASS_DROW,CLASS_SORCERER,1,0,1);
    learned_colors(BLUE_MAGIC,5,1);
    assert(hybrid_book_color_rank(&actor,BLUE_MAGIC)==5);
    assert(hybrid_book_color_rank(&actor,RED_MAGIC)==3);
    assert(!hybrid_secondary_rank_write(&pc.hybrid,CLASS_SORCERER,
                                       HYBRID_TRAIN_COLOR,RED_MAGIC,4));
    pc.hybrid.secondary_progression.colors[RED_MAGIC]=4;
    /* Structurally valid legacy training is capped on read, never rewritten. */
    assert(hybrid_player_save_valid(&actor));
    assert(!hybrid_secondary_ranks_valid(&pc.hybrid));
    assert(hybrid_book_color_rank(&actor,RED_MAGIC)==3);
    assert(hybrid_book_color_rank(&actor,BLUE_MAGIC)==5);
    assert(pc.hybrid.secondary_progression.colors[RED_MAGIC]==4);
    pc.hybrid.secondary_progression.colors[RED_MAGIC]=6;
    assert(!hybrid_player_save_valid(&actor));
    assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    cast(v);assert(actor.hit==1&&actor.mana==1000000&&chainspell==0);
    fixture(CLASS_DROW,CLASS_SORCERER,1,0,1);
    for(int i=0;i<6;i++)pc.colors[i]=5; /* Cannot borrow the original raw array. */
    assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    cast(v);assert(actor.hit==1&&actor.mana==999600);
    fixture(CLASS_DROW,CLASS_SORCERER,1,0,1);
    learned_colors(BLUE_MAGIC,5,1);
    pc.hybrid_present=0;assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    pc.hybrid_present=2;assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    fixture(CLASS_DROW,CLASS_SORCERER,1,0,1);
    learned_colors(BLUE_MAGIC,5,1);
    actor.class=CLASS_MAGE;assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    fixture(CLASS_SORCERER,0,1,0,1);
    pc.player_load_failed=1;assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    fixture(CLASS_SORCERER,0,1,0,1);
    const long bad[]={-1,LONG_MIN,6,LONG_MAX};
    unsigned char before[sizeof(actor)],bank[sizeof(pc)];
    memcpy(before,&actor,sizeof(actor));memcpy(bank,&pc,sizeof(pc));
    init_mm();long next=number_range(1,1000000);init_mm();
    for(size_t i=0;i<4;i++)assert(!hybrid_book_color_rank(&actor,bad[i]));
    assert(number_range(1,1000000)==next);
    assert(!memcmp(before,&actor,sizeof(actor))&&!memcmp(bank,&pc,sizeof(pc)));
    assert(!hybrid_book_color_rank(NULL,BLUE_MAGIC));
    actor.pcdata=NULL;assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    SET_BIT(actor.act,ACT_IS_NPC);assert(!hybrid_book_color_rank(&actor,BLUE_MAGIC));
    actor.pcdata=&pc;REMOVE_BIT(actor.act,ACT_IS_NPC);
    fixture(CLASS_SORCERER,CLASS_MAGE,1,0,1);
    learned_colors(BLUE_MAGIC,5,0);
    assert(!hybrid_live_secondary(&actor)); /* Original access is not gated by this. */
    for(int i=0;i<6;i++)assert(hybrid_book_color_rank(&actor,i)==5);
    actor.level=LEV_AWA;chainspell=0;cast(v);
    assert(actor.mana==1000000&&chainspell==0);
}

static void resets_and_invalid(void)
{
    fixture(CLASS_DROW,CLASS_SORCERER,12,0,1);
    assert(hybrid_secondary_specialize(&pc.hybrid,CLASS_SORCERER,BLUE_MAGIC));
    assert(hybrid_secondary_rank_write(&pc.hybrid,CLASS_SORCERER,
                                       HYBRID_TRAIN_COLOR,BLUE_MAGIC,5));
    assert(hybrid_player_save_valid(&actor));
    assert(hybrid_power_rank(&actor,CLASS_SORCERER,HYBRID_TRAIN_COLOR,BLUE_MAGIC)==5);
    assert(get_color(&actor,BLUE_MAGIC)==0); /* No native color permission grant. */
    assert(hybrid_book_chain_limit(&actor)==4);
    actor.style[0]=STYLE_TWOWEAPON;
    assert(hybrid_book_chain_limit(&actor)==4); /* No equipment/stance budget swap. */
    fixture(CLASS_SORCERER,CLASS_MONK,12,0,1);
    long v[7];cast(v);long mana=actor.mana;
    cast(v);assert(actor.mana==mana-3*400&&chainspell==3);
    unsigned char chcopy[sizeof(actor)],pccopy[sizeof(pc)];
    memcpy(chcopy,&actor,sizeof(actor));memcpy(pccopy,&pc,sizeof(pc));
    init_mm();long next=number_range(1,1000000);init_mm();
    assert(hybrid_book_chain_limit(&actor)==3);
    assert(number_range(1,1000000)==next);
    assert(!memcmp(chcopy,&actor,sizeof(actor))&&!memcmp(pccopy,&pc,sizeof(pc)));
    const long corrupt[]={2,-1,LONG_MAX};
    for(size_t i=0;i<3;i++) {
        fixture(CLASS_SORCERER,CLASS_MONK,12,0,1);
        pc.hybrid_present=corrupt[i];assert(hybrid_book_chain_limit(&actor)==0);
        cast(v);assert(actor.hit==1&&actor.mana==1000000&&chainspell==0);
    }
    fixture(CLASS_SORCERER,CLASS_MAGE,12,0,1);
    assert(!hybrid_live_secondary(&actor)); /* Unselected build is NOT a bypass. */
    assert(hybrid_book_chain_limit(&actor)==6);
    pc.hybrid.hybrid_id=LONG_MAX;assert(!hybrid_book_chain_limit(&actor));
    fixture(CLASS_SORCERER,CLASS_MONK,12,0,1);
    actor.class=CLASS_MAGE;assert(!hybrid_book_chain_limit(&actor));
    fixture(CLASS_SORCERER,0,12,0,1);
    pc.player_load_failed=1;assert(!hybrid_book_chain_limit(&actor));
    fixture(CLASS_SORCERER,CLASS_MAGE,12,0,1);
    actor.level=LEV_AWA;chainspell=0;cast(v);
    assert(actor.mana==1000000&&chainspell==0); /* Existing interpreter gate. */
    fixture(CLASS_SORCERER,CLASS_MONK,12,0,1);
    pages[0].specpower=1;cast(v); /* Self-referencing chain is still bounded. */
    assert(chainspell==3&&actor.mana==1000000-3*400);
    assert(!hybrid_book_chain_limit(NULL));
    actor.pcdata=NULL;assert(!hybrid_book_chain_limit(&actor));actor.pcdata=&pc;
    SET_BIT(actor.act,ACT_IS_NPC);actor.pcdata=NULL;
    assert(hybrid_book_chain_limit(&actor)==10); /* Existing NPC budget, no record. */
    REMOVE_BIT(actor.act,ACT_IS_NPC);actor.pcdata=&pc;
    const long counts[]={-1,LONG_MIN,3,LONG_MAX};
    void(*functions[])(CHAR_DATA *,OBJ_DATA *,OBJ_DATA *,char *,int)=
        {adv_spell_damage,adv_spell_affect,adv_spell_action};
    for(size_t k=0;k<3;k++)for(size_t i=0;i<4;i++) {
        fixture(CLASS_SORCERER,CLASS_MONK,12,0,1);
        chainspell=counts[i];pages[0].level=LONG_MAX;
        memcpy(chcopy,&actor,sizeof(actor));memcpy(pccopy,&pc,sizeof(pc));
        init_mm();next=number_range(1,1000000);init_mm();
        functions[k](&actor,&book,&pages[0],"alpha",-1);
        assert(chainspell==counts[i]&&number_range(1,1000000)==next);
        assert(!memcmp(chcopy,&actor,sizeof(actor))&&!memcmp(pccopy,&pc,sizeof(pc)));
        functions[k](&actor,&book,NULL,"alpha",-1);
    }
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")) {native_dump(1);return 0;}
    if(argc==2&&!strcmp(argv[1],"--cap-probe")) {
        fixture(CLASS_SORCERER,CLASS_DRAGON,12,0,1);long v[7];cast(v);
        fprintf(stderr,"Sorcerer/Dragon: mana spent %ld; approved maximum %d\n",
                1000000-actor.mana,3*400);
        assert(actor.mana==1000000-3*400);
        return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--secondary-probe")) {
        fixture(CLASS_DROW,CLASS_SORCERER,1,0,1);
        learned_colors(BLUE_MAGIC,5,1);
        assert(hybrid_power_rank(&actor,CLASS_SORCERER,HYBRID_TRAIN_COLOR,BLUE_MAGIC)==5);
        assert(get_color(&actor,BLUE_MAGIC)==0);
        long v[7];cast(v);
        fprintf(stderr,"Learned secondary blue5: HP %ld; mana spent %ld\n",
                actor.hit,1000000-actor.mana);
        assert(actor.hit>1&&actor.mana==999600);
        return 0;
    }
    /* Captured from the unmodified native Sorcerer on9b3885e:210 executed
     * interpreter/page/cost/effect/RNG snapshots, not a new-code oracle. */
    assert(native_dump(0)==UINT64_C(0x85c64faa8bc58818));
    paired();trained_replays();color_boundaries();resets_and_invalid();
    while(actor.affected)affect_remove(&actor,actor.affected);
    puts("hybrid book limits: fixed3/4/6 both orders, native10, reset/invalid/RNG checks passed");
    return 0;
}
