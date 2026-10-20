"""Only an explicit existing-record color route may reach paid training."""
import hashlib
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_vision_catalog import without_vision_info_adapter
from test_hybrid_creation_catalog import without_sorcerer_creation_adapter


class HybridColorsCatalog(unittest.TestCase):
    def test_preview_route_is_explicit_and_conversion_stays_off(self):
        text=(SRC/"classes/hybrid/selfbrid.c").read_text()
        self.assertIn("hybrid_colors_command(ch, argument)",text)
        self.assertLess(text.index("hybrid_colors_command("),text.index("Hybrid preview only"))
        self.assertNotIn("hybrid_training_commit(",text)
        self.assertNotIn("hybrid_player_attach(",text)
        ui=(SRC/"classes/hybrid/colors.c").read_text()
        self.assertIn('strcmp(args[0],"colors")',ui)
        self.assertIn("s->secondary_class!=CLASS_SORCERER",ui)
        self.assertIn("get_trust(ch)<LEV_SOR",ui)

    def test_atomic_backend_only_two_whitelisted_actions(self):
        text=(SRC/"classes/hybrid/colors.c").read_text()
        self.assertEqual(text.count("hybrid_training_commit("),1)
        self.assertIn("request.action=HYBRID_LESSON_RANK",text)
        self.assertIn("request.action=HYBRID_LESSON_SPECIALIZE",text)
        self.assertNotRegex(text,r"\b(?:hybrid_player_attach|hybrid_player_commit|"
                            r"do_colors|do_autoclass|fopen|fwrite|rename)\s*\(")
        self.assertNotRegex(text,r"ch->(?:class|primal|exp|pcdata->\w+)\s*(?:=(?!=)|[-+]=)")
        self.assertIn("hybrid_secondary_ranks_valid(s)",text)
        self.assertIn("HYBRID_LESSON_SAVE_UNCERTAIN",text)
        self.assertIn("Do not repeat",text)

    def test_focus_is_explicit_permanent_confirmation_no_implicit_learning(self):
        text=(SRC/"classes/hybrid/colors.c").read_text()
        for part in ('strcmp(args[3],"confirm")',"if(focus!=-1)","if(focus==-1)",
                     "Focus grants no ranks","if(n==15)return 0","const unsigned char *p"):
            self.assertIn(part,text)
        self.assertNotIn("one_argument(",text)

    def test_native_trainers_getters_and_command_table_unchanged(self):
        for path,sha in {
            "classes/sorcerer/sorcerer.c":"91fd66c8ba3a020d09525ca3fdbd13c33065812d",
            "act_info.c":"b24546978b3f3cf3a2191c8ec0803383bb89dea1",
            "interp.c":"ccb0125d4264a8a5447618c29a9240ec674d2eb4",
        }.items():
            data=(SRC/path).read_bytes()
            if path=="classes/sorcerer/sorcerer.c":
                data=without_sorcerer_creation_adapter(data.decode()).encode()
            if path=="act_info.c":
                data=without_vision_info_adapter(data.decode()).encode()
            self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),sha)

    def test_real_engine_files_reload_and_failure_coverage(self):
        self.assertIn("Genesis/src/classes/hybrid/colors.c",make_values(ROOT,"ENGINE_SOURCES"))
        self.assertIn("tests/test_hybrid_colors",make_values(ROOT,"TESTS"))
        test=(ROOT/"tests/test_hybrid_colors.c").read_text()
        for part in ("interpret(ch,line)","load_char_short", "SAVE_FAULT_DIRSYNC",
                     "SAVE_FAULT_RENAME","recovery is pending","save_char_obj_checked",
                     "assert(ch->pcdata->colors[i]==4)", "ch->primal==550",
                     "for(int focus=0;focus<6;focus++)"):
            self.assertIn(part,test)
        self.assertIn("tests/test_hybrid_colors: tests/test_hybrid_colors.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())


if __name__=="__main__":
    unittest.main()
