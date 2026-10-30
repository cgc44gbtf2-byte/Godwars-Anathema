#!/usr/bin/env python3
"""Opt-in default-layout Discord alerts; never restarts or updates the MUD."""
import argparse
from contextlib import contextmanager
import fcntl
import getpass
import http.client
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import tempfile
import time
import urllib.error
import urllib.request
import warnings

CONFIG_DIR = Path("/etc/anathema")
CONFIG = CONFIG_DIR / "monitor.json"
STATE_DIR = Path("/var/lib/anathema-monitor")
STATE = STATE_DIR / "state.json"
APP = Path("/opt/anathema/deploy")
UNITS = Path("/etc/systemd/system")
UPDATE_LOCK = Path("/run/anathema-update/lock")
WEBHOOK = re.compile(r"https://discord\.com/api/(?:v10/)?webhooks/[0-9]{6,30}/[A-Za-z0-9_.-]{20,300}")
PATHS = ("/", "/var/lib/anathema", "/var/backups")


class MonitorError(Exception):
    pass


def require(value, text):
    if not value:
        raise MonitorError(text)


def protected(path, directory=False, private=False):
    value = path.lstat()
    require((stat.S_ISDIR(value.st_mode) if directory else stat.S_ISREG(value.st_mode))
            and value.st_uid == 0 and not value.st_mode & (0o077 if private else 0o022),
            "Unsafe monitor file ownership, type or permissions")


def private_directory(path):
    protected(path.parent, directory=True)
    path.mkdir(mode=0o700, exist_ok=True)
    protected(path, directory=True, private=True)


def read_json(path):
    protected(path, private=True)
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    with os.fdopen(fd, "rb") as stream:
        value = os.fstat(stream.fileno())
        require(stat.S_ISREG(value.st_mode) and value.st_uid == 0 and
                value.st_nlink == 1 and not value.st_mode & 0o077 and value.st_size <= 65536,
                "Unsafe monitor configuration/state")
        result = json.loads(stream.read(65537))
        require(isinstance(result, dict), "Invalid monitor JSON object")
        return result


def atomic(path, value):
    fd, name = tempfile.mkstemp(dir=path.parent, prefix=".monitor-")
    try:
        with os.fdopen(fd, "wb") as stream:
            os.fchmod(stream.fileno(), 0o600)
            stream.write(value)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
        fd = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def config():
    protected(CONFIG_DIR, directory=True, private=True)
    value = read_json(CONFIG)
    require(value.get("schema") == 1 and isinstance(value.get("webhook"), str)
            and WEBHOOK.fullmatch(value["webhook"]), "Invalid monitor configuration/webhook")
    require(type(value.get("disk_percent")) is int and 1 <= value["disk_percent"] <= 99
            and type(value.get("min_free_bytes")) is int and value["min_free_bytes"] > 0,
            "Invalid disk thresholds")
    return value


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        raise MonitorError("Discord redirects are refused")


def send(url, text):
    require(WEBHOOK.fullmatch(url), "Invalid Discord webhook")
    require(len(text) <= 1900, "Alert exceeds delivery limit")
    payload = json.dumps({"username": "Anathema Alerts", "content": text,
                          "allowed_mentions": {"parse": []}}).encode()
    request = urllib.request.Request(url, data=payload, method="POST",
                                    headers={"Content-Type": "application/json",
                                             "User-Agent": "Anathema-Monitor/1"})
    try:
        with urllib.request.build_opener(NoRedirect()).open(request, timeout=10) as response:
            require(200 <= response.status < 300, "Discord delivery failed")
    except urllib.error.HTTPError as error:
        # Never stringify errors: urllib exceptions can contain the secret URL.
        raise MonitorError("Discord rejected delivery (HTTP %s)" % error.code) from None
    except (OSError, ValueError, http.client.HTTPException):
        raise MonitorError("Discord delivery unavailable") from None


def command(*args):
    return subprocess.run(args, check=True, text=True, capture_output=True, timeout=10).stdout


