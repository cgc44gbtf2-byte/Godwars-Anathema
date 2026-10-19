"""Book budgets restrict casts; never widen class/command permissions."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values


def blob(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


class HybridBooksCatalog(unittest.TestCase):
    def test_native_page_bodies_unchanged_after_guards_removed(self):
        text = (SRC / "classes/sorcerer/sorcerer.c").read_text()
        self.assertEqual(text.count("if (!book_page_begin(ch, book, page, argument)) return;"), 3)
        self.assertNotIn("++chainspell > 10", text)
        text = text.replace('#include "classes/hybrid/books.h"\n', "")
        text = re.sub(r"/\* Shared across mixed page kinds\..*?\n}\n\n", "", text,
                      count=1, flags=re.S)
        for kind in ("damage", "affect", "action"):
            start = text.index("void adv_spell_" + kind + "(")
            end = text.find("\nvoid ", start + 1)
            if end < 0:
                end = len(text)
            body = text[start:end].replace(
                "    if (!book_page_begin(ch, book, page, argument)) return;\n", "")
            anchor = ("    if (ch->mana < cost)" if kind == "action"
                      else "    one_argument(argument, arg);")
            body = body.replace(anchor, "    extern long chainspell;\n\n"
                                "    if (++chainspell > 10) return;\n\n" + anchor, 1)
            text = text[:start] + body + text[end:]
        self.assertEqual(blob(text.encode()), "6e3d479485fdcaa9e8c19e8f5bc67f90eb391c31")

    def test_guard_precedes_page_initializers_and_is_bounded(self):
        text = (SRC / "classes/sorcerer/sorcerer.c").read_text()
        for kind in ("damage", "affect", "action"):
            body = text.split("void adv_spell_" + kind + "(", 1)[1].split("\nvoid ", 1)[0]
            self.assertLess(body.index("book_page_begin("), body.index("page->"))
        for fragment in ("chainspell < 0", "chainspell >= limit",
                         "!page", "!argument", "++chainspell;"):
            self.assertIn(fragment, text)

    def test_identity_budget_not_ready_build_or_equipment(self):
        text = (SRC / "classes/hybrid/books.c").read_text()
        self.assertIn("hybrid_player_save_valid(ch)", text)
        self.assertIn("hybrid_book_chain_policy", text)
        self.assertNotIn("hybrid_live_secondary", text)
        self.assertNotRegex(text, r"\b(?:get_eq_char|get_style|number_range|"
                            r"number_percent|hybrid_player_attach|fopen|SET_BIT)\s*\(")
        self.assertNotRegex(text, r"ch->\w+\s*=(?!=)")
        self.assertIn("return limit ? limit : 10", text)

    def test_native_getters_and_interpreter_unmodified(self):
        for path, expected in {
            "act_info.c": "9510008085cc695c0f58a563d62415b3c18cb8cd",
            "interp.c": "ccb0125d4264a8a5447618c29a9240ec674d2eb4",
        }.items():
            self.assertEqual(blob((SRC / path).read_bytes()), expected)

    def test_actual_engine_baselines_and_boundary_replays(self):
        self.assertIn("Genesis/src/classes/hybrid/books.c", make_values(ROOT, "ENGINE_SOURCES"))
        self.assertIn("tests/test_hybrid_books", make_values(ROOT, "TESTS"))
        make = (ROOT / "Makefile").read_text()
        self.assertIn("tests/test_hybrid_books: tests/test_hybrid_books.c $(ENGINE_OBJECTS)", make)
        self.assertRegex(make, r"HYBRID_ACCESS_SOURCES.*books.c")
        test = (ROOT / "tests/test_hybrid_books.c").read_text()
        for fragment in ("0x85c64faa8bc58818", "interpret(&actor", "LONG_MAX",
                         "LONG_MIN", "ADV_DAMAGE", "ADV_AFFECT", "ADV_ACTION",
                         "get_color(&actor,BLUE_MAGIC)==0", "hybrid_live_secondary",
                         "No equipment/stance budget swap", "actor.level=LEV_AWA",
                         "pages[0].specpower=1", "memcmp(native,hybrid"):
            self.assertIn(fragment, test)
        self.assertNotRegex(test, r"\b(?:void|long|int|bool)\s+(?:get_color|"
                            r"hybrid_player_save_valid|interpret|adv_spell_\w+)\s*\([^;]*\)\s*\{")


if __name__ == "__main__":
    unittest.main()
