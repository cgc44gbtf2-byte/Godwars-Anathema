"""Reverse only terminal auxiliary checkpoint additions; retain prior pins."""
def once(text,new,old=""):
    assert text.count(new)==1,(new[:90],text.count(new))
    return text.replace(new,old,1)

def without_aux_shutdown_stage(text):
    from note_editor_source import without_note_editor_stage
    text=without_note_editor_stage(text)
    scopes=(
        ("/* Forced terminal board checkpoint. */\n",
         "/* End forced terminal board checkpoint. */\n\n"),
        ("/* Forced terminal leaderboard checkpoint. */\n",
         "/* End forced terminal leaderboard checkpoint. */\n\n"),
        ("    /* Forced terminal auxiliary checkpoint. */\n",
         "    /* End forced terminal auxiliary checkpoint. */\n"),
        ("    /* Manual auxiliary checkpoint must finish before shutdown is announced. */\n",
         "    /* End manual auxiliary checkpoint. */\n"),
        ("    /* Copyover auxiliary checkpoint precedes handoff and descriptor changes. */\n",
         "    /* End copyover auxiliary checkpoint. */\n"),
    )
    changed=False
    for start,end in scopes:
        if start not in text:continue
        assert text.count(start)==text.count(end)==1
        a=text.index(start);b=text.index(end,a)+len(end)
        text=text[:a]+text[b:];changed=True
    if changed:
        text=once(text,'#include "auxiliary_save.h"\n')
        if "void restart_copyover(" in text:
            text=once(text,"Check primary/board/leaderboard saves,","Check primary saves,")
    header=("/* Include merc.h first. One final pass ignores periodic retry backoff.\n"
            " * FALSE includes a visible replacement whose directory sync was uncertain.\n"
            " * These functions run only in normal execution, never signal handlers. */\n"
            "bool flush_notes(void);\n"
            "bool flush_leaderboard(void);\n"
            "bool auxiliary_save_checkpoint(void);\n\n")
    if header in text:text=once(text,header)
    return text
