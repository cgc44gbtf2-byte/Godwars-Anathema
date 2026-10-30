"""Invert only the exact reviewed secondary Drow Read Aura access change."""
import hashlib

OLD="c7538786daa4c270d6c6a1af4be56059d5951262871cda18b71e23e9a84f4b98"
FINAL="fa55ab38606cef6765b09bd55cbff955193f4f99cb2fc41f6a0d0a0028bf9107"
PRE_MONK="49b8c52188e12a1d70d4bcb875bf7ada577c666089cfc527c2545ae16a6bf8cb"
GRANT="""    /* Each learned source supplies the same free reading, not a second use. */
    const bool drow_ready = hybrid_power_drow(ch, DROW_READ);
    const bool free_aura = monk_ready || drow_ready;
"""

def without_drow_aura_stage(text):
    from drow_heighten_source import without_drow_heighten_stage
    text=without_drow_heighten_stage(text)
    if "void do_readaura(" not in text:return text
    start=text.index("void do_readaura(")
    end=text.index("\nvoid do_shield(",start)
    body=text[start:end]
    digest=hashlib.sha256(body.encode()).hexdigest()
    if digest in (OLD,PRE_MONK):return text
    assert digest==FINAL,("Unreviewed Drow aura edits",digest)
    assert body.count(GRANT)==1 and body.count("!free_aura")==11
    restored=body.replace(GRANT,"").replace("!free_aura","!monk_ready")
    assert hashlib.sha256(restored.encode()).hexdigest()==OLD,"Drow aura inverse drift"
    return text[:start]+restored+text[end:]
