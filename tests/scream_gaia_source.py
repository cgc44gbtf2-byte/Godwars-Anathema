"""Reverse the exact Scream of Gaia restoration for earlier source pins."""
import hashlib
import re

HANDLER_SHA256 = "1e632e459095ca42ffaeae435c10389387738fb3c9bb8ec162544ae597932d61"


def without_scream_gaia(text):
    match = re.search(r"void do_screamgaia\(.*?(?=\nvoid do_|\Z)", text, re.S)
    if match:
        assert hashlib.sha256(match[0].encode()).hexdigest() == HANDLER_SHA256, \
            "Unexpected Scream of Gaia handler edit"
        end = match.end() + (1 if text[match.end():match.end() + 1] == "\n" else 0)
        text = text[:match.start()] + text[end:]
    replacements = (
        ('if (get_tribe(ch, TRIBE_GET_OF_FENRIS) >= 4) strcat(buf, " Screamgaia");',
         'if (get_tribe(ch, TRIBE_GET_OF_FENRIS) >= 4) strcat(buf, " Scream[unimplemented]");'),
        ('DECLARE_DO_FUN(do_screamgaia);\n', ''),
        ('    { "screamgaia",\tdo_screamgaia,\tPOS_FIGHTING,\t LEV_AWA,  LOG_NORMAL\t},\n', ''),
        ('4: Scream of Gaia - Interrupts eligible foes in the room for one pulse; costs 5000 mana. <screamgaia>',
         '4: Scream of Gaia - No working command is implemented. <unimplemented>'),
    )
    for old, new in replacements:
        if old in text:
            assert text.count(old) == 1, "Ambiguous Scream of Gaia source"
            text = text.replace(old, new, 1)
    return text
