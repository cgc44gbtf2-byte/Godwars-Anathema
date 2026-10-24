/* Real engine, synthetic characters and disposable codec streams only. */
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "classes/hybrid/state.h"
#include "classes/hybrid/specialization.h"

static CHAR_DATA actor;
static PC_DATA player;

static const struct expectation {
    long owner;
    size_t count;
} expected[] = {
    {CLASS_WEREWOLF,3}, {CLASS_WEREWOLF,5}, {CLASS_WEREWOLF,20},
    {CLASS_WEREWOLF,3}, {CLASS_WEREWOLF,1}, {CLASS_SORCERER,6},
    {CLASS_HIGHLANDER,4}, {CLASS_NINJA,5}, {CLASS_MONK,4},
    {CLASS_DROW,2}, {CLASS_DRAGON,1}, {CLASS_DRAGON,1},
    {CLASS_MAGE,9}, {CLASS_MAGE,3}, {CLASS_MAGE,1},
    {CLASS_MAGE,1}, {CLASS_MAGE,1}
};
_Static_assert(sizeof(expected)/sizeof(expected[0]) == HYBRID_PROGRESS_KIND_COUNT,
               "independent inventory");

/* Explicit native mapping, independent of the production metadata offsets. */
static long *native(enum hybrid_progression_kind kind)
{
    switch (kind) {
    case HYBRID_PROGRESS_BREED: return player.breed;
    case HYBRID_PROGRESS_AUSPICE: return player.auspice;
    case HYBRID_PROGRESS_TRIBE: return player.tribes;
    case HYBRID_PROGRESS_SELECTED_GIFT: return player.tgift;
    case HYBRID_PROGRESS_WOLF_RANK: return &player.rank;
    case HYBRID_PROGRESS_COLOR: return player.colors;
    case HYBRID_PROGRESS_HIGHLAND: return player.highland;
    case HYBRID_PROGRESS_NINJA: return player.ninja;
    case HYBRID_PROGRESS_MONK: return player.monk;
    case HYBRID_PROGRESS_DROW: return player.drow;
    case HYBRID_PROGRESS_DRAGON_COLOR: return &player.dragcol;
    case HYBRID_PROGRESS_DRAGON_SKILLS: return &player.dragskills;
    case HYBRID_PROGRESS_SPHERE: return actor.spheres;
    case HYBRID_PROGRESS_QUINT: return actor.quint;
    case HYBRID_PROGRESS_ARETE: return &actor.arete;
    case HYBRID_PROGRESS_PARADOX: return &actor.paradox;
    case HYBRID_PROGRESS_PARADOX_WARD: return &actor.paradox_ward;
    default: assert(0); return NULL;
    }
}

static void fresh(long parent)
{
    memset(&actor, 0, sizeof(actor));
    memset(&player, 0, sizeof(player));
    actor.class = parent;
    actor.pcdata = &player;
    actor.level = LEV_AWA;
    for (int k = 0; k < HYBRID_PROGRESS_KIND_COUNT; ++k)
        for (size_t i = 0; i < expected[k].count; ++i)
            native((enum hybrid_progression_kind)k)[i] = 1000 + k * 100 + (long)i;
}

static void roundtrip(const HYBRID_STATE *state)
{
    HYBRID_STATE loaded;
    long before[HYBRID_PROGRESSION_WORDS], after[HYBRID_PROGRESSION_WORDS];
    assert(hybrid_progression_export(&state->secondary_progression, before,
                                     HYBRID_PROGRESSION_WORDS));
    FILE *fp = tmpfile();
    assert(fp && hybrid_state_write(fp, state));
    assert(fputs("NEXT\n", fp) >= 0 && fflush(fp) == 0);
    rewind(fp);
    char header[32];
    assert(fgets(header, sizeof(header), fp));
    assert(!strncmp(header, "AnathemaHybrid 4 ", strlen("AnathemaHybrid 4 ")));
    rewind(fp);
    memset(&loaded, 0xa5, sizeof(loaded));
    assert(hybrid_state_read(fp, &loaded) && hybrid_state_valid(&loaded));
    assert(loaded.hybrid_id == state->hybrid_id &&
           loaded.primary_class == state->primary_class &&
           loaded.secondary_class == state->secondary_class);
    assert(hybrid_progression_export(&loaded.secondary_progression, after,
                                     HYBRID_PROGRESSION_WORDS));
    assert(!memcmp(before, after, sizeof(before)));
    char next[16];
    assert(fgets(next, sizeof(next), fp) && !strcmp(next, "NEXT\n"));
    assert(fclose(fp) == 0);
}

