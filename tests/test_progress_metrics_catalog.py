"""Checked metric semantics plus exact source pins outside reviewed scopes."""
import hashlib
import re
import unittest
from test_class_layout import ROOT,SRC,make_values
from progress_metrics_source import without_progress_metrics_stage

class ProgressMetricsCatalog(unittest.TestCase):
    def test_all_other_source_in_six_modules_unchanged(self):
        expected={"handler.c":"9c2ee576804e8ce3dc58f300f3262aae957e47e1",
                  "random.c":"73d7cb87ce111b0969f2e20bcadd438212b69a84",
                  "comm.c":"925a22782183ae8e9269197b5504b4505d1962ae",
                  "act_wiz.c":"b5f5ce839115cb5176b2da130bf38b306149f00f",
                  "more.c":"94245671b79931e1749010e1779411d42f9bea83",
                  "act_info.c":"416f38b1b50f7cea9d3a0dc53cd3c2011eea7750"}
        for path,sha in expected.items():
            data=without_progress_metrics_stage((SRC/path).read_text()).encode()
            self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),sha,path)

    def test_shared_checked_apis_and_legacy_rounding(self):
        header=(SRC/"progress_metrics.h").read_text()
        for name in ("get_hours_checked","get_ratio_checked","grant_age_pk"):
            self.assertIn("bool "+name+"(",header)
        hours=(SRC/"handler.c").read_text().split("/* Shared played-time metrics scope:",1)[1].split("bool is_name(",1)[0]
        ratio=(SRC/"random.c").read_text().split("/* Shared PK-score metrics scope:",1)[1].split("void do_leaderclear(",1)[0]
        for token in ("__builtin_sub_overflow","elapsed, 0L","seconds / 3600","hours < AGE_PK",
                      "ch->pcdata == NULL","CAN_PK(ch)","FLAG_PK"):
            self.assertIn(token,hours)
        for token in ("__builtin_add_overflow","sum == 0","sum, sum","100L, ch->pkill",
                      "numerator, ch->pkill","numerator / denominator","*ratio = result"):
            self.assertIn(token,ratio)

    def test_no_policy_or_display_caller_uses_ambiguous_legacy_fallback(self):
        calls=[]
        for path in SRC.rglob("*.c"):
            for line in path.read_text().splitlines():
                if re.search(r"\bget_(?:hours|ratio)\s*\(",line):calls.append((path.name,line.strip()))
        self.assertEqual(sorted(calls),[("handler.c","long get_hours( CHAR_DATA *ch )"),
                                        ("random.c","long get_ratio(CHAR_DATA *ch)")])
        board=(SRC/"random.c").read_text().split("void check_leaderboard(",1)[1].split("/* Shared PK-score",1)[0]
        for token in ("have_ratio && ratio >","have_ratio && ratio <","have_hours && hours >"):
            self.assertIn(token,board)
        self.assertIn("if (grant_age_pk(ch))",(SRC/"comm.c").read_text())
        for path in ("act_wiz.c","more.c"):
            self.assertIn('"unavailable"',(SRC/path).read_text())

    def test_real_engine_suite_and_native_hash_wired(self):
        self.assertIn("tests/test_progress_metrics",make_values(ROOT,"TESTS"))
        source=(ROOT/"tests/test_progress_metrics.c").read_text()
        for token in ("rows==3200","1863285937676416231","grant_age_pk(&actor)",
                      "get_ratio_checked(&actor,&actual)","get_hours_checked(&actor,&actual)",
                      "__int128","load_leaderboard()","sandbox_begin","n<14"):
            self.assertIn(token,source)
        self.assertIn("test_progress_metrics_catalog.py",(ROOT/"Makefile").read_text())
        self.assertNotIn("bool grant_age_pk(",source)

    def test_rankings_and_existing_pk_flags_are_not_replaced_on_failure(self):
        source=(ROOT/"tests/test_progress_metrics.c").read_text()
        for token in ("actual==(valid?","!memcmp(&leader_board,&before",
                      "leader_board.bestpk_number==88","leader_board.tt_number==31",
                      "leader_board.mk_number==10","!grant_age_pk(&actor)&&IS_SET",
                      "access(\"../area/leader.txt\",F_OK)!=0"):
            self.assertIn(token,source)

    def test_scope_and_limits_documented(self):
        source=(ROOT/"docs/MESSAGES.md").read_text()
        for token in ("3,200","checked","eligibility","unavailable","unrelated counters",
                      "compatibility","parser","leaderboard"):
            self.assertIn(token,source)

if __name__=="__main__":unittest.main()
