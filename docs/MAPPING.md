# Maps in MudForge and Mudlet

Anathema sends authenticated GMCP `Room.Info` packets. Your client builds and
stores an explored map; the server does not supply coordinates or a complete
world atlas. For connection setup, see [CLIENTS.md](CLIENTS.md). For the packet
schema and visibility rules, see [GMCP.md](GMCP.md).

## MudForge

Enable GMCP and map data collection. In the map's three-dot menu, **Protocol
Configuration** should detect:

| Setting | Packet field |
| --- | --- |
| Room number | `num` |
| Room name | `name` |
| Area / zone | `area` or `zone` |
| Terrain | `terrain` or `environment` |
| Exits | `exits` |

Leave automatic detection unchanged when the displayed values are correct.
Walk between rooms to populate the map. Closed exits are not advertised;
irregular geometry and non-compass travel may need manual corrections.

### Optional clean 2D appearance

The photographed MudForge version has two settings controls:

- The gear in the **Map title bar** styles the widget frame, font and background.
- The gear **inside the map beside the room banner** opens General, Look,
  Motion, Data and Advanced. These controls may move in other client versions.

Suggested presentation settings, not server requirements:

- **General:** enable Node Mode for separate rooms and connector lines.
  Start at 1.2x zoom and 20px link length; keep Follow Player enabled.
- Reduce Recent Room Trail to 8–10 rooms, or set it to 0 to disable.
  Hide the banner VNUM if you prefer a simpler room title.
- **Look:** try Nord Night. Optional background `#111827` and room outline
  `#94A3B8` give a dark, muted presentation.
- Custom colors, terrain colors and room flags can override the palette.
  The small reset beside a color removes that override, not explored rooms.
- Resize the map panel before increasing zoom. Hide unused panels if needed.

These are appearance preferences, not changes to exits, terrain or gameplay.
Do not use **Clear Map Data** for cosmetic cleanup. Use **Export Map Data**
before deliberately deleting or replacing a map database.

## Mudlet

Enable GMCP and install/configure a mapping script in addition to the Mapper
display. The [Mudlet client guide](../clients/mudlet/README.md#mapper-compatibility)
lists the Anathema fields and environment names. Installing the optional vitals
HUD alone does not install a mapper.

## Verified scope and limitations

The operator supplied screenshots of the authenticated hosted MudForge map and
confirmed that explored connections remain when backtracking and after
reconnecting. This is a user-observed result, not automated certification of
every MudForge version. Specific Mudlet mapper import and GUI behavior remain
unverified.

Automated engine and real-copyover tests validate the server's packet data.
They do not validate a client's rendering, route planning or stored map layout.
No automatic walking is added. Visibility restrictions can suppress new room
packets while a client continues showing its last known room.
