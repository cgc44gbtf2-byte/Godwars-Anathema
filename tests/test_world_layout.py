"""World seed, installer selection and safe local-start regression checks."""
import contextlib
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


setup = module("world_setup_tests", ROOT / "deploy/setup.py")
local = module("local_start_tests", ROOT / "tools/start_local.py")


class WorldChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="anathema-world-layout-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.world = self.root / "Genesis/area"
        self.world.mkdir(parents=True)
        (self.world / "area.lst").write_text("fixture.are\n$\n")
        (self.world / "fixture.are").write_text("#AREA\nFixture~\n#$\n")
        binary = self.root / "Genesis/src/merc"
        binary.parent.mkdir()
        binary.write_text("fixture executable, never execute\n")
        binary.chmod(0o700)

    def test_versioned_world_is_complete(self):
        files = setup.world_files(ROOT / "Genesis/area")
        self.assertGreater(len(files), 60)
        self.assertIn("relic.are", files)
        self.assertIn("class.hlp", files)
        self.assertTrue(all("/" not in name for name in files))

    def test_greeting_has_anathema_branding_and_original_attribution(self):
        helps = (ROOT / "Genesis/area/main.hlp").read_text(encoding="latin1")
        greeting = helps.split("-1 GREETING~", 1)[1].split("~", 1)[0]
        for text in ("Anathema", "Created by LaWaylon (Luto)",
                     "Copyright (c) 2026 LaWaylon (Luto) - Anathema modifications",
                     "Furey, Hatchet and Kahn", "God Wars was created by KaVir",
                     "Original Genesis development: Akuma", "Xentoth"):
            self.assertIn(text, greeting)
        self.assertNotIn("owned and coded by", greeting)
        self.assertNotIn("Created from Genesis", helps)
        self.assertNotIn("asmodeus666@", greeting)
        self.assertNotIn("donate+azmokuma", helps)

    def test_active_area_branding_is_anathema(self):
        for path in (ROOT / "Genesis/area").glob("*.are"):
            self.assertNotIn("genesis", path.read_text(encoding="latin1").lower(), path.name)

    def test_leader_help_uses_registry_not_historical_accounts(self):
        helpfile = (ROOT / "Genesis/area/class.hlp").read_text(encoding="latin1")
        leaders = helpfile.split("0 Leaders~", 1)[1].split("~", 1)[0]
        self.assertIn("clan roster", leaders)
        self.assertIn("Elementalist and Sorcerer", leaders)
        self.assertNotIn("Baenrae - Laele", leaders)

    def test_obsolete_runtime_branding_is_gone(self):
        for filename, old in (("comm.c", "Welcome to Godwars: Genesis"),
                              ("act_info.c", "GW: Genesis started"),
                              ("signals.c", "GENESIS IS ATTEMPTING"),
                              ("classes/vampire/vampire.c", "supported Genesis discipline")):
            self.assertNotIn(old, (ROOT / "Genesis/src" / filename).read_text())

    def test_directory_import_excludes_runtime_files(self):
        for name in ("copyover.data", "ban.txt", "disabled.txt", ".gitkeep"):
            (self.world / name).write_text("must not import")
        self.assertEqual(set(setup.world_files(self.world)), {"area.lst", "fixture.are"})

    def test_directory_missing_reference_is_rejected(self):
        (self.world / "fixture.are").unlink()
        with self.assertRaises(ValueError):
            setup.world_files(self.world)

    def test_directory_source_symlink_is_rejected(self):
        link = self.root / "alias"
        link.symlink_to(self.world, target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "symlink"):
            setup.world_files(link)

    def test_directory_entry_symlink_is_rejected(self):
        path = self.world / "fixture.are"
        path.unlink()
        path.symlink_to(self.root / "Genesis/src/merc")
        with self.assertRaises((ValueError, OSError)):
            setup.world_files(self.world)

    def test_directory_fifo_is_rejected_without_blocking(self):
        os.mkfifo(self.world / "pipe.are")
        with self.assertRaisesRegex(ValueError, "regular"):
            setup.world_files(self.world)

    def test_directory_case_duplicates_are_rejected(self):
        (self.world / "FIXTURE.ARE").write_text("duplicate")
        with self.assertRaisesRegex(ValueError, "duplicate"):
            setup.world_files(self.world)

    def test_directory_size_limit_is_enforced(self):
        with patch.object(setup, "LIMIT", 1), self.assertRaisesRegex(ValueError, "size"):
            setup.world_files(self.world)

    def test_installer_defaults_to_versioned_world(self):
        with patch.object(setup, "REPO", self.root), contextlib.redirect_stdout(io.StringIO()) as output:
            with patch("sys.argv", ["setup.py"]):
                setup.main()
        self.assertIn(str(self.world), output.getvalue())

    def test_runtime_preparation_is_private_and_preserves_data(self):
        player = self.root / "Genesis/player"
        player.mkdir()
        (player / "fixture").write_text("preserve")
        local.prepare_runtime(self.root / "Genesis")
        self.assertEqual((player / "fixture").read_text(), "preserve")
        for name in local.RUNTIME:
            self.assertEqual((self.root / "Genesis" / name).stat().st_mode & 0o777, 0o700)

    def test_runtime_symlink_rejection_has_no_partial_chmod(self):
        before = self.world.stat().st_mode
        (self.root / "Genesis/player").symlink_to(self.world, target_is_directory=True)
        with self.assertRaises(ValueError):
            local.prepare_runtime(self.root / "Genesis")
        self.assertEqual(self.world.stat().st_mode, before)

    def test_runtime_foreign_owner_is_rejected(self):
        with patch.object(local.os, "geteuid", return_value=os.getuid() + 1), self.assertRaises(ValueError):
            local.prepare_runtime(self.root / "Genesis")

    def test_root_start_is_rejected(self):
        with patch.object(local, "REPO", self.root), patch.object(local.os, "geteuid", return_value=0):
            with patch.object(local.os, "execv") as execute, self.assertRaisesRegex(ValueError, "root"):
                local.main([])
            execute.assert_not_called()

    def test_check_mode_does_not_start_or_create_runtime(self):
        with patch.object(local, "REPO", self.root), patch.object(local.os, "execv") as execute:
            with contextlib.redirect_stdout(io.StringIO()):
                local.main(["--check"])
            execute.assert_not_called()
        self.assertFalse((self.root / "Genesis/player").exists())

    def test_start_uses_correct_directory_and_port(self):
        with patch.object(local, "REPO", self.root), patch.object(local.os, "geteuid", return_value=1000):
            with patch.object(local, "prepare_runtime") as prepare, patch.object(local.os, "chdir") as chdir, patch.object(local.os, "umask") as umask:
                with patch.object(local.os, "execv") as execute:
                    local.main(["--port", "4040"])
                    umask.assert_called_once_with(0o077)
                    binary = str(self.root / "Genesis/src/merc")
                    execute.assert_called_once_with(binary, [binary, "4040"])
                chdir.assert_called_once_with(self.world)
                prepare.assert_called_once_with(self.root / "Genesis")

    def test_invalid_port_is_rejected(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
            local.main(["--port", "0"])
        self.assertEqual(error.exception.code, 2)

    def test_live_state_is_ignored_but_seed_files_are_not(self):
        (self.root / ".gitignore").write_bytes((ROOT / ".gitignore").read_bytes())
        subprocess.run(["git", "init", "-q", str(self.root)], check=True, capture_output=True)
        for name in ("Genesis/player/Example", "Genesis/backup/Example",
                     "Genesis/notes/general", "Genesis/log/current", "Genesis/clans/registry",
                     "Genesis/area/copyover.data", "Genesis/area/ban.txt"):
            result = subprocess.run(["git", "check-ignore", "--no-index", "-q", name], cwd=self.root)
            self.assertEqual(result.returncode, 0, name)
        for name in ("Genesis/area/area.lst", "Genesis/area/relic.are", "Genesis/area/class.hlp",
                     "Genesis/player/.gitkeep", "Genesis/clans/.gitkeep"):
            result = subprocess.run(["git", "check-ignore", "--no-index", "-q", name], cwd=self.root)
            self.assertEqual(result.returncode, 1, name)


if __name__ == "__main__":
    unittest.main()
