/* Standalone policy tests: NOT live class handlers or game-stat tests. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "classes/hybrid/forms.h"

#define ALL HYBRID_FEATURE_ALL
#define CLAWS HYBRID_FEATURE_CLAWS
#define FANGS HYBRID_FEATURE_FANGS

static HYBRID_FORM_STATE fresh(const HYBRID_DEFINITION *d, int order)
{
    HYBRID_FORM_STATE state;
    memset(&state, 0, sizeof state);
    assert(hybrid_forms_prepare(d, d->parents[order], &state));
    assert(hybrid_forms_valid(&state));
    assert(hybrid_forms_mode(&state) == HYBRID_BODY_NORMAL);
    assert(state.parents[0] == d->parents[order]);
    assert(!state.requested[0] && !state.requested[1]);
    assert(!hybrid_features_effective(&state));
    return state;
}

static int same(const HYBRID_FORM_STATE *a, const HYBRID_FORM_STATE *b)
{
    if (a->hybrid_id != b->hybrid_id ||
        a->activation_serial != b->activation_serial ||
        a->fusion.hybrid_id != b->fusion.hybrid_id ||
        a->fusion.grants != b->fusion.grants ||
        a->fusion.compatible != b->fusion.compatible) return 0;
    for (int n = 0; n < 2; ++n)
        if (a->parents[n] != b->parents[n] ||
            a->requested[n] != b->requested[n] ||
            a->components[n].token != b->components[n].token ||
            a->components[n].grants != b->components[n].grants ||
            a->components[n].compatible != b->components[n].compatible)
            return 0;
    return 1;
}

static void all_pairs(void)
{
    for (size_t i = 0; i < hybrid_count(); ++i) {
        for (int order = 0; order < 2; ++order) {
            for (int first = 0; first < 2; ++first) {
                for (int exit = 0; exit < 2; ++exit) {
                    HYBRID_FORM_STATE s = fresh(hybrid_at(i), order), before;
                    unsigned int grants[2] = {CLAWS, FANGS};
                    unsigned int compatible[2] = {ALL, FANGS};
                    unsigned int tokens[2] = {7, 7}; /* parent-local tokens */
                    HYBRID_FUSION_PROFILE profile = {hybrid_at(i)->id, 0, FANGS};
                    unsigned long first_serial, fused_serial, split_serial;
                    int second = 1 - first, remaining = 1 - exit;
                    assert(hybrid_feature_request(&s, s.parents[0], CLAWS, 1));
                    assert(hybrid_feature_request(&s, s.parents[1], FANGS, 1));
                    first_serial = hybrid_form_begin(&s, s.parents[first],
                        tokens[first], grants[first], compatible[first]);
                    assert(first_serial && hybrid_forms_mode(&s) == HYBRID_BODY_SINGLE);
                    before = s;
                    assert(!hybrid_form_begin(&s, s.parents[second], 7, 0, ALL));
                    assert(same(&s, &before)); /* never silently replace */
                    fused_serial = hybrid_form_fuse(&s, s.parents[second],
                        tokens[second], grants[second], compatible[second], &profile);
                    assert(fused_serial > first_serial);
                    assert(hybrid_forms_mode(&s) == HYBRID_BODY_FUSED);
                    /* Fusion has its own profile: do NOT union parent grants. */
                    assert(hybrid_features_effective(&s) == FANGS);
                    before = s;
                    assert(!hybrid_form_end(&s, s.parents[first], 7, first_serial));
                    assert(!hybrid_form_fuse(&s, s.parents[first], 7, 0, ALL, &profile));
                    assert(same(&s, &before));
                    assert(hybrid_form_end(&s, s.parents[exit], 7, fused_serial));
                    split_serial = s.activation_serial;
                    assert(split_serial > fused_serial);
                    assert(hybrid_forms_mode(&s) == HYBRID_BODY_SINGLE);
                    assert(s.components[remaining].token == 7);
                    assert(!s.components[exit].token);
                    assert(hybrid_features_effective(&s) ==
                           (remaining == 0 ? ALL : FANGS));
                    before = s;
                    assert(!hybrid_form_end(&s, s.parents[remaining], 7, fused_serial));
                    assert(same(&s, &before)); /* pre-split callbacks are stale */
                    assert(hybrid_form_end(&s, s.parents[remaining], 7, split_serial));
                    assert(hybrid_forms_mode(&s) == HYBRID_BODY_NORMAL);
                    assert(hybrid_features_effective(&s) == ALL);
                    assert(hybrid_forms_release_parent(&s, s.parents[0]));
                    assert(hybrid_features_effective(&s) == FANGS);
                    assert(hybrid_forms_release_parent(&s, s.parents[1]));
                    assert(!hybrid_features_effective(&s));
                }
            }
        }
    }
}

