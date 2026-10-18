"""Real TCP signup/authorization in an isolated world, never a live host."""
import pathlib
import re
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
        if time.monotonic() > deadline:
            raise AssertionError("Expected signup/authorization response did not arrive")
        try:
            part = client.recv(65536)
        except socket.timeout:
            continue
        assert part, "Connection closed during signup/authorization"
        data += part
        assert len(data) < 2_000_000
    return data


def signup(client, name):
    client.sendall(name.encode() + b"\n")
    until(client, b"tombstone")
    client.sendall(b"y\n"); until(client, b"Give me a password")
    client.sendall(b"fixture-only-passphrase\n"); until(client, b"retype password")
    client.sendall(b"fixture-only-passphrase\n"); until(client, b"your sex")
    client.sendall(b"m\n"); until(client, b"support ANSI")
    # The legacy line reader coalesces adjacent line endings. Wait for MOTD
    # before sending its Enter, rather than pipelining a swallowed blank line.
    client.sendall(b"n\n"); until(client, b"Anathema modifications")
    client.sendall(b"\n"); until(client, b"Welcome to Godwars")


with tempfile.TemporaryDirectory(prefix="anathema-owner-wire-") as directory:
    runtime = pathlib.Path(directory)
    for name in ("area", "player", "backup", "notes", "log", "clans"):
        (runtime / name).mkdir(mode=0o700)
    (runtime / "src").symlink_to(REPO / "Genesis/src", target_is_directory=True)
    stage_world(REPO, runtime)
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    with (runtime / "server.log").open("w") as log:
        process = subprocess.Popen([str(REPO / "Genesis/src/merc"), str(port)],
                                   cwd=runtime / "area", stdout=log, stderr=log)
        owner = other = None
        try:
            deadline = time.monotonic() + 25
            while True:
                assert process.poll() is None, "Server exited before signup"
                try:
                    owner = socket.create_connection(("127.0.0.1", port), timeout=1)
                    break
                except OSError:
                    if time.monotonic() > deadline:
                        raise AssertionError("Server did not start")
                    time.sleep(0.1)
            signup(owner, "Luto")
            owner.sendall(b"authorize\n")
            until(owner, b"highest staff level (7)")
            for name in ("player/Luto", "backup/Luto"):
                data = (runtime / name).read_bytes()
                assert re.search(rb"(?m)^Level\s+7\s*$", data)
                # This engine derives trust from Level; no extra save field.
                assert not re.search(rb"(?m)^Trust\s+", data)
            owner.sendall(b"authorize\n")
            until(owner, b"already have the highest staff level")
            other = socket.create_connection(("127.0.0.1", port), timeout=1)
            signup(other, "Yami")
            other.sendall(b"authorize\n")
            until(other, b"only available to Luto")
            assert not (runtime / "player/Yami").exists()
            process.send_signal(signal.SIGTERM)
            assert process.wait(timeout=25) == 0
        except BaseException:
            log.flush()
            print("\n".join((runtime / "server.log").read_text(errors="replace").splitlines()[-45:]))
            raise
        finally:
            for client in (owner, other):
                if client:
                    client.close()
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=25)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait(timeout=5)
print("Isolated TCP signup, Luto highest-rank authorization, durable saves and Yami rejection passed")
