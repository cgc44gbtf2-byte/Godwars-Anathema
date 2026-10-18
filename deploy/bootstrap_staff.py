#!/usr/bin/env python3
"""Offline, operator-only first-staff bootstrap for already-created accounts.

Never call this from the game or while the server holds clans/Lock. No game
password, password hash, token, or file contents are displayed.
"""
import argparse
import fcntl
import os
from pathlib import Path
import secrets
import stat
import sys


ROLES = {"Yami": (7, 500000), "Luto": (6, 300000)}
DIR_FLAGS = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC
FILE_FLAGS = os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC
MAX_SAVE_BYTES = 8 * 1024 * 1024


class BootstrapError(Exception):
    pass


def directory(fd, label, *, owner=None, private=False):
    info = os.fstat(fd)
    if not stat.S_ISDIR(info.st_mode) or (info.st_mode & 0o022):
        raise BootstrapError(f"{label}: unsafe directory permissions")
    if owner is not None and info.st_uid != owner:
        raise BootstrapError(f"{label}: unexpected directory owner")
    if private and (info.st_mode & 0o077):
        raise BootstrapError(f"{label}: expected private directory mode")
    return info


def save_bytes(directory_fd, account, owner):
    fd = os.open(account, FILE_FLAGS, dir_fd=directory_fd)
    try:
        info = os.fstat(fd)
        if (not stat.S_ISREG(info.st_mode) or info.st_nlink != 1
                or info.st_uid != owner or (info.st_mode & 0o077)
                or info.st_size < 30 or info.st_size > MAX_SAVE_BYTES):
            raise BootstrapError(f"{account}: unsafe or incomplete player save")
        with os.fdopen(os.dup(fd), "rb") as reader:
            data = reader.read(MAX_SAVE_BYTES + 1)
        if len(data) > MAX_SAVE_BYTES or len(data) != info.st_size or b"\x00" in data:
            raise BootstrapError(f"{account}: changed or invalid player save")
        return data, info
    finally:
        os.close(fd)


def player_fields(data, account):
    lines = data.splitlines(keepends=True)
    if not lines or lines[0].rstrip(b"\r\n") != b"#PLAYER":
        raise BootstrapError(f"{account}: missing #PLAYER section")
    fields = {}
    end = None
    for pos, line in enumerate(lines[1:], 1):
        raw = line.rstrip(b"\r\n")
        if raw == b"End":
            end = pos
            break
        key, sep, value = raw.partition(b" ")
        if key in (b"Name", b"Password", b"Level", b"HpMana"):
            if not sep or key in fields:
                raise BootstrapError(f"{account}: duplicate or malformed player field")
            fields[key] = (pos, value.strip())
    if end is None or not any(l.rstrip(b"\r\n") == b"#END" for l in lines[end + 1:]):
        raise BootstrapError(f"{account}: incomplete save sections")
    if set(fields) != {b"Name", b"Password", b"Level", b"HpMana"}:
        raise BootstrapError(f"{account}: missing required player field")
    if fields[b"Name"][1] != account.encode() + b"~":
        raise BootstrapError(f"{account}: saved name differs from filename")
    password = fields[b"Password"][1]
    if len(password) <= 1 or not password.endswith(b"~"):
        raise BootstrapError(f"{account}: password is not set")
    try:
        level = int(fields[b"Level"][1])
        hp = fields[b"HpMana"][1].split()
        if len(hp) != 4:
            raise ValueError
        tuple(int(part) for part in hp)
    except ValueError as exc:
        raise BootstrapError(f"{account}: malformed level or resources") from exc
    if not 0 <= level <= 7:
        raise BootstrapError(f"{account}: invalid stored level")
    return lines, fields, level, password


def elevated(lines, fields, level, resources):
    result = lines.copy()
    for key, value in ((b"Level", str(level).encode()),
                       (b"HpMana", b" ".join([str(resources).encode()] * 4))):
        pos = fields[key][0]
        newline = b"\r\n" if result[pos].endswith(b"\r\n") else b"\n"
        result[pos] = key + b" " * (13 - len(key)) + value + newline
    return b"".join(result)


def write_new(directory_fd, name, data, *, uid=None, gid=None):
    fd = os.open(name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC,
                 0o600, dir_fd=directory_fd)
    try:
        os.fchmod(fd, 0o600)
        if uid is not None and (os.fstat(fd).st_uid, os.fstat(fd).st_gid) != (uid, gid):
            os.fchown(fd, uid, gid)
        view = memoryview(data)
        while view:
            written = os.write(fd, view)
            if written <= 0:
                raise OSError("short write while staging bootstrap")
            view = view[written:]
        os.fsync(fd)
    except BaseException:
        try:
            os.unlink(name, dir_fd=directory_fd)
        except OSError:
            pass
        raise
    finally:
        os.close(fd)


