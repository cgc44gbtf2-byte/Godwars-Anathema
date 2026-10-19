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
The command does not change player files or existing single-class checks.

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

The standalone `AnathemaHybrid` codec now writes version 4 and reads versions
1, 2, 3 and 4. Older records preserve their existing banks/core; v1/v2 receive
empty added progression, including -1 for owned unselected choices. Loading checks
IDs, pair membership, ranges, field count, ownership, blood consistency and
trailing data before replacing its destination. Malformed, unsupported,
truncated and overlong records are rejected without partially replacing state.
Older readers cannot read a new v4 record. The new stable build ID is zero
on older records; loading never guesses clan identity or silently resets ranks.

**The codec is now connected to optional player saving/loading.** The record
is still not a complete secondary class: forms, remaining buffs/timers,
absorption/familiar lifecycle, social clan membership and live ability adapters
remain pending. Native build selection and paid learning now have backend APIs
described below; player-facing trainers and secondary teaching remain unwired.
Validation does not certify every combination of
prerequisites, resource defaults or effective abilities. Timers are stored, not
ticked; no blood renewal or upkeep is enabled. Persistence failure must abort
the load/transaction; the codec alone does not provide atomicity or
durability. Player files now support an optional record; socket handoff remains v2.

## Native progression and secondary specialization limits

`classes/hybrid/progression.c` gives the remaining native banks explicit
secondary ownership, without copying the original character:

- Werewolf Breed, Auspice, Tribe, selected gifts and rank.
- Sorcerer colors, Highlander training, Ninja skills/Ki/Harakiri, Monk training,
  Drow powers/speed and Dragon color/skill bits.
- Mage spheres, quintessence, Arete, signed paradox and paradox ward.

Together with the original banks this covers the main progression arrays for
all ten classes. It is not an inventory of every live activation or timer.
Native Ninja/Dragon/Drow arrays mix progression with transient values/flags;
storing them does not reconcile a live form or activate their effects.
Paradox remains signed because native backlash can leave it negative.

The raw progression reader requires the matching primary actor and an exact
owned parent. The raw secondary writer validates a candidate record, rejecting
wrong owners, indexes, known rank-ceiling violations, unknown Drow/Dragon bits
and nonzero reserved slots without changing the record. Primary reads retain
the original stored values; there is no primary write API. Neither raw writes
nor specialization selection train a live character or pay training costs.

`classes/hybrid/specialization.c` applies the agreed rule to **all eight
secondary classes with ranked power trees**, not just the Werewolf example:
one chosen specialization at its normal native cap; the others at half
rounded up. No choice means all branches use their secondary half caps.

| Secondary parent | Full specialization | Other branches |
| --- | --- | --- |
| Werewolf | Breed OR Auspice OR Tribe family, up to 5 | Other families 3 |
| Vampire | One discipline at its clan-sensitive native cap | 7 → 4, 6 → 3, 5 → 3 |
| Mage | One sphere at native 5 or 4 | Selected spheres 5 → 3, others 4 → 2 |
| Elementalist | One element: 8 before focus, 10 after focus | 4 normally; focused opposite 3 |
| Sorcerer | One color at 5 | Other colors 3 |
| Highlander | One training path at 6 | Other paths 3 |
| Ninja | One principle at 10 | Other principles 5 |
| Monk | One path at 8 | Other paths 4 |

Werewolf uses a family ceiling of five: selected gifts normally train to five
and nonselected gifts to four, so the chosen family does not automatically
raise every path to five. Other families cap at three, still respecting any
lower native limit. Drow/Dragon boolean unlocks and fixed choices are not
multiple ranked trees and are not halved. Stats, pools, timers, Ki, Arete and
Werewolf rank are not specialization branches and receive no half-cap rule.

`hybrid_secondary_specialize` records an initial branch choice and accepts the
same choice idempotently; it does not provide respecialization. The old
Werewolf choice/cap/rank APIs delegate to this general implementation.
`hybrid_parent_training_cap` calculates the normal original cap or the
secondary cap. `hybrid_parent_rank_read` preserves native effective getters
for the original, but clips secondary effective ranks to the allowed cap.
`hybrid_secondary_rank_write` rejects above-cap assignments atomically.
`hybrid_secondary_ranks_valid` identifies stored ranked progression requiring
review; it does not certify complete class initialization or ability eligibility.

