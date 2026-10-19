#!/usr/bin/env python3
"""Default-layout Anathema updater. Preview unless --apply; never restores data."""
import argparse
from contextlib import contextmanager
import fcntl
import gzip
import grp
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import pwd
import re
import shutil
import signal
import socket
import stat
import subprocess
import tarfile
import tempfile
import time
import urllib.parse
import urllib.request

REPO = "cgc44gbtf2-byte/Godwars-Anathema"
BRANCH = "Godwars-Anathema"
API = "https://api.github.com/repos/" + REPO
APP = Path("/opt/anathema")
DATA = Path("/var/lib/anathema")
STORE = Path("/var/backups/anathema")
RECORD = APP / "deploy/release.json"
BINARY = APP / "src/merc"
LOCK = Path("/run/anathema-update/lock")
SERVICE = "anathema.service"
REQUIRED = ("Build", "Validate systemd unit (does not start service)",
            "Regression tests", "Isolated boot and graceful stop",
            "Address and undefined-behavior sanitizers")
MAX_DOWNLOAD = 32 * 1024 * 1024
MAX_BINARY = 128 * 1024 * 1024
SHA = re.compile(r"[0-9a-f]{40}")
HEX256 = re.compile(r"[0-9a-f]{64}")
BACKUP_ID = re.compile(r"[0-9]{8}T[0-9]{6}Z-[a-z0-9_]{8}")


class UpdateError(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise UpdateError(message)


def run(*args):
    return subprocess.run(args, check=True, text=True, capture_output=True,
                          timeout=900).stdout


class SafeRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, url):
        target = urllib.parse.urlsplit(url)
        require(target.scheme == "https" and target.hostname in
                ("api.github.com", "codeload.github.com") and
                target.username is None and target.port in (None, 443),
                "Unexpected GitHub redirect")
        return super().redirect_request(req, fp, code, msg, headers, url)


def download(url, limit):
    request = urllib.request.Request(url, headers={
        "User-Agent": "Anathema-updater/1", "Accept": "application/vnd.github+json"})
    with urllib.request.build_opener(SafeRedirect()).open(request, timeout=30) as stream:
        data = stream.read(limit + 1)
    require(len(data) <= limit, "Download exceeds size limit")
    return data


def api(path):
    return json.loads(download(API + path, 2 * 1024 * 1024))


def checked_release():
    sha = api("/git/ref/heads/" + BRANCH)["object"]["sha"]
    require(isinstance(sha, str) and SHA.fullmatch(sha), "Invalid remote SHA")
    runs = api("/actions/runs?head_sha=" + sha + "&per_page=100")["workflow_runs"]
    chosen = []
    for event in ("push", "pull_request"):
        candidates = [r for r in runs if r.get("head_sha") == sha and
                      r.get("event") == event and
                      r.get("head_branch") == BRANCH and
                      r.get("path") == ".github/workflows/ci.yml"]
        require(candidates, "Required " + event + " CI workflow not found")
        latest = max(candidates, key=lambda r: (r["run_number"], r["run_attempt"], r["id"]))
        require(latest.get("status") == "completed" and latest.get("conclusion") == "success",
                event + " CI is not successful; wait or fix CI")
        listing = api("/actions/runs/" + str(latest["id"]) + "/jobs?per_page=100")
        jobs = listing["jobs"]
        expected = {"regression (debian:12)", "regression (debian:13)",
                    "regression (ubuntu:22.04)", "regression (ubuntu:24.04)"}
        require(listing["total_count"] == 4 and len(jobs) == 4 and
                {j["name"] for j in jobs} == expected, "Unexpected CI matrix")
        for job in jobs:
            require(job["status"] == "completed" and job["conclusion"] == "success",
                    "CI matrix job not successful")
            for name in REQUIRED:
                matches = [s for s in job["steps"] if s["name"] == name]
                require(len(matches) == 1 and matches[0]["status"] == "completed"
                        and matches[0]["conclusion"] == "success", "CI gate missing: " + name)
        chosen.append(latest["id"])
    return sha, chosen


