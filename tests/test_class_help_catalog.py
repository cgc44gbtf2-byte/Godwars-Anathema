"""Help-only source preservation and command-name regressions."""
import hashlib
from pathlib import Path
import re
import unittest

from class_help_source import HIGHLAND_GUIDANCE, PROOFS, without_class_help
from elemental_help_source import ELEMENT_HELP

ROOT = Path(__file__).resolve().parents[1]


def blob(text):
    data = text.encode()
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


class ClassHelpCatalog(unittest.TestCase):
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
