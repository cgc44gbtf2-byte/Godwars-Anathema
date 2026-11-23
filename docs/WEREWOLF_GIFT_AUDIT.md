# Werewolf gift help audit

Scope: the 105 native Werewolf help slots, their command registrations,
rank gates, passive consumers, and overlap descriptions. This began as a
help/test-only change. Disquiet, Quicksand, Lycaon, Scream of Gaia, Metis Totem
Gift, Reach, Wrath and Uktena Invisibility were
subsequently restored as described below; training costs and rank caps remain unchanged. Disquiet's
saved affect identity advances compatibility to save epoch 4; Quicksand is
runtime-only and adds no saved affect or identifier. Lycaon is runtime-only;
its temporary display morph and polymorph bit are omitted from saves.
Totem Gift uses only transient runtime state: the stationary object, owner's
barrier pool and pulse timer are not serialized.

## Status legend

| Marker | Meaning | Slots |
| --- | --- | ---: |
| A: command | A registered handler contains gameplay behavior; not a safety certification | 65 |
| P: auto | A passive has an identified production consumer | 28 |
| G: upgrade | Modifies an existing command, not an extra automatic action | 1 |
| V: unverified | No matching dedicated effect located; absence is not proven universally | 9 |
| U: unimplemented | No separate implemented gift found for that slot | 1 |
| X: unavailable | Legacy handler exists but is not registered for player commands | 1 |
| D: disabled | Unregistered legacy handler also explicitly has no gameplay effect | 0 |

`wwpowers` is a command list, not proof that every listed command works.
Disquiet was the identified registered stub in the initial audit. It now has
the explicitly selected hitroll/damroll effect, rather than the EM calm effect.
Red Talons 5 Lycaon is a transient display-only wolf appearance, not a form
grant: it costs 1000 mana, lasts 30 pulses, and never alters saved identity or
wolf-form combat stats.
Get of Fenris 4 Scream of Gaia is a non-damaging room interruption: 5000 mana,
one pulse of target interruption, six pulses of target stun immunity, and two
pulses of caster recovery. It affects only eligible foes, not allies or safe
targets, and cannot refresh an active interruption.
Metis 5 Totem Gift is a stationary tribe-themed object; it does not fight or
follow. It costs 5000 mana and absorbs up to 10000 incoming damage for its
owner over 10 pulses. It clears on pool depletion, expiry, room departure,
class loss or loss of Metis 5; successful calls impose 12 pulses of recovery,
while rejected calls do not spend mana or recovery.

## All trees, in rank order

Read each five-letter sequence as ranks 1 through 5. The individual names,
short descriptions and command spellings are in `Genesis/area/class.hlp`.

| Tree | Ranks 1–5 | Principal corrections / evidence |
| --- | --- | --- |
| Homid | A A A A A | Persuasion 1, Reshape Object 3, restored Disquiet 4; `wwbreed.c` |
| Metis | A A A A A | Porcupine requires war-form; Totem Gift creates a stationary owner-only barrier |
| Lupus | A A P A A | Beast Life 4, not shield-piercing scry; elemental attacks do not summon pets |
| Ragabash | A A P A P | Ordinary track suppression; silver vulnerability modifier, not universal immunity |
| Theurge | A G A A A | Mother gains self-targeting at 2; Grasp moves both actors; Lobotomy drains mana |
| Philodox | P P P A A | Disguise identity at 2; damage cap/break-avoidance at 3; Geas targets NPCs |
| Galliard | A V A A A | No dedicated Distractions consumer found; Bridge Walker can misdirect travel |
| Ahroun | V P P P A | No separate Razor Claws damage bonus verified; initiative at 2, silver claws at 3 |
| Black Furies | A A P P A | Aeolus 2, natural-style poison 3, opening charge 4, Body Wrack 5 |
| Bone Gnawers | P P A A V | Poison-spell resistance and rating-scaled soak; no separate Survivor effect verified |
| Children of Gaia | A P A A A | Avoidance/armor 2, Dazzle 3, Guardian warning 4; Guardian is not a summoned pet |
| Fianna | A A A A A | Form Illusion 3, Clone Object 4, Balor's Gaze 5; no Spriggan body form |
| Get of Fenris | P P P A P | Flee blocking scales from rank 1; acid blood at 3; Scream of Gaia interrupts eligible foes |
| Glass Walkers | A A A A A | Read Aura is inspection, not a vision toggle; Surge attacks eligible room targets |
| Red Talons | V V P A A | Claw-parry claims not verified; combat rage ceiling at 3; Quicksand snare at 4; transient appearance curse at 5 |
| Shadow Lords | P A A A A | Clap is active; Dominator drains rage; no separate Open Wounds power |
| Silent Striders | P V P U A | Initiative at 1 and 3; no verified Fortitude consumer or distinct rank-4 gift; Reach 5 pulls an eligible near-Umbra target out |
| Silver Fangs | V V A A X | Wrath 3 frightens eligible hostile vampires; Shield 4; Mastery unavailable; no implemented Luna's Avenger body form |
| Uktena | A P V A A | Shroudnight 1; underwater movement permission 2; flight claim not verified; ordinary Invisibility restored at 4 |
| Wendigos | A A A P A | Cutting Wind is not typed cold damage; Invoke has specific weather modes |
| Stargazers | A P P P P | Hit/damage scaling, rating-based Obfuscate sight, avoidance and probabilistic redirection |

