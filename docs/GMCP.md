# GMCP vitals for Mudlet and MudForge

GMCP is optional. Enable GMCP in your client and reconnect to the operator's
normal game endpoint. Clients without it retain ordinary text play.
The server adds **data**, not an autohealer, mapper or party panel.
An [optional Mudlet HUD](../clients/mudlet/README.md) is now supplied separately;
MudForge still needs its own widget/plugin configuration.

## Packages

The server offers Telnet option 201 (`IAC WILL GMCP`). Once the client accepts
with `IAC DO GMCP`, each snapshot consists of two GMCP subnegotiations:

```text
Char.Vitals {"hp":71,"maxhp":100,"mp":33,"maxmp":99}
Anathema.Vitals {"available":true,"class":"vampire","blood":{"current":17,"max":50},"ki":null,"quintessence":null,"paradox":null,"class_points":null,"willpower":null}
```

All numeric values are signed JSON integers copied from existing gameplay
fields, not percentages. Negative HP can represent an incapacitated player.
No movement/stamina stat is invented. Consumers should guard against zero
maxima and clamp gauge drawings, not modify gameplay values.
Legacy values outside a client's exact integer range may lose display precision.

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

- No vitals before entering an authenticated in-game session.
- Only the descriptor's reciprocally attached PC is reported. No opponents,
  room IDs, character names, passwords, inventory, or another player's data.
- Switched/NPC, object/head and detached sessions do not expose body stats.
  A previously active view receives zero standard vitals and
  `"available":false` with null custom resources, rather than stale vitals.
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

ASCII JSON literals and integers are UTF-8-compatible. GMCP output bypasses
ANSI/color expansion and uses the bounded binary-safe socket output queue.
This does not change legacy ASCII **command input** or add TLS.

## Client access

Mudlet scripts can read `gmcp.Char.Vitals` and `gmcp.Anathema.Vitals` and react
to the corresponding `gmcp.Char.Vitals` / `gmcp.Anathema.Vitals` events.
For example, inspect `gmcp.Char.Vitals.hp` and `.maxhp` after the first snapshot.
MudForge exposes GMCP to its widgets/plugins; use these package names and the
schema above, rather than relying on prompt wording.

Actual GUI versions and MudForge's hosted bridge have not been exercised here.
No claim is made that every built-in widget automatically supports the custom
Anathema package. Packet-level tests are not GUI certification.

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
acknowledgements, fragmented payloads, privacy, null clearing, editors, caching,
LONG_MIN/MAX formatting, failed output and recovery. Every-split snoop checks
protect metadata from becoming staff-client protocol state. Restart tests
reject partial control output in new, offered, active and disabled GMCP states. Independent
Python tests decode JSON over every packet split. The production-executable smoke test
checks pre-login privacy and fresh JSON packets after real exec on the same
TCP socket, while retaining save-failure and plain-client checks.

Protocol reference: <https://mudstandards.org/mud/gmcp/>.
