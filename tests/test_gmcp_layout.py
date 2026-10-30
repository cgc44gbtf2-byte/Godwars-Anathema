"""GMCP module ownership and independent JSON/wire-probe regressions."""
import hashlib
import json
import unittest
from gmcp_probe import GMCPProbe
from test_class_layout import ROOT, SRC, make_values


class GMCPChecks(unittest.TestCase):
    def test_nonpie_is_only_a_sanitizer_setting(self):
        source=(ROOT/"Makefile").read_text()
        recipe=source.split("sanitize:\n",1)[1].split("\nclean:",1)[0]
        self.assertIn("-fno-pie",recipe)
        self.assertIn("-no-pie",recipe)
        self.assertNotIn("-fno-pie",source.split("sanitize:\n",1)[0])
        self.assertNotIn("-no-pie",(SRC/"Makefile").read_text())

    def test_ci_keeps_checks_and_validates_sanitizer_runtime(self):
        ci=(ROOT/".github/workflows/ci.yml").read_text()
        self.assertIn("python3 tests/sanitizer_startup.py",ci)
        self.assertIn("-fno-pie",ci)
        self.assertIn('-no-pie"',ci)
        self.assertIn("make sanitize",ci)
        self.assertNotIn("continue-on-error",ci)
        self.assertNotIn("randomize_va_space",ci)
        self.assertNotIn("mmap_rnd_bits",ci)

    def test_vitals_and_transport_unchanged(self):
        source=(SRC/"gmcp.c").read_text()
        start=source.index("/* Normal player names are ASCII letters")
        end=source.index("bool gmcp_update(",start)
        source=source[:start]+(
            "/* Only literals and signed integers enter JSON. No names, room identifiers,\n"
            " * opponents, secrets or user-provided text need JSON/Telnet escaping here. */\n"
        )+source[end:]
        changes = (
            ("    char name[385];\n", ""),
            ("    identity_name(name, ch ? ch->name : NULL);\n", ""),
            ('        "%c%c%cChar.Base {\\"name\\":\\"%s\\",\\"level\\":%ld,\\"class\\":\\"%s\\"}%c%c"\n', ""),
            ('        255,250,GMCP_OPTION,name,ch ? ch->level : 0L,\n'
             '        ch ? class_name(ch->class) : "none",255,240,\n', ""),
        )
        for new,old in changes:
            self.assertEqual(source.count(new),1)
            source=source.replace(new,old,1)
        data=source.encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "d4a23865b8f9ce270fd02c88493b1744b3967aae")

    def test_identity_uses_existing_privacy_and_bounded_cache(self):
        source=(SRC/"gmcp.c").read_text()
        self.assertEqual(source.count("Char.Base "),1)
        self.assertLess(source.index("if (!descriptor_is_in_game(d))"),
                        source.index("identity_name(name,"))
        self.assertIn("i < 64",source)
        self.assertIn("count >= sizeof(frame)",source)
        self.assertIn("ch ? ch->level : 0L",source)
        self.assertIn("ch ? class_name(ch->class)",source)

    def test_identity_json_every_split(self):
        for name in ('Luto','"\\\n',"\u00ff", "A"*64, ""):
            value={"name":name,"level":7,"class":"werewolf"}
            data=self.packet("Char.Base",value)
            for split in range(len(data)+1):
                probe=GMCPProbe()
                probe.feed(data[:split]);probe.feed(data[split:])
                self.assertEqual(probe.records,[("Char.Base",value)])

    def test_mana_alias_json_across_every_split(self):
        for mana,maxmana in ((0,0),(-1,100),(33,99),(-(2**63),2**63-1)):
            value={"hp":1,"maxhp":100,"mp":mana,"maxmp":maxmana,
                   "mana":mana,"maxmana":maxmana}
            wire=self.packet("Char.Vitals",value)
            for split in range(len(wire)+1):
                probe=GMCPProbe()
                probe.feed(wire[:split]);probe.feed(wire[split:])
                self.assertEqual(probe.records,[("Char.Vitals",value)])
                data=probe.records[0][1]
                self.assertEqual(data["mp"],data["mana"])
                self.assertEqual(data["maxmp"],data["maxmana"])

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