def archive_source(data, destination):
    """Select source explicitly; never extract archive paths, accounts or world."""
    entries, root, total = {}, None, 0
    with gzip.GzipFile(fileobj=io.BytesIO(data)) as compressed:
        raw = compressed.read(MAX_DOWNLOAD + 1)
    require(len(raw) <= MAX_DOWNLOAD, "Expanded archive exceeds size limit")
    with tarfile.open(fileobj=io.BytesIO(raw), mode="r:") as archive:
        for count, item in enumerate(archive):
            require(count < 5000, "Too many archive entries")
            parts = PurePosixPath(item.name).parts
            require(parts and not item.name.startswith("/") and
                    not any(x in ("..", ".") for x in item.name.split("/")) and
                    "\\" not in item.name, "Unsafe archive path")
            if root is None:
                root = parts[0]
            require(parts[0] == root, "Multiple archive roots")
            relative = PurePosixPath(*parts[1:])
            if item.isdir():
                continue
            selected = (relative.parts[:2] == ("Genesis", "src") or
                        str(relative) == "deploy/update_compat.json")
            if not selected:
                continue
            require(item.isfile() and not item.issym() and not item.islnk(),
                    "Source links/special files are not supported")
            if str(relative) != "deploy/update_compat.json":
                require(relative.suffix in (".c", ".h") or
                        relative.name in ("Makefile", "modules.mk"), "Unexpected source entry")
            key = str(relative)
            require(key not in entries and len(entries) < 1000 and
                    0 <= item.size <= 4 * 1024 * 1024, "Duplicate/oversized source")
            stream = archive.extractfile(item)
            value = stream.read(item.size + 1)
            total += len(value)
            require(len(value) == item.size and total <= MAX_DOWNLOAD, "Invalid source size")
            entries[key] = value
    require("Genesis/src/Makefile" in entries and
            "Genesis/src/classes/modules.mk" in entries and
            "deploy/update_compat.json" in entries, "Release is missing required source")
    compat = json.loads(entries.pop("deploy/update_compat.json"))
    require(compat == {"schema": 1, "save_epoch": 2, "handoff": 2},
            "Unsupported save/handoff compatibility; use a reviewed migration")
    for name, value in entries.items():
        path = destination.joinpath(*PurePosixPath(name).parts[2:])
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as output:
            output.write(value)
    return compat


def protected(path, directory=False):
    info = path.lstat()
    require((stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode))
            and info.st_uid == 0 and not info.st_mode & 0o022,
            "Expected root-owned protected path: " + str(path))
    if not directory:
        require(info.st_nlink == 1, "Unexpected hard links: " + str(path))


