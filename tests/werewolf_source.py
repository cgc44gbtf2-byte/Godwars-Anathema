"""Strictly reverse this reviewed Werewolf change for older source pins.

Each changed function has an exact current SHA and its original text. This
does not discard a trainer, predicate, or an arbitrary neighboring function.
"""
import hashlib
import json
import re
from pathlib import Path

DATA=Path(__file__).with_name("werewolf_native_pins.json")
PIN="91172065423dcd4c759d1c8587d89d0cb6c2c1180246ae007224a86c3ec610e2"
FUNCTION=re.compile(
    r"^(?:void|char\s*\*|enum hybrid_lesson_result)\s*(\w+)\s*\(",re.M)

def spans(text):
    matches=list(FUNCTION.finditer(text))
    return {match[1]:(match.start(),matches[i+1].start()
                     if i+1<len(matches) else len(text))
            for i,match in enumerate(matches)}

def without_werewolf_stage(text):
    from werewolf_lifecycle_source import without_werewolf_lifecycle_stage
    text=without_werewolf_lifecycle_stage(text)
    raw=DATA.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==PIN,"Werewolf proof fixture changed"
    for proof in json.loads(raw):
        bounds=spans(text).get(proof["name"])
        if bounds is None:
            continue
        start,end=bounds
        body=text[start:end]
        if body==proof["old"]:
            continue
        assert hashlib.sha256(body.encode()).hexdigest()==proof["new_sha256"],proof["name"]
        text=text[:start]+proof["old"]+text[end:]
    removes=[]
    if "void do_advance(" in text:
        removes+=['#include "advancement.h"\n','#include "classes/hybrid/player.h"\n']
    if "void do_rage(" in text:
        removes+=['#include <string.h>\n','#include <limits.h>\n']
    if "void do_werewolf(" in text:
        removes+=['#include <limits.h>\n']
    removes+=['#include "../werewolf/advancement.h"\n',
              '   { "shift", do_shift, POS_FIGHTING, LEV_AWA, LOG_NORMAL },\n',
              'DECLARE_DO_FUN( do_shift );\n']
    for fragment in removes:
        if fragment in text:
            assert text.count(fragment)==1
            text=text.replace(fragment,"",1)
    return text
