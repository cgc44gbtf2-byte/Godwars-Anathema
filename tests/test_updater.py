"""No live network, service, system account or host-installation mutations."""
import copy
import gzip
import importlib.util
import io
import json
import os
from pathlib import Path
import pwd
import shutil
import stat
import sys
import tarfile
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("anathema_update", ROOT / "deploy/update.py")
u = importlib.util.module_from_spec(spec)
spec.loader.exec_module(u)
HEAD = "a" * 40
OLD, NEW = b"\x7fELFold-fixture", b"\x7fELFnew-fixture"


def make_archive(extra=(), compat=None, makefile=None):
    files = [
        ("root/Genesis/src/Makefile", makefile or b"all:\n\ttrue\n"),
        ("root/Genesis/src/classes/modules.mk", b"CLASS_MODULE_SOURCES =\n"),
        ("root/Genesis/src/comm.c", b"/* fixture */\n"),
        ("root/deploy/update_compat.json", json.dumps(
            compat or {"schema": 1, "save_epoch": 2, "handoff": 2}).encode()),
        ("root/Genesis/player/Oldaccount", b"NOT TO BE EXTRACTED"),
    ]
    data = io.BytesIO()
    with tarfile.open(fileobj=data, mode="w:gz") as archive:
        for name, value in files + list(extra):
            item = tarfile.TarInfo(name)
            if value is None:
                item.type = tarfile.SYMTYPE
                item.linkname = "/etc/passwd"
                archive.addfile(item)
            else:
                item.size = len(value)
                archive.addfile(item, io.BytesIO(value))
    return data.getvalue()


class ArchiveTests(unittest.TestCase):
    def extract(self, data):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            result = u.archive_source(data, folder)
            return result, sorted(str(p.relative_to(folder)) for p in folder.rglob("*"))

    def test_only_source_is_written(self):
        compat, names = self.extract(make_archive())
        self.assertEqual(compat["save_epoch"], 2)
        self.assertIn("classes/modules.mk", names)
        self.assertIn("comm.c", names)
        self.assertFalse(any("player" in p or "Oldaccount" in p for p in names))

    def test_traversal_rejected(self):
        for name in ("root/Genesis/src/../escape.c", "/root/Genesis/src/evil.c",
                     "root/Genesis/src/./evil.c", "root/Genesis/src/evil\\name.c"):
            with self.subTest(name=name), self.assertRaises(u.UpdateError):
                self.extract(make_archive([(name, b"x")]))

    def test_source_symlink_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive([("root/Genesis/src/link.c", None)]))

    def test_duplicate_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive([("root/Genesis/src/comm.c", b"other")]))

    def test_multiple_roots_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive([("other/Genesis/src/x.c", b"x")]))

    def test_unexpected_source_file_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive([("root/Genesis/src/merc", NEW)]))

    def test_future_epoch_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive(compat={"schema": 1, "save_epoch": 3, "handoff": 2}))

    def test_legacy_target_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive(compat={"schema": 1, "save_epoch": 1, "handoff": 2}))

    def test_future_handoff_rejected(self):
        with self.assertRaises(u.UpdateError):
            self.extract(make_archive(compat={"schema": 1, "save_epoch": 1, "handoff": 3}))

    def test_compression_bomb_bounded(self):
        with mock.patch.object(u, "MAX_DOWNLOAD", 1024), self.assertRaises(u.UpdateError):
            self.extract(gzip.compress(b"x" * 1025))


