"""Exact inverse of the native Gargoyle recruitment-state guard."""
PATH = "Genesis/src/clans.c"
BASE_BLOB = "f5c8384be70d74b6568edb06cedfb8f0e16589d3"
OLD = """    return player_ready(sponsor) && session_ready(student) && sponsor != student &&
        student->class == CLASS_NONE && !student->pcdata->hybrid_present &&
"""
NEW = """    return player_ready(sponsor) && session_ready(student) && sponsor != student &&
        !gargoyle_statue(sponsor) &&
        student->class == CLASS_NONE && !student->pcdata->hybrid_present &&
"""

def without_gargoyle_recruit(text):
    if "static bool recruit_together(" in text:
        assert text.count(NEW) <= 1, "Ambiguous Gargoyle recruitment inverse"
        return text.replace(NEW, OLD)
    return text
