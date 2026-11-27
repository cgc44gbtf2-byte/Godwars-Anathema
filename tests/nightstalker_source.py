"""Reverse exact reviewed Nightstalker edits, retaining older whole-source pins.

The compressed fixture contains only [path, prior Git blob, new/old hunks].
Other changes are left visible to the historical hashes. New tests separately
check all restored blobs and reject an unrelated neighboring edit.
"""
import base64
import json
import zlib

DATA = """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"""
PROOFS = json.loads(zlib.decompress(base64.b64decode(DATA)))
for entry in PROOFS:
    if entry[0] == "Genesis/src/clan.c":
        assert entry[1] == "de56f64e972769f46ae56793d1d2830aa868d0f4"
        entry[1] = "20f72db248df402ff505a3c23e115ba39361d8ab"
for path, _, hunks in PROOFS:
    if path.endswith("/clans.c"):
        hunks.extend([
            ("""        if (ch->class != registry.members[m].class_id ||
            (ch->class == CLASS_NINJA &&
             IS_NIGHTSTALKER(ch) !=
             !strcmp(recruit_recipes[registry.members[m].recruit_recipe - 1].command,
                     "ninja nightstalker"))) {
""", """        if (ch->class != registry.members[m].class_id) {
"""),
            ("""        ch->class != registry.members[m].class_id ||
        (ch->class == CLASS_NINJA &&
         IS_NIGHTSTALKER(ch) !=
         !strcmp(recruit_recipes[registry.members[m].recruit_recipe - 1].command,
                 "ninja nightstalker"))) {
""", """        ch->class != registry.members[m].class_id) {
"""),
        ])
MARKERS = {
    "clan.c": "void do_autoclass(",
    "clans.c": "static bool recruit_project(",
    "ninja.c": "void do_principles(",
    "handler.c": "bool can_see(",
    "merc.h": "#define IS_NINJA(ch)",
    "update.c": "void mobile_update(",
}

def without_nightstalker_stage(text):
    from native_drow_source import without_native_drow_spider_expiry
    text = without_native_drow_spider_expiry(text)
    from gargoyle_source import without_gargoyle_stage
    text = without_gargoyle_stage(text)
    for path, _, hunks in PROOFS:
        if MARKERS[path.rsplit("/", 1)[-1]] not in text:
            continue
        for new, old in hunks:
            assert text.count(new) <= 1, "Ambiguous Nightstalker inverse"
            if new in text:
                text = text.replace(new, old, 1)
    return text
