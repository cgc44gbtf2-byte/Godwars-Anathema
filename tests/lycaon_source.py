"""Strictly reverse the bounded Lycaon feature for older source pins."""
import hashlib
import re

HANDLER_SHA256 = "69e59882cc63b66757332da031d7e4bc04d3564e16c05aa8c0dcab00f95137c5"
OLD_HANDLER = '''void do_lycaon( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MSL];

    one_argument(argument,arg);

    if (IS_NPC(ch)) return;

    if (!IS_WEREWOLF(ch))
    {
\tstc("Huh?\\n\\r",ch);
\treturn;
    }

    if (get_tribe(ch,TRIBE_RED_TALONS) < 5)
    {
\tstc("You require Red Talons 5 to use Curse of Lycaon.\\n\\r",ch);
\treturn;
    }

    if (arg[0] == '\\0')
    {
\tstc("Curse whom?\\n\\r",ch);
\treturn;
    }

    if ((victim = get_char_room(ch,arg)) == NULL)
    {
\tstc("They aren't here.\\n\\r",ch);
\treturn;
    }

    if (is_safe(ch,victim)) return;

    stc("This power has been gutted.\\n\\r", ch);

/*    stc("You sneer as you force them into the form of a wolf.\\n\\r",ch);
    act("$n sneers as $e concentrates on $N",ch,NULL,victim,TO_NOTVICT);
    act("$n sneers as $e concentrates on you.",ch,NULL,victim,TO_VICT);*/

    return;
}
'''

def _replace_once(text, old, new):
    assert text.count(old) == 1, "Lycaon source differs from reviewed patch"
    return text.replace(old, new, 1)

def without_lycaon(text):
    if "void do_lycaon(" in text:
        match = re.search(r"void do_lycaon\(.*?(?=\nvoid do_|\Z)", text, re.S)
        assert match
        digest = hashlib.sha256(match[0].encode()).hexdigest()
        if digest == HANDLER_SHA256:
            text = text[:match.start()] + OLD_HANDLER + text[match.end():]
        else:
            assert match[0] == OLD_HANDLER, "Unexpected Lycaon handler edit"
    if 'if (get_tribe(ch, TRIBE_RED_TALONS) >= 5) strcat(buf, " Lycaon");' in text:
        text = _replace_once(
            text,
            'if (get_tribe(ch, TRIBE_RED_TALONS) >= 5) strcat(buf, " Lycaon");',
            'if (get_tribe(ch, TRIBE_RED_TALONS) >= 5) strcat(buf, " Lycaon[disabled]");')
    text = text.replace('    long                lycaon_timer;\n', '')
    text = text.replace('DECLARE_DO_FUN(do_lycaon);\nvoid werewolf_clear_lycaon(CHAR_DATA *ch);\n', '')
    text = text.replace(
        '   { "lycaon",\t\tdo_lycaon,\tPOS_FIGHTING,\t LEV_AWA,  LOG_NORMAL\t},\n',
        '')
    text = text.replace('#include <string.h>\n', '', 1) if 'void werewolf_clear_lycaon' in text else text
    if "void werewolf_clear_lycaon" in text:
        text = re.sub(
            r"void werewolf_clear_lycaon\(CHAR_DATA \*ch\)\n\{.*?\n\}\n\n",
            "", text, count=1, flags=re.S)
        text = _replace_once(
            text,
            '        if (ch->lycaon_timer == 1)\n'
            '        {\n'
            '            stc("The curse fades and your natural appearance returns.\\n\\r", ch);\n'
            '            act("$n\'s wolf form fades away.", ch, NULL, NULL, TO_ROOM);\n'
            '            werewolf_clear_lycaon(ch);\n'
            '        }\n'
            '        else if (ch->lycaon_timer > 1)\n'
            '            --ch->lycaon_timer;\n',
            "")
    morph_save = ('    fprintf( fp, "Morph        %s~\\n",\n'
                  '        ch->lycaon_timer > 0 ? "" : ch->morph);\n')
    if morph_save in text:
        text = _replace_once(
            text, morph_save,
            '    fprintf( fp, "Morph        %s~\\n",\tch->morph\t\t);\n')
    affect_save = ('    fprintf( fp, "AffectedBy   %li\\n",\n'
                   '        ch->lycaon_timer > 0 ? ch->affected_by & ~AFF_POLYMORPH : ch->affected_by);\n')
    if affect_save in text:
        text = _replace_once(
            text, affect_save,
            '    fprintf( fp, "AffectedBy   %li\\n",\tch->affected_by\t\t);\n')
    help = ('5: Curse of Lycaon - Changes an eligible target\'s appearance to wolf form '
            'for 30 pulses; costs 1000 mana. <lycaon>')
    if help in text:
        text = _replace_once(
            text, help,
            '5: Curse of Lycaon - Handler is a disabled stub; it does not transform targets. <disabled>')
    return text