static void release_and_requests(void)
{
    const HYBRID_DEFINITION *d = hybrid_at(0);
    HYBRID_FORM_STATE s = fresh(d, 0), before;
    HYBRID_FUSION_PROFILE p = {d->id, FANGS, ALL};
    unsigned long serial;
    assert(hybrid_feature_request(&s, s.parents[0], ALL, 1));
    assert(hybrid_feature_request(&s, s.parents[1], CLAWS, 1));
    assert(hybrid_form_begin(&s, s.parents[0], 1, CLAWS, ALL));
    serial = hybrid_form_fuse(&s, s.parents[1], 2, FANGS, ALL, &p);
    assert(serial && hybrid_features_effective(&s) == ALL);
    assert(hybrid_forms_release_parent(&s, s.parents[0]));
    assert(!s.requested[0] && s.requested[1] == CLAWS);
    assert(hybrid_forms_mode(&s) == HYBRID_BODY_SINGLE);
    assert(hybrid_features_effective(&s) == ALL);
    before = s;
    assert(!hybrid_form_end(&s, s.parents[1], 2, serial));
    assert(same(&s, &before));
    assert(hybrid_form_end(&s, s.parents[1], 2, s.activation_serial));
    assert(hybrid_features_effective(&s) == CLAWS);
    assert(hybrid_forms_release_parent(&s, s.parents[1]));
    assert(!hybrid_features_effective(&s));
}

static void all_feature_profiles(void)
{
    const HYBRID_DEFINITION *d = hybrid_at(0);
    unsigned long cases = 0;
    for (unsigned int g0 = 0; g0 <= ALL; ++g0)
    for (unsigned int c0 = 0; c0 <= ALL; ++c0)
    for (unsigned int g1 = 0; g1 <= ALL; ++g1)
    for (unsigned int c1 = 0; c1 <= ALL; ++c1)
    for (unsigned int gf = 0; gf <= ALL; ++gf)
    for (unsigned int cf = 0; cf <= ALL; ++cf) {
        if ((g0 & ~c0) || (g1 & ~c1) || (gf & ~cf)) continue;
        for (unsigned int r0 = 0; r0 <= ALL; ++r0)
        for (unsigned int r1 = 0; r1 <= ALL; ++r1)
        for (int exit = 0; exit < 2; ++exit) {
            HYBRID_FORM_STATE s = fresh(d, 0);
            HYBRID_FUSION_PROFILE p = {d->id, gf, cf};
            if (r0) assert(hybrid_feature_request(&s, s.parents[0], r0, 1));
            if (r1) assert(hybrid_feature_request(&s, s.parents[1], r1, 1));
            assert(hybrid_form_begin(&s, s.parents[0], 1, g0, c0));
            assert(hybrid_form_fuse(&s, s.parents[1], 2, g1, c1, &p));
            assert(hybrid_features_effective(&s) == ((r0 | r1 | gf) & cf));
            assert(hybrid_form_end(&s, s.parents[exit], exit ? 2 : 1,
                                   s.activation_serial));
            assert(hybrid_features_effective(&s) ==
                   ((r0 | r1 | (exit ? g0 : g1)) & (exit ? c0 : c1)));
            assert(hybrid_form_end(&s, s.parents[1 - exit], exit ? 1 : 2,
                                   s.activation_serial));
            assert(hybrid_features_effective(&s) == (r0 | r1));
            ++cases;
        }
    }
    assert(cases == 23328);
}

