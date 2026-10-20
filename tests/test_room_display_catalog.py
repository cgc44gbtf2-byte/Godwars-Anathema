"""Room/exit bounds supplement native full-engine output and policy tests."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from room_display_source import without_room_display_stage

class RoomDisplayCatalog(unittest.TestCase):
    def test_only_room_display_scopes_changed(self):
        data=without_room_display_stage((SRC/"act_info.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "7fad52f6e6da6626ea1f307c6b604ede8ab3af0c")

    def test_bounded_titles_and_exit_append(self):
        source=(SRC/"act_info.c").read_text()
        look=source.split("/* Room display scope:",1)[1].split("void do_examine(",1)[0]
        exits=source.split("/* Exit display scope:",1)[1].split("/* Score command redone",1)[0]
        for token in ("vsnprintf(buf + used, MSL - used","MSL - 3","hashes % 2",
                      'ch->in_room->name : "Somewhere"',"argument == NULL"):
            self.assertIn(token,look)
        self.assertEqual(look.count("room_report(buf, 0,"),6)
        self.assertNotIn('sprintf(buf, "#g%s',look)
        self.assertNotIn("strcat(",exits)
        self.assertNotIn("sprintf(",exits)
        self.assertIn("ch->in_room == NULL",exits)
        self.assertIn("strlen(buf)",exits)

    def test_visibility_and_navigation_policy_retained(self):
        source=(SRC/"act_info.c").read_text()
        look=source.split("void do_look(",1)[1].split("void do_examine(",1)[0]
        exits=source.split("void do_exits(",1)[1].split("/* Score command redone",1)[0]
        for token in ("WORLD_FOG","EX_CLOSED","IS_OUTSIDE(ch)","effective_nightsight(ch)","IS_BUI(ch)"):
            self.assertIn(token,exits)
        for token in ("effective_shadowsight(ch)","AFF_DEEPUMBRA","PLR_AUTOEXIT",
                      "char_to_room(ch,location)","EX_ISDOOR","check_blind(ch)",
                      "obj->value[2] >= LIQ_MAX","obj->pIndexData != NULL"):
            self.assertIn(token,look)

    def test_real_engine_native_goldens_and_boundary_paths(self):
        self.assertIn("tests/test_room_display",make_values(ROOT,"TESTS"))
        source=(ROOT/"tests/test_room_display.c").read_text()
        for token in ("rows==1792","10727128317469626511","n<15","WORLD_FOG",
                      "PLR_ANSI","actor.affected_by==before","actor.in_room==&room",
                      "room_index_hash[bucket]","--native-dump","long bucket"):
            self.assertIn(token,source)
        self.assertIn("tests/test_room_display_catalog.py",(ROOT/"Makefile").read_text())
        self.assertNotIn("void do_look",source)

    def test_clipping_and_remaining_scope_documented(self):
        source=(ROOT/"docs/MESSAGES.md").read_text()
        for token in ("1,792","detailed exit","liquid","prototype","character scores",
                      "navigation","visibility"):
            self.assertIn(token,source)

if __name__=="__main__":unittest.main()
