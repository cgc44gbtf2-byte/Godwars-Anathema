"""Only checked metric implementations and their narrow caller sites are masked."""
def without_progress_metrics_stage(text):
    from aux_shutdown_source import without_aux_shutdown_stage
    text=without_aux_shutdown_stage(text)
    from reader_fields_source import without_reader_fields_stage
    text=without_reader_fields_stage(text)
    text=text.replace('#include "progress_metrics.h"\n',"",1)
    if "bool is_name(" in text and ("long get_hours(" in text or "long get_hours( " in text):
        marker="/* Shared played-time metrics scope:"
        start=text.index(marker) if marker in text else text.index("long get_hours(")
        stop=text.index("bool is_name(",start)
        text=text[:start]+"/* played-time metric scope */\n\n"+text[stop:]
    if "void check_leaderboard(" in text:
        marker="/* Checked leaderboard metrics scope:"
        start=text.index(marker) if marker in text else text.index("void check_leaderboard(")
        end="/* Shared PK-score metrics scope:" if "/* Shared PK-score metrics scope:" in text else "long get_ratio("
        stop=text.index(end,start)
        text=text[:start]+"/* leaderboard derived-metric scope */\n\n"+text[stop:]
    if "long get_ratio(" in text:
        marker="/* Shared PK-score metrics scope:"
        start=text.index(marker) if marker in text else text.index("long get_ratio(")
        stop=text.index("void do_leaderclear(",start)
        text=text[:start]+"/* PK-score metric scope */\n\n"+text[stop:]
    if "static bool character_hours(" in text:
        start=text.index("static bool character_hours(");stop=text.index("void do_score(",start)
        text=text[:start]+"/* character checked-metric delegation */\n\n"+text[stop:]
    marker="/* Checked login age scope. */"
    old="if (!CAN_PK(ch) && get_hours(ch) >= AGE_PK)"
    if marker in text or old in text:
        start=text.index(marker) if marker in text else text.index(old)
        start=text.rfind("\n",0,start)+1
        stop=text.index("if (ch->level < LEV_SLE)",start)
        text=text[:start]+"/* login age grant scope */\n\n"+text[stop:]
    marker="    /* Checked staff hours scope. */"
    old='    sprintf(buf, "Hours: %li.  Played: %li.  Timer: %li.  Act: %li.'
    if marker in text or old in text:
        start=text.index(marker) if marker in text else text.index(old)
        stop=text.index("    stc(buf, ch);",start)
        text=text[:start]+"    /* staff checked-hour display scope */\n"+text[stop:]
    marker="    /* Checked covenant score scope. */"
    old="gch->hit, gch->mana, gch->status, get_ratio(gch)"
    if marker in text or old in text:
        start=text.index(marker) if marker in text else text.rfind("    sprintf( buf,",0,text.index(old))
        start=text.rfind("\n",0,start)+1
        stop=text.index("send_to_char( buf, ch );",start)
        stop=text.rfind("\n",start,stop)+1
        text=text[:start]+"    /* covenant checked-score display scope */\n"+text[stop:]
    return text
