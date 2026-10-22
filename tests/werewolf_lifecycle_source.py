"""Invert only the reviewed legacy transform loop; reject all later edits."""
import hashlib
import re

BEFORE="75212059872dcc378184d47fc6195fbbdff9a341e49782d7196d40174d60ce30"
AFTER="474ea5275219cc3388c949cbd21acab46de3eae380245e4dd5df5eb67f66cf31"
HISTORICAL="552b3887c1109690b445e9578fa10510045f5e5811409b30b493a55106a01115"
EDITS=[
    (99,117,"\t    multi_hit(ch, vch);\n"*3),
    (97,98,"\tif ( vch->in_room == ch->in_room && can_see( ch, vch) )\n"),
    (91,92,"\t    else if (vch->in_room->area == ch->in_room->area)\n"),
    (89,90,"\t    if (vch->in_room == ch->in_room)\n"),
    (72,77,""),
    (19,25,'    if (IS_POLYAFF(ch, POLY_MASK)) do_doppleganger(ch, "self");\n'),
    (10,13,'        ch->in_room == NULL) return;\n'),
    (6,9,""),
]

def without_werewolf_lifecycle_stage(text):
    match=re.search(r"void do_werewolf\(.*?(?=\nvoid do_|\Z)",text,re.S)
    if match is None:return text
    digest=hashlib.sha256(match[0].encode()).hexdigest()
    if digest in (BEFORE,HISTORICAL):return text
    assert digest==AFTER,("unexpected legacy transform loop edit",digest)
    lines=match[0].splitlines(keepends=True)
    for start,end,old in EDITS:lines[start:end]=old.splitlines(keepends=True)
    restored="".join(lines)
    assert hashlib.sha256(restored.encode()).hexdigest()==BEFORE
    return text[:match.start()]+restored+text[match.end():]
