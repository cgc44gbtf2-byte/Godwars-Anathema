"""Invert only the explicit missing-owner rejection for attunement."""

GUARD = """    if (obj->questowner == NULL || obj->questowner[0] == '\\0' ||
        str_cmp(ch->name,obj->questowner))
"""

def without_native_drow_attune_owner_guard(text):
    if "void do_attune(" not in text:
        return text
    start=text.index("void do_attune(")
    end=text.index("\nvoid do_acidbolt(",start)
    body=text[start:end]
    if GUARD not in body:
        if "obj->questowner == NULL" in body or "obj->questowner[0]" in body:
            raise AssertionError("Unreviewed Drow attunement owner guard")
        return text
    assert body.count(GUARD) == 1, "Unreviewed Drow attunement ownership edit"
    body=body.replace(GUARD, "    if (str_cmp(ch->name,obj->questowner))\n", 1)
    return text[:start]+body+text[end:]
