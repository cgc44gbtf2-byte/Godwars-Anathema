# Hybrid conversion: staged implementation

> Current compatibility: **save epoch 4, hybrid core v4, optional form ledger
> v1, socket handoff v2**.
> Disquiet adds a named saved affect; epoch3 references below describe earlier
> stages, not the current rollback policy. The core hybrid record and handoff
> layouts are unchanged. Follow `deploy/UPDATING.md`: do not downgrade to an epoch3
> binary after running an epoch4 release.

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

### Secondary Monk Stun Fist (existing records only)

`stunfist <target>` is now routed through owned Monk access and learned Tech.
A secondary Monk must select the full Tech path and actually learn Tech7 or8;
other secondary Monk paths cap Tech at4, even if a migrated raw bank contains
rank7/8. No original-parent Monk training is read as a secondary bank or
overwritten. Original Monks retain their native learned-rank behavior regardless
of the secondary parent, and secondary records must be valid before use.

The ability still costs 5,000 mana only after its native target/safety checks.
It interrupts the target for1second, recovers the caster for2seconds, and applies
shared per-target immunity for6seconds from application. Another attacker cannot
refresh that immunity or evade it with a different parent class. Melee Counter
still intercepts it in the same place, after native spending/recovery; the native
safe-room, sleeping/offline, charm/master, missing-room and no-kill guards stay.
Failed rank/eligibility checks grant no interruption, resources or free attacks.
Mana remains the character's existing shared mana, not a copied secondary pool.

The real-engine suite pins264 native snapshots (including rank/cost boundaries),
compares9720 paired replays across all nine pairings and both original-parent
orientations, and exercises nine corrupt/permission guards plus nine real
shared-immunity timer sequences. The native CSV SHA256 is
`5507225d148cfb29700f4a585b4e035e8e75265ed6b7061b80abec9cf725c82c`.
Only two guard expressions in the Monk module change; no other powers or
`IS_MONK`/native getter definitions are broadened.

These power adapters do not enable conversion or change the save format
(record4/save epoch3/handoff2). Explicit secondary-Monk training controls
are described below; attachment remains backend-only. Live PvP balance
and full hybrid forms remain unproven.
The Stun Fist stage itself added no new conversion and no new trainer.
Generated help describes this scope; custom live-world helps retain priority.

### Secondary Monk direct attacks (existing records only)

The five native Tech attacks accept valid secondary-Monk records and use the
effective learned Tech rank, not raw storage or the original parent's ranks.
Tech 1–4 remains available on any secondary Monk path; Tech 5+ requires the
chosen full Tech path. Nothing grants training for free. Original Monks keep
the native rank getter and all their original abilities.

- `repeat <target>` — Tech 1; at least 1,200 mana to start, then 600 per
  attempted unarmed strike, up to `1 + floor(Tech/2)` attacks; 15-pulse wait.
  Its actual hit damage still uses the original parent's weapon/style rules.
- `aurashot <target>` — Tech 2; 2,000 mana, 12-pulse wait. A landed shot
  deals `(200–300) × Tech` and heals the caster for 80% of displayed damage,
  capped at max HP.
- `blitz` — Tech 3; 5,000 mana, 18-pulse wait. Native group/safety filters
  remain: hostile NPCs in the room and only PCs already in combat with the
  caster. NPC damage is `(750–1,250) × Tech`; PC damage is `(500–750) × Tech`.
- `cripple <target>` — Tech 5; 2,000 mana, 9-pulse wait. Its unarmed melee
  contact still meets Counter before damage; a hit drains the target's mana
  by three times its damage without going below zero.
- `darkblaze <target>` — Tech 6; 2,000 mana, 18-pulse wait. Retains the
  native blind duration and strips the target's holy sight, including
  source-owned Sorcerer sight, without changing the other sight modes.

Native command bodies remain structurally unchanged beyond scoped class/rank
gates and separately characterized safety repairs: Aura Shot's HP addition,
Cripple's mana subtraction, missing combat room/PC data, and callback-driven
target/actor removal in Repeat/Blitz. The tests reproduced the old failures,
pin 720 normal native command snapshots, compare 32,400 paired real-engine
command runs, and reject 50 invalid or untrained secondary cases. They do
not establish live PvP balance or full multi-class form compatibility.

No playable `selfbrid` conversion, saved form ownership or
access to Chimera or clan leadership commands is added by the attack adapter.
Almighty Favor, Life Cloak and Celestial Path are handled separately below.
Record4/save epoch3/handoff2 are unchanged;
custom world help files still take priority over generated hybrid help.

### Secondary Monk persistent buffs (existing records only)

Validated secondary Monks can use the original commands with their effective
learned Mantra rank. `almighty` needs Mantra5 and `lifecloak` needs Mantra8,
so both require the chosen full Mantra path. The other secondary paths cap
Mantra at4. Original Monks retain their normal ranks and access in either
parent order. This grants no ranks, focus UI, trainer or playable conversion.

- `almighty [target]` costs5,000 mana with a12-pulse wait; omitting the target
  blesses self. The native single-target NPC option remains available.
  `almighty group` costs20,000 mana and waits24 pulses only when it blesses at
  least one eligible group PC; already-blessed recipients do not stack.
  The existing shared flag grants the native100 soak,15 avoidance and100
  damage-cap bonuses. No second copy of those bonuses is added for hybrids.