**Raw storage is deliberately separate from effective permission.** Legacy
records may preserve raw ranks above the new secondary cap. Roundtripping
them must not silently destroy data, but the effective accessor cannot expose
above-cap secondary power, including after reload or a cap-lowering selection.
Raw getters, raw writers and direct bank pointers are never authorization.
Future teaching/training/ability adapters must use the checked rank API,
validate normal prerequisites and learning, and handle payment/save atomically.

Vampire cap profile begins uninitialized: the backend refuses effective
secondary discipline access until a legitimate native initializer supplies
standard-clan or Caitiff/Panders caps. The initializer is restricted to empty
banks and cannot silently reclassify existing training. This profile is **not
clan affiliation**, which remains pending. Thaumaturgy's native cap stays five.
Original-class caps use the original actor's actual clan/selections.

The native Elementalist focus gate requires rank eight in every element,
which would be impossible under secondary caps. The standalone
`hybrid_secondary_element_focus` instead requires every element at its allowed
pre-focus cap (chosen eight, others four). It then applies the normal focus
analogue: chosen ten, opposite three, others four, in one validated candidate.
It grants nothing from an untrained/undertrained state and cannot be repeated.
Affinities inconsistent with the selected specialization have no effective
secondary rank access. This does not activate native auras/forms, and no
secondary unfocus/reset/respec route has been introduced.

**Legacy trainers, teaching and combat do not call these APIs yet.** The
read-only `selfbrid` command still cannot create, train or charge a hybrid.
These APIs are policy/storage, not live trainers or permissions.
Branchless secondary parents retain their normal roster with normal learning
and costs, not free mastery. No primary class has a new specialization penalty.

## Optional player-file integration and initialization

`classes/hybrid/player.c` adds an inline, zero-initialized optional record to
`PC_DATA`. No command attaches one yet. Ordinary players still save without
an `AnathemaHybrid` line, and old player files load with no secondary parent.
The player's `class` field and original trained arrays remain authoritative;
no bitwise ancestry mask or primary-array copying is introduced.

Full and authentication-only loaders preserve the bounded v4 record (including
old v1/v2/v3 migrations). Ownership is checked at explicit `End`, allowing `Class`
to occur later in a file. Duplicate, malformed, unsupported, overlong, truncated
or wrong-primary records fail closed: login/recovery/staff loading rejects the
shell and atomic saves refuse to overwrite the file. Full loads validate before
equipment/style repair. Player and backup writes use their existing independent
atomic transactions, retries and durability reporting. Invalid in-memory hybrid
state also blocks saving; it is not silently stripped. Recycled PC_DATA is
zeroed, with no heap ownership or new teardown callbacks.

`hybrid_secondary_initialize` builds a candidate without touching the actor:
Vampire generation9 with capacity300/current0 and unknown disciplines (`-1`);
Dragon hatchling age12 with color unselected; Werewolf rank1 with gifts
unselected; Mage Arete1 with spheres unselected; other banks remain untrained.
This follows native creation metadata without invoking `autoclass`, changing
primary stats/clan/equipment, or granting trained ranks or currency. Vampire
first-learning markers are preserved separately from locked rank0;
effective secondary rank access never exposes negative ranks. Vampire clan
profile, Dragon color and Mage choices use the explicit build APIs below;
social clan membership still requires a separate live integration.

This is **not playable conversion**: checked attachment is a backend operation,
not a staff/player command and not itself a durable conversion transaction.
No conversion or player-facing secondary trainer is enabled. The limited
checked combat/Highlander adapters below are the first live access integration.
The 50,000QP conversion remains preview-only. One active parent/hybrid form
policy still needs live adapters and safe restart reconciliation.

The supported save epoch is now3; socket handoff stays2. Forward updates read
older player files. The reviewed updater blocks downgrade/rollback to epoch1/2
because old binaries can silently discard the optional record. Cross-epoch
startup failures stop for operator repair instead of auto-starting the old
binary. Adopted unknown binaries are conservatively recorded as epoch3.
Use an epoch3-compatible corrective release; do not manually replace it with an
older binary once hybrid records may have been saved.