def bootstrap(root: Path, account: str, apply: bool = False):
    if account not in ROLES:
        raise BootstrapError("only Yami and Luto are eligible")
    if not root.is_absolute():
        raise BootstrapError("root must be an absolute installation path")
    root_fd = os.open(root, DIR_FLAGS)
    opened = [root_fd]
    staged = []
    try:
        directory(root_fd, "installation", owner=os.geteuid())
        dirs = {}
        for label in ("clans", "player", "backup"):
            fd = os.open(label, DIR_FLAGS, dir_fd=root_fd)
            opened.append(fd)
            dirs[label] = fd
        clan = directory(dirs["clans"], "clans", private=True)
        player = directory(dirs["player"], "player", private=True)
        directory(dirs["backup"], "backup", owner=player.st_uid, private=True)
        if clan.st_uid != player.st_uid:
            raise BootstrapError("clans and player directories must have the same service owner")
        lock = os.open("Lock", os.O_RDWR | os.O_NOFOLLOW | os.O_CLOEXEC,
                       dir_fd=dirs["clans"])
        opened.append(lock)
        lock_info = os.fstat(lock)
        if (not stat.S_ISREG(lock_info.st_mode) or lock_info.st_nlink != 1
                or lock_info.st_uid != clan.st_uid or (lock_info.st_mode & 0o077)):
            raise BootstrapError("clans/Lock is not the expected private server lock")
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as exc:
            raise BootstrapError("server is running; log out and stop it first") from exc
        named_lock = os.stat("Lock", dir_fd=dirs["clans"], follow_symlinks=False)
        if (named_lock.st_dev, named_lock.st_ino) != (lock_info.st_dev, lock_info.st_ino):
            raise BootstrapError("server lock changed during bootstrap")

        current = {}
        for label in ("player", "backup"):
            current[label] = save_bytes(dirs[label], account, player.st_uid)
        parsed = {label: player_fields(current[label][0], account)
                  for label in current}
        if parsed["player"][3] != parsed["backup"][3]:
            raise BootstrapError(f"{account}: primary/backup credentials differ; synchronize saves first")
        levels = [parsed[label][2] for label in current]
        if levels[0] != levels[1]:
            raise BootstrapError(f"{account}: primary/backup levels differ; reconcile them first")
        target, resources = ROLES[account]
        if levels[0] >= target:
            raise BootstrapError(f"{account}: already at or above intended staff level")
        if not apply:
            print(f"Dry run: {account} level {levels[0]} -> {target}; "
                  "both saves present and consistent. No files changed.")
            return

        try:
            os.mkdir("bootstrap-backups", 0o700, dir_fd=root_fd)
        except FileExistsError:
            pass
        snapshots = os.open("bootstrap-backups", DIR_FLAGS, dir_fd=root_fd)
        opened.append(snapshots)
        directory(snapshots, "bootstrap-backups", owner=os.geteuid(), private=True)
        os.fsync(root_fd)
        for label in ("player", "backup"):
            snap = account + "." + label + ".pre-bootstrap"
            if os.path.lexists(root / "bootstrap-backups" / snap):
                raise BootstrapError(f"{account}: prior snapshot exists; inspect before retrying")
        for label in ("player", "backup"):
            write_new(snapshots, account + "." + label + ".pre-bootstrap",
                      current[label][0])
        os.fsync(snapshots)
        # Stage both writes before the first rename; do not expose a partial file.
        for label in ("backup", "player"):
            original, info = current[label]
            lines, fields, _, _ = parsed[label]
            content = elevated(lines, fields, target, resources)
            temp = ".bootstrap-" + account + "-" + secrets.token_hex(8)
            write_new(dirs[label], temp, content, uid=info.st_uid, gid=info.st_gid)
            staged.append((dirs[label], temp))
        for label in ("backup", "player"):
            fd, temp = staged[0]
            os.replace(temp, account, src_dir_fd=fd, dst_dir_fd=fd)
            staged.pop(0)
            os.fsync(fd)
        print(f"Applied: {account} -> level {target}. "
              "Original primary/backup saved in private bootstrap-backups/.")
    except (OSError, BootstrapError) as exc:
        if apply:
            print("Bootstrap stopped. Do not start the server after an interrupted "
                  "apply; inspect both saves and private originals.", file=sys.stderr)
        raise
    finally:
        for fd, temp in staged:
            try:
                os.unlink(temp, dir_fd=fd)
            except FileNotFoundError:
                pass
        for fd in reversed(opened):
            os.close(fd)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("/var/lib/anathema"))
    parser.add_argument("--account", choices=tuple(ROLES), required=True)
    parser.add_argument("--apply", action="store_true",
                        help="apply offline promotion (default: dry run)")
    args = parser.parse_args()
    try:
        bootstrap(args.root, args.account, args.apply)
    except (BootstrapError, OSError) as exc:
        parser.exit(1, f"Bootstrap refused: {exc}\n")


if __name__ == "__main__":
    main()
