# GMCP identity and vitals for Mudlet and MudForge

GMCP is optional. Enable GMCP in your client and reconnect to the operator's
normal game endpoint. Clients without it retain ordinary text play.
The server supplies authenticated vitals and current-room mapping data; the
client builds its own explored map. It does not send a pre-drawn atlas or party
data. An [optional Mudlet HUD](../clients/mudlet/README.md) is supplied separately.

## Packages

The server offers Telnet option 201 (`IAC WILL GMCP`). Once the client accepts
with `IAC DO GMCP`, the authenticated snapshot contains the three status
subnegotiations below. A separate `Room.Info` packet is sent when room-map data
changes.

```text
Char.Base {"name":"Example","level":2,"class":"vampire"}
Char.Vitals {"hp":71,"maxhp":100,"mp":33,"maxmp":99,"mana":33,"maxmana":99}
Anathema.Vitals {"available":true,"class":"vampire","blood":{"current":17,"max":50},"ki":null,"quintessence":null,"paradox":null,"class_points":null,"willpower":null}
Room.Info {"num":12001,"name":"Temple of Mara","area":"Temple","zone":"Temple","environment":"city","terrain":"city","exits":{"n":12002,"e":12003}}
```

## Room mapping

`Room.Info` follows the common GMCP Room package shape used by MudForge's map
widget and Mudlet mapping scripts:

| Field | Meaning |
| --- | --- |
| `num` | Stable room vnum |
| `name` | Room title |
| `area` / `zone` | Area name (both aliases are supplied) |
| `environment` / `terrain` | Existing sector label (both aliases are supplied) |
| `exits` | Open exits keyed by `n`, `e`, `s`, `w`, `u`, `d`, with destination vnums |

Closed exits are omitted, matching the game's ordinary automatic exits list.
The world has no authored room coordinates, so clients infer layout from
movement and build a local map as players explore. No room descriptions,
occupants, hidden player state, or other players' locations are sent. The
packet is authenticated-session-only and independently cached: vitals changing
does not resend an unchanged room, and a room transition does not require a
vitals change. Sleeping, blindness and obscuring darkness suppress new map
records; the client may still display the last room it learned.

MudForge's Map Widget can use GMCP `Room.Info`; if needed, set its field
mappings to `num`, `name`, `area`, `environment` (or `terrain`) and `exits`.
Mudlet needs a mapping script in addition to its Mapper display. Its IRE mapping
script accepts `gmcp.Room.Info` with these fields; follow the script's
game-specific setup and environment registration. Neither client receives a
pre-populated coordinate atlas from the server.
Portals, non-compass travel and irregular world geometry may require manual
mapper corrections. No automatic walking or route execution is added.

`Char.Base` supplies the authenticated player's name, actual character level
(not trust, class rank or an invented RPG level), and primary class. It uses the
same lowercase class labels as `Anathema.Vitals`; it does not infer a hybrid
label. No race or status-effect list is supplied. Normal names are at most 12
ASCII letters. For legacy/corrupt data, export is bounded to 64 name bytes;
quotes, backslashes, control and non-ASCII bytes are JSON-escaped. This does not
add UTF-8 input support or alter stored names.

All numeric values are signed JSON integers copied from existing gameplay
fields, not percentages. Negative HP can represent an incapacitated player.
No movement/stamina stat is invented. Consumers should guard against zero
maxima and clamp gauge drawings, not modify gameplay values.
Legacy values outside a client's exact integer range may lose display precision.

`mana` / `maxmana` are exact aliases of `mp` / `maxmp`, supplied for MudForge's
documented status-widget mappings. Existing Mudlet scripts and the bundled HUD
keep their original fields. No movement resource, fake maximum or inferred
percentage is added. Consumers should read the fields they support, not require
an exact four-key object.

Every custom key is sent in each snapshot. `null` explicitly clears a
non-applicable resource after class changes, rather than leaving stale data.

| Custom field | Applicability | Current / maximum |
| --- | --- | --- |
| blood | Vampire | Existing blood / blood pool |
| ki | Ninja | Existing Ki / 300, matching score |
| quintessence | Mage | Existing BODY quintessence / null (no invented cap) |
| paradox | Mage | Existing paradox / null (not a spendable mana pool) |
| class_points | Drow, Dragon | Existing current points / null (no invented cap) |
| willpower | Werewolf | Existing willpower current / maximum |

Monk, Elemental, Sorcerer and Highlander receive health/mana; class-specific
fields remain null where there is no corresponding resource above.
Unclassed characters use `"class":"none"`. Unsupported class IDs use `"unknown"`,
not an inferred hybrid build. No stats, costs, powers or class eligibility change.

## Privacy, updates and traffic

- No identity or vitals before entering an authenticated in-game session.
- Only the descriptor's reciprocally attached PC is reported. No opponents,
  passwords, inventory, or another player's data. Only the player's own name
  and current room's public map data are supplied.
