"""Exact inverse of the native PC-death stone-form cleanup only."""
BASE = "13cd215915ef8259b06f93cdcc4818f31456a24e"
NEW = """    /* A revived PC returns to flesh before altar recovery and autosave.
     * Keep earned Visceratika/build identity and all unrelated form policy. */
    REMOVE_BIT(victim->polyaff, POLY_GARGOYLE_STATUE);
    victim->hit = 1;
"""
OLD = "    victim->hit = 1;\n"


def without_gargoyle_death(text):
    if text.count(NEW) == 1:
        return text.replace(NEW, OLD, 1)
    return text