- `lifecloak` requires positive, completely full HP and mana. It leaves
  `floor(4 × HP / 5)` HP and spends all current mana. At zero or negative HP,
  a still-qualified Mantra8 owner consumes the cloak once and recovers
  `max(1, floor(max HP / 5))` HP. Integer arithmetic avoids large-HP rounding
  drift. Unowned, invalid or no-longer-qualified saved cloak flags are removed
  at the death check instead of granting a free rescue.

Favor's sunrise sweep now covers every loaded character, including switched
originals, linkless PCs, editors, head-only PCs and NPC recipients. It clears
only Favor, not Cloak or other effects. Cloak consumption likewise leaves
Favor and unrelated flags intact. Tests pin324 native buff snapshots, compare
14,580 paired real-engine command replays, exercise invalid records and live
sunrise cleanup, and preserve both buff flags through180 full/short loads
across all45 hybrid pairings in both orders.

### Offline Favor expiry

Favor now saves an optional `*AnathemaFavor 1 <grant> <dawn>` comment beside
the existing `More` field. The deadline is an absolute **game-calendar**
hour at the original next sunrise, not a fresh duration on login. Full and
short loaders retain a valid blessing before that dawn and remove only Favor
at or after it. Month/year rollover is supported; repeated saves do not
refresh its lifetime. Cloak, Guardian and other effects are not removed.
Invalid, duplicate, overlong, truncated or future-grant metadata cannot
grant protection or lock an otherwise valid player out. Unverifiable legacy
flags have no proven remaining lifetime: **unverifiable legacy Favor is
cleared on load**, so it must be recast after the first upgrade.

An older binary already treats `*` lines as comments and can read these
files, but does not enforce the new offline deadlines while running.
If it rewrites a player, it drops the optional deadline; on re-upgrade,
that unverifiable Favor is cleared rather than resurrected. Record4/save
epoch3/handoff2 remain unchanged; no hybrid record or copyover schema changes.
Clock checks follow the native 17-month/35-day calendar. Reboot/copyover uses
the existing boot-time clock reconstruction, which may expire a blessing
sooner after server lag. Manual clock rewinds outside the grant/deadline
window fail closed; this is not a wall-clock timestamp or protection against
arbitrary administrator time manipulation. Loaded/NPC sunrise cleanup also
clears the deadline. Ordinary save timing is not a crash journal or power-loss
guarantee; no player-file sweep or live-data edit is performed by this update.

An older binary predating this adapter cannot rescue a secondary Monk with
Cloak, even if it can parse the save. Before rollback, move such characters
out of danger and do not rely on that protection. Chimera and live form
ownership remain untouched. Custom world help still takes priority over
generated hybrid help; no live area files are imported by a binary update.

### Secondary Monk Celestial Path

`celestial <target>` accepts a valid secondary-Monk record with learned
Mantra7. Only the selected full Mantra path can reach7; all other secondary
paths remain capped at4, including migrated raw ranks above that effective cap.
Original Monks retain their full native access and ranks in either parent order.
No primary training, secondary training or class identity is changed by travel.

A successful cast costs **1,500 mana**, with no new recovery or random roll.
Insufficient or negative mana is rejected before movement, effects or spending.
Native destination privacy, solitary/no-teleport room rules, player summon
consent, same-room rejection, target-level restriction and Guardian warning
remain. NPC targets retain their native consent exception. The origin-room
policy is not broadened or tightened by this adapter; this is not a global
travel-policy rewrite. The interpreter still requires standing and awake level.

Separately characterized native repairs reject missing caster context safely.
Shared visibility lookup rejects missing actors and ordinary unplaced targets
without dereferencing a null room; existing self, staff and item-vision
shortcuts remain. Tests reproduce seven old failure cases, pin180 native
snapshots with sufficient mana, compare **8,100 paired** real-engine command
replays, check10 invalid records and exercise **90 full/short** player loads.
These establish scoped behavior and bank retention, not live PvP balance.

No conversion, clan privileges, Chimera or other live
forms are enabled. Record4/save epoch3/handoff2 remain unchanged. Custom world
help files retain priority; no live world/help import is performed here.

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

The `AnathemaHybrid` core codec writes v4 and reads versions 1–4. Older records
preserve their banks/core; v1/v2 receive empty added progression, including -1
for owned unselected choices. A separate optional `AnathemaHybridForms 1` line
stores the parent-owned logical form ledger and activation serial. Older
player readers safely skip this unknown line; older writers may drop it, so
keep form transformations disabled until the running code supports the ledger.
New readers default a missing line to the normal/base form. Both records are
validated before accepting a player; bad IDs, parent pairs, masks, counters,
truncation or trailing tokens fail closed. The core save epoch and socket
handoff remain unchanged. The stable build ID is zero on older records; loading
never guesses clan identity or silently resets ranks.

**The codec is now connected to optional player saving/loading.** The form
ledger survives new-code saves and copyover, but is not yet connected to native
transformation handlers. The record is still not a complete secondary class:
remaining buffs/timers,
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
| Highlander | Weapon/Attack/Defense combat bonuses | Speed timer/DEX boost and forge/gear ownership |
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

