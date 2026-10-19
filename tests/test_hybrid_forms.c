/* Synthetic policy tests, not live form-handler or combat integration tests. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/classes/class_ids.h"
#include "../Genesis/src/classes/hybrid/forms.h"

static HYBRID_FORM_STATE fresh(size_t index, size_t order)
{
    HYBRID_FORM_STATE state;
    const HYBRID_DEFINITION *h = hybrid_at(index);
    memset(&state, 0xa5, sizeof(state));
    assert(hybrid_forms_prepare(h, h->parents[order], &state));
    assert(hybrid_forms_valid(&state));
    assert(state.parents[0] == h->parents[order]);
    assert(state.parents[1] == h->parents[1 - order]);
    assert(!state.requested[0] && !state.requested[1]);
    assert(!state.form_token && !state.activation_serial);
    assert(!hybrid_features_effective(&state));
    return state;
}

static void equal(const HYBRID_FORM_STATE *a, const HYBRID_FORM_STATE *b)
{
    assert(a->hybrid_id == b->hybrid_id);
    assert(!memcmp(a->parents, b->parents, sizeof(a->parents)));
    assert(!memcmp(a->requested, b->requested, sizeof(a->requested)));
    assert(a->form_parent == b->form_parent && a->form_token == b->form_token);
    assert(a->form_grants == b->form_grants && a->compatible == b->compatible);
    assert(a->activation_serial == b->activation_serial);
}

static void all_pairs(void)
{
    for (size_t i = 0; i < hybrid_count(); ++i) {
        for (size_t order = 0; order < 2; ++order) {
            HYBRID_FORM_STATE state = fresh(i, order), before;
            long first = state.parents[0], second = state.parents[1];
            assert(hybrid_feature_request(&state, first, HYBRID_FEATURE_CLAWS, 1));
            assert(hybrid_feature_request(&state, second, HYBRID_FEATURE_FANGS, 1));
            assert(hybrid_features_effective(&state) == HYBRID_FEATURE_ALL);
            unsigned long activation = hybrid_form_begin(
                &state, first, 10, HYBRID_FEATURE_ALL, HYBRID_FEATURE_ALL);
            assert(activation == 1);
            before = state;
            assert(!hybrid_form_begin(&state, first, 10, 0, HYBRID_FEATURE_ALL));
            equal(&state, &before);
            assert(!hybrid_form_begin(&state, first, 11, 0, HYBRID_FEATURE_ALL));
            equal(&state, &before);
            assert(!hybrid_form_begin(&state, second, 12, 0, HYBRID_FEATURE_ALL));
            equal(&state, &before);
            assert(!hybrid_form_end(&state, second, 10, activation));
            equal(&state, &before);
            assert(!hybrid_form_end(&state, first, 12, activation));
            equal(&state, &before);
            assert(!hybrid_form_end(&state, first, 10, 0));
            equal(&state, &before);
            assert(hybrid_feature_request(&state, second, HYBRID_FEATURE_CLAWS, 1));
            assert(hybrid_form_end(&state, first, 10, activation));
            assert(hybrid_features_effective(&state) == HYBRID_FEATURE_ALL);
            assert(hybrid_forms_release_parent(&state, first));
            assert(hybrid_features_effective(&state) == HYBRID_FEATURE_ALL);
            assert(hybrid_forms_release_parent(&state, second));
            assert(hybrid_features_effective(&state) == 0);

            /* Noncorporeal profile suspends requests, never erases them. */
            assert(hybrid_feature_request(&state, first, HYBRID_FEATURE_CLAWS, 1));
            assert(hybrid_feature_request(&state, second, HYBRID_FEATURE_FANGS, 1));
            activation = hybrid_form_begin(&state, first, 20, 0, 0);
            assert(activation && hybrid_features_effective(&state) == 0);
            assert(state.requested[0] == HYBRID_FEATURE_CLAWS);
            assert(state.requested[1] == HYBRID_FEATURE_FANGS);
            assert(hybrid_form_end(&state, first, 20, activation));
            assert(hybrid_features_effective(&state) == HYBRID_FEATURE_ALL);

            /* A delayed teardown must not end a later identical form. */
            unsigned long later = hybrid_form_begin(&state, first, 20, 0, 0);
            assert(later > activation);
            before = state;
            assert(!hybrid_form_end(&state, first, 20, activation));
            equal(&state, &before);
            assert(hybrid_form_end(&state, first, 20, later));

            /* Remove only one parent's requests, not another owner's form. */
            activation = hybrid_form_begin(&state, second, 30,
                                            HYBRID_FEATURE_CLAWS,
                                            HYBRID_FEATURE_ALL);
            assert(activation);
            assert(hybrid_forms_release_parent(&state, first));
            assert(state.form_parent == second && state.form_token == 30);
            assert(hybrid_features_effective(&state) == HYBRID_FEATURE_ALL);
            assert(hybrid_forms_release_parent(&state, second));
            assert(state.form_token == 0 && hybrid_features_effective(&state) == 0);
            assert(hybrid_forms_valid(&state));
        }
    }
}

