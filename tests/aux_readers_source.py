"""Reverse only the owned leaderboard reader/guards for older whole-file pins.

The compressed literal is the exact previous load_leaderboard implementation,
not an entire module or a permissive normalization of unrelated functions.
"""
import base64
import zlib

OLD_LEADER = (
    "eJyVjj1rwzAQhufoV1w1SSU4e43HFAomW2cjW1IwtT56lttCyH+vLLfplMJpkDh07/M+H2HUMAWlu8kobbAPCrWQ7MIgn+eX9giPNtasjKMFIWyEBmyIxgteVQeFRh22bJW+Et8DRy4lNA2cXttWluCF7aZw7uaEoz8LfkQM+AR/KfAhZebi9QOXNdvp8KMz5AfFCsrc8oUmLejrgr2We9vsinnVmznFt84rZ1bNLKd/a22U9f3A4nqDt8g23ol8BiSW3BKEFlIBla0pbE1iO4q3o3k7irejeadEYK/LBPb7QGCvy/+z7TCF2WzzlbFv+nk/Xg=="
)

def without_aux_readers_stage(text):
    if "static bool leaderboard_load_failed;" not in text:
        return text
    start=text.index("void load_leaderboard()")
    end=text.index("void save_leaderboard()",start)
    text=text[:start]+zlib.decompress(base64.b64decode(OLD_LEADER)).decode()+text[end:]
    for include in ("#include <errno.h>\n",'#include "file_number.h"\n',
                    '#include "file_string.h"\n','#include "file_word.h"\n'):
        text=text.replace(include,"",1)
    text=text.replace("static bool leaderboard_load_failed;\n","",1)
    text=text.replace("""    if(leaderboard_load_failed){
        log_string("save_leaderboard: rejected file is read-only");
        return;
    }
""","",1)
    text=text.replace("""    /* This existing explicit reset intentionally replaces the stored scores. */
    leaderboard_load_failed=FALSE;
""","",1)
    return text
