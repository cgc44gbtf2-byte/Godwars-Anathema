#ifndef ANATHEMA_HYBRID_RESOURCES_H
#define ANATHEMA_HYBRID_RESOURCES_H

struct char_data;

/* Explicit parent and pool; CP_CURRENT aliases must never select the other
 * parent's points. BODY quint only, not a complete Mage spell-cost adapter. */
enum hybrid_resource {
    HYBRID_RESOURCE_BLOOD,
    HYBRID_RESOURCE_DROW_DP,
    HYBRID_RESOURCE_DRAGON_DP,
    HYBRID_RESOURCE_KI,
    HYBRID_RESOURCE_BODY_QUINT
};

/* Primary reads need no hybrid record and never change native storage.
 * Checked secondary reads/mutations require a ready, owned live record.
 * Failure leaves destination/actor unchanged. No writable bank is exposed. */
int hybrid_resource_read(const struct char_data *ch,long parent,
                         enum hybrid_resource resource,long *destination);
int hybrid_secondary_resource_spend(struct char_data *ch,long parent,
                                    enum hybrid_resource resource,long amount);
/* Gains exceeding capacity/long limits fail, rather than silently losing
 * currency. Earned Drow/Dragon DP increases BOTH current and lifetime total;
 * this is not a refund API. Spending never changes lifetime earnings.
 * Zero is an accepted no-op. No mutation calls save/charges original fields.
 * Normal atomic player saves persist these runtime balances, like native
 * combat resources; this is NOT a durable payment/conversion transaction. */
int hybrid_secondary_resource_gain(struct char_data *ch,long parent,
                                   enum hybrid_resource resource,long amount);

/* Called once at the existing online-PC mobile-update cadence. Only the
 * independent Ninja Ki/rage/Harakiri scalars are ticked. No flags, callback,
 * RNG, form upkeep, Celerity, renewal or auto-transformation is inferred. */
void hybrid_secondary_mobile_update(struct char_data *ch);

#endif
