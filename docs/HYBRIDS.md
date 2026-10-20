# Hybrid conversion: staged implementation

## Colors, help and approved book policy

Class labels follow **HELP CLASS**, not a separate autoclass palette. Dragon
retains its default color. Each hybrid has its own fixed, distinct letter-color
motif using both parents' colors, with the original parent leading in scoped
views. No hash or gameplay RNG chooses the pattern. Names/IDs used by commands
and saved records are unchanged.

`help hybrid` / `help selfbrid` provide the full45-pair catalog, grouped into
ten original-parent sections of nine. Each hybrid appears under both eligible
original parents. The command list remains restricted to the player's nine
choices. Each parent class help gets corresponding links; each full hybrid name
has a generated fallback help topic. Existing file-backed custom topics win.
The help module neither edits loaded HELP_DATA nor writes world files, so a
binary update/copyover delivers these additions without overwriting live OLC
help. Full legacy help reorganization remains deferred.

**Enforced for valid hybrid records:** Sorcerer/Mage chains6 pages; Sorcerer with Drow,
Elementalist or Vampire4; with Dragon, Highlander, Monk, Ninja or Werewolf3.
These fixed limits apply in both parent orders, including Sorcerer as the
original class: an explicit exception to full-original-power access.
Standalone Sorcerers keep10. The higher-melee/lower-book tradeoff motivated
this policy; no measured balance or fight results are claimed. Damage, affect
and action pages share one chain budget, checked before their page effects,
costs, messages or RNG. Limits apply even before secondary build/training is
configured; malformed player records fail closed. Changing equipment or stance
does not increase the budget. The legacy counter is saturated instead of
overshooting the cap and rejects invalid negative/overflowed values.

Existing book damage/affect pages now resolve **earned secondary Sorcerer color
ranks** through a page-scoped adapter. One chosen color may reach5; the other
five reach3. Zero/untrained ranks confer no color strength, and unrelated raw
primary color fields cannot be borrowed. Invalid records fail closed without
class/array projection, combining ranks or altering training. Original Sorcerers
retain their full native color ranks, even when the secondary build is not yet
configured; only the approved book-chain budget is reduced.

Native color getters and interpreter level gates are unchanged. `chant` already
accepts all classes at its native tier; no new casting-permission route is needed.
Dedicated color-training and material-creation routes are described below;
player conversion still needs integration.
Native actors without a hybrid record keep the original10-page
limit. No schema or handoff changes. Full secondary Sorcerer command integration
remains pending; no PvP balance claim follows from page-prefix replays.

## Available in this stage

### Secondary Sorcerer materials (existing records only)

`sorccreate book`, `sorccreate page` and `sorccreate pen` now recognize a
validated secondary Sorcerer parent through a single scoped creation-entry
adapter. No class macro, primary field, color rank, training choice or
interpreter permission is changed. Native Sorcerers, including original
Sorcerers in both parent orders, retain the same recipes and access.
The native Sorcerer-tier/RESTING command-table gates remain in effect.

Books and pens cost10 primal each; blank pages are free. There is no extra
quest, gold or mana fee. Materials retain their existing item types, hold flags,
maker ownership and zero cost/points (no resale/refund value). A missing
protoplasm template is rejected before any payment. Having the secondary
parent's unranked material-creation utility does not grant learned colors.

This uses ordinary player-save timing and is not an atomic inventory/currency transaction
or cross-player trade guarantee. Actual full/short player-file reloads verify
saved materials, owner, zero prices, primal and hybrid records; they do not
prove crash/powercut behavior. Native `chant` was already available at its
command tier to all classes: this stage does not add casting permission.
Writing/finishing pages, enchantment and vision use the scoped adapters below;
Other class powers and crafting still need their own adapters.
No playable hybrid conversion is enabled.

### Secondary Sorcerer page writing and finishing (existing records only)

Valid secondary Sorcerers can author magical pages through the native `write`
command. Hold a pen and carry the page or book:

```text
hold pen
write book title My spellbook
write page title Red embers
write page line start.damage.spell
write page line end.red
```

The two magical-author checks recognize the owned Sorcerer parent; the native
page compiler, color terminators, optional messages, affect/action syntax,
duplicate/invalid-line failure flags and size limit are unchanged. Native
Sorcerers keep their full writing access. Staff override, NPC rejection,
RESTING/LEV_SLE interpreter gates, ordinary titles and existing finished-page
annotations retain their previous behavior. Invalid/unrelated secondary
records do not gain magical-author permission: ordinary writing can still
mark a page failed, exactly as for native non-Sorcerers.

