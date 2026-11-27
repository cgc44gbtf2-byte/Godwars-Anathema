"""Metis Totem Gift registration, transient lifetime and balance contract."""
import unittest
from test_class_layout import ROOT, SRC


class WerewolfTotemCatalog(unittest.TestCase):
    def test_native_metis_rank_five_command_and_help(self):
        helpfile = (ROOT / "Genesis/area/class.hlp").read_text()
        metis = helpfile.split("0 Metis~", 1)[1].split("~", 1)[0]
        self.assertRegex(metis, r"5: Totem Gift - .*<totemgift>")
        self.assertIn("10 pulses", metis)
        self.assertIn("10,000", metis)
        self.assertIn("5,000 mana", metis)

        table = (SRC / "interp.c").read_text()
        self.assertRegex(table, r'\{\s*"totemgift"\s*,\s*do_totemgift\s*,\s*POS_FIGHTING')
        body = (SRC / "classes/werewolf/totem.c").read_text()
        self.assertIn("!IS_WEREWOLF(ch)", body)
        self.assertIn("get_breed(ch, BREED_METIS) < 5", body)
        self.assertIn("get_tribe(ch, tribe) < 1", body)

    def test_only_external_positive_damage_uses_the_pool(self):
        damage = (SRC / "combat_damage.c").read_text()
        self.assertIn("if (ch != victim) dam = werewolf_totem_absorb(victim, dam);", damage)
        body = (SRC / "classes/werewolf/totem.c").read_text()
        for value in ("METIS_TOTEM_MANA 5000", "METIS_TOTEM_ABSORPTION 10000",
                      "METIS_TOTEM_DURATION 10"):
            self.assertIn(value, body)
        self.assertIn("damage -= absorbed", body)
        self.assertIn("totem->wear_flags = 0", body)
        self.assertIn("act(\"$n calls a spirit totem", body)
        self.assertNotIn("multi_hit(", body)

    def test_temporary_object_and_barrier_are_not_saved(self):
        header = (SRC / "merc.h").read_text()
        self.assertIn("metis_totem_timer", header)
        self.assertIn("metis_totem_absorb", header)
        for path in ("save.c", "player_save.c"):
            self.assertNotIn("metis_totem_", (SRC / path).read_text())

    def test_expiry_movement_training_and_class_loss_clear_the_object(self):
        self.assertIn("werewolf_totem_update(ch);", (SRC / "combat_status.c").read_text())
        self.assertIn("werewolf_totem_clear(ch);", (SRC / "handler.c").read_text())
        self.assertIn("werewolf_totem_clear(victim);", (SRC / "clearclass.c").read_text())
        body = (SRC / "classes/werewolf/totem.c").read_text()
        self.assertIn("get_breed(ch, BREED_METIS) < 5", body)
        self.assertIn("ch->position == POS_DEAD", body)

    def test_runtime_suite_is_registered(self):
        makefile = (ROOT / "Makefile").read_text()
        self.assertIn("tests/test_werewolf_totem", makefile)
        test = (ROOT / "tests/test_werewolf_totem.c").read_text()
        for assertion in ("== 10000", "== 10", "actors[0].mana = 5000",
                          "char_from_room", "combat_status_update", "hurt_person"):
            self.assertIn(assertion, test)


if __name__ == "__main__":
    unittest.main()
