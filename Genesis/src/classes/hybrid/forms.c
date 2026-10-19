/* Original Anathema transformation policy; no legacy-handler side effects. */
#include <stddef.h>
#include <limits.h>
#include "forms.h"

static int parent_slot(const HYBRID_FORM_STATE *state, long parent)
{
    if (state->parents[0] == parent) return 0;
    if (state->parents[1] == parent) return 1;
    return -1;
}

static int known_features(unsigned int mask)
{
    return (mask & ~HYBRID_FEATURE_ALL) == 0;
}

int hybrid_forms_valid(const HYBRID_FORM_STATE *state)
{
    const HYBRID_DEFINITION *definition;
    if (!state) return 0;
    definition = hybrid_by_id(state->hybrid_id);
    if (!definition || state->parents[0] == state->parents[1] ||
        !hybrid_has_parent(definition, state->parents[0]) ||
        !hybrid_has_parent(definition, state->parents[1]) ||
        !known_features(state->requested[0]) ||
        !known_features(state->requested[1]) ||
        !known_features(state->form_grants) ||
        !known_features(state->compatible) ||
        (state->form_grants & ~state->compatible)) return 0;
    if (!state->form_token)
        return state->form_parent == 0 && state->form_grants == 0 &&
               state->compatible == HYBRID_FEATURE_ALL;
    return state->activation_serial != 0 &&
           parent_slot(state, state->form_parent) >= 0;
}

int hybrid_forms_prepare(const HYBRID_DEFINITION *definition, long primary,
                         HYBRID_FORM_STATE *destination)
{
    HYBRID_FORM_STATE candidate = {0};
    if (!definition || !destination ||
        hybrid_by_id(definition->id) != definition ||
        !hybrid_has_parent(definition, primary)) return 0;
    candidate.hybrid_id = definition->id;
    candidate.parents[0] = primary;
    candidate.parents[1] = definition->parents[
        definition->parents[0] == primary ? 1 : 0];
    candidate.compatible = HYBRID_FEATURE_ALL;
    if (!hybrid_forms_valid(&candidate)) return 0;
    *destination = candidate;
    return 1;
}

unsigned long hybrid_form_begin(HYBRID_FORM_STATE *state, long parent,
                                unsigned int logical_token,
                                unsigned int grants, unsigned int compatible)
{
    if (!hybrid_forms_valid(state) || parent_slot(state, parent) < 0 ||
        state->form_token || !logical_token || !known_features(grants) ||
        !known_features(compatible) || (grants & ~compatible) ||
        state->activation_serial == ULONG_MAX) return 0;
    /* Refuse replacement, including same-parent replacement. The eventual
     * handler must finish its old-form teardown before starting another. */
    state->form_parent = parent;
    state->form_token = logical_token;
    state->form_grants = grants;
    state->compatible = compatible;
    return ++state->activation_serial;
}

int hybrid_form_end(HYBRID_FORM_STATE *state, long parent,
                    unsigned int logical_token, unsigned long activation)
{
    if (!hybrid_forms_valid(state) || !logical_token ||
        state->form_parent != parent || state->form_token != logical_token ||
        state->activation_serial != activation)
        return 0;
    state->form_parent = 0;
    state->form_token = 0;
    state->form_grants = 0;
    state->compatible = HYBRID_FEATURE_ALL;
    return 1;
}

int hybrid_feature_request(HYBRID_FORM_STATE *state, long parent,
                           unsigned int features, int enabled)
{
    int slot;
    if (!hybrid_forms_valid(state) || !features || !known_features(features) ||
        (slot = parent_slot(state, parent)) < 0) return 0;
    if (enabled) state->requested[slot] |= features;
    else state->requested[slot] &= ~features;
    return 1;
}

int hybrid_forms_release_parent(HYBRID_FORM_STATE *state, long parent)
{
    int slot;
    if (!hybrid_forms_valid(state) ||
        (slot = parent_slot(state, parent)) < 0) return 0;
    state->requested[slot] = 0;
    if (state->form_parent == parent)
        return hybrid_form_end(state, parent, state->form_token,
                               state->activation_serial);
    return 1;
}

unsigned int hybrid_features_effective(const HYBRID_FORM_STATE *state)
{
    if (!hybrid_forms_valid(state)) return 0;
    return (state->requested[0] | state->requested[1] | state->form_grants)
           & state->compatible;
}
