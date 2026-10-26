"""Undo only the reviewed learned-Drow gate in the shared night-sight handler."""

GRANT = """    /* A learned secondary grant supplies this same toggle, not a second
     * effect or permission to use any other original-parent power. */
    const bool drow_ready = hybrid_power_drow(ch, DROW_NIGHT);
"""

def without_drow_night_stage(text):
    if "void do_nightsight(" not in text:
        return text
    start = text.index("void do_nightsight(")
    end = text.index("\nvoid do_outcast(", start)
    body = text[start:end]
    if GRANT not in body:
        return text
    body = body.replace(GRANT, "", 1)
    for old in (
        "if (!IS_VAMPIRE(ch) && !IS_WEREWOLF(ch)",
        "if (IS_WEREWOLF(ch) && get_breed(ch, BREED_METIS) < 3)",
        "if (IS_VAMPIRE(ch) && get_disc(ch,DISC_PROTEAN) < 1)",
        "if (IS_DROW(ch) && !get_drow(ch, DROW_NIGHT))",
        "if (IS_DRAGON(ch) && !IS_DRAGAFF(ch, DRAG_NIGHT))",
        "if (IS_MAGE(ch) && ch->spheres[MFOR] < 1)",
    ):
        new = old.replace("if (", "if (!drow_ready && ", 1)
        assert body.count(new) == 1, ("Unreviewed night-sight gate", new)
        body = body.replace(new, old, 1)
    return text[:start] + body + text[end:]
