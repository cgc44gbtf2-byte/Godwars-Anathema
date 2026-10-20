"""Numeric rejection through real short/full login; unrelated player stays online."""
import pathlib
import re
import secrets
import signal
import socket
import subprocess
import tempfile
import time
from world_fixture import stage_world

REPO = pathlib.Path(__file__).resolve().parents[1]


def until(client, needle):
    deadline, data = time.monotonic() + 25, b""
    client.settimeout(0.5)
    while needle.lower() not in data.lower():
        assert time.monotonic() < deadline, "Numeric smoke response timed out"
        try:
            part = client.recv(65536)
        except socket.timeout:
            continue
        assert part, "Unexpected disconnect"
        data += part
        assert len(data) < 2_000_000
    return data


def closed(client):
    deadline = time.monotonic() + 25
    client.settimeout(0.5)
    while time.monotonic() < deadline:
        try:
            if not client.recv(65536):
                return
        except socket.timeout:
            continue
    raise AssertionError("Expected numeric smoke disconnect did not arrive")


def signup(client, name, password):
    client.sendall(name.encode() + b"\n"); until(client, b"tombstone")
    client.sendall(b"y\n"); until(client, b"Give me a password")
    client.sendall(password + b"\n"); until(client, b"retype password")
    client.sendall(password + b"\n"); until(client, b"your sex")
    client.sendall(b"m\n"); until(client, b"support ANSI")
    client.sendall(b"n\n"); until(client, b"Anathema modifications")
    client.sendall(b"\n"); until(client, b"Welcome to Godwars")


def run(word_failures=False, string_failures=False):
    with tempfile.TemporaryDirectory(prefix="anathema-number-wire-") as directory:
        runtime = pathlib.Path(directory)
        for name in ("area", "player", "backup", "notes", "log", "clans"):
            (runtime / name).mkdir(mode=0o700)
        (runtime / "src").symlink_to(REPO / "Genesis/src", target_is_directory=True)
        stage_world(REPO, runtime)
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        with (runtime / "server.log").open("w") as log:
            process = subprocess.Popen(
                [str(REPO / "Genesis/src/merc"), str(port)],
                cwd=runtime / "area", stdout=log, stderr=log)
            clients = []

            def connect():
                client = socket.create_connection(("127.0.0.1", port), timeout=1)
                clients.append(client)
                return client

            try:
                deadline = time.monotonic() + 25
                while True:
                    assert process.poll() is None, "Server exited before numeric smoke"
                    try:
                        control = connect()
                        break
                    except OSError:
                        assert time.monotonic() < deadline, "Numeric smoke boot timed out"
                        time.sleep(0.1)
                # Generated, ephemeral credentials are never printed or retained.
                password = secrets.token_urlsafe(24).encode()
                signup(control, "Holdfixture", password)
                victim = connect()
                signup(victim, "Luto", password)
                victim.sendall(b"authorize\n")
                until(victim, b"highest staff level (7)")
                victim.sendall(b"quit\n"); closed(victim)
                player = runtime / "player/Luto"
                original = player.read_bytes()
                bad_short, count = re.subn(rb"(?m)^Gold[ \t]+[^\r\n]*",
                                          b"Gold +", original)
                assert count == 1
                if word_failures:
                    # Private fixture only: exceed the engine's 399-byte word.
                    bad_short = b"#PLAYER\n" + b"X" * 401 + b"\n"
                if string_failures:
                    bad_short = b"#PLAYER\nName Luto~\nTitle unterminated"
                player.write_bytes(bad_short)
                rejected = connect()
                rejected.sendall(b"Luto\n")
                until(rejected, b"Player data could not be loaded safely")
                closed(rejected)
                assert player.read_bytes() == bad_short
                control.sendall(b"level\n"); until(control, b"Generation:")
                assert original.count(b"#END\n") == 1
                bad_full = original.replace(
                    b"#END\n",
                    b"#OBJECT\nName failure token~\nNest 0\nCost +\nEnd\n#END\n")
                if word_failures:
                    # A full-login inventory word with no closing quote.
                    bad_full = original.replace(
                        b"#END\n", b"#OBJECT\n'unterminated\n")
                if string_failures:
                    # Short loading skips inventory. This string is only read
                    # during full loading after the real password is checked;
                    # it fails before a prototype or world item is needed.
                    bad_full = original.replace(
                        b"#END\n", b"#OBJECT\nName unterminated")
                player.write_bytes(bad_full)
                rejected = connect()
                rejected.sendall(b"Luto\n"); until(rejected, b"Password")
                rejected.sendall(password + b"\n")
                until(rejected, b"Player data could not be loaded safely")
                closed(rejected)
                assert player.read_bytes() == bad_full
                control.sendall(b"level\n"); until(control, b"Generation:")
                player.write_bytes(original)
                repaired = connect()
                repaired.sendall(b"Luto\n"); until(repaired, b"Password")
                repaired.sendall(password + b"\n")
                until(repaired, b"Do you wish to enter Wizinvis")
                repaired.sendall(b"n\n"); until(repaired, b"Welcome to Godwars")
                repaired.sendall(b"level\n"); until(repaired, b"Generation:")
                control.sendall(b"level\n"); until(control, b"Generation:")
                process.send_signal(signal.SIGTERM)
                assert process.wait(timeout=15) == 0
            finally:
                for client in clients:
                    client.close()
                if process.poll() is None:
                    process.kill()
                    process.wait()
        text = (runtime / "server.log").read_text()
        assert "Normal termination of game." in text
        assert "AddressSanitizer" not in text and "runtime error:" not in text
    print("Isolated " + ("string" if string_failures else
                         "word" if word_failures else "numeric") +
          " short/full-login rejection, unchanged files, "
          "unrelated online player, repaired login and SIGTERM passed")


if __name__ == "__main__":
    run()
