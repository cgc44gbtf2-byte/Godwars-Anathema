"""Invert only the reviewed pre-allocation summon-stat rejection."""

DECLARATION = "    long spider_hitroll, spider_damroll, spider_armor;\n"
GUARD = """    /* Reject unrepresentable summon stats before allocating a follower. */
    spider_hitroll = char_hitroll(ch);
    spider_damroll = char_damroll(ch);
    spider_armor = char_ac(ch);
    if (spider_hitroll < LONG_MIN + 50 ||
        spider_damroll < LONG_MIN + 50 ||
        spider_armor < LONG_MIN + 50)
    {
        stc("Your stats cannot safely summon a spider. Please tell staff.\\n\\r",ch);
        return;
    }
"""

def without_native_drow_spider_stats(text):
    from native_drow_attune_source import without_native_drow_attune_owner_guard
    text = without_native_drow_attune_owner_guard(text)
    if "void do_summonspider(" not in text:
        return text
    start = text.index("void do_summonspider(")
    end = text.index("\n/* Consumable recipe", start)
    body = text[start:end]
    if "spider_hitroll" not in body and "/* Reject unrepresentable summon stats" not in body:
        return text
    for new, old in (
        (DECLARATION, ""),
        (GUARD, ""),
        ("victim->hitroll = spider_hitroll - 50;", "victim->hitroll = char_hitroll(ch) - 50;"),
        ("victim->damroll = spider_damroll - 50;", "victim->damroll = char_damroll(ch) - 50;"),
        ("victim->armor = spider_armor - 50;", "victim->armor = char_ac(ch) - 50;"),
    ):
        assert body.count(new) == 1, ("Unreviewed spider stat edit", new)
        body = body.replace(new, old, 1)
    return text[:start] + body + text[end:]
