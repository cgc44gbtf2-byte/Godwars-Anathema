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

static int valid_profile(unsigned int grants, unsigned int compatible)
{
    return known_features(grants) && known_features(compatible) &&
           !(grants & ~compatible);
}

static unsigned int component_count(const HYBRID_FORM_STATE *state)
{
    return !!state->components[0].token + !!state->components[1].token;
}

static HYBRID_FORM_COMPONENT empty_component(void)
{
    HYBRID_FORM_COMPONENT result = {0, 0, HYBRID_FEATURE_ALL};
    return result;
}

static HYBRID_FUSION_PROFILE empty_fusion(void)
{
    HYBRID_FUSION_PROFILE result = {0, 0, HYBRID_FEATURE_ALL};
    return result;
}

int hybrid_forms_valid(const HYBRID_FORM_STATE *state)
{
    const HYBRID_DEFINITION *definition;
    unsigned int count;
    if (!state) return 0;
    definition = hybrid_by_id(state->hybrid_id);
    if (!definition || state->parents[0] == state->parents[1] ||
        !hybrid_has_parent(definition, state->parents[0]) ||
        !hybrid_has_parent(definition, state->parents[1]) ||
        !known_features(state->requested[0]) ||
        !known_features(state->requested[1])) return 0;
    for (int slot = 0; slot < 2; ++slot) {
        const HYBRID_FORM_COMPONENT *c = &state->components[slot];
        if (!valid_profile(c->grants, c->compatible)) return 0;
        if (!c->token && (c->grants || c->compatible != HYBRID_FEATURE_ALL))
            return 0;
    }
    count = component_count(state);
    if (!valid_profile(state->fusion.grants, state->fusion.compatible))
        return 0;
    if (count == 2) {
        if (state->fusion.hybrid_id != state->hybrid_id) return 0;
    } else if (state->fusion.hybrid_id || state->fusion.grants ||
               state->fusion.compatible != HYBRID_FEATURE_ALL) return 0;
    /* Leave one serial available for every component that can still end. */
    return !count || (state->activation_serial != 0 &&
                     state->activation_serial <= ULONG_MAX - count);
}

enum hybrid_body_mode hybrid_forms_mode(const HYBRID_FORM_STATE *state)
{
    if (!hybrid_forms_valid(state)) return HYBRID_BODY_INVALID;
    return (enum hybrid_body_mode)component_count(state);
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
    candidate.components[0] = empty_component();
    candidate.components[1] = empty_component();
    candidate.fusion = empty_fusion();
    if (!hybrid_forms_valid(&candidate)) return 0;
    *destination = candidate;
    return 1;
}

unsigned long hybrid_form_begin(HYBRID_FORM_STATE *state, long parent,
                                unsigned int logical_token,
                                unsigned int grants, unsigned int compatible)
{
    int slot;
    if (hybrid_forms_mode(state) != HYBRID_BODY_NORMAL ||
        (slot = parent_slot(state, parent)) < 0 || !logical_token ||
        !valid_profile(grants, compatible) ||
        state->activation_serial > ULONG_MAX - 2) return 0;
    state->components[slot] =
        (HYBRID_FORM_COMPONENT){logical_token, grants, compatible};
    return ++state->activation_serial;
}

unsigned long hybrid_form_fuse(HYBRID_FORM_STATE *state, long parent,
                               unsigned int logical_token,
                               unsigned int grants, unsigned int compatible,
                               const HYBRID_FUSION_PROFILE *profile)
{
    int slot;
    if (hybrid_forms_mode(state) != HYBRID_BODY_SINGLE || !profile ||
        (slot = parent_slot(state, parent)) < 0 ||
        state->components[slot].token || !logical_token ||
        !valid_profile(grants, compatible) ||
        profile->hybrid_id != state->hybrid_id ||
        !valid_profile(profile->grants, profile->compatible) ||
        state->activation_serial > ULONG_MAX - 3) return 0;
    state->components[slot] =
        (HYBRID_FORM_COMPONENT){logical_token, grants, compatible};
    state->fusion = *profile;
    return ++state->activation_serial;
}

int hybrid_form_end(HYBRID_FORM_STATE *state, long parent,
                    unsigned int logical_token, unsigned long activation)
{
    int slot;
    if (!hybrid_forms_valid(state) || !logical_token ||
        (slot = parent_slot(state, parent)) < 0 ||
        state->components[slot].token != logical_token ||
        state->activation_serial != activation)
        return 0;
    state->components[slot] = empty_component();
    state->fusion = empty_fusion();
    ++state->activation_serial;
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
    HYBRID_FORM_STATE candidate;
    int slot;
    if (!hybrid_forms_valid(state) ||
        (slot = parent_slot(state, parent)) < 0) return 0;
    candidate = *state;
    candidate.requested[slot] = 0;
    if (candidate.components[slot].token &&
        !hybrid_form_end(&candidate, parent, candidate.components[slot].token,
                         candidate.activation_serial)) return 0;
    *state = candidate;
    return 1;
}

unsigned int hybrid_features_effective(const HYBRID_FORM_STATE *state)
{
    unsigned int grants = 0, compatible = HYBRID_FEATURE_ALL;
    if (!hybrid_forms_valid(state)) return 0;
    if (component_count(state) == 2) {
        /* Fusion is one explicit profile, NOT the union of parent grants. */
        grants = state->fusion.grants;
        compatible = state->fusion.compatible;
    } else {
        for (int slot = 0; slot < 2; ++slot) {
            if (state->components[slot].token) {
                grants = state->components[slot].grants;
                compatible = state->components[slot].compatible;
            }
        }
    }
    return (state->requested[0] | state->requested[1] | grants) & compatible;
}
