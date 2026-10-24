"""Scry changes are confined to one handler, with real native-engine coverage."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values
from scry_source import without_scry_stage

class ScryCatalog(unittest.TestCase):
    def body(self):
        return (SRC/"clan.c").read_text().split("void do_scry(",1)[1].split(
            "\nvoid do_readaura(",1)[0]

    def test_entire_shared_module_restores_exact_previous_source(self):
        text=without_scry_stage((SRC/"clan.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(text)).encode()+b"\0"+text).hexdigest(),
                         "bf76a113a1025f153744556f9f6b0276d64c4a6b")

    def test_invalid_context_is_rejected_before_parser_or_lookup(self):
        body=self.body()
        self.assertIn("ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL",body)
        self.assertLess(body.index("ch == NULL"),body.index("one_argument("))
        self.assertIn('argument != NULL ? argument : ""',body)
        self.assertIn("== NULL || victim->in_room == NULL",body)

    def test_bounded_list_labels_and_locations(self):
        body=self.body()
        self.assertEqual(body.count("snprintf("),2)
        for fragment in ('sizeof(": ")','sizeof(".\\n\\r")',
                         'label != NULL ? label : "Someone"',
                         'victim->in_room->name != NULL ? victim->in_room->name : "(unknown)"'):
            self.assertIn(fragment,body)
        self.assertNotIn('sprintf(buf,"%s',body)

    def test_independent_learned_monk_source_only(self):
        body=self.body()
        for fragment in ("HYBRID_TRAIN_MONK, MONK_MANTRA) >= 2",
                         "&& !IS_BUI(ch) && !monk_ready)",
                         "if (!IS_ITEMAFF(ch,ITEMA_VISION) && !monk_ready)",
                         "if (IS_MAGE(ch) && !monk_ready)"):
            self.assertIn(fragment,body)
        self.assertEqual(body.count("hybrid_power_rank("),1)
        fixture=(ROOT/"tests/test_hybrid_scry.c").read_text()
        for fragment in ("scry_pairs();scry_bad_records();","count==23328","count==72",
                         "bad<5","LONG_MIN","scry_loaders();"):
            self.assertIn(fragment,fixture)
        self.assertIn("scry (Mantra2; free)",(SRC/"classes/hybrid/help.c").read_text())

    def test_native_privacy_warning_and_remote_state_policies_retained(self):
        body=self.body()
        for fragment in ("d->connected != CON_PLAYING","IS_BUI(victim) && !IS_BUI(ch)",
                         "MORE_SHIELDED","get_disc(ch, DISC_AUSPEX) >= 6",
                         "get_breed(ch, BREED_LUPUS) == 5",
                         "get_breed(ch,BREED_HOMID) < 5","guardian_message( victim )",
                         "ch->mana -= 500","chroom = ch->in_room","ch->in_room = chroom",
                         "AFF_SHADOWPLANE","DROW_GLOBE"):
            self.assertIn(fragment,body)
        self.assertNotIn("save_char",body)

    def test_real_engine_fixture_and_compatibility(self):
        self.assertIn("tests/test_hybrid_scry",make_values(ROOT,"TESTS"))
        fixture=(ROOT/"tests/test_hybrid_scry.c").read_text()
        for fragment in ("interpret(ch,command)","count==1440","scry_safety(i)",
                         "memcmp(&before,ch->pcdata","number_range(0,1000000)",
                         "native-dump","load_char_short","save_char_obj_checked"):
            self.assertIn(fragment,fixture)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_safety_documentation(self):
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for fragment in ("Native Scry safety","six sanitizer failures",
                         "1,440 native","missing target room","No conversion"):
            self.assertIn(fragment,docs)

if __name__=="__main__":
    unittest.main()
