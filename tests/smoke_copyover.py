"""Actual exec/recovery with one synthetic staff player and private world data."""
import pathlib
import signal
import socket
import subprocess
import sys
import tempfile
import time
from world_fixture import stage_world
from gmcp_probe import GMCPProbe

repo = pathlib.Path(__file__).resolve().parents[1]


def until(client, needle, timeout=25, trace=None, ready=None):
    deadline = time.monotonic() + timeout
    received = b""
    client.settimeout(0.5)
    while (not received or
           (needle is not None and needle.lower() not in received.lower()) or
           (ready is not None and not ready())):
        if time.monotonic() > deadline:
            raise AssertionError("Expected server response did not arrive")
        try:
            part = client.recv(65536)
        except socket.timeout:
            continue
        assert part, "Connection closed before expected response"
        if trace is not None:
            trace(part)
        received += part
        assert len(received) < 2_000_000, "Unexpected unbounded server output"
    return received


with tempfile.TemporaryDirectory(prefix="anathema-copyover-") as directory:
    runtime = pathlib.Path(directory)
    for name in ("area", "player", "backup", "notes", "log", "clans"):
        (runtime / name).mkdir(mode=0o700)
    (runtime / "src").symlink_to(repo / "Genesis/src", target_is_directory=True)
    stage_world(repo, runtime)
    subprocess.run([str(repo / "tests/test_restart"), "--seed",
                    str(runtime / "area")], check=True)
    original_player = (runtime / "player/Restarter").read_bytes()
    hybrid_line = next(line for line in original_player.splitlines()
                       if line.startswith(b"AnathemaHybrid "))
    assert hybrid_line.startswith(b"AnathemaHybrid 4 ")
    rejected_player = b"#PLAYER\nName Rejected~\nAnathemaHybrid 99 1\nEnd\n#END\n"
    (runtime / "player/Rejected").write_bytes(rejected_player)
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    with (runtime / "server.log").open("w") as log:
        process = subprocess.Popen([str(repo / "Genesis/src/merc"), str(port)],
                                   cwd=runtime / "area", stdout=log, stderr=log)
        staff = pending = newcomer = None
        try:
            deadline = time.monotonic() + 25
            while True:
                assert process.poll() is None, "Server exited before login"
                try:
                    staff = socket.create_connection(("127.0.0.1", port), timeout=1)
                    break
                except OSError:
                    if time.monotonic() > deadline:
                        raise AssertionError("Server did not start")
                    time.sleep(0.1)
            gmcp = GMCPProbe()
            until(staff, b"\xff\xfb\xc9", trace=gmcp.feed)
            assert gmcp.offers == 1 and not gmcp.records
            staff.sendall(b"\xff\xfd\xc9\xff\xfa\xc9Core.Hello "
                          b'{"client":"synthetic-test","version":1}\xff\xf0')
            staff.sendall(b"Restarter\n")
            until(staff, b"Password", trace=gmcp.feed)
            assert not gmcp.records, "Vitals leaked before authentication"
            staff.sendall(b"test-only-passphrase\n")
            until(staff, b"Wizinvis", trace=gmcp.feed)
            assert not gmcp.records, "Vitals leaked before entering the game"
            staff.sendall(b"n\n")
            until(staff, b"Welcome to Godwars", trace=gmcp.feed)
            staff.sendall(b"save\n")
            until(staff, b"Saved.", trace=gmcp.feed,
                  ready=lambda: {"Char.Vitals", "Anathema.Vitals"} <= {
                      p for p, _ in gmcp.records})
            assert {"Char.Vitals", "Anathema.Vitals"} <= {p for p, _ in gmcp.records}
            vitals = next(value for package, value in gmcp.records if package == "Char.Vitals")
            assert set(vitals) == {"hp", "maxhp", "mp", "maxmp"}
            assert all(isinstance(value, int) for value in vitals.values())
            assert not any("Restarter" in str(value) for _, value in gmcp.records)
            pending = socket.create_connection(("127.0.0.1", port), timeout=3)
            until(pending, None)

            # Both manual restart paths must refuse failed authoritative writes.
            (runtime / "player").rename(runtime / "unavailable")
            staff.sendall(b"shutdown\n")
            until(staff, b"Shutdown cancelled", trace=gmcp.feed)
            assert process.poll() is None and not (runtime / "area/shutdown.txt").exists()
            staff.sendall(b"copyover\n")
            until(staff, b"Copyover cancelled", trace=gmcp.feed)
            assert gmcp.resets == 0 and gmcp.offers == 1
            assert process.poll() is None and not (runtime / "area/copyover.data").exists()
            (runtime / "unavailable").rename(runtime / "player")

            # Production restart.c, real execl, original PID and active TCP socket.
            pid = process.pid
            # A partial client subnegotiation must remain out of command input
            # across execl; its remaining payload includes a misleading command.
            staff.sendall(b"copyover\n\xff\xfa\x18Mud")
            until(staff, b"Copyover recovery complete.", trace=gmcp.feed)
            assert gmcp.resets == 1 and gmcp.offers == 2
            before_recovery = len(gmcp.records)
            assert process.poll() is None and process.pid == pid
            assert not (runtime / "area/copyover.data").exists()
            # Finish the old subnegotiation before acknowledging the reset;
            # interleaving option acknowledgements inside SB would be invalid.
            staff.sendall(b"Forge\nnot-a-command\xff\xf0"
                          b"\xff\xfe\xc9\xff\xfd\xc9save\n")
            until(staff, b"Saved.", trace=gmcp.feed,
                  ready=lambda: {"Char.Vitals", "Anathema.Vitals"} <= {
                      p for p, _ in gmcp.records[before_recovery:]})
            assert {"Char.Vitals", "Anathema.Vitals"} <= {
                p for p, _ in gmcp.records[before_recovery:]}
            pending.settimeout(5)
            try:
                while pending.recv(65536):
                    pass
            except ConnectionResetError:
                pass
            newcomer = socket.create_connection(("127.0.0.1", port), timeout=3)
            until(newcomer, None)
            newcomer.sendall(b"Rejected\n")
            until(newcomer, b"Player data could not be loaded safely")
            assert (runtime / "player/Rejected").read_bytes() == rejected_player
            updated_player = (runtime / "player/Restarter").read_bytes()
            assert hybrid_line in updated_player.splitlines(), "Copyover lost hybrid storage"
            staff.sendall(b"shutdown\n")
            assert process.wait(timeout=15) == 0, "Checked shutdown did not exit cleanly"
        except Exception:
            log.flush()
            diagnostic = (runtime / "server.log").read_text(errors="replace")
            print("Synthetic copyover server log (last 120 lines):", file=sys.stderr)
            print("\n".join(diagnostic.splitlines()[-120:]), file=sys.stderr)
            raise
        finally:
            for client in (staff, pending, newcomer):
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
    assert text.count("ready to rock on port") == 2, "Actual re-exec was not observed"
    assert "Copyover recovery complete." in text and "Normal termination of game." in text
    assert not list((runtime / "player").glob(".*")), "Temporary player saves leaked"
print("Isolated save-failure refusal, GMCP privacy/reset, real exec/TCP recovery and checked shutdown passed")
