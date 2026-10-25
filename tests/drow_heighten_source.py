"""Undo only the exact reviewed secondary Drow Heighten access adapter."""
import hashlib

OLD="7e94f7e3ce428fadbaf61b37656d178d687b81c64fc4062d9aeed4e8c6a8387d"
FINAL="8752e31ae3ec8d2746a7be96ca6dc2ebcb5da77f1bef417282b674c8a84c905f"
GRANT="""    /* Independent learned sources share one toggle and one native wait. */
    const bool drow_ready = hybrid_power_drow(ch, DROW_SIGHT);
    const bool heighten_ready = monk_ready || drow_ready;
"""

def without_drow_heighten_stage(text):
    if "void do_truesight(" not in text:return text
    start=text.index("void do_truesight(")
    end=text.index("\nvoid do_soulmask(",start)
    body=text[start:end]
    digest=hashlib.sha256(body.encode()).hexdigest()
    # Older strict normalizers may already have removed the Monk adapter.
    from heighten_source import OLD_SHA,SAFETY_SHA
    if digest in (OLD,OLD_SHA,SAFETY_SHA):return text
    assert digest==FINAL,("Unreviewed Drow Heighten edits",digest)
    assert body.count(GRANT)==1 and body.count("!heighten_ready && ")==9
    restored=body.replace(GRANT,"").replace("!heighten_ready && ","!monk_ready && ")
    assert hashlib.sha256(restored.encode()).hexdigest()==OLD,"Drow Heighten inverse drift"
    return text[:start]+restored+text[end:]
