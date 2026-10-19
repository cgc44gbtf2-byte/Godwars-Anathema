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

## Parent-bank storage foundation

`classes/hybrid/state.c` provides an isolated secondary-parent bank for the
two confirmed shared-field collisions: Vampire/Elementalist `powers[20]`
and Drow/Dragon `classpoints[2]`. The original parent's existing fields remain
authoritative and are not copied or modified. Preparing a candidate record
starts these secondary banks at zero, preserves the primary identity, and
does not charge quest points or grant any class.

Bank access requires the exact secondary-parent identity and an owned bank.
The version-1 `AnathemaHybrid` codec stores one bounded record; loading checks
IDs, pair membership, numeric range, field count, bank ownership and trailing
data before replacing its destination. Malformed, unsupported, truncated and
overlong records are rejected without partially replacing state.

**This codec is not connected to live player saving/loading yet.** It is not
a complete secondary class: generation/age, rage, forms and remaining
class-owned fields still need mapping and valid class-specific initialization.
Structural bank validation is not power-rank validation or conversion
eligibility. Persistence failure must abort the eventual load/transaction;
the codec alone does not provide atomicity or durability. Player-file and
copyover schemas remain unchanged in this stage.

## Transformation policy foundation

Agreed policy: one full-body transformation at a time; compatible features
such as claws and fangs may coexist. `classes/hybrid/forms.c` models one logical
form rather than counting legacy flags (werebat, for example, sets two).
Each parent owns its feature requests separately from the full form's grants.
Ending a form or releasing one parent preserves requests owned by the other.

An incoming form must be rejected until the old form has explicitly ended;
there is no automatic replacement that skips teardown. Per-activation serials
reject delayed cleanup from an earlier activation of the same form in the
same still-live policy object. Serial exhaustion fails closed, without wrap.
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
- Keep earned training from the original parent.
- Start the second parent's progression untrained; do not copy ranks or grant
  free mastery from similarly numbered legacy fields.
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

Vampire and Elementalist share legacy `powers[]` slots. Drow and Dragon share
`classpoints[]`. Further shared state includes generation, rage/beast,
transformations, clan affiliations and rank-dependent calculations. These need
explicit class ownership and migration, not global bitwise-OR class checks.

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
