"""Shared combat ownership/build checks; gameplay is tested in the C suites."""
import collections
import re
import subprocess
import unittest

from test_class_layout import ROOT, SRC, make_values

OWNERS = {
    "style.c": ("get_style", "update_style", "do_grip", "improve_style",
                "improve_weapon"),
    "stance.c": ("do_skill", "improve_stance", "do_stance", "do_autostance",
                 "autostance", "dambonus"),
    "combat_state.c": ("set_fighting", "stop_fighting"),
    "combat_rules.c": ("is_safe", "auto_safe"),
    "combat_status.c": ("combat_pair_in_room", "combat_target_live",
                        "combat_status_update"),
    "combat_initiative.c": ("get_speed",),
    "combat_defense.c": ("get_soak", "check_avoid", "avoidblow", "breakavoid"),
    "combat_damage.c": ("damage", "hurt_fighting", "hurt_person",
                         "mod_damcap", "get_damcap"),
    "combat_death.c": ("update_pos", "make_corpse", "death_cry", "make_head",
                        "raw_kill", "behead", "death_message"),
    "combat_rewards.c": ("group_gain", "xp_compute", "dp_gain"),
}


class CombatLayoutChecks(unittest.TestCase):
    def test_one_definition_and_explicit_owner(self):
        owners = collections.defaultdict(list)
        for path in SRC.rglob("*.c"):
            source = re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(), flags=re.S)
            for fn in re.findall(
                r"(?m)^(?:void|bool|long)\s+(\w+)\s*\([^;{}]*\)\s*\{", source
            ):
                owners[fn].append(str(path.relative_to(SRC)))
        for owner, functions in OWNERS.items():
            for fn in functions:
                self.assertEqual(owners[fn], [owner], fn)

    def test_production_build_selects_each_owner_once(self):
        selected = make_values(SRC, "O_FILES")
        for owner in OWNERS:
            self.assertEqual(selected.count(owner.removesuffix(".c") + ".o"), 1)

    def test_native_test_build_selects_each_owner_once(self):
        selected = make_values(ROOT, "ENGINE_OBJECTS")
        for owner in OWNERS:
            self.assertEqual(
                selected.count("tests/stun_engine/" + owner.removesuffix(".c") + ".o"), 1
            )

    def test_cleanup_removes_new_native_artifacts(self):
        result = subprocess.run(
            ["make", "-n", "--no-print-directory", "clean"],
            cwd=SRC, check=True, capture_output=True, text=True,
        )
        for owner in OWNERS:
            self.assertIn(owner.removesuffix(".c") + ".o", result.stdout)

    def test_regression_is_part_of_default_test_and_sanitizer_path(self):
        selected = make_values(ROOT, "TESTS")
        self.assertEqual(selected.count("tests/test_stance_style"), 1)
        self.assertEqual(selected.count("tests/test_combat_calculations"), 1)
        self.assertEqual(selected.count("tests/test_combat_damage_death"), 1)
        makefile = (ROOT / "Makefile").read_text()
        self.assertRegex(makefile, r"(?s)sanitize:.*?\$\(MAKE\) test")

    def test_extracted_licensed_code_keeps_notices(self):
        for owner in ("style.c", "combat_state.c", "combat_rules.c",
                      "combat_initiative.c", "combat_defense.c",
                      "combat_damage.c", "combat_death.c", "combat_rewards.c"):
            source = (SRC / owner).read_text()
            self.assertIn("Original Diku Mud copyright", source)
            self.assertIn("Merc Diku Mud improvments copyright", source)
            self.assertIn("God Wars Mud improvements copyright", source)


if __name__ == "__main__":
    unittest.main()
