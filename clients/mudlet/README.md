# Optional Anathema HUD for Mudlet

Server: **137.184.145.164**, TCP port **4000**. Use your unique game password.
The normal endpoint is plain Telnet, not TLS. This HUD is display-only: it
does not send commands, autoheal, choose combat actions, reconnect, download
anything, or change your console borders.

## Install

1. Use a current Mudlet 4.x with its bundled Geyser UI and enable GMCP in the
   profile's connection preferences.
2. Download `AnathemaHUD.xml` from this private repository. In Mudlet's
   **Scripts** editor, use **Import** to import that XML and save the profile.
   Alternatively create one Script, paste `anathema_hud.lua` and save.
   Install **one** form, not both.
3. Reconnect to the server, log in, and wait for the first complete vitals pair.

Installing or reloading while already connected deliberately does **not**
read old cached GMCP tables. Reconnect afterward. The server uses changed-only
updates; a stationary character might otherwise receive no fresh snapshot.
No reconnect is performed automatically, especially during combat.

The small overlay appears in the upper-right. It shows health/mana bars and
only resources appropriate to your current class. Unknown/zero caps are
numeric readouts rather than invented progress bars. Negative/over-cap raw
values remain visible while bar drawings stay between 0% and 100%.
Legacy values beyond exact Lua integer precision are marked `~` (approximate).
Malformed, non-finite or non-applicable resources are not displayed.

## Controls and removal

Run these in Mudlet's Lua console, not as server commands:

```lua
AnathemaHUD:hide()
AnathemaHUD:show()
AnathemaHUD.panel:move(20, 20)
AnathemaHUD:destroy()
```

To keep the HUD removed after profile restart, delete its imported Script
after calling `:destroy()`. Simply disabling a Script does not unload code
already running; destroy it or restart the profile as well.
To change the default position, edit the `config` table in the Lua source
and regenerate the XML, or edit the imported Script's copy directly.

## Behavior and tested scope

- Disconnect, new connection, GMCP disable/re-enable and reload clear old data.
- Fresh `Char.Vitals` followed by `Anathema.Vitals` is required. Unavailable,
  switched-body and incomplete views do not display previous stats.
- The class whitelist prevents stale merged/null fields from another class
  being drawn. Text contains fixed labels and validated numbers, not raw HTML.
- Reload removes the old owned UI and handlers; unrelated namespaces are not
  overwritten. Partial setup failures clean up registered handlers and widgets.
- No timer or prompt trigger is required; this does not parse combat text.

Tests execute the canonical source and XML payload in real LuaJIT/Lua 5.1,
using headless Mudlet/Geyser API doubles. The XML is parsed and checked against
the canonical source. This is **not** actual Qt GUI/import certification.
Verify import, appearance and event handling in your installed Mudlet version.
This is a Mudlet package, not a MudForge widget.

Developer checks:

```sh
python3 tools/build_mudlet_hud.py
python3 tools/build_mudlet_hud.py --check
python3 tests/test_mudlet_hud.py
```

LuaJIT or Lua 5.1 is required for these tests, not for running the C server.
See [the server packet schema](../../docs/GMCP.md).