static void invalid_state(HYBRID_FORM_STATE state)
{
    HYBRID_FORM_STATE before = state;
    assert(!hybrid_forms_valid(&state));
    assert(!hybrid_features_effective(&state));
    assert(!hybrid_form_begin(&state, state.parents[0], 1, 0, HYBRID_FEATURE_ALL));
    equal(&state, &before);
    assert(!hybrid_form_end(&state, state.parents[0], 1, 1));
    equal(&state, &before);
    assert(!hybrid_feature_request(&state, state.parents[0],
                                   HYBRID_FEATURE_CLAWS, 1));
    equal(&state, &before);
    assert(!hybrid_forms_release_parent(&state, state.parents[0]));
    equal(&state, &before);
}

static void invalid(void)
{
    HYBRID_FORM_STATE base = fresh(44, 0), state;
    state = base; state.hybrid_id = 0; invalid_state(state);
    state = base; state.hybrid_id = LONG_MAX; invalid_state(state);
    state = base; state.parents[1] = state.parents[0]; invalid_state(state);
    state = base; state.parents[1] = CLASS_MAGE; invalid_state(state);
    state = base; state.requested[0] = UINT_MAX; invalid_state(state);
    state = base; state.compatible = 0; invalid_state(state);
    state = base; state.form_grants = HYBRID_FEATURE_CLAWS; invalid_state(state);
    state = base; state.form_parent = state.parents[0]; invalid_state(state);
    state = base; state.form_token = 1; invalid_state(state);
    state = base; state.form_token = 1; state.form_parent = state.parents[0];
    invalid_state(state); /* active token requires an activation serial */
    assert(!hybrid_forms_valid(NULL) && !hybrid_features_effective(NULL));
    assert(!hybrid_form_begin(NULL, CLASS_VAMPIRE, 1, 0, HYBRID_FEATURE_ALL));
    assert(!hybrid_form_end(NULL, CLASS_VAMPIRE, 1, 1));
    assert(!hybrid_feature_request(NULL, CLASS_VAMPIRE, HYBRID_FEATURE_CLAWS, 1));
    assert(!hybrid_forms_release_parent(NULL, CLASS_VAMPIRE));
    state = base;
    assert(!hybrid_forms_prepare(NULL, CLASS_VAMPIRE, &state));
    assert(!hybrid_forms_prepare(hybrid_by_id(45), CLASS_VAMPIRE, NULL));
    assert(!hybrid_forms_prepare(hybrid_by_id(45), CLASS_DROW, &state));
    HYBRID_DEFINITION forged = *hybrid_by_id(45);
    assert(!hybrid_forms_prepare(&forged, CLASS_VAMPIRE, &state));
    equal(&state, &base);
    assert(!hybrid_form_begin(&state, -1, 1, 0, HYBRID_FEATURE_ALL));
    assert(!hybrid_form_begin(&state, state.parents[0], 0, 0, HYBRID_FEATURE_ALL));
    assert(!hybrid_form_begin(&state, state.parents[0], 1, UINT_MAX,
                             HYBRID_FEATURE_ALL));
    assert(!hybrid_form_begin(&state, state.parents[0], 1,
                             HYBRID_FEATURE_CLAWS, HYBRID_FEATURE_FANGS));
    assert(!hybrid_feature_request(&state, -1, HYBRID_FEATURE_CLAWS, 1));
    assert(!hybrid_feature_request(&state, state.parents[0], 0, 1));
    assert(!hybrid_feature_request(&state, state.parents[0], UINT_MAX, 1));
    assert(!hybrid_forms_release_parent(&state, -1));
    equal(&state, &base);
    state.activation_serial = ULONG_MAX;
    HYBRID_FORM_STATE before = state;
    assert(hybrid_forms_valid(&state));
    assert(!hybrid_form_begin(&state, state.parents[0], 1, 0, HYBRID_FEATURE_ALL));
    equal(&state, &before); /* refuse serial wrap, never reuse zero/old serial */
}

