"""Invert only the exact reviewed secondary Drow Spider Eyes Scry adapter."""
import hashlib

OLD="70576485a2c3c78d0b67f575ed4417566e9c5d0c212796aa76489327e6d650b4"
FINAL="2bcfcbbd1edbf5ecda673c2c2ab701944df2633865fbd8f94a23e9b28f5c5223"
SAFETY="ac6431d9087a2af41914d8975312d2f14f9904731a45f378b48efb99d14cdb03"
PRE_SCRY="b21b4b23c0b3c5aaaf78ebaae51569b0fce9b4c57aa7313aa3fc24b26e809b57"
GRANT="""    /* Spider Eyes grants the same use, not extra range or shield bypass. */
    const bool drow_ready = hybrid_power_drow(ch, DROW_EYES);
    const bool free_scry = monk_ready || drow_ready;
"""

def without_drow_scry_stage(text):
    if "void do_scry(" not in text:return text
    start=text.index("void do_scry(")
    end=text.index("\nvoid do_readaura(",start)
    body=text[start:end]
    digest=hashlib.sha256(body.encode()).hexdigest()
    # Older strict normalizers may have already removed the Monk/Scry stage.
    if GRANT not in body:
        assert digest in (OLD,SAFETY,PRE_SCRY),("Unreviewed Drow Scry edits",digest)
        return text
    assert digest==FINAL,("Unreviewed Drow Scry edits",digest)
    assert body.count(GRANT)==1 and body.count("!free_scry")==3
    restored=body.replace(GRANT,"").replace("!free_scry","!monk_ready")
    assert hashlib.sha256(restored.encode()).hexdigest()==OLD,"Drow Scry inverse drift"
    return text[:start]+restored+text[end:]
