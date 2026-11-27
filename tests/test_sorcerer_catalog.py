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
area = (root / "Genesis/area/school.are").read_text()
if not re.search(r"^#3730\nsorctrainer ", area, re.M): fail("school.are lacks the sorctrainer mob 3730")
if "M 0 3730 1 3700" not in area: fail("sorctrainer is not reset into the Mud School entrance")
if "sorctrainer" not in (src / "classes/sorcerer/apprentice.c").read_text(): fail("apprentice.c ignores sorctrainer")
help_text = (root / "Genesis/area/class.hlp").read_text()
help_entries = re.findall(r"^(-?\d+) ([^\n~]+)~\n([^~]*)~", help_text, re.M)
for keyword, phrases in (
    ("Learn", ("20 x (number of runes", "maximum 50 primal", "30 primal",
               "3 runes, 2 glyphs", "all ten other runes")),
    ("Recipe", ("50 x L mana", "40 x L mana", "Master gives NO discount",
                "slash, pierce or blunt", "53 combinations", "minimum mana cost")),
    ("Apprentice", ("5 primal", "10 primal", "10-minute", "2 minutes",
                   "5 lessons", "100 primal", "already count as full Sorcerers")),
):
    matches = [(level, body) for level, names, body in help_entries
               if keyword.lower() in names.lower().split()]
    if len(matches) != 1: fail("expected one in-game help entry for " + keyword)
    level, body = matches[0]
    if level != "0": fail(keyword + " help must be available to players")
    if len(body.replace("\n", "\n\r").encode()) >= 4096:
        fail(keyword + " help exceeds the string buffer")
    for phrase in phrases:
        if phrase not in body: fail(keyword + " help missing " + phrase)
learn_help = next(body for _, names, body in help_entries if "Learn" in names.split())
for kind in ("runes", "glyphs", "sigils"):
    names = re.search(r"%s\[\w+\] = \{(.*?)\};" % kind, comp, re.S)[1]
    for name in re.findall(r'\{"([^"]+)"', names):
        if not re.search(r"\b" + name + r"\b", learn_help):
            fail("Learn help missing component " + name)
print("sorcerer catalogue: 53 recipes, 11/10/4 components, docs and saves in sync")
