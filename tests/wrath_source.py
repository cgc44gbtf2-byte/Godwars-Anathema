"""Reverse only the reviewed guarded Wrath restoration for historical pins."""
import hashlib
import re

BEFORE = "65bdba7ed80d7e33bced723fd801f98b8ab48cab22726ef205d70b8ef5a13dfe"
AFTER = "b08d33935faeed08095a3dac108fee2779693686492a0f36974d60ad92d92090"
OLD = '''void do_wrath( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *ich;\x20
    CHAR_DATA *fch;
    AFFECT_DATA af;
    long sn;

    if (IS_NPC(ch)) return;

    if (!IS_WEREWOLF(ch))
    {
\tstc("Huh?\\n\\r",ch);
\treturn;
    }

    if (get_tribe(ch,TRIBE_SILVER_FANGS) < 3)
    {
\tstc("You require Silver Fangs 3 to use Wrath of Gaia.\\n\\r",ch);
\treturn;
    }

    if ( ( sn = skill_lookup( "fear" ) ) < 0 )
    {
\tstc( "Bug - please inform Akuma.\\n\\r", ch );
\treturn;
    }

    stc("You start to howl.\\n\\r",ch);
    act("$n starts to howl.",ch,NULL,NULL,TO_ROOM);

    for ( ich = ch->in_room->people; ich != NULL; ich = fch )
    {
\tfch = ich->next_in_room;
        if (IS_NPC(ich))
\t    continue;
        if ((!IS_VAMPIRE(ich) || ch->beast <= 0))
\t    continue;
\tif (ich == ch)
\t    continue;
\tif ( IS_AFFECTED(ich, AFF_FEAR) )
\t    continue;

\tstc("You are overcome by fear.\\n\\r",ich);
\tact("$n screams in terror!",ich,NULL,NULL,TO_ROOM);
\taf.type      = sn;
\taf.location  = APPLY_HITROLL;
\taf.modifier  = 0 - get_tribe(ch,TRIBE_SILVER_FANGS);
\taf.duration  = number_range(1,2);
\taf.bitvector = AFF_FEAR;
\taffect_to_char( ich, &af );

\taf.location  = APPLY_DAMROLL;
\taf.modifier  = 0 - get_tribe(ch,TRIBE_SILVER_FANGS);
\taffect_to_char( ich, &af );
    }
    WAIT_STATE(ch,20);
    return;
}
'''


def without_wrath(text):
    from invisibility_source import without_invisibility
    text = without_invisibility(text)
    match = re.search(r"void do_wrath\(.*?(?=\nvoid do_mastery\()", text, re.S)
    if match:
        digest = hashlib.sha256(match[0].encode()).hexdigest()
        assert digest in (BEFORE, AFTER), "Unexpected Wrath handler edit"
        if digest == AFTER:
            text = text[:match.start()] + OLD + text[match.end():]
    for current, original in (
        ('strcat(buf, " Wrath");', 'strcat(buf, " Wrath[unavailable]");'),
        ('   { "wrath",\t\tdo_wrath,\tPOS_FIGHTING,\t LEV_AWA,  LOG_NORMAL\t},\n', ''),
        ('DECLARE_DO_FUN( do_wrath);\n', ''),
        ('3: Wrath of Gaia - Frightens eligible nearby vampires with positive Beast, reducing hitroll and damroll by gift rank. <wrath>',
         '3: Wrath of Gaia - A legacy handler exists, but no player command is registered. <unavailable>'),
    ):
        if current in text:
            assert text.count(current) == 1, current
            text = text.replace(current, original, 1)
    return text
