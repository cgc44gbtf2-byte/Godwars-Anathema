"""Exact inverse of Gargoyle menu-only changes; old module pins stay intact."""
PROOFS = [
    ("Genesis/src/clan.c", "595bece6019df85626678fb7c40637b9d2aca531", [
        ("""    if (gargoyle_visceratika(ch) > 0) {
        long rank = gargoyle_visceratika(ch);
        any_powers = any_commands = TRUE;
        stc("Visceratika: Camouflage, Stoneform [on|off|status].\\n\\r", ch);
        snprintf(buf, sizeof(buf),
            "  Stone-form passives: %ld%% damage reduction.\\n\\r", rank * 10);
        stc(buf, ch);
        if (rank >= 4)
            stc("  Punch resistance (stone form only).\\n\\r", ch);
        if (rank == 5)
            stc("  25% reflected HP damage (surviving stone-form hits; next pulse).\\n\\r", ch);
    }
""", ""),
    ]),
    ("Genesis/src/interp.c", "aeca1aa46fd952bac72a5b89c273e55b65cd5b80", [
        ('             str_cmp(name, "powers") &&\n', ""),
    ]),
]

def without_gargoyle_menu(text):
    from web_source import without_web_safety
    text = without_web_safety(text)
    for path, _, hunks in PROOFS:
        marker = "void do_powers(" if path.endswith("/clan.c") else "void interpret("
        # The native source places spaces between function names and '('.
        if marker not in text and marker.replace("(", " (") not in text:
            continue
        for new, old in hunks:
            assert text.count(new) <= 1, "Ambiguous Gargoyle menu inverse"
            text = text.replace(new, old)
    return text
