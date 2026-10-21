"""Hash-guarded inverse of only the two reviewed Galliard command bodies."""
import hashlib
import re
from werewolf_touch_source import without_werewolf_touch_stage

EDITS={
    "wyld": (
        "aacff57b8a86ec2f797c83dedefda7e80688ea0b006ebc80cd89e8bc984961b9",
        "d810971e7e271b2174f569ca246de015857f1f0a7e975daad9886c8c08b65f88", [
        (7,10,'    if (IS_NPC(ch)) return;\n\n'
            '    if (!IS_WEREWOLF(ch) && ch->in_room == NULL)\n'
            '    {\n\tsend_to_char("Huh?\\n\\r",ch);\n\treturn;\n    }\n'),
        (17,21,"    if ( ( victim = ch->fighting ) == NULL && ch->hit > 0 )\n"),
        (23,24,"\treturn;\n"),(25,35,""),
        (41,42,"\tif ( IS_NPC(vch) ) continue;\n"),
        (46,46,'\t    if (victim != NULL)\n\t    {\n'
            '\t\tsprintf(buf,"CALL OF THE WYLD:[$n is under attack by %s at %s]", \n'
            '\t\t    IS_NPC(victim) ? victim->short_descr : victim->name,\n'
            '                    ch->in_room->name);\n'),
        (47,47,'\t    }\n\t    else if (ch->hit < 1)\n\t    {\n'
            '\t\tsprintf(buf,"CALL OF THE WYLD:[$n is badly wounded at %s]", \n'
            '                    ch->in_room->name);\n'
            '\t\tact(buf,ch,NULL,vch, TO_VICT);\n\t    }\n\t    else\n\t    {\n'
            '\t\tsprintf(buf,"CALL OF THE WYLD:[$n needs assistance at %s]", \n'
            '                    ch->in_room->name);\n'
            '\t\tact(buf,ch,NULL,vch, TO_VICT);\n\t    }\n'),
    ]),
    "bridgewalk": (
        "20822473c3b22fc576bb0aebd5cfe2605b0355ccd85f9bf12baee0c0b5050c56",
        "afca1d9a324f28b6994c5453e1db9b66695e63a7d5e51894e87b08e45bcd50a7", [
        (6,7,""),(8,16,""),
        (27,28,'        stc("You require Ragabash 5 to use Bridge Walker.\\n\\r",ch);\n'),
        (35,35,'    if (IS_ITEMAFF(ch,ITEMA_BLISSFUL))\n\tdo_blissful(ch,"");\n\n'),
        (43,46,'\tstc( "A random moon fluctuation disrupts your bridge!\\n\\r", ch );\n'
            '\tfor ( ; ; )\n'),
        (54,72,""),(73,75,""),(85,86,""),
    ]),
}

def without_werewolf_galliard_stage(text):
    text=without_werewolf_touch_stage(text)
    for name,(before,after,edits) in EDITS.items():
        match=re.search(r"void do_"+name+r"\s*\(.*?(?=\nvoid do_|\Z)",text,re.S)
        if not match:continue
        digest=hashlib.sha256(match[0].encode()).hexdigest()
        if digest==before:continue
        assert digest==after,("unexpected Galliard edit",name,digest)
        lines=match[0].splitlines(keepends=True)
        for start,end,old in reversed(edits):lines[start:end]=old.splitlines(keepends=True)
        restored="".join(lines)
        assert hashlib.sha256(restored.encode()).hexdigest()==before,name
        text=text[:match.start()]+restored+text[match.end():]
    return text