class CITests(unittest.TestCase):
    def setUp(self):
        self.runs = [dict(id=i, run_number=1, run_attempt=1, event=event,
                          head_sha=HEAD, head_branch=u.BRANCH,
                          path=".github/workflows/ci.yml", status="completed",
                          conclusion="success") for i, event in ((10, "push"), (20, "pull_request"))]
        self.jobs = [dict(name=name, status="completed", conclusion="success",
                          steps=[dict(name=n, status="completed", conclusion="success")
                                 for n in u.REQUIRED]) for name in (
            "regression (debian:12)", "regression (debian:13)",
            "regression (ubuntu:22.04)", "regression (ubuntu:24.04)")]

    def api(self, path):
        if path.startswith("/git/ref"):
            return {"object": {"sha": HEAD}}
        if path.startswith("/actions/runs?"):
            return {"workflow_runs": self.runs}
        return {"total_count": len(self.jobs), "jobs": self.jobs}

    def check(self):
        with mock.patch.object(u, "api", side_effect=self.api):
            return u.checked_release()

    def test_all_forty_gates_required(self):
        self.assertEqual(self.check(), (HEAD, [10, 20]))

    def test_failed_workflow_rejected(self):
        self.runs[0]["conclusion"] = "failure"
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_pending_workflow_rejected(self):
        self.runs[1]["status"] = "in_progress"
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_latest_failure_not_hidden_by_old_success(self):
        self.runs.append(dict(self.runs[0], id=30, run_number=2, conclusion="failure"))
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_wrong_head_not_accepted(self):
        self.runs[1]["head_sha"] = "b" * 40
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_other_workflow_not_accepted(self):
        self.runs[1]["path"] = ".github/workflows/other.yml"
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_missing_os_rejected(self):
        self.jobs.pop()
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_skipped_gate_rejected(self):
        self.jobs[2]["steps"][4]["conclusion"] = "skipped"
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_missing_gate_rejected(self):
        self.jobs[0]["steps"].pop()
        with self.assertRaises(u.UpdateError):
            self.check()

    def test_duplicate_gate_rejected(self):
        self.jobs[0]["steps"].append(copy.deepcopy(self.jobs[0]["steps"][0]))
        with self.assertRaises(u.UpdateError):
            self.check()


