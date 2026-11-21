# Unpacked world and compatibility audit

The versioned seed is `Genesis/area/`: 69 world/help/list files selected from
`Genesis.zip`, without historical accounts, saves, notes, bans, copyover data,
logs, object files or the old executable. `area.lst` lists 65 area/help files.
Three historical files remain **inactive**: `arioch.are`, `midge.are` and
`start.are`. They are not silently enabled or substituted for the active world.
The ZIP remains historical reference, not the default installer source.

## Engine-loaded scope

The real engine loads the checked-in world in private temporary directories;
the audit does not read or change a live server's state.

| Loaded data | Count |
| --- | ---: |
| Areas | 60 |
| Rooms | 3,484 |
| Object prototypes | 1,255 |
| Mobile prototypes | 1,051 |
| Exits | 7,903 |
| Stored resets | 4,777 |
| Populated item-spell slots | 233 |
| Room triggers after removal of the dead route | 26 |
| Traps | 2 |
| Shops | 49 |

The engine validates duplicate identifiers, reset references, spell-slot
resolution and NPC special-function names as it loads. Additional checks cover
hard-required core/class prototypes, all 28 class-crafting templates, missile
ammunition/type compatibility, active trigger/trap dependencies, container
resets, equipment slots, shop hours/margins, exit targets and key references.
The catalog tests also read the unpacked seed rather than the historical ZIP.
The loaded seed has no missing mandatory prototypes, invalid populated item
spell slots, dangling positive exit destinations or missing positive key IDs
under these checks. This is not a claim that every skill/spell has been
exercised in multiplayer.

## Confirmed fixes

- The loader initialized only live door flags, while area resets restored
  zero persistent flags. Door definitions disappeared on reset. Load/reset
  tests now cover the editor's complete 0..8 door-protection encoding; invalid
  encodings are rejected instead of silently discarded.
- A reset also changed the nominal reverse exit when that exit actually led
  to a third room. It now requires a reciprocal connection. A three-room
  regression case reproduces the old failure and verifies the correction.
- Removed the obsolete `staff` teleport/menu option in Maraden room 3206:
  destination 25008 belongs to inactive `start.are`. No replacement destination
  or obsolete training area was invented/enabled.
- Cleared ten nonexistent key references on **non-door** exits in Draconia,
  High Tower and Mud School. Their passability is unchanged; no keys or locks
  were invented.
- Corrected player help for consent-based `teachdisc`, the five implemented
  Thaumaturgy commands, Stun Fist, Drow bolt crafting and Counter. Malkavian's
  unavailable mapping is explicit. Old Potency generation-change, forced
  Blood Rage and guaranteed Cauldron-death claims no longer misdescribe them.
  Leadership help uses the live managed registry instead of asserting that
  old, unimported player accounts lead Anathema's classes.
- Player-facing greetings, MOTD, staff roster, school/relic names and runtime
  messages use Anathema. The greeting credits LaWaylon (Luto) for Anathema
  modifications while preserving the original Diku/Merc/God Wars/Ethereal
  Mists/Genesis attribution. The old owner's donation/contact details are
  not advertised as Anathema's.

## Startup and tests

As an ordinary user: `make run` (optional `PORT=4040`). It validates the seed,
prepares private runtime directories without replacing saves, and starts in
the correct directory. `python3 tools/start_local.py --check` validates without
starting. Root SSH operators should use `deploy/setup.py`'s fresh systemd
installation, which builds/runs as the `anathema` service account.

`make test` includes the real-world integrity suite and layout/startup checks.
`make sanitize` runs the native suite under ASan/UBSan. The TCP/shutdown,
signup/authorization and real executable copyover smokes now stage the
unpacked seed, not the old ZIP.

For numeric raw records, build and run:

```sh
make tests/test_world_integrity
./tests/test_world_integrity --dump > world-audit.jsonl
```

The JSONL contains static world metadata and a summary, not player saves.
Nine synthetic door fixtures are tested after the summary and excluded from
the world counts. A missing first-boot `leader.txt` is expected empty runtime
metadata, not a missing world file.

## Remaining uncertainties and operating limits

- There are 101 connections whose nominal reverse exit points elsewhere.
  Most occur in Ultima (66) and Pyramid (15); mazes/one-way routes can be
  intentional. They remain unchanged pending map-design review, not labeled
  as 101 confirmed bugs.
- The bundled exits are all encoded as non-doors. The corrected loader now
  preserves valid future/editor door definitions, but descriptions mentioning
  doors were not automatically converted into lockable barriers.
- Optional Hell room 30000 is absent; the existing object-form recovery uses
  its tested Temple fallback. No Hell room or teacher characters were seeded.
- Historical help beyond the updated entries still needs editorial review.
  Prototype presence/valid spell mappings do not establish class balance,
  complete quest progression or every NPC's behavioral correctness.
- Legacy OLC builder authorization needs a separate hardening pass before
  delegating builder accounts: it uses substring matching and editor
  entry/dispatch checks are inconsistent. Historical area-builder metadata
  is not an authoritative Anathema staff roster. This audit does not certify
  safe untrusted-builder access.
- Replacing a live world's files can destroy customization. The installer
  refuses existing installations; back up and review migrations separately.
  No live-host install, deployment or GUI-client certification is implied.
- The uploaded build log contains warning diagnostics but no compiler error
  lines. In particular, legacy formatting warnings still require their own
  source audit; this world release does not suppress them or claim zero bugs.
- The retained historical ZIP may contain old runtime material. Do not
  extract it wholesale or assume unpacking this seed sanitizes Git history.
  Original game/world permissions and licensing still require review.