@contextmanager
def maintenance():
    fd = None
    try:
        if UPDATE_LOCK.exists():
            fd = os.open(UPDATE_LOCK, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
            value = os.fstat(fd)
            require(stat.S_ISREG(value.st_mode) and value.st_uid == 0
                    and not value.st_mode & 0o077, "Unsafe updater lock")
            try:
                fcntl.flock(fd, fcntl.LOCK_SH | fcntl.LOCK_NB)
            except BlockingIOError:
                yield True
                return
        yield False
    finally:
        if fd is not None:
            os.close(fd)


def sample(settings, skip_service=False):
    problems, restarts = {}, None
    if not skip_service:
        try:
            output = command("systemctl", "show", "anathema.service", "--no-pager",
                             "--property=ActiveState,SubState,Result,MainPID,NRestarts")
            service = dict(line.split("=", 1) for line in output.splitlines() if "=" in line)
            require(service.get("ActiveState") in ("active", "inactive", "failed",
                    "activating", "deactivating", "reloading"), "Unknown service state")
            restarts = int(service["NRestarts"])
            require(restarts >= 0, "Invalid restart counter")
            if service["ActiveState"] != "active" or int(service.get("MainPID", "0")) <= 0:
                problems["service"] = "MUD service is not active (state: %s)." % service["ActiveState"]
        except (MonitorError, OSError, ValueError, KeyError, subprocess.SubprocessError):
            problems["service"] = "MUD service status could not be read."
    seen = set()
    for path in PATHS:
        try:
            device = os.stat(path).st_dev
            if device in seen:
                continue
            seen.add(device)
            fs = os.statvfs(path)
            free = fs.f_bavail * fs.f_frsize
            used = 100 * (fs.f_blocks - fs.f_bavail) // max(1, fs.f_blocks)
            inode_used = 100 * (fs.f_files - fs.f_favail) // max(1, fs.f_files)
            if used >= settings["disk_percent"] or free < settings["min_free_bytes"] or (
                    fs.f_files and inode_used >= settings["disk_percent"]):
                problems["disk:" + str(device)] = (
                    "Low disk space on %s: %d%% used, %.2f GiB available; inodes %d%% used."
                    % (path, used, free / 1024**3, inode_used))
        except OSError:
            problems["path:" + path] = "Cannot inspect disk space at %s." % path
    return problems, restarts


def transition(old, problems, restarts, now, skip_service=False):
    result = dict(old)
    previous = old.get("problems", {})
    problems = dict(problems)
    if skip_service:
        if "service" in previous:
            problems["service"] = previous["service"]
        restarts = old.get("restarts")
    pending = list(old.get("pending", []))
    for key, text in problems.items():
        if previous.get(key) != text and (key not in previous or key == "service"):
            pending.append("ALERT: " + text)
    for key in previous.keys() - problems.keys():
        pending.append("RECOVERED: " + ("MUD service is active." if key == "service"
                                       else "Disk check %s is healthy again." % key))
    last = old.get("restarts")
    if restarts is not None and restarts > (last if last is not None else 0):
        pending.append("RESTART: systemd reports %d automatic restart(s); current state: %s."
                       % (restarts, "unavailable" if "service" in problems else "active"))
    if problems and not pending and now - old.get("last_sent", now) >= 21600:
        pending.append("REMINDER: " + " ".join(problems.values()))
    if len(pending) > 50:
        pending = ["Earlier events were coalesced during an extended delivery outage."] + pending[-49:]
    result.update(problems=problems, restarts=restarts, pending=pending)
    return result


def deliver(state, settings, now):
    if not state.get("pending") or now < state.get("next_attempt", 0):
        return state
    lines, count = ["Godwars: Anathema server monitor"], 0
    for event in state["pending"]:
        if len("\n".join(lines + [event])) > 1900:
            break
        lines.append(event)
        count += 1
    require(count > 0, "Malformed oversized pending event")
    try:
        send(settings["webhook"], "\n".join(lines))
    except MonitorError:
        failures = min(10, state.get("failures", 0) + 1)
        state.update(failures=failures, next_attempt=now + min(3600, 60 * 2**(failures - 1)))
        print("Discord delivery failed; queued events retained with bounded retry backoff.")
    else:
        state.update(pending=state["pending"][count:], failures=0, next_attempt=0, last_sent=now)
    return state


def check(settings):
    private_directory(STATE_DIR)
    fd = os.open(STATE_DIR / "lock", os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        old = read_json(STATE) if STATE.exists() else {}
        with maintenance() as skip:
            problems, restarts = sample(settings, skip)
        now = time.time()
        state = transition(old, problems, restarts, now, skip)
        persisted = True
        try:
            atomic(STATE, (json.dumps(state) + "\n").encode())
        except OSError:
            persisted = False
            state["pending"].insert(0, "ALERT: Monitor state storage is unavailable; duplicate alerts are possible.")
        state = deliver(state, settings, now)
        try:
            atomic(STATE, (json.dumps(state) + "\n").encode())
        except OSError:
            persisted = False
        require(persisted, "Monitor state storage failed; delivery was attempted without durable deduplication")
    finally:
        os.close(fd)


def install():
    config()
    for folder in (Path("/opt"), APP.parent, APP, Path("/etc"), UNITS.parent, UNITS):
        protected(folder, directory=True)
    source = Path(__file__).resolve().parent
    for name, target in (("monitor.py", APP / "monitor.py"),
                         ("anathema-monitor.service", UNITS / "anathema-monitor.service"),
                         ("anathema-monitor.timer", UNITS / "anathema-monitor.timer")):
        atomic(target, (source / name).read_bytes())
        target.chmod(0o644)
    command("systemctl", "daemon-reload")
    command("systemctl", "enable", "--now", "anathema-monitor.timer")
    print("Monitor timer enabled. The MUD service was not restarted or modified.")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    choice = parser.add_mutually_exclusive_group(required=True)
    for name in ("configure", "install", "test", "check"):
        choice.add_argument("--" + name, action="store_true")
    args = parser.parse_args(argv)
    require(os.geteuid() == 0, "Monitor setup/check requires root")
    if args.configure:
        require(os.isatty(0), "Configuration requires a private interactive SSH terminal")
        private_directory(CONFIG_DIR)
        with warnings.catch_warnings():
            warnings.simplefilter("error", getpass.GetPassWarning)
            try:
                url = getpass.getpass("Discord webhook URL (hidden; never paste into chat): ").strip()
            except getpass.GetPassWarning:
                raise MonitorError("Cannot hide terminal input; configuration cancelled") from None
        require(WEBHOOK.fullmatch(url), "Invalid Discord webhook URL")
        atomic(CONFIG, (json.dumps({"schema": 1, "webhook": url, "disk_percent": 90,
                                   "min_free_bytes": 1024**3}) + "\n").encode())
        print("Configuration saved privately. No message sent.")
    elif args.install:
        install()
    elif args.test:
        send(config()["webhook"], "Godwars: Anathema monitor test. Delivery configuration works.")
        print("Discord accepted the test message.")
    else:
        check(config())


if __name__ == "__main__":
    try:
        main()
    except MonitorError as error:
        print("Monitor stopped: " + str(error))
        raise SystemExit(1)
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError):
        print("Monitor stopped: configuration, state or host operation failed; no secret values logged.")
        raise SystemExit(1)