## Source map

- Active Breed handlers: `Genesis/src/classes/werewolf/wwbreed.c` and
  `Genesis/src/classes/werewolf/totem.c`.
- Active Auspice handlers: `Genesis/src/classes/werewolf/wwauspice.c`.
- Active Tribe handlers, including rank-5 Silent Striders Reach:
  `Genesis/src/classes/werewolf/wwtribe.c`.
- Shared commands and rank gates: `clan.c`, `more.c`, and `interp.c`.
- Every `<auto>` slot has an exact production consumer recorded in
  `tests/test_werewolf_help_catalog.py`, including combat, movement, visibility,
  regeneration and spell consumers. A descriptive upkeep message alone is
  deliberately **not** accepted as proof.
- The tests check all 105 slots, canonical command/tree/rank correspondence
  with `wwpowers`, actual command registrations and function definitions,
  explicit gap classifications, the Mother upgrade, and passive consumers.

## Overlaps and remaining risks

Grandmother's Touch uses the same Mother handler and cooldown. It is an upgrade,
not a second healing trigger. There are 65 unique canonical registered names
in the gift menu, including the restored Disquiet, Quicksand, Lycaon, Reach, Wrath, Invisibility,
Scream of Gaia and Metis Totem Gift.

Get of Fenris 1, Shadow Lords 1 and Philodox 3 retain their separate +100
damage-cap contributions. Lupus 3, Children of Gaia 2 and Stargazers 4 retain
their separate avoidance contributions. Spirit Ward and Shield reuse aura
shielding; reusing a flag or command does not imply two independently stacking
toggles. No overlap was removed or rebalanced.

Nine unverified entries need behavioral tests or a design decision before being
advertised as working gifts. Old upkeep text still contains unsupported claims
and is not authoritative. Helios fire-protection checks vary by spell handler;
the help intentionally does not promise universal fire immunity. Post-hit
actor liveness, multi-target callbacks, arithmetic boundaries and world-template
dependencies remain separate runtime-safety work, not validated by this audit.

Protection descriptions are deliberately narrow: Resist Toxin identifies the
standard poison spell, and Heart of Ice identifies the standard frost-breath
spell. The native Werewolf Halo does not receive the Elementalist-only
Sunshield soak branch, so its help does not promise that soak bonus. Spirit
of the Fish describes underwater movement permission, not an unverified
universal breathing/drowning immunity.

No secondary Werewolf transformation or fusion is enabled. These native help
entries do not establish secondary-parent command permissions. Binary updates
preserve live area files: review and merge the relevant help entries into a
custom live `class.hlp` separately. Do not overwrite customized world files
merely to apply this documentation update.

## Missing-power comparison with the other sources

Reviewed the existing local snapshots of Gathering 1.01, GodWars 1996,
GodWars 2k6, GWLow5, Myth 3.0, AtD 2.0, Ethereal Mists, LoC, TSOB/Dreams of
Demise, Vortex and VW 1.0.2. Duplicate Myth uploads are counted once.
This is an archival source comparison, not a fresh pull or a successful build
of those projects. No foreign C code has been imported.

