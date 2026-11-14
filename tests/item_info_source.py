"""Mask only the separately characterized comparison and object-score scopes."""
from room_display_source import without_room_display_stage
def without_item_info_stage(text):
    text=without_room_display_stage(text)
    for marker, original, end, label in (
        ("/* Item comparison scope:", "void do_compare(", "void do_credits(",
         "item comparison"),
        ("/* Item score reporting scope:", "void obj_score(",
         "/* Do_prompt from Morgenes", "item score"),
    ):
        start=text.index(marker) if marker in text else text.index(original)
        stop=text.index(end,start)
        text=text[:start]+"/* "+label+" scope */\n\n"+text[stop:]
    return text.replace("#include <stdarg.h>\n","",1)
