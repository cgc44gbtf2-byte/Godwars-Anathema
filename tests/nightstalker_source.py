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
