"""Mask only the independently characterized character-score report."""
def without_character_score_stage(text):
    marker="/* Character score scope:"
    start=text.index(marker) if marker in text else text.index("void do_score(")
    stop=text.rfind("\n",start,text.index("day_name",start))+1
    return text[:start]+"/* character score scope */\n\n"+text[stop:]