| Anathema gap | Source evidence | Assessment |
| --- | --- | --- |
| Disquiet | EM `werewolf.c:do_disquiet`, also GW2k6, Myth and TSOB; `MORE_CALM` is consumed in interpreter/update code | Real calming behavior exists. Best first restoration reference; must add current target-safety and lifecycle checks rather than copy it unchanged |
| Disquiet alternatives | GWLow5 and AtD `garou.c:do_disquiet` apply hitroll/damroll affects | Different debuff design, rank/resource schema and semantics; not interchangeable with the EM version |
| Razor Claws | GWLow5 `garou.c:do_razorclaws` and `fight.c` explicitly select different damage dice when claws and its flag are set | Real combat reference, but foreign gate is above rank 1 and archived flag declarations are malformed; needs an original, tested adaptation |
| Invisibility | EM/GW2k6/Myth/TSOB register `do_invisibility`; EM invokes the existing invisibility spell | Native Uktena 4 restored after shared visibility/ownership fixes and caster-state review; zero resource cost, existing timed affect and recovery preserved |
| Luna's Avenger | AtD/LoC `garou.c:do_lunasavenger`; `GAROU_AVENGER` is read in their combat damage paths | A real body-form candidate, not a drop-in. Foreign slot/rank differs and Anathema's existing Shield must not be displaced; hybrid form ownership/balance still need design |
| Lambent Flame | AtD `garou.c:do_lambent`, registered through `data/commands.dat` | An active room-blinding action, not the missing native rank-2 avoidance passive; different rank, costs and target checks |
| Quicksand | AtD `do_quicksand` is data-registered and spends gnosis to toggle `WOLF_QUICKSAND` | The native Red Talons 4 version is now a temporary, single-target movement snare; the archived toggle was not copied because its flag had no verified consumer here |
| Totem Gift | GWLow5/LoC contain declarations of `do_totemgift` | Implemented natively as a stationary tribe-themed object with an owner-only damage pool; archived declarations were not copied |
| Scream of Gaia | Several projects register `do_scream`, but the examined handlers require Vampire/Melpominee powers | Same command name, wrong class/power. Not evidence of a completed Werewolf Scream of Gaia |
| Other unverified passives | No dedicated effect or consumer was confirmed for the remaining entries | Keep honest gap labels; do not infer implementation from help text |
| Reach | Native rank-5 handler restored as the Silent Striders command; it extracts an eligible target from near Umbra under current target-safety rules | No cost/recovery was added; foreign names are not needed |
| Wrath | Native Silver Fangs 3 handler restored with caster/target guards and the corrected target-Beast check | Retains normal fear, rank-based hit/damage penalties and zero mana cost; 20-pulse recovery only when a target is affected |
| Mastery | Native handler remains unbound. Foreign names also occur in Angel, Shadow or legend systems | Name collisions are not Werewolf implementations and must not be imported as substitutes |

For Razor Claws, some related snapshots only expose flag/display changes in the
searched paths; the GWLow5 combat consumer is the distinguishing evidence.
AtD uses a data-driven command registry, so an empty static-table index was
not treated as proof that its commands are unavailable.

The initial recommendation was EM-style Disquiet. The selected implementation
instead uses a hit/damage debuff, independently written for native state.
Treat Razor Claws as a separate balance
decision, and postpone new body forms until the form-ownership integration is
ready. Review licensing/credits and test native/hybrid caps, shared effects,
safe targets, persistence and cleanup before any foreign-derived behavior ships.

## Selected Reach restoration

`reach <target>` is Silent Striders 5. It removes only the ordinary near-Umbra
flag from an eligible same-room, non-fighting target; the caster remains in the
normal plane. Deep Umbra targets, safe rooms, protected/non-PK/AFK/link-dead
players, protected NPCs and malformed or removed actors are rejected. The
existing handler had no mana charge or recovery, so none was introduced. The
command adds no new serialized state. Rank 4 remains explicitly unassigned.

## Selected Wrath restoration

`wrath` is Silver Fangs 3. Its existing two fear affects reduce hitroll and
damroll by the learned rank, capped at 5, with the same random 1–2 duration in
ordinary affect updates. Normal expiry removes them on the update after zero.
Only eligible hostile, awake, online PK vampires with positive Beast qualify;
the caster's Beast is no longer used. Safe rooms, allies, protected players,
peace protection even during combat, incompatible planes, existing fear and
underflowing stat subtraction are rejected. No mana is charged. The existing
20-pulse caster recovery now applies only when at least one target is affected.
This is not a stun or a direct flee/damage command; existing fear consumers are
unchanged. Normal affect saves and stat restoration are preserved.

Real-engine normal and sanitizer fixtures cover 45 rejection guards, 192
ordinary casts, 192 arithmetic boundaries, multiple and protected room targets,
non-refresh across casters, actual expiry, and full/short saved reloads.
Exact inverse fixtures pin all other production/help bytes to the prior head.
No new hybrid permission, training price, saved field or migration is introduced.

## Remaining Mastery safety audit

