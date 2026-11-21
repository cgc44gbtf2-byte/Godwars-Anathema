/* Native build selection, independent of the primary clan and social registry. */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "build.h"
#include "specialization.h"

static const HYBRID_BUILD_DEFINITION builds[] = {
    {1,CLASS_VAMPIRE,"Ventrue",{DISC_DOMINATE,DISC_FORTITUDE,DISC_PRESENCE},0},
    {2,CLASS_VAMPIRE,"Nosferatu",{DISC_ANIMALISM,DISC_OBFUSCATE,DISC_POTENCE},0},
    {3,CLASS_VAMPIRE,"Brujah",{DISC_CELERITY,DISC_POTENCE,DISC_PRESENCE},0},
    {4,CLASS_VAMPIRE,"Assamite",{DISC_CELERITY,DISC_OBFUSCATE,DISC_QUIETUS},0},
    {5,CLASS_VAMPIRE,"Gangrel",{DISC_ANIMALISM,DISC_FORTITUDE,DISC_PROTEAN},0},
    {6,CLASS_VAMPIRE,"Serpents of the Light",{DISC_OBFUSCATE,DISC_PRESENCE,DISC_SERPENTIS},0},
    {7,CLASS_VAMPIRE,"Lasombra",{DISC_DOMINATE,DISC_OBTENEBRATION,DISC_POTENCE},0},
    {8,CLASS_VAMPIRE,"Tzimisce",{DISC_AUSPEX,DISC_ANIMALISM,DISC_VICISSITUDE},0},
    {9,CLASS_VAMPIRE,"Ravnos",{DISC_FORTITUDE,DISC_ANIMALISM,DISC_CHIMERSTRY},0},
    {101,CLASS_MAGE,"Akashic Brotherhood",{SPHERE_MIND,-1,-1},0},
    {102,CLASS_MAGE,"Celestial Chorus",{SPHERE_PRIME,-1,-1},0},
    {103,CLASS_MAGE,"Cult of Ecstasy",{SPHERE_TIME,-1,-1},0},
    {104,CLASS_MAGE,"Dreamspeakers",{SPHERE_SPIRIT,-1,-1},0},
    {105,CLASS_MAGE,"Euthanatos",{SPHERE_ENTROPY,-1,-1},0},
    {106,CLASS_MAGE,"Order of Hermes",{SPHERE_FORCES,-1,-1},0},
    {107,CLASS_MAGE,"Sons of Ether",{SPHERE_MATTER,-1,-1},0},
    {108,CLASS_MAGE,"Verbena",{SPHERE_LIFE,-1,-1},0},
    {109,CLASS_MAGE,"Virtual Adepts",{SPHERE_CORRESPONDENCE,-1,-1},0},
    {110,CLASS_MAGE,"Hollow Ones",{-1,-1,-1},0},
    {201,CLASS_DRAGON,"Bronze",{0,0,0},PURPLE_MAGIC},
    {202,CLASS_DRAGON,"Red",{0,0,0},RED_MAGIC},
    {203,CLASS_DRAGON,"Blue",{0,0,0},BLUE_MAGIC},
    {204,CLASS_DRAGON,"Silver",{0,0,0},GREEN_MAGIC},
    {205,CLASS_DRAGON,"Gold",{0,0,0},YELLOW_MAGIC},
    {206,CLASS_DRAGON,"Black",{0,0,0},BLACK_MAGIC}
};

size_t hybrid_build_count(void) { return sizeof(builds)/sizeof(builds[0]); }
const HYBRID_BUILD_DEFINITION *hybrid_build_at(size_t index)
{
    return index < hybrid_build_count() ? &builds[index] : NULL;
}
const HYBRID_BUILD_DEFINITION *hybrid_build_by_id(long id)
{
    for (size_t i=0;i<hybrid_build_count();++i) if(builds[i].id==id) return &builds[i];
    return NULL;
}

