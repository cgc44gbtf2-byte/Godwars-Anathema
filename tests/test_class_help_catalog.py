"""Help-only source preservation and command-name regressions."""
import hashlib
from pathlib import Path
import re
import unittest

from class_help_source import PROOFS, without_class_help

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


if __name__ == "__main__":
    unittest.main()