Authoring does not grant color ranks, change permanent focus or charge primal,
mana, quest points or gold. Native authors can finish pages of untrained colors;
that still does not supply learned casting strength. Existing learned-color
adapters and the approved6/4/3 hybrid book-chain budgets remain enforced at
casting, with standalone Sorcerers retaining10. Titles and page changes use
ordinary player-save timing, not an atomic inventory/trade transaction or a
crash/powercut guarantee. Full and short player-file reload tests cover completed
pages, hybrid records and resources; only full loads restore inventory.

This does not enable hybrid conversion, teacher/social
roles, other class trainers or live transformation/fusion handlers. No file-based
world help is replaced: existing custom helps still take priority.

### Sorcerer enchantment: claimed-owner gear only

`enchant <item>` recognizes a validated secondary Sorcerer as well as an
original Sorcerer. **All Sorcerer enchanters must own the target's claim.**
`questowner` must match the caster's complete player name, case-insensitively:
the `questmaker`, present carrier, partial names and name lists are not ownership.
There is no staff bypass. Null/empty claims and someone else's gear are rejected
before allocating affects or setting any enchantment flag.

Use the existing `claim <item>` command on unowned eligible gear, then
`enchant <item>`. Claiming cannot replace another player's claim; use the
existing owner-authorized transfer system instead. Enchant never claims items
automatically or rewrites owner/maker fields. This is an explicit new ownership
restriction on native Sorcerers too, not just a hybrid limitation.

Native enchantment is otherwise unchanged: an unequipped carried weapon or
armor receives permanent +5 hitroll and +5 damroll once. Existing artifact and
already-Sorcerer-enchanted exclusions remain. There is no new primal, mana,
quest-point or gold fee, no color-rank grant and no item price rewrite.
Original-class access, native command-tier/position gates and NPC rejection
remain in effect. Unrelated or malformed secondary records gain no access.

The same owner check applies to the shared `cast 'enchant weapon'` spell for
all casters, so switching command routes cannot bypass it. Its separate native
bonuses, casting costs, quest marker and artifact/relic exclusions are unchanged.
This is not a new Sorcerer casting permission: the shared spell retains its
existing class/skill access. Failed casting attempts keep normal casting-cost
semantics; the Sorcerer `enchant` command itself remains free.

Existing enchantments are not removed, and this does not add permanent binding
or change ownership transfers after enchanting. Item/claim changes use ordinary
player-save timing, not an atomic transfer or powercut guarantee. Full/short
reload tests preserve owner, record, resources, enchantment marker and the
permanent bonuses. The native object-affect file format stores duration,
modifier and location, restoring type=-1 and bitvector=0; this stage preserves
that format rather than adding a migration. Full live
hybrid conversion/forms remain pending; no PvP balance claim is made.

### Fixed hybrid-name color motifs

Each of the 45 hybrids now has its own fixed, distinct visible letter pattern,
not a random split or one shared alternating pattern. Its eight-letter motif
repeats across alphabetic characters; spaces keep the preceding color without
advancing the motif. Only the two parents' existing `help class` colors are used.
The original parent leads in `selfbrid list/info` and each parent-class help
section. Viewing the opposite original-parent order swaps the two colors, not
the motif. A standalone hybrid help topic uses your class when it is a parent,
otherwise the registry's canonical first parent. Patterns consume no RNG and
never alter the plain name, roster, eligibility, powers, prices or saved data.

### Vision ownership and legacy migration

`vision` is now available to a validated secondary Sorcerer, and remains
available to original Sorcerers. There is no new fee, color-rank grant or class
projection. Native command tier/position rules and NPC rejection remain.
The command retains its native enable/fade messages.

Sorcerer holy sight, shadow sight and night sight now have separate source bits
in the existing saved `Extra` field, instead of overwriting the shared legacy
`act`, `affected_by` and `vampaff` flags. Effective visibility, looking/scanning,
eye displays and relevant sight-dependent effects combine the sources. Native
provider toggles still operate on their own raw flags, so a grant gained or
removed while vision is active is not accidentally consumed when it fades.
Hostile holy-sight removal strips both raw and Sorcerer holy sight, but retains
the other two components. Using `vision` again restores holy sight as before.
Death and broad staff affect resets also expire the owned shadow component,
matching the legacy clearing of `affected_by`; holy/night components remain.
Resetting a class clears the source bits through existing Extra reset behavior.
Invalid/unrelated secondary records cannot activate these new sources.

