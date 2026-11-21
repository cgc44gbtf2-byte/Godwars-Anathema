"""Reverse only the hash-verified Shield handler; preserve every other byte."""
import hashlib
import json
from pathlib import Path

FIXTURE=Path(__file__).with_name("shield_native.json")
PIN="939f3da0b39e4f11e8c08fb9c9c060f9b4db6bcd26a30ef24256b612c094e124"
REVIEWED={
    "1610a9a8fdab8dfa23ddd8a2e61859bdedd28ae29e75ce726dda2ec1706f663b",
    "48fcc092603986573a5a4a771ed17e0128d1b8562c4460f863d6b6691c445cfe",
}

def without_shield_stage(text):
    from heighten_source import without_heighten_stage
    text=without_heighten_stage(text)
    if "void do_shield(" not in text:return text
    raw=FIXTURE.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==PIN,"Native Shield fixture changed"
    old=json.loads(raw)["old"]
    start=text.index("void do_shield(")
    end=text.index("\nvoid do_serpent(",start)
    current=text[start:end]
    if current==old:return text
    assert hashlib.sha256(current.encode()).hexdigest() in REVIEWED,"Unreviewed Shield edits"
    return text[:start]+old+text[end:]