static void invalid_inputs(void)
{
    const HYBRID_DEFINITION *d = hybrid_at(0);
    HYBRID_FORM_STATE s = fresh(d, 0), before = s, bad;
    HYBRID_DEFINITION fake = *d;
    HYBRID_FUSION_PROFILE p = {d->id, 0, ALL};
    assert(!hybrid_forms_valid(NULL));
    assert(hybrid_forms_mode(NULL) == HYBRID_BODY_INVALID);
    assert(!hybrid_features_effective(NULL));
    assert(!hybrid_forms_prepare(NULL, 0, &s));
    assert(!hybrid_forms_prepare(&fake, fake.parents[0], &s));
    assert(!hybrid_forms_prepare(d, 0, &s));
    assert(!hybrid_forms_prepare(d, d->parents[0], NULL));
    assert(!hybrid_form_begin(NULL, 0, 1, 0, ALL));
    assert(!hybrid_form_fuse(NULL, 0, 1, 0, ALL, &p));
    assert(!hybrid_form_end(NULL, 0, 1, 1));
    assert(!hybrid_feature_request(NULL, 0, 1, 1));
    assert(!hybrid_forms_release_parent(NULL, 0));
    assert(!hybrid_form_begin(&s, 0, 1, 0, ALL));
    assert(!hybrid_form_begin(&s, s.parents[0], 0, 0, ALL));
    assert(!hybrid_form_begin(&s, s.parents[0], 1, 4, ALL));
    assert(!hybrid_form_begin(&s, s.parents[0], 1, CLAWS, FANGS));
    assert(!hybrid_form_begin(&s, s.parents[0], 1, 0, 4));
    assert(!hybrid_form_fuse(&s, s.parents[0], 1, 0, ALL, &p));
    assert(!hybrid_feature_request(&s, 0, CLAWS, 1));
    assert(!hybrid_feature_request(&s, s.parents[0], 0, 1));
    assert(!hybrid_feature_request(&s, s.parents[0], 4, 1));
    assert(!hybrid_forms_release_parent(&s, 0));
    assert(!hybrid_form_end(&s, s.parents[0], 1, 0));
    assert(same(&s, &before));

    assert(hybrid_form_begin(&s, s.parents[0], 1, CLAWS, ALL));
    before = s;
    assert(!hybrid_form_fuse(&s, s.parents[1], 2, 0, ALL, NULL));
    assert(!hybrid_form_fuse(&s, 0, 2, 0, ALL, &p));
    assert(!hybrid_form_fuse(&s, s.parents[0], 2, 0, ALL, &p));
    assert(!hybrid_form_fuse(&s, s.parents[1], 0, 0, ALL, &p));
    assert(!hybrid_form_fuse(&s, s.parents[1], 2, 4, ALL, &p));
    p.hybrid_id = 0;
    assert(!hybrid_form_fuse(&s, s.parents[1], 2, 0, ALL, &p));
    p.hybrid_id = d->id;
    p.grants = CLAWS; p.compatible = FANGS;
    assert(!hybrid_form_fuse(&s, s.parents[1], 2, 0, ALL, &p));
    p.grants = 0; p.compatible = 4;
    assert(!hybrid_form_fuse(&s, s.parents[1], 2, 0, ALL, &p));
    assert(same(&s, &before));

    bad = s; bad.hybrid_id = 0; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.parents[1] = bad.parents[0]; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.requested[1] = 4; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.components[1].grants = CLAWS; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.components[1].compatible = 0; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.fusion.hybrid_id = d->id; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.fusion.compatible = 0; assert(!hybrid_forms_valid(&bad));
    bad = s; bad.activation_serial = 0; assert(!hybrid_forms_valid(&bad));
    before = bad;
    assert(!hybrid_forms_release_parent(&bad, bad.parents[0]));
    assert(!hybrid_feature_request(&bad, bad.parents[0], CLAWS, 1));
    assert(!hybrid_features_effective(&bad));
    assert(same(&bad, &before));
}

