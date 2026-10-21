"""Exact inverse edits for historical native source pins, not production code.

Only the four reviewed command bodies are restored to commit 715db0f.
Both before/after SHA-256s are checked: future edits cannot silently disappear
inside normalization. Line spans refer to the fixed after-body, end-exclusive.
The real-engine suite independently checks the corrected behavior and goldens.
"""
import hashlib
import re
from werewolf_reaction_source import without_werewolf_reaction_stage

EDITS={
    "elemental": (
        "cdf1dbf588a77dc27051dd721e8edf3eb8a82e97c0a15ea0b12ffb8e086e2d0b",
        "78109199cf0272b319c48ab43aa573c52aebea02a9126af15d6b0191141cb22b",
        [
            (3,4,""),(7,12,""),
            (36,38,"    if (arg2[0] == '\\0' || (victim = get_char_room(ch, arg2)) == NULL)\n"),
            (39,41,"\tif ((victim = ch->fighting) == NULL)\n\t{\n\t    stc(\"They aren't here.\\n\\r\", ch);\n\t    return;\n\t}\n"),
            (51,52,"\tWAIT_STATE(ch, 12);\n"),
            (60,61,"        WAIT_STATE(ch, 6);\n"),
            (67,74,
             '        dam = number_range(250, 500);\n'
             '        damage(ch, victim, dam, TYPE_HIT, "boulder rain");\n'
             '        dam = number_range(250, 500);  \n'
             '        damage(ch, victim, dam, TYPE_HIT, "boulder rain");\n'
             '        dam = number_range(250, 500);  \n'
             '        damage(ch, victim, dam, TYPE_HIT, "boulder rain");\n'
             '        dam = number_range(250, 500);  \n'
             '        damage(ch, victim, dam, TYPE_HIT, "boulder rain");\n'
             '        WAIT_STATE(ch, 12);\n'),
            (82,83,"        WAIT_STATE(ch, 12);\n"),
        ]),
    "surge": (
        "7e7353da6225464c2ddeab3fc84309868ed171fea6ddfdc054a0cd0497cbf033",
        "8f4e556d3f710305662fa48a7acba9449cbc0f78c53d5ff4dfafbeff62d932fe",
        [
            (4,5,""),(6,10,"    if (IS_NPC(ch)) return;\n"),
            (26,27,"    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)\n"),
            (28,32,""),(41,42,"    WAIT_STATE(ch, 12);\n"),
        ]),
    "cutting": (
        "650a1c19024e4e9e1af88cf82c81461a555ca481b59889039d7d3f64c86608f5",
        "38d435e47f93ddc317ad3d0377e245d6da762156538625ddca7f0870290290b1",
        [
            (3,4,""),(5,10,""),
            (41,43,"    if (victim != NULL && number_range(1, (40 - get_tribe(ch, TRIBE_WENDIGOS) * 5)) == 1)\n"),
            (52,53,"    WAIT_STATE(ch, 12);\n"),
        ]),
    "invoke": (
        "82dae95d5c31daf635704cddb7de3104beead4e2e29885dc63c07c30119ba9d6",
        "f399a8d0a04e78e3c54cc562113747f5db3688d1457e176212c4c4cea43a4725",
        [
            (4,5,""),(8,13,""),
            (120,122,"\tif ((victim = ch->fighting) == NULL && (victim = get_char_room(ch, arg2)) == NULL)\n"),
            (130,137,
             "\tdam = number_range(1000, 2000) + (100 * ch->pcdata->rank);\n"
             '\tdamage(ch, victim, dam, TYPE_HIT, "lightning blast");\n'
             "\tdam = number_range(1000, 2000) + (100 * ch->pcdata->rank);\n"
             '\tdamage(ch, victim, dam, TYPE_HIT, "lightning blast");\n'
             "\tdam = number_range(1000, 2000) + (100 * ch->pcdata->rank);\n"
             '\tdamage(ch, victim, dam, TYPE_HIT, "lightning blast");\n'
             "\tch->pcdata->weather = 10;\n"),
        ]),
}

def without_werewolf_combat_stage(text):
    text=without_werewolf_reaction_stage(text)
    for name,(before,after,edits) in EDITS.items():
        match=re.search(r"void do_"+name+r"\(.*?(?=\nvoid do_|\Z)",text,re.S)
        if not match:continue
        current=match[0]
        digest=hashlib.sha256(current.encode()).hexdigest()
        if digest==before:continue
        assert digest==after,("unexpected Werewolf command edit",name,digest)
        lines=current.splitlines(keepends=True)
        for start,end,old in reversed(edits):lines[start:end]=old.splitlines(keepends=True)
        restored="".join(lines)
        assert hashlib.sha256(restored.encode()).hexdigest()==before,name
        text=text[:match.start()]+restored+text[match.end():]
    return text
