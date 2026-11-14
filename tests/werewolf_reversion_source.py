"""Strict inverse of the reviewed Nightsight and class-reset cleanup changes."""
import hashlib
import re

BEFORE="6b13f9620e10f2d92936c581b7dcb1c16049d21f653471566472c3dadd231a1d"
AFTER="ec79059815d8607dc6afd14b460b86b3e5e8bb9efd230f6e409cabb62ff538c4"
HISTORICAL="088514797c0d9d611552c04deb844806a9219ccb90b9cddc0e60826fa7e3390a"
NEW='''    /* Cleanup must not require the rank that originally enabled the form. */
    if (IS_VAMPAFF(ch, VAM_NIGHTSIGHT))
    {
if (get_disc(ch, DISC_PROTEAN) < 1)
{
    send_to_char("The green glow in your eyes fades.\\n\\r",ch);
    act("The green glow in $n's eyes fades.", ch, NULL, NULL, TO_ROOM);
}
else
{
    send_to_char("The red glow in your eyes fades.\\n\\r",ch);
    act("The red glow in $n's eyes fades.", ch, NULL, NULL, TO_ROOM);
}
REMOVE_BIT(ch->vampaff, VAM_NIGHTSIGHT);
    }
'''
OLD='    if (IS_VAMPAFF(ch, VAM_NIGHTSIGHT)) do_nightsight(ch, "");\n'

def without_werewolf_reversion_stage(text):
    from drow_globe_source import without_drow_globe_stage
    text=without_drow_globe_stage(text)
    from shield_source import without_shield_stage
    text=without_shield_stage(text)
    from class_reset_source import without_class_reset_stage
    text=without_class_reset_stage(text)
    match=re.search(r"void do_unwerewolf\(.*?(?=\nvoid do_|\Z)",text,re.S)
    if match:
        digest=hashlib.sha256(match[0].encode()).hexdigest()
        if digest not in (BEFORE,HISTORICAL):
            assert digest==AFTER,("unexpected Werewolf reversion edit",digest)
            assert match[0].count(NEW)==1
            restored=match[0].replace(NEW,OLD)
            assert hashlib.sha256(restored.encode()).hexdigest()==BEFORE
            text=text[:match.start()]+restored+text[match.end():]
    if "void do_clearclass(" in text:
        digest=hashlib.sha256(text.encode()).hexdigest()
        if digest!="cc3145c446d9b1f41a3e3360c32f2dd0751a5cb552abf4426f1b3eb0e600034f":
            assert digest=="84754f0847dfdce18325f247ece2fe4ea9ff7e9e2e978e8ef9655a39d4961684"
            for line in ("    free_string(victim->morph);\n",
                         "    REMOVE_BIT(victim->act, PLR_WOLFMAN);\n"):
                assert text.count(line)==1
                text=text.replace(line,"",1)
            assert hashlib.sha256(text.encode()).hexdigest()==(
                "cc3145c446d9b1f41a3e3360c32f2dd0751a5cb552abf4426f1b3eb0e600034f")
    return text
