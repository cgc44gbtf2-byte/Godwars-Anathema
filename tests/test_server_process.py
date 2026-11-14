"""Deterministic process-level coverage for smoke startup, not gameplay retries."""
import pathlib
import socket
import subprocess
import tempfile
import unittest
from unittest import mock
import server_process as server


class ServerProcessTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="anathema-startup-unit-")
        self.addCleanup(self.directory.cleanup)
        self.root = pathlib.Path(self.directory.name)
        self.log = (self.root / "server.log").open("w")
        self.addCleanup(self.log.close)
        self.binary = self.root / "fake-server"

    def program(self, body):
        self.binary.write_text("#!/usr/bin/env python3\n" + body)
        self.binary.chmod(0o700)

    def start(self, **kwargs):
        process, port = server.start_server(self.binary, self.root, self.log, **kwargs)
        self.addCleanup(self.stop, process)
        return process, port

    @staticmethod
    def stop(process):
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)

    def listener(self):
        self.program(
            "import errno,socket,sys,time\n"
            "port=int(sys.argv[1]); listener=socket.socket()\n"
            "try: listener.bind(('0.0.0.0',port))\n"
            "except OSError as error:\n"
            " if error.errno!=errno.EADDRINUSE: raise\n"
            " print('Init_socket: bind: Address already in use',flush=True)\n"
            " sys.exit(1)\n"
            "listener.listen()\n"
            "print(f'Godwars: Anathema is ready to rock on port {port}.',flush=True)\n"
            "time.sleep(60)\n")

    def test_real_bind_conflict_uses_another_port_before_any_connection(self):
        self.listener()
        with socket.socket() as occupied:
            occupied.bind(("0.0.0.0", 0))
            occupied.listen()
            busy = occupied.getsockname()[1]
            choose = server._unused_port
            attempts = []

            def select():
                port = busy if not attempts else choose()
                attempts.append(port)
                return port

            with mock.patch.object(server, "_unused_port", side_effect=select), \
                 mock.patch.object(server.socket, "create_connection",
                                   side_effect=AssertionError("Connected before readiness")):
                process, port = self.start()
            self.assertEqual(len(attempts), 2)
            self.assertNotEqual(port, busy)
            self.assertIsNone(process.poll())
            self.assertEqual((self.root / "server.log").read_text().count(
                "ready to rock on port"), 1)

    def test_probe_matches_engine_wildcard_bind(self):
        with mock.patch.object(server.socket, "socket") as factory:
            reservation = factory.return_value.__enter__.return_value
            reservation.getsockname.return_value = ("0.0.0.0", 54321)
            self.assertEqual(server._unused_port(), 54321)
            reservation.bind.assert_called_once_with(("0.0.0.0", 0))

    def test_ordinary_boot_error_is_not_retried(self):
        self.program("print('Invalid administrative configuration',flush=True)\n"
                     "raise SystemExit(1)\n")
        with mock.patch.object(server.subprocess, "Popen", wraps=subprocess.Popen) as launch:
            with self.assertRaisesRegex(AssertionError, "Invalid administrative"):
                self.start()
            self.assertEqual(launch.call_count, 1)

    def test_bind_retries_are_bounded(self):
        self.program("print('Init_socket: bind: Address already in use',flush=True)\n"
                     "raise SystemExit(1)\n")
        with mock.patch.object(server.subprocess, "Popen", wraps=subprocess.Popen) as launch:
            with self.assertRaisesRegex(AssertionError, "Address already in use"):
                self.start()
            self.assertEqual(launch.call_count, server.MAX_BIND_ATTEMPTS)

    def test_other_bind_errors_are_not_retried(self):
        self.program("print('Init_socket: bind: Permission denied',flush=True)\n"
                     "raise SystemExit(1)\n")
        with mock.patch.object(server.subprocess, "Popen", wraps=subprocess.Popen) as launch:
            with self.assertRaisesRegex(AssertionError, "Permission denied"):
                self.start()
            self.assertEqual(launch.call_count, 1)

    def test_conflict_text_with_wrong_exit_status_is_not_retried(self):
        self.program("print('Init_socket: bind: Address already in use',flush=True)\n"
                     "raise SystemExit(2)\n")
        with mock.patch.object(server.subprocess, "Popen", wraps=subprocess.Popen) as launch:
            with self.assertRaisesRegex(AssertionError, "status 2"):
                self.start()
            self.assertEqual(launch.call_count, 1)

    def test_old_log_conflict_does_not_reclassify_a_new_boot_error(self):
        self.log.write("Init_socket: bind: Address already in use\n")
        self.log.flush()
        self.program("print('Actual new boot error',flush=True)\nraise SystemExit(1)\n")
        with mock.patch.object(server.subprocess, "Popen", wraps=subprocess.Popen) as launch:
            with self.assertRaisesRegex(AssertionError, "Actual new boot error"):
                self.start()
            self.assertEqual(launch.call_count, 1)

    def test_timeout_kills_and_reaps_child_without_retry(self):
        process = mock.Mock()
        process.poll.return_value = None
        with mock.patch.object(server.subprocess, "Popen", return_value=process) as launch, \
             mock.patch.object(server.time, "monotonic", side_effect=[0, 0, 2]):
            with self.assertRaisesRegex(AssertionError, "deadline"):
                self.start(timeout=1)
            self.assertEqual(launch.call_count, 1)
        process.kill.assert_called_once_with()
        process.wait.assert_called_once_with(timeout=10)

    def test_reported_readiness_then_exit_is_not_retried(self):
        process = mock.Mock()
        process.poll.return_value = 1

        def exited(command, **kwargs):
            self.log.write("Init_socket: bind: Address already in use\n"
                           f"Godwars: Anathema is ready to rock on port {command[1]}.\n")
            self.log.flush()
            return process

        with mock.patch.object(server.subprocess, "Popen", side_effect=exited) as launch:
            with self.assertRaisesRegex(AssertionError, "after reporting readiness"):
                self.start()
            self.assertEqual(launch.call_count, 1)

    def test_conflict_alongside_another_error_is_not_retried(self):
        self.program("print('Init_socket: bind: Address already in use',flush=True)\n"
                     "print('Additional startup failure',flush=True)\n"
                     "raise SystemExit(1)\n")
        with mock.patch.object(server.subprocess, "Popen", wraps=subprocess.Popen) as launch:
            with self.assertRaisesRegex(AssertionError, "Additional startup failure"):
                self.start()
            self.assertEqual(launch.call_count, 1)

    def test_startup_spawning_error_is_not_retried(self):
        with mock.patch.object(server.subprocess, "Popen",
                               side_effect=OSError("fixture spawn failure")) as launch:
            with self.assertRaisesRegex(OSError, "fixture spawn"):
                self.start()
            self.assertEqual(launch.call_count, 1)


if __name__ == "__main__":
    unittest.main()
