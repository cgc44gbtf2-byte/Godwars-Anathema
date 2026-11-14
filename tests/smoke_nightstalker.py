"""Native Nightstalker identity through real TCP commands and executable exec.

Only synthetic players in a private temporary copy of the world are used.
"""
import pathlib
import signal
import socket
import subprocess
import sys
import tempfile
import time
from server_process import start_server
from world_fixture import stage_world

REPO = pathlib.Path(__file__).resolve().parents[1]

def until(client, needle):
    data = b""
    deadline = time.monotonic() + 25
    client.settimeout(.5)
    while needle.lower() not in data.lower():
        if time.monotonic() > deadline:
            raise AssertionError("Nightstalker response deadline exceeded")
        try:
            part = client.recv(65536)
        except socket.timeout:
            continue
        assert part, "Connection closed before Nightstalker response"
        data += part
        assert len(data) < 2_000_000
    return data

with tempfile.TemporaryDirectory(prefix="anathema-nightstalker-") as directory:
    runtime = pathlib.Path(directory)
    for name in ("area", "player", "backup", "notes", "log", "clans"):
        (runtime / name).mkdir(mode=0o700)
    (runtime / "src").symlink_to(REPO / "Genesis/src", target_is_directory=True)
    stage_world(REPO, runtime)
    subprocess.run([str(REPO / "tests/test_restart"), "--seed",
                    str(runtime / "area")], check=True)
    player = runtime / "player/Restarter"
    lines = player.read_text().splitlines()
    # Reuse the real loader's authenticated fixture, but make this a purely
    # native Ninja. No hybrid conversion or secondary powers are exercised.
    lines = [line for line in lines if not line.startswith(
        ("AnathemaHybrid ", "AnathemaHybridForms "))]
    assert sum(line.startswith("Clandisc ") for line in lines) == 1
    assert sum(line.startswith("Ninja ") for line in lines) == 1
    lines = ["Clandisc 1001 -1 -1" if line.startswith("Clandisc ") else
             "Ninja 10 10 10 300 0" if line.startswith("Ninja ") else line
             for line in lines]
    player.write_text("\n".join(lines) + "\n")
    with (runtime / "server.log").open("w") as log:
        process, port = start_server(REPO / "Genesis/src/merc", runtime / "area", log)
        client = None
        try:
            deadline = time.monotonic() + 25
            while True:
                assert process.poll() is None
                try:
                    client = socket.create_connection(("127.0.0.1", port), timeout=1)
                    break
                except OSError:
                    if time.monotonic() > deadline:
                        raise AssertionError("Nightstalker server did not start")
                    time.sleep(.1)
            until(client, b"tombstone")
            client.sendall(b"Restarter\n"); until(client, b"Password")
            client.sendall(b"test-only-passphrase\n"); until(client, b"Wizinvis")
            client.sendall(b"n\n"); until(client, b"Welcome to Godwars")
            for after_exec in (False, True):
                client.sendall(b"principle\n")
                until(client, b"Nightstalker: no Michi")
                client.sendall(b"michi\n")
                until(client, b"Nightstalkers cannot enter Michi")
                client.sendall(b"save\n"); until(client, b"Saved.")
                fields = next(line.split() for line in player.read_text().splitlines()
                              if line.startswith("Clandisc "))
                assert fields[1] == "1001", "Nightstalker identity lost"
                assert "AnathemaHybrid " not in player.read_text()
                if not after_exec:
                    client.sendall(b"copyover\n")
                    until(client, b"Copyover recovery complete")
            client.sendall(b"shutdown\n")
            assert process.wait(timeout=15) == 0
        except Exception:
            log.flush()
            print("\n".join((runtime / "server.log").read_text(
                errors="replace").splitlines()[-80:]), file=sys.stderr)
            raise
        finally:
            if client is not None:
                client.close()
            if process.poll() is None:
                process.send_signal(signal.SIGTERM)
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
    text = (runtime / "server.log").read_text()
    assert text.count("ready to rock on port") == 2
    assert "Copyover recovery complete." in text
    assert "Normal termination of game." in text
    assert not list((runtime / "player").glob(".*"))
print("Nightstalker: native identity/menu/Michi rejection retained across real exec/TCP and checked shutdown")
