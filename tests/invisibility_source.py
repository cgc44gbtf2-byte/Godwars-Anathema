"""Reverse only the reviewed shared invisibility and Uktena changes."""
import hashlib
import re

GRANTED = '''static bool invisibility_granted(CHAR_DATA *ch, const AFFECT_DATA *removed)
{
    for (AFFECT_DATA *af = ch->affected; af != NULL; af = af->next)
        if (af != removed && IS_SET(af->bitvector, AFF_INVISIBLE)) return TRUE;
    for (OBJ_DATA *obj = ch->carrying; obj != NULL; obj = obj->next_content) {
        if (obj->wear_loc == WEAR_NONE) continue;
        if (equivalent_equipment_spell(equipment_spell(obj)) == 5) return TRUE;
        for (AFFECT_DATA *af = obj->pIndexData->affected; af != NULL; af = af->next)
            if (IS_SET(af->bitvector, AFF_INVISIBLE)) return TRUE;
        for (AFFECT_DATA *af = obj->affected; af != NULL; af = af->next)
            if (IS_SET(af->bitvector, AFF_INVISIBLE)) return TRUE;
    }
    return FALSE;
}

'''
GUARD = '''    (void)argument;
    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL ||
        !hybrid_player_save_valid(ch) || ch->hit <= 0 ||
        ch->position < POS_STANDING || ch->wait > 0 ||
        IS_COMBAT_PAUSED(ch) || IS_SET(ch->pcdata->flag, FLAG_AFK) ||
        !combat_character_in_room(ch->in_room, ch))
        return;'''


def without_invisibility(text):
    from faeriekin_source import without_faeriekin_stage
    text = without_faeriekin_stage(text)
    match = re.search(r"void do_invisibility\(.*?(?=\nvoid do_fetishdoll\()", text, re.S)
    if match:
        body = match[0]
        digest = hashlib.sha256(body.encode()).hexdigest()
        assert digest in (
            "162b3d524e019f3d52bd7c5aef57d9c85b184144dab8dda29bdd9d0f3978a3c1",
            "29d336d70ed450db476d679905f2b45de03e0ecabc3b534f6040f8e36cc921f6",
        ), "Unreviewed Uktena handler change"
        if digest.startswith("29d336d"):
            body = body.replace(GUARD, "    if (IS_NPC(ch)) return;", 1)
            body = body.replace("!IS_WEREWOLF(ch) || ch->level < LEV_AWA", "!IS_WEREWOLF(ch)", 1)
            body = body.replace('    if ((sn = skill_lookup("invis")) < 0) return;\n', "", 1)
            body = body.replace("IS_AFFECTED(ch, AFF_INVISIBLE) || is_affected(ch, sn) ||\n"
                                "        is_affected(ch, gsn_mass_invis)",
                                "IS_AFFECTED(ch, AFF_INVISIBLE)", 1)
            body = body.replace("    (*skill_table[sn].spell_fun)",
                                '    if ( ( sn = skill_lookup( "invis" ) ) < 0 ) return;\n'
                                "    (*skill_table[sn].spell_fun)", 1)
            text = text[:match.start()] + body + text[match.end():]
    for new, old in (
        ("static bool invisibility_granted(CHAR_DATA *ch, const AFFECT_DATA *removed);\n\n", ""),
        ("bool invisibility_active = IS_AFFECTED(ch, AFF_INVISIBLE);\n", ""),
        ("if (invisibility_active && IS_SET(paf->bitvector, AFF_INVISIBLE) &&\n"
         "    invisibility_granted(ch, paf))\n"
         "    SET_BIT(ch->affected_by, AFF_INVISIBLE);\n", ""),
        (GRANTED, ""),
        ("else if (IS_AFFECTED(ch, AFF_INVISIBLE) && (sn == 46 || sn == 5)\n"
         "&& !invisibility_granted(ch, NULL))",
         "else if (IS_AFFECTED(ch, AFF_INVISIBLE) && (sn == 46 || sn == 5)\n"
         "\t\t&& !is_affected(ch, 46))"),
        ("    if (IS_AFFECTED(victim, AFF_INVISIBLE) &&\n"
         "        !IS_AFFECTED(ch, AFF_DETECT_INVIS) && !effective_holylight(ch))\n"
         "        return FALSE;\n\n", ""),
        ('strcat(buf, " Invisibility");', 'strcat(buf, " Invisibility[unavailable]");'),
        ('   { "invisibility",\tdo_invisibility,POS_STANDING,\t LEV_AWA,  LOG_NORMAL\t},\n', ""),
        ("DECLARE_DO_FUN( do_invisibility);\n", ""),
        ("4: Invisibility - Ordinary timed concealment, countered by detection; no resource cost. <invisibility>",
         "4: Invisibility - A legacy handler exists, but no player command is registered. <unavailable>"),
    ):
        if new in text:
            assert text.count(new) == 1, "Repeated invisibility edit"
            text = text.replace(new, old, 1)
    return text
