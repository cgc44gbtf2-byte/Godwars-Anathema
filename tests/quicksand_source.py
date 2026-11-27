"""Reverse only the reviewed Quicksand additions for historical source pins."""
import hashlib
import re

BEFORE = "8fed27b9dbd130108b66cae32313af4bc6f32f173a110ef544e204e899eb88ef"
AFTER = "532bddeca2dbc914e2971c4dd8356d084462885b3804f5abb41cdc1f3583ee6b"
OLD = '''void do_quicksand( CHAR_DATA *ch, char *argument )
{
    if (!IS_WEREWOLF(ch))
    {
\tstc("Huh?\\n\\r", ch);
\treturn;
    }

    if (get_tribe(ch,TRIBE_RED_TALONS) < 4)
    {
\tstc("You require Red Talons 4 to use Quicksand.\\n\\r", ch);
\treturn;
    }

    stc("Power Gutted - Akuma.\\n\\r", ch);
    return;
}

'''


def without_quicksand(text):
    from reach_source import without_reach
    text = without_reach(text)
    from werewolf_totem_source import without_totem_gift
    text = without_totem_gift(text)
    from lycaon_source import without_lycaon
    text = without_lycaon(text)
    match = re.search(r"void do_quicksand\(.*?(?=\nvoid do_|\Z)", text, re.S)
    if match:
        digest = hashlib.sha256(match[0].encode()).hexdigest()
        assert digest in (BEFORE, AFTER), "Unexpected Quicksand handler edit"
        if digest == AFTER:
            text = text[:match.start()] + OLD + text[match.end():]
            include = '#include "classes/hybrid/player.h"\n'
            assert text.count(include) == 1
            text = text.replace(include, "", 1)
    text = text.replace(
        'strcat(buf, " Quicksand");',
        'strcat(buf, " Quicksand[disabled]");')
    if '    long                quicksand_timer;\n' in text:
        text = text.replace('    long                quicksand_timer;\n', '')
        text = text.replace('/* Transient runtime state;', '/* Transient combat state;')
    text = text.replace('DECLARE_DO_FUN(do_quicksand);\n', '')
    text = text.replace(
        '   { "quicksand",\tdo_quicksand,\tPOS_FIGHTING,\t LEV_AWA,  LOG_NORMAL\t},\n', '')
    text = text.replace('    ch->quicksand_timer = 0;\n\n', '')
    text = text.replace(
        '    if (ch->quicksand_timer > 0) {\n'
        '        stc("The quicksand holds you fast; you cannot move that way.\\n\\r", ch);\n'
        '        return;\n'
        '    }\n\n', '')
    text = text.replace(
        '        if (ch->quicksand_timer > 0 && --ch->quicksand_timer == 0)\n'
        '        {\n'
        '            stc("You pull free of the quicksand.\\n\\r", ch);\n'
        '            act("$n pulls free of the quicksand.", ch, NULL, NULL, TO_ROOM);\n'
        '        }\n', '')
    return text
