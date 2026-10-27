#!/usr/bin/env python3
"""Real TCP, save and exec-copyover on a private synthetic Gargoyle runtime."""
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
            raise AssertionError("Gargoyle response deadline exceeded")
        try:
            part = client.recv(65536)
        except socket.timeout:
            continue
        assert part, "Connection closed before Gargoyle response"
        data += part
        assert len(data) < 2_000_000
    return data

def power_menu(client):
    client.sendall(b"powers\n")
    data = until(client, b"next pulse")
    assert b"Visceratika:" in data and b"Camouflage" in data and b"Stoneform" in data
    assert b"50% damage reduction" in data and b"Punch resistance" in data
    assert b"25% reflected HP damage" in data
    assert b"Thaumaturgy:" not in data and b"You have no disciplines" not in data
    assert b"cannot act while a stone statue" not in data

with tempfile.TemporaryDirectory(prefix="anathema-gargoyle-") as directory:
    runtime = pathlib.Path(directory)
    for name in ("area", "player", "backup", "notes", "log", "clans"):
        (runtime / name).mkdir(mode=0o700)
    (runtime / "src").symlink_to(REPO / "Genesis/src", target_is_directory=True)
    stage_world(REPO, runtime)
    subprocess.run([str(REPO / "tests/test_restart"), "--seed",
                    str(runtime / "area")], check=True)
    player = runtime / "player/Restarter"
    lines = [line for line in player.read_text().splitlines() if not line.startswith(
        ("AnathemaHybrid ", "AnathemaHybridForms "))]
    assert all(sum(line.startswith(key + " ") for line in lines) == 1
               for key in ("Class", "Clandisc", "Powers", "Vampgen", "Primal", "Quest"))
    # CLASS_VAMPIRE is 1; this fixture exercises no secondary hybrid state.
    lines = ["Class 1" if line.startswith("Class ") else
             "Clandisc 1002 -1 -1" if line.startswith("Clandisc ") else
             "Vampgen 9" if line.startswith("Vampgen ") else
             "Primal 100" if line.startswith("Primal ") else
             "Quest 400" if line.startswith("Quest ") else
             "Powers " + " ".join("5" if i == 17 else "0" for i in range(20))
             if line.startswith("Powers ") else line for line in lines]
    player.write_text("\n".join(lines) + "\n")
    with (runtime / "server.log").open("w") as log:
        process, port = start_server(REPO / "Genesis/src/merc", runtime / "area", log)
        client = None
        try:
            client = socket.create_connection(("127.0.0.1", port), timeout=2)
            until(client, b"tombstone")
            client.sendall(b"Restarter\n"); until(client, b"Password")
            client.sendall(b"test-only-passphrase\n"); until(client, b"Wizinvis")
            client.sendall(b"n\n"); until(client, b"Welcome to Godwars")
            for after_exec in (False, True):
                if after_exec:
                    client.sendall(b"stoneform status\n")
                    until(client, b"Stone form: immobile defensive statue")
                    power_menu(client)
                    client.sendall(b"stoneform off\n")
                    until(client, b"returns to flesh")
                if not after_exec:
                    client.sendall(b"varmor mark\n")
                    until(client, b"appears in your hands")
                client.sendall(b"discipline\n")
                output = until(client, b"To improve a discipline rating")
                assert b"Visceratika" in output and b"Thaumaturgy" not in output
                power_menu(client)
                client.sendall(b"tasteblood self\n")
                until(client, b"not trained high enough")
                client.sendall(b"stoneform on\n")
                until(client, b"immobile defensive stone statue")
                power_menu(client)
                client.sendall(b"north\n")
                until(client, b"cannot act while a stone statue")
                client.sendall(b"save\n"); until(client, b"Saved.")
                text = player.read_text()
                fields = next(line.split() for line in text.splitlines()
                              if line.startswith("Clandisc "))
                assert fields[1:] == ["1002", "-1", "-1"]
                assert "AnathemaHybrid " not in text
                assert next(line.split()[1] for line in text.splitlines()
                            if line.startswith("Polyaff ")) == "2097152"
                assert any(line.split() == ["Vnum", "29630"] for line in text.splitlines())
                assert next(line.split()[1] for line in text.splitlines()
                            if line.startswith("Primal ")) == "0"
                assert next(line.split()[1] for line in text.splitlines()
                            if line.startswith("Quest ")) == "0"
                if not after_exec:
                    client.sendall(b"copyover\n")
                    until(client, b"Copyover recovery complete")
            client.sendall(b"stoneform off\n")
            until(client, b"returns to flesh")
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
                    process.kill(); process.wait()
    text = (runtime / "server.log").read_text(errors="replace")
    assert text.count("ready to rock on port") == 2
    assert "Copyover recovery complete." in text and "Normal termination of game." in text
    assert "AddressSanitizer" not in text and "runtime error:" not in text
print("Gargoyle TCP/save/actual exec-copyover/Thaumaturgy-denial passed")
