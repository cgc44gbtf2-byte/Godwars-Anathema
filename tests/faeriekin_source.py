"""Remove only the Faerie Kin addition before historical source comparisons."""


def without_faeriekin_stage(text):
    for current, original in (
        ('if (get_tribe(ch, TRIBE_FIANNA) >= 3) strcat(buf, " Faeriekin");\n', ''),
        ('#include "classes/werewolf/faeriekin.h"\n', ''),
        ('   { "faeriekin",\tdo_faeriekin,\tPOS_STANDING,\t LEV_AWA,  LOG_NORMAL\t},\n', ''),
        ('3: Faerie Kin - Summons a modest faerie spirit for ten minutes. <faeriekin>\n', ''),
    ):
        if current in text:
            assert text.count(current) == 1, "Repeated Faerie Kin source addition"
            text = text.replace(current, original, 1)
    return text
