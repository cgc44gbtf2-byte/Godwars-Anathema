"""Scope and backward-reader checks for optional Favor lifetime metadata."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values
from test_hybrid_celestial_catalog import without_celestial_stage

def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

def without_favour_metadata_monk(text):
    text=without_celestial_stage(text)
    text=once(text,'#include "classes/monk/favour.h"\n',"")
    text=once(text,
        '    if (!monk_favour_clock_valid(&time_info))\n    {\n'
        'stc("The blessing is unavailable while the world clock is invalid.\\n\\r", ch);\n'
        'return;\n    }\n\n',"")
    text=once(text,"monk_favour_grant(vch, &time_info);","SET_BIT(vch->more, MORE_FAVOUR);")
    text=once(text,"monk_favour_grant(victim, &time_info);","SET_BIT(victim->more, MORE_FAVOUR);")
    return once(text,"monk_favour_clear(vch);","REMOVE_BIT(vch->more, MORE_FAVOUR);")

def without_favour_metadata_merc(text):
    return once(text,"    long                favour_since; /* Optional next-dawn metadata, game hours. */\n"
                     "    long                favour_until;\n","")

def without_favour_metadata_save(text):
    text=once(text,'#include "classes/monk/favour.h"\n',"")
    text=once(text,'    fprintf( fp, "More         %li\\n",monk_favour_saved_more(ch, &time_info));\n'
                   '    monk_favour_write(fp, ch, &time_info);',
                   '    fprintf( fp, "More         %li\\n",  ch->more\t\t);')
    text=once(text,'    if (!str_cmp(word, "*AnathemaFavor")) monk_favour_read(ch, fp);\n'
                   '    else fread_to_eol( fp );','\t    fread_to_eol( fp );')
    return once(text,"                monk_favour_reconcile(ch, &time_info);\n","")

class FavourExpiryCatalog(unittest.TestCase):
    def test_native_bodies_restore_exact_base_blobs(self):
        for path,normalizer,sha in (
            ("classes/monk/monk.c",without_favour_metadata_monk,"9548574faf5f46918c83f7f87c5eb8b88f606e0e"),
            ("merc.h",without_favour_metadata_merc,"0a247f670d224d6d24c7309fc5b975b15cea3ee2"),
            ("save.c",without_favour_metadata_save,"fdf27500c781cb876a21175fceafe5a039ccd2ad")):
            self.assertEqual(blob(normalizer((SRC/path).read_text())),sha)

    def test_optional_metadata_uses_legacy_comment_branch(self):
        text=(SRC/"save.c").read_text()
        branch=text.split("case '*':",1)[1].split("case 'A':",1)[0]
        self.assertIn('if (!str_cmp(word, "*AnathemaFavor"))',branch)
        self.assertIn("else fread_to_eol( fp );",branch)
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual((compat["save_epoch"],compat["handoff"]),(3,2))
        self.assertIn("monk_favour_reconcile(ch, &time_info);",text)

    def test_parser_is_bounded_and_only_clears_favor(self):
        text=(SRC/"classes/monk/favour.c").read_text()
        for part in ("char line[128]","LONG_MAX-digit","c!='\\n'",
                     "bool duplicate=","version!=1","now>=ch->favour_since",
                     "now<ch->favour_until","&~MORE_FAVOUR"):
            self.assertIn(part,text)
        self.assertEqual(text.count("REMOVE_BIT("),1)
        self.assertNotIn("MORE_CLOAK",text)
        self.assertNotIn("player_load_failed",text)

    def test_both_real_loaders_and_calendar_cases(self):
        text=(ROOT/"tests/test_favour_expiry.c").read_text()
        self.assertIn("casts==42840",text)
        self.assertIn("elapsed<=24",text)
        self.assertIn("t.year=LONG_MAX",text)
        self.assertIn("tests/test_favour_expiry",make_values(ROOT,"TESTS"))
        text=(ROOT/"tests/test_hybrid_player.c").read_text()
        for part in ("static void favour_expiry(void)","if(expired)time_info.hour=6",
                     "ch->more&keep","monk_favour_grant(ch, &time_info)"):
            self.assertIn(part,text)

    def test_docs_explain_legacy_and_clock_semantics(self):
        text=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("Offline Favor expiry","*AnathemaFavor",
                     "unverifiable legacy","game-calendar","older binary"):
            self.assertIn(part,text)

if __name__=="__main__":
    unittest.main()
