# Administrative policy files

`disabled.txt` and `ban.txt` are parsed in `admin_config.c`, independently of
command dispatch. Complete, validated snapshots replace the live policy only
after a successful read and close.

- Disabled commands keep the legacy `command level disabled_by` records and
  final `END` marker. Levels must be in the supported range. Unknown command
  names are still skipped after their complete record is validated.
- Ban files keep one hostname/suffix per line; trailing line comments retain
  the legacy ignored behavior. An empty ban file means no bans, not a blank
  suffix matching every host.
- Missing files mean empty policies. Empty `disabled.txt` is not a complete
  policy: use `END` or remove the file.
- Readers reject incomplete/overlong fields, malformed numbers, non-regular
  files, symlinks, files above 256 KiB, and more than 1,024 records. FIFOs are
  rejected without waiting for a writer.
- Adding a ban or disabled-command record checks these same record/serialized
  size limits before changing the live list. Invalid whitespace-containing
  hostnames are refused instead of being saved as a different suffix.

On a rejected reload, the previous live list remains intact and that policy's
modifying commands and save function become read-only. Listing and enforcing
the previous policy remain available. Repair the file and restart; no new
in-game reload command is added. A successful internal reload clears quarantine.

Startup stops with a clear diagnostic if either administrative policy cannot
be validated. It must not enter the game loop with an unvalidated ban or
disabled-command policy. Investigate the file instead of deleting restrictions
merely to bypass the startup check.

Disabled-command and ban displays are bounded/streamed. Ban nodes use the
existing recycled pool; old permanent nodes are not incorrectly heap-freed.

## Remaining work

This stage fixes reads, reload ownership, display bounds and corrupt-file
quarantine. Healthy-file `save_disabled` and `save_bans` still use their legacy
unchecked writes/unlinks and success messages. Atomic configuration writes and
write-failure acknowledgment are a separate follow-up. No file-format,
command-trust, class or gameplay changes are introduced for valid native data.
