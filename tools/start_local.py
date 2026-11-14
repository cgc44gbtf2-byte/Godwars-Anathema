#!/usr/bin/env python3
"""Validate the unpacked world and start the MUD as the current ordinary user."""
import argparse
import importlib.util
import os
from pathlib import Path
import stat

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("anathema_world", REPO / "deploy/setup.py")
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)
RUNTIME = ("area", "player", "backup", "notes", "log", "clans")


def prepare_runtime(root):
    """Never chown, remove files, follow runtime symlinks or import accounts."""
    paths = [root / name for name in RUNTIME]
    for path in paths:
        if os.path.lexists(path):
            info = path.lstat()
            if not stat.S_ISDIR(info.st_mode) or info.st_uid != os.geteuid():
                raise ValueError("Runtime directories must be ordinary directories owned by you: " + str(path))
    for path in paths:
        path.mkdir(mode=0o700, exist_ok=True)
        path.chmod(0o700)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=4000)
    parser.add_argument("--check", action="store_true", help="validate files without starting or changing directories")
    args = parser.parse_args(argv)
    if not 1024 < args.port <= 65535:
        parser.error("port must be 1025..65535")
    files = setup.world_files(REPO / "Genesis/area")
    binary = REPO / "Genesis/src/merc"
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise ValueError("Build first with make")
    if args.check:
        print("World ready:", len(files), "files; area.lst references verified; executable present")
        return
    if os.geteuid() == 0:
        raise ValueError("Do not run the game as root. Use an ordinary-user checkout or deploy/setup.py --apply --start.")
    os.umask(0o077)
    prepare_runtime(REPO / "Genesis")
    os.chdir(REPO / "Genesis/area")
    os.execv(str(binary), [str(binary), str(args.port)])


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError) as error:
        raise SystemExit("Local start stopped: " + str(error))
