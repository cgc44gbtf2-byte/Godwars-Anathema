"""Hash-guarded inverse of the two reviewed 8658ffd Werewolf reaction edits."""
import hashlib
import re

EDITS={
    "bodywrack":(
        "b29c8171422a5ad322449eef16b25863fd9c44f9e7e42af80b9d1c0b90d24839",
        "f3b1ae3fdba59c8a1b7d95c3e84266edb50aed4442a64fafbc0f48a826263205",
        [(5,11,""),(41,42,""),(73,74,"\tWAIT_STATE( ch, 12 );\n"),
         (78,79,"    WAIT_STATE( ch, 12 );\n")]),
    "clap":(
        "035d420e45490c364d577b15088d4c155034674f3a8b6b0753fec6b186153d03",
        "d0d42c30f0774d7f4376e38e6f6a748327b4655a2c9376393599565637278077",
        [(7,14,""),
         (28,29,"\tfor ( victim = ch->in_room->people; victim; victim = victim_next )\n"),
         (30,33,""),(36,37,""),
         (73,74,'\t\t    send_to_char("You fall to the ground, stunned!\\n\\r",ch);\n'),
         (79,80,"\tWAIT_STATE( ch, 24 );\n")]),
}

def without_werewolf_reaction_stage(text):
    for name,(before,after,edits) in EDITS.items():
        match=re.search(r"void do_"+name+r"\(.*?(?=\nvoid do_|\Z)",text,re.S)
        if not match:continue
        digest=hashlib.sha256(match[0].encode()).hexdigest()
        if digest==before:continue
        assert digest==after,("unexpected Werewolf reaction edit",name,digest)
        lines=match[0].splitlines(keepends=True)
        for start,end,old in reversed(edits):lines[start:end]=old.splitlines(keepends=True)
        restored="".join(lines)
        assert hashlib.sha256(restored.encode()).hexdigest()==before,name
        text=text[:match.start()]+restored+text[match.end():]
    return text
