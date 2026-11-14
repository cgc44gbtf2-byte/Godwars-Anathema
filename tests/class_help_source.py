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
NINJA_GUIDANCE = (
    'stc("View powers: principle <sora/chikyu/ningenno>\\n\\r"\n'
    '    "Train: principle <sora/chikyu/ningenno> improve\\n\\r"\n'
    '    "Training buys one rank immediately: 10 x next rank primal; maximum 10.\\n\\r", ch);\n'
)
SORCERER_GUIDANCE = (
    '\tstc("View ranks: colors (free).\\n\\r"\n'
    '    "Train: colors <purple/red/blue/green/yellow/black>\\n\\r"\n'
    '    "Training buys one rank immediately: 10 x next rank primal; maximum 5.\\n\\r", ch);'
)
DRAGON_GUIDANCE = (
    '\tstc("View eligible choices: dragonskill (free).\\n\\r"\n'
    '    "Buy a skill: dragonskill <skill> (DP; required age applies).\\n\\r"\n'
    '    "Buy the next age: dragonskill age (DP/QP).\\n\\r"\n'
    '    "Purchases spend immediately; learning a skill does not activate it.\\n\\r"\n'
    '    "Skill DP: fangs/nightsight 1000; claws/horns 2000; wings/breath 3000;\\n\\r"\n'
    '    "tail 4000; truesight 5000; dust 6000; magic/scry 7000;\\n\\r"\n'
    '    "readaura 8000; shield 9000. Known skills cannot be bought twice.\\n\\r"\n'
    '    "Age DP/QP by current age: Hatchling 2500/0; Very Young 5000/0;\\n\\r"\n'
    '    "Young 7500/0; Juvenile 10000/0; Young Adult 15000/500;\\n\\r"\n'
    '    "Adult 25000/1000; Mature Adult 35000/1500; Old 50000/2000;\\n\\r"\n'
    '    "Very Old 70000/2500; Ancient 100000/3000. Age purchases stop at Wyrm.\\n\\r"\n'
    '    "Eligible choices:\\n\\r", ch);'
)
MONK_GUIDANCE = (
    'stc("View: monklearn <mantra/art/tech> (free).\\n\\r"\n'
    '    "Train: monklearn <mantra/art/tech> gain.\\n\\r"\n'
    '    "Training buys one rank immediately: 10 x next rank primal; maximum 8.\\n\\r", ch);'
)
VAMPIRE_GUIDANCE = (
    'stc("Viewing is free; discipline <name> buys one rank immediately.\\n\\r"\n'
    '    "In-clan: 5 x next rank primal (max 7); other unlocked disciplines: 15 x (max 5).\\n\\r"\n'
    '    "Caitiff/Panders: 5 x next rank primal (max 6).\\n\\r"\n'
    '    "An unlocked, untrained discipline starts at rank 1 for 5/15 primal, not a free rank.\\n\\r"\n'
    '    "Use powers to list trained abilities; training does not activate them.\\n\\r", ch);\n'
    'stc(IS_GARGOYLE(ch) ? "Visceratika always has a maximum rank of 5.\\n\\r" :\n'
    '    "Thaumaturgy always has a maximum rank of 5.\\n\\r", ch);\n'
)
WEREWOLF_VIEW_GUIDANCE = (
    '    stc("Viewing gifts is free. Use gifts to see ratings, prices, and train.\\n\\r", ch);'
)
WEREWOLF_GIFT_GUIDANCE = (
    'stc("Your selected breed/auspice/tribe gifts cost 10 x next rank primal (max 5);\\n\\r"\n'
    '    "other gifts cost 15 x next rank primal (max 4). Each purchase trains one rank immediately.\\n\\r", ch);'
)
PROOFS = [
    ("Genesis/src/classes/werewolf/werewolf.c",
     "36e08820873319e226c52a26f7fbf98dc3ae9a50", [
         ('if (!any_powers) stc("You have no gifts.\\n\\r", ch);\n'
          + WEREWOLF_VIEW_GUIDANCE,
          'if (!any_powers) stc("You have no gifts.\\n\\r", ch);'),
         ('stc("To improve a gift rating, type: gifts <name>\\n\\r", ch);\n'
          + WEREWOLF_GIFT_GUIDANCE,
          'stc("To improve a gift rating, type: gifts <name>\\n\\r", ch);'),
     ]),
    ("Genesis/src/classes/monk/monk.c",
     "051fe16da87e9b7647c819512e833893c22762b0", [
         (MONK_GUIDANCE,
          '\tstc("Syntax: Display: Monklearn <mantra/art/tech>.\\n\\r", ch);\n'
          '\tstc("        Improve: Monklearn <mantra/art/tech> gain.\\n\\r", ch);'),
     ]),
    ("Genesis/src/classes/dragon/dragon.c",
     "d49760816c58db4303b9bf7200e2b52fa4b87016", [
         (DRAGON_GUIDANCE, '\tstc("Learn which dragon skill:\\n\\r", ch);'),
     ]),
    ("Genesis/src/classes/sorcerer/sorcerer.c",
     "e44a6b703dbf47456776c2207ac4ee70ff7e8956", [
         (SORCERER_GUIDANCE, '\tstc("To improve type \\"colors <color>\\".\\n\\r", ch);'),
     ]),
    ("Genesis/src/classes/ninja/ninja.c",
     "7425898de585ebbac6d4500cf861f6c40cf721c3", [
         (NINJA_GUIDANCE, ""),
         ("To improve, type: principle <sora/chikyu/ningenno> improve.",
          "To improve, type: prin <sora/chikyu/ningenno> improve."),
     ]),
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
        (VAMPIRE_GUIDANCE, ""),
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
