# Sorcerer components, recipes and apprenticeship

Adapted from Gathering 1.01 `class_mage.c` for Anathema; not a faithful port. Gathering `do_teach` rewrote `ch->level` and trust; that pattern is deliberately NOT copied.

## Components

Stored in new `pcdata->components[3]` (runes, glyphs, sigils bit masks), saved as a `Components` line read by both the full and short loaders. A missing line loads as zero, so existing saves are unchanged.

| Kind | Bits | Count |
|---|---|---|
| Runes | Fire 1, Air 2, Earth 4, Water 8, Dark 16, Light 32, Life 64, Death 128, Mind 256, Spirit 512, Master 1024 | 11 |
| Glyphs | Creation 1, Destruction 2, Summoning 4, Transformation 8, Transportation 16, Enhancement 32, Reduction 64, Control 128, Protection 256, Information 512 | 10 |
| Sigils | Self 1, Targeting 2, Area 4, Object 8 | 4 |

Costs are paid in primal: rune = 20 x (runes known + 1); glyph = 10 x (glyphs known + 1), capped at 50; sigil = 30 flat.
Prerequisites: a glyph needs at least one rune; a sigil needs at least one glyph; the Master rune needs all ten element runes. Relearning a known component is refused before payment. Unknown names show usage. Only Sorcerers learn. An apprentice may learn up to 3 runes, 2 glyphs and 2 sigils until graduation.

Command: `learn` (list), `learn rune|glyph|sigil <name>`.

## Recipes

A recipe is cast with `recipe <rune> <glyph> <sigil> [target]` and requires all three learned components. Custom spellbooks and `chant` are untouched. Gathering keys are rune*10000 + glyph*10 + sigil terms.

Safety policy (user decision: fix unsafe recipes, do not drop them): global reach becomes same-room; offensive recipes need is_safe, no ROOM_SAFE and a legal target; summon needs consent; every recipe charges mana and a wait state.

