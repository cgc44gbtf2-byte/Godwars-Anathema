"""Actual TCP note post failure/retry, in a disposable owner-controlled world."""
import pathlib
import secrets
import signal
import socket
import subprocess
import tempfile
import time
from smoke_numbers import signup, until
from world_fixture import stage_world
REPO=pathlib.Path(__file__).resolve().parents[1]

def run():
    with tempfile.TemporaryDirectory(prefix="anathema-writer-wire-") as directory:
        runtime=pathlib.Path(directory)
        for name in ("area","player","backup","notes","log","clans"):
            (runtime/name).mkdir(mode=0o700)
        stage_world(REPO,runtime)
        general=runtime/"notes/General"
        general.write_bytes(b"")
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1",0));port=reservation.getsockname()[1]
        with (runtime/"server.log").open("w") as log:
            process=subprocess.Popen([str(REPO/"Genesis/src/merc"),str(port)],
                cwd=runtime/"area",stdout=log,stderr=log)
            client=None
            try:
                deadline=time.monotonic()+25
                while True:
                    assert process.poll() is None,"Writer smoke boot failed"
                    try:
                        client=socket.create_connection(("127.0.0.1",port),timeout=1)
                        break
                    except OSError:
                        assert time.monotonic()<deadline,"Writer smoke boot timed out"
                        time.sleep(0.1)
                until(client,b"Anathema")
                signup(client,"Luto",secrets.token_urlsafe(15).encode())
                client.sendall(b"authorize\n");until(client,b"highest staff level (7)")
                client.sendall(b"board general\n");until(client,b"Current board changed")
                # Refused by the trusted-directory check, including when root
                # runs the test. Never touch a real server or change its modes.
                (runtime/"notes").chmod(0o770)
                before=general.read_bytes()
                client.sendall(b"note write\nall\nwriter subject\n1\nfixed body\nEND\np\n")
                output=until(client,b"draft has been kept")
                assert b"Note posted" not in output
                assert general.read_bytes()==before
                (runtime/"notes").chmod(0o700)
                client.sendall(b"p\n");until(client,b"Note posted")
                data=general.read_bytes()
                assert data.count(b"Subject writer subject~")==1
                assert data.count(b"fixed body")==1
                assert not list((runtime/"notes").glob(".save-*"))
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=25)
                assert process.returncode==0
                assert general.read_bytes()==data
            except Exception:
                log.flush()
                print((runtime/"server.log").read_text(errors="replace")[-4500:])
                raise
            finally:
                (runtime/"notes").chmod(0o700)
                if client:client.close()
                if process.poll() is None:
                    process.kill();process.wait(timeout=5)
    print("Auxiliary TCP writer refusal, retained draft, single retry and TERM passed.")
if __name__=="__main__":run()
