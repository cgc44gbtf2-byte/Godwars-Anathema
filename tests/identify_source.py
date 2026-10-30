"""Mask only the independently tested identify-output scope for older pins."""
def without_identify_stage(text):
    from drow_poison_source import without_drow_poison_stage
    text = without_drow_poison_stage(text)
    marker = "/* Identification helpers are deliberately local;"
    start = text.index(marker) if marker in text else text.index("void spell_identify(")
    end = text.index("void spell_infravision(", start)
    return (text[:start] + "/* identify output scope */\n\n" + text[end:]).replace(
        "#include <stdarg.h>\n", "", 1)
