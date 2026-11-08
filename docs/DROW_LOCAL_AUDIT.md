# Local native Drow audit

Uncommitted investigation notes. No GitHub publication or deployment.
Selfbrid feature work remains paused.

## Follow-up — crafting, attunement and spider stat boundaries

The initial follow-up added tests and documentation. After approval, a narrow
Summon Spider stat guard was added locally. Earlier local fixes are preserved.

### Equipment checks passed

- Bolts reject zero, negative, signed, malformed, excessive and extra-argument
  batches without changing DP, quest points, wait, inventory counts or weight.
- Missing funds from `LONG_MIN` through 99 DP reject the one-bolt recipe.
- Damaged ammo templates (wrong type, missing take flag, negative weight or
  negative spell encoding) and negative/overflowing carry counters reject
  without charging or creating items.
- One- and 20-bolt batches cost exactly 100 DP per bolt, no quest points or
  wait, have zero resale/quest value, retain their maker and remain unbound.
  Extraction restores inventory counts/weight.
- Attunement rejects a carried item made by the caster but bound to another
  owner, unowned items, all seven excluded item types and artifacts. A missing
  owner field now fails closed without calling the engine's null-string
  diagnostic path. Rejections preserve flags and resources. A valid owned item
  attunes, and repeating the command makes no further change.

These are the exercised cases, not a claim of every possible equipment
combination or a general shared-flight/source-ownership audit.

### Additional command/equipment review

Reviewed the native Drow prayer, upkeep, Darkness, Faerie Fire, spider,
attunement, relic/bolt crafting and crossbow paths for target validation,
resource timing, arithmetic bounds and cleanup. Crossbow behavior is covered
across six directions, safe-target refusals, poison immunity, misses/deaths,
room restoration, ammo ownership and compatibility.

### Fixed locally: stale crossbow/ammunition prototypes

An engine-level regression reproduced a malformed-template case: an already
loaded missile or bolt retained its runtime `item_type` after its object
prototype was changed to another type. `shoot` trusted only the live object
type, so the stale item could still consume 500 DP and one bolt. The regression
failed before the fix.

The shot-ammo selector now requires both the live item and its current
prototype to be `ITEM_MISSILE` for the weapon and `ITEM_AMMO` for the bolt.
Missing prototypes, incompatible live objects, and prototype/runtime type
mismatches reject before the shot charge, projectile extraction, damage or
combat effects. Valid custom missile templates, legacy bolt-reference
normalization, and the existing 500-DP/one-projectile costs remain unchanged.

The real-engine ammo regression passed after the fix, including the two stale
prototype cases and existing ownership, compatibility, safe-target, barrier,
miss/death, persistence and crafting coverage. A strict source inverse restores
the historical Drow source hash and rejects mutations to either new type guard.
This is a bounded equipment audit, not a claim that every command or
engine-wide callback path has been exhaustively verified.

### Fixed locally: Summon Spider stat subtraction overflow

`do_summonspider` copies the caster's computed hitroll, damage roll and armor
with a subtraction of 50. Each subtraction is undefined when its computed
input is below `LONG_MIN + 50`. Three independent full-HP native probes set
one input to `LONG_MIN`; the pre-fix focused UBSan probes exited 1 at lines 285, 286 and
287 respectively. The failure is in the summon assignment itself, not the
shared stat getter. These are seeded extreme/inconsistent stats, not evidence
of a live player crash or a normal-training exploit.

Twenty-four representable cases (eight values for each stat, including
`LONG_MIN + 50`, `LONG_MIN + 51`, ordinary negatives/zero and `LONG_MAX`)
preserve the exact native minus-50 result, free activation and follower-slot
cleanup. No broader stat-getter repair or balance change was made.

The approved fix computes each stat once and rejects any value below
`LONG_MIN + 50` before `create_mobile`. Normal minus-50 results and free summon
costs are unchanged; no clamp or new balance cap is introduced.

Verification after rebuilding the changed Drow module:

- Default fixture: **660 real-world command checks**, normal and focused
  ASan/UBSan, passed. Instrumentation covers Drow, handler, magic, clan and
  update modules; this is not a full-engine sanitizer claim.
