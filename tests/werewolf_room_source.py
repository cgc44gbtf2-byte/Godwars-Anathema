"""Exact hash-guarded inverse of three ce98690 room-command repairs."""
import hashlib
import re

EDITS={
    "breathwyld": (
        "723ce1a5c8231bbc03aa9e95a7a9c24405f658a3bfe20c880fc8b990c870c06d",
        "e87d1e65ad3bfc38c427fcc4424758df1927125fc7f093321f6bffe6423a88be", [
        (4,5,"    int heal;\n"),
        (6,12,"    if (IS_NPC(ch)) return;\n"),
        (37,38,"\tvch->hit += heal;\n"),
    ]),
    "banshee": (
        "5ad595b2580763188583eab8e0bc3488e4ee291320f04db8dd6a2aee948cfd80",
        "7db8ec5015fb7ca91bee5cdec52be48e41e7c50e287ae26761faa3b8170f6a45", [
        (4,6,""),(8,12,"    if (IS_NPC(ch)) return;\n"),
        (40,41,"    for (ich = ch->in_room->people; ich != NULL; ich = fch)\n"),
        (42,45,""),(54,58,""),
        (61,68,"    if (follow) SET_BIT(ch->act, PLR_AUTOFOLLOW);\n    WAIT_STATE(ch, 18);\n"),
    ]),
    "obedience": (
        "0f57e7fbe55ab8f51e15cfdc6a75bfaad08d0ab040f4e94039faf018ebaa43b8",
        "77de2e703f00830adecc5cc2af9b5e84eb1f4ad72cfb19edb7d74afd82c66a11", [
        (7,9,""),(11,19,""),
        (51,52,'\tsprintf( buf, "I command everyone to %s", action );\n'),
        (54,55,"\tfor ( victim = ch->in_room->people; victim != NULL; victim = vch_next )\n"),
        (56,59,""),
        (108,118,"\t    if (IS_NPC(victim)) REMOVE_BIT(victim->act, ACT_COMMANDED);\n"),
        (157,158,'\tsprintf( buf, "I command %s to %s", victim->short_descr, action );\n'),
        (159,160,'\tsprintf( buf, "I command %s to %s", victim->morph, action );\n'),
        (161,162,'\tsprintf( buf, "I command %s to %s", victim->name, action );\n'),
        (210,218,"    if (IS_NPC(victim)) REMOVE_BIT(victim->act, ACT_COMMANDED);\n"),
    ]),
}

def without_werewolf_room_stage(text):
    from gargoyle_source import without_gargoyle_stage
    text = without_gargoyle_stage(text)
    changed=False
    for name,(before,after,edits) in EDITS.items():
        match=re.search(r"void do_"+name+r"\(.*?(?=\nvoid do_|\Z)",text,re.S)
        if not match:continue
        digest=hashlib.sha256(match[0].encode()).hexdigest()
        if digest==before:continue
        assert digest==after,("unexpected Werewolf room edit",name,digest)
        lines=match[0].splitlines(keepends=True)
        for start,end,old in reversed(edits):lines[start:end]=old.splitlines(keepends=True)
        restored="".join(lines)
        assert hashlib.sha256(restored.encode()).hexdigest()==before,name
        text=text[:match.start()]+restored+text[match.end():]
        changed=True
    if changed:
        assert text.count("#include <limits.h>\n")==1
        text=text.replace("#include <limits.h>\n","",1)
    return text
