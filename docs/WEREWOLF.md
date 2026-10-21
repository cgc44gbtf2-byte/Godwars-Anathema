# Werewolf commands and advancement

## Advancement

`advance info` shows the next rank's requirements without spending anything.
`advance` buys one rank after every requirement is met. Completed quests and
quest points are not required or spent. Monster kills are **cumulative**:
they are checked, not deducted.

| New rank | Normal primal | Normal XP | Normal monster kills |
| --- | ---: | ---: | ---: |
| 2 | 100 | 1,000,000 | 1,000 |
| 3 | 200 | 2,000,000 | 2,000 |
| 4 | 300 | 3,000,000 | 3,000 |

Any valid hybrid with Werewolf as either parent uses double all three
requirements. These are per-purchase primal/XP prices, not lifetime totals.

| New rank | Hybrid primal | Hybrid XP | Hybrid monster kills |
| --- | ---: | ---: | ---: |
| 2 | 200 | 2,000,000 | 2,000 |
| 3 | 400 | 4,000,000 | 4,000 |
| 4 | 600 | 6,000,000 | 6,000 |

Rank 4 remains the solo advancement cap. Native advancement keeps
ordinary player-save timing: save after training; this is not a new journal or durable
payment transaction. The secondary Werewolf training backend uses the same
prices and its existing checked, single-player save transaction. That backend
does not itself enable player conversion or add a secondary advancement UI.

## Shifting

`shift werewolf` (also `shift wolf` or `shift crinos`) uses the existing Rage
command. Rage builds by its existing amount and may need several uses before
reaching 100 and transforming. Transformation retains its native hostile
effects: use care around other characters. No five-form system, free class
abilities, or additional combat bonuses have been added.

`shift human` (also `shift homid`) uses Calm: **Children of Gaia 1** is still
required, the player must stop fighting and stand, and several uses may be
needed to reduce rage below the transformation threshold. Bare `shift` chooses
the direction from the player's current form. The command does not give
secondary Werewolf hybrids access to a live Werewolf form.

## Gift command listings

Use `gifts` for learned ranks, `tribegifts` for native gift choices, and
`wwpowers` for command names. Black Furies and Bone Gnawers now use their own
Tribe ratings in that listing, rather than unrelated Auspice ratings.
Bodywrack is listed at its actual rank-5 handler requirement. Use `spiritward`,
not the unrelated Mage `ward` command; `reshapeobject`, `spiritguardian`,
`halosun`, `formillusion`, `shroudnight`, `clapthunder`, and `paralyzingstare` are the explicit command
names rather than ambiguous or nonexistent abbreviations.

Totemgift and Scream are unimplemented. Quicksand and Lycaon remain disabled. Reach,
Wrath, Mastery, and Invisibility have source handlers but are **unavailable**:
they are not registered as player commands and have not been exposed through
this compatibility fix. In particular, the old Mastery implementation needs a
separate review of forced-command and staff-target protections before use.

This review covers advancement, shift/Rage/Calm, command registration, gift
menu gates, malformed gift data and oversized selector/transformation input.
It is not a claim that every Werewolf gift's combat callbacks, target safety,
world dependencies, or live multiplayer balance have been fully validated.

## Help corrections and overlap audit

The bundled `class.hlp` now identifies Clap of Thunder as the active
`clapthunder` command, not an automatic effect. Its stun is a chance, not a
guarantee. The two placeholder gift descriptions are replaced with verified
behavior: Visage of Fenris adds 100 to the Werewolf damage cap, and the old
Open Wounds entry is replaced by the actual rank-4 Strength of the Dominator
command. The core Werewolf, Shift and Ranks entries now match the implemented
command names and advancement rules rather than unsupported five-form claims.

The 105 named Breed/Auspice/Tribe entries contain no repeated power names.
Available entries in `wwpowers` also have unique canonical command names.
This does not mean every mechanical benefit is unique: Get of Fenris 1,
Shadow Lords 1 and Philodox 3 each contribute a separate 100 damage-cap bonus.
Those existing, additive bonuses are retained; an overlap is not evidence of
an accidental duplicate grant. Shared commands such as `heighten`, `scry`,
`nightsight` and `readaura` are shared entry points, not extra power copies.
No powers, ranks, prices or combat bonuses were removed by this help audit.

All 105 gift slots now have a source-review classification; see
[the gift audit](WEREWOLF_GIFT_AUDIT.md) for evidence and remaining gaps.
Source review is not a claim that every command is safe in every combat state.

