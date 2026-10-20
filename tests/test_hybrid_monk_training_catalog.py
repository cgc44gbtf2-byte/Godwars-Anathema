"""Secondary-only UI; native command, lesson backend and disk format pinned."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values

def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

def blob(text):
    from file_numbers_source import without_file_number_save
    text=without_file_number_save(text)
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

class MonkTrainingCatalog(unittest.TestCase):
    def test_preview_restores_exact_previous_source(self):
        text=(SRC/"classes/hybrid/selfbrid.c").read_text()
        text=once(text,'#include "monk_training.h"\n',"")
        text=once(text,"    if (hybrid_monk_training_command(ch, argument)) return;\n","")
        text=once(text,"/* List/info remain read-only. Explicit colors/monk subcommands train only an",
                       "/* List/info remain read-only. An explicit colors subcommand can train only an")
        self.assertEqual(blob(text),"5d74784c64dbdd28ba5c547323bcab58be659437")

    def test_native_training_backend_getters_save_and_colors_are_unchanged(self):
        for path,sha in (("classes/monk/monk.c","051fe16da87e9b7647c819512e833893c22762b0"),
                         ("classes/hybrid/training.c","4646c2494470f6427efbba6da69752b32590c2a2"),
                         ("classes/hybrid/colors.c","3c060e0ff58f5ef05305e6c4921acc47fff1e36c"),
                         ("merc.h","b843f157662ab181daabe913d74685f1c8bc75e6"),
                         ("interp.c","ccb0125d4264a8a5447618c29a9240ec674d2eb4"),
                         ("save.c","3655e4f99b37b0d5165e58dc73d78c00b47564fc")):
            self.assertEqual(blob((SRC/path).read_text()),sha)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":3,"handoff":2})

    def test_exact_secondary_route_bounded_input_explicit_gain_and_confirmation(self):
        text=(SRC/"classes/hybrid/monk_training.c").read_text()
        for part in ('strcmp(args[0],"monk")',"char out[16]","if(n==15)return 0",
                     "ch->class==CLASS_MONK","s->secondary_class!=CLASS_MONK",
                     "get_trust(ch)<LEV_AWA",'strcmp(args[2],"gain")',
                     'strcmp(args[3],"confirm")',"focus!=-1","focus==-1"):
            self.assertIn(part,text)
        self.assertLess(text.index("ch->class==CLASS_MONK"),text.index("hybrid_live_secondary(ch)"))
        dispatch=(SRC/"classes/hybrid/selfbrid.c").read_text()
        self.assertLess(dispatch.index("hybrid_monk_training_command("),
                        dispatch.index("Hybrid preview only"))

    def test_only_atomic_backend_writes_and_effective_rank_display(self):
        text=(SRC/"classes/hybrid/monk_training.c").read_text()
        for part in ("hybrid_training_commit(ch,&request)","hybrid_secondary_ranks_valid(s)",
                     "hybrid_power_rank(ch,CLASS_MONK,HYBRID_TRAIN_MONK,selected)",
                     "hybrid_parent_training_cap","10*(rank+1)",
                     "HYBRID_LESSON_SAVE_UNCERTAIN","Do not repeat",
                     "HYBRID_LESSON_SAVE_PENDING","no ranks granted"):
            self.assertIn(part,text)
        self.assertNotRegex(text,r"\b(?:fopen|fwrite|rename|do_monklearn|hybrid_player_attach)\s*\(")
        self.assertNotRegex(text,r"ch->(?:primal|class|mana|hit|pcdata)\s*=(?!=)")
        self.assertNotIn("pcdata->monk[",text)

    def test_actual_interpreter_and_full_short_durable_tests(self):
        text=(ROOT/"tests/test_hybrid_monk_training.c").read_text()
        for part in ("confirmed_focuses==27","paid_lessons==432","reloads==918",
                     "ch->primal==440","ch->primal==820","load_char_short",
                     "SAVE_FAULT_DIRSYNC","SAVE_FAULT_RENAME","ch->desc=NULL",
                     "shell.original=ch","staff review","spiritguardian",
                     "interpret(ch,line)","ch->pcdata->monk[i]==5"):
            self.assertIn(part,text)
        self.assertIn("tests/test_hybrid_monk_training",make_values(ROOT,"TESTS"))
        self.assertIn("Genesis/src/classes/hybrid/monk_training.c",make_values(ROOT,"ENGINE_SOURCES"))
        self.assertNotRegex(text,r"\b(?:hybrid_training_commit|hybrid_player_commit)\s*\([^;]*\)\s*\{")

    def test_documented_commands_cost_caps_and_unfinished_scope(self):
        doc=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("selfbrid monk focus mantra confirm","selfbrid monk mantra gain",
                     "cap8","caps4","10 x next rank","432 paid","918 full/short",
                     "native `monklearn`","not every secondary power"):
            self.assertIn(part,doc)
        self.assertIn("selfbrid monk",(SRC/"classes/hybrid/help.c").read_text())

if __name__=="__main__":
    unittest.main()
