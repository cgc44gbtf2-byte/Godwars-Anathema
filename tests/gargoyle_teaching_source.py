"""Exact inverse of the Gargoyle teaching-state guard; native pins stay strict."""
PATH = "Genesis/src/classes/vampire/vampire.c"
BASE_BLOB = "d87ec7a50dccef86dc1afd42f605c0fa6eff73e4"
OLD = """    return vampire_ready(ch) && ch->position == POS_STANDING
        && ch->fighting == NULL && ch->desc != NULL
"""
NEW = """    return vampire_ready(ch) && ch->position == POS_STANDING
        && !gargoyle_statue(ch)
        && ch->fighting == NULL && ch->desc != NULL
"""

def without_gargoyle_teaching(text):
    if "static bool teaching_ready(" in text:
        assert text.count(NEW) <= 1, "Ambiguous Gargoyle teaching inverse"
        return text.replace(NEW, OLD)
    return text
