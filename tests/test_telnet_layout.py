"""Transport ownership, build coverage and deterministic smoke-wait regressions."""
import ast
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


class SmokeWaitChecks(unittest.TestCase):
    def wait(self, chunks, needle=None, timeout=25, trace=None, ready=None):
        class FakeSocketModule:
            class timeout(Exception):
                pass

        class Client:
            def settimeout(self, value):
                self.timeout = value

            def recv(self, count):
                value = chunks.pop(0)
                if value is None:
                    raise FakeSocketModule.timeout()
                return value

        class Clock:
            value = 0

            def monotonic(self):
                self.value += 1
                return self.value

        tree = ast.parse((ROOT / "tests/smoke_copyover.py").read_text())
        function = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                        and n.name == "until")
        namespace = {"time": Clock(), "socket": FakeSocketModule}
        exec(compile(ast.Module(body=[function], type_ignores=[]), "<smoke-wait>", "exec"),
             namespace)
        return namespace["until"](Client(), needle, timeout, trace, ready)

    def test_delayed_greeting_retries_read_not_connection(self):
        self.assertEqual(self.wait([None, None, b"greeting"]), b"greeting")

    def test_greeting_eof_fails_immediately(self):
        with self.assertRaisesRegex(AssertionError, "Connection closed"):
            self.wait([b""])

    def test_unresponsive_server_has_deadline(self):
        with self.assertRaisesRegex(AssertionError, "did not arrive"):
            self.wait([None] * 10, timeout=2)

    def test_partial_expected_response_waits(self):
        self.assertEqual(self.wait([None, b"Sa", None, b"ved."], b"Saved."),
                         b"Saved.")

    def test_unbounded_output_still_fails(self):
        with self.assertRaisesRegex(AssertionError, "unbounded"):
            self.wait([b"x" * 2_000_000], b"never")

    def test_text_needle_does_not_skip_fragmented_metadata(self):
        seen = bytearray()
        self.assertEqual(
            self.wait([b"Saved.", None, b"metadata"], b"Saved.",
                      trace=seen.extend, ready=lambda: b"metadata" in seen),
            b"Saved.metadata")


if __name__ == "__main__":
    unittest.main()
