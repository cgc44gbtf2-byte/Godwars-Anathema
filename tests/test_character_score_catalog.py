"""Character score only; all other info commands/calculators remain pinned."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from character_score_source import without_character_score_stage

class CharacterScoreCatalog(unittest.TestCase):
    def test_other_commands_and_getters_unchanged(self):
        data=without_character_score_stage((SRC/"act_info.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "1038824065d1d92b11b39c3c2b6a3151f54a51a5")

    def test_text_and_context_bounds(self):
        source=(SRC/"act_info.c").read_text().split("/* Character score scope:",1)[1].split("day_name",1)[0]
        for token in ("vsnprintf(buf, MSL, format, args)","MSL - 3","hashes % 2",
                      "ch == NULL || IS_NPC(ch) || ch->pcdata == NULL",
                      'ch->name : "Someone"','ch->pcdata->title : ""',
                      'ch->createtime : "unknown"',"obj_score(ch,ch->pcdata->chobj)"):
            self.assertIn(token,source)
        self.assertEqual(source.count("character_report(buf,"),2)
        self.assertIn("PK Score: unavailable",source)
        self.assertIn("Playing time unavailable",source)

    def test_local_legacy_arithmetic_order_is_checked(self):
        source=(SRC/"act_info.c").read_text().split("/* Character score scope:",1)[1].split("void do_score(",1)[0]
        for token in ("return get_hours_checked(ch, hours)",
                      "return get_ratio_checked(ch, ratio)"):
            self.assertIn(token,source)
        self.assertNotIn("ch->pkill =",source)
        self.assertNotIn("ch->played =",source)

    def test_whole_engine_output_and_oracles_wired(self):
        self.assertIn("tests/test_character_score",make_values(ROOT,"TESTS"))
        source=(ROOT/"tests/test_character_score.c").read_text()
        for token in ("rows==4224","4978974497349168729","pk_cases==196",
                      "hour_cases==2744","__int128","--native-dump","actor.stats[n]=5",
                      "actor.pdeath=-1","ACT_IS_NPC","obj_score(&actor,&obj)","memcmp(&saved,&pc"):
            self.assertIn(token,source)
        self.assertIn("test_character_score_catalog.py",(ROOT/"Makefile").read_text())
        self.assertNotIn("void do_score(",source)

    def test_scope_and_invalid_data_policy_documented(self):
        source=(ROOT/"docs/MESSAGES.md").read_text()
        for token in ("4,224","196","2,744","unavailable","get_hours","get_ratio",
                      "combat metric","object-form"):
            self.assertIn(token,source)

if __name__=="__main__":unittest.main()
