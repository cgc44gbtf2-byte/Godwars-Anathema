"""Reverse only the reviewed Metis Totem Gift integration for older pins."""
import re


def without_totem_gift(text):
    removals = (
        ('    if (ch != victim) dam = werewolf_totem_absorb(victim, dam);\n', ''),
        ('        werewolf_totem_update(ch);\n', ''),
        ('    if (ch->metis_totem_id != 0)\n'
         '    {\n'
         '        werewolf_totem_clear(ch);\n'
         '        stc("Your tribal totem fades as you leave its room.\\n\\r", ch);\n'
         '    }\n', ''),
        ('    werewolf_totem_clear(victim);\n', ''),
        ('    long                metis_totem_timer;\n'
         '    long                metis_totem_absorb;\n'
         '    unsigned long       metis_totem_id;\n'
         '    ROOM_INDEX_DATA *   metis_totem_room;\n', ''),
        ('    unsigned long metis_totem_id;\n', ''),
        ('DECLARE_DO_FUN(do_totemgift);\n', ''),
        ('void werewolf_totem_clear(CHAR_DATA *ch);\n'
         'void werewolf_totem_update(CHAR_DATA *ch);\n'
         'long werewolf_totem_absorb(CHAR_DATA *ch, long damage);\n', ''),
        ('strcat(buf, " Totemgift");',
         'strcat(buf, " Totemgift[unimplemented]");'),
        ('5: Totem Gift - Summons your tribe\'s stationary totem; it absorbs up to 10,000 damage for you over 10 pulses, costs 5,000 mana, and fades if you leave. <totemgift>',
         '5: Totem Gift - No working command is implemented. <unimplemented>'),
    )
    for current, previous in removals:
        count = text.count(current)
        if count > 1:
            raise AssertionError("Ambiguous Totem Gift inverse")
        text = text.replace(current, previous)
    text, count = re.subn(
        r'(?m)^\s*\{\s*"totemgift"\s*,\s*do_totemgift\s*,[^\n]*\n', '', text)
    if count > 1:
        raise AssertionError("Ambiguous Totem Gift command binding")
    return text
