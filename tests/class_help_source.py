"""Reverse only reviewed class-help strings; preserve older gameplay pins."""
from elemental_help_source import ELEMENT_HUNKS

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
HIGHLAND_GUIDANCE = (
    '\tstc("Syntax: increase <attack/defense/weaponry/powers>\\n\\r"\n'
    '        "Training buys one rank immediately: 15 x next rank primal; maximum 6.\\n\\r"\n'
    '        "Power commands: bflash (Powers 2), bash (Powers 3), htwist (Powers 4),\\n\\r"\n'
    '        "                judge (Powers 5), lunge (Powers 6).\\n\\r"\n'
    '        "Counter: counter (Powers 4 + Defense 4); wield your own forged katana to activate.\\n\\r"\n'
    '        "Use \'counter off\' to lower your guard. Other ability restrictions still apply.\\n\\r", ch);'
)
PROOFS = [
    ("Genesis/src/classes/highlander/highland.c",
     "4d0d0c9f802b65b914571fb9999d0556a885ded3", [
         (HIGHLAND_GUIDANCE,
          '\tstc("Syntax: increase <attack/defense/weaponry/powers>",ch);'),
     ]),
    ("Genesis/src/classes/elemental/elemental.c",
     "90fa65b2627a1f25c9ef29de916427dfb33aa074", ELEMENT_HUNKS),
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
