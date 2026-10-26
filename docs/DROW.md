# Native Drow: commands, gear and verification

This covers original/native Drow characters. It does not add secondary powers,
change class identity or enable hybrid conversion.

## Learning

No deity, altar or trainer NPC is required. The target is the Drow **receiving**
the power:

```text
prayer
prayer self acidbolt
prayer self speed
prayer self dualwield
prayer <other-Drow-name> <power>
```

Bare `prayer` lists the menu; it does not buy a gift. Another Drow must be in the
same room and belong to your house. The payer spends DP, the recipient receives the gift.
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

## Houses and clan commands

House roles are separate from learned Drow gifts, Speed and class ranks.
Promoting or removing a house member does not grant or erase class training.
Staff first create a managed house with
`clanadmin create <online-Drow-leader> <house name>`. An existing legacy house
name must be adopted using that same name; a cached label alone is not a
managed leader/officer role.

| Command | Purpose |
|---|---|
| `clan info` / `clan roster [page]` | See your managed house, role and registered members, including offline members |
| `dinduct <player>` | Leader/officer offers membership to an available Drow in the same room |
| `clan accept <inviter>` / `clan decline` | Recipient explicitly accepts the named offer or declines it |
| `promotion <member>` / `failure <member>` | Leader appoints or removes an officer |
| `outcast <member>` | Remove a lower-ranked member of your own house, online or offline |
| `clan transfer <member>` | Leader offers leadership; the member must accept |
| `clan leave` / `clan disband confirm` | Leave as a non-leader, or explicitly disband as leader |

Offers expire after 120 seconds and are cleared on registry reload/server
restart. Inviter and recipient must use their own awake, standing,
out-of-combat sessions; eligibility and authority are checked again at
acceptance. Being in another house, on another plane or hidden from each
other prevents an offer.

The atomic clan registry is authoritative. Offline removals override stale
player-file house labels on the next load. A failed save before replacement
keeps the old roles; uncertain durability after replacement freezes further
writes until staff repair the storage issue and use `clanadmin retry`.
Do not edit live player files to assign managed roles.

The native-house regression uses real interpreter input, saved players,
registry reloads, offline removal/relogin, leadership transfer and unchanged
Drow training. Atomic-save failures and recovery are also tested explicitly
for Drow, in addition to the existing ten-class governance checks. This audit
does not change runtime permissions, ranks, costs or save formats.

## Communication

`sign <message>` is the **global Drow class channel**, not a private house
channel. Drow from other houses (or without a house) can receive it; subscribed
staff can also read and send. Being in another room, on another plane or asleep
does not make this channel private. Do not use it for house-only secrets.
There is no separate native Drow house-chat command in the current command table.
`clan` manages membership and roles; it does not send house messages.

Use `channels` to see `+SIGN` / `-sign`, `channels -sign` to mute sending and
receiving, and `channels +sign` to rejoin. These preferences persist through a
normal player save and reload; `channels -all` / `+all` also include Sign.
Gagged, silenced or room-silenced senders cannot sign. These restrictions prevent
sending, not receipt. Ordinary non-Drow players do not gain access by enabling
the channel preference.

The actual-world regression routes messages through real playing descriptors,
including different houses, remote/sleeping Drow, staff, another class and a
descriptor still at login. It checks sender/recipient mutes, saved preferences,
literal message text and unchanged powers/DP. No channel permission or runtime
behavior is changed by this audit.

## Dimensional Door

Use `dimensiondoor <target>` to travel to a visible character in another room.
It is a native Drow command, not a prayer purchase. The existing command does
not spend mana or DP. Player destinations must permit travel with `summon on`;
`summon off` refuses it. This preference persists through a normal save/reload.

Safe, private and solitary destination rooms, illusion targets, eye mobs,
targets above the existing level-800 boundary, missing targets and same-room
targets are refused. Ordinary NPC targets do not require the player summon
preference. Target lookup retains the shared visibility rules; this command
does not override blindness or plane visibility. A destination player's active
Spirit Guardian receives the existing warning when location protections or
summon consent block the attempt.

The real-world audit tests all 32 combinations of the three protected-room
flags, illusion and player consent, plus the individual lookup/level/NPC
boundaries. It uses real saved/reloaded player targets, checks Guardian notices,
room-list membership and unchanged HP/mana/DP/training. This is test coverage,
not a new travel policy or a blanket restriction on travel out of origin rooms.

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

### Equipment-use follow-up

The real-world suite now attunes all five crafted relics and checks ownership,
loyalty and the correct worn slot after reload. Repeated attunement is refused
without a charge; ammunition cannot be attuned.

Crossbow checks use the shipped bow and bolt recipes in all six directions:
twelve hits keep their original damage/RNG snapshot, consume one bolt and
500 DP each, apply existing sleep poison, and restore the shooter's room view
without cancelling stealth, Globe or silence. Protected targets consume neither
DP nor ammunition. Existing tests retain miss/barrier costs and compatibility,
owner, crafting and persistence rules.

`shoot` now refuses missing characters/player data before dereferencing them;
a null argument follows the normal syntax response. Three unsafe direct calls
were reproduced under UBSan before this guard was added. This is defensive
handler validation, not evidence of a reported live-server crash. Normal player
shooting, costs, damage and equipment rules are unchanged.

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
balance, or crash/power-cut durability. The house and Sign checks above do not
cover every possible hostile room-text callback. A cross-player
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
