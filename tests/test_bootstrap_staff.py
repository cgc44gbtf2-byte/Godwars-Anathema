"""Offline staff bootstrap: synthetic saves only, never actual accounts."""
from pathlib import Path
import fcntl
import io
import os
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "deploy"))
from bootstrap_staff import BootstrapError, bootstrap  # noqa: E402


def record(name, password=b"synthetic", level=1):
    return (b"#PLAYER\nName         " + name.encode() + b"~\n"
            + b"Level        " + str(level).encode() + b"\n"
            + b"HpMana       25 100 35 100\n"
            + b"Password     " + password + b"~\n"
            + b"Description  synthetic text~\nEnd\n#END\n")


class BootstrapChecks(unittest.TestCase):
    def setUp(self):
        for stream in ("stdout", "stderr"):
            capture = patch("sys." + stream, new_callable=io.StringIO)
            capture.start()
            self.addCleanup(capture.stop)
        self.tmp = tempfile.TemporaryDirectory(prefix="anathema-bootstrap-test-")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for name in ("player", "backup", "clans"):
            (self.root / name).mkdir(mode=0o700)
        (self.root / "clans/Lock").write_bytes(b"")
        (self.root / "clans/Lock").chmod(0o600)
        for name in ("Yami", "Luto"):
            for store in ("player", "backup"):
                path = self.root / store / name
                path.write_bytes(record(name))
                path.chmod(0o600)

    def test_preview_does_not_change_saves(self):
        before = (self.root / "player/Yami").read_bytes()
        bootstrap(self.root, "Yami")
        self.assertEqual((self.root / "player/Yami").read_bytes(), before)
        self.assertFalse((self.root / "bootstrap-backups").exists())

    def test_objects_unchanged_and_engine_role_constants_match(self):
        import re
        from bootstrap_staff import ROLES
        header = (ROOT / "Genesis/src/merc.h").read_text()
        maximum = re.search(r"#define\s+MAX_LEVEL\s+(\d+)", header)
        self.assertIsNotNone(maximum)
        self.assertRegex(header, r"#define\s+LEV_ST\s+\(MAX_LEVEL\)")
        self.assertRegex(header, r"#define\s+LEV_IMP\s+\(LEV_ST\s*-\s*1\)")
        self.assertEqual(ROLES["Yami"][0], int(maximum.group(1)))
        self.assertEqual(ROLES["Luto"][0], int(maximum.group(1)) - 1)
        original = record("Yami").replace(
            b"#END\n", b"#OBJECT\nVnum 1234\nLevel 99\nEnd\n#END\n")
        for store in ("player", "backup"):
            (self.root / store / "Yami").write_bytes(original)
        bootstrap(self.root, "Yami", True)
        for store in ("player", "backup"):
            self.assertIn(b"#OBJECT\nVnum 1234\nLevel 99\nEnd\n",
                          (self.root / store / "Yami").read_bytes())

    def test_duplicate_field_and_truncated_save_refused(self):
        player = self.root / "player/Yami"
        for content in (
            record("Yami").replace(b"Level        1\n", b"Level 1\nLevel 1\n"),
            record("Yami").replace(b"#END\n", b""),
        ):
            player.write_bytes(content)
            with self.assertRaises(BootstrapError):
                bootstrap(self.root, "Yami", True)

    def test_promotes_only_named_saves_with_private_originals(self):
        originals = {
            (name, store): (self.root / store / name).read_bytes()
            for name in ("Yami", "Luto") for store in ("player", "backup")
        }
        bootstrap(self.root, "Yami", True)
        bootstrap(self.root, "Luto", True)
        for name, (level, hp) in (("Yami", (7, 500000)), ("Luto", (6, 300000))):
            for store in ("player", "backup"):
                data = (self.root / store / name).read_bytes()
                expected = originals[(name, store)].replace(
                    b"Level        1", b"Level        " + str(level).encode()
                ).replace(
                    b"HpMana       25 100 35 100",
                    b"HpMana       " + b" ".join([str(hp).encode()] * 4)
                )
                self.assertEqual(data, expected)
                snapshot = self.root / "bootstrap-backups" / (name + "." + store + ".pre-bootstrap")
                self.assertEqual(snapshot.read_bytes(), originals[(name, store)])
                self.assertEqual(snapshot.stat().st_mode & 0o777, 0o600)
        self.assertEqual((self.root / "bootstrap-backups").stat().st_mode & 0o777, 0o700)
        with self.assertRaises(BootstrapError):
            bootstrap(self.root, "Yami", True)

    def test_active_game_lock_prevents_changes(self):
        fd = os.open(self.root / "clans/Lock", os.O_RDWR)
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            with self.assertRaisesRegex(BootstrapError, "server is running"):
                bootstrap(self.root, "Yami", True)
            self.assertFalse((self.root / "bootstrap-backups").exists())
        finally:
            os.close(fd)

    def test_password_mismatch_and_missing_backup_rejected(self):
        backup = self.root / "backup/Yami"
        backup.write_bytes(record("Yami", password=b"different"))
        with self.assertRaisesRegex(BootstrapError, "credentials differ"):
            bootstrap(self.root, "Yami", True)
        backup.unlink()
        with self.assertRaises(FileNotFoundError):
            bootstrap(self.root, "Yami", True)
        self.assertFalse((self.root / "bootstrap-backups").exists())

    def test_symlink_and_unsafe_mode_rejected(self):
        target = self.root / "outside"
        target.write_bytes(record("Yami"))
        player = self.root / "player/Yami"
        player.unlink()
        player.symlink_to(target)
        with self.assertRaises(OSError):
            bootstrap(self.root, "Yami", True)
        self.assertEqual(target.read_bytes(), record("Yami"))
        player.unlink()
        player.write_bytes(record("Yami"))
        player.chmod(0o644)
        with self.assertRaisesRegex(BootstrapError, "unsafe"):
            bootstrap(self.root, "Yami", True)

    def test_existing_snapshot_blocks_retry_without_overwrite(self):
        backup_dir = self.root / "bootstrap-backups"
        backup_dir.mkdir(mode=0o700)
        snapshot = backup_dir / "Yami.player.pre-bootstrap"
        snapshot.write_bytes(b"existing")
        with self.assertRaisesRegex(BootstrapError, "prior snapshot"):
            bootstrap(self.root, "Yami", True)
        self.assertEqual(snapshot.read_bytes(), b"existing")
        self.assertEqual((self.root / "player/Yami").read_bytes(), record("Yami"))

    def test_failed_first_rename_keeps_both_originals(self):
        with patch("bootstrap_staff.os.replace", side_effect=OSError("injected")):
            with self.assertRaises(OSError):
                bootstrap(self.root, "Yami", True)
        for store in ("player", "backup"):
            self.assertEqual((self.root / store / "Yami").read_bytes(), record("Yami"))
            self.assertEqual(
                (self.root / "bootstrap-backups" / ("Yami." + store + ".pre-bootstrap")).read_bytes(),
                record("Yami"))
            self.assertFalse(list((self.root / store).glob(".bootstrap-*")))

    def test_failed_second_rename_reports_partial_and_preserves_originals(self):
        replace = os.replace
        calls = 0
        def fail_second(*args, **kwargs):
            nonlocal calls
            calls += 1
            if calls == 2:
                raise OSError("injected")
            return replace(*args, **kwargs)
        with patch("bootstrap_staff.os.replace", side_effect=fail_second):
            with self.assertRaises(OSError):
                bootstrap(self.root, "Yami", True)
        self.assertEqual((self.root / "player/Yami").read_bytes(), record("Yami"))
        self.assertIn(b"Level        7", (self.root / "backup/Yami").read_bytes())
        for store in ("player", "backup"):
            self.assertEqual(
                (self.root / "bootstrap-backups" / ("Yami." + store + ".pre-bootstrap")).read_bytes(),
                record("Yami"))


if __name__ == "__main__":
    unittest.main()