static void all_pairs(void)
{
    for (size_t pair = 0; pair < hybrid_count(); ++pair) {
        const HYBRID_DEFINITION *h = hybrid_at(pair);
        for (size_t order = 0; order < 2; ++order) {
            HYBRID_STATE state, before;
            unsigned char a[sizeof(actor)], p[sizeof(player)];
            fresh(h->parents[order]);
            memcpy(a, &actor, sizeof(actor)); memcpy(p, &player, sizeof(player));
            assert(hybrid_state_prepare(&actor, h, &state));
            for (int k = 0; k < HYBRID_PROGRESS_KIND_COUNT; ++k) {
                enum hybrid_progression_kind kind = (enum hybrid_progression_kind)k;
                for (size_t i = 0; i <= expected[k].count; ++i) {
                    long output = -999;
                    int own_primary = expected[k].owner == state.primary_class &&
                                      i < expected[k].count;
                    int own_secondary = expected[k].owner == state.secondary_class &&
                                        i < expected[k].count;
                    assert(hybrid_parent_progression_read(
                        &actor, &state, state.primary_class, kind, i, &output)
                           == own_primary);
                    assert(output == (own_primary ? native(kind)[i] : -999));
                    memcpy(&before, &state, sizeof(state));
                    assert(!hybrid_secondary_progression_write(
                        &state, state.primary_class, kind, i, 1));
                    assert(!memcmp(&before, &state, sizeof(state)));
                    output = -999;
                    assert(hybrid_parent_progression_read(
                        &actor, &state, state.secondary_class, kind, i, &output)
                           == own_secondary);
                    long empty = (kind == HYBRID_PROGRESS_SELECTED_GIFT ||
                                  kind == HYBRID_PROGRESS_DRAGON_COLOR) ? -1 : 0;
                    assert(output == (own_secondary ? empty : -999));
                    int writable = own_secondary &&
                        !(kind == HYBRID_PROGRESS_MONK && i == 3) &&
                        !(kind == HYBRID_PROGRESS_TRIBE && i > TRIBE_WENDIGOS);
                    assert(hybrid_secondary_progression_write(
                        &state, state.secondary_class, kind, i, 1) == writable);
                    if (!writable) assert(!memcmp(&before, &state, sizeof(state)));
                    output = -999;
                    assert(hybrid_parent_progression_read(
                        &actor, &state, state.secondary_class, kind, i, &output)
                           == own_secondary);
                    assert(output == (writable ? 1 : (own_secondary ? empty : -999)));
                    assert(!memcmp(a, &actor, sizeof(actor)));
                    assert(!memcmp(p, &player, sizeof(player)));
                }
            }
            roundtrip(&state);
            for (int k = -1; k <= HYBRID_PROGRESS_KIND_COUNT; ++k) {
                long output = -999;
                memcpy(&before, &state, sizeof(state));
                assert(!hybrid_secondary_progression_write(
                    &state, state.secondary_class, (enum hybrid_progression_kind)k,
                    (size_t)-1, 1));
                assert(!hybrid_parent_progression_read(
                    &actor, &state, state.secondary_class,
                    (enum hybrid_progression_kind)k, (size_t)-1, &output));
                assert(output == -999 && !memcmp(&before, &state, sizeof(state)));
            }
        }
    }
}

static size_t family_count(int family)
{
    return family == 0 ? 3 : (family == 1 ? 5 : TRIBE_WENDIGOS + 1);
}