static uint32_t next(uint32_t *seed)
{
    *seed ^= *seed << 13;
    *seed ^= *seed >> 17;
    *seed ^= *seed << 5;
    return *seed;
}

static void model_sequences(void)
{
    uint32_t seed = UINT32_C(0xa17a7e);
    for (size_t i = 0; i < hybrid_count(); ++i) {
        for (size_t order = 0; order < 2; ++order) {
            HYBRID_FORM_STATE state = fresh(i, order);
            unsigned int request[2] = {0, 0}, active = 0, profile = 3, grants = 0;
            int owner = -1;
            unsigned long serial = 0;
            for (size_t step = 0; step < 1000; ++step) {
                HYBRID_FORM_STATE before = state;
                unsigned int op = next(&seed) % 4, slot = next(&seed) % 3;
                long parent = slot < 2 ? state.parents[slot] : -1;
                int expected, actual;
                if (op == 0) {
                    unsigned int token = next(&seed) % 4;
                    unsigned int offered = next(&seed) % 8;
                    unsigned int compat = next(&seed) % 8;
                    expected = slot < 2 && !active && token &&
                               offered < 4 && compat < 4 && !(offered & ~compat);
                    unsigned long began = hybrid_form_begin(
                        &state, parent, token, offered, compat);
                    actual = began != 0;
                    if (expected) {
                        active = token; owner = (int)slot;
                        profile = compat; grants = offered;
                        assert(began == ++serial);
                    }
                } else if (op == 1) {
                    unsigned int token = next(&seed) % 4;
                    unsigned long supplied = (next(&seed) & 1) ? serial : 0;
                    expected = active && token == active && (int)slot == owner &&
                               supplied == serial;
                    actual = hybrid_form_end(&state, parent, token, supplied);
                    if (expected) {active = 0; owner = -1; grants = 0; profile = 3;}
                } else if (op == 2) {
                    unsigned int features = next(&seed) % 8;
                    int enabled = (int)(next(&seed) & 1);
                    expected = slot < 2 && features && features < 4;
                    actual = hybrid_feature_request(&state, parent, features, enabled);
                    if (expected) {
                        if (enabled) request[slot] |= features;
                        else request[slot] &= ~features;
                    }
                } else {
                    expected = slot < 2;
                    actual = hybrid_forms_release_parent(&state, parent);
                    if (expected) {
                        request[slot] = 0;
                        if ((int)slot == owner) {
                            active = 0; owner = -1; grants = 0; profile = 3;
                        }
                    }
                }
                assert(!!actual == !!expected);
                if (!expected) equal(&state, &before);
                assert(hybrid_forms_valid(&state));
                assert(state.activation_serial == serial);
                assert(hybrid_features_effective(&state) ==
                       ((request[0] | request[1] | grants) & profile));
            }
        }
    }
}

int main(void)
{
    all_pairs();
    invalid();
    model_sequences();
    puts("hybrid form policy regressions passed (90000 modeled operations)");
    return 0;
}