def sync_dir(path):
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def atomic_bytes(path, value, mode=0o600):
    fd, temporary = tempfile.mkstemp(prefix=".anathema-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as output:
            output.write(value)
            os.fchmod(output.fileno(), mode)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
        sync_dir(path.parent)
    finally:
        if os.path.lexists(temporary):
            os.unlink(temporary)


def write_record(record):
    atomic_bytes(RECORD, (json.dumps(record, sort_keys=True) + "\n").encode())


def read_binary(path, owner):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        require(stat.S_ISREG(info.st_mode) and info.st_uid == owner and
                info.st_nlink == 1 and 0 < info.st_size <= MAX_BINARY,
                "Invalid executable ownership/type/size")
        with os.fdopen(fd, "rb", closefd=False) as stream:
            value = stream.read(MAX_BINARY + 1)
        require(len(value) == info.st_size and value.startswith(b"\x7fELF"),
                "Invalid executable content")
        return value
    finally:
        os.close(fd)


def binary_record(value, commit=None, adopted=False):
    return {"schema": 1, "commit": commit, "binary_sha256": hashlib.sha256(value).hexdigest(),
            "save_epoch": 2, "handoff": 2, "status": "active",
            "provenance": "operator-adopted" if adopted else "CI-checked"}


def validate_record(record):
    require(isinstance(record, dict) and record.get("schema") == 1 and
            record.get("save_epoch") in (1, 2) and record.get("handoff") == 2 and
            record.get("status") in ("active", "staged") and
            isinstance(record.get("binary_sha256"), str) and
            HEX256.fullmatch(record["binary_sha256"]) and
            (record.get("commit") is None or
             isinstance(record["commit"], str) and SHA.fullmatch(record["commit"])),
            "Invalid/incompatible installed release record")
    return record


def load_record(value, adopt=False, recovering=False):
    if RECORD.exists() or RECORD.is_symlink():
        protected(RECORD)
        record = validate_record(json.loads(RECORD.read_text()))
        if record["binary_sha256"] != hashlib.sha256(value).hexdigest():
            require(recovering, "Installed binary changed; refusing an ordinary update")
            return binary_record(value, adopted=True)
        return record
    require(adopt, "First update requires --adopt-current: attest this is a modern "
            "yescrypt-compatible Anathema build, not historical Genesis")
    return binary_record(value, adopted=True)


def service_info():
    result = run("systemctl", "show", SERVICE, "--no-pager",
                 "--property=User,Group,WorkingDirectory,ExecStart,ActiveState,MainPID")
    values = dict(line.split("=", 1) for line in result.splitlines() if "=" in line)
    require(values.get("User") == "anathema" and values.get("Group") == "anathema"
            and values.get("WorkingDirectory") == str(DATA / "area") and
            re.search(r"path=/opt/anathema/src/merc\s*;", values.get("ExecStart", ""))
            and re.search(r"argv\[\]=/opt/anathema/src/merc 4000\s*;",
                          values.get("ExecStart", "")), "Unsupported service layout/port")
    return values


def host_preflight():
    require(os.geteuid() == 0, "--apply/--verify require root")
    account = pwd.getpwnam("anathema")
    require(account.pw_uid != 0, "Service account must not be root")
    for path in (Path("/opt"), APP, APP / "src", APP / "deploy",
                 Path("/var"), Path("/var/lib"), DATA, Path("/var/backups")):
        protected(path, directory=True)
    protected(BINARY)
    for name in ("area", "player", "backup", "notes", "log", "clans"):
        path = DATA / name
        info = path.lstat()
        require(stat.S_ISDIR(info.st_mode) and info.st_uid == account.pw_uid
                and not info.st_mode & 0o077, "Unexpected runtime directory: " + str(path))
    link = DATA / "src"
    require(link.is_symlink() and link.lstat().st_uid == 0 and
            link.resolve() == APP / "src", "Protected copyover symlink is incorrect")
    service_info()
    return account


@contextmanager
def update_lock():
    if not LOCK.parent.exists():
        protected(LOCK.parent.parent, directory=True)
        LOCK.parent.mkdir(mode=0o700)
    protected(LOCK.parent, directory=True)
    require(not LOCK.parent.stat().st_mode & 0o077, "Updater lock directory is not private")
    fd = os.open(LOCK, os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
    try:
        info = os.fstat(fd)
        require(stat.S_ISREG(info.st_mode) and info.st_uid == 0 and
                info.st_nlink == 1 and not info.st_mode & 0o077, "Unsafe updater lock")
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise UpdateError("Another updater is running") from error
        yield
    finally:
        os.close(fd)


def running_installed(info=None):
    info = info or service_info()
    pid = int(info.get("MainPID", "0"))
    if info.get("ActiveState") != "active" or pid <= 0:
        return False
    return os.path.samestat((Path("/proc") / str(pid) / "exe").stat(), BINARY.stat())


def greeting():
    deadline = time.monotonic() + 25
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", 4000), timeout=1) as connection:
                data = b""
                while time.monotonic() < deadline:
                    connection.settimeout(min(1, max(0.01, deadline - time.monotonic())))
                    try:
                        chunk = connection.recv(8192)
                    except socket.timeout:
                        continue
                    require(chunk, "Server closed before its greeting")
                    data += chunk
                    require(len(data) <= 65536, "Oversized greeting")
                    if b"tombstone:" in data.lower() and b"anathema" in data.lower():
                        return
        except (OSError, UpdateError):
            time.sleep(0.2)
    raise UpdateError("No Anathema TCP greeting within 25 seconds")


def stop():
    run("systemctl", "stop", SERVICE)
    info = service_info()
    require(info.get("ActiveState") in ("inactive", "failed") and
            info.get("MainPID") == "0", "Service did not stop; binary unchanged")


def start_checked():
    run("systemctl", "reset-failed", SERVICE)
    run("systemctl", "start", SERVICE)
    greeting()
    require(running_installed(), "Service is not running the installed executable")


def build(sha, account):
    """Return a root-held byte copy, not a service-controlled path."""
    archive = download(API + "/tarball/" + sha, MAX_DOWNLOAD)
    with tempfile.TemporaryDirectory(prefix="anathema-update-", dir="/var/tmp") as temporary:
        staging = Path(temporary)
        source = staging / "src"
        source.mkdir()
        archive_source(archive, source)
        for path in (staging, *staging.rglob("*")):
            os.chown(path, account.pw_uid, account.pw_gid)
        compile_source(source, account.pw_name)
        return read_binary(source / "merc", account.pw_uid)


def compile_source(source, username):
    with tempfile.TemporaryFile() as log:
        process = subprocess.Popen(
            ("runuser", "-u", username, "--", "setpriv", "--no-new-privs",
             "--", "make", "-C", str(source), "-j2"),
            stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
            env={"PATH": "/usr/sbin:/usr/bin:/sbin:/bin",
                 "HOME": "/nonexistent", "LANG": "C", "LC_ALL": "C"})
        try:
            result = process.wait(timeout=900)
        except subprocess.TimeoutExpired as error:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            raise UpdateError("Build timed out; build process group terminated") from error
        require(result == 0, "Build failed; game process untouched. "
                "Check source/dependencies with an unprivileged build.")


def builder_account(service):
    try:
        account = pwd.getpwnam("anathema-build")
    except KeyError:
        run("useradd", "--system", "--user-group", "--no-create-home", "--home-dir", "/nonexistent",
            "--shell", "/usr/sbin/nologin", "anathema-build")
        account = pwd.getpwnam("anathema-build")
    require(account.pw_uid not in (0, service.pw_uid) and
            account.pw_gid not in (0, service.pw_gid),
            "Dedicated build account must be separate from root and the game account")
    require(set(os.getgrouplist(account.pw_name, account.pw_gid)) == {account.pw_gid},
            "Build account must not have supplementary groups")
    group = grp.getgrgid(account.pw_gid)
    require(group.gr_name == "anathema-build" and
            set(group.gr_mem).issubset({account.pw_name}),
            "Build account must have its own private primary group")
    return account


def copy_state(source, destination):
    """Pin directory descriptors; never follow source links during root backup."""
    def copy_directory(source_fd, target, depth):
        require(depth <= 128, "Runtime directory nesting exceeds backup limit")
        target.mkdir(mode=0o700)
        with os.scandir(source_fd) as entries:
            for entry in entries:
                value = entry.stat(follow_symlinks=False)
                output = target / entry.name
                if stat.S_ISLNK(value.st_mode):
                    os.symlink(os.readlink(entry.name, dir_fd=source_fd), output)
                elif stat.S_ISDIR(value.st_mode):
                    child = os.open(entry.name, os.O_RDONLY | os.O_DIRECTORY |
                                    os.O_NOFOLLOW, dir_fd=source_fd)
                    try:
                        copy_directory(child, output, depth + 1)
                    finally:
                        os.close(child)
                else:
                    require(stat.S_ISREG(value.st_mode), "Special runtime files cannot be backed up")
                    child = os.open(entry.name, os.O_RDONLY | os.O_NOFOLLOW |
                                    os.O_NONBLOCK, dir_fd=source_fd)
                    try:
                        opened = os.fstat(child)
                        require(stat.S_ISREG(opened.st_mode), "Runtime file changed type during backup")
                        remaining = opened.st_size
                        fd = os.open(output, os.O_WRONLY | os.O_CREAT | os.O_EXCL |
                                     os.O_NOFOLLOW, 0o600)
                        with os.fdopen(fd, "wb") as stream:
                            while remaining:
                                chunk = os.read(child, min(remaining, 1024 * 1024))
                                require(chunk, "Runtime file shrank during backup")
                                stream.write(chunk)
                                remaining -= len(chunk)
                    finally:
                        os.close(child)
    source_fd = os.open(source, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        copy_directory(source_fd, destination, 0)
    finally:
        os.close(source_fd)


def snapshot(value, record, online):
    if not STORE.exists():
        STORE.mkdir(mode=0o700)
        sync_dir(STORE.parent)
    protected(STORE, directory=True)
    require(not STORE.stat().st_mode & 0o077, "Backup directory must be private (0700)")
    space = sum(p.lstat().st_size for p in DATA.rglob("*") if p.is_file() and not p.is_symlink())
    require(shutil.disk_usage(STORE).free > space + len(value) + 64 * 1024 * 1024,
            "Insufficient backup disk space")
    folder = Path(tempfile.mkdtemp(prefix=time.strftime("%Y%m%dT%H%M%SZ-",
                                                       time.gmtime()), dir=STORE))
    # mkdtemp's random suffix matches BACKUP_ID on supported CPython.
    require(BACKUP_ID.fullmatch(folder.name), "Invalid generated backup ID")
    copy_state(DATA, folder / "state")
    for directory, _, names in os.walk(folder / "state", topdown=False, followlinks=False):
        for name in names:
            path = Path(directory) / name
            if path.is_file() and not path.is_symlink():
                fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW)
                try:
                    os.fsync(fd)
                finally:
                    os.close(fd)
        sync_dir(Path(directory))
    atomic_bytes(folder / "merc", value, 0o755)
    manifest = {"schema": 1, "previous": record,
                "state_snapshot": "online-not-point-in-time" if online else "service-stopped"}
    atomic_bytes(folder / "manifest.json", (json.dumps(manifest) + "\n").encode())
    sync_dir(STORE)
    return folder.name


def backup_binary(identifier):
    require(isinstance(identifier, str) and BACKUP_ID.fullmatch(identifier),
            "Use a backup ID, not a path")
    protected(STORE, directory=True)
    folder = STORE / identifier
    protected(folder, directory=True)
    require(not folder.stat().st_mode & 0o077, "Backup directory is not private")
    protected(folder / "manifest.json")
    manifest = json.loads((folder / "manifest.json").read_text())
    require(manifest.get("schema") == 1, "Unknown backup schema")
    record = validate_record(manifest["previous"])
    value = read_binary(folder / "merc", 0)
    require(hashlib.sha256(value).hexdigest() == record["binary_sha256"],
            "Backup executable checksum mismatch")
    return value, dict(record, status="active")


def deploy(value, next_record, mode, adopt=False, recovering=False):
    old = read_binary(BINARY, 0)
    previous = load_record(old, adopt, recovering)
    validate_record(next_record)
    require(next_record["save_epoch"] >= previous["save_epoch"],
            "Unsafe save-epoch downgrade: older binaries may discard hybrid data. "
            "Use a reviewed data migration, not binary-only rollback.")
    info = service_info()
    online = info.get("ActiveState") == "active"
    if not recovering:
        require(previous["status"] != "staged", "Verify or roll back the staged update first")
        require(not online or running_installed(info),
                "Running executable differs from disk; resolve pending manual copyover first")
    if mode == "copyover":
        require(online and running_installed(info), "Copyover staging requires the old binary running")
    backup, replaced, stopped = None, False, False
    try:
        if mode == "restart":
            stopped = True
            stop()
        backup = snapshot(old, previous, mode == "copyover")
        # Rename can succeed before directory fsync fails; recovery must cover both.
        replaced = True
        atomic_bytes(BINARY, value, 0o755)
        record = dict(next_record, status="staged" if mode == "copyover" else "active",
                      previous_backup=backup)
        write_record(record)
        if mode == "restart":
            start_checked()
        return backup
    except Exception as error:
        if replaced:
            if next_record["save_epoch"] > previous["save_epoch"]:
                # The new process may have written optional hybrid records even
                # if its greeting failed. Never let an old executable discard
                # them. Leave the new executable and require operator repair.
                stop()
                raise UpdateError("Update failed across a save-epoch upgrade; "
                                  "service stopped, no old binary started. "
                                  "Backup ID: " + str(backup) +
                                  ". Repair with an epoch-compatible release; "
                                  "data were not restored.") from error
            try:
                stop()
                atomic_bytes(BINARY, old, 0o755)
                write_record(previous)
                start_checked()
            except Exception as recovery_error:
                raise UpdateError("Update failed; automatic binary recovery also failed. "
                                  "Backup ID: " + str(backup) +
                                  ". Inspect service/storage before explicit rollback.") from recovery_error
            raise UpdateError("Update failed; previous binary restored and greeting checked. "
                              "Backup ID: " + str(backup)) from error
        if stopped and online:
            try:
                start_checked()
            except Exception as recovery_error:
                raise UpdateError("Backup/pre-install failed and old service could not restart; "
                                  "binary was not replaced") from recovery_error
        raise


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true", help="explicitly change this host")
    parser.add_argument("--mode", choices=("restart", "copyover"), default="restart")
    parser.add_argument("--adopt-current", action="store_true",
                        help="first use: attest current binary is modern Anathema, not old Genesis")
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--rollback", metavar="BACKUP_ID")
    action.add_argument("--verify", action="store_true")
    args = parser.parse_args(argv)
    require(not args.rollback or args.mode == "restart", "Rollback uses a maintenance restart")
    if args.verify:
        host_preflight()
        with update_lock():
            value = read_binary(BINARY, 0)
            record = load_record(value)
            require(running_installed(), "Still running the old binary; copyover has not completed")
            greeting()
            write_record(dict(record, status="active"))
        print("Verified: service executable matches the installed binary; TCP greeting passed.")
        return
    if args.rollback:
        require(BACKUP_ID.fullmatch(args.rollback), "Use a backup ID, not a path")
        if not args.apply:
            print("Preview: roll back binary from " + args.rollback +
                  " with a maintenance restart; live state is NOT restored. Add --apply.")
            return
        host_preflight()
        with update_lock():
            value, record = backup_binary(args.rollback)
            backup = deploy(value, record, "restart", args.adopt_current, recovering=True)
        print("Binary rolled back. Current data retained. Pre-rollback backup: " + backup)
        return
    sha, runs = checked_release()
    print("CI-checked target: " + sha + "; workflows " + ", ".join(map(str, runs)))
    print("Mode: " + args.mode + ". Save unsaved OLC edits with asave before maintenance.")
    if not args.apply:
        print("Preview only. No host changes. Add --apply; first managed update also needs "
              "--adopt-current. Copyover staging does NOT perform the in-game handover.")
        return
    account = host_preflight()
    with update_lock():
        current = load_record(read_binary(BINARY, 0), args.adopt_current)
        require(current["status"] != "staged", "Verify or roll back the staged update first")
        value = build(sha, builder_account(account))
        # Do not silently adopt a moving branch while building.
        require(api("/git/ref/heads/" + BRANCH)["object"]["sha"] == sha,
                "Branch moved during build; retry after reviewing the new head")
        backup = deploy(value, binary_record(value, sha), args.mode, args.adopt_current)
    print("Backup ID: " + backup)
    if args.mode == "copyover":
        print("STAGED ONLY. Process left running. Online snapshot is NOT point-in-time. "
              "As Luto run copyover in-game, then python3 deploy/update.py --verify over SSH.")
    else:
        print("Updated: installed binary running; TCP greeting passed. Current world/data retained.")


if __name__ == "__main__":
    try:
        main()
    except (UpdateError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        # Build output may contain arbitrary file data; do not dump captured streams.
        print("Update stopped: " + str(error), file=__import__("sys").stderr)
        raise SystemExit(1)