**Migration limitation:** legacy raw flags have no reliable provider provenance
and are not guessed or cleared. When practical, original Sorcerers should turn
their old `vision` off **before upgrading** and verify the fade message, then
enable the new version afterward. Already upgraded or offline legacy saves with
unattributed flags need staff review; the new toggle intentionally does not
strip them. If their independent raw sight remains on, fading the new source
will not make them blind or remove that legacy sight. This is conservative
preservation, not an automatic ownership migration.

Source bits use ordinary player-save timing. Both full/short player loaders and
actual executable copyover preserve the existing Extra field; no new file
syntax is added. Record4/save epoch3/handoff2 remain unchanged. An old binary
ignores the new bits, so do not rely on this sight during binary rollback.
Full conversion/forms/other secondary powers and PvP balance remain pending.
Custom world help files still take priority; this is generated help, not an
automatic live-world help replacement.

### Native material-factory ownership audit

The recipe audit also reproduced a native item-factory ownership defect:
objects directly borrowed mutable OLC template strings. Crafting renamed
instances, later template edits or object extraction could free/invalidate
strings still referenced elsewhere. A separate prerequisite safety fix gives
instances their own copies of all nine mutable text fields, while continuing to
share immutable boot-loaded strings. Missing names/descriptions become empty
strings; absent secondary messages use the existing `(null)` marker.
The real factory/extractor regression runs128 mutable/shared/partial-template
cycles, including edits while instances are alive. Native item recipes, spell
page behavior and RNG snapshots are otherwise unchanged.

### Secondary Sorcerer training UI (existing records only)

`selfbrid colors` shows isolated secondary ranks, stored values and effective
caps. It does not attach a hybrid record. Original Sorcerers continue to use
their unchanged native `colors` trainer; unrelated/invalid records cannot train.
The new subcommand requires the same Sorcerer-tier access as native `colors`.

- `selfbrid colors focus blue` previews the free, permanent full-color choice.
- `selfbrid colors focus blue confirm` atomically saves that choice without
  granting ranks. One color may reach5; the other five may reach3.
- `selfbrid colors blue` buys exactly one rank for10 x next rank primal, using
  the existing lesson transaction. Full training costs450 primal; quest points
  and experience are not charged. No primary color array or class is swapped.

Choose a focus before training. No respec or raw-rank normalization is offered.
Legacy stored ranks above effective policy caps require staff review before
new lessons, rather than silently reducing training. Real connected original
actors must be standing and out of combat for writes. Failed saves retain no
lesson/payment; primary save recovery must succeed before retry. Directory-fsync
uncertainty keeps the visible saved candidate/payment and blocks another lesson
until recovery: the UI never refunds or repeats an uncertain transaction.

Listing/focus previews, `selfbrid list` and `selfbrid info` remain read-only.
Conversion, other class trainers and full live forms
remain disabled or pending; this UI is not a conversion eligibility check.
Material creation is a separate, scoped adapter described above.

The original Anathema implementation contains the 45 named pairs supplied for
this server, with stable hybrid IDs 1–45. These are **not base-class IDs**.
The two parent orders resolve to the same definition. Unknown parents, combined
base-class masks, duplicate parents and invalid IDs do not resolve.

`selfbrid`, `selfbrid list` and `selfbrid info <listed hybrid name>` are read-only.
The list/info command permits only combinations containing the current base
class. It shows planned quest price/balance and numeric readiness, **not
complete conversion eligibility**.
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
absorption/familiar lifecycle, social clan membership and remaining live ability adapters
remain pending. Native build selection and paid learning now have backend APIs
described below. Secondary Sorcerer focus/color ranks and material creation now
have player-facing routes for existing records; other trainers and secondary teaching
remain unwired.
Validation does not certify every combination of
prerequisites, resource defaults or effective abilities. Independent Ninja
scalars now tick as described below; other timers, blood renewal and
form-dependent upkeep remain unwired. Persistence failure must abort
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
No conversion is enabled. The explicit secondary Sorcerer color UI described
above is now available for existing records; other trainers remain pending. The limited
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
Conversion previews remain read-only; normal players cannot create hybrids.
Only the existing-record secondary Sorcerer color UI described above reaches
paid lessons. Other secondary commands and the conversion/eligibility
transaction remain pending.
Save record v4/epoch 3 and socket handoff v2 are unchanged by this stage.

