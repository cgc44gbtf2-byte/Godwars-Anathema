"""Exact reflection inverse, with a pinned checksum for the added C block."""
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
damage = (ROOT / "Genesis/src/combat_damage.c").read_text()
BLOCK = damage[damage.index("/* Retaliation is deferred"):damage.index("void damage(")]
assert hashlib.sha256(BLOCK.encode()).hexdigest() == (
    "41b6f6980f0bf330f215bd8b246e40dc4fa4f4712d4bed614d2cfaace1cbd9c8")

PROOFS = [
    ("Genesis/src/combat_damage.c", "a3806218afd6a8ed21f7a063f765fd066b47c92e", [
        (BLOCK, ""),
        ("    long hit_before;\n", ""),
        ("    hit_before = victim->hit > 0 ? victim->hit : 0;\n", ""),
        ("""    if (!gargoyle_dispatching_reflection && ch != victim && dam > 0 &&
        gargoyle_statue(victim) && gargoyle_visceratika(victim) == 5 &&
        victim->hit > 0 && victim->hit < hit_before)
        gargoyle_reflect_later(victim, ch, (hit_before - victim->hit) / 4);

""", ""),
    ]),
    ("Genesis/src/comm.c", "79931b4a1a1293aef2032366e107b8ab29678e92", [
        ("        combat_status_update();\n        gargoyle_reflection_pulse();\n",
         "        combat_status_update();\n"),
    ]),
    ("Genesis/src/merc.h", "d9ed947c5611d368dd61d3255595e52861fb7c47", [
        ("    unsigned long       gargoyle_reflection_id; /* Runtime only; clear_char resets it. */\n", ""),
        ("void gargoyle_reflection_pulse(void);\n", ""),
    ]),
]
MARKERS = {"combat_damage.c": "void hurt_person(",
           "comm.c": "void game_loop_", "merc.h": "#define IS_NINJA(ch)"}

def without_gargoyle_reflection(text):
    for path, _, hunks in PROOFS:
        if MARKERS[path.rsplit("/", 1)[-1]] not in text:
            continue
        for new, old in hunks:
            limit = 2 if path.endswith("/comm.c") else 1
            assert text.count(new) <= limit, "Ambiguous reflection inverse"
            text = text.replace(new, old)
    return text
