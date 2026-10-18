"""Build the single-script Mudlet XML from the canonical Lua source."""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "clients/mudlet/anathema_hud.lua"
TARGET = ROOT / "clients/mudlet/AnathemaHUD.xml"


def document():
    lua = SOURCE.read_text(encoding="utf-8").replace("]]>", "]]]]><![CDATA[>")
    return (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<!DOCTYPE MudletPackage>\n'
        '<MudletPackage version="1.001">\n'
        '  <ScriptPackage>\n'
        '    <Script isActive="yes" isFolder="no">\n'
        '      <name>Anathema HUD</name>\n'
        '      <script><![CDATA[' + lua + ']]></script>\n'
        '      <eventHandlerList />\n'
        '    </Script>\n'
        '  </ScriptPackage>\n'
        '</MudletPackage>\n'
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = document()
    if args.check:
        if not TARGET.exists() or TARGET.read_text(encoding="utf-8") != expected:
            raise SystemExit("Regenerate clients/mudlet/AnathemaHUD.xml with tools/build_mudlet_hud.py")
        print("Mudlet XML matches its canonical Lua source")
    else:
        TARGET.write_text(expected, encoding="utf-8")
        print("Generated clients/mudlet/AnathemaHUD.xml")
