"""The only act_info changes are independently characterized item reports."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from item_info_source import without_item_info_stage

class ItemInspectionCatalog(unittest.TestCase):
    def test_all_other_info_commands_and_getters_remain_pinned(self):
        data=without_item_info_stage((SRC/"act_info.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "d4b24a952210f67fbfadabb480e9908422c72280")

    def test_bounded_text_and_invalid_data(self):
        source=(SRC/"act_info.c").read_text()
        body=source.split("/* Item score reporting scope:",1)[1].split("/* Do_prompt",1)[0]
        for token in ("vsnprintf(buf, size, format, args)","size - 3","hashes % 2",
                      "obj == NULL","obj->pIndexData != NULL ?",
                      'obj->short_descr != NULL ? obj->short_descr : "something"',
                      "itemtype < MAX_SKILL && skill_table[itemtype].name != NULL"):
            self.assertIn(token,body)
        self.assertNotIn("sprintf( buf, \"You were created by",body)

    def test_comparison_not_based_on_rounded_average(self):
        source=(SRC/"act_info.c").read_text()
        body=source.split("/* Item comparison scope:",1)[1].split("void do_credits(",1)[0]
        for token in ("a / 2 + b / 2","a % 2 + b % 2","remainders[n] < 0",
                      "remainders[n] > 1","item_damage_compare(obj1, obj2)",
                      "obj1 == obj2","can_see_obj( ch, obj2 )"):
            self.assertIn(token,body)
        self.assertNotIn("obj1->value[1] + obj1->value[2]",body)

    def test_whole_engine_suite_is_wired_and_pinned(self):
        self.assertIn("tests/test_item_inspection",make_values(ROOT,"TESTS"))
        source=(ROOT/"tests/test_item_inspection.c").read_text()
        for token in ("rows==432","rows==300","13185047630302027763",
                      "2747055546945648736","__int128","a<13","d<13",
                      'char arg[]="silver other"',"obj_score(&actor,&obj)"):
            self.assertIn(token,source)
        self.assertNotIn("void do_compare",source)
        self.assertIn("test_item_inspection_catalog.py",(ROOT/"Makefile").read_text())

    def test_limits_and_scope_are_documented(self):
        source=(ROOT/"docs/MESSAGES.md").read_text()
        for token in ("object-form","732","28,561","compare","exit","cyclic"):
            self.assertIn(token,source)

if __name__=="__main__":unittest.main()
