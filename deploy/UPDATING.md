# Updating an existing default installation

This is for the systemd layout created by `deploy/setup.py`, not a fresh
installation or a custom port/layout. **Do not rerun the fresh installer.**
The repository must remain publicly readable: this updater does not accept
GitHub tokens or store credentials.

Obtain this helper once with `git pull --ff-only` in your Anathema checkout.
Run it over SSH from that checkout. Do not enter these commands in Mudlet.

## Preview (no installation changes)

```sh
python3 deploy/update.py
```

The helper pins the remote `Godwars-Anathema` commit and requires both the
push and pull-request `ci.yml` workflows to pass all eight Debian 12/13 and
Ubuntu 22.04/24.04 matrix jobs and all 40 required gates. Pending, failed,
missing or rate-limited checks block the update. A preview does not compile,
create users/backups, stop the service or replace files.

The first managed update additionally requires `--adopt-current`. Use it
only for a **modern Anathema build such as the level/exp-fixed release**, not
an historical Genesis executable. This is your attestation of compatibility,
not machine verification of the old executable's source. Its original
commit remains unknown in the backup record.

## Compatible update without stopping the running process

First save unsaved OLC world edits with the in-game `asave` facilities.
From your **root SSH terminal**:

```sh
python3 deploy/update.py --mode copyover --apply --adopt-current
```

After the first managed update omit `--adopt-current`:

```sh
python3 deploy/update.py --mode copyover --apply
```

Wait for **STAGED ONLY**. The new executable is on disk but the old process
is still running. This successful staging path does not stop/start systemd
and does not send in-game commands or use your Luto password.

Then, **inside the game as Luto**, run:

```text
copyover
```

Finally, over SSH:

```sh
python3 deploy/update.py --verify
```

Verification compares the service process's actual executable inode with the
installed binary, checks its hash against the release record, and checks a
bounded local TCP greeting. Only then is the staged record marked active.
Merely receiving a greeting from the old process is **not** verification.
An unverified staged release blocks the next update.

Some login sessions can disconnect during copyover. Copyover is not a
guarantee of zero downtime, successful player saves or preserved unsaved OLC
edits. A binary alone cannot update a running process.

## Maintenance restart

For a deliberate maintenance window, use:

```sh
python3 deploy/update.py --mode restart --apply
```

Add `--adopt-current` on first use. This gracefully stops the service, takes
a stopped-service snapshot, atomically replaces the binary, starts the unit
and checks the local greeting and executable identity. Players disconnect.
It does not use `kill -9`.

## Backups and rollback

Every install prints a backup ID. Backups live in the root-only directory
`/var/backups/anathema`; inspect IDs with:

```sh
ls -1 /var/backups/anathema
```

Each completed backup contains the previous executable, its checksum and
release metadata, and a state-directory snapshot. Snapshots do not follow
symlinks to copy external targets. **Copyover-mode snapshots are online and
not globally point-in-time consistent**; maintenance-restart snapshots are
taken with this service stopped. Neither captures unsaved memory or writes
made by unrelated processes.
Snapshot reads use directory descriptors and no-follow opens, rejecting
special files and file-to-symlink/FIFO swaps. Snapshot directories/files
are restricted to root (0700/0600); original ownership and timestamps are
not a filesystem-image restoration contract.

To restore a previously backed-up binary from the same supported save epoch:

```sh
python3 deploy/update.py --rollback BACKUP_ID --apply
```

Substitute the printed ID, not a filesystem path. Rollback works without
GitHub/network access, takes another backup, then performs a maintenance
restart. **It preserves current live player/world data. It does not restore
the old state snapshot or erase players' newer progress.** Recovery of
damaged data is a separate, deliberate operator procedure.

If installation/startup checking fails after replacement, the helper attempts
to restore the prior binary and verify its greeting. Failure still returns
a nonzero status; it does not claim the new release succeeded. If recovery
also fails, it reports the backup ID and stops trying. Storage failures or
power loss can require explicit rollback/manual recovery. Binary and release
metadata replacements are individually durable writes, not one atomic
multi-file transaction.

If copyover staging fails after replacement, automatic recovery can use a
graceful maintenance restart and disconnect players. Only the successful
staging path promises to leave the current process untouched.

## Boundaries and security

- Only the binary and root-owned release metadata are updated. Existing
  areas, players, passwords, notes, clans, service unit and dependencies are
  not replaced by repository defaults. No historical accounts are imported.
- Builds run as the dedicated `anathema-build` account, **not root or the game
  account**. The first applied update creates this system account if needed.
  The build environment is minimal, supplementary groups are refused, and
  `setpriv --no-new-privs` prevents gaining setuid/file-capability privileges.
  No operator tokens are passed to make.
  This is privilege separation, not a complete sandbox for hostile code.
- Archives are size-limited and source entries are selected explicitly;
  traversal, links and unexpected source artifacts are rejected.
- The supported declaration is `save_epoch: 4`, `handoff: 2`. Named Disquiet
  affects require the new reader. Installed epoch1/2/3 metadata may advance
  to4; binary-only downgrade from4 is refused because older readers cannot
  safely restore those affects. A failed epoch upgrade stops instead of
  automatically restarting an older reader. Repair with an epoch4-compatible
  release. Unknown operator-adopted binaries are conservatively tagged3.
  Handoff2 remains compatible; this is not permission to force a downgrade.
  Older player files and optional v4 hybrid records remain readable.
  Same-epoch rollback remains available. On first adoption, the conservative
  epoch3 tag deliberately prevents automatic recovery to the unknown binary
  after the epoch4 executable has run. The helper does not edit live player
  files to erase new affects for a downgrade.
  Future
  save-format, copyover-format, dependency or service changes need a reviewed
  migration, not blindly changing these numbers. The declaration cannot
  prove all future source changes are compatible.
- Executable ownership, protected paths, private runtime directories, backup
  checksums and a single-updater lock are checked. Custom layouts/ports are
  intentionally rejected. Running/disk binary mismatches require operator
  resolution; no arbitrary binary is silently adopted.
- No unattended updates, scheduled pulls, firewall changes, automatic state
  restoration or backup pruning are installed. Keep independent backups and
  monitor disk space. Network/API/build failures before deployment leave
  the game process untouched.
- A TCP greeting/inode check is not a gameplay or long-running multiplayer
  certification. This helper is tested with isolated filesystem fixtures,
  mocked service/network failures and existing engine smoke tests; live
  host/systemd and real GUI-client update trials still require operator
  validation.
