#ifndef ANATHEMA_HYBRID_FORMS_H
#define ANATHEMA_HYBRID_FORMS_H

#include "hybrid.h"

#define HYBRID_FEATURE_CLAWS 1U
#define HYBRID_FEATURE_FANGS 2U
#define HYBRID_FEATURE_ALL (HYBRID_FEATURE_CLAWS | HYBRID_FEATURE_FANGS)

/* Persisted hybrid form ledger, separate from native flags and training.
 * Logical form tokens are supplied by reviewed class adapters: multiple
 * legacy flags may describe one form. This arbitrates logical-body and feature
 * ownership only; it does not alter native flags or authorize class powers.
 * All trained powers remain available in every form. Runtime transformation
 * handlers still need to map native class forms to these logical tokens. */
typedef struct hybrid_form_component {
    unsigned int token;
    unsigned int grants;
    unsigned int compatible;
} HYBRID_FORM_COMPONENT;

/* Future adapters must select a reviewed profile for this exact hybrid.
 * These feature masks are not numerical stat bonuses or power permissions. */
typedef struct hybrid_fusion_profile {
    long hybrid_id;
    unsigned int grants;
    unsigned int compatible;
} HYBRID_FUSION_PROFILE;

typedef struct hybrid_form_state {
    long hybrid_id;
    long parents[2];
    unsigned int requested[2];
    HYBRID_FORM_COMPONENT components[2];
    HYBRID_FUSION_PROFILE fusion;
    unsigned long activation_serial;
} HYBRID_FORM_STATE;

enum hybrid_body_mode {
    HYBRID_BODY_INVALID = -1,
    HYBRID_BODY_NORMAL = 0,
    HYBRID_BODY_SINGLE = 1,
    HYBRID_BODY_FUSED = 2
};

int hybrid_forms_prepare(const HYBRID_DEFINITION *definition, long primary,
                         HYBRID_FORM_STATE *destination);
int hybrid_forms_valid(const HYBRID_FORM_STATE *state);
enum hybrid_body_mode hybrid_forms_mode(const HYBRID_FORM_STATE *state);
/* Begin/fuse return the new nonzero body serial, or zero on failure.
 * Every body transition, including split/end, changes the serial. Future
 * adapters must rebind remaining-form callbacks to the new body serial.
 * The counter reserves enough serials for all exits, so overflow cannot
 * trap an active body or let stale callbacks match a later activation. */
unsigned long hybrid_form_begin(HYBRID_FORM_STATE *state, long parent,
                                unsigned int logical_token,
                                unsigned int grants, unsigned int compatible);
unsigned long hybrid_form_fuse(HYBRID_FORM_STATE *state, long parent,
                               unsigned int logical_token,
                               unsigned int grants, unsigned int compatible,
                               const HYBRID_FUSION_PROFILE *profile);
int hybrid_form_end(HYBRID_FORM_STATE *state, long parent,
                    unsigned int logical_token, unsigned long activation);
int hybrid_feature_request(HYBRID_FORM_STATE *state, long parent,
                           unsigned int features, int enabled);
int hybrid_forms_release_parent(HYBRID_FORM_STATE *state, long parent);
unsigned int hybrid_features_effective(const HYBRID_FORM_STATE *state);

#endif
