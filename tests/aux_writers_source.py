"""Reverse only this stage's exact leaderboard writer/flags for prior pins."""
import base64
import zlib

OLD_SAVE = (
    "eJydk01LxDAQhs/bXzEEhEZq9+7S4wpC8eZNCGkzWaJpU5PsVln0t5t+sLSyHmIOgQnzzDPvISejBDh+QqaRC7SV4VakNDknEM7DY7mHW9ntxkrJdNHEtOGCSa40Cjq1D0ebA3PeqvaQkt9z78HiK9YeBciAgXLhgYs70+pPQneXGRb90bZT/ZXMbkhT2UEB0nTYpiTPtzzA22l87j88yUhPKIWigKfnsqQjd042y4321hoLvVU+lOANLPCwwGZtHm7ZBdLL4M6A3Ljvl5ZkM8XGUHmFzndvrOUNzhnWjFZ/M8emQnuVumbqjY1XXaA4V6zmHwYRaRCxhiYyQxOdoYnM0ERn8D7OMPTHGd7rOMPQvzLU2jiEQISH8F1/AA61YfY="
)
def replace_once(text,new,old):
    assert text.count(new)==1,(new[:90],text.count(new))
    return text.replace(new,old,1)

def without_aux_writers_update(text):
    from drow_globe_source import without_drow_globe_stage
    text=without_drow_globe_stage(text)
    hook="    save_notes();\n    retry_leaderboard_save();\n"
    if hook not in text:return text
    text=replace_once(text,'#include "auxiliary_save.h"\n',"")
    return replace_once(text,hook,"")

def without_aux_writers_stage(text):
    from aux_shutdown_source import without_aux_shutdown_stage
    text=without_aux_shutdown_stage(text)
    if "/* Atomic leaderboard writer scope. */" not in text:
        return text
    start=text.index("/* Atomic leaderboard writer scope. */")
    end=text.index("void do_leader(",start)
    text=text[:start]+zlib.decompress(base64.b64decode(OLD_SAVE)).decode()+text[end:]
    for line in ('#include "atomic_save.h"\n','#include <limits.h>\n',
                 "static bool leaderboard_save_pending;\n",
                 "static time_t leaderboard_retry_after;\n",
                 "    leaderboard_save_pending=FALSE;\n",
                 "    leaderboard_retry_after=0;\n"):
        text=replace_once(text,line,"")
    text=replace_once(text,
        "    if (changed && (!leaderboard_save_pending || current_time>=leaderboard_retry_after))\n"
        "        save_leaderboard();",
        "    if (changed) save_leaderboard();")
    return replace_once(text,
        '    if (ch != NULL) stc(leaderboard_save_pending ?\n'
        '        "Leader board reset in memory; disk save is pending.\\n\\r" :\n'
        '        "Leader board cleared.\\n\\r", ch);',
        '    if (ch != NULL) stc("Leader board cleared.\\n\\r", ch);')
