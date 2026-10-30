/* Real interpreter and room views; independently trained sources share Scry. */
#define SCRY_FIXTURE_ONLY
#include "test_hybrid_scry.c"

static const long ds_originals[]={CLASS_DRAGON,CLASS_ELEM,CLASS_HIGHLANDER,
    CLASS_MAGE,CLASS_MONK,CLASS_NINJA,CLASS_SORCERER,CLASS_VAMPIRE,CLASS_WEREWOLF};

static void ds_fixture(long primary,long secondary,int learned,int native)
{
    scry_fixture(primary,secondary,0,-1,native);
    if(primary==CLASS_DROW) {
        if(learned)SET_BIT(pcs[0].drow[DROW_POWERS],DROW_EYES);
        else REMOVE_BIT(pcs[0].drow[DROW_POWERS],DROW_EYES);
    } else SET_BIT(pcs[0].drow[DROW_POWERS],DROW_EYES); /* Unowned stale data. */
    if(secondary==CLASS_DROW)
        pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=learned?DROW_EYES:0;
}

static void ds_pairs(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int order=0;order<2;order++)
    for(int learned=0;learned<2;learned++)for(int native=0;native<4;native++)
    for(int mode=0;mode<12;mode++)for(int m=0;m<3;m++){
        long primary=order?CLASS_DROW:ds_originals[p],secondary=order?ds_originals[p]:CLASS_DROW;
        ds_fixture(primary,secondary,learned,native);actors[0].mana=mana_values[m];
        SNAPSHOT actual,expected;scry_run(actors,mode,&actual);
        if(learned&&!order) {
            /* Existing independently learned Monk route on the same original
             * preserves that original's native warning/shield exceptions. */
            scry_fixture(primary,primary==CLASS_MONK?0:CLASS_MONK,2,MONK_MANTRA,native);
            SET_BIT(pcs[0].drow[DROW_POWERS],DROW_EYES);
        } else ds_fixture(primary,0,learned,native);
        actors[0].mana=mana_values[m];scry_run(actors,mode,&expected);
        if(memcmp(&actual,&expected,sizeof actual)) {
            fprintf(stderr,"Drow Scry p=%u order=%d learned=%d native=%d mode=%d mana=%d\n",
                p,order,learned,native,mode,m);
            abort();
        }
        ++count;
    }
    assert(count==5184);
}

static void ds_bad_records(void)
{
    unsigned count=0;
    for(unsigned p=0;p<9;p++)for(int native=0;native<4;native++)
    for(int bad=0;bad<6;bad++){
        ds_fixture(ds_originals[p],CLASS_DROW,1,native);
        switch(bad){
        case 0:pcs[0].hybrid_present=0;break;
        case 1:pcs[0].hybrid_present=2;break;
        case 2:pcs[0].player_load_failed=1;break;
        case 3:pcs[0].hybrid.hybrid_id=0;break;
        case 4:pcs[0].hybrid.secondary_progression.drow[DROW_POWERS]=-1;break;
        case 5:pcs[0].hybrid.secondary_class=CLASS_MONK;break;
        }
        SNAPSHOT actual,expected;scry_run(actors,0,&actual);
        ds_fixture(ds_originals[p],0,0,native);scry_run(actors,0,&expected);
        assert(!memcmp(&actual,&expected,sizeof actual));++count;
    }
    assert(count==216);
}

static void ds_sources_and_loss(void)
{
    ds_fixture(CLASS_MONK,CLASS_DROW,1,0);pcs[0].monk[MONK_MANTRA]=2;
    SNAPSHOT both,one;scry_run(actors,0,&both);
    scry_fixture(CLASS_MONK,0,2,-1,0);scry_run(actors,0,&one);
    assert(!memcmp(&both,&one,sizeof both));
    ds_fixture(CLASS_MAGE,CLASS_DROW,1,0);actors[0].mana=LONG_MIN;
    scry_run(actors,0,&one);
    assert(strstr(outputs[0],"Remote scry fixture")&&actors[0].mana==LONG_MIN);
    ds_fixture(CLASS_MAGE,CLASS_DROW,1,0);pcs[0].hybrid_present=0;
    scry_run(actors,0,&one);assert(strstr(outputs[0],"mind sphere"));
    ds_fixture(CLASS_MAGE,CLASS_DROW,1,1);pcs[0].hybrid_present=0;
    actors[0].mana=500;scry_run(actors,0,&one);
    assert(strstr(outputs[0],"Remote scry fixture")&&actors[0].mana==0);
    ds_fixture(CLASS_SORCERER,CLASS_DROW,1,0);
    scry_run(actors,4,&one);
    assert(strstr(outputs[0],"unable to locate")&&strstr(outputs[1],"failed to use Scry"));
    ds_fixture(CLASS_WEREWOLF,CLASS_DROW,1,3);
    scry_run(actors,4,&one);assert(!strstr(outputs[1],"Scry"));
}

int main(int argc,char **argv)
{
    if(argc==2&&!strcmp(argv[1],"--native-dump")){scry_native(1);return 0;}
    scry_native(0);ds_pairs();ds_bad_records();ds_sources_and_loss();
    for(int i=0;i<9;i++)scry_safety(i);
    puts("Drow Scry:1440 native snapshots,5184 paired commands,216 rejected-record "
         "fallback cases,6 source/privacy/loss cases and9 safety boundaries passed.");
    return 0;
}