int hybrid_build_record_valid(const HYBRID_STATE *state)
{
    if (!state) return 0;
    if (!state->secondary_build) return 1; /* Old records stay unconfigured. */
    const HYBRID_BUILD_DEFINITION *b=hybrid_build_by_id(state->secondary_build);
    if (!b || b->parent!=state->secondary_class) return 0;
    if (b->parent==CLASS_VAMPIRE)
        return state->secondary_progression.vampire_profile==HYBRID_VAMPIRE_STANDARD &&
            !memcmp(b->branches,state->secondary_branches,sizeof(b->branches));
    if (b->parent==CLASS_DRAGON)
        return state->secondary_progression.dragon_color==b->color;
    if (b->parent==CLASS_MAGE) {
        if (b->id!=110 && state->secondary_branches[0]!=b->branches[0]) return 0;
        for (size_t i=0;i<3;++i)
            for (size_t j=i+1;j<3;++j)
                if(state->secondary_branches[i]>=0 &&
                   state->secondary_branches[i]==state->secondary_branches[j])return 0;
        return 1;
    }
    return 0;
}

int hybrid_secondary_build_choose(HYBRID_STATE *state,long id)
{
    const HYBRID_BUILD_DEFINITION *b=hybrid_build_by_id(id);
    if (!b || !hybrid_state_valid(state) || state->secondary_class!=b->parent) return 0;
    if (state->secondary_build) return state->secondary_build==id;
    HYBRID_STATE candidate=*state;
    if (b->parent==CLASS_VAMPIRE) {
        /* Only fresh UNKNOWN banks. Do not rewrite legacy locked rank0 or
         * silently infer an old player's clan/profile from its raw values. */
        for (size_t i=0;i<20;++i) if(candidate.secondary_powers[i]!=-1)return 0;
        for (size_t i=0;i<3;++i) if(candidate.secondary_branches[i]!=-1)return 0;
        if(candidate.secondary_progression.vampire_profile)return 0;
        static const int basic[] = {DISC_ANIMALISM,DISC_AUSPEX,DISC_CELERITY,
            DISC_DOMINATE,DISC_FORTITUDE,DISC_OBFUSCATE,DISC_POTENCE,DISC_PRESENCE};
        memset(candidate.secondary_powers,0,sizeof(candidate.secondary_powers));
        for(size_t i=0;i<sizeof(basic)/sizeof(basic[0]);++i)
            candidate.secondary_powers[basic[i]]=-1;
        for(size_t i=0;i<3;++i)candidate.secondary_powers[b->branches[i]]=-1;
        memcpy(candidate.secondary_branches,b->branches,sizeof(b->branches));
        candidate.secondary_progression.vampire_profile=HYBRID_VAMPIRE_STANDARD;
    } else if(b->parent==CLASS_MAGE) {
        for(size_t i=0;i<3;++i)if(candidate.secondary_branches[i]!=-1)return 0;
        for(size_t i=0;i<9;++i)if(candidate.secondary_progression.spheres[i])return 0;
        memcpy(candidate.secondary_branches,b->branches,sizeof(b->branches));
    } else {
        if(candidate.secondary_progression.dragon_color!=-1 ||
           candidate.secondary_progression.dragon_skills)return 0;
        candidate.secondary_progression.dragon_color=b->color;
    }
    candidate.secondary_build=id;
    if(!hybrid_state_valid(&candidate))return 0;
    *state=candidate;
    return 1;
}

int hybrid_secondary_gift_select(HYBRID_STATE *state,size_t family,long gift)
{
    static const long maxima[]={BREED_MAX,AUSPICE_AHROUN,TRIBE_WENDIGOS};
    if(!hybrid_state_valid(state)||state->secondary_class!=CLASS_WEREWOLF ||
       family>=3 || gift<0 || gift>maxima[family])return 0;
    long *slot=&state->secondary_progression.selected_gifts[family];
    if(*slot!=-1)return *slot==gift;
    HYBRID_STATE candidate=*state;
    candidate.secondary_progression.selected_gifts[family]=gift;
    if(!hybrid_state_valid(&candidate))return 0;
    *state=candidate;
    return 1;
}

int hybrid_secondary_sphere_select(HYBRID_STATE *state,size_t slot,long sphere)
{
    if(!hybrid_state_valid(state)||state->secondary_class!=CLASS_MAGE ||
       state->secondary_build<101||state->secondary_build>110 ||
       slot>=3 || sphere<0||sphere>SPHERE_TIME ||
       (slot==0&&state->secondary_build!=110))return 0;
    if(state->secondary_branches[slot]!=-1)
        return state->secondary_branches[slot]==sphere;
    HYBRID_STATE candidate=*state;
    candidate.secondary_branches[slot]=sphere;
    if(!hybrid_state_valid(&candidate))return 0;
    *state=candidate;
    return 1;
}
