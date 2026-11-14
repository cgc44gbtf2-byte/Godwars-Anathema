/* Real interpreter and native sight engine, not permission stubs. */
#define HEIGHTEN_FIXTURE_ONLY
#include "test_hybrid_heighten.c"

static const long dh_originals[]={CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};

static void dh_fixture(long primary,long secondary,int learned,int grant,int monk)
{
    ht_fixture(primary,secondary,monk,MONK_MANTRA,grant);
    if(primary==CLASS_DROW)
        pcs[0].drow[DROW_POWERS]=learned?DROW_SIGHT:0;
    else SET_BIT(pcs[0].drow[DROW_POWERS],DROW_SIGHT); /* Not an owned grant. */
    if(secondary==CLASS_DROW)
        pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=learned?DROW_SIGHT:0;
}

static void dh_pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int learned=0;learned<2;learned++)for(int grant=0;grant<3;grant++)
    for(int monk=0;monk<3;monk++)for(int active=0;active<2;active++)
    for(int vision=0;vision<2;vision++)for(int argument=0;argument<2;argument++){
        long primary=order?CLASS_DROW:dh_originals[p],secondary=order?dh_originals[p]:CLASS_DROW;
        dh_fixture(primary,secondary,learned,grant,monk);
        int ready=hybrid_power_drow(actors,DROW_SIGHT)||
            hybrid_power_rank(actors,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA)>=1;
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        if(vision)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        SNAPSHOT actual,expected;ht_run(actors,argument,&actual);
        dh_fixture(ready?CLASS_MONK:primary,0,0,grant,ready?1:monk);
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        if(vision)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        ht_run(actors,argument,&expected);
        if(memcmp(&actual,&expected,sizeof actual)){
            fprintf(stderr,"Drow Heighten p=%u order=%d learned=%d grant=%d monk=%d active=%d vision=%d arg=%d\n",
                p,order,learned,grant,monk,active,vision,argument);
            abort();
        }
        ++count;
    }
    assert(count==2592);
}

static void dh_bad_records(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int grant=0;grant<3;grant++)
    for(int bad=0;bad<6;bad++)for(int active=0;active<2;active++){
        dh_fixture(dh_originals[p],CLASS_DROW,1,grant,0);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.hybrid_id=0;break;
        case 4:pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
        case 5:pcs[0].hybrid.secondary_class=CLASS_MONK;break;
        }
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        SNAPSHOT actual,expected;ht_run(actors,0,&actual);
        dh_fixture(dh_originals[p],0,0,grant,0);
        if(active)SET_BIT(actors[0].act,PLR_HOLYLIGHT);
        ht_run(actors,0,&expected);
        assert(!memcmp(&actual,&expected,sizeof actual));++count;
    }
    assert(count==324);
}

static void dh_sources_and_loss(void)
{
    for(int independent=0;independent<2;independent++){
        dh_fixture(independent?CLASS_SORCERER:CLASS_MONK,CLASS_DROW,1,independent?2:0,1);
        if(independent)SET_BIT(actors[0].extra,EXTRA_SORC_VISION);
        actors[0].mana=LONG_MIN;
        SNAPSHOT s;ht_run(actors,0,&s);
        assert(IS_SET(actors[0].act,PLR_HOLYLIGHT)&&actors[0].wait==6);
        assert(actors[0].mana==LONG_MIN);
        /* Lose both learned sources; an existing toggle can still be dismissed. */
        pcs[0].monk[MONK_MANTRA]=0;pcs[0].hybrid_present=0;
        actors[0].wait=0;ht_run(actors,0,&s);
        assert(!IS_SET(actors[0].act,PLR_HOLYLIGHT)&&actors[0].wait==0);
        assert(!!effective_holylight(actors)==independent);
        assert(!!IS_SET(actors[0].extra,EXTRA_SORC_HOLY)==independent);
        ht_run(actors,0,&s);
        assert(!IS_SET(actors[0].act,PLR_HOLYLIGHT));
    }
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){ht_native(1);return 0;}
    ht_native(0);dh_pairs();dh_bad_records();dh_sources_and_loss();
    puts("Drow Heighten:1428 native snapshots,2592 paired commands,324 rejected-record "
         "fallback/dismissal cases,2 shared-source/lost-training/independent-vision cases passed.");
    return 0;
}
