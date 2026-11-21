"""Package consistency and real Lua 5.1-compatible headless HUD regressions."""
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LUA = ROOT / "clients/mudlet/anathema_hud.lua"
XML = ROOT / "clients/mudlet/AnathemaHUD.xml"


class MudletHUDTests(unittest.TestCase):
    def run_lua(self, source):
        runtime = shutil.which("luajit") or shutil.which("lua5.1")
        self.assertIsNotNone(runtime, "HUD tests require luajit or lua5.1 (not a server runtime dependency)")
        result = subprocess.run(
            [runtime, str(ROOT / "tests/test_mudlet_hud.lua"), str(source)],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=30, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout[-6000:])
        self.assertIn("Mudlet HUD headless Lua cases passed: 23", result.stdout)

    def test_canonical_lua_behavior(self):
        self.run_lua(LUA)

    def test_generated_xml_matches_lua(self):
        result = subprocess.run(
            ["python3", str(ROOT / "tools/build_mudlet_hud.py"), "--check"],
            capture_output=True, text=True, timeout=10,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        root = ET.parse(XML).getroot()
        self.assertEqual(root.tag, "MudletPackage")
        scripts = root.findall("./ScriptPackage/Script")
        self.assertEqual(len(scripts), 1)
        self.assertEqual(scripts[0].get("isActive"), "yes")
        self.assertEqual(scripts[0].findtext("script"), LUA.read_text(encoding="utf-8"))
        self.assertFalse(root.findall(".//Trigger") or root.findall(".//Alias"))

    def test_xml_payload_behavior(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "from_xml.lua"
            source.write_text(ET.parse(XML).findtext("./ScriptPackage/Script/script"), encoding="utf-8")
            self.run_lua(source)


if __name__ == "__main__":
    unittest.main()
