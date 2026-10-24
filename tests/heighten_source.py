"""Reverse only the reviewed Heighten handler, retaining every older source pin."""
import hashlib

OLD_SHA="2d24a16e96ec25603facfd6795286205b61296f9785cada2254c8fa9c9d83990"
SAFETY_SHA="daab1c4ad6bc7e694ffec9e46619a995f7b09e7fe95e826f6bba22323d546a4c"
FINAL_SHA="7e94f7e3ce428fadbaf61b37656d178d687b81c64fc4062d9aeed4e8c6a8387d"
GRANT="""    const bool monk_ready = hybrid_power_rank(ch, CLASS_MONK,
        HYBRID_TRAIN_MONK, MONK_MANTRA) >= 1;

"""
DISMISS='''    /* Turning a shared effect off must not require its former training. */
    if (IS_SET(ch->act, PLR_HOLYLIGHT))
    {
REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
stc("Your senses return to normal.\\n\\r", ch);
return;
    }

'''
OLD_TOGGLE='''    if (IS_SET(ch->act, PLR_HOLYLIGHT))
    {
\tREMOVE_BIT(ch->act, PLR_HOLYLIGHT);
\tstc("Your senses return to normal.\\n\\r", ch);
    }
    else
    {
\tSET_BIT(ch->act, PLR_HOLYLIGHT);
\tstc("Your senses increase to incredible proportions.\\n\\r", ch);
\tWAIT_STATE(ch, 6);
    }
'''
NEW_TOGGLE='''\tSET_BIT(ch->act, PLR_HOLYLIGHT);
\tstc("Your senses increase to incredible proportions.\\n\\r", ch);
\tWAIT_STATE(ch, 6);
'''

def without_heighten_stage(text):
    if "void do_truesight(" not in text:return text
    start=text.index("void do_truesight(")
    end=text.index("\nvoid do_soulmask(",start)
    current=text[start:end]
    digest=hashlib.sha256(current.encode()).hexdigest()
    if digest==OLD_SHA:return text
    assert digest in (SAFETY_SHA,FINAL_SHA),"Unreviewed Heighten edits"
    restored=current
    if digest==FINAL_SHA:
        assert restored.count(GRANT)==1
        restored=restored.replace(GRANT,"")
        assert restored.count("!monk_ready && ")==9
        restored=restored.replace("!monk_ready && ","")
    assert restored.count(DISMISS)==1 and restored.count(NEW_TOGGLE)==1
    restored=restored.replace(DISMISS,"").replace(NEW_TOGGLE,OLD_TOGGLE)
    restored=restored.replace("ch == NULL || IS_NPC(ch) || ch->pcdata == NULL","IS_NPC(ch)",1)
    assert hashlib.sha256(restored.encode()).hexdigest()==OLD_SHA,"Heighten inverse drift"
    return text[:start]+restored+text[end:]