| Gathering key | Rune | Glyph | Sigil | Anathema spell | Casts | Class | Notes |
|---|---|---|---|---|---|---|---|
| 10022 | fire | destruction | targeting | fireball | 2 | Guarded | Double fireball |
| 10024 | fire | destruction | area | burning hands | 1 | Guarded | Flame bolts hit each legal foe in the room |
| 10328 | fire | enhancement | object | protection | 1 | Adapted | Resistance: no matching spell; grants Protection |
| 20022 | air | destruction | targeting | chill touch | 1 | Guarded | Gathering global reach is same-room |
| 20024 | air | destruction | area | call lightning | 2 | Guarded | Double call lightning; spell keeps its own weather rules |
| 20082 | air | transformation | targeting | energy drain | 1 | Guarded | Energy drain |
| 20322 | air | enhancement | targeting | fly | 1 | Direct | Fly |
| 20328 | air | enhancement | object | shield | 1 | Adapted | Spellproof: no matching spell; grants Shield |
| 40024 | earth | destruction | area | earthquake | 2 | Guarded | Double earthquake |
| 40081 | earth | transformation | self | stone skin | 1 | Direct | Stone skin |
| 40322 | earth | enhancement | targeting | giant strength | 1 | Guarded | Gathering global reach is same-room |
| 40328 | earth | enhancement | object | armor | 1 | Adapted | Repair: no matching spell; grants Armor |
| 80018 | water | creation | object | create spring | 1 | Direct | Create spring |
| 80024 | water | destruction | area | gas breath | 2 | Guarded | Double gas breath |
| 80321 | water | enhancement | self | mana | 2 | Direct | Double mana |
| 81282 | water | control | targeting | web | 1 | Adapted | Jail of water was unfinished in Gathering; Web binds a foe |
| 160022 | dark | destruction | targeting | harm | 2 | Guarded | Double harm |
| 160162 | dark | transportation | targeting | curse | 1 | Guarded | Gathering global reach is same-room |
| 160321 | dark | enhancement | self | giant strength | 1 | Adapted | Frenzy is an empty spell in Anathema; grants Giant strength |
| 160322 | dark | enhancement | targeting | bless | 1 | Adapted | Darkblessing is an empty spell in Anathema; grants Bless |
| 160642 | dark | reduction | targeting | poison | 1 | Guarded | Gathering global reach is same-room |
| 161284 | dark | control | area | sleep | 1 | Guarded | Sleep |
| 320018 | light | creation | object | continual light | 1 | Direct | Continual light |
| 320024 | light | destruction | area | colour spray | 3 | Guarded | Triple colour spray |
| 320321 | light | enhancement | self | sanctuary | 1 | Direct | Sanctuary |
| 320322 | light | enhancement | targeting | bless | 1 | Guarded | Gathering global reach is same-room |
| 322561 | light | protection | self | protection | 1 | Direct | Protection |
| 322562 | light | protection | targeting | armor | 1 | Direct | Armor |
| 322564 | light | protection | area | shield | 1 | Guarded | Gathering global reach is same-room |
| 640018 | life | creation | object | create food | 3 | Direct | Triple create food |
| 640022 | life | destruction | targeting | dispel evil | 2 | Guarded | Double dispel evil; same-room |
| 640321 | life | enhancement | self | refresh | 3 | Direct | Triple refresh |
| 640322 | life | enhancement | targeting | heal | 2 | Direct | Double heal |
| 640324 | life | enhancement | area | heal | 1 | Guarded | Gathering global reach is same-room |
| 640328 | life | enhancement | object | enchant armor | 1 | Direct | Enhance armor |
| 640642 | life | reduction | targeting | energy drain | 2 | Guarded | Double drain life; same-room |
| 645122 | life | information | targeting | know alignment | 1 | Guarded | Gathering global reach is same-room |
| 1280044 | death | summoning | area | guardian | 1 | Guarded | Triple guardian limited to one guardian per cast |
| 1280328 | death | enhancement | object | preserve | 1 | Adapted | Preserve: no matching spell; extends an object timer by 24 ticks, capped at 200 |
| 1280642 | death | reduction | targeting | weaken | 1 | Guarded | Weaken |
| 2560321 | mind | enhancement | self | detect magic | 1 | Direct | Truesight maps to Detect magic |
| 2560642 | mind | reduction | targeting | dispel magic | 1 | Guarded | Gathering global reach is same-room |
| 2565122 | mind | information | targeting | readaura | 1 | Guarded | Runs the existing readaura command (same-room) |
| 2565124 | mind | information | area | scry | 1 | Guarded | Runs the existing scry command |
| 2565128 | mind | information | object | identify | 1 | Guarded | Identify on a carried object |
| 5120018 | spirit | creation | object | soulblade | 1 | Guarded | Double soulblade limited to one per cast |
| 5120044 | spirit | summoning | area | summon | 1 | Guarded | Summon needs the target to have summon enabled and keeps every spell restriction |
| 5120161 | spirit | transportation | self | teleport | 1 | Guarded | Teleport blocked in combat, safe and no-teleport rooms |
| 5120321 | spirit | enhancement | self | pass door | 1 | Direct | Pass door |
| 5120328 | spirit | enhancement | object | enchant weapon | 1 | Direct | Enchant weapon |
| 5120642 | spirit | reduction | targeting | faerie fire | 1 | Guarded | Gathering global reach is same-room |
| 5121284 | spirit | control | area | faerie fog | 1 | Direct | Faerie fog |
| 5122561 | spirit | protection | self | shield | 1 | Adapted | No-magic aura was unfinished in Gathering; grants Shield |

All 53 Gathering recipes are present; none was dropped. "Adapted" rows replace a spell that Anathema does not have, or that is an empty stub, with the closest working effect. The Master rune is not part of any recipe key; knowing it cuts every recipe's mana cost by 25 percent.

Mana cost is the spell's normal cost multiplied by its casts. Every recipe also applies the spell's wait state.

## Apprenticeship

State is `pcdata->apprentice[4]`: [0] stage (0 legacy, 1 apprentice, 2 graduated), [1] lessons, [2] last lesson time, [3] reward claimed. Saved as an `Apprentice` line; missing means zeros.

- Existing Sorcerers load as stage 0 and count as graduated; they keep status, colors and books and never repeat apprenticeship.
- New Sorcerers created through class selection start at stage 1.
- Commands: `apprentice` (status), `apprentice teach <player>`, `apprentice accept`, `apprentice decline`, `apprentice train` (at an NPC).
- Player mentor: a graduated or legacy Sorcerer in the room; the apprentice must accept the offer. One lesson per 10 minutes. The apprentice pays 5 primal.
- NPC trainer: an awake, non-fighting NPC in the room whose name contains `sorctrainer`. Same cooldown, 10 primal.
- Graduation: 5 lessons, at least 3 runes, 1 glyph, 1 sigil, and color ranks totalling at least 3. Stage becomes 2 once; a one-time 100 primal reward is guarded by the reward flag.
- Nothing here touches `ch->level`, trust or staff status. Stage never decreases.

## Compatibility

New fields zero-initialise in both loaders and are reset by clearclass with colors.
