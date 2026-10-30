"""Reverse only the class-ritual recovery dispatch guard for historical pins."""

RECRUIT_GUARD = """      /* An accepted class ritual may be waiting for durable recovery. */
      if (cmd_table[cmd].do_fun == do_autoclass && ch->class == CLASS_NONE &&
          clan_block_class_change(ch))
         return;
"""

def without_class_recruitment_guard(text):
    assert text.count(RECRUIT_GUARD) <= 1
    return text.replace(RECRUIT_GUARD, "", 1)