- Switched/NPC, object/head and detached sessions do not expose body stats.
  A previously active view receives zero standard vitals and
  `"available":false` with null custom resources, rather than stale vitals.
  `Char.Base` clears to an empty name, level 0 and class `"none"`.
- In-world note/OLC editors remain authenticated sessions.
- Snapshots are checked on each existing output-loop tick, including idle
  regeneration, and queued only when their content changes. Text prompts
  need not be enabled. There is no per-frame timer or new background thread.
- DONT GMCP stops new snapshots. Repeated acknowledgements do not cause
  negotiation loops or duplicate unchanged snapshots.
- Staff snooping receives written game text, not another connection's GMCP
  or Telnet controls. Filtering retains framing across partial writes and
  mid-packet snoop attachment; telemetry cannot overwrite the staff UI.
- Client GMCP subnegotiations (including Core.Hello/Supports) are safely
  discarded. This is a read-only server-push feed, not a command/query API.

ASCII JSON with escaped name bytes and integers is UTF-8-compatible. GMCP output bypasses
ANSI/color expansion and uses the bounded binary-safe socket output queue.
This does not change legacy ASCII **command input** or add TLS.

## Client access

Mudlet scripts can read `gmcp.Char.Base`, `gmcp.Char.Vitals`,
`gmcp.Anathema.Vitals` and `gmcp.Room.Info`, and react to the corresponding
GMCP package events.
For example, inspect `gmcp.Char.Vitals.hp` and `.maxhp` after the first snapshot.
MudForge exposes GMCP to its widgets/plugins; use these package names and the
schema above, rather than relying on prompt wording.
Its **Show Character Info** status-bar setting uses `Char.Base` for the name,
level and class. Enable it and reconnect after updating. Authenticated game
telemetry is tested with the production executable; the new identity header's
appearance in the hosted GUI still needs a logged-in user check.

MudForge web v2.0.3915 was exercised in headless Chromium against the hosted
bridge: it reached the public Anathema greeting and login-name prompt without
authentication. No existing player login or live copyover was performed by that
probe. Authenticated JSON and actual copyover are independently tested with the
isolated production executable. This is not full GUI certification, and no claim
is made that built-in widgets consume every custom Anathema resource.

## Restart, implementation and tests

`gmcp.c` owns negotiation state, privacy gates and cached snapshots.
`telnet_input.c` owns framing and bounded raw output. `comm.c` offers GMCP
for new TCP descriptors and calls updates in both existing output loops.

The copyover manifest stays at version 2 and still reads version 1. Recovery
sends WONT then WILL GMCP, clears the descriptor cache, and waits for a fresh
DO before publishing. The reset is sent **after successful exec**; failed or
cancelled exec leaves the old process/client state intact.
Copyover is refused while a Telnet control packet is partway through actual
transmission. Let output drain and retry; a persistently stalled client may
need staff attention. Wholly unsent text/snapshots can still be discarded,
as in existing copyover; a reset and fresh snapshots restore the vitals view.
Rollback to a pre-v2 executable still requires checked save and stop/start;
see [CLIENTS.md](CLIENTS.md).

`test_gmcp.c` runs the real reader/writer and covers all ten classes, option
acknowledgements, fragmented payloads, privacy, null clearing, editors, separate
vitals/room caches, Room.Info exits and escaping, LONG_MIN/MAX formatting,
failed output and recovery. Every-split snoop checks
protect metadata from becoming staff-client protocol state. Restart tests
reject partial control output in new, offered, active and disabled GMCP states. Independent
Python tests decode JSON over every packet split. The production-executable smoke test
checks pre-login privacy and fresh JSON packets after real exec on the same
TCP socket, while retaining save-failure and plain-client checks. All recovered
snapshots must retain both matching mana field pairs. The real-engine suite
checks 588 class/value combinations, including negative, zero and LONG_MIN/MAX
values, without changing the original privacy and output-budget guards.
Identity tests cover all 255 non-NUL name bytes, null and bounded non-terminated
names, extreme levels, identity-only changes, privacy clearing and caching.
The wire smoke independently JSON-decodes actual engine name output byte by
byte and requires `Char.Base` before and after real copyover. Its explicit
old-executable upgrade mode permits missing identity only before the exec.

CI sanitizer binaries use `-fno-pie` / `-no-pie` to avoid old GCC sanitizer
startup/address-layout conflicts on newer Linux runners. Normal server builds
keep their platform defaults. A separate startup probe checks 100 configured
launches and verifies that deliberate ASan/UBSan violations still fail.
Its PIE probe is diagnostic only; every actual MUD regression remains fail-fast
and is run once, without changing host ASLR or disabling sanitizer checks.

Protocol reference: <https://mudstandards.org/mud/gmcp/>.