Silver Fangs 5 Mastery remains unregistered and unavailable to players. Its
legacy handler checks primary Werewolf class, gift rank, a nearby Werewolf
target and strictly lower Garou rank, then passes the remaining text directly
to the target's interpreter. It has no gift-specific command allowlist or
forced-command context. This is not an enabled player exploit.

Twenty-two isolated real-engine cases, repeated under ASan/UBSan, used only
the harmless `config +brief` command to demonstrate the authorization gap.
The setting changed for a lower-ranked staff target, non-PK players, group
members, link-dead targets, peace-protected targets and targets in safe rooms.
A sleeping, dead, recovering or invalid-save caster could invoke the handler
directly; a sleeping or waiting target could execute this configuration command.
Existing Dominate resistance did not block it, and `conf +brief` also resolved.
The ordinary gift-rank, primary-class and equal-Garou-rank denials still worked.

The target interpreter independently rejected AFK, frozen and combat-paused
targets, but Mastery still charged the caster's 12-pulse recovery. Empty
command text also charged recovery. No mana was consumed. These findings
do not establish that every possible forced command succeeds: target trust,
posture and interpreter protections still apply. Destructive commands were
not executed, and no live characters, saves or services were used.

Restoration needs an explicit permitted-action policy, target/caster guards
and callback-safe completion handling. Prefer a small exact action allowlist
over a blacklist around unrestricted interpretation. No Mastery binding or
runtime change is part of this audit.

## Initial Uktena Invisibility safety audit

Before restoration, Uktena 4 Invisibility was unregistered. Its legacy handler invokes the
ordinary `invis` spell directly. This charges no mana, DP or primal, applies
one 24-update affect and imposes 12 pulses of recovery. It is not a new form
or a separate concealment system. Rank 3, wrong-primary-class and NPC denials
work; a normally flagged existing invisibility source is not refreshed.

Sixteen isolated activation cases reproduced missing direct-handler checks:
dead, sleeping, recovering, AFK, combat-paused and invalid-save casters could
apply the affect. Ordinary interpreter checks may independently reject some
of these states, but the handler itself does not enforce them. Safe-room,
zero-mana and fighting activation also work; those are observed policies,
not automatically defects for a self-only power. If an invis affect remains
after its flag is cleared, another activation can duplicate the affect.

The pre-fix shared `can_see` path did not consult ordinary `AFF_INVISIBLE`.
A normal same-room observer without detection could still see the affected
Werewolf. This is a shared visibility issue, not a problem confined to an
unregistered Werewolf command. No claim is made here about every client,
display or targeting path.

The pre-fix shared affect-removal path also cleared the invisibility flag while an
invisibility-granting armor item stays equipped. Conversely, removing that
armor while the ordinary invis spell remains preserved the spell and flag
in the tested configuration (actual invis skill index 46). Blanket claims
that all removal orders are broken would be incorrect.

Real affect updates preserved the flag while decrementing 24 to zero and
removed it on update 25; normal `visible` removed the affect and flag.
Training loss and a subsequent class change left the ordinary timed spell
active, rather than treating it as a continuously owned class toggle.
Full and short saved reloads preserved the affect, duration and flag, and
stripping the reloaded effect cleaned them. These checks passed normally and
under ASan/UBSan against the unchanged production engine. Expiry fixtures
reset idle timers to avoid unrelated automatic logout; persistence used only
temporary synthetic players. No live characters or services were used.

These results blocked registration until the following restoration. The audit
itself made no runtime changes; the subsequent fixes preserve separate
Obfuscate, Kakusu, Mage invisibility and special-vision policies.

## Selected Uktena and shared invisibility restoration

`invisibility` now dispatches to native/original-parent Werewolves with Uktena
4 or 5. It rejects invalid saves, dead/sleeping/non-standing, absent, AFK,
recovering and combat-paused casters. Already-invisible characters and existing
`invis`/`mass invis` affects cannot be refreshed or duplicated, even if a raw flag was lost.
Safe-room self-casting remains allowed; no mana, DP or primal is charged.
Success retains 12 pulses of recovery and the ordinary 24-update spell affect,
removed on the update after zero. Training/class loss retains the already-cast
spell's normal lifetime. No new fields or hybrid permissions are introduced.
Staff `invis` remains earlier in the command table to preserve its priority.

Shared ordinary invisibility now hides targets from ordinary observers and
character lookup. Detect Invisibility, holy sight and existing staff/item-vision
shortcuts retain their roles; detection does not bypass separate concealment,
blindness or incompatible-plane rules. This does not redesign other stealth.
Removing an affect or equipment source retains other live spell, object-affect,
prototype-affect and implicit equipment invisibility sources. Expiry retains
equipment invisibility. Existing explicit `visible` reveals are unchanged.

