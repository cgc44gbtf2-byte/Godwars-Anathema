"""Invert only reviewed native Drow summon/crafting repairs.

Previous full-source hashes remain authoritative. Changes outside these exact
edits are never removed, including any change to a power, cost or stat formula.
"""

CONTEXT = """    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL)
        return;
"""
PREFLIGHT = """    /* Validate dependencies before reserving a follower or creating a mob.
     * create_mobile(NULL) terminates the process; no staging room is needed. */
    prototype = get_mob_index(MOB_VNUM_GUARDIAN);
    sn = skill_lookup("guardian");
    if (prototype == NULL || sn < 0)
    {
        stc("The spider summon is unavailable; please tell staff.\\n\\r",ch);
        return;
    }
    victim = create_mobile(prototype);
"""
OLD_CREATE = """    ch->pcdata->followers++;
    victim = create_mobile(get_mob_index(MOB_VNUM_GUARDIAN));
    char_to_room(victim,get_room_index(ROOM_VNUM_DISCONNECTION));
"""
OLD_SPEECH = """    strcpy(buf,"Come forth, minion of Lloth, and serve me!");
    do_say(ch, buf);

    stc("A spider skitters out of the shadows and kneels before you.\\n\\r", ch);
    act("$N skitters out of the shadows and kneels before $n.", ch, NULL, victim, TO_ROOM);

    if ((sn = skill_lookup("guardian")) < 0)
    {
\tstc("Bug - please inform Akuma.\\n\\r", ch);
\treturn;
    }
    
"""
NEW_SPEECH = """    stc("A spider skitters out of the shadows and kneels before you.\\n\\r", ch);
    act("$N skitters out of the shadows and kneels before $n.", ch, NULL, victim, TO_ROOM);
    /* Speech can execute room-text callbacks. Finish ownership/charm first,
     * and never dereference the summoner or pet after the callback. */
    strcpy(buf,"Come forth, minion of Lloth, and serve me!");
    do_say(ch, buf);
"""
RELIC_GUARDS = """    if (pObjIndex->item_type != (vnum == OBJ_VNUM_DROW_CROSSBOW ? ITEM_MISSILE : ITEM_ARMOR) ||
        !IS_SET(pObjIndex->wear_flags,ITEM_TAKE) ||
        !IS_SET(pObjIndex->wear_flags,wear_flag) || pObjIndex->weight < 0)
    {
        stc("The Drow relic template is invalid; please tell staff.\\n\\r",ch);
        return;
    }
    if (ch->carry_number < 0 || ch->carry_number == LONG_MAX ||
        ch->carry_weight < 0 || ch->carry_weight > LONG_MAX - pObjIndex->weight)
    {
        stc("You cannot safely carry that Drow relic.\\n\\r",ch);
        return;
    }

"""

def once(text, new, old):
    assert text.count(new) == 1, ("Unreviewed native Drow edit", new[:100])
    return text.replace(new, old, 1)

def without_native_drow_stage(text):
    if "void do_summonspider(" not in text:
        return text
    start = text.index("void do_summonspider(")
    end = text.index("\n/* Consumable recipe", start)
    body = text[start:end]
    if PREFLIGHT in body:
        body = once(body, "    MOB_INDEX_DATA *prototype;\n", "")
        body = once(body, CONTEXT+"\n", "")
        body = once(body, "ch->pcdata->followers != 0", "ch->pcdata->followers >= 1")
        body = once(body, PREFLIGHT, OLD_CREATE)
        body = once(body, "    char_to_room(victim,ch->in_room);",
                    "    char_from_room(victim);\n    char_to_room(victim,ch->in_room);")
        body = once(body, "    ch->pcdata->followers++;\n    add_follower",
                    OLD_SPEECH+"    add_follower")
        body = once(body, NEW_SPEECH, "")
        text = text[:start]+body+text[end:]
    start = text.index("void do_drowarmor(")
    end = text.index("\nvoid do_door(", start)
    body = text[start:end]
    if RELIC_GUARDS in body:
        body = once(body, "    long vnum = 0, wear_flag = 0;",
                    "    long vnum = 0;")
        body = once(body, CONTEXT, "")
        for piece, vnum, flag in (
            ("cloak", "29652", "ITEM_WEAR_ABOUT"),
            ("helmet", "29653", "ITEM_WEAR_HEAD"),
            ("breastplate", "29654", "ITEM_WEAR_BODY"),
            ("bracer", "29655", "ITEM_WEAR_WRIST"),
            ("crossbow", "OBJ_VNUM_DROW_CROSSBOW", "ITEM_WEAR_HOLD"),
        ):
            body = once(body, f'if (!str_cmp(arg,"{piece}")) '
                        f'{{vnum = {vnum}; wear_flag = {flag};}}',
                        f'if (!str_cmp(arg,"{piece}")) vnum = {vnum};')
        body = once(body, RELIC_GUARDS, "")
        text = text[:start]+body+text[end:]
    return text
