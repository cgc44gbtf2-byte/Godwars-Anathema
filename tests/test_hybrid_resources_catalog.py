"""Guard runtime-accounting boundaries; engine behavior is tested in C."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT, SRC, make_values


class HybridResourceCatalog(unittest.TestCase):
    def test_native_update_except_owned_hook_and_checked_debits(self):
        text=(SRC/"update.c").read_text()
        include='#include "classes/hybrid/resources.h"\n'
        hook='\t    hybrid_secondary_mobile_update(ch);\n'
        self.assertEqual(text.count(include),1)
        self.assertEqual(text.count(hook),1)
        text=text.replace(include,"").replace(hook,"")
        one=("    ch->blood[BLOOD_CURRENT] = ch->blood[BLOOD_CURRENT] > 0 ?\n"
             "        ch->blood[BLOOD_CURRENT] - 1 : 0;")
        five=("    ch->blood[BLOOD_CURRENT] = ch->blood[BLOOD_CURRENT] > 5 ?\n"
              "        ch->blood[BLOOD_CURRENT] - 5 : 0;")
        self.assertEqual(text.count(one),1)
        self.assertEqual(text.count(five),2)
        restored=text.replace(one,"\t\t    ch->blood[BLOOD_CURRENT]--;").replace(
            five,"\t\t    ch->blood[BLOOD_CURRENT] -= 5;").encode()
        digest=hashlib.sha1(b"blob "+str(len(restored)).encode()+b"\0"+restored).hexdigest()
        self.assertEqual(digest,"7095ed57455edd4950fa766a55d69810e8122c4d")
        original=(SRC/"update.c").read_text()
        self.assertLess(original.index("else if (IS_DROW(ch))"),
                        original.index("hybrid_secondary_mobile_update(ch);"))
        self.assertLess(original.index("hybrid_secondary_mobile_update(ch);"),
                        original.index("/* Examine call for special procedure */"))

    def test_real_engine_wiring(self):
        selected=make_values(ROOT,"ENGINE_SOURCES")
        self.assertEqual(selected.count("Genesis/src/classes/hybrid/resources.c"),1)
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_hybrid_resources",make)
        self.assertIn("tests/test_hybrid_resources.c $(ENGINE_OBJECTS)",make)
        self.assertIn("python3 tests/test_hybrid_resources_catalog.py",make)

    def test_primary_storage_not_mutated(self):
        text=(SRC/"classes/hybrid/resources.c").read_text()
        self.assertNotRegex(text,r"ch->(?:blood|quint)\[[^\]]+\]\s*[-+*/]?=")
        self.assertNotRegex(text,r"ch->pcdata->(?:ninja|classpoints)\[[^\]]+\]\s*[-+*/]?=")
        self.assertNotRegex(text,r"ch->(?:class|vampgen|arete|paradox)\s*[-+*/]?=(?!=)")
        self.assertEqual(text.count("memcpy(&ch->pcdata->hybrid,"),2)
        self.assertIn("hybrid_state_valid(&candidate)",text)

    def test_no_inferred_activation_or_randomness(self):
        text=(SRC/"classes/hybrid/resources.c").read_text()
        self.assertNotRegex(text,r"\b(?:SET_BIT|REMOVE_BIT|TOGGLE_BIT|number_range|number_percent|interpret|do_\w+)\s*\(")
        self.assertIn("hybrid_live_secondary(ch)",text)
        self.assertIn("!IS_PLAYING(ch->desc)",text)
        self.assertIn("amount>LONG_MAX-*total",text)
        self.assertIn("amount>capacity-*slot",text)

    def test_existing_wire_epochs_remain(self):
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual(compat["save_epoch"],3)
        self.assertEqual(compat["handoff"],2)
        state=(SRC/"classes/hybrid/state.c").read_text()
        self.assertIn('AnathemaHybrid 4',state)

    def test_no_conversion_trainer_or_durable_payment(self):
        command=(SRC/"classes/hybrid/selfbrid.c").read_text()
        runtime=(SRC/"classes/hybrid/resources.c").read_text()
        for name in ("hybrid_player_attach","hybrid_player_commit",
                     "save_char_obj_checked","hybrid_training_commit"):
            self.assertNotIn(name+"(",runtime)
        self.assertNotIn("hybrid_player_attach(",command)
        self.assertNotIn("hybrid_secondary_resource_",command)


if __name__=="__main__":
    unittest.main()
