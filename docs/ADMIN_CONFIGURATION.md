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

## Saving and failure handling

Valid policies use fixed-name atomic replacement: create a private temporary
file in the same directory, check serialization/flush/fsync/close, rename, then
fsync the directory. Empty policies retain the legacy **absent file** format;
checked unlink plus directory fsync protects this deletion. An already missing
file also requires directory fsync before confirming a durable empty policy.
Non-regular targets and symlinks cannot be replaced or deleted.
As with atomic player/leaderboard saves, the area directory must be owned by
the server uid and not group/other-writable. New policy files are private (0600).

If an operation fails before rename/unlink, the previous file is kept and
the modifying command restores its old in-memory list. It does **not** say
the change succeeded. If rename/unlink succeeds but directory fsync fails,
the new policy is visible, but power-loss durability is uncertain. The command
warns the player and pauses further changes to that policy until a validated
reload/restart. Runtime restrictions still follow the newly visible state.

The open-file reserve is restored on every attempted disk operation, including
failed ones. File contents and command trust remain unchanged for normal
successful operations. No gameplay, class, player-save, or handoff schema
changes are introduced.

These guarantees are for one trusted server process and one local filesystem.
They do not replace backups, coordinate multiple writers, journal unfinished
staff actions across crashes, or prove durability under a real power cut.
