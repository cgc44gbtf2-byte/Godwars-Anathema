"""Fresh-install helper: archive validation and simulated host orchestration only."""
import importlib.util
import os
from pathlib import Path
import stat
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("setup_helper", ROOT / "deploy/setup.py")
setup = importlib.util.module_from_spec(spec)
spec.loader.exec_module(setup)


class SetupChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="anathema-setup-test-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def archive(self, entries):
        name = self.root / "world.zip"
        with zipfile.ZipFile(name, "w") as archive:
            for key, value in entries:
                archive.writestr(key, value)
        return name

    def valid(self):
        return [("Genesis/area/area.lst", "world.are\n$"),
                ("Genesis/area/world.are", "#AREA\nFixture~\n")]

    def test_only_world_data_is_imported(self):
        path = self.archive(self.valid() + [
            ("Genesis/player/Luto", "never import"), ("Genesis/area/ban.txt", "old"),
            ("Genesis/area/copyover.data", "old"), ("Genesis/src/merc", "old")])
        self.assertEqual(set(setup.world_files(path)), {"world.are", "area.lst"})

    def test_missing_list_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "area.lst"):
            setup.world_files(self.archive(self.valid()[1:]))

    def test_traversal_and_missing_references_fail(self):
        for value in ("../player/Luto\n$", "/etc/passwd\n$", "absent.are\n$"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                setup.world_files(self.archive([("Genesis/area/area.lst", value)]))

    def test_list_requires_final_sentinel(self):
        with self.assertRaises(ValueError):
            setup.world_files(self.archive([("Genesis/area/area.lst", "world.are")]))

    def test_case_duplicate_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            setup.world_files(self.archive(self.valid() + [("Genesis/area/WORLD.ARE", "bad")]))

    def test_symlink_world_entry_is_rejected(self):
        entry = zipfile.ZipInfo("Genesis/area/world.are")
        entry.create_system = 3
        entry.external_attr = (stat.S_IFLNK | 0o777) << 16
        with self.assertRaises(ValueError):
            setup.world_files(self.archive([self.valid()[0], (entry, "/etc/passwd")]))

    def test_world_size_limit_is_enforced(self):
        with patch.object(setup, "LIMIT", 1), self.assertRaisesRegex(ValueError, "size"):
            setup.world_files(self.archive(self.valid()))

    def test_existing_or_dangling_target_is_never_overwritten(self):
        target = self.root / "target"
        target.symlink_to(self.root / "absent")
        with self.assertRaisesRegex(ValueError, "existing"):
            setup.fresh_paths([target])

    def test_new_files_are_exclusive_and_private(self):
        target = self.root / "file"
        setup.new_file(target, b"fixture", 0o600)
        self.assertEqual(target.stat().st_mode & 0o777, 0o600)
        with self.assertRaises(FileExistsError):
            setup.new_file(target, b"replaced", 0o600)
        self.assertEqual(target.read_bytes(), b"fixture")

    def test_binary_symlinks_are_refused(self):
        target = self.root / "binary"
        target.symlink_to("/dev/null")
        with self.assertRaises(OSError):
            setup.install_binary(target, self.root / "installed", os.getuid())

    def test_apply_requires_root(self):
        with patch.object(setup.os, "geteuid", return_value=1000), self.assertRaisesRegex(ValueError, "sudo"):
            setup.install({}, False)

    def test_simulated_fresh_install_and_explicit_start(self):
        repo = self.root / "repo"
        (repo / "Genesis/src").mkdir(parents=True)
        (repo / "Genesis/src/Makefile").write_text("# fixture\n")
        (repo / "deploy").mkdir()
        (repo / "deploy/INSTALL.md").write_text("fixture instructions")
        (repo / "deploy/anathema.service").write_text("[Service]\n")
        app, data, unit = self.root / "app", self.root / "data", self.root / "unit"
        commands, state = [], {"created": False}
        account = SimpleNamespace(pw_uid=os.getuid(), pw_gid=os.getgid())
        def user(_):
            if not state["created"]:
                raise KeyError
            return account
        def run(*args, **options):
            commands.append(args)
            if args[0] == "useradd":
                state["created"] = True
            if args[0] == "runuser":
                self.assertEqual(options["cwd"], Path(args[-2]).parent)
                self.assertEqual(options["cwd"].stat().st_mode & 0o777, 0o755)
                (Path(args[-2]) / "merc").write_bytes(b"\x7fELF-fixture")
        read_text, is_dir = Path.read_text, Path.is_dir
        def read(path, *args, **kwargs):
            if str(path) == "/etc/os-release":
                return 'ID=debian\nVERSION_ID="12"\n'
            return read_text(path, *args, **kwargs)
        def directory(path):
            return True if str(path) == "/run/systemd/system" else is_dir(path)
        with patch.multiple(setup, REPO=repo, APP=app, DATA=data, UNIT=unit, BUILD_ROOT=self.root), \
             patch.object(setup, "run", side_effect=run), \
             patch.object(setup, "trusted_parents"), \
             patch.object(setup.os, "geteuid", return_value=0), \
             patch.object(setup.pwd, "getpwnam", side_effect=user), \
             patch.object(setup.grp, "getgrnam", side_effect=KeyError), \
             patch.object(Path, "read_text", read), patch.object(Path, "is_dir", directory):
            previous = os.umask(0o077)
            try:
                setup.install({"area.lst": b"world.are\n$", "world.are": b"fixture"}, True)
            finally:
                os.umask(previous)
            self.assertEqual((app / "src/merc").stat().st_mode & 0o777, 0o755)
            self.assertEqual(data.stat().st_mode & 0o777, 0o755)
            self.assertEqual((data / "player").stat().st_mode & 0o777, 0o700)
            self.assertFalse(list((data / "player").iterdir()))
            self.assertFalse(list((data / "backup").iterdir()))
            self.assertEqual((data / "src").resolve(), app / "src")
            self.assertEqual(commands[-1], ("systemctl", "enable", "--now", "anathema.service"))
            before = len(commands)
            with self.assertRaisesRegex(ValueError, "existing"):
                setup.install({}, True)
            self.assertEqual(len(commands), before)


if __name__ == "__main__":
    unittest.main()
