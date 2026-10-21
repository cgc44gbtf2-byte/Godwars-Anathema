# Werewolf gift help audit

Scope: the 105 native Werewolf help slots, their command registrations,
rank gates, passive consumers, and overlap descriptions. This began as a
help/test-only change. The Disquiet restoration below is the sole newly
implemented gift; training costs, rank caps and existing permissions remain
unchanged. Its saved affect identity advances compatibility to save epoch 4.

## Status legend

| Marker | Meaning | Slots |
| --- | --- | ---: |
| A: command | A registered handler contains gameplay behavior; not a safety certification | 58 |
| P: auto | A passive has an identified production consumer | 28 |
| G: upgrade | Modifies an existing command, not an extra automatic action | 1 |
| V: unverified | No matching dedicated effect located; absence is not proven universally | 9 |
| U: unimplemented | No separate implemented gift found for that slot | 3 |
| X: unavailable | Legacy handler exists but is not registered for player commands | 4 |
| D: disabled | Unregistered legacy handler also explicitly has no gameplay effect | 2 |

`wwpowers` is a command list, not proof that every listed command works.
Disquiet was the identified registered stub in the initial audit. It now has
the explicitly selected hitroll/damroll effect, rather than the EM calm effect.

## All trees, in rank order

Read each five-letter sequence as ranks 1 through 5. The individual names,
short descriptions and command spellings are in `Genesis/area/class.hlp`.

| Tree | Ranks 1–5 | Principal corrections / evidence |
| --- | --- | --- |
| Homid | A A A A A | Persuasion 1, Reshape Object 3, restored Disquiet 4; `wwbreed.c` |
| Metis | A A A A U | Porcupine requires war-form; Totem Gift is not implemented |
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
| Get of Fenris | P P P U P | Flee blocking scales from rank 1; acid blood at 3, not a Thor command |
| Glass Walkers | A A A A A | Read Aura is inspection, not a vision toggle; Surge attacks eligible room targets |
| Red Talons | V V P D D | Claw-parry claims not verified; combat rage ceiling at 3; two disabled handlers |
| Shadow Lords | P A A A A | Clap is active; Dominator drains rage; no separate Open Wounds power |
| Silent Striders | P V P U X | Initiative at 1 and 3; no verified Fortitude consumer or distinct rank-4 gift |
| Silver Fangs | V V X A X | Shield 4; Wrath/Mastery unavailable; no implemented Luna's Avenger body form |
| Uktena | A P V X A | Shroudnight 1; underwater movement permission 2; flight claim not verified |
| Wendigos | A A A P A | Cutting Wind is not typed cold damage; Invoke has specific weather modes |
| Stargazers | A P P P P | Hit/damage scaling, rating-based Obfuscate sight, avoidance and probabilistic redirection |

## Source map

- Active Breed handlers: `Genesis/src/classes/werewolf/wwbreed.c`.
- Active Auspice handlers: `Genesis/src/classes/werewolf/wwauspice.c`.
- Active Tribe handlers: `Genesis/src/classes/werewolf/wwtribe.c`.
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
not a second healing trigger. There are 58 unique canonical registered names
in the gift menu, including the restored Disquiet.

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
| Invisibility | EM/GW2k6/Myth/TSOB register `do_invisibility`; EM invokes the existing invisibility spell | Anathema already has a handler but no player binding. Re-enabling requires permission, visibility ownership and cleanup review, not a blind alias |
| Luna's Avenger | AtD/LoC `garou.c:do_lunasavenger`; `GAROU_AVENGER` is read in their combat damage paths | A real body-form candidate, not a drop-in. Foreign slot/rank differs and Anathema's existing Shield must not be displaced; hybrid form ownership/balance still need design |
| Lambent Flame | AtD `garou.c:do_lambent`, registered through `data/commands.dat` | An active room-blinding action, not the missing native rank-2 avoidance passive; different rank, costs and target checks |
| Quicksand | AtD `do_quicksand` is data-registered and spends gnosis to toggle `WOLF_QUICKSAND` | Only toggle/message and expiry consumers located. No movement/combat slowdown consumer verified; not a proven complete replacement |
| Totem Gift | GWLow5/LoC contain declarations of `do_totemgift` | A prototype is not an implementation; no complete matching handler found |
| Scream of Gaia | Several projects register `do_scream`, but the examined handlers require Vampire/Melpominee powers | Same command name, wrong class/power. Not evidence of a completed Werewolf Scream of Gaia |
| Lycaon / other unverified passives | No matching complete native-equivalent implementation established in this pass | Keep honest gap labels; do not invent behavior or infer implementation from help text |
| Reach / Wrath / Mastery | Native handlers already exist but are unbound. Foreign `wrath`/`mastery` names also occur in Angel, Shadow or legend systems | Name collisions are not Werewolf implementations and must not be imported as substitutes |

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
No missing power, secondary Werewolf command adapter or new body form is enabled.

Body Wrack and Clap additionally guard post-retaliation/death callbacks and
nonpositive-HP targets. Clap validates cached next members and sends stun feedback
to the actual victim. Their 1,216 ordinary mechanics/RNG snapshots match the
previous engine; 210 message hashes change solely for that feedback correction.
Controlled callback tests model removal, movement and interruption, not an
exhaustive live death-script audit. Existing damage, access, odds, cooldowns and
Clap's protected-target early-return policy remain unchanged.