static void gift_policy(void)
{
    for (size_t pair = 0; pair < hybrid_count(); ++pair) {
        const HYBRID_DEFINITION *h = hybrid_at(pair);
        if (!hybrid_has_parent(h, CLASS_WEREWOLF)) continue;
        long other = h->parents[h->parents[0] == CLASS_WEREWOLF ? 1 : 0];
        for (int chosen = 0; chosen < 3; ++chosen) {
            HYBRID_STATE state, before;
            long value;
            fresh(other);
            assert(hybrid_state_prepare(&actor, h, &state));
            for (int family = 0; family < 3; ++family) {
                assert(hybrid_secondary_progression_write(
                    &state, CLASS_WEREWOLF, HYBRID_PROGRESS_SELECTED_GIFT, family, 0));
                assert(hybrid_parent_gift_training_cap(
                    &actor, &state, CLASS_WEREWOLF,
                    (enum hybrid_gift_family)family, 0, &value) && value == 3);
                assert(!hybrid_secondary_rank_write(
                    &state, CLASS_WEREWOLF, (enum hybrid_training_tree)family, 0, 4));
            }
            assert(hybrid_secondary_gift_choose(
                &state, CLASS_WEREWOLF, (enum hybrid_gift_family)chosen));
            for (int family = 0; family < 3; ++family) {
                for (size_t i = 0; i < family_count(family); ++i) {
                    long cap = family == chosen ? (i == 0 ? 5 : 4) : 3;
                    assert(hybrid_parent_gift_training_cap(
                        &actor, &state, CLASS_WEREWOLF,
                        (enum hybrid_gift_family)family, i, &value) && value == cap);
                    assert(hybrid_secondary_rank_write(
                        &state, CLASS_WEREWOLF, (enum hybrid_training_tree)family, i, cap));
                    memcpy(&before, &state, sizeof(state));
                    assert(!hybrid_secondary_rank_write(
                        &state, CLASS_WEREWOLF,
                        (enum hybrid_training_tree)family, i, cap+1));
                    assert(!memcmp(&before, &state, sizeof(state)));
                    assert(hybrid_parent_gift_rank(
                        &actor, &state, CLASS_WEREWOLF,
                        (enum hybrid_gift_family)family, i, &value) && value == cap);
                }
            }
            assert(hybrid_secondary_gift_choose(
                &state, CLASS_WEREWOLF, (enum hybrid_gift_family)chosen));
            memcpy(&before, &state, sizeof(state));
            assert(!hybrid_secondary_gift_choose(
                &state, CLASS_WEREWOLF, (enum hybrid_gift_family)((chosen+1)%3)));
            assert(!memcmp(&before, &state, sizeof(state)));
            assert(hybrid_secondary_progression_write(
                &state, CLASS_WEREWOLF, HYBRID_PROGRESS_SELECTED_GIFT, chosen, 1));
            assert(hybrid_parent_gift_rank(&actor,&state,CLASS_WEREWOLF,
                   (enum hybrid_gift_family)chosen,0,&value) && value==4);
            /* Raw ranks survive a cap-lowering selection, but effective use
             * cannot bypass that new cap. Raw storage is not permission. */
            assert(!hybrid_secondary_ranks_valid(&state));
            assert(hybrid_secondary_rank_write(&state,CLASS_WEREWOLF,
                   (enum hybrid_training_tree)chosen,0,4));
            assert(hybrid_secondary_ranks_valid(&state));
            roundtrip(&state);
            /* Original WW retains native ranks/getters, not secondary ceilings. */
            fresh(CLASS_WEREWOLF);
            for (int family = 0; family < 3; ++family) player.tgift[family] = 0;
            assert(hybrid_state_prepare(&actor, h, &state));
            for (int family = 0; family < 3; ++family) {
                for (size_t i = 0; i < family_count(family); ++i) {
                    assert(hybrid_parent_gift_rank(
                        &actor, &state, CLASS_WEREWOLF,
                        (enum hybrid_gift_family)family, i, &value) && value == 5);
                    assert(hybrid_parent_gift_training_cap(
                        &actor, &state, CLASS_WEREWOLF,
                        (enum hybrid_gift_family)family, i, &value) &&
                           value == (i == 0 ? 5 : 4));
                }
            }
            player.breed[1] = -1;
            assert(hybrid_parent_gift_rank(&actor, &state, CLASS_WEREWOLF,
                                           HYBRID_GIFT_BREED, 1, &value) &&
                   value == get_breed(&actor, 1));
        }
    }
}

static void reject_record(const char *text, size_t length)
{
    HYBRID_STATE destination, before;
    memset(&destination, 0x5a, sizeof(destination));
    memcpy(&before, &destination, sizeof(before));
    FILE *fp = tmpfile();
    assert(fp && fwrite(text, 1, length, fp) == length && fflush(fp) == 0);
    rewind(fp);
    assert(!hybrid_state_read(fp, &destination));
    assert(!memcmp(&before, &destination, sizeof(before)));
    assert(fclose(fp) == 0);
}