The real-engine fixture checks 150 class/sight cases, 98 two-source/removal-order
combinations, 19 Uktena rejection cases, rank-4/5 zero-cost casts, non-refresh,
normal expiry, original-parent access and full/short reloaded cleanup.
Exact inverse checks reconstruct the preceding six production/help files,
while preserving all unrelated bytes and the older source proofs.

## Selected Disquiet restoration

`disquiet <target>` requires native/original-parent Werewolf Homid 4.
At Homid 4 it reduces raw hitroll and damroll by 20 each with affect duration 4;
Homid 5 gives 25 each with duration 5. Standard zero-duration cleanup happens
on the following affect update, not immediately when the counter reaches zero.
Existing effects cannot
be stacked or refreshed, even by another caster. No mana/primal charge was
added; successful and resisted attempts impose a 12-pulse caster wait.

It permits eligible NPCs and PK-enabled player targets, never self, protected
staff, dead targets, safe-room targets or targets rejected by the normal
combat-safety checks. Existing Presence immunity and ten-point willpower
resistance apply to players; Guardian warnings remain independent.
The effect does not charm, order, interrupt commands or grant secondary
Werewolf access. Shared raw-stat arithmetic outside application/removal
remains a separate engine-wide concern.

This partially overlaps Whelp's Body, not an identical duplicate:
Whelp also changes armor, uses a different strength/duration and retains its
existing access rules. Its code is unchanged. Disquiet cannot stack with
another Disquiet; other existing debuffs retain normal affect stacking.

The two named affects use the standard save/load, dispel and tick-expiry
paths, preserving their remaining ticks across relog/copyover. Offline time
does not age ordinary affects. Existing skill indices and object spell slots
are unchanged; the new affect name is appended in previously unused space.
Save epoch 4 prevents binary-only rollback to older readers that do not
understand that name. Handoff version 2 is unchanged.

## Follow-up: native offensive callback boundaries

Elemental Gift, Invoke's lightning, Cutting Wind and Power Surge now validate
room membership before using callback arguments again. A multi-hit sequence
ends if its actor/target is removed, moved or unable to continue; a Surge ends
if its cached next room member disappears. Cutting Wind never rolls/applies its
follow-up stun after losing the target. Explicit Elemental/Invoke target names
take precedence over the current opponent; unknown names fail without a hit.

The dedicated full-engine suite captures 1,064 native/original-hybrid and
secondary-denial snapshots, including messages and RNG. Test-only callback
injection deliberately frees synthetic NPCs or moves/interrupts actors; the old
handlers reproduce sanitizer failures at those boundaries. This establishes
the callback contract, not every live death script or recycled-address case.
Ordinary damage ranges, access, costs, weather modes and cooldowns are unchanged.
No additional missing power, secondary Werewolf command adapter or new body
form is enabled.

Body Wrack and Clap additionally guard post-retaliation/death callbacks and
nonpositive-HP targets. Clap validates cached next members and sends stun feedback
to the actual victim. Their 1,216 ordinary mechanics/RNG snapshots match the
previous engine; 210 message hashes change solely for that feedback correction.
Controlled callback tests model removal, movement and interruption, not an
exhaustive live death-script audit. Existing damage, access, odds, cooldowns and
Clap's protected-target early-return policy remain unchanged.

The remaining Tribe room loops now include checked Breath of the Wyld healing,
post-flee Banshee membership checks, and post-command Obedience cleanup (both
single-target and room-wide). Obedience command input and announcements are
bounded. A further 912 ordinary engine cases pin existing effects, resistance,
messages and RNG; controlled callback tests cover extraction/movement of caster,
target and cached next member. No additional gift or secondary-parent permission
is enabled in that pass. Call of the Wyrm and then-undispatched Wrath of Gaia
were left unchanged there. Wrath's subsequent guarded restoration is described
above; Call of the Wyrm's transformation policies remain outside that work.

Galliard utility follow-up: Call of the Wyld now bounds reports and validates
its room/opponent, while retaining wounded calls and the existing recipients.
Bridge Walker's fallback is finite even when no world room qualifies; after
4,096 random misses it scans the original range and either finds a legal landing
or stays put. Its rank error now correctly says Galliard 5. Eight hundred
ordinary native/original-parent cases preserve messages and RNG; extremely sparse
worlds may reach the new deterministic fallback. No access/cost/rank expansion,
world-file replacement or Wyrm transformation redesign is included.
