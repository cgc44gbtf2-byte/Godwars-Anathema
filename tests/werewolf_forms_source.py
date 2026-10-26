"""Hash-guarded inverse of only the Wyrm/Song caller safety changes."""
import hashlib
import re
from werewolf_touch_source import without_werewolf_touch_stage

EDITS={
    "wyrmcall":(
        "0f06c15d5726aa9d1d6a64c5ba2e200f56045fc92ccfec872716143ae8a9a3f0",
        "6cd9ed841abb41bdfba5dfed7b2299029a9309f5d90337cb65c1c34536b543f6",
        [(68,70,"    WAIT_STATE(ch,8);\n"),(66,67,""),
         (46,61,'\t    if (IS_ITEMAFF(ich,ITEMA_PEACE))\tdo_majesty(ich, "");\n'
                '\t    if (IS_POLYAFF(ich,POLY_SHADOW))\tdo_shadowbody(ich, "");\n'
                '\t    if (IS_POLYAFF(ich,POLY_MIST))\tdo_change(ich, "human");\n'),
         (31,32,""),(28,30,""),
         (26,27,"    for (ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room)\n"),
         (3,10,"")]),
    "songrage":(
        "1284f649be2668e462a91a4bd7ed25bb905a3e9446ee8eae9ace9e1e4f6449e2",
        "b402461067ea1fedfc16ed6f99280b06e2a6bc88896c868ee829d16fefa3489e",
        [(70,72,"    WAIT_STATE(ch,30);\n"),
         (60,63,"\t    victim->pcdata->wolf += number_range(10,20);\n"),
         (57,58,""),(39,41,""),(4,10,"")]),
}

def without_werewolf_forms_stage(text):
    from drow_globe_source import without_drow_globe_stage
    text=without_drow_globe_stage(text)
    for name,(before,after,edits) in EDITS.items():
        match=re.search(r"void do_"+name+r"\s*\(.*?(?=\nvoid do_|\Z)",text,re.S)
        if not match:continue
        digest=hashlib.sha256(match[0].encode()).hexdigest()
        if digest==before:continue
        assert digest==after,("unexpected Werewolf form caller edit",name,digest)
        lines=match[0].splitlines(keepends=True)
        for start,end,old in edits:
            lines[start:end]=old.splitlines(keepends=True)
        restored="".join(lines)
        assert hashlib.sha256(restored.encode()).hexdigest()==before,name
        text=text[:match.start()]+restored+text[match.end():]
    return without_werewolf_touch_stage(text)
