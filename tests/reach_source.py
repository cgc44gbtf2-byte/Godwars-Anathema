"""Reverse only the reviewed Silent Striders Reach restoration."""
import hashlib
import re

AFTER = "33ef394cf2f47791db87da0b9d9a9a22fd8491b21c773df69f13236ab3669848"
OLD = '''void do_reach( CHAR_DATA *ch, char *argument )
{
    char arg [MAX_INPUT_LENGTH];\x20\x20
    CHAR_DATA *victim;

    argument = one_argument( argument, arg );

    if ( IS_NPC(ch) ) return;

    if ( !IS_WEREWOLF(ch) )\x20
    {
\tstc("Huh?\\n\\r",ch);
\treturn;
    }

    if (get_tribe(ch,TRIBE_SILENT_STRIDERS) < 5)
    {
\tstc("You require Theurge 5 to Reach the Umbra.\\n\\r",ch);
\treturn;
    }

    if ( arg[0] == '\\0' )
    {
        stc( "Who do you want to pull with you into Umbra?\\n\\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        stc( "They are not here.\\n\\r", ch );
        return;
    }

    if (ch == victim)
    {
\tstc("All you have to do is step sideways.\\n\\r", ch);
\treturn;
    }

    if (IS_AFFECTED(ch, AFF_SHADOWPLANE))
    {
\tstc("Not while in the Umbra.\\n\\r", ch);
\treturn;
    }
    if (!IS_AFFECTED(victim, AFF_SHADOWPLANE))
    {
\tstc("They aren't in the Umbra.\\n\\r", ch);
\treturn;
    }

    act("You reach toward $N and pull $m out of the Umbra",ch,NULL,victim,TO_CHAR);
    act("$n reaches toward $N and pulls $m out of the Umbra.",ch,NULL,victim,TO_NOTVICT);
    act("$n reaches toward you and pulls you out of the Umbra.",ch,NULL,victim,TO_VICT);
    REMOVE_BIT(victim->affected_by, AFF_SHADOWPLANE);
    return;
}'''


def without_reach(text):
    from wrath_source import without_wrath
    text = without_wrath(text)
    match = re.search(r"void do_reach\(.*?(?=\nvoid do_wrath\()", text, re.S)
    if match:
        current = match.group(0)
        if current != OLD + "\n":
            assert hashlib.sha256(current.encode()).hexdigest() == AFTER, \
                "Unexpected Reach handler edit"
            text = text[:match.start()] + OLD + "\n" + text[match.end():]

    replacements = (
        ('if (get_tribe(ch, TRIBE_SILENT_STRIDERS) >= 5) strcat(buf, " Reach");',
         'if (get_tribe(ch, TRIBE_SILENT_STRIDERS) >= 5) strcat(buf, " Reach[unavailable]");'),
        ('   { "reach",\t\tdo_reach,\tPOS_STANDING,\t LEV_AWA,  LOG_NORMAL\t},\n', ''),
        ('DECLARE_DO_FUN( do_reach);\n', ''),
        ('5: Reach the Umbra - Pulls an eligible, non-fighting target out of the near Umbra. <reach>',
         '5: Reach the Umbra - A legacy handler exists, but no player command is registered. <unavailable>'),
    )
    for current, original in replacements:
        if current in text:
            assert text.count(current) == 1, current
            text = text.replace(current, original, 1)
    return text
