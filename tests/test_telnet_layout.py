"""Transport ownership and build coverage; wire behavior is tested in C."""
import collections
import re
import unittest
from test_class_layout import ROOT, SRC, make_values


class TelnetLayoutChecks(unittest.TestCase):
    def test_one_production_owner(self):
        objects = make_values(SRC, "O_FILES")
        self.assertEqual(objects.count("telnet_input.o"), 1)
        source = (SRC / "telnet_input.c").read_text()
        self.assertIn("Original Diku Mud copyright", source)
        self.assertIn("God Wars Mud improvements copyright", source)
        owners = []
        for path in SRC.glob("*.c"):
            if re.search(r"^bool telnet_input\(", path.read_text(), re.M):
                owners.append(path.name)
        self.assertEqual(owners, ["telnet_input.c"])

    def test_one_engine_owner(self):
        objects = make_values(ROOT, "ENGINE_OBJECTS")
        self.assertEqual(objects.count("tests/stun_engine/telnet_input.o"), 1)

    def test_native_suite_and_isolated_fixture_link(self):
        tests = make_values(ROOT, "TESTS")
        self.assertEqual(tests.count("tests/test_telnet"), 1)
        self.assertFalse([x for x, n in collections.Counter(tests).items() if n > 1])
        build = (ROOT / "Makefile").read_text()
        self.assertRegex(build, r"tests/test_comm:[^\n]*\n[^\n]*Genesis/src/telnet_input.c")

    def test_socket_and_binary_paths(self):
        comm = (SRC / "comm.c").read_text()
        self.assertIn("telnet_input(d, raw, (size_t)nRead)", comm)
        self.assertIn("received < budget", comm)
        self.assertIn("memcpy(outbuf, d->outbuf, (size_t)d->outtop)", comm)
        self.assertNotIn("strncpy(outbuf, d->outbuf", comm)

    def test_copyover_versioned_and_legacy_readable(self):
        restart = (SRC / "restart.c").read_text()
        self.assertIn('HANDOFF_MAGIC "ANATHEMA_COPYOVER 2\\n"', restart)
        self.assertIn('HANDOFF_LEGACY_MAGIC "ANATHEMA_COPYOVER 1\\n"', restart)
        self.assertIn("telnet_state_valid", restart)


if __name__ == "__main__":
    unittest.main()
