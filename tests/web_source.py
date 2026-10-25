"""Reverse only the hash-checked Web handler; retain all older source pins."""
import hashlib

OLD="70d00d9257ed6d361ce7909dfe1265604b1d37069f589df4c75876e6bb70ff35"
SAFETY="a55cd1fa7f0b42fb7b3d393d008e388827b779610c2e7562a0df6cddaef908fc"
FINAL="6b7c6094c634242d7e1d97ddc1a36bb05398477831e660231385bc4ecf25089c"
GRANT="""    /* Independent grants, one spell: a learned Drow web has its native free
     * activation even for Ninja originals. Otherwise retain the Ninja route. */
    const bool drow_ready = hybrid_power_drow(ch, DROW_WEB);
"""
CONTEXT="""    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL) return;
    argument = one_argument(argument != NULL ? argument : "", arg);"""
OLD_CONTEXT="""    argument = one_argument(argument, arg);

    if (IS_NPC(ch)) return;"""

def once(text,new,old):
    assert text.count(new)==1,("Unreviewed Web fragment",new)
    return text.replace(new,old)

def without_web_stage(text):
    if "void do_web(" not in text:return text
    start=text.index("void do_web(");end=text.index("\nvoid do_entrancement(",start)
    body=text[start:end];digest=hashlib.sha256(body.encode()).hexdigest()
    if digest==OLD:return text
    assert digest in (SAFETY,FINAL),("Unreviewed Web edits",digest)
    if digest==FINAL:
        body=once(body,GRANT,"")
        body=once(body,"if (!IS_NINJA(ch) && !hybrid_power_parent(ch,CLASS_DROW))",
                       "if (!IS_NINJA(ch) && !IS_DROW(ch))")
        body=once(body,"if (!IS_NINJA(ch) && !drow_ready)",
                       "if (IS_DROW(ch) && !get_drow(ch, DROW_WEB))")
        assert body.count("!drow_ready && ")==3
        body=body.replace("!drow_ready && ","")
    body=once(body,"    ROOM_INDEX_DATA *combat_room;\n","")
    body=once(body,CONTEXT,OLD_CONTEXT)
    body=once(body,"    /* A current-opponent fallback must still be a live local actor. */\n"
                   "    if (!combat_character_in_room(ch->in_room, victim)) return;\n","")
    body=once(body,'if ((sn = skill_lookup("web")) < 0 || skill_table[sn].spell_fun == NULL) return;',
                   'if ((sn = skill_lookup("web")) < 0) return;')
    body=once(body,"    combat_room = ch->in_room;\n","")
    body=once(body,"    if (!combat_character_in_room(combat_room, ch)) return;\n","")
    assert hashlib.sha256(body.encode()).hexdigest()==OLD,"Web inverse drift"
    return text[:start]+body+text[end:]