The existing Highlander commands `bflash` (Powers 2), `bash` (Powers 3),
`judge` (Powers 5) and `lunge` (Powers 6) use owned, effective ranks. Their
native costs/waits, safe-target checks and effect implementations remain intact.
Secondary Highlanders can inspect/focus/train the four ranked paths with
`selfbrid highland`; one path is capped at6, the rest at3, with each paid rank
costing 15 times its next rank. The focus is permanent and grants no ranks.
Primary/native `increase` training remains unchanged; secondary Highlanders are
directed to the isolated trainer. `htwist` still uses a shared native Celerity
timer and is not enabled for the secondary bank. `forge` and class-bound gear ownership also
remain native-only.
Powers 5 requires choosing that full secondary path. Counter retains BOTH
Defense 4 and Powers 4: under one-full-tree/other-paths 3 it is deliberately
unavailable to a secondary Highlander, including when a migrated raw bank has
both ranks 4. Original Highlanders retain Counter normally. Secondary Lunge
requires Powers 6; Htwist and forge/class gear remain native-only.

The real-engine regression captures 20 original profiles before the combat
hooks, checks 180 untrained pair orientations and compares 90 learned secondary
pairs with independent native-class counterparts. It also exercises inherited
flag isolation, raw over-cap masking, corrupt/unconfigured records, odd-speed
truncation, integer limits, brittle soak, final speed floor, injury protection,
unchanged RNG, interpreter-routed Highlander powers and Counter refusal.
These are mechanics checks, not proof of PvP balance or multiplayer stability.
Conversion previews remain read-only; normal players cannot create hybrids.
Only the existing-record secondary Sorcerer color UI described above reaches
paid lessons and the secondary Werewolf, Monk, and Highlander training UIs.
Other secondary commands and the conversion/eligibility transaction remain
pending.
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

## Drow command adapters — no new conversion or activation flags

Three existing commands now check the appropriate parent's earned Drow flags:
`attunement <item>` (`DROW_ATTUNE`), `faeriefire <target>` (`DROW_FIRE`) and
`acidbolt [target]` (`DROW_ACID`).
The original Drow uses its unchanged native getter. A secondary Drow must
have a valid owned record and have learned that exact flag: neither the other
power flag nor stale original-class Drow fields substitutes for training.
Drow remains branchless: this adds no half-cap or specialization requirement.
The initialization/paid-training/conversion policies are unchanged.

These commands cost zero to activate. These adapters do not invent a
DP, mana or primal charge, grant a learned flag, change a class, borrow a form
flag or alter either parent's training/resource banks. Learning still has its
normal backend costs; the limited Drow player-facing trainer is described below. The separate
Sorcerer color UI does not grant Drow ranks or item permissions.

Attunement keeps the existing exact item-owner check and all excluded item
types/artifact restrictions. Already-loyal items do not change again. This
does not allow equipping another class's gear or change item provenance.
Faerie Fire uses the same three timed target affects (-50 damroll/hitroll/AC,
duration 10), native resistance roll, safety checks and 12-pulse caster lag.
Existing Faerie Fire blocks duplicates rather than stacking or refreshing,
regardless of which class originally applied it. This is a target spell
effect with an existing expiry/save lifecycle, not a new caster form.

Acid Bolt retains the native single `one_hit` attack and 8-pulse lag. Its hit,
damage and weapon effects use the original class's existing combat rules, not
a new damage formula or extra attack. Empty target defaults to the current
opponent. Existing safety checks remain. A separate native safety repair rejects
missing caster context and checks cached-room membership before applying lag
after a combat callback; it never dereferences a removed caster.

Real interpreter tests cover native Drow and all nine secondary orientations:
individual earned flags, normal RNG parity, unchanged original training and
owned banks, effect removal, duplicate prevention, protected/AFK/link-dead
targets, resistance, missing targets, item ownership and excluded types.
Untrained actors and eight invalid/unauthorized cases fail closed, including
wrong parent, absent/corrupt record, failed load, NPC, stale original flags and
an unrelated learned flag. The native-file hash is unchanged after narrowly
normalizing the include, six permission checks and Acid Bolt safety guards.
Other Drow attacks, forms, gear creation, summon/teleport, upkeep and social
commands remain native-only; no `IS_*` macro or interpreter table was widened.
Record v4, save epoch 4 and handoff v2 are unchanged. Conversion stays disabled.

### Secondary Drow training for integrated powers

For an existing secondary Drow record only:

```text
selfbrid drow
selfbrid drow attunement
selfbrid drow attunement learn
selfbrid drow faeriefire
selfbrid drow faeriefire learn
selfbrid drow acidbolt
selfbrid drow acidbolt learn
selfbrid drow llothsweb
selfbrid drow llothsweb learn
selfbrid drow readaura
selfbrid drow readaura learn
selfbrid drow heighten
selfbrid drow heighten learn
selfbrid drow spidereyes
selfbrid drow spidereyes learn
selfbrid drow poisonimmunity
selfbrid drow poisonimmunity learn
selfbrid drow blindfight
selfbrid drow blindfight learn
```