static void serial_exhaustion(void)
{
    const HYBRID_DEFINITION *d = hybrid_at(0);
    HYBRID_FORM_STATE s = fresh(d, 0), before;
    HYBRID_FUSION_PROFILE p = {d->id, 0, ALL};
    s.activation_serial = ULONG_MAX - 4;
    assert(hybrid_form_begin(&s, s.parents[0], 1, CLAWS, ALL) == ULONG_MAX - 3);
    assert(hybrid_form_fuse(&s, s.parents[1], 2, FANGS, ALL, &p) == ULONG_MAX - 2);
    assert(hybrid_form_end(&s, s.parents[0], 1, ULONG_MAX - 2));
    assert(s.activation_serial == ULONG_MAX - 1);
    assert(hybrid_form_end(&s, s.parents[1], 2, ULONG_MAX - 1));
    assert(s.activation_serial == ULONG_MAX && hybrid_forms_valid(&s));
    before = s;
    assert(!hybrid_form_begin(&s, s.parents[0], 1, 0, ALL));
    assert(same(&s, &before));
    s = fresh(d, 0); s.activation_serial = ULONG_MAX - 2;
    assert(hybrid_form_begin(&s, s.parents[0], 1, 0, ALL) == ULONG_MAX - 1);
    before = s;
    assert(!hybrid_form_fuse(&s, s.parents[1], 2, 0, ALL, &p));
    assert(same(&s, &before));
    assert(hybrid_forms_release_parent(&s, s.parents[0]));
    assert(s.activation_serial == ULONG_MAX);
    s = fresh(d, 0); s.activation_serial = ULONG_MAX - 1;
    before = s;
    assert(!hybrid_form_begin(&s, s.parents[0], 1, 0, ALL));
    assert(same(&s, &before));
}

typedef struct model {
    unsigned int token[2], grants[2], compatible[2], requested[2];
    unsigned int fusion_grants, fusion_compatible;
    unsigned long serial;
} MODEL;

