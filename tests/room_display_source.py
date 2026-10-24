"""Only the room/exit output scopes are masked for earlier source pins."""
from character_score_source import without_character_score_stage
def without_room_display_stage(text):
    text=without_character_score_stage(text)
    for marker,original,end,label in (
        ("/* Room display scope:", "void do_look(", "void do_examine(", "room look"),
        ("/* Exit display scope:", "void do_exits(", "/* Score command redone - Kai */", "room exits"),
    ):
        start=text.index(marker) if marker in text else text.index(original)
        stop=text.index(end,start)
        text=text[:start]+"/* "+label+" scope */\n\n"+text[stop:]
    return text