## Owned resources and independent Ninja clocks

`resources.c` provides checked runtime reads/spending/gains for Vampire current
blood, Drow DP, Dragon DP, Ninja Ki and Mage BODY quintessence. The caller names
both parent and pool; the two DP pools cannot alias each other or the original
class's `CP_CURRENT`. Reads may inspect the original parent without any hybrid
record, but the mutation APIs can ONLY change a valid, ready secondary bank.
Wrong owners/pools, NPCs, corrupt/unconfigured state, negative amounts,
insufficient balances, capacity overflow and lifetime-counter overflow fail
without partially changing the actor or output. Zero is an accepted no-op.
No writable pointer is returned.

Spending reduces only the owned current balance. Earned DP gains increase
both owned current points and lifetime earnings, matching native reward
semantics; this is not a refund operation. Blood gains cannot exceed the
owned pool, Ki cannot exceed 300, and BODY quint gains cannot exceed owned
Arete. Migrated excess BODY quint is retained and spendable, not silently
discarded, but cannot grow further. Other Mage fields are deliberately outside
this API. Native spells can also require mana or health-backed Heart's Blood;
their complete costs, prerequisites and safe health fallback need a separate
payment adapter, not just a BODY-quint debit.
No resource-consuming spell, secondary feeding/reward route or trainer is
enabled merely by adding this accounting layer.

The existing online-PC mobile loop calls the new secondary updater once after
its original-class branch. Only a ready secondary Ninja is affected:
Ki regenerates by 2 per mobile update, or 5 when meditating, capped at 300;
the owned Harakiri counter decreases toward zero; an existing positive Ki/rage
boost decays outside combat or rises toward effective Sora rank times 25 in
combat. A zero boost never activates itself. Native over-cap boosts are not
silently reset. Other parents, offline/not-playing actors and invalid records
are unchanged. This uses the existing mobile-update cadence, not seconds.

Kakusu regeneration is conservatively suspended while the shared flag is set.
The secondary updater never clears that flag, even in combat: the separate
secondary activation/flag owner is not implemented. It does not infer primary
Vampire Celerity, rage, Deep Umbra or form flags; spend original resources;
invoke transformations; renew blood powers; change stat bonuses; or consume
RNG. Secondary Ki activation and Harakiri's combat effects remain unwired.

These are ordinary runtime balances, persisted by the existing atomic player
saves at normal save timing. They are NOT an immediate durable multi-currency
payment, conversion transaction or crash journal. Paid training retains its
separate checked candidate/atomic-save transaction. No schema or compatibility
epoch changed: record v4, save epoch 3, socket handoff v2.

The real-engine resource suite covers all 45 funded parent orientations,
22,500 deterministic modeled gains/spends, current/lifetime overflow, caps,
negative/mismatched inputs and codec round trips. Paired real mobile updates
across all 90 orientations preserve the original actor/PC fields and RNG
sequence while ticking only the intended bank. Ninja boundary tests cover
meditation, effective Sora half-caps, zero and LONG_MAX counters, hidden flags,
and disconnected/invalid actors. A source-hash check proves the native update
file is byte-identical to the previous release after removing its one include,
one secondary call and the separate checked native blood-debit fix below.
This is not PvP balance or live-host validation.

### Separately characterized native Vampire fix

The real engine reproduced Obfuscate taking zero blood to -1 before clearing
the flag. Native Obfuscate/Shadow/Peace upkeep now subtracts its normal 1/5/5
costs without allowing negative balances or signed underflow from invalid
legacy LONG_MIN values. The existing expiry order/messages/flag cleanup are
unchanged; combined powers still expire at their own point in the native order.
The existing Vampire suite checks all seven flag combinations at nine blood
boundaries (63 cases, including zero, negative, LONG_MIN and LONG_MAX).
That new regression failed on the old debit before the fix. This does not
enable secondary Vampire upkeep or infer ownership of a primary activation.