- All three formerly failing `--spider-stat-*` modes now assert safe rejection
  at both `LONG_MIN` and `LONG_MIN + 49`. These six cases also run by default.
  They assert an explanatory message, unchanged character-list links, no pet
  or follower reservation, no wait and unchanged resources/inventory.
- The 24 representable boundary summons retain exact stats and cleanup.
- All 23 native catalog tests, full `make check-layout`, production build and
  `git diff --check` passed. Historical source pins remain unchanged through a
  strict inverse with negative mutation tests.

The earlier red UBSan logs remain in `/tmp/drow_spider_extreme_{hitroll,damroll,armor}.log`.
Post-fix normal/focused-sanitizer logs are `/tmp/drow_spider_fixed.log` and
`/tmp/drow_spider_fixed_san.log`. No commit, GitHub push or deployment was made.
Shared stat-getter overflow and other malformed-data paths are outside the
spider fix. Attunement's missing-owner case was separately made an explicit
fail-closed check and added to the equipment regression.

## Skip Disc — independent ownership fixed locally

The original interpreter/world audit reproduced:

- Skip Disc clears `AFF_FLYING` supplied by a live Fly affect.
  The spell affect remains attached, but effective flight is lost.
- It also clears flight while the granting item remains worn.
- Removing Fly or removing a flight item can clear an active disc's flight.
- If Skip Disc training is removed while flying, dismissal is refused and
  the raw flight bit remains set.

The native disc now owns a separate `DROW_DISC` active-state bit in the existing
saved Drow power bank. It is not a learned gift or a secondary-parent grant.
Effective flight combines that source with raw spell/equipment flight without
copying the disc's flag into their shared `AFF_FLYING` bit. Class and PC guards
restrict this source to native Drow; class reset already clears the bank.

Toggling the disc never clears another source. Removing Fly or a flight item
does not cancel an active disc. A native Drow may dismiss their disc after
losing its training, but cannot reactivate it without the gift. Activation and
dismissal remain free, without wait; the existing learned speed bonus is unchanged.
Flight items equipped during disc flight still establish their own raw grant
(both item spell encodings 3 and 39). This equipment-order regression was
reproduced failing before its two-line guard repair.

Read-only native mastery reports ignore the new active-state bit, just as
they ignore the active Darkness globe. This is bookkeeping, not resumed
selfbrid feature work or new secondary-parent access.

Verified locally:

- Full native fixture: 596 real-world command checks, normal and focused
  ASan/UBSan. Instrumented native/handler/magic/clan/update modules are not a
  claim of full-engine sanitizer coverage.
- Travel and 72 blocked-exit cases cover all six directions and air/deep-water
  terrain: missing exits, closed doors, water/granite walls and occupied
  private/solitary rooms.
- Independent spell/item/disc removal in both activation orders, both flight-item
  encodings, combined worn-item/disc save/reload, and saved training-loss dismissal.
- Null/NPC/non-Drow guards, original bitmask return semantics, and unchanged
  native mastery inputs/results when only the active disc is toggled.
- Class reset/reload clears the marker; 240 native reset snapshots, 100 bank
  reset/reloads and 200 real class selections passed.
- Cross-class runtime, access, Web and mastery regressions, full
  `make check-layout`, production build and exact historical source inverses.

Limits: existing unmarked saved raw flight cannot reliably be attributed to a
disc, spell or item. It is preserved, not automatically cleared or migrated;
ambiguous legacy records need explicit review. General overlap bugs solely
between other raw flight providers are not repaired by this disc-specific fix.
No automatic training-loss/death revocation policy was added. The save format
and compatibility metadata are unchanged; an older binary will not recognize
the new native active marker as disc flight, so do not assume active-disc
continuity when downgrading.

## Darkness — native upkeep and cleanup

### Confirmed defects and current status

1. Native `mobile_update` subtracts 150 before checking whether DP is exhausted.
   A globe loaded with `LONG_MIN` DP causes signed overflow. The focused UBSan
   reproduction exits 1 at `update.c`'s `CP_CURRENT -= 150`.
   Activation itself correctly rejects insufficient/negative funds; the
   reproduction seeds an already-active corrupt/extreme record. Ordinary
   nonnegative balances did not reproduce this overflow.
   **Fixed locally:** compare against 150 before subtracting. Balances at or
   below upkeep become zero and take the existing globe-dismissal path;
   balances above upkeep pay exactly 150. The former `LONG_MIN` reproduction
   now passes focused ASan/UBSan. Activation costs and exhaustion messages
   are unchanged.