## Disquiet

Use `disquiet <target>` with Homid 4. It lowers hitroll and damroll by
`5 × Homid rank` each with standard affect duration 4 or 5.
Normal zero-duration cleanup occurs on the following affect update.
It cannot stack or refresh an active Disquiet. There is no new mana/primal
charge; success or Presence resistance imposes a three-second caster wait.
Only eligible NPCs or PK-enabled players may be targeted; safe rooms, staff,
self and normal combat protections are respected. Presence resistance can
consume ten willpower under the existing resistance-toggle policy.

The effect is saved, expires through ordinary affect ticks, and can be
removed through normal dispelling. Offline time does not age it. This does
not grant secondary-parent Werewolf access. See `deploy/UPDATING.md` before
updating: save epoch 4 blocks unsafe rollback to older effect readers.
Binary updates preserve live area files. These are bundled help corrections,
not an automatic replacement of a server's customized `class.hlp`: review and
merge the help changes separately before reloading the live world.

## Combat callback safety

Elemental Gift's earth sequence and Invoke's lightning sequence stop further
hits if the actor or target is removed, changes rooms or can no longer continue.
Cutting Wind does not roll for or apply a stun to a removed/moved target.
Power Surge validates the actor and cached next room member before continuing;
it stops safely if a callback removes that cached next member.

An explicit target for Elemental Gift or storm lightning is now honored even
while fighting someone else. An unknown named target fails rather than silently
substituting the current opponent. Omitting the target still uses the current
opponent, provided both characters remain in the same room.

No new gifts, secondary-parent permissions, damage ranges, rank requirements,
prices or ordinary cooldowns are added. Save epoch 4 and handoff 2 are unchanged.
Surviving native encounters retain their recorded HP, messages and RNG results.
The callback tests deliberately model extraction, movement and interruption:
they are not a claim that every native death script or live PvP interaction has
been exhaustively tested. Other gift arithmetic and callback paths still need
separate review.

### Body Wrack and Clap retaliation

Body Wrack and Clap now validate the caster after kill/retaliation callbacks;
Clap also validates its cached next room member before dereferencing it. A moved,
removed or interrupted caster cannot continue the room attack. Dead/nonpositive
HP targets are rejected or skipped before direct subtraction. Clap's stun
feedback now goes to the stunned victim instead of telling the caster they fell.

The real-engine suite records 1,216 native/original-parent/secondary-denial cases,
including actual NPC retaliation. HP, wait, position, fight links and RNG match
the old engine in every ordinary case. Exactly 210 message hashes change for the
intentional stun-feedback correction; the corrected full-output hash is pinned.
Separate test-only callback wrappers model removal/movement/interruption and
cached-next extraction. These are callback-contract tests, not proof that every
native death path frees its killer. Damage, stun odds, cooldowns and access remain
unchanged. The legacy protected-target policy still ends Clap's room wave rather
than skipping that target; this patch does not rebalance it. No new power or
secondary Werewolf access is enabled. Save epoch 4 and handoff 2 are unchanged.

### Other room-wide effects

Howl of the Banshee and both Obedience paths validate room membership after
flee/command callbacks. A vanished cached next target ends the wave safely.
Banshee restores the living caster's temporary autofollow setting even after
movement; Obedience clears a surviving NPC's temporary commanded flag in its
new room. Removed characters are not dereferenced during cleanup. An interrupted
or moved caster cannot continue imposing the room effect.

Breath of the Wyld keeps its healing formula and max-HP cap, with checked
addition at extreme HP and fail-closed malformed rank arithmetic. Obedience
rejects oversized command input and bounds its announcement formatting.
Normal targets, rank gates, cooldowns, immunity/resistance and resource policies
are unchanged. Secondary-parent Werewolf access is not added.

The real-engine suite pins 912 ordinary native/original-hybrid/secondary-denial
cases, including messages and RNG. Ten pre-fix failures were reproduced with
sanitizers (healing/input boundaries and test-only flee/command callbacks).
These injections establish callback contracts, not a claim that every ordinary
flee or forced command frees its participants.

Call of the Wyrm and the undispatched Wrath of Gaia were inspected but are
unchanged: Wyrm's existing visibility/shape toggles are not newly redesigned,
and Wrath remains unavailable through the command table. Power Surge and Clap
retain the earlier safeguards and targeting policies. Character-list integrity,
same-address recycling, broader forced-command authorization and live scripted
multiplayer interactions still require separate review.
