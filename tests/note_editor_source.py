"""Reverse only the exact note-draft fixes, retaining prior whole-file pins."""
def without_note_editor_stage(text):
    scopes=(
        ("/* Release an owned, unpublished draft before recycling PC data. */\n",
         "/* End draft release. */\n"),
        ("/* Check byte capacity before copying or replacing the owned draft. */\n",
         "/* End input capacity guard. */\n"),
        ("/* The file format and buffer limit count bytes, including colour codes and CRLF.\n",
         "/* End draft capacity guard. */\n"),
        ("/* Revalidate access after an interrupted session or a staff-level change. */\n",
         "/* End posting access recheck. */\n"),
    )
    for start,end in scopes:
        if start not in text:continue
        assert text.count(start)==text.count(end)==1
        a=text.index(start);b=text.index(end,a)+len(end)
        text=text[:a]+text[b:]
    marker="/* Capacity was checked before replacing the draft text. */"
    if marker in text:
        assert text.count(marker)==1
        text=text.replace(marker,
            "\t/* Check for overflow */\n\t\n"
            "\tif ((strlen2(letter) + strlen2(buf)) > MAX_NOTE_TEXT)\n"
            "\t{ /* Note too long, take appropriate steps */\n"
            '\t\twrite_to_buffer (d, "Note too long!\\n\\r", 0);\n'
            "\t\tfree_note (ch->pcdata->in_progress);\n"
            "\t\tch->pcdata->in_progress = NULL;\t\t\t/* important */\n"
            "\t\td->connected = CON_PLAYING;\n\t\treturn;\n\t}",1)
    return text