The listing/details are read-only. `learn` buys exactly one existing flag:
Attunement costs **1,000 DP**; Faerie Fire and Acid Bolt cost **5,000 DP each**;
Lloth's Web costs **8,000 DP**, Read Aura **6,000 DP**, Heightened Senses
**20,000 DP**, Spider Eyes **7,500 DP**, and Poison Immunity and Blind Fighting
**5,000 DP** each, matching the native
prayer prices. Their activation remains free, with native item ownership,
target-safety, resistance, affect and lag rules. The lesson writes only the
secondary Drow bank through the existing checked save transaction; original
class identity, primary Drow storage, other currencies and save format are
unchanged. Primary Drow characters continue using native `prayer`; this UI
does not intercept or alter it.

Only these nine integrated powers are offered by this trainer in this stage.
Other Drow unlocks remain backend-only pending their complete runtime and
learning review; backend storage is not permission to expose an unadapted
power. No compatibility change: record4/save epoch4/handoff2. Conversion and
live hybrid forms remain disabled.

### Lloth's Web and Ninja overlap

An earned secondary `DROW_WEB` grant enables the existing `web [target]`
command, using the native level-50 web spell and 12-pulse wait. Native targeting,
protection, dodge chance, duplicate-web handling and two-tick armor/web affect
remain unchanged; no extra attack, damage formula, effect stack or activation
charge is added. The learned Drow grant is independent: even a Ninja original
uses its free activation when trained, without spending Ki or requiring Chikyu8.
Without a learned Drow grant, original Ninjas still require Chikyu8 and pay
10 Ki, including the native cost on a dodged/already-webbed cast. Secondary
Ninja access is not enabled by this change.

A separate native repair rejects missing caster context and a remote/stale
current-opponent fallback, checks the spell pointer, and verifies cached-room
membership after the spell callback before charging Ki or applying lag.
This does not solve same-address actor reuse or malformed linked lists.
Tests preserve 224 pre-change native snapshots, exercise both parent orders
and both shared grants, and cover full/short saves and failed/uncertain lessons.
Record4/save epoch4/handoff2 remain unchanged; playable conversion and live
hybrid form integration remain disabled.

### Read Aura — independent learned Drow access

For an existing valid secondary Drow record, preview with
`selfbrid drow readaura`, then use `selfbrid drow readaura learn` to buy the
native `DROW_READ` power for **6,000 secondary DP**. The checked lesson save
preserves original-class DP and training; a failed save does not charge.
An uncertain durability result blocks another lesson until save recovery.

Use `readaura <character|carried item>`. A learned Drow source supplies the
same free reading as native Drow, even when the original class has an
untrained or more restrictive aura route. Learned Monk Mantra2 and Drow
sources do not stack, add extra readings, or duplicate costs. Without a
learned source, the original class's prerequisites, Mage mana charge and
Werewolf restrictions remain unchanged. No new Shield/Scry access is granted.
Target formatting, guardian feedback, illusion handling and item identification
are unchanged. Record4/save epoch4/handoff2 are unchanged; playable conversion
and live hybrid forms remain disabled. Existing custom world helps retain priority.

### Heightened Senses — independent learned Drow access

For an existing valid secondary Drow record, `selfbrid drow heighten`
previews the lesson. `selfbrid drow heighten learn` costs **20,000 secondary DP**,
the native prayer price, through the existing checked save transaction.
Pre-commit save failure preserves points and training. Uncertain durability
applies the lesson once and blocks another lesson until save recovery.

Use `heighten` to toggle the existing shared sight flag. Activation is free
and retains the native six-pulse wait; dismissal is free and imposes no wait.
An earned Drow source and Monk Mantra1 share one toggle, not two bonuses.
Original-class prerequisites remain the fallback when neither source is learned.
Missing, rejected or untrained secondary records never supply a grant.
Turning the flag off remains possible after training loss. An already-active
legacy flag is not automatically revoked or assigned a new source; this stage
does not add automatic provenance migration. Sorcerer vision and item-based
sight remain independent. No new ranks, costs, schema or form permissions are
introduced. Record4/save epoch4/handoff2 are unchanged; playable conversion
and live hybrid forms remain disabled. Custom world helps retain priority.

### Spider Eyes — independent learned Drow Scry

For an existing valid secondary Drow record, preview with
`selfbrid drow spidereyes`, then use `selfbrid drow spidereyes learn` to buy
the existing flag for **7,500 secondary DP**, the native prayer price.
The checked lesson transaction preserves points/training on pre-commit
failure; uncertain durability applies once and blocks lessons until recovery.
Original Drow characters continue using native `prayer`.

Use `scry [target]`. A learned Drow source supplies one free use, independent
of the original class's untrained or more expensive Scry route. Learned
Monk Mantra2 and Drow Spider Eyes do not produce extra readings or bonuses.
With no learned source, native prerequisites and the Mage-only 500-mana
charge remain unchanged. Invalid or untrained secondary records grant nothing.
The already-integrated Spider Eyes avoidance passive remains unchanged;
this stage does not add a second passive or modify combat calculations.

Shielding, item-vision exceptions, staff privacy, original Werewolf/Vampire
advantages, Guardian warnings, visibility and remote-room restoration retain
their native rules. Spider Eyes is not an item-vision grant or shield bypass.
No flag is activated or retained by using Scry; losing training removes this
route on the next command without changing independent native grants.
No class swap, teleport, schema, save epoch or handoff change is introduced.
Conversion and live hybrid forms remain disabled; custom world helps retain
priority. This is not a live-host or PvP balance certification.

