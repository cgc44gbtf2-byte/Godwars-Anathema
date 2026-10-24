"""Reverse exact owned reader extraction/guards, not whole source modules."""
import base64
import zlib
OLD_DISABLED="eJx9VNFu2jAUfecr7vIw2SkDnovYxASVqmVMarunqoqS2CkWjm05SelW8e+zDQk2QfMDwc7xveece2+mMSQyI0BYneWcEihkVWWC1BBPRwDwJhkBbhBph0DYnn/YH7Pu7pM1xKWan/ar+8fl92S9SlfLpyXE/XmxzTTEIqtod8KleAXmdqeTLkNaMl03sIDN7yTxAaUyh6VUVADqE1kGY4h0hH0oKwF9KhWGaQwbeZZXMk4nExDyiuLbk+bj0rRptfBjWvaWgKbGjr3UBJDJ4CP2WxMeUN3otKgUshfGsN6s0p/Lhx/rB+zoZPVRu3m6iKyGza8nDwa3+Mzk48zotEyIOyYINFvacQcm3LaxigIVpdSAmGE9m0NRkdQhntnLxKZ+nr3AHNjNDQ6SOO86DYNLY8ca40teubFl1xV3FAa7ltl50fEnkho3ZAP0ndXNt0BC6EDevgKKHndMKWZMbHdC7oXvQwRBa5i9q3k0nuF5EOlYR9FWOdWukJYQp2+UB+l7pK14j+t7Nf8ToA/nv5TX1NWbEBB0P2i4/4i0fZ5xLou0ohWq2V8qSxSMFr4Qo7587cIu4LPv9wB3lLgY6r8E+hoXYBuCtAqFZgxvCfpuZzccZg8V4Acjr64iD4O2Gsyir+DgfzMKLk0d+teH0egf0H9JVw=="
OLD_BANS="eJyVUsFOhDAQvfMVI6eWrO4HEE0wi4kROWz03ODSRpJCm5Z1Nxr/3bYUKOxeHA6k815n3rwpAHyJpgYuqpp8VJ1GGH4icPH0XOSQMJn682NWkl32lkFiiIRXuneIR8cc3EP5XhTjJSZNgglJO0Cx4dz15z7eQKxiHN5uGKAbJvF4NqFof1RdSGJCAYLUfLPIha5Jqq/IqGDIVMUQ8m2lAxeaWihd5H3PX5+cMTtFxbk4kJa2SDfftvLYGIdV5O1DV7XU8HWvSH2UiClqzD0JVTstAXdAekGo4CsxVr6ztLGWDp7ihdYZhXBuyjW95JnFGF30vGYHW5vSbvptEhgPcPpsOAVkRzq0EtkJN5CXO/Ka7V/yPcYrg//n2YV19ncdH0awslmj/AtcxAStJvXhGyx3Mix84gavI9lGzpHoD1+FwmY="
BAN_GUARD='    if(!admin_bans_ready()){\n        send_to_char("Ban configuration is read-only until repaired and reloaded.\\n\\r",ch);\n        return;\n    }\n'
DISABLE_GUARD='      if(!admin_disabled_ready()){\n         send_to_char("Disabled-command configuration is read-only until repaired and reloaded.\\n\\r",ch);\n         return;\n      }\n'
BOOT_GUARD='        if(!admin_configuration_healthy()){\n            log_string("Invalid administrative configuration; repair disabled.txt/ban.txt before startup");\n            exit(1); /* Never start with an unvalidated command/ban policy. */\n        }\n'
ADD_DISABLED='         if(!admin_disabled_can_add(&cmd_table[i],ch->name,get_trust(ch))){\n            send_to_char("Disabled-command policy limit reached or invalid record; no change.\\n\\r",ch);\n            return;\n         }\n'
ADD_BAN='    if(!admin_ban_can_add(arg)){\n        send_to_char("Ban policy limit reached or invalid hostname; no change.\\n\\r",ch);\n        return;\n    }\n'
def once(text,new,old,count=1):
    assert text.count(new)==count,(new[:90],text.count(new),count)
    return text.replace(new,old)
def without_admin_config_stage(text):
    from werewolf_source import without_werewolf_stage
    text=without_werewolf_stage(text)
    from admin_saves_source import without_admin_saves_stage
    text=without_admin_saves_stage(text)
    if '#include "admin_config.h"\n' not in text:return text
    text=once(text,'#include "admin_config.h"\n',"")
    if "/* Administrative readers live in admin_config.c. */" in text:
        text=once(text,"/* Administrative readers live in admin_config.c. */\n\n",
                  zlib.decompress(base64.b64decode(OLD_DISABLED)).decode())
        text=once(text,"   /* Ban reader lives in admin_config.c. */\n\n",
                  zlib.decompress(base64.b64decode(OLD_BANS)).decode())
        text=once(text,DISABLE_GUARD,"")
        text=once(text,ADD_DISABLED,"")
        text=once(text,"      char buf[2*MAX_INPUT_LENGTH+128];","      char buf[100];")
        text=once(text,'snprintf (buf, sizeof buf, "%-12s %5li %-12s\\n\\r",',
                  'sprintf (buf, "%-12s %5li %-12s\\n\\r",')
        for which,file in (("disabled","disabled.txt"),("bans","ban.txt")):
            text=once(text,
                f'      if(!admin_{which}_ready()){{\n'
                f'         log_string("save_{which}: rejected configuration is read-only");\n'
                '         return;\n      }\n',"")
    elif "void do_ban(" in text:
        text=once(text,BAN_GUARD,"",count=2)
        text=once(text,ADD_BAN,"")
        text=once(text,"void do_ban( CHAR_DATA *ch, char *argument )\n{\n",
                  "void do_ban( CHAR_DATA *ch, char *argument )\n{\n    char buf[MAX_STRING_LENGTH];\n")
        text=once(text,
            'send_to_char( "Banned sites:\\n\\r", ch );\n'
            '\tfor ( pban = ban_list; pban != NULL; pban = pban->next )\n\t{\n'
            '    send_to_char( pban->name, ch );\n    send_to_char( "\\n\\r", ch );\n\t}\n',
            '\tstrcpy( buf, "Banned sites:\\n\\r" );\n'
            '\tfor ( pban = ban_list; pban != NULL; pban = pban->next )\n\t{\n'
            '\t    strcat( buf, pban->name );\n\t    strcat( buf, "\\n\\r" );\n\t}\n'
            '\tsend_to_char( buf, ch );\n')
    elif "void boot_db(" in text:
        text=once(text,BOOT_GUARD,"")
    else:raise AssertionError("Unexpected admin-config normalization target")
    return text
