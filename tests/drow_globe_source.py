"""Invert only reviewed Globe ownership edits for older whole-source pins.

The original full-source hashes remain the final authority; unmatched edits
are left visible so those pins still fail.
"""

UPKEEP = '''    /* A hybrid's Drow upkeep is independent of its original-class update
     * branch; it must run even when that primary class has its own upkeep. */
    if (!IS_DROW(ch) && hybrid_drow_globe_active(ch))
    {
int upkeep=hybrid_drow_globe_upkeep(ch,150);
if (upkeep==0)
{
    stc("You are unable to maintain your globe of darkness.\\n\\r",ch);
    act("Darkness fades from the room as $n relaxes.",ch,NULL,NULL,TO_ROOM);
}
    }

'''

def without_drow_globe_stage(text):
    text = text.replace(
        "\tif (!IS_VAMPIRE(ich) && !IS_DROW(ich))\n\t{\n"
        "\t    /* Only the owned secondary Globe is dispelled; do not run native\n"
        "\t     * Vampire/Drow transformation cleanup on the original parent. */\n"
        "\t    if (hybrid_drow_globe_active(ich)) hybrid_drow_globe_set(ich,0);\n"
        "\t    continue;\n\t}\n",
        "\tif (!IS_VAMPIRE(ich) && !IS_DROW(ich)) continue;\n")
    text = text.replace(
        "\t\t}\n\t    }\n\n\t    /* A hybrid's Drow upkeep is independent of its original-class update\n"
        "\t     * branch; it must run even when that primary class has its own upkeep. */\n"
        "\t    if (!IS_DROW(ch) && hybrid_drow_globe_active(ch))\n\t    {\n"
        "\t\tint upkeep=hybrid_drow_globe_upkeep(ch,150);\n\t\tif (upkeep==0)\n\t\t{\n"
        '\t\t    stc("You are unable to maintain your globe of darkness.\\n\\r",ch);\n'
        '\t\t    act("Darkness fades from the room as $n relaxes.",ch,NULL,NULL,TO_ROOM);\n', "")
    if "void do_scry(" in text:
        text = text.replace("else if (hybrid_drow_globe_active(ch))",
                            "else if (get_drow(ch, DROW_GLOBE))")
        text = text.replace("\thybrid_drow_globe_set(ch,0);",
                            "\tREMOVE_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);")
        text = text.replace("\thybrid_drow_globe_set(ch,1);",
                            "\tSET_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);")
    if "void char_from_room(" in text:
        text = text.replace("    if (hybrid_drow_globe_active(ch))",
                            "    if (IS_DROW(ch) && get_drow(ch, DROW_GLOBE))")
        text = text.replace("if (hybrid_drow_globe_active(vch)) return FALSE;",
                            "\tif ((IS_DROW(vch) && get_drow(vch, DROW_GLOBE))) return FALSE;")
    text = text.replace('    if (hybrid_drow_globe_active(ch)) stc("You are currently using',
                        '\t    if (get_drow(ch, DROW_GLOBE)) stc("You are currently using')
    text = text.replace("hybrid_drow_globe_set(victim,0);",
                        "\tREMOVE_BIT(victim->pcdata->drow[DROW_POWERS], DROW_GLOBE);")
    text = text.replace("\tif (!IS_VAMPIRE(ich) && !IS_DROW(ich) &&\n\t    !hybrid_power_parent(ich,CLASS_DROW)) continue;",
                        "\tif (!IS_VAMPIRE(ich) && !IS_DROW(ich)) continue;")
    text = text.replace("\tif (hybrid_drow_globe_active(ich))", "\tif (IS_DROW(ich))")
    text = text.replace("\t    hybrid_drow_globe_set(ich,0);",
                        "\t    if (get_drow(ich, DROW_GLOBE)) REMOVE_BIT(ich->pcdata->drow[DROW_POWERS], DROW_GLOBE);")
    # These five modules gained an include; clan.c and drow.c already had it.
    if any(marker in text for marker in (
            "void do_blur(", "void do_look(", "void char_from_room(",
            "void mobile_update(", "/* Death templates accept")):
        text = text.replace('#include "classes/hybrid/access.h"\n', "", 1)
    if "void do_globedark(" in text:
        start = text.index("void do_globedark(")
        end = text.index("\nstatic OBJ_DATA *drow_shot_ammunition", start)
        body = text[start:end]
        body = body.replace("!hybrid_power_parent(ch,CLASS_DROW)", "!IS_DROW(ch)")
        body = body.replace("!hybrid_power_drow(ch, DROW_DARK)", "!get_drow(ch, DROW_DARK)")
        body = body.replace("hybrid_drow_globe_active(ch)", "get_drow(ch, DROW_GLOBE)")
        body = body.replace("hybrid_drow_globe_set(ch,0);",
                            "\tREMOVE_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);")
        body = body.replace("hybrid_drow_points_current(ch) < 500",
                            "ch->pcdata->classpoints[CP_CURRENT] < 500")
        body = body.replace("\nif (ch->pcdata->classpoints[CP_CURRENT] < 500)",
                            "\n\tif (ch->pcdata->classpoints[CP_CURRENT] < 500)")
        charge = '''if (!hybrid_drow_points_charge(ch,500) ||
    !hybrid_drow_globe_set(ch,1))
{
    stc("The globe cannot be activated right now.\\n\\r",ch);
    return;
}
'''
        if charge in body:
            body = body.replace(charge, "", 1)
            line = 'act("$n concentrates as the room fades into darkness!", ch, NULL, NULL, TO_ROOM);\n'
            body = body.replace(line, line +
                "\tSET_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);\n"
                "\tch->pcdata->classpoints[CP_CURRENT] -= 500;\n", 1)
        text = text[:start] + body + text[end:]
    pairs = (
        (UPKEEP, ""),
        ("if (hybrid_drow_globe_active(ch)) hybrid_drow_globe_set(ch,0);",
         "if (get_drow(ch, DROW_GLOBE)) REMOVE_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);"),
        ("if (hybrid_drow_globe_active(vch)) return FALSE;",
         "if ((IS_DROW(vch) && get_drow(vch, DROW_GLOBE))) return FALSE;"),
        ("    if (hybrid_drow_globe_active(vch))",
         "    if (IS_DROW(vch) && get_drow(ch, DROW_GLOBE))"),
        ('    if (hybrid_drow_globe_active(ch)) stc("You are currently using',
         '    if (get_drow(ch, DROW_GLOBE)) stc("You are currently using'),
        ("    if (hybrid_drow_globe_active(ch))\nact(",
         "    if (IS_DROW(ch) && get_drow(ch, DROW_GLOBE))\nact("),
        ("    else if (hybrid_drow_globe_active(ch))\n    {\nhybrid_drow_globe_set(ch,0);\ndo_look(ch, \"auto\");\nhybrid_drow_globe_set(ch,1);",
         "    else if (get_drow(ch, DROW_GLOBE))\n    {\nREMOVE_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);\ndo_look(ch, \"auto\");\nSET_BIT(ch->pcdata->drow[DROW_POWERS], DROW_GLOBE);"),
        ("victim->hit <= 0 && hybrid_drow_globe_active(victim)",
         "victim->hit <= 0 && IS_DROW(victim) && get_drow(victim, DROW_GLOBE)"),
        ("hybrid_drow_globe_set(victim,0);",
         "REMOVE_BIT(victim->pcdata->drow[DROW_POWERS], DROW_GLOBE);"),
        ("if (!IS_VAMPIRE(ich) && !IS_DROW(ich) &&\n    !hybrid_power_parent(ich,CLASS_DROW)) continue;",
         "if (!IS_VAMPIRE(ich) && !IS_DROW(ich)) continue;"),
        ("if (hybrid_drow_globe_active(ich))\n{\n    hybrid_drow_globe_set(ich,0);",
         "if (IS_DROW(ich))\n{\n    if (get_drow(ich, DROW_GLOBE)) REMOVE_BIT(ich->pcdata->drow[DROW_POWERS], DROW_GLOBE);"),
    )
    for new, old in pairs:
        text = text.replace(new, old)
    return text
