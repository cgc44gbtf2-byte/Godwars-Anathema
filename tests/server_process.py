"""Start a disposable smoke server without connecting before it owns its port.

Only an exited pre-readiness process with the exact EADDRINUSE diagnostic
may be retried. World/engine failures, timeouts and post-readiness failures
must fail the smoke test instead of restarting it.
"""
import pathlib
import socket
import subprocess
import time

MAX_BIND_ATTEMPTS = 3
BIND_CONFLICT = b"Init_socket: bind: Address already in use"


def _unused_port():
    # Match the engine's INADDR_ANY bind, not just the loopback interface.
    with socket.socket() as reservation:
        reservation.bind(("0.0.0.0", 0))
        return reservation.getsockname()[1]


def start_server(binary, cwd, log, timeout=25):
    """Return (process, port) only after this child reports startup readiness."""
    deadline = time.monotonic() + timeout
    path = pathlib.Path(log.name)
    for attempt in range(MAX_BIND_ATTEMPTS):
        if time.monotonic() >= deadline:
            raise AssertionError("Smoke server startup deadline expired")
        port = _unused_port()
        log.flush()
        offset = path.stat().st_size
        process = subprocess.Popen(
            [str(binary), str(port)], cwd=cwd, stdout=log, stderr=log)
        ready = f"Godwars: Anathema is ready to rock on port {port}.".encode()
        try:
            while True:
                log.flush()
                output = path.read_bytes()[offset:]
                status = process.poll()
                if ready in output:
                    assert status is None, "Smoke server exited after reporting readiness"
                    return process, port
                if status is not None:
                    # Never retry a general boot error or a failure after the
                    # test has connected/logged in. No client exists yet here.
                    conflict = output.strip() == BIND_CONFLICT
                    if status == 1 and conflict and attempt + 1 < MAX_BIND_ATTEMPTS:
                        break
                    raise AssertionError(
                        f"Smoke server exited before readiness (status {status}):\n"
                        + output[-8192:].decode(errors="replace"))
                if time.monotonic() >= deadline:
                    raise AssertionError("Smoke server startup deadline expired")
                time.sleep(0.02)
        except BaseException:
            if process.poll() is None:
                process.kill()
            process.wait(timeout=10)
            raise
        process.wait(timeout=10)
    raise AssertionError("Smoke server could not claim a test port")