2. With an active globe but no learned `DROW_DARK` gift, the command refuses
   dismissal. The next native upkeep pulse keeps it active and charges 150 DP
   (1,000 to 850 in the reproduction). This is a manually simulated training
   loss/inconsistent record, not evidence that normal class-reset commands
   leave this state. **Fixed locally for native Drow:** an active globe may
   be dismissed even without `DROW_DARK`. Without the gift, a second command
   cannot reactivate it, regardless of DP. Automatic revocation on training
   loss was not added; the player must dismiss it or normal exhaustion/death
   cleanup must run. Secondary-parent behavior was not expanded.

### Verified existing behavior

- Activation costs 500 DP, with no wait; manual dismissal costs nothing.
- Twelve activation boundaries from `LONG_MIN` through `LONG_MAX` reject or
  charge correctly.
- Twelve upkeep boundaries, including `LONG_MIN`, -1, 0, 149, 150, 151 and
  `LONG_MAX`, now safely charge 150 or clamp to zero and clear the globe.
- Movement retains the globe on its caster and does not write room light or
  `ROOM2_DARKNESS` flags. No room-state residue was observed on departure.
- Full player save/reload retains an active globe and its DP.
- `update_pos` at zero HP clears the globe.
- Native mobile upkeep does not charge a disconnected character. The tested
  globe and balance remain active/unchanged for that pulse. This is a policy
  observation, not proof that offline elapsed time should be billed.

Reproduction modes in `tests/test_native_drow.c`:

- `--darkness-lifecycle`: real activation, pulse, movement, save/reload,
  disconnection and zero-HP checks, followed by training-loss cleanup checks.
- `--darkness-extreme`: isolated `LONG_MIN` upkeep regression; now asserts a
  zero balance and dismissed globe, and passes focused ASan/UBSan.
- `--darkness-dismissal`: seven training-loss balance cases from `LONG_MIN`
  through `LONG_MAX`: dismissal is free and immediate, unrelated gifts and
  resources are preserved, reactivation is denied, no subsequent upkeep is
  charged, and saved cleanup reloads without a globe.

The native upkeep arithmetic and native training-loss dismissal permission
are now fixed locally. The full native fixture includes the formerly failing
minimum balance and training-loss cleanup. Activation still requires the gift
and 500 DP. Disconnected upkeep, secondary-parent permissions and save formats
are unchanged.

Validation: the old upkeep reproduced a UBSan failure (exit 1); the old
dismissal failed the new native regression (exit 134). The fixed extreme case
and lifecycle/dismissal checks passed focused ASan/UBSan. All 575 real-world
native command checks, 21 native catalog tests, `make check-layout`, and the
production build passed. Exact inverse tests restore the prior sources
byte-for-byte and reject cost or permission mutations; historical source
hashes were not replaced. Focused instrumentation is not a full-engine
sanitizer run.

No commits, GitHub writes, live-server changes or deployment were performed.

## Drow relic crafting — template and carry rejection review

Reviewed the four armor recipes and the crossbow recipe in `do_drowarmor`.
The command checks the expected prototype item type, `ITEM_TAKE`, the recipe's
wear slot, nonnegative weight, and safe carry-number/weight headroom before
creating an object or charging 8,000 Drow points and 400 quest points.

Added a local regression matrix for each recipe covering wrong prototype type,
missing take flag, missing required slot, negative template weight, and
carry-number/weight overflow. Every rejection preserved both balances and left
the inventory and carry counters unchanged. The existing valid-crafting
boundary checks still passed with the exact established costs and ownership.
The Drow relic path exposed no additional defect, so this pass made no
production-code change.

Verification: `tests/test_class_gear` and `tests/test_drow_ammo` passed, along
with `git diff --check`. This remains local audit work only; no GitHub write,
commit, live-server change, or deployment was made.
