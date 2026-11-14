"""Help-only source preservation and command-name regressions."""
import hashlib
from pathlib import Path
import re
import unittest

from class_help_source import (
    DRAGON_GUIDANCE, HIGHLAND_GUIDANCE, MONK_GUIDANCE, NINJA_GUIDANCE, SORCERER_GUIDANCE,
    PROOFS, without_class_help,
)
from elemental_help_source import ELEMENT_HELP

ROOT = Path(__file__).resolve().parents[1]


def blob(text):
    data = text.encode()
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


class ClassHelpCatalog(unittest.TestCase):
    def test_monk_training_guidance_matches_registered_command_and_existing_cost(self):
        commands = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0])
        self.assertEqual(next(fn for cmd, fn in commands if cmd.startswith("monklearn")),
                         "do_monklearn")
        source = (ROOT / "Genesis/src/classes/monk/monk.c").read_text()
        body = source.split("void do_monklearn(", 1)[1].split("\nvoid ", 1)[0]
        self.assertIn("10 x next rank primal; maximum 8.", MONK_GUIDANCE)
        self.assertIn("cost = (ch->pcdata->monk[improve] + 1) * 10;", body)
        self.assertIn("get_monk(ch, improve) >= 8", body)
        self.assertIn('!str_cmp(arg2, "gain")', body)

    def test_dragon_guidance_uses_registered_trainer_and_existing_prices(self):
        commands = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0])
        self.assertEqual(next(fn for cmd, fn in commands if cmd.startswith("dragonskill")),
                         "do_dragonskill")
        source = (ROOT / "Genesis/src/classes/dragon/dragon.c").read_text()
        body = source.split("void do_dragonskill(", 1)[1].split("\nvoid ", 1)[0]
        self.assertIn("cost = 1000 * (12 - age);", body)
        constants = dict(re.findall(
            r"#define\s+(AGE_\w+)\s+(\d+)",
            (ROOT / "Genesis/src/merc.h").read_text()))
        for names, price in (
            ("fangs/nightsight", 1000), ("claws/horns", 2000),
            ("wings/breath", 3000), ("tail", 4000), ("truesight", 5000),
            ("dust", 6000), ("magic/scry", 7000), ("readaura", 8000), ("shield", 9000),
        ):
            self.assertIn(f"{names} {price}", DRAGON_GUIDANCE)
            for name in names.split("/"):
                age = re.search(
                    r'!str_cmp\(arg, "' + name + r'"\)\)\s*\{age = (AGE_\w+);',
                    body).group(1)
                self.assertEqual(1000 * (12 - int(constants[age])), price)
        for age, dp, qp in (
            ("HATCHLING", 2500, 0), ("VYOUNG", 5000, 0), ("YOUNG", 7500, 0),
            ("JUVENILE", 10000, 0), ("YADULT", 15000, 500),
            ("ADULT", 25000, 1000), ("MATURE", 35000, 1500),
            ("OLD", 50000, 2000), ("VOLD", 70000, 2500), ("ANCIENT", 100000, 3000),
        ):
            case = re.search(r"case AGE_" + age + r":(.*?)break;", body).group(1)
            self.assertIn(f"cost = {dp};", case)
            if qp:
                self.assertIn(f"qpcost = {qp};", case)
            self.assertIn(f"{dp}/{qp}", DRAGON_GUIDANCE)
        for age in ("WYRM", "AWYRM"):
            self.assertNotIn("case AGE_" + age + ":", body)
        self.assertIn("Age purchases stop at Wyrm.", DRAGON_GUIDANCE)

    def test_sorcerer_training_guidance_matches_command_colors_cost_and_cap(self):
        commands = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0])
        self.assertEqual(next(fn for cmd, fn in commands if cmd.startswith("colors")),
                         "do_colors")
        source = (ROOT / "Genesis/src/classes/sorcerer/sorcerer.c").read_text()
        body = source.split("void do_colors(", 1)[1].split("\nvoid ", 1)[0]
        for color in ("purple", "red", "blue", "green", "yellow", "black"):
            self.assertIn(color, SORCERER_GUIDANCE)
            self.assertIn(f'!str_cmp(argument, "{color}")', body)
        self.assertIn("10 x next rank primal; maximum 5.", SORCERER_GUIDANCE)
        self.assertIn("cost = 10 * (get_color(ch, color) + 1);", body)
        self.assertIn("get_color(ch, color) >= 5", body)

    def test_ninja_training_guidance_uses_registered_command_and_existing_cost(self):
        commands = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0])
        self.assertEqual(next(fn for cmd, fn in commands if cmd.startswith("principle")),
                         "do_principles")
        source = (ROOT / "Genesis/src/classes/ninja/ninja.c").read_text()
        body = source.split("void do_principles(", 1)[1].split("\nvoid ", 1)[0]
        self.assertIn("10 x next rank primal; maximum 10.", NINJA_GUIDANCE)
        self.assertIn("cost = (get_ninja(ch, improve) + 1) * 10;", body)
        self.assertIn("get_ninja(ch, improve) >= 10", body)
        self.assertIn('!str_cmp(arg2, "improve")', body)

    def test_only_reviewed_help_changes_restore_exact_previous_sources(self):
        for path, expected, hunks in PROOFS:
            text = (ROOT / path).read_text()
            with self.subTest(path=path):
                self.assertEqual(blob(without_class_help(text)), expected)
                self.assertEqual(without_class_help(without_class_help(text)),
                                 without_class_help(text))
                self.assertNotEqual(blob(without_class_help(text + "\n/* mutation */\n")),
                                    expected)
                for new, _ in hunks:
                    self.assertEqual(text.count(new), 1)
                    mutated = text.replace(new, new + "/* mutation */", 1)
                    self.assertNotEqual(blob(without_class_help(mutated)), expected)

    def test_sphere_summaries_only_name_registered_commands(self):
        source = (ROOT / "Genesis/src/classes/mage/mage.c").read_text()
        help_text = source.split("void do_spheres(", 1)[1].split(
            "void do_clansphere(", 1)[0]
        commands = set(re.findall(
            r'\{\s*"([^"]+)"\s*,', (ROOT / "Genesis/src/interp.c").read_text()))
        for names in re.findall(r'powers include ([^.]+)\.', help_text):
            for name in re.split(r", | and ", names):
                self.assertTrue(name in commands, f"Unregistered Mage help command: {name}")
        self.assertIn("rift", commands)
        self.assertNotIn("eventually do fun things", help_text)

    def test_elementalist_help_matches_registered_handlers_and_actual_ranks(self):
        source = (ROOT / "Genesis/src/classes/elemental/elemental.c").read_text()
        commands = dict(re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0]))
        self.assertEqual(len(ELEMENT_HELP), 25)
        for element, _, rank, _, command in ELEMENT_HELP:
            handler = "do_cloak" if command == "cloakofsight" else "do_" + command
            self.assertEqual(commands.get(command), handler, command)
            body = re.split(r"\bvoid\s+" + handler + r"\s*\(", source, maxsplit=1)[1]
            body = body.split("\nvoid ", 1)[0]
            compact = re.sub(r"\s+", "", body)
            guard = f"ch->pcdata->powers[ELEM_{element}]<{rank}"
            if command == "sunbath":
                guard = "hybrid_power_rank(ch,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,ELEM_SUN)<3"
            self.assertTrue(guard in compact, f"Help rank differs from {handler}")

    def test_highlander_guidance_matches_registered_handlers_and_actual_ranks(self):
        source = (ROOT / "Genesis/src/classes/highlander/highland.c").read_text()
        commands = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0])
        for name, handler, rank in (
            ("bflash", "do_bladeflash", 2), ("bash", "do_bash", 3),
            ("htwist", "do_htwist", 4), ("judge", "do_judgement", 5),
            ("lunge", "do_lunge", 6),
        ):
            self.assertEqual(next(fn for cmd, fn in commands if cmd.startswith(name)),
                             handler)
            self.assertIn(f"{name} (Powers {rank})", HIGHLAND_GUIDANCE)
            body = re.split(r"\bvoid\s+" + handler + r"\s*\(", source, maxsplit=1)[1]
            body = re.split(r"\n(?:void|bool)\s+", body, maxsplit=1)[0]
            self.assertIn(f"HIGH_ABILITY)<{rank}", re.sub(r"\s+", "", body))
        self.assertEqual(next(fn for cmd, fn in commands if cmd.startswith("counter")),
                         "do_counter")
        counter = source.split("static bool counter_ready(", 1)[1].split(
            "\nstatic bool counter_equipped(", 1)[0]
        for path in ("HIGH_DEFENSE", "HIGH_ABILITY"):
            self.assertIn(f"{path})>=4", re.sub(r"\s+", "", counter))
        self.assertIn("counter (Powers 4 + Defense 4)", HIGHLAND_GUIDANCE)
        self.assertIn("15 x next rank primal; maximum 6.", HIGHLAND_GUIDANCE)

    def test_vampire_and_werewolf_advertisements_dispatch_correctly(self):
        commands = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
            (ROOT / "Genesis/src/interp.c").read_text().split("social_table")[0])
        exact = dict(commands)
        aliases = {
            "beckon": "do_beckon", "mirror": "do_mirrorimage",
            "clone": "do_cloneobject", "control": "do_control",
            "condition": "do_conditioning", "shroud": "do_shroudnight",
            "arms": "do_arms", "entrance": "do_entrancement",
        }
        for path, function, minimum in (
            ("clan.c", "do_powers", 70),
            ("classes/werewolf/werewolf.c", "do_wwpowers", 50),
        ):
            source = (ROOT / "Genesis/src" / path).read_text()
            body = source.split("void " + function + "(", 1)[1].split("\nvoid ", 1)[0]
            checked = 0
            for literal in re.findall(r'(?:strcpy|strcat)\(buf,\s*"([^"]+)"', body):
                for token in literal.split(":", 1)[-1].replace("\\n\\r", "").strip(" .").split():
                    if not token.isalpha():
                        continue
                    name = token.lower()
                    expected = exact.get(name, aliases.get(name))
                    actual = next((fn for cmd, fn in commands if cmd.startswith(name)), None)
                    self.assertIsNotNone(expected, f"Undocumented menu alias: {name}")
                    self.assertEqual(actual, expected, name)
                    checked += 1
            self.assertGreaterEqual(checked, minimum)


if __name__ == "__main__":
    unittest.main()
