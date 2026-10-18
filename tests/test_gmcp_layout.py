"""GMCP module ownership and independent JSON/wire-probe regressions."""
import json
import unittest
from gmcp_probe import GMCPProbe
from test_class_layout import ROOT, SRC, make_values


class GMCPChecks(unittest.TestCase):
    def packet(self, package="Char.Vitals", value=None):
        value = {"hp": 17, "maxhp": 100} if value is None else value
        raw = json.dumps(value).encode().replace(b"\xff", b"\xff\xff")
        return b"\xff\xfa\xc9" + package.encode() + b" " + raw + b"\xff\xf0"

    def test_every_packet_split(self):
        data = b"\xff\xfb\xc9" + self.packet()
        for cut in range(len(data) + 1):
            probe = GMCPProbe()
            probe.feed(data[:cut]); probe.feed(data[cut:])
            self.assertEqual(probe.offers, 1)
            self.assertEqual(probe.records, [("Char.Vitals", {"hp": 17, "maxhp": 100})])

    def test_every_byte_and_reset(self):
        probe = GMCPProbe()
        for byte in b"\xff\xfc\xc9\xff\xfb\xc9" + self.packet("Anathema.Vitals", {"blood": None}):
            probe.feed(bytes([byte]))
        self.assertEqual((probe.resets, probe.offers), (1, 1))
        self.assertEqual(probe.records, [("Anathema.Vitals", {"blood": None})])

    def test_malformed_json_fails(self):
        with self.assertRaises(ValueError):
            GMCPProbe().feed(b"\xff\xfa\xc9Char.Vitals {bad}\xff\xf0")

    def test_payload_bound(self):
        with self.assertRaisesRegex(AssertionError, "Oversized"):
            GMCPProbe().feed(b"\xff\xfa\xc9" + b"x" * 8193)

    def test_one_owner_each_build(self):
        self.assertEqual(make_values(SRC, "O_FILES").count("gmcp.o"), 1)
        self.assertEqual(make_values(ROOT, "ENGINE_OBJECTS").count("tests/stun_engine/gmcp.o"), 1)
        self.assertEqual(make_values(ROOT, "TESTS").count("tests/test_gmcp"), 1)

    def test_autonomous_tick_hooks_and_privacy_gate(self):
        comm = (SRC / "comm.c").read_text()
        self.assertEqual(comm.count("if (!gmcp_update(d))"), 2)
        self.assertIn("if (!gmcp_offer(dnew))", comm)
        gmcp = (SRC / "gmcp.c").read_text()
        self.assertIn("descriptor_is_in_game(d)", gmcp)
        self.assertIn("ch->desc != d", gmcp)
        self.assertIn("d->original != NULL", gmcp)
        self.assertIn("MORE_HEAD", gmcp)

    def test_copyover_keeps_existing_manifest_version(self):
        restart = (SRC / "restart.c").read_text()
        self.assertIn('HANDOFF_MAGIC "ANATHEMA_COPYOVER 2\\n"', restart)
        self.assertIn("if (!gmcp_recover(d))", restart)
        row = restart.split("struct handoff_row {", 1)[1].split("};", 1)[0]
        self.assertFalse("gmcp_state" in row, "GMCP state must not enter the handoff")
        self.assertIn("d->telnet_output_state != TELNET_DATA", restart)


if __name__ == "__main__":
    unittest.main()
