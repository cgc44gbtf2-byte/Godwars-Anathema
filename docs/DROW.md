# Native Drow: commands, gear and verification

This covers original/native Drow characters. It does not add secondary powers,
change class identity or enable hybrid conversion.

## Learning

Use `prayer <person> <power>`. Training yourself is allowed; another Drow must
belong to your house. The payer spends DP, the recipient receives the gift.
Already learned gifts are not charged again. Speed has four ranks.

| Prayer power | DP | How it is used |
|---|---:|---|
| AcidBolt | 5,000 | `acidbolt <target>` |
| LlothsWeb | 8,000 | `web <target>` |
| Darkness | 9,500 | `globedarkness`; 500 DP activation, 150 DP upkeep |
| FaerieFire | 5,000 | `faeriefire <target>` |
| SpiderBite | 7,500 | Existing combat proc and damage-cap contribution |
| DualWield | 10,000 | Wear a second weapon in the off hand |
| Blindfight | 5,000 | Existing avoidance behavior while blinded |
| SummonSpider | 8,000 | `summonspider`; one shared summoned follower slot |
| SpiderEyes | 7,500 | `scry [target]`; existing avoidance-break contribution |
| Readaura | 6,000 | `readaura <character or carried item>` |
| Darkvision | 5,000 | `darkvision` / `nightsight`; one shared toggle |
| Heighten | 20,000 | `heighten`; existing sight toggle and lag |
| PoisonImmunity | 5,000 | Existing poison and crossbow-sleep protection |
| SkipDisc | 5,000 | `skipdisc`; existing flight, speed and avoidance behavior |
| Attunement | 1,000 | `attunement <owned item>`; existing exclusions apply |
| Might | 8,000 | Existing damage multiplier and damage-cap contribution |
| Toughness | 8,000 | Existing soak contribution |
| Speed | 10,000 per rank | Four ranks; existing combat calculations |

`drowupkeep` lists learned gifts. `dimensiondoor <target>` retains native room
and target restrictions. House and communication commands are unchanged.

## Class equipment

The crafting command is **`drowarmor`**, not `classeq`.

| Recipe | Equipment slot | Cost |
|---|---|---|
| `drowarmor cloak` | About body | 8,000 DP + 400 quest points |
| `drowarmor helmet` | Head | 8,000 DP + 400 quest points |
| `drowarmor breastplate` | Body | 8,000 DP + 400 quest points |
| `drowarmor bracer` | Wrist | 8,000 DP + 400 quest points |
| `drowarmor crossbow` | Held | 8,000 DP + 400 quest points |
| `drowarmor bolts [1-20]` | Unworn ammunition | 100 DP each |

The crossbow uses `shoot <direction> <target>` and compatible carried bolts.
Crafted relics keep their maker/owner assignment. Costs, template stats, effects,
wear slots and ownership rules are not rebalanced.

## Repairs in this stage

- Spider dependencies are checked before creating a mob or reserving a follower.
  A missing guardian template now refuses safely rather than exiting the server.
- The spider is created directly in the summoner's room; the unrelated
  disconnection room is no longer needed.
- Follower ownership and charm are established before summon speech can run a
  room-text callback. Speech is now last; its visible ordering therefore changes.
- Relic recipes reject wrong object types, missing take/wear flags, negative
  weights and unsafe carry-counter arithmetic before creating or charging.
- Roomless, NPC and null direct calls to these two handlers refuse safely.

## Regression coverage

`tests/test_native_drow.c` boots a private copy of the **shipped world**:
all 17 prayer purchases, affordability/duplicates, Speed's cap, native training
reload, five actual relics worn/reloaded, owned attunement, dual wield, Faerie
Fire, Web, Acid Bolt, poison protection, dimensional-door room safety, native
sight/flight/Globe toggles and spider creation/extraction are exercised.
Failure cases cover a missing guardian, malformed relics, carry arithmetic,
invalid follower counts, roomless calls, missing disconnection room and a real
summon-speech teleport callback.

Existing full-engine native snapshots continue covering combat/passive formulas,
Acid Bolt/Web/Scry/aura/vision behavior, equipment effects/restrictions, ammunition
compatibility, ranged safety, resource use and persistence.
An exact source inverse preserves the previous complete Drow source hash; its
negative tests reject unrelated stat/cost/guard changes.

### Native Speed training/reporting follow-up

`drowupkeep` now shows Speed even if it is the only trained gift and always ends
the Speed entry with a line break. Its numeric output is bounded, including the
largest and smallest stored `long` values.

Ranks outside 0–4 are reported as invalid, not silently repaired. `prayer` refuses
to charge for increasing an invalid Speed rank; normal ranks still cost 10,000 DP
and stop at four. No save-format or combat-formula change is made.

Tests cover Speed-only and mixed reports at every legal rank, invalid-rank
refusals, read-only reporting, same-house versus different-house prayer, payer
affordability, duplicate grants, and the recipient's power/Speed reload.

This is not proof of live-server configuration, every custom OLC edit, PvP
balance, or crash/power-cut durability. Clan membership administration and every
possible hostile room-text callback are not newly audited here. A cross-player
prayer still uses ordinary separate player saves, not an atomic two-player
transaction. The existing
shared flag/follower conventions are retained rather than replaced.

### Native Speed combat validation

All five native Speed combat reads now use a single rank getter. Legal ranks
0–4 keep their existing timing, damage roll, damage cap, avoidance and
avoidance-breaking formulas. Invalid ranks contribute zero Speed benefit and
remain stored unchanged for staff review; no automatic player-file repair or
save-format migration is performed.

The combat regression pins 120 legal profiles across six equipment styles,
Skip Disc on/off and wounded/unwounded states to the original engine's results.
It also checks 96 invalid-rank projections against the matching untrained
profile, including `LONG_MIN` and `LONG_MAX`. The old engine reproduced signed
overflow in damage roll and attack timing before this repair.
