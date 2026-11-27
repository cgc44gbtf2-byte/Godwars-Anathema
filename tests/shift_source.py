"""Reverse only the explicit Werewolf shift threshold behavior."""

SHIFT_RAGE = """    if (minimum_shift)
        ch->pcdata->wolf = 100;
    else
    {
        ch->pcdata->wolf += 25;
        if (get_auspice(ch, AUSPICE_AHROUN) >= 4) ch->pcdata->wolf = 300;
        else if (ch->pcdata->wolf > 300) ch->pcdata->wolf = 300;
    }"""
OLD_RAGE = """    ch->pcdata->wolf += 25;

    if (get_auspice(ch, AUSPICE_AHROUN) >= 4) ch->pcdata->wolf = 300;
    else if (ch->pcdata->wolf > 300) ch->pcdata->wolf = 300;"""


def without_shift(text):
    if SHIFT_RAGE not in text:
        return text
    assert text.count(SHIFT_RAGE) == 1
    assert text.count("    bool minimum_shift;\n") == 1
    assert text.count('    minimum_shift = !str_cmp(arg, "shift");\n') == 1
    text = text.replace(SHIFT_RAGE, OLD_RAGE, 1)
    text = text.replace("    bool minimum_shift;\n", "", 1)
    text = text.replace('    minimum_shift = !str_cmp(arg, "shift");\n', "", 1)
    return text