static void migrations_and_codec(void)
{
    for (int version = 1; version <= 2; ++version) {
        for (size_t pair = 0; pair < hybrid_count(); ++pair) {
            const HYBRID_DEFINITION *h = hybrid_at(pair);
            for (size_t order = 0; order < 2; ++order) {
                HYBRID_STATE state;
                HYBRID_PROGRESSION empty;
                long a[HYBRID_PROGRESSION_WORDS], b[HYBRID_PROGRESSION_WORDS];
                FILE *fp = tmpfile();
                assert(fp && fprintf(fp,"AnathemaHybrid %d %ld %ld %ld",
                                      version,h->id,h->parents[order],h->parents[1-order]) > 0);
                for (int i = 0; i < 22; ++i) assert(fputs(" 0", fp) >= 0);
                if (version == 2) {
                    long secondary = h->parents[1-order];
                    int branches = secondary == CLASS_MAGE || secondary == CLASS_VAMPIRE;
                    assert(fprintf(fp," 0 %d %d %d 0 0 0 0 0 0",
                                   branches ? -1 : 0, branches ? -1 : 0,
                                   branches ? -1 : 0) > 0);
                }
                assert(fputs("\nNEXT\n", fp) >= 0 && fflush(fp) == 0);
                rewind(fp);
                assert(hybrid_state_read(fp, &state) && hybrid_state_valid(&state));
                assert(hybrid_progression_prepare(state.secondary_class, &empty));
                assert(hybrid_progression_export(&empty,a,HYBRID_PROGRESSION_WORDS));
                assert(hybrid_progression_export(&state.secondary_progression,b,
                                                 HYBRID_PROGRESSION_WORDS));
                assert(!memcmp(a,b,sizeof(a)));
                char next[16];
                assert(fgets(next,sizeof(next),fp) && !strcmp(next,"NEXT\n"));
                assert(fclose(fp) == 0);
                roundtrip(&state);
            }
        }
    }
    /* Nonempty v2 payload retains old banks/core, not just empty records. */
    FILE *legacy = tmpfile();
    assert(legacy && fputs("AnathemaHybrid 2 45 2 1",legacy)>=0);
    for(int i=0;i<20;++i) assert(fprintf(legacy," %d",i+1)>0);
    assert(fputs(" 0 0 13 19 4 -1 37 53 41 97 31 42\n",legacy)>=0);
    assert(fflush(legacy)==0);
    rewind(legacy);
    HYBRID_STATE old;
    assert(hybrid_state_read(legacy,&old) && fclose(legacy)==0);
    assert(old.secondary_generation==13 && old.secondary_rage==37 &&
           old.secondary_speed_timer==53 && old.secondary_branches[0]==19 &&
           old.secondary_branches[1]==4 && old.secondary_branches[2]==-1 &&
           old.secondary_blood[0]==41 && old.secondary_blood[1]==97 &&
           old.secondary_beast==31 && old.secondary_truebeast==42);
    for(int i=0;i<20;++i)assert(old.secondary_powers[i]==i+1);
    roundtrip(&old);
    HYBRID_STATE state;
    fresh(CLASS_WEREWOLF);
    assert(hybrid_state_prepare(&actor,hybrid_by_id(35),&state));
    assert(state.secondary_class == CLASS_MAGE);
    assert(hybrid_secondary_progression_write(&state,CLASS_MAGE,
                                              HYBRID_PROGRESS_PARADOX,0,LONG_MIN));
    assert(hybrid_secondary_progression_write(&state,CLASS_MAGE,
                                              HYBRID_PROGRESS_QUINT,0,LONG_MAX));
    assert(hybrid_secondary_progression_write(&state,CLASS_MAGE,
                                              HYBRID_PROGRESS_QUINT,1,123456789));
    assert(hybrid_secondary_progression_write(&state,CLASS_MAGE,
                                              HYBRID_PROGRESS_QUINT,2,987654321));
    assert(hybrid_secondary_progression_write(&state,CLASS_MAGE,
                                              HYBRID_PROGRESS_ARETE,0,10));
    assert(hybrid_secondary_progression_write(&state,CLASS_MAGE,
                                              HYBRID_PROGRESS_PARADOX_WARD,0,53));
    roundtrip(&state);
    FILE *fp = tmpfile();
    assert(fp && hybrid_state_write(fp,&state) && fflush(fp)==0);
    rewind(fp);
    char good[4096], changed[8192];
    assert(fgets(good,sizeof(good),fp) && fclose(fp)==0);
    size_t length = strlen(good);
    for (size_t i=0;i<length;++i) reject_record(good,i);
    for (size_t i=0;i<length;++i) {
        memcpy(changed,good,length);
        changed[i]=(char)((unsigned char)changed[i]^0x80);
        reject_record(changed,length);
    }
    snprintf(changed,sizeof(changed),"%.*s 0\n",(int)length-1,good);
    reject_record(changed,strlen(changed));
    memcpy(changed,good,length);
    changed[strlen("AnathemaHybrid ")]='5';
    reject_record(changed,length);
    strcpy(changed,"AnathemaHybrid 3 ");
    memset(changed+strlen(changed),' ',5000);
    changed[5016]='\n';
    reject_record(changed,5017);
    /* Owned progression from a different secondary must not load. */
    state.secondary_progression.colors[0]=1;
    assert(!hybrid_state_valid(&state));
    fp=tmpfile();
    assert(fp && !hybrid_state_write(fp,&state) && ftell(fp)==0 && fclose(fp)==0);
}

