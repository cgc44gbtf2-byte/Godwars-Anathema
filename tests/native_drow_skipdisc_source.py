"""Undo only reviewed native disc ownership edits for historical source pins."""
import base64
import hashlib

OLD_SKIP = base64.b64decode(
    "dm9pZCBkb19za2lwZGlzYyggQ0hBUl9EQVRBICpjaCwgY2hhciAqYXJndW1lbnQgKQp7CiAgICBpZiAoIUlTX0RST1coY2gpKQogICAgewoJc3RjKCJIdWg/XG5cciIsIGNoKTsKCXJldHVybjsKICAgIH0KCiAgICBpZiAoIWdldF9kcm93KGNoLCBEUk9XX1NLSVApKQogICAgewoJc3RjKCJZb3UgbmVlZCB0byBvYnRhaW4gc2tpcCBkaXNjIGZpcnN0LlxuXHIiLCBjaCApOwoJcmV0dXJuOwogICAgfQoKICAgIGlmIChJU19BRkZFQ1RFRChjaCwgQUZGX0ZMWUlORykpCiAgICB7CglzdGMoIllvdSBsZWFwIG9mZiB5b3VyIHNraXAgZGlzYy5cblxyIiwgY2gpOwoJUkVNT1ZFX0JJVChjaC0+YWZmZWN0ZWRfYnksIEFGRl9GTFlJTkcpOwogICAgfQogICAgZWxzZQogICAgewoJc3RjKCJZb3UgbGVhcCBvbnRvIHlvdXIgc2tpcCBkaXNjLlxuXHIiLCBjaCk7CglTRVRfQklUKGNoLT5hZmZlY3RlZF9ieSwgQUZGX0ZMWUlORyk7CiAgICB9CiAgICByZXR1cm47Cn0K"
).decode()

DISC_FLAG = """/* Native active-state ownership, not a learned gift or secondary-parent grant. */
#define DROW_DISC 524288
"""
DISC_MACROS = """/* Disc flight is derived, never written over spell/equipment-owned raw flight. */
#define DROW_DISC_ACTIVE(ch) \\
    ((ch) != NULL && !IS_NPC(ch) && (ch)->pcdata != NULL && \\
     (ch)->class == CLASS_DROW && IS_SET((ch)->pcdata->drow[DROW_POWERS], DROW_DISC))
#define IS_AFFECTED(ch, sn) \\
    (IS_SET((ch)->affected_by | (DROW_DISC_ACTIVE(ch) ? AFF_FLYING : 0), (sn)))
"""

def once(text, new, old):
    assert text.count(new) == 1, "Unreviewed native Skip Disc edit"
    return text.replace(new, old, 1)

def without_native_drow_skipdisc(text):
    from native_drow_spider_stats_source import without_native_drow_spider_stats
    text = without_native_drow_spider_stats(text)
    from native_drow_source import without_native_drow_ammo_template_guard
    text = without_native_drow_ammo_template_guard(text)
    for code in ("39", "3 "):
        new = f"else if ((sn == {code}) && (IS_SET(ch->affected_by, AFF_FLYING))) return;\n"
        if new in text:
            text = once(text, new,
                        f"\telse if ((sn == {code}) && (IS_AFFECTED(ch, AFF_FLYING)))       return;\n")
    if "names,17,DROW_GLOBE|DROW_DISC);" in text:
        text = once(text, "names,17,DROW_GLOBE|DROW_DISC);",
                    "names,17,DROW_GLOBE);")
    if "/* Native active-state ownership," in text:
        text = once(text, DISC_FLAG, "")
        text = once(text, DISC_MACROS,
                    "#define IS_AFFECTED(ch, sn)\t(IS_SET((ch)->affected_by, (sn)))\n")
    if "/* Dismiss only this source, including after training loss. */" in text:
        start = text.index("void do_skipdisc(")
        end = text.index("\nvoid do_drowupkeep(", start)
        body = text[start:end]
        assert hashlib.sha256(body.encode()).hexdigest() == (
            "e3a94472f2cbde35eb3e71a82838f97c2b0460dbe2c16075884bcc0fcc2b583c"
        ), "Unreviewed native Skip Disc handler"
        text = text[:start] + OLD_SKIP + text[end:]
    return text
