"""Reverse exactly the two reviewed Blind Fighting grant predicates."""
import hashlib

OLD="1172cc7e85c343b6a37af949547062dfe1736d1865c6d635f57fdb594a80ef3e"
NEW="db8206d66ca8dfd6c964cde0b28bd5f8fc7bb54ab538ccc8472e092ba8d5ea2c"

def without_drow_blindfight_stage(text):
    from native_drow_source import without_native_drow_speed
    text=without_native_drow_speed(text)
    a=text.index("bool check_avoid(");b=text.index("long avoidblow(",a)
    body=text[a:b]
    digest=hashlib.sha256(body.encode()).hexdigest()
    if "hybrid_power_drow" not in body:
        assert digest==OLD,("Unreviewed Blind Fighting edits",digest)
        return text
    assert digest==NEW,("Unreviewed Blind Fighting edits",digest)
    for actor in ("victim","ch"):
        grant=f"hybrid_power_drow({actor}, DROW_BFIGHT)"
        assert body.count(grant)==1
        body=body.replace(grant,f"get_drow({actor}, DROW_BFIGHT)",1)
    assert hashlib.sha256(body.encode()).hexdigest()==OLD
    assert text.count('#include "classes/hybrid/access.h"\n')==1
    return (text[:a]+body+text[b:]).replace('#include "classes/hybrid/access.h"\n',"",1)