static void native_boundaries(void)
{
    const struct {
        long owner;
        enum hybrid_progression_kind kind;
        size_t index;
        long good, bad;
    } cases[]={
        {CLASS_SORCERER,HYBRID_PROGRESS_COLOR,5,5,6},
        {CLASS_HIGHLANDER,HYBRID_PROGRESS_HIGHLAND,3,6,7},
        {CLASS_MONK,HYBRID_PROGRESS_MONK,2,8,9},
        {CLASS_MONK,HYBRID_PROGRESS_MONK,3,0,1},
        {CLASS_DROW,HYBRID_PROGRESS_DROW,DROW_SPEED,4,5},
        {CLASS_DROW,HYBRID_PROGRESS_DROW,DROW_POWERS,DROW_MAX|DROW_GLOBE,131072},
        {CLASS_DRAGON,HYBRID_PROGRESS_DRAGON_COLOR,0,BLACK_MAGIC,BLACK_MAGIC+1},
        {CLASS_DRAGON,HYBRID_PROGRESS_DRAGON_SKILLS,0,DRAG_SHIELD|DRAG_WINGSOUT,8192},
        {CLASS_MAGE,HYBRID_PROGRESS_SPHERE,8,5,6},
        {CLASS_MAGE,HYBRID_PROGRESS_ARETE,0,10,11},
        {CLASS_MAGE,HYBRID_PROGRESS_QUINT,2,LONG_MAX,-1},
        {CLASS_MAGE,HYBRID_PROGRESS_PARADOX_WARD,0,LONG_MAX,-1},
        {CLASS_WEREWOLF,HYBRID_PROGRESS_WOLF_RANK,0,5,6},
        {CLASS_WEREWOLF,HYBRID_PROGRESS_SELECTED_GIFT,0,BREED_MAX,BREED_MAX+1},
        {CLASS_WEREWOLF,HYBRID_PROGRESS_SELECTED_GIFT,1,AUSPICE_AHROUN,AUSPICE_AHROUN+1},
        {CLASS_WEREWOLF,HYBRID_PROGRESS_SELECTED_GIFT,2,TRIBE_WENDIGOS,TRIBE_WENDIGOS+1},
        {CLASS_WEREWOLF,HYBRID_PROGRESS_TRIBE,19,0,1},
    };
    for(size_t i=0;i<sizeof(cases)/sizeof(cases[0]);++i){
        HYBRID_STATE state,before;
        fresh(CLASS_VAMPIRE);
        assert(hybrid_state_prepare(&actor,
               hybrid_by_parents(CLASS_VAMPIRE,cases[i].owner),&state));
        assert(hybrid_secondary_progression_write(&state,cases[i].owner,
               cases[i].kind,cases[i].index,cases[i].good));
        long output=-999;
        assert(hybrid_parent_progression_read(&actor,&state,cases[i].owner,
               cases[i].kind,cases[i].index,&output) && output==cases[i].good);
        memcpy(&before,&state,sizeof(state));
        assert(!hybrid_secondary_progression_write(&state,cases[i].owner,
               cases[i].kind,cases[i].index,cases[i].bad));
        assert(!memcmp(&before,&state,sizeof(state)));
        roundtrip(&state);
    }
}

