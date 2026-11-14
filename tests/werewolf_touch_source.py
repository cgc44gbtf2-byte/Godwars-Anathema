"""Hash-guarded inverse of the reviewed Mother's Touch arithmetic repair."""
import hashlib
import re

def without_werewolf_touch_stage(text):
    match=re.search(r"void do_mother\s*\(.*?(?=\nvoid do_|\Z)",text,re.S)
    if not match:return text
    before="29a6938b81a9a4ae3482ddb1beff0c8e1806f4d96ffc6d679b9cc52f17837c2d"
    after="03fec8e934af6b69ac9dac2caeeffc6f093f6dcbd7b220597265a5c05442d233"
    digest=hashlib.sha256(match[0].encode()).hexdigest()
    if digest==before:return text
    assert digest==after,("unexpected Mother's Touch edit",digest)
    lines=match[0].splitlines(keepends=True)
    for start,end,old in (
        (64,72,"\tvictim->hit += get_auspice(ch, AUSPICE_THEURGE) * victim->max_hit * .03;\n"),
        (52,58,""),
        (17,19,"    if (get_auspice(ch, AUSPICE_THEURGE) < 1)\n"),
        (5,6,""),
    ):
        lines[start:end]=old.splitlines(keepends=True)
    restored="".join(lines)
    assert hashlib.sha256(restored.encode()).hexdigest()==before
    text=text[:match.start()]+restored+text[match.end():]
    assert text.count("#include <limits.h>\n")==1
    return text.replace("#include <limits.h>\n","",1)
