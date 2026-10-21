"""Reverse only the counted Scry safety/access edits; never rewrite other code."""
from aura_source import once

CONTEXT=('    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL) return;\n'
         '    argument = one_argument( argument != NULL ? argument : "", arg );')
OLD_CONTEXT=('    argument = one_argument( argument, arg );\n\n'
             '    if (IS_NPC(ch)) return;')
LABEL=('\t    const char *label = IS_AFFECTED(victim, AFF_POLYMORPH)\n'
       '\t        ? victim->morph : victim->name;\n'
       '\t    snprintf(buf, sizeof(buf), "%.*s: ", (int)(sizeof(buf) - sizeof(": ")),\n'
       '\t             label != NULL ? label : "Someone");')
OLD_LABEL=('\t    if ( IS_AFFECTED(victim, AFF_POLYMORPH) )\n'
           '\t\tsprintf(buf,"%s: ", victim->morph);\n'
           '\t    else\n\t\tsprintf(buf,"%s: ", victim->name);')
LOCATION=('\t\tsnprintf(buf, sizeof(buf), "%.*s.\\n\\r", (int)(sizeof(buf) - sizeof(".\\n\\r")),\n'
          '\t\t         victim->in_room->name != NULL ? victim->in_room->name : "(unknown)");')
GRANT=('    /* A learned Monk source is free and independent of the original class. */\n'
       '    const bool monk_ready = hybrid_power_rank(ch, CLASS_MONK,\n'
       '        HYBRID_TRAIN_MONK, MONK_MANTRA) >= 2;\n\n')

def without_scry_stage(text):
    start=text.index("void do_scry(");end=text.index("\nvoid do_readaura(",start)
    body=text[start:end]
    if GRANT in body:
        body=once(body,GRANT,"")
        body=once(body,"&& !IS_MONK(ch) && !IS_MAGE(ch) && !IS_BUI(ch) && !monk_ready)",
                       "&& !IS_MONK(ch) && !IS_MAGE(ch) && !IS_BUI(ch))")
        body=once(body,"if (!IS_ITEMAFF(ch,ITEMA_VISION) && !monk_ready)",
                       "if (!IS_ITEMAFF(ch,ITEMA_VISION))")
        body=once(body,"if (IS_MAGE(ch) && !monk_ready)","if (IS_MAGE(ch))")
    if CONTEXT in body:
        body=once(body,CONTEXT,OLD_CONTEXT)
        body=once(body,LABEL,OLD_LABEL)
        body=once(body,LOCATION,'\t\tsprintf(buf,"%s.\\n\\r", victim->in_room->name);')
        body=once(body,"== NULL || victim->in_room == NULL )","== NULL )")
    return text[:start]+body+text[end:]