### Blind Fighting — learned Drow passive

For an existing valid secondary Drow record, preview with
`selfbrid drow blindfight` and learn with `selfbrid drow blindfight learn`.
The native unlock costs **5,000 secondary DP**, saved through the same checked
lesson transaction. Failed saves do not apply the lesson or charge; uncertain
durability blocks another lesson until save recovery.

The passive removes the existing 25% unseen-opponent penalty from the
character's avoidance and avoidance-breaking roll. It does not grant sight,
reveal hidden targets, bypass targeting/safety checks, add attacks, or remove
the unarmed/contact avoidance reduction. Seeing an opponent already avoids
that penalty; learning the power adds no bonus in that case. Both sides of a
fight check their own independent grant, with unchanged RNG draws and native
integer truncation. There is no activation command, active flag, upkeep or
stacked multiplier. Native/original Drow retain their existing learned access.

An untrained or invalid secondary record grants nothing. Losing the learned
flag removes the benefit on the next calculation without cleanup or rewriting
stored training. No record, save-epoch or copyover-format change.
Conversion and live hybrid forms remain disabled; custom world help still
takes precedence over generated help.

### Poison Immunity — learned Drow passive

For an existing valid secondary Drow record, preview with
`selfbrid drow poisonimmunity`, then use
`selfbrid drow poisonimmunity learn` to buy the native unlock for
**5,000 secondary DP**. Original Drow characters continue using `prayer`.
The checked lesson saves only the secondary bank and its DP payment.
Save failure refuses the lesson without payment; uncertain durability applies
it once and blocks further lessons until save recovery.

There is no activation command or upkeep charge. The learned source blocks
the shared poison spell and the Drow crossbow's poison-induced sleep.
Physical bolt damage, ammunition consumption and the shooter's 500-DP price
are unchanged. This does not grant general sleep, toxin, spell or damage
immunity; Vampire and Bone Gnawer poison-spell exemptions remain independent.
It neither cures existing poison nor wakes a previously sleeping character.
Removing training removes only this source on the next attack; there is no
active flag, delayed cleanup or new save field.

Native and original-parent Drow behavior is preserved. Invalid, rejected,
missing or failed-load records and stale foreign primary flags grant nothing.
Conversion/live forms remain disabled; custom world helps retain priority.

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

### Secondary Monk Spirit Guardian

`spiritguardian` is the existing **warning toggle**, not a pet, an extra
attacker or a transformation. A validated secondary Monk needs learned
Mantra6 on the full Mantra path; other secondary paths cap at4 and cannot
activate it, even if a migrated raw rank is higher. Original Monks retain
their normal ranks and access.

Activation accepts either an existing native grant or the new learned
secondary-Monk grant. Original Vampire Necromancy4 and original Werewolf
Children of Gaia4 remain sufficient, independently of secondary training.
The ranks are never added. One native `MORE_GUARDIAN` toggle supplies the
existing warnings; having two eligible parents grants no second guardian,
extra warning or stronger effect. Secondary Vampire/Werewolf power adapters
are not added by this Monk stage.

Native costs and timing remain: **zero mana**, no new random roll or recovery,
and the existing standing/minimum-level interpreter entry. Ignored command
arguments remain ignored. Invalid secondary records cannot activate it.
As a separate native safety repair, valid player actors can dismiss an active
guardian even after class/training loss or record rejection; doing so only
removes the toggle. Missing actors/data are rejected without a crash.

Tests preserve240 valid native snapshots, compare **4,860 paired** commands
plus **5,184 overlap** replays, check10 invalid states, exercise9 real Read Aura
warning consumers and **90 full/short** player loads. Unrelated flags, class,
learned banks, mana and the actor list stay intact. This is not a PvP balance
test. The four old context/stuck-dismissal failures were reproduced before
the native repair, separately from the adapter.

Guardian lifecycle reconciliation now supplies **automatic revocation** when no
supported grant remains. On the **next main-loop pulse**, before input, an
ineligible player's toggle clears silently, including link-dead, editing and
switched originals. Another valid native or learned secondary-Monk grant keeps
the same single toggle active; ranks are never combined. NPC prototype flags
are not modified. Qualifying for the power does not automatically reactivate
an expired guardian: the player must issue `spiritguardian` again.

Both player loaders reconcile after class/training and the optional hybrid
record are validated. Older or staff-edited flags without a current grant clear
on load. A **pure save** projection masks unsupported guardians from disk without
mutating the live actor during a successful or failed write. No timestamps,
costs, new record fields or save/handoff epoch change are added. This is
not a provenance migration: the flag's historical source is not guessed;
current supported grants determine whether it may remain active.

Lifecycle tests reproduce four old pulse/save/full/short-loader failures, then
compare 2,970 command/eligibility cases, 11,880 connection-state pulses, 20,790
pure projections, 32 rejected-record/native-grant combinations and **630 full/short**
loads, including legacy flags forcibly retained on disk. No other flags,
learned banks or resources are changed.
No conversion, clan-role expansion or live forms
are enabled. Record4/save epoch3/handoff2 remain unchanged. Generated help is
updated; custom world helps keep priority and no live help import is performed.

