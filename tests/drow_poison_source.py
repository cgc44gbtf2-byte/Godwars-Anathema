"""Reverse only the two exact reviewed Drow poison-immunity predicates."""
import hashlib
from native_drow_source import without_native_drow_shoot_context

PINS=(
    ("void spell_poison(","void spell_protection(",
     "0d6628b2f988d48388f2b895b1d9730c7d101328bfab00841df79282b2e2f437",
     "ffd27a44ea7cba889ae33344b210bb64cf55605c1e97073e736cb5993a3f1fd5",
     "else if (hybrid_power_drow(victim, DROW_PIMM)) return;",
     "else if (get_drow(victim, DROW_PIMM)) return;"),
    ("void do_shoot(",None,
     "324e43b8c1c5ec6cb50b1f6c351980f98bcc112f9d615ae4168dc7fe73544d00",
     "744cfc1f9208118b98586da1bb87ea9b2082072c2cfa6a3dd75fe121b1c0a32b",
     "if (hybrid_power_drow(victim, DROW_PIMM))",
     "if (IS_DROW(victim) && get_drow(victim, DROW_PIMM))"),
)

def without_drow_poison_stage(text):
    text=without_native_drow_shoot_context(text)
    for start,end,old,new,grant,original in PINS:
        if start not in text:continue
        a=text.index(start);b=text.index(end,a) if end else len(text)
        body=text[a:b];digest=hashlib.sha256(body.encode()).hexdigest()
        if grant not in body:
            assert digest==old,("Unreviewed Drow immunity edits",digest)
            continue
        assert digest==new,("Unreviewed Drow immunity edits",digest)
        assert body.count(grant)==1
        restored=body.replace(grant,original,1)
        assert hashlib.sha256(restored.encode()).hexdigest()==old
        text=text[:a]+restored+text[b:]
        if start=="void spell_poison(":
            assert text.count('#include "classes/hybrid/access.h"\n')==1
            text=text.replace('#include "classes/hybrid/access.h"\n',"",1)
    return text
