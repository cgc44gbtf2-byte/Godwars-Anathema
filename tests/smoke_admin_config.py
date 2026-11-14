"""Disposable-world boot policy and real staff commands; no live server."""
import pathlib
import secrets
import signal
import socket
import stat
import subprocess
import tempfile
import time
from smoke_numbers import signup, until
from world_fixture import stage_world
REPO=pathlib.Path(__file__).resolve().parents[1]

def scenario(kind):
    with tempfile.TemporaryDirectory(prefix="anathema-policy-wire-") as directory:
        runtime=pathlib.Path(directory)
        for name in ("area","player","backup","notes","log","clans"):
            (runtime/name).mkdir(mode=0o700)
        stage_world(REPO,runtime)
        area=runtime/"area"
        (area/"ban.txt").write_text("''\n" if kind=="bad-ban" else "")
        (area/"disabled.txt").write_text("help 7" if kind=="bad-disabled" else "chat 7 Luto\nEND\n")
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1",0));port=reservation.getsockname()[1]
        with (runtime/"server.log").open("w") as log:
            process=subprocess.Popen([str(REPO/"Genesis/src/merc"),str(port)],
                cwd=area,stdout=log,stderr=log)
            client=None
            try:
                if kind!="valid":
                    process.wait(timeout=25)
                    assert process.returncode!=0
                    log.flush()
                    assert "Invalid administrative configuration" in (runtime/"server.log").read_text()
                    return
                deadline=time.monotonic()+25
                while True:
                    assert process.poll() is None,"Policy smoke boot failed"
                    try:
                        client=socket.create_connection(("127.0.0.1",port),timeout=1)
                        break
                    except OSError:
                        assert time.monotonic()<deadline,"Policy smoke boot timed out"
                        time.sleep(0.1)
                until(client,b"Anathema")
                signup(client,"Luto",secrets.token_urlsafe(15).encode())
                client.sendall(b"authorize\n");until(client,b"highest staff level (7)")
                client.sendall(b"disable chat\n");until(client,b"Command enabled.")
                assert not (area/"disabled.txt").exists()
                client.sendall(b"disable chat\n");until(client,b"Command disabled.")
                assert (area/"disabled.txt").read_text()=="chat 7 Luto\nEND\n"
                assert stat.S_IMODE((area/"disabled.txt").stat().st_mode)==0o600
                client.sendall(b"disable chat\n");until(client,b"Command enabled.")
                assert not (area/"disabled.txt").exists()
                client.sendall(b"ban blocked.invalid\n");until(client,b"Ok.")
                assert (area/"ban.txt").read_text()=="blocked.invalid\n"
                assert stat.S_IMODE((area/"ban.txt").stat().st_mode)==0o600
                client.sendall(b"allow blocked.invalid\n");until(client,b"Ok.")
                assert not (area/"ban.txt").exists()
                process.send_signal(signal.SIGTERM);process.wait(timeout=25)
                assert process.returncode==0
            except Exception:
                log.flush()
                print((runtime/"server.log").read_text(errors="replace")[-4500:])
                raise
            finally:
                if client:client.close()
                if process.poll() is None:process.kill();process.wait(timeout=5)

def run():
    for kind in ("valid","bad-disabled","bad-ban"):scenario(kind)
    print("Administrative policy TCP signup, staff commands, rejected boot and TERM passed.")
if __name__=="__main__":run()
