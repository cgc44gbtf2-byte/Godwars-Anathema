/* Real shared Read Aura handler: supplement, never replace, native access. */
#define main monk_aura_fixture_main
#include "test_hybrid_aura.c"
#undef main

static const long originals[]={CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};

static void drow_fixture(long primary,long secondary,int learned,int native)
{
    fixture(primary,secondary,0,-1,native);
    /* Unowned legacy arrays must not supply secondary grants. */
    if(primary==CLASS_DROW)pcs[0].drow[DROW_POWERS]=learned?DROW_READ:0;
    else pcs[0].drow[DROW_POWERS]|=DROW_READ;
    if(secondary==CLASS_DROW)
        pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=learned?DROW_READ:0;
}

static void drow_pairs(void)
{
    unsigned pairs=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int learned=0;learned<2;learned++)for(int native=0;native<4;native++)
    for(int mode=0;mode<8;mode++)for(int m=0;m<3;m++){
        long primary=order?CLASS_DROW:originals[p],secondary=order?originals[p]:CLASS_DROW;
        drow_fixture(primary,secondary,learned,native);
        actors[0].mana=mana_values[m];
        if(mode==2)SET_BIT(actors[0].extra,EXTRA_FAKE_CON);
        SNAPSHOT actual,expected;run(actors,mode,&actual);
        drow_fixture(learned?CLASS_DROW:primary,0,learned,native);
        /* Guardian-warning stealth is still owned by the original Werewolf
         * Homid5, not the aura source. Compare that one overlap against the
         * existing independently learned Monk route on the same original. */
        if(learned&&primary==CLASS_WEREWOLF&&native==3&&mode==7)
            fixture(CLASS_WEREWOLF,CLASS_MONK,2,MONK_MANTRA,native);
        actors[0].mana=mana_values[m];
        if(mode==2)SET_BIT(actors[0].extra,EXTRA_FAKE_CON);
        run(actors,mode,&expected);
        if(memcmp(&actual,&expected,sizeof actual)){
            fprintf(stderr,"Drow aura pair p=%u order=%d learned=%d native=%d mode=%d mana=%d\n",
                    p,order,learned,native,mode,m);
            for(unsigned n=0;n<12;n++)fprintf(stderr,"%u:%ld/%ld ",n,actual.values[n],expected.values[n]);
            fprintf(stderr,"\ntext=%" PRIx64 "/%" PRIx64 "\n",actual.text,expected.text);
            abort();
        }
        ++pairs;
    }
    assert(pairs==3456);
}

static void drow_bad_records(void)
{
    unsigned cases=0;
    for(unsigned p=0;p<9;p++)for(int native=0;native<4;native++)
    for(int bad=0;bad<6;bad++){
        drow_fixture(originals[p],CLASS_DROW,1,native);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.hybrid_id=0;break;
        case 4:pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
        case 5:pcs[0].hybrid.secondary_class=CLASS_MONK;break;
        }
        SNAPSHOT actual,expected;run(actors,0,&actual);
        drow_fixture(originals[p],0,0,native);run(actors,0,&expected);
        assert(!memcmp(&actual,&expected,sizeof actual));++cases;
    }
    assert(cases==216);
    /* Two valid grants still produce exactly one reading, at no mana cost. */
    drow_fixture(CLASS_MONK,CLASS_DROW,1,0);pcs[0].monk[MONK_MANTRA]=2;
    SNAPSHOT both,one;run(actors,0,&both);
    fixture(CLASS_MONK,0,2,-1,0);run(actors,0,&one);
    assert(!memcmp(&both,&one,sizeof both));
    drow_fixture(CLASS_MAGE,CLASS_DROW,1,0);actors[0].mana=LONG_MIN;
    run(actors,0,&one);assert(strstr(outputs[0],"Hp:")&&actors[0].mana==LONG_MIN);
    drow_fixture(CLASS_WEREWOLF,CLASS_DROW,1,3);run(actors,1,&one);
    assert(strstr(outputs[0],"Object 'relic'"));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){native_cases(1);return 0;}
    native_cases(0);drow_pairs();drow_bad_records();
    puts("Drow Read Aura:960 native snapshots,3456 paired commands,"
         "216 rejected-record/native fallback cases and3 independent overlaps passed.");
    return 0;
}