### Explicit secondary-Monk training controls

Only an already attached, validated secondary-Monk record can use this UI.
Original Monks continue to use native `monklearn`, with its existing full
ranks, caps, prices and ordinary save timing unchanged.

- `selfbrid monk` lists the three effective/stored ranks, caps and next prices.
- `selfbrid monk mantra` displays one path without buying anything.
- `selfbrid monk focus mantra` previews a free, permanent full-path choice.
- `selfbrid monk focus mantra confirm` atomically saves that choice, granting no ranks.
- `selfbrid monk mantra gain` buys exactly one rank for **10 x next rank** primal.

The path names are `tech`, `mantra` and `art`. The chosen path has cap8;
the other two have caps4. Confirmed focus cannot be changed through this UI.
Pick and confirm a focus before buying ranks. Every lesson uses the existing
secondary-only transaction backend: own connected body, standing, out of
combat, valid training data, enough primal and no pending primary-save recovery.
Native primary banks, currency unrelated to the lesson and class identity
are never swapped or normalized. Over-policy stored ranks are displayed at
their effective caps and require staff review rather than silently changing.

Failed saves apply neither payment nor lesson. If the new file is visible but
directory durability is uncertain, the lesson/payment are applied once and
further purchases wait for primary-save recovery; **do not repeat** the lesson.
Neither training nor choosing focus automatically activates a guardian or
other power. However, not every secondary power has been integrated: conversion,
Invite Beast, Chimera and other live hybrid forms remain disabled/unadapted.
This UI does not add casting permissions or those missing power adapters.

Real-engine tests cover27 parent/focus paths, **432 paid** rank lessons and27
free focus transactions, **918 full/short** path loads, all nine original-Monk
pairings using unchanged native `monklearn`, malformed input, cap/raw-rank
guards, missing/switching/invalid actor contexts, save failures and uncertain
durability recovery. Secondary Mantra6 enables explicit `spiritguardian` only
on the full Mantra path; training alone never turns it on.
Record4/save epoch3/handoff2 are unchanged. Generated help is updated, but
custom world helps keep priority; no live help-file import is performed.

### Secondary Werewolf family training (existing records only)

`selfbrid wolf` is available only to a character with a valid attached
secondary-Werewolf record. It leaves native `gifts` and primary Werewolf data
alone. The player permanently chooses one of **Breed, Auspice or Tribe** as
the full secondary family; the other two families cap at3. Within the full
family, the selected gift follows its native cap5 and other gifts keep their
native cap4. Gifts in the other families cap at3. The free family and gift
choices require an explicit `confirm`; neither grants a rank.

- `selfbrid wolf` shows the secondary gift ranks, saved choices, effective
  caps and next-rank prices.
- `selfbrid wolf focus breed` previews the permanent full-family choice;
  append `confirm` to save it.
- `selfbrid wolf gift tribe black-furies` previews the permanent gift choice
  for that family; append `confirm` to save. The catalog displays all valid
  gift keys.
- `selfbrid wolf gain tribe black-furies` buys one rank. Native gift pricing is
  retained: **10 x next rank** for the selected gift in a family, otherwise
  **15 x next rank**.

Choose a full family before training. The existing atomic lesson transaction
requires the player's own connected body, standing and out of combat, valid
training state, enough primal, and no pending primary-save recovery. It writes
only the secondary Werewolf banks and the lesson cost; class identity and native
primary banks are not swapped or rewritten. Failed saves charge and train
nothing. An uncertain directory sync applies the lesson once and blocks
another purchase until primary-save recovery; do not repeat it.

This only wires saved ranks and their passive effects already supported by the
hybrid access layer. **Not all active Werewolf commands or forms accept a
secondary Werewolf yet**, and conversion remains disabled; do not interpret
this trainer as complete hybrid playability. Original Werewolves continue
using their native `gifts` command and normal behavior.

### Secondary Monk Read Aura (existing records only)

