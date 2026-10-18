# Shared combat cleanup

This is a staged refactor, not a completed replacement of the combat engine.
The baseline is PR #1 commit `d523e560f8a923e36e84b839a4186be212d8d9b3`.
Class powers remain owned by `Genesis/src/classes/`; they are not duplicated
in the shared engine.

## Current ownership

| File under `Genesis/src/` | Responsibility |
| --- | --- |
| `stance.c` | Stance commands, autostance, stance training, combined skill display and stance raw-damage modifiers |
| `style.c` | Equipment-driven styles, `grip`, bounded proficiency getter, style and weapon training |
| `combat_state.c` | Starting/stopping fights and linked fight-list membership |
| `combat_rules.c` | Explicit-command and automatic safety policies |
| `combat_status.c` | Target liveness, action pauses and immunity/reuse timers |
| `fight.c` | Remaining attack loop, speed/defense calculations, damage, death/rewards and action commands |
| `core.c` | General formatted output and legacy item/output helpers; no fight-list management |
| `handler.c` | Equipment-derived flags and effects; styles call its existing refresh helper after hand promotion |
| `save.c` | Save format and load-time normalization, not combat decisions |

Both production and full-engine tests link the new owners exactly once.
`tests/test_combat_layout.py` enforces ownership, build and cleanup boundaries.

## Confirmed repairs in this batch

1. The loader preserves the legacy inactive stance marker `-1`, repairs
   active stance indexes outside `[-1,10]`, autostance outside `[0,10]` and
   active style indexes outside `[1,6]`. Invalid/legacy `0` styles become
   Unarmed; the full loader reconciles this with equipment *after* restoring
   objects, preserving a valid Two-Hand grip. It clamps proficiency entries to the
   engine-wide supported `[0,200]` range. It does **not** impose a new
   class-specific load-time cap or alter the serialized field layout.
   `skill` and avoidance-breaking also guard their indexes independently.
2. An attacker relaxing to no stance no longer bypasses the defender's
   Crab/Dragon/Swallow raw-damage divisor. Named stance delays and existing
   coefficients are unchanged. Crab's separate soak interaction is unchanged.
3. Moving a held weapon into an empty main hand refreshes equipment-derived
   flags immediately, including silver's left/right hand markers. The
   existing Two-Hand grip retention and shield/natural style rules remain.
4. Safety entry points reject null actors/targets and missing rooms *before*
   dereferencing them. `multi_hit()` also now reads actor style only after
   its null/pause guard. Explicit `is_safe()` and automatic `auto_safe()` have
   intentionally different policies; those policies have not been merged.

5. A separate command audit identified `relevel` reachable at `LEV_SLE`
   that raised a character to top staff level from a match against a
   hard-coded, player-registerable name. The command registration, handler
   and declaration have been removed. The existing staff-only `delevel`
   command and persisted staff accounts are not changed. Inspect live
   account/audit records separately if this code has been exposed publicly;
   repository tests cannot determine whether anyone used it.
    First-staff setup instead uses the operator-only offline bootstrap
    documented in `deploy/INSTALL.md`, after account creation and logout.

No new class powers, costs, gear stats, counters or save schema are introduced.
Malformed training values are repaired in memory; the usual successful
save path subsequently persists those repaired values.

## Regression coverage

`tests/test_stance_style.c` links the real engine with synthetic players,
descriptors, equipment and save fragments. It covers malformed indexes,
legacy `-1` and valid maxed training, defensive guards for values introduced
after loading, each affected defensive stance with an un-stanced attacker,
combat relaxation, silver hand promotion, grip toggling, shield versus claw
style selection, all eleven class/no-class style-cap profiles, five advanced
stance prerequisites, fight-list insertion/removal and safety entry guards.
It runs in both `make test` and `make sanitize`, without loading live accounts.
`tests/test_admin_gate.c` verifies the historical eligibility of the two
names without creating accounts; the static command-table test ensures the
name-gated elevation path is no longer available.
`tests/test_bootstrap_staff.py` uses synthetic files to check offline-lock
refusal, both role mappings, byte-preservation, protected snapshots, unsafe
paths/permissions, divergent credentials and interrupted two-file replacement.

## Remaining staged work

The request covers the shared combat system, not just these initial owners.
Next extract and characterize defenses/initiative, damage application,
death/rewards and generic combat commands from `fight.c`. Give each a single
owner and keep legacy public entry points compatible. Spell dispatch remains
in `magic.c` and class abilities remain in their class modules; audit their
engine boundaries instead of moving unrelated code wholesale.

Before each extraction, retain exact function-body comparisons and native
characterization tests. Keep callback instrumentation used by restored-spell
tests intact. Run the full normal/sanitizer suites, catalog/layout checks,
production build, isolated TCP/TERM and actual copyover smoke checks.
CI covers Debian 12/13 and Ubuntu 22.04/24.04.

## Optimization rules

No throughput improvement is claimed for a file split. Profile real shared
combat paths before optimizing. Preserve RNG draw order, training side
effects, per-step floating-to-integer truncation, attack costs and safety
checks. Do not cache proficiency across a multi-hit round: training can change
it during that round. Natural, Two-Hand and Shield modifiers still apply
after the base damage cap; whether to rebalance that order is a separate
decision. Legacy warnings, long multiplayer sessions and live balance are
not proved safe by isolated unit tests.
