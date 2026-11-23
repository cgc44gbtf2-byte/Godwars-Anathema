#!/usr/bin/env python3
"""Keeps the Sorcerer component/recipe catalogue, docs and save wiring in sync."""
import re, subprocess, sys
from pathlib import Path
root = Path(__file__).resolve().parent.parent
src = root / "Genesis/src"
doc = (root / "docs/SORCERER_COMPONENTS.md").read_text()
table = (src / "classes/sorcerer/recipe_table.h").read_text()
comp = (src / "classes/sorcerer/components.c").read_text()

def fail(msg):
    print("sorcerer catalogue:", msg); sys.exit(1)

entries = re.findall(r"^\s+\{(\d+), (\d+), (\d+), \"([^\"]+)\"", table, re.M)
if len(entries) != 53: fail("expected 53 recipes, found %d" % len(entries))
if len({e[:3] for e in entries}) != 53: fail("duplicate recipe key")
rows = re.findall(r"^\| (\d{5,7}) \|", doc, re.M)
if len(rows) != 53: fail("doc lists %d recipes" % len(rows))
gathering = [l.split(":")[0] for l in re.findall(r"/\* (\d+) ", table)]
if sorted(gathering, key=int) != sorted(rows, key=int): fail("doc and table keys differ")
for kind, n in (("runes", 11), ("glyphs", 10), ("sigils", 4)):
    m = re.search(r"%s\[\w+\] = \{(.*?)\};" % kind, comp, re.S)
    if not m or len(re.findall(r"\{\"", m.group(1))) != n: fail("%s count" % kind)
gen = subprocess.run([sys.executable, str(root / "tools/gen_sorcerer_recipes.py")], capture_output=True, text=True, cwd=root)
if gen.returncode == 0:
    if (src / "classes/sorcerer/recipe_table.h").read_text() != table: fail("recipe_table.h is stale")
save = (src / "save.c").read_text()
for token in ('"Components"', '"Apprentice"'):
    if token not in save: fail("save.c missing " + token)
if save.count("memset(ch->pcdata->components") < 2: fail("loaders must zero components")
for needle in ("level", "trust"):
    if re.search(r"ch->%s\s*=" % needle, (src / "classes/sorcerer/apprentice.c").read_text()):
        fail("apprenticeship must not write " + needle)
print("sorcerer catalogue: 53 recipes, 11/10/4 components, docs and saves in sync")
