# Hybrid conversion: staged implementation

## Available in this stage

The original Anathema implementation contains the 45 named pairs supplied for
this server, with stable hybrid IDs 1–45. These are **not base-class IDs**.
The two parent orders resolve to the same definition. Unknown parents, combined
base-class masks, duplicate parents and invalid IDs do not resolve.

`selfbrid`, `selfbrid list` and `selfbrid info Bloodmoon Revenant` are read-only.
The command shows whether the current class is one of the required parents.
This is only a parent check, **not complete conversion eligibility**.
It cannot create a hybrid, deduct currency, strip gear or grant powers.
No player-file fields or existing single-class checks changed.

## Parent-owned storage foundation

`classes/hybrid/state.c` provides an isolated secondary-parent bank for the
confirmed shared-field collisions: Vampire/Elementalist `powers[20]`,
Drow/Dragon `classpoints[2]`, Vampire generation/Dragon age, Vampire/Mage
`cland[3]` selections, Vampire/Werewolf/Ninja `wolf` activation state, and
Vampire/Highlander `celerity` timers. Vampire blood capacity/current,
beast and truebeast also have independent secondary storage.

The original parent's existing fields remain authoritative and are not copied
or modified. Preparing a candidate starts banks, resources, activations and
timers empty. Owned branch choices use -1 (unselected); generation/age zero is
an explicit uninitialized placeholder, **not a usable native class default**.
No quest points are charged and no positive blood, ranks or powers are granted.

`hybrid_parent_value_read` requires a matching original-class actor and valid
record, and reads the requested parent's exact owned field by value. Primary
reads use the existing legacy field; secondary reads use only the isolated
record. `hybrid_secondary_value_write` cannot write original-class storage or
another parent's field. It validates a candidate copy before committing, so
invalid ownership, field/index, negative counters, out-of-range generation/age
or branch IDs, or blood current exceeding pool leave the record unchanged.
Initialize blood pool before current; lower current before lowering the pool.
This is raw storage access, not effective rank/ability authorization.
The older secondary-bank pointer API remains available for existing callers;
it is not a validated assignment API and can invalidate a record if misused.

The standalone `AnathemaHybrid` codec now writes version 2 and reads versions
1 and 2. Loading v1 preserves its banks and supplies the new empty/uninitialized
fields, including -1 for owned branch choices. Loading checks IDs, pair
membership, ranges, field count, ownership, blood consistency and trailing
data before replacing its destination. Malformed, unsupported, truncated and
overlong records are rejected without partially replacing state. An older
v1-only reader cannot read a new v2 record.

**This codec is not connected to live player saving/loading yet.** The record
is still not a complete secondary class: Werewolf gifts/selections/rank, Mage
spheres/quintessence/paradox, other class-specific banks, forms and remaining
timers need ownership and adapters. Valid native class initialization and
Vampire learned/unknown rank states also remain pending. Raw bank validation
does not certify legal power ranks, meaningful branch combinations, trained
abilities or conversion eligibility. Timers are stored, not ticked; no blood
renewal or upkeep is enabled. Persistence failure must abort the eventual
load/transaction; the codec alone does not provide atomicity or durability.
Player-file and copyover schemas remain unchanged in this stage.

## Transformation policy foundation

The approved policy is one logical body: normal, either parent form alone,
or a distinct hybrid-only fused form when the other parent transforms.
Ending either component of fusion leaves the other parent's form active;
ending the remaining component returns to normal. Replacing a form owned by
the same parent or adding a third component is rejected.

`classes/hybrid/forms.c` models these transitions without counting legacy flags
(werebat, for example, sets two). A fusion requires an explicit feature profile
with this exact hybrid identity; it uses that one profile, not the union of
the parent form grants. Parent profiles are retained only for a later split.
Each parent independently owns requested claws/fangs. Compatibility may suspend
these requests without erasing them, and releasing one parent preserves the
other's feature ownership and remaining form.

Every body transition, including a split or final exit, changes the activation
serial. This rejects callbacks from before fusion or before the split. Future
adapters must rebind remaining-form callbacks to the new serial. Serial space
is reserved for all remaining exits, so exhaustion rejects new activations
without preventing a valid active body from ending.
Do not reinitialize/recycle a policy object while callbacks retain references;
this module does not solve character-address reuse or actor liveness.

Compatible-feature masks are supplied by future reviewed form adapters.
For example, a noncorporeal profile can suppress claws/fangs while retaining
the requests so they resume on reverting. Removing a request while suppressed
still removes it; shared features are combined, not numerically stacked.

**No legacy form handler uses this policy yet.** Logical tokens and compatible
features are not a form catalog or permission checks. Class ownership, trained
power requirements, individual form compatibility, stats, equipment, flight,
nightsight, teardown, and save/copyover reconciliation must be reviewed and
connected before activation. The current policy only arbitrates claws/fangs;
it grants no live ability, changes no existing form and is not serialized.
If multiple abilities of one parent supply the same feature, their adapter
must aggregate those sources before changing that parent's request.

## Agreed conversion behavior

- Exactly two distinct parents, using the names in the roster.
- Keep the original parent's full normal power/ability access, earned training
  and native caps. Secondary specialization does not restrict the original.
- Start the second parent's progression untrained; do not copy ranks or grant
  free mastery from similarly numbered legacy fields.
- A branchless secondary parent exposes its normal full ability roster, with
  normal prerequisites and training costs, not free mastered ranks.
- For a secondary Werewolf, select one family (Breed, Auspice or Tribe) for
  normal cap 5; the other two cap at 3, without exceeding lower native caps.
  These training restrictions and choices are not implemented yet.
- Conversion price: **50,000 quest points**, selected for Anathema. The preview
  reports the planned price and whether the current balance meets it, but
  performs no deduction. Runtime price configuration is not implemented yet.
- Both parents determine eligibility and ability inheritance.
- Exclude hybrids from base-class Master/Elite rankings.
- Test abilities and balance before enabling conversion.
- Before conversion: maxed stances, styles, weapon proficiencies, trainable
  STR/DEX/CON/INT/WIS and all powers applicable to the original class/build;
  at least 50,000 maximum HP and 50,000 maximum mana. These are requirements,
  not free upgrades. The second parent still starts untrained.

The current preview does not check these mastery/stat prerequisites. Style
and weapon caps depend on the original class (100/150/200); stance training
caps at 200. Natural style currently requires a Dragon, Vampire or Werewolf
with claws. Require every style the original class can legitimately train,
at its existing cap; exempt unavailable styles. No cross-class training,
existing style rules or caps were changed.

## Work required before conversion

Confirmed shared storage now has standalone secondary ownership and raw-value
accessors, but legacy ability handlers, regeneration/upkeep, combat calculations
and training still use the original fields. Transformations, clan affiliations,
remaining native fields and rank-dependent calculations still need explicit
class ownership and migration, not global bitwise-OR class checks.

The next stages must provide parent-owned progression/resource records;
backward-compatible save/load and crash recovery; parent-aware training,
ability, equipment, ranking and teardown rules; and tested interactions for
every pair. The mastery/stat prerequisites above must be implemented and tested.

The eventual conversion must validate first, require explicit confirmation,
and commit a durable save without leaving partially changed characters on
failure. The donor command's 100,000 stats, legend gear, mastery/status gates
are not implicitly imported into Anathema's economy. The quest-point price was
separately approved; its balance still needs testing.

The read-only command was written for Anathema. No donor `clearclass`/staff
command dispatch, random trainer aggression or destructive preflight was copied.
