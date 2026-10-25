from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Genesis/src/classes/hybrid"


class HybridDrowTrainingCatalog(unittest.TestCase):
    def test_only_runtime_adapted_powers_are_exposed(self):
        ui = (SRC / "drow_training.c").read_text()
        for entry in ('{"attunement",DROW_ATTUNE,1000}',
                      '{"faeriefire",DROW_FIRE,5000}',
                      '{"acidbolt",DROW_ACID,5000}',
                      '{"llothsweb",DROW_WEB,8000}',
                      '{"readaura",DROW_READ,6000}',
                      '{"heighten",DROW_SIGHT,20000}',
                      '{"spidereyes",DROW_EYES,7500}'):
            self.assertIn(entry, ui)
        for unsupported in ("DROW_DARK",
                            "DROW_BITE", "DROW_DUALWIELD", "DROW_BFIGHT",
                            "DROW_SPIDER",
                            "DROW_NIGHT", "DROW_PIMM",
                            "DROW_SKIP", "DROW_MIGHT", "DROW_TOUGH"):
            self.assertNotIn(unsupported, ui)

    def test_lesson_uses_atomic_backend_and_secondary_record(self):
        ui = (SRC / "drow_training.c").read_text()
        self.assertIn("HYBRID_LESSON_DROW_POWER", ui)
        self.assertIn("hybrid_training_commit(ch,&request)", ui)
        self.assertIn("hybrid_live_secondary(ch)", ui)
        self.assertIn("s->secondary_class!=CLASS_DROW", ui)
        self.assertIn('strcmp(args[2],"learn")', ui)
        self.assertIn("hybrid_power_drow(ch,u->flag)", ui)
        self.assertIn("ch->class==CLASS_DROW", ui)

    def test_runtime_dispatch_and_build_manifest(self):
        dispatch = (SRC / "selfbrid.c").read_text()
        modules = (ROOT / "Genesis/src/classes/modules.mk").read_text()
        self.assertIn('#include "drow_training.h"', dispatch)
        self.assertIn("hybrid_drow_training_command(ch, argument)", dispatch)
        self.assertIn("classes/hybrid/drow_training.c", modules)
        self.assertIn("tests/test_hybrid_drow_training.c", (ROOT / "Makefile").read_text())

    def test_costs_match_native_drow_prayer(self):
        native = (ROOT / "Genesis/src/classes/drow/drow.c").read_text()
        trainer = (SRC / "training.c").read_text()
        self.assertIn('arg2, "attunement")) {grant = DROW_ATTUNE; cost = 1000;}', native)
        self.assertIn('arg2, "faeriefire")) {grant = DROW_FIRE; cost = 5000;}', native)
        self.assertIn('arg2, "acidbolt")) {grant = DROW_ACID; cost = 5000;}', native)
        self.assertIn("{DROW_FIRE,5000,0}", trainer)
        self.assertIn("{DROW_ACID,5000,0}", trainer)
        self.assertIn('arg2, "llothsweb")) {grant = DROW_WEB; cost = 8000;}', native)
        self.assertIn("{DROW_WEB,8000,0}", trainer)
        self.assertIn('arg2, "readaura")) {grant = DROW_READ; cost = 6000;}', native)
        self.assertIn("{DROW_READ,6000,0}", trainer)
        self.assertIn('arg2, "heighten")) {grant = DROW_SIGHT; cost = 20000;}', native)
        self.assertIn("{DROW_SIGHT,20000,0}", trainer)
        self.assertIn('arg2, "spidereyes")) {grant = DROW_EYES; cost = 7500;}', native)
        self.assertIn("{DROW_EYES,7500,0}", trainer)
        self.assertIn("{DROW_ATTUNE,1000,0}", trainer)


if __name__ == "__main__":
    unittest.main()