## Native build choices and parent-aware training backend

`build.c` stores a stable class-build ID, not a social clan membership, rank
or leader role. Its 25 choices exactly match native `autoclass`: nine Vampire
clans, ten Mage traditions (including Hollow Ones), and six Dragon colors.
Malkavian/Caitiff/Panders are not invented as new autoclass choices.
Werewolf Breed/Auspice/Tribe selections and Mage avatar/personal spheres
(plus Hollow Ones' clan sphere) are checked, unique where native rules require
it, and one-time. Specialization remains an independent one-full-tree choice.
No operation calls `autoclass` on a hybrid or swaps its primary class/arrays.
Native Vampire basic/clan disciplines begin UNKNOWN (-1); unavailable
non-clan disciplines remain zero and require a legitimate teacher. Build
selection cannot reset taught/trained banks. Legacy unconfigured builds
require explicit compatible initialization/migration; they are not guessed.

`training.c` quotes a candidate without mutating the actor, then recomputes
it at commit. Ranked lessons cover all eight branched classes using native
prices and the agreed secondary caps. Werewolf gifts cost `(next rank)*10`
for selected gifts or `*15` otherwise; Vampire disciplines cost `*5` in-clan
or `*15` out-of-clan, with native first learning to rank1. Mage, Elementalist,
Sorcerer, Ninja and Monk ranks cost `*10` primal; Highlander costs `*15`.
An unavailable Vampire discipline returns NEEDS_TEACHER, never a free unlock.
Reserved discipline slots, Ki/timers, and combat-only flags are not lessons.
Elementalist focus uses the existing secondary pre-focus-cap policy, with
no additional basic training after focus and no live aura/form activation.

Branchless parents retain their native progression: Drow's 17 unlock prices
and four speed increases use secondary DP; Dragon's 13 skills retain their
required ages and prices, and age progression charges its normal DP/QP.
Mage Arete costs `(next Arete)*500000` experience and caps at10. Werewolf
self-advancement requires the normal completed quests and monster kills,
stops at rank4, and does not grant the staff-controlled fifth rank.
Secondary DP income, teacher consent/lessons and ability activation are
separate future adapters; this backend never funds an empty bank.

Commit requires the real connected original actor, standing/out of combat,
and no pending primary save. A shadow player serializes the candidate and
payment through the existing primary atomic transaction, including inventory.
Pre-rename failure keeps both ranks and money unchanged. A successful rename
with failed directory fsync keeps the visible rank/payment in memory, queues
durability retry and blocks further lessons until primary retry succeeds.
It must NOT be refunded/replayed: the new file may already survive a crash.
Backup is queued independently; no multi-file or multi-player transaction
is promised. No caller-supplied quote is trusted for payment.

These are tested backend APIs, **not player-accessible trainers**. `selfbrid`
remains read-only. Original-class requests return PRIMARY for the unchanged
native trainer; original training/caps/access are not replaced. Conversion,
secondary teaching and class powers/passives/forms/upkeep still need live
adapters and balance testing before any player can convert.

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
  The standalone specialization backend models this; live trainers, commands
  and teaching have not been connected.
- The restriction covers every ranked secondary power tree, including Ninja,
  Monk, Sorcerer and Highlander. Only the secondary is restricted; the original
  class retains its full normal progression and access.
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

The next stages must complete clan/build initialization and remaining state;
connect live form recovery, parent-aware training,
ability, equipment, ranking and teardown rules; and tested interactions for
every pair. The mastery/stat prerequisites above must be implemented and tested.

The eventual conversion must validate first, require explicit confirmation,
and commit a durable save without leaving partially changed characters on
failure. The donor command's 100,000 stats, legend gear, mastery/status gates
are not implicitly imported into Anathema's economy. The quest-point price was
separately approved; its balance still needs testing.

The read-only command was written for Anathema. No donor `clearclass`/staff
command dispatch, random trainer aggression or destructive preflight was copied.

## Checked combat access — first live integration

`access.c` recognizes a secondary only when the attached record matches the
original actor and is structurally valid. Vampire/Mage/Dragon require an
explicit native build; their initialized core and Werewolf rank must be
present. Older unconfigured records remain inactive. Positive effective ranks
are capped by the agreed secondary policy; retained raw ranks above a half-cap
do not bypass that cap. First-learning/unavailable Vampire markers grant nothing.
Primary getters and all ten `IS_*` class predicates remain native-only.
No handler temporarily changes the character's class or swaps its primary banks.

`passives.c` supplies learned secondary contributions to initiative, soak,
avoidance, avoidance-breaking, damage cap, hitroll, damroll and armor. These
add once to the original class's contribution. Armor/hitroll enter the existing
calculations before their divisions; injury penalties and speed/soak conditions
are applied at their native stage, not separately for each parent. Secondary
Philodox's learned injury protection applies to the three raw combat stats.
Integer additions saturate at long limits, and Drow speed retains native
compound truncation even for negative avoidance.
This is not every class passive: class-specific contact, immunity and damage
callbacks outside these paths remain native-only until separately adapted.

| Secondary parent | Learned contributions in this stage | Still excluded |
|---|---|---|
| Vampire | Fortitude armor/soak; Potence damroll/damage cap | Celerity activation, blood/rage, form/bodycraft flags, damage multipliers and advanced damage callbacks |
| Werewolf | Learned Breed/Auspice/Tribe combat bonuses and injury protection | Rank-based class baseline, rage, forms and activated gifts |
| Mage | Time speed/avoidance; Entropy avoidance-breaking/damage cap | Spellcasting, quint/paradox and physical damage multiplier |
| Highlander | Weapon/Attack/Defense combat bonuses | Speed timer/DEX boost, forge/gear ownership and Lunge |
| Ninja | Ningenno speed/avoidance-breaking; Chikyu avoidance/damage cap | Ki, Harakiri, weapon multipliers and toggles |
| Monk | Learned Arts speed, armor, soak, accuracy, damroll, avoidance and damage cap | Chimera flags, weapon multipliers and activated techniques |
| Drow | Trained speed and learned Skip/Tough/Eyes/Might/Bite bonuses | Spells, active shields, flight and shape ownership |
| Elementalist | Learned Wind/Time/Earth/Moon/Fire/Infinity combat bonuses | Aura/shield ownership, forms, weapon multipliers and active powers |
| Sorcerer | Checked color-rank API; no learned-only bonus in these eight native paths | Color spellcasting and resources |
| Dragon | Checked learned-skill query; no form-free bonus in these paths | All form-dependent effects and lifecycle/resource handlers |

There are no free Vampire/Werewolf class baseline bonuses, inferred shared
activation flags, copied primary resources or compounded weapon multipliers.
The primary retains its full existing behavior, including its own active forms.
No NPC receives a secondary bonus.

Three existing Highlander commands now use owned, effective ranks:
`bflash` needs Powers 2, `bash` Powers 3, and `judge` Powers 5. Their native
costs/waits, safe-target checks and effect implementations remain intact.
Powers 5 requires choosing that full secondary path. Counter retains BOTH
Defense 4 and Powers 4: under one-full-tree/other-paths 3 it is deliberately
unavailable to a secondary Highlander, including when a migrated raw bank has
both ranks 4. Original Highlanders retain Counter normally. Htwist and Lunge
are not broadened, and secondary forge/class gear is not enabled.

The real-engine regression captures 20 original profiles before the combat
hooks, checks 180 untrained pair orientations and compares 90 learned secondary
pairs with independent native-class counterparts. It also exercises inherited
flag isolation, raw over-cap masking, corrupt/unconfigured records, odd-speed
truncation, integer limits, brittle soak, final speed floor, injury protection,
unchanged RNG, interpreter-routed Highlander powers and Counter refusal.
These are mechanics checks, not proof of PvP balance or multiplayer stability.
`selfbrid` is still read-only: no normal player can convert or reach these
secondary paths until the conversion/eligibility transaction is implemented.
Save record v4/epoch 3 and socket handoff v2 are unchanged by this stage.
