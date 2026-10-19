#ifndef ANATHEMA_HYBRID_FORMS_H
#define ANATHEMA_HYBRID_FORMS_H

#include "hybrid.h"

#define HYBRID_FEATURE_CLAWS 1U
#define HYBRID_FEATURE_FANGS 2U
#define HYBRID_FEATURE_ALL (HYBRID_FEATURE_CLAWS | HYBRID_FEATURE_FANGS)

/* Transient policy state, NOT a character's legacy flags or training.
 * Logical form tokens are supplied by future reviewed handler adapters:
 * multiple legacy flags can describe one form. This component only arbitrates
 * exclusivity/feature ownership; it does NOT authorize a class power.
 * No live handler, stat, gear, save or combat formula uses this state yet. */
typedef struct hybrid_form_state {
    long hybrid_id;
    long parents[2];
    unsigned int requested[2];
    long form_parent;
    unsigned int form_token;
    unsigned int form_grants;
    unsigned int compatible;
    unsigned long activation_serial;
} HYBRID_FORM_STATE;

int hybrid_forms_prepare(const HYBRID_DEFINITION *definition, long primary,
                         HYBRID_FORM_STATE *destination);
int hybrid_forms_valid(const HYBRID_FORM_STATE *state);
/* Begin returns a nonzero activation serial, or zero on failure.
 * End requires that serial so a delayed teardown cannot end a later
 * activation of the same logical form in this still-live policy object. */
unsigned long hybrid_form_begin(HYBRID_FORM_STATE *state, long parent,
                                unsigned int logical_token,
                                unsigned int grants, unsigned int compatible);
int hybrid_form_end(HYBRID_FORM_STATE *state, long parent,
                    unsigned int logical_token, unsigned long activation);
int hybrid_feature_request(HYBRID_FORM_STATE *state, long parent,
                           unsigned int features, int enabled);
int hybrid_forms_release_parent(HYBRID_FORM_STATE *state, long parent);
unsigned int hybrid_features_effective(const HYBRID_FORM_STATE *state);

#endif
