"""Reverse only reviewed class-help strings; preserve older gameplay pins."""

SPHERES = [
    ("life", "Life powers include readaura, mageheal, betterbody and polymorphother.", 4),
    ("prime", "Prime powers include heartsblood, holysmote, mageenchant and ward.", 5),
    ("matter", "Matter powers include shatterwall, sculpture and alterweight.", 5),
    ("spirit", "Spirit powers include spiritsense, spiritcaress, stepsideways and breakdreamshell.", 5),
    ("entropy", "Entropy powers include beginnersluck, brittlebones and mutateephemera.", 5),
    ("correspondence", "Correspondence governs targeting range and unlocks rift creation.", 5),
    ("mind", "Mind powers include heighten, scry, shield and mageinvis.", 5),
    ("forces", "Forces powers include nightsight, electricalchaos, friction and inferno.", 4),
    ("time", "Time powers include contingency and doublecast.", 4),
]
MAGE_HUNKS = [
    (
        ("\t\tstc(" if name == "mind" else "")
        + f'"{summary}\\n\\r"\n'
        + " " * indent
        + '"Use \'magepowers\' for rank-based unlocks; other spell requirements still apply.\\n\\r"\n'
        + " " * indent
        + f'"Train with \'sphere {name} learn\'.\\n\\r"',
        (" \t\tstc(" if name == "mind" else "")
        + '"This sphere will eventually do fun things.\\n\\r"',
    )
    for name, summary, indent in SPHERES
]
MAGE_HUNKS.append((
    'if (ch->spheres[MMAT] < 2) stc("no standalone rank-1 command", ch);\n'
    '        if (ch->spheres[MMAT] >= 2) stc("shatterwall", ch);',
    'if (ch->spheres[MMAT] >= 1) stc("analyze", ch);\n'
    '        if (ch->spheres[MMAT] >= 2) stc(", shatterwall", ch);',
))
MAGE_HUNKS.extend([
    ('stc(", friction", ch);', 'stc(", frictioncurse", ch);'),
    ("Matter 1: No standalone command is registered.", "Matter 1: Analyze: warlock.c"),
])
GARGOYLE_HINT = """if (IS_GARGOYLE(ch))
    stc("Train with 'discipline visceratika' (15 primal for rank 1), "
        "then use 'stoneform on'. No free rank is granted.\\n\\r", ch);
"""
PROOFS = [
    ("Genesis/src/classes/mage/mage.c",
     "667337169a49331e1ec68678faf4ba6699a573cf", MAGE_HUNKS),
    ("Genesis/src/clan.c", "6cc9fa0d03e3a87c55a622f9ef959288e8550570", [
        (GARGOYLE_HINT, ""),
        ('"You are now a #7Monk#n.\\n\\r"', '"You are now a #7Monk#n.\\nr\\r"'),
    ]),
    ("Genesis/src/rot_wiz.c", "85a30f376afd332a05e6528fd9e5fe2f8047dfff", [
        ('"You are now a Monk.\\n\\r"', '"You are now a Monk.\\nr\\r"'),
    ]),
]


def without_class_help(text):
    for _, _, hunks in PROOFS:
        for new, old in hunks:
            assert text.count(new) <= 1, "Ambiguous class-help inverse"
            text = text.replace(new, old)
    return text
