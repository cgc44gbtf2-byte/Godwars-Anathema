"""Corrupt auxiliary files must not prevent boot/login or be overwritten."""
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
    with tempfile.TemporaryDirectory(prefix="anathema-aux-wire-") as directory:
        runtime=pathlib.Path(directory)
        for name in ("area","player","backup","notes","log","clans"):
            (runtime/name).mkdir(mode=0o700)
        stage_world(REPO,runtime)
        damaged={
            runtime/"notes/General":
                b"Sender old~\nDate old~\nStamp 1\nExpire 1\nTo all~\n"
                b"Subject expired~\nText expired~\nSender broken~\nWrong key\n",
            runtime/"area/leader.txt":b"Unfinished~\n9999999999999999999999999999\n",
        }
        for path,data in damaged.items():path.write_bytes(data)
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1",0));port=reservation.getsockname()[1]
        with (runtime/"server.log").open("w") as log:
            process=subprocess.Popen([str(REPO/"Genesis/src/merc"),str(port)],
                cwd=runtime/"area",stdout=log,stderr=log)
            client=None
            try:
                deadline=time.monotonic()+25
                while True:
                    assert process.poll() is None,"Auxiliary-file boot failed"
                    try:
                        client=socket.create_connection(("127.0.0.1",port),timeout=1)
                        break
                    except OSError:
                        assert time.monotonic()<deadline,"Auxiliary boot timed out"
                        time.sleep(0.1)
                until(client,b"Anathema")
                # Fresh sleepers cannot write General at all. Use the existing
                # isolated owner bootstrap to test the *corrupt-file* guard,
                # rather than accidentally testing the normal level gate.
                signup(client,"Luto",secrets.token_urlsafe(15).encode())
                client.sendall(b"authorize\n");until(client,b"highest staff level (7)")
                client.sendall(b"board general\n");until(client,b"Current board changed")
                client.sendall(b"note write\n");until(client,b"read-only")
                client.sendall(b"leaderboard\n");until(client,b"LEADER BOARD")
                client.sendall(b"score\n");until(client,b"Character Created:")
                assert process.poll() is None
                process.send_signal(signal.SIGTERM)
                assert process.wait(timeout=15)==0
            finally:
                if client is not None:client.close()
                if process.poll() is None:process.kill();process.wait()
        for path,data in damaged.items():assert path.read_bytes()==data
        text=(runtime/"server.log").read_text()
        assert "load_board: bad/incomplete board" in text
        assert "load_leaderboard: rejected file" in text
        assert "Normal termination of game." in text
        assert "AddressSanitizer" not in text and "runtime error:" not in text
    print("Isolated corrupt notes/leaderboard boot, signup, read-only post, score and SIGTERM passed; files unchanged")

if __name__=="__main__":run()
