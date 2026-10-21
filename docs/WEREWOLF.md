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

Other legacy gift descriptions still need a full source-by-source help review;
this patch does not certify every old passive or command description.
Binary updates preserve live area files. These are bundled help corrections,
not an automatic replacement of a server's customized `class.hlp`: review and
merge the help changes separately before reloading the live world.