`readaura <target>` now accepts learned secondary **Mantra2**, on any legal
full-path choice (including the other paths' normal cap4). It reads a character
in the same room or identifies a carried object. This is the **free Monk source**:
it does not need the original parent's Auspex, Glass Walkers, Sora, Dragon,
Drow or Mage aura power, and Mage originals do not pay the Mage-only500 mana
when using a qualified Monk grant. Werewolf originals can use the Monk source
to identify carried objects and read characters despite native Cybersenses'
Werewolf-only restrictions. Existing Homid5 warning stealth is retained.

Original Monks keep their full native access. Without a qualified Monk grant,
all native class/item checks, resource costs, failure messages and restrictions
remain unchanged. Fake-condition displays and existing guardian warnings
remain intact; this is read-only and does not alter an item's owner or enchants.
Untrained/rejected secondary records grant nothing, while original-class
access continues independently. Native class macros/getters, training,
command tier and save formats remain unchanged.

A separate safety repair rejects invalid actor context before lookups and
handles missing/oversized NPC descriptions without passing them through the
legacy message expander. Long aura labels are clipped with their NPC suffix
preserved; observers get a generic description. This is not a repair of
other shared message-formatting callers, which still need a separate audit.
The shared `act`/`act2`/`kavitem` layer has since been bounded; see
[message formatting](MESSAGES.md) for coverage and remaining callers.

The real-engine suite retains **960 native** snapshots and compares
**15,552 paired** parent/order/focus/rank/target/mana cases,90 rejected-record
and original-access cases,12 independent native/Monk overlaps and
**72 full/short** loaded characters. Six pre-existing crash boundaries were
reproduced locally before the safety repair and pass afterward.
Invite Beast, Chimera and other unfinished
secondary adapters remain separate work. No conversion or live hybrid forms
are enabled; no world/help import is performed and custom world helps keep
priority.

## Native Scry safety

Scry now rejects missing actor/player/room context before parsing, treats a null
argument as an empty survey and reports a missing target room without borrowing
it. Survey labels and room names are bounded, with useful fallbacks for missing
names. These repairs follow **six sanitizer failures** reproduced against the
previous handler; all nine malformed-context/display boundaries now pass.

The **1,440 native** command snapshots retain identical output, mana, room
membership, temporary shadow/globe flags and RNG results. Native class/item
grants, Mage's500-mana policy, shields, staff privacy and guardian warnings
are unchanged. No conversion or new class permission is enabled by this
standalone safety repair.

### Secondary Monk Scry

Existing valid records with learned **Mantra2** can use `scry` to survey online
players or `scry <target>` to view a target's room. This is the **free Monk
source**: it does not borrow the original class's native Scry training or
vision item, and a qualified Mage/Monk does not pay the Mage-only500 mana.
Both parent orders are supported; original Monks keep full native access.

The original class's native/item grants remain independent when the secondary
rank is untrained, capped below the requirement or rejected. The secondary
grant adds no shield bypass, staff visibility, remote attacks, free ranks or
automatic activation. Native Auspex/Lupus survey bypasses and Homid guardian
warning stealth remain native original-class properties; named-target shield
and vision-item rules are unchanged. Room membership, shadow/globe flags and
RNG state are restored exactly as for the native command.

The real-engine suite keeps the **1,440 native** snapshots unchanged and runs
**23,328 paired** parent/order/focus/rank/target/mana commands,90 rejected-record
and original-access cases,9 native-grant overlaps and **72 full/short** loads.
No conversion, new save schema, live hybrid form or world/help import is enabled.
Other unfinished secondary powers remain separate work.

### Secondary Monk Shield / Mindblock

Existing valid records with learned **Mantra2** may use `shield` or its native
alias `mindblock`. No full-path focus is required for rank2; both parent orders
work, and original Monks retain native training and access. Secondary access
uses only the owned effective Monk rank, never an unrelated raw primary bank.
Malformed/rejected secondary records grant no activation. The original class's
native Vampire Obfuscate6, Dragon shield skill, Silver Fangs4 and Mage Mind3
routes remain independent. A vision item alone does not grant Shield.

This is the existing **free, self-only toggle**: no mana/primal/quest fee,
new wait, RNG, ranks or flags. `shield <name>` retains its native ignored-
argument behavior and never targets that name. Both aliases operate on one
shared `MORE_SHIELDED` effect; grants do not stack or multiply protection.
Native Scry survey/named-target shield rules and item/staff bypasses remain
unchanged. Native Read Aura does **not** check this shield; this adapter does
not silently add that new protection.

Already-active shields may now be **manually dismissed even after training,
class or secondary-record eligibility is lost**. This does not allow turning
them back on without a current grant. The handler now rejects missing actor/
player context before reading it, and does not parse unused/null arguments.
These repairs are separate from secondary access. No automatic eligibility
revocation or legacy provider-provenance migration is included: an existing
flag remains until manually dismissed or cleared by normal game behavior.

The saved flag and hybrid record use the existing full/short player loaders;
no schema, epoch or handoff changes. Player saves retain ordinary timing,
not a new crash journal or powercut guarantee. Live forms,
full secondary command coverage and playable conversion remain separate work.

### Secondary Monk Heightened Senses

Existing validated secondary Monks with learned **Mantra1** may use `heighten`
(Heightened Senses / Eyes of God). Every Monk focus can reach rank1. The
adapter reads the effective owned secondary rank, not unrelated raw primary
training. Original Monks keep native access and ranks. Native Dragon, Drow,
Highlander, Mage, Ninja, Vampire and Werewolf grants remain independent when
the secondary grant is absent, untrained or rejected.

This is the native **free, self-only** holy-sight toggle: activation retains its
**six-pulse wait**, dismissal has no added wait, and neither spends resources
or rolls RNG. Arguments remain ignored; `heighten <name>` never targets another
character. One shared raw holy-sight flag is toggled, without stacking,
changing other sight flags or adding provider ownership. Sorcerer vision's
separately owned holy, shadow and night sources are preserved and still
provide their normal effective sight after Heighten is turned off.

A separate safety fix rejects missing actor/player context and permits
**manual dismissal after training or class eligibility is lost**, without
allowing reactivation. Null/unused arguments remain safe; no room is required
by this self-only command. Existing raw flags have no reliable provenance,
so this is not an automatic source migration or eligibility-revocation sweep.

Tests retain1,428 native snapshots, compare15,552 paired real-engine commands
across both parent orders, verify432 rejected-record cases and144 full/short
reloads, and exercise activation timing and independent Sorcerer sight.
The old handler reproduced two invalid-context sanitizer failures and ten
lost-eligibility dismissal failures before repair.

The shared saved flag and hybrid codec remain unchanged (record4/save epoch4/
handoff2). Ordinary player-save timing is not a crash journal or powercut
guarantee. Custom world helps retain priority over generated fallback help.
Invite Beast, Chimera, full secondary access, playable conversion and live
hybrid forms remain separate work; no PvP balance claim is made.

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

## Original-class mastery preview

`selfbrid info <full hybrid name>` now combines numeric training, original-class
mastery and the **50,000 QP** balance into a read-only prerequisite preview.
“Requirements met” is **not conversion authorization**. No conversion, payment,
automatic training, normalization, saving or form change occurs. Existing hybrid
records and rejected player data block a new-conversion preview.

Original-parent requirements use raw native training, never the secondary
“one full branch, remaining branches rounded-up half” limits:

| Original class | Permanent mastery checked |
|---|---|
| Vampire | Basic, selected clan and unlocked disciplines; native caps 7 clan / 5 other, clanless 6, Thaumaturgy always 5 |
| Werewolf | All 3 breed, 5 auspice and 13 tribe gift trees: selected gift in each family 5, others 4; native advancement rank 4 |
| Sorcerer | All six colors at 5 |
| Drow | All 17 prayer-granted unlocks and speed 4 |
| Highlander | Weaponry, Attack, Defense and Powers at 6 |
| Mage | All nine spheres: three selected branches 5, others 4; Arete 10 |
| Dragon | All 13 learnable skills, chosen native color, Wyrm age |
| Ninja | Sora, Chikyu and Ningenno at 10 |
| Monk | Techniques, Mantras and Arts at 8 |
| Elementalist | Choose a focus: focused element 10, opposite 6, other six elements 8 |

Werewolf advancement now costs primal and XP, not completed quests. Any hybrid
with a Werewolf parent pays double the normal primal/XP prices and needs double
the cumulative monster kills, including when Werewolf is the original parent.
The rank-4 cap and normal gift caps are unchanged. See [Werewolf commands and
advancement](WEREWOLF.md) for the exact rank table and Shift compatibility rules.

Complete original build choices first: three distinct Mage spheres, three distinct
standard Vampire clan disciplines, all three Werewolf gift choices, and an
Elementalist focus. Caitiff/Panders use their native clanless caps instead of a
three-discipline clan bonus. Custom social clan names do not replace or grant
selected power branches. Optional locked Vampire rank0 powers are **unavailable
disciplines**, exempt until taught/unlocked; rank -1 means known but untrained.
Basic and selected clan disciplines cannot be skipped by zeroing their ranks.
Reserved/disabled discipline slots are not trainable requirements; nonzero data
there needs staff review. This adds no teaching command or unlock.

Wyrm is the native age trainer's last attainable age, not the staff-only Ancient Wyrm.
Invalid choices, duplicate branches, negative/over-cap ranks and unknown unlock
bits require review rather than silently counting as mastered. Ordinary unfinished
choices/training are reported as missing. Do not repair a save by blindly setting
every storage slot to a maximum: arrays also hold unrelated timers/resources.

Current HP/mana, blood, DP, ki, primal, quintessence, exp, active claws/fangs,
temporary forms and active Drow globe/Dragon wings/horns flags are not permanent
mastery requirements. Max HP/mana must still each be at least 50,000; the five base
stats, stances, weapon proficiencies and currently trainable styles retain the
existing numeric rules. Gear is neither inspected nor granted. No new generic
spell or resource-pool grinding requirement is introduced.

Native trainers, original power access, secondary caps, book limits, inventory,
Record4/save epoch3/handoff2 are unchanged. The read-only preview must not be
used as an authorization token or bypass a fresh commit-time check.

### Atomic conversion backend (not player-enabled)

`hybrid_player_convert_commit` is a tested backend transaction, not a command.
At the commit boundary it rechecks the canonical parent pair, live unswitched
descriptor/body, standing and out-of-combat state, save-recovery status, the
complete original-parent mastery/numeric requirements, and the 50,000-QP
balance. It constructs a fresh secondary bank from the class initializer;
callers cannot inject trained ranks or copy/normalize primary-class storage.

The candidate class record and QP deduction are serialized together through
the primary atomic player save. Before-rename failure leaves the conversion
and payment unapplied (the ordinary primary-save retry flag may still be set).
After-rename directory-sync uncertainty applies the state and price exactly
once, marks primary durability recovery, and rejects another conversion.
Success keeps the original class ID and all primary training/resources intact;
only the secondary record is attached and 50,000 quest points are charged.
The next normal backup checkpoint remains separate, so there is no
multi-destination atomicity or power-loss journal guarantee.

`selfbrid` deliberately does not call this API yet. Secondary ability/command
coverage, class-specific resource upkeep, clan identity and live forms are not
complete; enabling conversion now would expose a hybrid with a partial power
set. Existing hybrids are also blocked from repeating the transaction. Player
conversion remains disabled until those runtime gaps are closed and audited.
