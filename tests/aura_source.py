"""Restore the exact pre-audit shared module, with scoped counted edits only."""
def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

CONTEXT=('    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL) return;\n'
         '    argument = one_argument( argument != NULL ? argument : "", arg );')
ORIGINAL_CONTEXT=('    argument = one_argument( argument, arg );\n\n'
                  '    if (IS_NPC(ch)) return;')
LABEL=('    /* Do not pass missing/oversized mobile labels through the legacy act\n'
       '     * expander; the bounded aura line below retains a useful NPC suffix. */\n'
       '    const char *npc_label = IS_NPC(victim) ? victim->short_descr : NULL;\n'
       '    if (IS_NPC(victim) && (npc_label == NULL ||\n'
       '        strlen(npc_label) > sizeof(buf) - sizeof(" is an NPC.\\n\\r")))\n'
       '        act("$n examines someone intently.",ch,NULL,victim,TO_NOTVICT);\n'
       '    else\n'
       '        act("$n examines $N intently.",ch,NULL,victim,TO_NOTVICT);')
PRINT=('    if (IS_NPC(victim))\n'
       '        snprintf(buf, sizeof(buf), "%.*s is an NPC.\\n\\r",\n'
       '                 (int)(sizeof(buf) - sizeof(" is an NPC.\\n\\r")),\n'
       '                 npc_label != NULL ? npc_label : "Someone");')
GRANT=('    /* A learned Monk grant is independent of the original class. Its\n'
       '     * native free aura reading also applies to Mage/Werewolf originals. */\n'
       '    const bool monk_ready = hybrid_power_rank(ch, CLASS_MONK,\n'
       '        HYBRID_TRAIN_MONK, MONK_MANTRA) >= 2;\n\n')

def without_aura_stage(text):
    from reader_fields_source import without_reader_fields_stage
    text=without_reader_fields_stage(text)
    start=text.index("void do_readaura(");end=text.index("\nvoid do_shield(",start)
    body=text[start:end]
    if GRANT in body:
        text=once(text,'#include "classes/hybrid/access.h"\n',"")
        start=text.index("void do_readaura(");end=text.index("\nvoid do_shield(",start)
        body=once(text[start:end],GRANT,"")
        body=once(body,"&& !IS_MONK(ch) && !IS_MAGE(ch) && !monk_ready)",
                       "&& !IS_MONK(ch) && !IS_MAGE(ch))")
        for cls in ("NINJA","VAMPIRE","WEREWOLF","DROW","DRAGON","MONK","MAGE"):
            body=once(body,"(!monk_ready && IS_"+cls+"(ch) &&","(IS_"+cls+"(ch) &&")
        body=once(body,"if (IS_MAGE(ch) && !monk_ready)","if (IS_MAGE(ch))")
        body=once(body,"|| (IS_WEREWOLF(ch) && !monk_ready))","|| IS_WEREWOLF(ch))")
        body=once(body,"if (IS_WEREWOLF(ch) && get_breed(ch, BREED_HOMID) >= 5 && !monk_ready)",
                       "if (IS_WEREWOLF(ch) && get_breed(ch, BREED_HOMID) >= 5)")
    body=once(body,CONTEXT,ORIGINAL_CONTEXT)
    body=once(body,LABEL,'    act("$n examines $N intently.",ch,NULL,victim,TO_NOTVICT);')
    body=once(body,PRINT,'    if (IS_NPC(victim)) sprintf(buf, "%s is an NPC.\\n\\r",victim->short_descr);')
    return text[:start]+body+text[end:]