static unsigned int random_word(void)
{
    static unsigned int state = 0x927af10bU;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static unsigned int count(const MODEL *m)
{
    return !!m->token[0] + !!m->token[1];
}

static void model_end(MODEL *m, int slot)
{
    m->token[slot] = m->grants[slot] = 0;
    m->compatible[slot] = ALL;
    m->fusion_grants = 0; m->fusion_compatible = ALL;
    ++m->serial;
}

static void compare(const HYBRID_FORM_STATE *s, const MODEL *m)
{
    unsigned int grants = 0, compatible = ALL, n = count(m);
    assert(hybrid_forms_valid(s));
    assert((int)hybrid_forms_mode(s) == (int)n);
    assert(s->activation_serial == m->serial);
    for (int slot = 0; slot < 2; ++slot) {
        assert(s->components[slot].token == m->token[slot]);
        assert(s->components[slot].grants == m->grants[slot]);
        assert(s->components[slot].compatible == m->compatible[slot]);
        assert(s->requested[slot] == m->requested[slot]);
        if (m->token[slot]) {
            grants = m->grants[slot]; compatible = m->compatible[slot];
        }
    }
    assert(s->fusion.grants == m->fusion_grants);
    assert(s->fusion.compatible == m->fusion_compatible);
    assert(s->fusion.hybrid_id == (n == 2 ? s->hybrid_id : 0));
    if (n == 2) {
        grants = m->fusion_grants; compatible = m->fusion_compatible;
    }
    assert(hybrid_features_effective(s) ==
        ((m->requested[0] | m->requested[1] | grants) & compatible));
}

static void model_sequences(void)
{
    unsigned int fusions = 0, splits = 0;
    for (size_t i = 0; i < hybrid_count(); ++i) {
        for (int order = 0; order < 2; ++order) {
            HYBRID_FORM_STATE s = fresh(hybrid_at(i), order);
            MODEL m = {.compatible = {ALL, ALL}, .fusion_compatible = ALL};
            for (unsigned int step = 0; step < 2000; ++step) {
                HYBRID_FORM_STATE before = s;
                unsigned int op = random_word() % 5, slot = random_word() % 3;
                long parent = slot < 2 ? s.parents[slot] : 0;
                unsigned int token = random_word() % 4;
                unsigned int grants = random_word() % 8;
                unsigned int compatible = random_word() % 8;
                int valid = grants <= ALL && compatible <= ALL &&
                            !(grants & ~compatible);
                int expected = 0, actual = 0;
                if (op == 0 || op == 1) {
                    HYBRID_FUSION_PROFILE p = {
                        random_word() & 1 ? s.hybrid_id : 0,
                        random_word() % 8, random_word() % 8};
                    int supplied = (random_word() % 8) != 0;
                    unsigned long serial;
                    if (op == 0) {
                        expected = slot < 2 && !count(&m) && token && valid;
                        serial = hybrid_form_begin(&s, parent, token, grants, compatible);
                    } else {
                        expected = slot < 2 && count(&m) == 1 && !m.token[slot] &&
                            token && valid && supplied && p.hybrid_id == s.hybrid_id &&
                            p.grants <= ALL && p.compatible <= ALL &&
                            !(p.grants & ~p.compatible);
                        serial = hybrid_form_fuse(&s, parent, token, grants, compatible,
                                                   supplied ? &p : NULL);
                    }
                    actual = serial != 0;
                    if (expected) {
                        m.token[slot] = token; m.grants[slot] = grants;
                        m.compatible[slot] = compatible;
                        if (op == 1) {
                            m.fusion_grants = p.grants;
                            m.fusion_compatible = p.compatible;
                            ++fusions;
                        }
                        ++m.serial;
                        assert(serial == m.serial);
                    }
                } else if (op == 2) {
                    unsigned long activation = random_word() % 3 == 0 ?
                        m.serial : (m.serial ? m.serial - 1 : 0);
                    expected = slot < 2 && token && m.token[slot] == token &&
                               activation == m.serial;
                    actual = hybrid_form_end(&s, parent, token, activation);
                    if (expected) {
                        if (count(&m) == 2) ++splits;
                        model_end(&m, (int)slot);
                    }
                } else if (op == 3) {
                    int enabled = random_word() & 1;
                    expected = slot < 2 && grants && grants <= ALL;
                    actual = hybrid_feature_request(&s, parent, grants, enabled);
                    if (expected) {
                        if (enabled) m.requested[slot] |= grants;
                        else m.requested[slot] &= ~grants;
                    }
                } else {
                    expected = slot < 2;
                    actual = hybrid_forms_release_parent(&s, parent);
                    if (expected) {
                        m.requested[slot] = 0;
                        if (m.token[slot]) {
                            if (count(&m) == 2) ++splits;
                            model_end(&m, (int)slot);
                        }
                    }
                }
                assert(actual == expected);
                if (!expected) assert(same(&s, &before));
                compare(&s, &m);
            }
        }
    }
    assert(fusions && splits);
    printf("modeled fusions=%u, splits=%u\n", fusions, splits);
}

int main(void)
{
    assert(hybrid_count() == 45);
    all_pairs();
    release_and_requests();
    all_feature_profiles();
    invalid_inputs();
    serial_exhaustion();
    model_sequences();
    puts("hybrid form policy: 360 ordered flows, 23328 feature-profile cases, 180000 modeled operations passed");
    return 0;
}