## Drow utility adapters — no new conversion or activation flags

Two existing commands now check the appropriate parent's earned Drow flags:
`attunement <item>` (`DROW_ATTUNE`) and `faeriefire <target>` (`DROW_FIRE`).
The original Drow uses its unchanged native getter. A secondary Drow must
have a valid owned record and have learned that exact flag: neither the other
utility flag nor stale original-class Drow fields substitutes for training.
Drow remains branchless: this adds no half-cap or specialization requirement.
The initialization/paid-training/conversion policies are unchanged.

Both native utilities cost zero to activate. These adapters do not invent a
DP, mana or primal charge, grant a learned flag, change a class, borrow a form
flag or alter either parent's training/resource banks. Learning still has its
normal backend costs; Drow player-facing training remains unwired. The separate
Sorcerer color UI does not grant Drow ranks or item permissions.

Attunement keeps the existing exact item-owner check and all excluded item
types/artifact restrictions. Already-loyal items do not change again. This
does not allow equipping another class's gear or change item provenance.
Faerie Fire uses the same three timed target affects (-50 damroll/hitroll/AC,
duration 10), native resistance roll, safety checks and 12-pulse caster lag.
Existing Faerie Fire blocks duplicates rather than stacking or refreshing,
regardless of which class originally applied it. This is a target spell
effect with an existing expiry/save lifecycle, not a new caster form.

Real interpreter tests cover native Drow and all nine secondary orientations:
individual earned flags, normal RNG parity, unchanged original training and
owned banks, effect removal, duplicate prevention, protected/AFK/link-dead
targets, resistance, missing targets, item ownership and excluded types.
Untrained actors and eight invalid/unauthorized cases fail closed, including
wrong parent, absent/corrupt record, failed load, NPC, stale original flags and
an unrelated learned flag. The native-file hash is unchanged after removing
only the include and four permission checks introduced for these two handlers.
Other Drow attacks, forms, gear creation, summon/teleport, upkeep and social
commands remain native-only; no `IS_*` macro or interpreter table was widened.
Record v4, save epoch 3 and handoff v2 are unchanged. Conversion stays disabled.

## Sunbath — trained secondary Elementalist, shared mana

`sunbath [target]` now uses earned, parent-owned Sun training (minimum 3).
Original Elementalists retain their native ranks. Secondary ranks must come
from a ready owned record and follow the existing full/half-cap and native
focus rules; stale original-class `powers[]` entries are never borrowed.
Full Sun focus follows the existing initialization/training/focus backend,
not a free rank grant from invoking a power.

Activation still costs exactly **800 of the caster's shared mana**, heals
1001–1500 HP with the native RNG, clamps to the target's max HP and imposes
18 pulses of caster lag. Missing targets and mana below 800 do not charge or
roll healing. Already-full targets still incur the native cost and lag.
No parent-owned Ki, blood, DP or quint is charged: mana is one existing shared
character stat, not a class bank. No new caster/target activation flag, form,
rank write, class swap, item, social role or synchronous player save is added.
The explicit local-target rule is unchanged; an omitted target reliably means
self, as the native command intended.

Before adapting permission checks, UBSan reproduced signed overflow at
`LONG_MAX-1` HP and division by zero when a malformed native Sun rank was 11
and the caster was fighting. Healing now saturates safely before adding past
`LONG_MAX`; the divided expression was immediately discarded by the RNG
assignment and was removed, without changing normal healing or its RNG.
The full normal suite also reproduced unreliable default targeting: Sunbath
overwrote its self-target with an empty lookup, while `number_argument()`
returned zero without initializing its output. The parser now outputs an
empty string while retaining ordinal zero, and Sunbath only looks up explicit
targets. A sentinel-buffer regression fails on the original parser object.
No other parser, alias, ordinal or interpreter command rule was redesigned.
Separate native-fix and hybrid-adapter commits preserve that distinction.

A new real-engine suite retains eight original HP/mana/lag/position/RNG
snapshots captured before either change. It runs 81 paired native/secondary
replays across all nine original parents, including nine legitimately fully
focused Sun-10 records, plus near-limit healing, below-threshold training,
invalid ownership/load/NPC records and the 799/800-mana boundary. The class
file matches its fixed native baseline after removing only the include and
two permission/rank replacements; all other Elementalist handlers are intact.
Other healing commands, stateful class powers, forms, gear, secondary trainers
and player conversion remain outside this stage. Record4/epoch3/handoff2 stay
unchanged. No conversion, fighting-style expansion, merge or deployment.

