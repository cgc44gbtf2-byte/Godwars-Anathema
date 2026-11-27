"""Exact reviewed Elementalist menu edits, never spell implementations."""

# Element, previous displayed rank, actual required rank, old prose, command.
ELEMENT_HELP = [
    ("FIRE", 1, 1, "Flare Arrow    - Launches flaming arrows", "flarearrow"),
    ("FIRE", 8, 8, "Fire Blade     - Ignites your weapons", "fireblade"),
    ("FIRE", 10, 10, "Phoenix Form   - Turn into a Phoenix", "phoenixform"),
    ("WATER", 1, 1, "Icicle Storm  - Conjures a barrage of icicles", "iciclestorm"),
    ("WATER", 5, 5, "Water Travel  - Allows quick travel", "watertravel"),
    ("WATER", 8, 8, "Aqua Barrier  - Surrounds you with a barrier of water", "aquabarrier"),
    ("WATER", 10, 10, "Water Control - Stuns opponents", "watercontrol"),
    ("WIND", 1, 1, "Wind Wings     - Lifts you into the air", "windwings"),
    ("WIND", 8, 8, "Wind Blades    - Slashes with blades of wind", "windblades"),
    ("WIND", 10, 10, "Atoning Winds  - Slows opponents", "atoningwinds"),
    ("EARTH", 1, 1, "Local Quake   - Shakes the earth", "localquake"),
    ("EARTH", 8, 8, "Stone Blast   - Blasts everything", "stoneblast"),
    ("EARTH", 10, 10, "Liquify Stone - Turns the land into quicksand", "liquifyearth"),
    ("SUN", 1, 3, "Sun Bath        - Showers target with healing light", "sunbath"),
    ("SUN", 5, 5, "Holy Aura       - Surrounds you with a holy aura", "holyaura"),
    ("SUN", 8, 8, "Solar Flash     - Blinds opponents", "solarflash"),
    ("SUN", 10, 10, "Solar Light     - Grants you a halo of sunlight", "solarlight"),
    ("MOON", 1, 3, "Mirage Slice   - Summons an illusionary double to attack", "mirageslice"),
    ("MOON", 8, 8, "Night Blade    - Life stealing double attack", "nightblade"),
    ("MOON", 10, 10, "Spiral Moon    - Weakens with dark energy", "spiralmoon"),
    ("TIME", 10, 10, "Slow           - Slows target", "slow"),
    ("INFIN", 1, 2, "Far Sight      - Watches from afar", "farsight"),
    ("INFIN", 3, 3, "Cloak of Sight - Hides you at a distance", "cloakofsight"),
    ("INFIN", 5, 5, "Infinite Sight - Opens your eyes to everything", "infinitesight"),
    ("INFIN", 10, 10, "Ultima Wave    - Creates a wave of destruction", "ultimawave"),
]

ELEMENT_HUNKS = [
    (
        f'if (ch->pcdata->powers[ELEM_{element}] >= {rank}) '
        f'stc("{rank:<2}: {prose} ({command}).\\n\\r"',
        f'if (ch->pcdata->powers[ELEM_{element}] >= {old_rank}) '
        f'stc("{old_rank:<2}: {prose}.\\n\\r"',
    )
    for element, old_rank, rank, prose, command in ELEMENT_HELP
]
