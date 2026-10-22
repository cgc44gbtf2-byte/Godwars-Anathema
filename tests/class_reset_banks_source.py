"""Narrow, exact inverse of primary bank cleanup; reject neighboring edits."""
import hashlib

BEFORE = "f8788c39c07bee30407c4f47e936558366c82d3bcc40abf4027415e14659f9e4"
BLOCK = """    /* These primary banks must not outlive the class being cleared.
     * Absorb belongs to Vampire Absorb Soul, not Sorcerer color training. */
    memset(victim->spheres,0,sizeof victim->spheres);
    memset(victim->quint,0,sizeof victim->quint);
    victim->arete = 0;
    victim->paradox = 0;
    victim->paradox_ward = 0;
    victim->beginners_luck = 0;
    victim->doublecast_timer = 0;
    memset(victim->pcdata->colors,0,sizeof victim->pcdata->colors);
    memset(victim->pcdata->absorb,0,sizeof victim->pcdata->absorb);
"""

def without_class_reset_banks(text):
    if "void do_clearclass(" not in text or BLOCK not in text:
        return text
    assert text.count(BLOCK) == 1
    old = text.replace(BLOCK, "", 1)
    assert hashlib.sha256(old.encode()).hexdigest() == BEFORE
    return old
