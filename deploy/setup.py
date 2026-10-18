#!/usr/bin/env python3
"""Guided FRESH Debian/Ubuntu install. Preview by default; --apply changes host."""
import argparse
import grp
import os
from pathlib import Path, PurePosixPath
import pwd
import shutil
import stat
import subprocess
import tempfile
import zipfile

REPO = Path(__file__).resolve().parents[1]
APP = Path("/opt/anathema")
DATA = Path("/var/lib/anathema")
UNIT = Path("/etc/systemd/system/anathema.service")
HOST = "137.184.145.164"
PORT = 4000
LIMIT = 128 * 1024 * 1024
BUILD_ROOT = Path("/var/tmp")


def world_files(archive):
    """Read only flat area/help/list data; never import accounts or runtime state."""
    files, seen, total = {}, set(), 0
    with zipfile.ZipFile(archive) as source:
        for entry in source.infolist():
            path = PurePosixPath(entry.filename)
            if str(path.parent) != "Genesis/area" or not (
                path.suffix.lower() in (".are", ".hlp") or path.name == "area.lst"
            ):
                continue
            if (not path.name or "\\" in path.name or path.name.casefold() in seen or
                    stat.S_ISLNK(entry.external_attr >> 16) or entry.is_dir()):
                raise ValueError("Unsafe/duplicate world-data entry")
            total += entry.file_size
            if total > LIMIT or entry.file_size > 16 * 1024 * 1024 or len(files) >= 2000:
                raise ValueError("World-data size limit exceeded")
            seen.add(path.name.casefold())
            files[path.name] = source.read(entry)
    if "area.lst" not in files:
        raise ValueError("Archive needs Genesis/area/area.lst")
    names = files["area.lst"].decode("ascii").split()
    if not names or names[-1] != "$" or "$" in names[:-1]:
        raise ValueError("area.lst must end with one $")
    for name in names[:-1]:
        if "/" in name or "\\" in name or name in (".", "..") or name not in files:
            raise ValueError("Missing/unsafe area.lst reference: " + name)
    return files


def fresh_paths(paths):
    for path in paths:
        if os.path.lexists(path):
            raise ValueError("Fresh install only; existing path must be inspected: " + str(path))


def new_file(path, data, mode, uid=None, gid=None):
    with path.open("xb") as out:
        out.write(data)
        out.flush()
        os.fchmod(out.fileno(), mode)
        if uid is not None:
            os.fchown(out.fileno(), uid, gid)
        os.fsync(out.fileno())


def install_binary(source, destination, uid):
    fd = os.open(source, os.O_RDONLY | os.O_NOFOLLOW)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_uid != uid or info.st_nlink != 1:
            raise ValueError("Build did not produce an owned regular executable")
        if not 0 < info.st_size < LIMIT:
            raise ValueError("Invalid executable size")
        with os.fdopen(fd, "rb", closefd=False) as binary:
            new_file(destination, binary.read(LIMIT), 0o755)
    finally:
        os.close(fd)


def run(*arguments, **options):
    subprocess.run(arguments, check=True, **options)

def trusted_parents():
    for path in (APP.parent, DATA.parent, UNIT.parent):
        info = path.stat()
        if not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022:
            raise ValueError("Installation parent must be root-owned and not writable by others: " + str(path))


def install(files, start):
    if os.geteuid() != 0:
        raise ValueError("Use sudo python3 deploy/setup.py --apply")
    release = {}
    for line in Path("/etc/os-release").read_text().splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            release[key] = value.strip('"')
    if (release.get("ID"), release.get("VERSION_ID")) not in {
        ("debian", "12"), ("debian", "13"), ("ubuntu", "22.04"), ("ubuntu", "24.04")
    }:
        raise ValueError("Supported hosts: Debian 12/13, Ubuntu 22.04/24.04")
    fresh_paths((APP, DATA, UNIT))
    trusted_parents()
    for lookup in (pwd.getpwnam, grp.getgrnam):
        try:
            lookup("anathema")
        except KeyError:
            continue
        raise ValueError("Fresh install only; anathema user/group already exists")
    if not Path("/run/systemd/system").is_dir():
        raise ValueError("Install on a host booted with systemd")
    source = REPO / "Genesis/src"
    if any(path.is_symlink() for path in source.rglob("*")):
        raise ValueError("Review source symlinks before installation")
    run("apt-get", "update")
    run("apt-get", "install", "-y", "--no-install-recommends",
        "build-essential", "libcrypt-dev", "systemd", "ca-certificates")
    run("useradd", "--system", "--user-group", "--no-create-home",
        "--home-dir", str(DATA), "--shell", "/usr/sbin/nologin", "anathema")
    account = pwd.getpwnam("anathema")
    with tempfile.TemporaryDirectory(prefix="anathema-build-", dir=BUILD_ROOT) as directory:
        staging = Path(directory)
        staging.chmod(0o755)
        build = staging / "src"
        shutil.copytree(source, build,
                        ignore=shutil.ignore_patterns("*.o", "*.d", "merc", "memwatch.log"))
        for path in [build, *build.rglob("*")]:
            os.chown(path, account.pw_uid, account.pw_gid)
        # A root-owned private checkout may be under /root (not traversable
        # by the service user). Start the build from accessible staging.
        run("runuser", "-u", "anathema", "--", "make", "-C", str(build), "-j1",
            cwd=staging)
        # No root compilation; protected parents keep the executable immutable.
        APP.mkdir(mode=0o755); (APP / "src").mkdir(mode=0o755)
        (APP / "deploy").mkdir(mode=0o755)
        DATA.mkdir(mode=0o755)
        for path in (APP, APP / "src", APP / "deploy", DATA):
            path.chmod(0o755)
        for name in ("area", "player", "backup", "notes", "log", "clans"):
            path = DATA / name
            path.mkdir(mode=0o700)
            path.chmod(0o700)
            os.chown(path, account.pw_uid, account.pw_gid)
        install_binary(build / "merc", APP / "src/merc", account.pw_uid)
    new_file(APP / "deploy/INSTALL.md", (REPO / "deploy/INSTALL.md").read_bytes(), 0o644)
    (DATA / "src").symlink_to(APP / "src", target_is_directory=True)
    for name, content in files.items():
        new_file(DATA / "area" / name, content, 0o600, account.pw_uid, account.pw_gid)
    new_file(UNIT, (REPO / "deploy/anathema.service").read_bytes(), 0o644)
    run("systemd-analyze", "verify", str(UNIT))
    run("systemctl", "daemon-reload")
    if start:
        run("systemctl", "enable", "--now", "anathema.service")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--world-archive", type=Path, default=REPO / "Genesis.zip")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--start", action="store_true", help="enable/start only after successful install")
    args = parser.parse_args()
    if args.start and not args.apply:
        parser.error("--start requires --apply")
    files = world_files(args.world_archive)
    print("Fresh install:", APP, DATA, UNIT, flush=True)
    print("Endpoint:", f"{HOST}:{PORT}", "- listens on all interfaces; no IP allowlist", flush=True)
    print("World files:", len(files), "- no historical player accounts, notes or runtime files", flush=True)
    print("Review world content and resolve original licensing before operation.", flush=True)
    if not args.apply:
        print("Preview only. Apply: sudo python3 deploy/setup.py --apply --start")
        return
    install(files, args.start)
    print("Installed. Start/stop: sudo systemctl start/stop anathema.service")
    print("Allow inbound TCP 4000 in host/provider firewalls if needed; no firewall rules changed.")
    print("Create and log in as Luto normally, then type authorize.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        raise SystemExit("Setup stopped; inspect partial state before retrying: " + str(error))