## Healing Touch — trained secondary Monk, shared mana

`touch [target]` requires ready owned Monk access and **Mantra4**. Original
Monks retain their full native ranks (getter cap8); secondary Mantra follows
the existing selected-full-branch policy:8 when selected,4 otherwise.
Choosing Tech or Art as the full branch does not disable legitimately
trained Mantra4. Neither original-class stale `monk[]` values nor invoking
the command grants secondary training. Original Monk8 remains usable when
an unrelated secondary parent has a different full branch selected.

The native rules are preserved: **5,000 shared mana minimum to attempt**;
healing is a400–600 roll times effective Mantra; actual charge is integer
`heal*3/2`; caster delay is18 pulses. A Mantra8 caster with5,000 mana can
fail the second affordability check after rolling healing, while spending
no mana and receiving no lag. That RNG consumption is intentional legacy
behavior, not silently standardized to Sunbath. Full-HP targets still cost
mana/lag; an existing longer delay is not shortened. Targets are self or
present in the caster's room, not world-wide. No Ki or other owned pool is
charged; mana is shared.

Native add-before-clamp signed overflow was reproduced under UBSan at
`LONG_MAX-1` HP and fixed in a separate native-only commit. The native
getter already caps Mantra8, keeping healing/cost multiplication bounded.
No global Monk predicate/getter, other Monk handler, callback, class flag,
form, trainer, social, command table or resource-bank rule was widened.

The existing full-engine healing suite now checks22 captured native
snapshots (Mantra4/8) and1,215 paired native/secondary replays across all9
original parents, ranks4–8, self/local/missing/remote targets, combat,
full/near-full HP, longer existing lag, low mana and exact/one-below costs.
Eight fixed seeds include odd healing to test integer rounding. Additional
cases cover36 low-rank refusals,6 unauthorized/invalid states,9
near-LONG_MAX target heals, both other full branches and original-parent
rank retention, with owned banks unchanged.

This remains backend access for audited existing powers, not playable
hybrid conversion. Save epoch3, record4 and copyover handoff2 are unchanged.

## Numeric readiness preview — no conversion

The player list is filtered to the nine hybrids containing the current
original base class. `selfbrid info` also rejects unrelated combinations,
including guessed names. Unsupported/unclassed characters get no choices.
This is parent compatibility, not permission to convert before mastery/quest/
safe-state requirements are complete. All45 internal definitions and permanent
IDs remain unchanged for validation, documentation and future saves.

`selfbrid info <full hybrid name>` now reports the numeric part of the agreed
prerequisites without changing any player data. A complete numeric report is
**not** eligibility to convert: parent/quest checks, original class/build
power mastery, existing hybrid/actor-state validation and the safe conversion
transaction are separate requirements. No automatic conversion, payment,
training, normalization, form switching or item creation is added.

The report checks max HP and max mana **at least50,000**, not current health or
remaining mana. It checks the five stored base stats against the real native
`stat_max` for the original class and current level, including the existing
staff adjustment; temporary gear/form bonuses cannot substitute for training.
All ten named stances require200. All four weapon proficiencies and existing
styles use the native original-class improvement caps, not a universal200.
Every class/slot is regression-tested against actual native improvement at
an over-cap value; cap-policy drift must fail CI, not change combat silently.

Natural style is unavailable to the seven original classes excluded by native
equipment-style selection, so they are exempt even if an obsolete value is
stored. Dragon, Vampire and Werewolf must master it: readiness does not depend
on whether claws happen to be extended when viewing the report. Their natural
style may require activating the relevant learned power/form to train.
All other five equipment styles are required at their existing class caps.
No new styles, disciplines, class restrictions or training opportunities were
introduced; `style.c` and `stance.c` remain byte-identical.

Negative values or stored ranks/stats **above** their native cap are flagged
for review, not credited or silently reduced. Vitals above50,000 are valid.
The original character and bank data remain intact. No report claims that
other powers/forms/trainers are fully wired or that conversion is enabled.
Record4/save epoch3/handoff2 remain unchanged. Help reorganization stays
deferred, using werewolf help as a layout reference with a safe live-data import.