class FilesystemTests(unittest.TestCase):
    def test_binary_reader_rejects_links_and_non_elf(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "merc"
            path.write_bytes(NEW)
            self.assertEqual(u.read_binary(path, os.geteuid()), NEW)
            link = path.with_name("link")
            link.symlink_to(path)
            with self.assertRaises(OSError):
                u.read_binary(link, os.geteuid())
            path.write_bytes(b"not ELF")
            with self.assertRaises(u.UpdateError):
                u.read_binary(path, os.geteuid())

    def test_binary_reader_rejects_hardlink(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "merc"
            path.write_bytes(NEW)
            os.link(path, path.with_name("other"))
            with self.assertRaises(u.UpdateError):
                u.read_binary(path, os.geteuid())

    def test_binary_reader_rejects_wrong_owner(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "merc"
            path.write_bytes(NEW)
            with self.assertRaises(u.UpdateError):
                u.read_binary(path, os.geteuid() + 1)

    def test_protected_rejects_writable_parent(self):
        info = SimpleNamespace(st_uid=0, st_mode=stat.S_IFDIR | 0o777, st_nlink=2)
        with mock.patch.object(Path, "lstat", return_value=info), self.assertRaises(u.UpdateError):
            u.protected(Path("/fixture"), directory=True)

    def test_protected_rejects_symlink(self):
        info = SimpleNamespace(st_uid=0, st_mode=stat.S_IFLNK | 0o777, st_nlink=1)
        with mock.patch.object(Path, "lstat", return_value=info), self.assertRaises(u.UpdateError):
            u.protected(Path("/fixture"))

    def test_atomic_rename_failure_preserves_old(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "merc"
            path.write_bytes(OLD)
            with mock.patch.object(u.os, "replace", side_effect=OSError("rename")):
                with self.assertRaises(OSError):
                    u.atomic_bytes(path, NEW, 0o755)
            self.assertEqual(path.read_bytes(), OLD)
            self.assertEqual(len(list(Path(temporary).iterdir())), 1)

    def test_unsupported_service_layout_rejected(self):
        with mock.patch.object(u, "run", return_value="User=root\nGroup=root\n"):
            with self.assertRaises(u.UpdateError):
                u.service_info()

    def test_matching_process_inode(self):
        info = {"ActiveState": "active", "MainPID": str(os.getpid())}
        with mock.patch.object(u, "BINARY", Path("/proc/self/exe").resolve()):
            self.assertTrue(u.running_installed(info))
        self.assertFalse(u.running_installed({"ActiveState": "inactive", "MainPID": "0"}))

    def test_root_required_for_apply(self):
        with mock.patch.object(u.os, "geteuid", return_value=1000), self.assertRaises(u.UpdateError):
            u.host_preflight()

    def test_dedicated_builder_rejects_game_uid(self):
        service = SimpleNamespace(pw_uid=1234, pw_gid=1234)
        account = SimpleNamespace(pw_uid=1234, pw_gid=2345)
        with mock.patch.object(u.pwd, "getpwnam", return_value=account):
            with self.assertRaises(u.UpdateError):
                u.builder_account(service)

    def test_dedicated_builder_rejects_root(self):
        with mock.patch.object(u.pwd, "getpwnam", return_value=SimpleNamespace(pw_uid=0, pw_gid=0)):
            with self.assertRaises(u.UpdateError):
                u.builder_account(SimpleNamespace(pw_uid=1234, pw_gid=1234))

    def test_dedicated_builder_rejects_supplementary_groups(self):
        account = SimpleNamespace(pw_uid=2345, pw_gid=2345, pw_name="anathema-build")
        with mock.patch.object(u.pwd, "getpwnam", return_value=account), \
             mock.patch.object(u.os, "getgrouplist", return_value=[2345, 27]):
            with self.assertRaises(u.UpdateError):
                u.builder_account(SimpleNamespace(pw_uid=1234, pw_gid=1234))

    def test_dedicated_builder_requires_private_primary_group(self):
        account = SimpleNamespace(pw_uid=2345, pw_gid=2345, pw_name="anathema-build")
        with mock.patch.object(u.pwd, "getpwnam", return_value=account), \
             mock.patch.object(u.os, "getgrouplist", return_value=[2345]), \
             mock.patch.object(u.grp, "getgrgid", return_value=SimpleNamespace(
                 gr_name="users", gr_mem=[])):
            with self.assertRaises(u.UpdateError):
                u.builder_account(SimpleNamespace(pw_uid=1234, pw_gid=1234))

    def test_compile_uses_minimal_environment_and_process_group(self):
        process = mock.Mock()
        process.wait.return_value = 0
        with mock.patch.object(u.subprocess, "Popen", return_value=process) as popen:
            u.compile_source(Path("/fixture/src"), "anathema-build")
        command = popen.call_args.args[0]
        options = popen.call_args.kwargs
        self.assertEqual(command[:3], ("runuser", "-u", "anathema-build"))
        self.assertIn("--no-new-privs", command)
        self.assertTrue(options["start_new_session"])
        self.assertEqual(set(options["env"]), {"PATH", "HOME", "LANG", "LC_ALL"})

    def test_compile_timeout_terminates_only_build_group(self):
        process = mock.Mock(pid=98765)
        process.wait.side_effect = [u.subprocess.TimeoutExpired("fixture", 900), 0]
        with mock.patch.object(u.subprocess, "Popen", return_value=process), \
             mock.patch.object(u.os, "killpg") as kill, self.assertRaises(u.UpdateError):
            u.compile_source(Path("/fixture/src"), "anathema-build")
        kill.assert_called_once_with(98765, u.signal.SIGKILL)

    def test_compile_failure_is_not_success(self):
        process = mock.Mock()
        process.wait.return_value = 2
        with mock.patch.object(u.subprocess, "Popen", return_value=process), self.assertRaises(u.UpdateError):
            u.compile_source(Path("/fixture/src"), "anathema-build")

    def test_redirect_host_rejected(self):
        with self.assertRaises(u.UpdateError):
            u.SafeRedirect().redirect_request(None, None, 302, "", {}, "https://evil.invalid/archive")

    def test_redirect_downgrade_rejected(self):
        with self.assertRaises(u.UpdateError):
            u.SafeRedirect().redirect_request(None, None, 302, "", {}, "http://codeload.github.com/x")

    def test_lock_prevents_concurrent_updaters(self):
        with tempfile.TemporaryDirectory() as temporary:
            lock = Path(temporary) / "private/lock"
            root_stat = SimpleNamespace(st_mode=stat.S_IFREG | 0o600,
                                        st_uid=0, st_nlink=1)
            with mock.patch.object(u, "LOCK", lock), mock.patch.object(u, "protected"), \
                 mock.patch.object(u.os, "fstat", return_value=root_stat):
                with u.update_lock():
                    with self.assertRaisesRegex(u.UpdateError, "Another updater"):
                        with u.update_lock():
                            pass

    def test_lock_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            (folder / "target").write_text("untouched")
            (folder / "lock").symlink_to(folder / "target")
            with mock.patch.object(u, "LOCK", folder / "lock"), \
                 mock.patch.object(u, "protected"), self.assertRaises(OSError):
                u.update_lock().__enter__()
            self.assertEqual((folder / "target").read_text(), "untouched")


class GreetingTests(unittest.TestCase):
    def test_fragmented_greeting_passes(self):
        connection = mock.MagicMock()
        connection.__enter__.return_value = connection
        connection.recv.side_effect = [b"Godwars: Ana", b"thema\nWhat do you wish carved ",
                                       b"upon your tombstone:"]
        with mock.patch.object(u.socket, "create_connection", return_value=connection):
            u.greeting()

    def test_missing_greeting_times_out(self):
        with mock.patch.object(u.socket, "create_connection", side_effect=OSError("refused")), \
             mock.patch.object(u.time, "monotonic", side_effect=range(100)), \
             mock.patch.object(u.time, "sleep"), self.assertRaises(u.UpdateError):
            u.greeting()

    def test_other_service_is_not_readiness(self):
        connection = mock.MagicMock()
        connection.__enter__.return_value = connection
        connection.recv.return_value = b"unrelated tombstone:"
        with mock.patch.object(u.socket, "create_connection", return_value=connection), \
             mock.patch.object(u.time, "monotonic", side_effect=range(100)), \
             mock.patch.object(u.time, "sleep"), self.assertRaises(u.UpdateError):
            u.greeting()


class BuildIntegrationTests(unittest.TestCase):
    @unittest.skipUnless(os.geteuid() == 0 and all(shutil.which(p) for p in
                         ("runuser", "make", "setpriv")), "requires disposable root CI container")
    def test_real_unprivileged_build_cannot_read_private_state(self):
        account = pwd.getpwnam("nobody")
        self.assertNotEqual(account.pw_uid, 0)
        with tempfile.TemporaryDirectory(prefix="anathema-private-test-") as temporary:
            secret = Path(temporary) / "state"
            secret.write_text("synthetic-private-fixture")
            makefile = ("all:\n"
                        "\ttest \"$$(id -u)\" != 0\n"
                        "\tgrep -q 'NoNewPrivs:[[:space:]]*1' /proc/self/status\n"
                        "\ttest ! -r " + str(secret) + "\n"
                        "\tprintf '\\177ELFfixture-built\\n' > merc\n").encode()
            with mock.patch.object(u, "download", return_value=make_archive(makefile=makefile)):
                value = u.build(HEAD, account)
            self.assertEqual(value, b"\x7fELFfixture-built\n")
            self.assertEqual(secret.read_text(), "synthetic-private-fixture")


class SnapshotRaceTests(unittest.TestCase):
    def race(self, directory=False, fifo=False):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "source"
            source.mkdir()
            private = root / "outside"
            private.mkdir()
            (private / "private").write_text("must not enter backup")
            if directory:
                (source / "live").mkdir()
            else:
                (source / "live").write_text("ordinary state")
            original = u.os.open
            def swapping_open(path, flags, *args, **kwargs):
                if path == "live" and "dir_fd" in kwargs:
                    if directory:
                        (source / "live").rmdir()
                    else:
                        (source / "live").unlink()
                    if fifo:
                        os.mkfifo(source / "live")
                    else:
                        (source / "live").symlink_to(
                            private if directory else private / "private")
                return original(path, flags, *args, **kwargs)
            with mock.patch.object(u.os, "open", side_effect=swapping_open):
                with self.assertRaises((OSError, u.UpdateError)):
                    u.copy_state(source, root / "snapshot")
            for path in (root / "snapshot").rglob("*"):
                if path.is_file() and not path.is_symlink():
                    self.assertNotIn(b"must not enter backup", path.read_bytes())

    def test_file_swapped_to_external_link_fails_closed(self):
        self.race()

    def test_directory_swapped_to_external_link_fails_closed(self):
        self.race(directory=True)

    def test_file_swapped_to_fifo_never_blocks(self):
        self.race(fifo=True)


class DeployTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.app, self.data = self.root / "app", self.root / "data"
        (self.app / "src").mkdir(parents=True)
        (self.app / "deploy").mkdir()
        (self.data / "player").mkdir(parents=True)
        (self.data / "player/Luto").write_text("fixture progress")
        self.binary = self.app / "src/merc"
        self.binary.write_bytes(OLD)
        self.record = self.app / "deploy/release.json"
        self.patches = [
            mock.patch.object(u, "APP", self.app), mock.patch.object(u, "DATA", self.data),
            mock.patch.object(u, "BINARY", self.binary), mock.patch.object(u, "RECORD", self.record),
            mock.patch.object(u, "STORE", self.root / "backups"),
            mock.patch.object(u, "LOCK", self.root / "lock/lock"),
            mock.patch.object(u, "protected"), mock.patch.object(u, "host_preflight"),
        ]
        for patch in self.patches:
            patch.start()
            self.addCleanup(patch.stop)
        self.online, self.running = True, OLD
        self.fail_new, self.fail_all = False, False
        real_read = u.read_binary
        self.addCleanup(mock.patch.stopall)
        mock.patch.object(u, "read_binary", side_effect=lambda p, owner: real_read(p, os.geteuid())).start()
        mock.patch.object(u, "service_info", side_effect=lambda: {
            "ActiveState": "active" if self.online else "inactive",
            "MainPID": "123" if self.online else "0"}).start()
        mock.patch.object(u, "running_installed",
                          side_effect=lambda *a: self.online and self.running == self.binary.read_bytes()).start()
        self.run_mock = mock.patch.object(u, "run", side_effect=self.command).start()
        mock.patch.object(u, "greeting", side_effect=self.health).start()
        # fstat owner is otherwise a real non-root CI user's uid in lock tests.
        self.lock = mock.patch.object(u, "update_lock")
        self.lock_mock = self.lock.start()
        self.addCleanup(self.lock.stop)

    def command(self, *args):
        if args[:2] == ("systemctl", "stop"):
            self.online = False
        elif args[:2] == ("systemctl", "start"):
            self.online = True
            self.running = self.binary.read_bytes()
        return ""

    def health(self):
        if self.fail_all or (self.fail_new and self.running == NEW):
            raise u.UpdateError("synthetic health failure")

    def install(self, mode="restart", adopt=True):
        return u.deploy(NEW, u.binary_record(NEW, HEAD), mode, adopt=adopt)

    def test_first_use_requires_adoption(self):
        with self.assertRaises(u.UpdateError):
            self.install(adopt=False)
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertTrue(self.online)

    def test_restart_preserves_data_and_creates_stopped_snapshot(self):
        identifier = self.install()
        self.assertEqual(self.binary.read_bytes(), NEW)
        self.assertTrue(self.online)
        self.assertEqual((self.data / "player/Luto").read_text(), "fixture progress")
        manifest = json.loads((u.STORE / identifier / "manifest.json").read_text())
        self.assertEqual(manifest["state_snapshot"], "service-stopped")
        self.assertEqual((u.STORE / identifier / "state/player/Luto").read_text(), "fixture progress")
        self.assertEqual(u.backup_binary(identifier)[0], OLD)
        self.assertEqual(json.loads(self.record.read_text())["status"], "active")

    def test_copyover_staging_never_stops_or_starts_service(self):
        identifier = self.install("copyover")
        self.assertEqual(self.running, OLD)
        self.assertEqual(self.binary.read_bytes(), NEW)
        self.assertTrue(self.online)
        self.assertEqual(self.run_mock.call_count, 0)
        self.assertEqual(json.loads(self.record.read_text())["status"], "staged")
        manifest = json.loads((u.STORE / identifier / "manifest.json").read_text())
        self.assertEqual(manifest["state_snapshot"], "online-not-point-in-time")

    def test_staged_blocks_next_update(self):
        self.install("copyover")
        with self.assertRaises(u.UpdateError):
            self.install()
        self.assertTrue(self.online)

    def test_copyover_staging_requires_running_server(self):
        self.online = False
        with self.assertRaises(u.UpdateError):
            self.install("copyover")
        self.assertEqual(self.binary.read_bytes(), OLD)

    def test_pending_manual_binary_swap_blocks_update(self):
        self.running = NEW
        with self.assertRaises(u.UpdateError):
            self.install()

    def test_failed_health_restores_old_binary(self):
        self.fail_new = True
        with self.assertRaisesRegex(u.UpdateError, "previous binary restored"):
            self.install()
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertTrue(self.online)
        self.assertEqual(json.loads(self.record.read_text())["binary_sha256"],
                         u.binary_record(OLD)["binary_sha256"])

    def test_forward_epoch_upgrade(self):
        u.write_record(dict(u.binary_record(OLD), save_epoch=1))
        self.install()
        self.assertEqual(json.loads(self.record.read_text())["save_epoch"], 2)
        self.assertEqual(self.binary.read_bytes(), NEW)

    def test_epoch_downgrade_rejected_before_changes(self):
        u.write_record(u.binary_record(OLD))
        with self.assertRaisesRegex(u.UpdateError, "Unsafe save-epoch downgrade"):
            u.deploy(NEW, dict(u.binary_record(NEW), save_epoch=1), "restart")
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertEqual(self.run_mock.call_count, 0)
        self.assertFalse(u.STORE.exists())

    def test_failed_epoch_upgrade_never_restarts_legacy(self):
        u.write_record(dict(u.binary_record(OLD), save_epoch=1))
        self.fail_new = True
        with self.assertRaisesRegex(u.UpdateError, "save-epoch upgrade"):
            self.install()
        self.assertEqual(self.binary.read_bytes(), NEW)
        self.assertFalse(self.online)
        self.assertEqual((self.data / "player/Luto").read_text(), "fixture progress")

    def test_failed_recovery_is_reported_truthfully(self):
        self.fail_all = True
        with self.assertRaisesRegex(u.UpdateError, "recovery also failed"):
            self.install()

    def test_backup_failure_restarts_old_without_install(self):
        with mock.patch.object(u, "snapshot", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                self.install()
        self.assertTrue(self.online)
        self.assertEqual(self.binary.read_bytes(), OLD)

    def test_post_rename_sync_failure_recovers(self):
        original = u.atomic_bytes
        failed = False
        def write(path, value, mode=0o600):
            nonlocal failed
            original(path, value, mode)
            if path == self.binary and not failed:
                failed = True
                raise OSError("directory fsync failed")
        with mock.patch.object(u, "atomic_bytes", side_effect=write):
            with self.assertRaisesRegex(u.UpdateError, "previous binary restored"):
                self.install()
        self.assertEqual(self.binary.read_bytes(), OLD)

    def test_record_write_failure_recovers(self):
        original = u.write_record
        calls = 0
        def write(record):
            nonlocal calls
            calls += 1
            if calls == 1:
                raise OSError("record write failure")
            original(record)
        with mock.patch.object(u, "write_record", side_effect=write):
            with self.assertRaisesRegex(u.UpdateError, "previous binary restored"):
                self.install()
        self.assertEqual(self.binary.read_bytes(), OLD)

    def test_low_disk_refuses_before_replacement(self):
        with mock.patch.object(u.shutil, "disk_usage", return_value=SimpleNamespace(free=1)):
            with self.assertRaises(u.UpdateError):
                self.install()
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertTrue(self.online)

    def test_explicit_rollback_preserves_new_progress(self):
        identifier = self.install()
        (self.data / "player/Luto").write_text("later progress")
        with mock.patch.object(u, "api", side_effect=AssertionError("network forbidden")):
            u.main(["--rollback", identifier, "--apply"])
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertEqual((self.data / "player/Luto").read_text(), "later progress")

    def test_corrupt_backup_refused(self):
        identifier = self.install()
        (u.STORE / identifier / "merc").write_bytes(b"\x7fELFcorrupt")
        with self.assertRaises(u.UpdateError):
            u.backup_binary(identifier)
        self.assertEqual(self.binary.read_bytes(), NEW)

    def test_backup_copies_links_without_reading_targets(self):
        outside = self.root / "not-part-of-world"
        outside.write_text("must not be copied")
        (self.data / "external").symlink_to(outside)
        identifier = self.install()
        copied = u.STORE / identifier / "state/external"
        self.assertTrue(copied.is_symlink())
        self.assertEqual(os.readlink(copied), str(outside))

    def test_interrupted_record_mismatch_can_be_explicitly_rolled_back(self):
        identifier = self.install()
        u.write_record(u.binary_record(OLD, adopted=True))
        with self.assertRaises(u.UpdateError):
            self.install()
        value, previous = u.backup_binary(identifier)
        u.deploy(value, previous, "restart", recovering=True)
        self.assertEqual(self.binary.read_bytes(), OLD)

    def test_stop_failure_never_replaces_binary(self):
        with mock.patch.object(u, "stop", side_effect=u.UpdateError("stop failed")):
            with self.assertRaises(u.UpdateError):
                self.install()
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertTrue(self.online)

    def test_rollback_traversal_refused(self):
        for name in ("../old", "/root/old", "not-an-id"):
            with self.subTest(name=name), self.assertRaises(u.UpdateError):
                u.main(["--rollback", name])

    def test_verify_does_not_claim_staged_binary_is_running(self):
        self.install("copyover")
        with self.assertRaises(u.UpdateError):
            u.main(["--verify"])
        self.assertEqual(json.loads(self.record.read_text())["status"], "staged")

    def test_verify_marks_actual_handover_active(self):
        self.install("copyover")
        self.running = NEW
        u.main(["--verify"])
        self.assertEqual(json.loads(self.record.read_text())["status"], "active")

    def test_checksum_mismatch_blocks_ordinary_update(self):
        self.install()
        self.binary.write_bytes(b"\x7fELFmanually modified")
        with self.assertRaises(u.UpdateError):
            self.install()

    def test_preview_never_builds_or_mutates_host(self):
        with mock.patch.object(u, "checked_release", return_value=(HEAD, [1, 2])), \
             mock.patch.object(u, "build", side_effect=AssertionError("must not build")):
            u.main([])
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertFalse(self.record.exists())

    def test_branch_movement_blocks_install(self):
        with mock.patch.object(u, "checked_release", return_value=(HEAD, [1, 2])), \
             mock.patch.object(u, "builder_account"), mock.patch.object(u, "build", return_value=NEW), \
             mock.patch.object(u, "api", return_value={"object": {"sha": "b" * 40}}):
            with self.assertRaises(u.UpdateError):
                u.main(["--apply", "--adopt-current"])
        self.assertEqual(self.binary.read_bytes(), OLD)
        self.assertTrue(self.online)


if __name__ == "__main__":
    unittest.main()
