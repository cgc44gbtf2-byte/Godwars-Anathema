"""Synthetic monitoring faults; never contacts Discord or controls a live MUD."""
import importlib.util
import http.client
import io
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock
import urllib.error

REPO = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("monitor", REPO / "deploy/monitor.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
URL = "https://discord.com/api/webhooks/123456789012345678/" + "x" * 68
SETTINGS = dict(webhook=URL, disk_percent=90, min_free_bytes=1024**3)
HEALTHY = "ActiveState=active\nSubState=running\nResult=success\nMainPID=123\nNRestarts=0\n"


class Events(unittest.TestCase):
    def test_initial_healthy_is_quiet(self):
        self.assertEqual(m.transition({}, {}, 0, 1)["pending"], [])

    def test_failure_then_recovery(self):
        state = m.transition({}, {"service": "offline"}, 0, 1)
        self.assertIn("ALERT:", state["pending"][0])
        state["pending"] = []
        self.assertIn("RECOVERED:", m.transition(state, {}, 0, 2)["pending"][0])

    def test_repeated_problem_is_quiet(self):
        old = dict(problems={"service": "offline"}, pending=[], last_sent=1, restarts=0)
        self.assertEqual(m.transition(old, {"service": "offline"}, 0, 120)["pending"], [])

    def test_reminder_after_six_hours(self):
        old = dict(problems={"service": "offline"}, pending=[], last_sent=1, restarts=0)
        self.assertTrue(m.transition(old, {"service": "offline"}, 0, 21602)["pending"])

    def test_restart_between_polls_is_recorded(self):
        state = m.transition(dict(restarts=2), {}, 4, 1)
        self.assertIn("4 automatic restart", state["pending"][0])

    def test_manual_counter_reset_is_not_crash(self):
        self.assertEqual(m.transition(dict(restarts=5), {}, 0, 1)["pending"], [])

    def test_maintenance_preserves_service_baseline(self):
        old = dict(problems={"service": "offline"}, restarts=5)
        state = m.transition(old, {}, None, 1, skip_service=True)
        self.assertEqual(state["problems"], old["problems"])
        self.assertEqual(state["restarts"], 5)
        self.assertFalse(state["pending"])

    def test_disk_values_changing_dont_spam(self):
        old = dict(problems={"disk:1": "low"}, pending=[], last_sent=1)
        self.assertFalse(m.transition(old, {"disk:1": "still low"}, None, 2)["pending"])

    def test_disk_recovery(self):
        state = m.transition(dict(problems={"disk:1": "low"}), {}, None, 1)
        self.assertIn("RECOVERED:", state["pending"][0])

    def test_pending_survives_recovery(self):
        state = m.transition(dict(problems={"service": "offline"}, pending=["old alert"]), {}, 0, 2)
        self.assertEqual(state["pending"][0], "old alert")
        self.assertEqual(len(state["pending"]), 2)

    def test_queue_is_bounded(self):
        state = m.transition(dict(pending=["x"] * 100), {"service": "offline"}, 2, 1)
        self.assertEqual(len(state["pending"]), 50)
        self.assertIn("coalesced", state["pending"][0])


class Delivery(unittest.TestCase):
    def test_success_acknowledges_only_sent_events(self):
        state = dict(pending=["a" * 800, "b" * 800, "c" * 800])
        with mock.patch.object(m, "send") as send:
            m.deliver(state, SETTINGS, 100)
        self.assertEqual(state["pending"], ["c" * 800])
        self.assertLessEqual(len(send.call_args.args[1]), 1900)

    def test_failure_retains_events_with_backoff(self):
        state = dict(pending=["offline"])
        with mock.patch.object(m, "send", side_effect=m.MonitorError("unavailable")):
            m.deliver(state, SETTINGS, 100)
        self.assertEqual(state["pending"], ["offline"])
        self.assertEqual(state["next_attempt"], 160)

    def test_backoff_prevents_retry(self):
        with mock.patch.object(m, "send") as send:
            m.deliver(dict(pending=["a"], next_attempt=100), SETTINGS, 99)
        send.assert_not_called()

    def test_max_backoff(self):
        state = dict(pending=["a"], failures=10)
        with mock.patch.object(m, "send", side_effect=m.MonitorError("fail")):
            m.deliver(state, SETTINGS, 1)
        self.assertEqual(state["next_attempt"], 3601)

    def test_mentions_disabled_and_request_bounded(self):
        response = mock.MagicMock()
        response.__enter__.return_value.status = 204
        opener = mock.Mock()
        opener.open.return_value = response
        with mock.patch.object(m.urllib.request, "build_opener", return_value=opener):
            m.send(URL, "@everyone test")
        request = opener.open.call_args.args[0]
        self.assertEqual(json.loads(request.data)["allowed_mentions"], {"parse": []})
        self.assertEqual(opener.open.call_args.kwargs["timeout"], 10)

    def test_errors_do_not_expose_secret_url(self):
        opener = mock.Mock()
        opener.open.side_effect = urllib.error.HTTPError(URL, 429, URL, {}, None)
        with mock.patch.object(m.urllib.request, "build_opener", return_value=opener):
            with self.assertRaises(m.MonitorError) as raised:
                m.send(URL, "test")
        self.assertNotIn(URL, str(raised.exception))
        self.assertIn("429", str(raised.exception))

    def test_malformed_http_response_cannot_leak_url(self):
        opener = mock.Mock()
        opener.open.side_effect = http.client.BadStatusLine(URL)
        with mock.patch.object(m.urllib.request, "build_opener", return_value=opener):
            with self.assertRaises(m.MonitorError) as raised:
                m.send(URL, "test")
        self.assertNotIn(URL, str(raised.exception))

    def test_rejects_unsafe_urls(self):
        for url in (URL.replace("https:", "http:"), URL + "?x=1", URL + "#x",
                    URL.replace("discord.com", "discord.com.evil.test"),
                    URL.replace("discord.com", "user@discord.com"),
                    URL.replace("discord.com", "discord.com:444")):
            with self.subTest(kind=url.split("/")[0]), self.assertRaises(m.MonitorError):
                m.send(url, "never sent")

    def test_no_redirects(self):
        with self.assertRaises(m.MonitorError):
            m.NoRedirect().redirect_request(None, None, 302, "", {}, "https://example.com")


class Sampling(unittest.TestCase):
    def sample(self, **overrides):
        fs = SimpleNamespace(f_bavail=80, f_frsize=1024**3, f_blocks=100,
                             f_files=100, f_favail=80)
        for k, v in overrides.items():
            setattr(fs, k, v)
        with mock.patch.object(m, "command", return_value=HEALTHY), \
             mock.patch.object(m.os, "stat", return_value=SimpleNamespace(st_dev=1)), \
             mock.patch.object(m.os, "statvfs", return_value=fs):
            return m.sample(SETTINGS)

    def test_same_filesystem_is_coalesced(self):
        self.assertEqual(len(self.sample(f_bavail=1)[0]), 1)

    def test_healthy_disks(self):
        self.assertEqual(self.sample(), ({}, 0))

    def test_inode_exhaustion(self):
        self.assertTrue(self.sample(f_favail=0)[0])

    def test_minimum_free_bytes(self):
        self.assertTrue(self.sample(f_bavail=1, f_frsize=512)[0])

    def test_unreadable_service_is_problem(self):
        with mock.patch.object(m, "command", side_effect=OSError()), \
             mock.patch.object(m, "PATHS", ()):
            self.assertIn("service", m.sample(SETTINGS)[0])

    def test_maintenance_does_not_query_service(self):
        with mock.patch.object(m, "command") as command, mock.patch.object(m, "PATHS", ()):
            self.assertEqual(m.sample(SETTINGS, True), ({}, None))
        command.assert_not_called()


class FilesAndUnits(unittest.TestCase):
    def test_install_only_enables_separate_timer(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            app, units = root / "deploy", root / "units"
            app.mkdir(); units.mkdir()
            with mock.patch.object(m, "APP", app), mock.patch.object(m, "UNITS", units), \
                 mock.patch.object(m, "config", return_value=SETTINGS), \
                 mock.patch.object(m, "protected"), mock.patch.object(m, "command") as command:
                m.install()
            self.assertEqual(command.call_args_list,
                [mock.call("systemctl", "daemon-reload"),
                 mock.call("systemctl", "enable", "--now", "anathema-monitor.timer")])
            self.assertTrue((app / "monitor.py").is_file())
            self.assertTrue((units / "anathema-monitor.timer").is_file())

    def test_configure_hides_url_and_writes_private_file(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / "config"
            output = io.StringIO()
            with mock.patch.object(m, "CONFIG", file), mock.patch.object(m, "private_directory"), \
                 mock.patch.object(m.os, "geteuid", return_value=0), \
                 mock.patch.object(m.os, "isatty", return_value=True), \
                 mock.patch.object(m.getpass, "getpass", return_value=URL), \
                 mock.patch("sys.stdout", output):
                m.main(["--configure"])
            self.assertEqual(json.loads(file.read_bytes())["webhook"], URL)
            self.assertEqual(stat.S_IMODE(file.stat().st_mode), 0o600)
            self.assertNotIn(URL, output.getvalue())

    def test_noninteractive_secret_input_refused(self):
        with mock.patch.object(m.os, "geteuid", return_value=0), \
             mock.patch.object(m.os, "isatty", return_value=False), \
             mock.patch.object(m.getpass, "getpass") as prompt, self.assertRaises(m.MonitorError):
            m.main(["--configure"])
        prompt.assert_not_called()

    def test_echo_fallback_is_refused(self):
        with mock.patch.object(m.os, "geteuid", return_value=0), \
             mock.patch.object(m.os, "isatty", return_value=True), \
             mock.patch.object(m, "private_directory"), \
             mock.patch.object(m.getpass, "getpass", side_effect=m.getpass.GetPassWarning()):
            with self.assertRaises(m.MonitorError):
                m.main(["--configure"])

    def test_queued_delivery_persists_across_checks(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            file = root / "state"
            with mock.patch.object(m, "STATE_DIR", root), mock.patch.object(m, "STATE", file), \
                 mock.patch.object(m, "private_directory"), \
                 mock.patch.object(m, "read_json", side_effect=lambda p: json.loads(p.read_bytes())), \
                 mock.patch.object(m, "sample", return_value=({"service": "offline"}, 1)), \
                 mock.patch.object(m, "send", side_effect=[m.MonitorError("offline"), None]) as send, \
                 mock.patch.object(m.time, "time", side_effect=[100, 150, 170]):
                m.check(SETTINGS)
                self.assertTrue(json.loads(file.read_bytes())["pending"])
                m.check(SETTINGS)
                self.assertEqual(send.call_count, 1)
                m.check(SETTINGS)
            self.assertEqual(send.call_count, 2)
            saved = json.loads(file.read_bytes())
            self.assertFalse(saved["pending"])
            self.assertNotIn("webhook", saved)

    def test_event_is_persisted_before_delivery(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            file = root / "state"
            def delivery(*args):
                self.assertTrue(json.loads(file.read_bytes())["pending"])
            with mock.patch.object(m, "STATE_DIR", root), mock.patch.object(m, "STATE", file), \
                 mock.patch.object(m, "private_directory"), \
                 mock.patch.object(m, "sample", return_value=({"service": "offline"}, 0)), \
                 mock.patch.object(m, "send", side_effect=delivery):
                m.check(SETTINGS)

    def test_full_storage_does_not_silence_disk_alert(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            with mock.patch.object(m, "STATE_DIR", root), \
                 mock.patch.object(m, "STATE", root / "state"), \
                 mock.patch.object(m, "private_directory"), \
                 mock.patch.object(m, "sample", return_value=({"disk:1": "disk full"}, 0)), \
                 mock.patch.object(m, "atomic", side_effect=OSError("full")), \
                 mock.patch.object(m, "send") as send, self.assertRaises(m.MonitorError):
                m.check(SETTINGS)
            send.assert_called_once()

    def test_atomic_private_write(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / "state"
            m.atomic(file, b"new")
            self.assertEqual(file.read_bytes(), b"new")
            self.assertEqual(stat.S_IMODE(file.stat().st_mode), 0o600)

    def test_atomic_replace_failure_preserves_old(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / "state"
            file.write_bytes(b"old")
            with mock.patch.object(m.os, "replace", side_effect=OSError()):
                with self.assertRaises(OSError):
                    m.atomic(file, b"new")
            self.assertEqual(file.read_bytes(), b"old")
            self.assertEqual(len(list(Path(folder).iterdir())), 1)

    def test_private_json_rejects_hardlinks_or_group_access(self):
        for links, mode in ((2, 0o600), (1, 0o640)):
            with tempfile.TemporaryDirectory() as folder:
                file = Path(folder) / "state"
                file.write_text("{}")
                with mock.patch.object(m, "protected"), mock.patch.object(
                    m.os, "fstat", return_value=SimpleNamespace(
                        st_mode=stat.S_IFREG | mode, st_uid=0, st_nlink=links, st_size=2)):
                    with self.assertRaises(m.MonitorError):
                        m.read_json(file)

    def test_nonroot_setup_rejected(self):
        with mock.patch.object(m.os, "geteuid", return_value=1000):
            with self.assertRaises(m.MonitorError):
                m.main(["--install"])

    def test_units_cannot_control_mud(self):
        unit = (REPO / "deploy/anathema-monitor.service").read_text()
        self.assertIn("ProtectSystem=strict", unit)
        self.assertIn("StateDirectoryMode=0700", unit)
        self.assertNotIn("ExecStop", unit)
        self.assertNotIn("Requires=anathema.service", unit)
        source = (REPO / "deploy/monitor.py").read_text()
        self.assertNotIn('"systemctl", "restart"', source)
        self.assertNotIn('"systemctl", "stop"', source)

    @unittest.skipUnless(shutil.which("systemd-analyze") and Path("/usr/bin/python3").exists(),
                         "distro systemd toolchain required")
    def test_unit_files_verify_without_starting_services(self):
        subprocess.run(["systemd-analyze", "verify",
                        str(REPO / "deploy/anathema-monitor.service"),
                        str(REPO / "deploy/anathema-monitor.timer")],
                       check=True, capture_output=True, timeout=20)


if __name__ == "__main__":
    unittest.main()