static void rejects_and_boundaries(void)
{
    HYBRID_STATE state, before;
    fresh(CLASS_VAMPIRE);
    assert(hybrid_state_prepare(&actor,hybrid_by_id(41),&state)); /* Ninja secondary */
    const struct {enum hybrid_progression_kind kind;size_t index;long value;} bad[]={
        {HYBRID_PROGRESS_NINJA,0,11},{HYBRID_PROGRESS_NINJA,2,-1},
        {HYBRID_PROGRESS_NINJA,NINJA_KI,301},
        {HYBRID_PROGRESS_NINJA,NINJA_HARAKIRI,-1},
        {HYBRID_PROGRESS_COLOR,0,1},
        {(enum hybrid_progression_kind)-1,0,1},
        {HYBRID_PROGRESS_KIND_COUNT,0,1}
    };
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i){
        memcpy(&before,&state,sizeof(state));
        assert(!hybrid_secondary_progression_write(&state,CLASS_NINJA,
                                                   bad[i].kind,bad[i].index,bad[i].value));
        assert(!memcmp(&before,&state,sizeof(state)));
    }
    assert(hybrid_secondary_progression_write(&state,CLASS_NINJA,
                                              HYBRID_PROGRESS_NINJA,0,10));
    assert(hybrid_secondary_progression_write(&state,CLASS_NINJA,
                                              HYBRID_PROGRESS_NINJA,NINJA_KI,300));
    assert(hybrid_secondary_progression_write(&state,CLASS_NINJA,
                                              HYBRID_PROGRESS_NINJA,NINJA_HARAKIRI,LONG_MAX));
    roundtrip(&state);
    long output=-999;
    memcpy(&before,&state,sizeof(state));
    assert(!hybrid_parent_progression_read(NULL,&state,CLASS_NINJA,
                                           HYBRID_PROGRESS_NINJA,0,&output));
    assert(!hybrid_parent_progression_read(&actor,NULL,CLASS_NINJA,
                                           HYBRID_PROGRESS_NINJA,0,&output));
    assert(!hybrid_parent_progression_read(&actor,&state,CLASS_NINJA,
                                           HYBRID_PROGRESS_NINJA,0,NULL));
    actor.class=CLASS_MAGE;
    assert(!hybrid_parent_progression_read(&actor,&state,CLASS_NINJA,
                                           HYBRID_PROGRESS_NINJA,0,&output));
    actor.class=state.primary_class;
    SET_BIT(actor.act,ACT_IS_NPC);
    assert(!hybrid_parent_progression_read(&actor,&state,CLASS_NINJA,
                                           HYBRID_PROGRESS_NINJA,0,&output));
    REMOVE_BIT(actor.act,ACT_IS_NPC);
    actor.pcdata=NULL;
    assert(!hybrid_parent_progression_read(&actor,&state,CLASS_NINJA,
                                           HYBRID_PROGRESS_NINJA,0,&output));
    actor.pcdata=&player;
    assert(output==-999 && !memcmp(&before,&state,sizeof(state)));
    assert(!hybrid_secondary_progression_write(NULL,CLASS_NINJA,
                                               HYBRID_PROGRESS_NINJA,0,1));
    assert(!hybrid_secondary_gift_choose(&state,CLASS_WEREWOLF,HYBRID_GIFT_BREED));
    assert(!hybrid_secondary_gift_choose(NULL,CLASS_WEREWOLF,HYBRID_GIFT_BREED));
    assert(!hybrid_parent_gift_rank(&actor,&state,CLASS_WEREWOLF,
                                    HYBRID_GIFT_BREED,0,&output));
    assert(!hybrid_parent_gift_training_cap(&actor,&state,CLASS_WEREWOLF,
                                            HYBRID_GIFT_BREED,0,&output));
    HYBRID_PROGRESSION record, original;
    assert(hybrid_progression_prepare(CLASS_WEREWOLF,&record));
    memcpy(&original,&record,sizeof(record));
    long values[HYBRID_PROGRESSION_WORDS];
    assert(hybrid_progression_export(&record,values,HYBRID_PROGRESSION_WORDS));
    assert(!hybrid_progression_export(&record,values,HYBRID_PROGRESSION_WORDS-1));
    assert(!hybrid_progression_import(CLASS_WEREWOLF,values,
                                      HYBRID_PROGRESSION_WORDS-1,&record));
    assert(!hybrid_progression_import(0,values,HYBRID_PROGRESSION_WORDS,&record));
    assert(!memcmp(&record,&original,sizeof(record)));
    values[HYBRID_PROGRESSION_WORDS-1]=LONG_MAX;
    assert(!hybrid_progression_import(CLASS_WEREWOLF,values,HYBRID_PROGRESSION_WORDS,&record));
    assert(!memcmp(&record,&original,sizeof(record)));
}

int main(void)
{
    all_pairs();
    gift_policy();
    migrations_and_codec();
    native_boundaries();
    rejects_and_boundaries();
    puts("hybrid progression: all 45 pairs/both orders, 17 owned banks, "
         "secondary WW caps/native preservation and v1/v2/v3 codec tests passed");
    return 0;
}
