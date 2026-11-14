/* Parent-owned Highlander focus and paid ranks; never writes native fields. */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../../merc.h"
#include "access.h"
#include "highlander_training.h"
#include "training.h"

static const char *const names[] = {"weaponry", "attack", "defense", "powers"};
_Static_assert(HIGH_WEAPON == 0 && HIGH_ATTACK == 1 && HIGH_DEFENSE == 2 &&
               HIGH_ABILITY == 3, "Native Highlander rank mapping changed");

static int word(const char **input, char out[16])
{
    const unsigned char *p = (const unsigned char *)*input;
    size_t n = 0;
    while (isspace(*p)) ++p;
    while (*p && !isspace(*p)) {
        if (n == 15) return 0;
        out[n++] = (char)tolower(*p++);
    }
    out[n] = '\0';
    *input = (const char *)p;
    return 1;
}

static int path(const char *name)
{
    for (int i = 0; i < 4; ++i)
        if (!strcmp(name, names[i])) return i;
    return -1;
}

static void usage(CHAR_DATA *ch)
{
    send_to_char("Use: selfbrid highland | selfbrid highland "
        "<attack/defense/weaponry/powers>\n\r"
        "Choose one permanent full path: selfbrid highland focus <path>\n\r"
        "Confirm the free choice: selfbrid highland focus <path> confirm\n\r"
        "Buy one rank: selfbrid highland <path> gain\n\r"
        "Chosen path cap6, other paths cap3; each rank costs "
        "15 x next rank primal. Focus grants no ranks. Native increase "
        "remains unchanged; conversion is still disabled.\n\r", ch);
}

static void result(CHAR_DATA *ch, enum hybrid_lesson_result r)
{
    switch (r) {
    case HYBRID_LESSON_OK:
        send_to_char("Secondary Highlander lesson completed and saved.\n\r", ch);
        break;
    case HYBRID_LESSON_NO_FUNDS:
        send_to_char("Not enough primal for the next Highlander rank. "
                     "Nothing changed.\n\r", ch);
        break;
    case HYBRID_LESSON_AT_CAP:
        send_to_char("That path is already at its secondary cap. Nothing changed.\n\r", ch);
        break;
    case HYBRID_LESSON_SAVE_PENDING:
        send_to_char("Primary save recovery is pending. Wait before another lesson.\n\r", ch);
        break;
    case HYBRID_LESSON_SAVE_FAILED:
        send_to_char("The save failed. No lesson or payment was applied. "
                     "Wait for primary save recovery before retrying.\n\r", ch);
        break;
    case HYBRID_LESSON_SAVE_UNCERTAIN:
        send_to_char("Lesson/payment applied; save durability is pending. "
                     "Do not repeat; wait for primary save recovery.\n\r", ch);
        break;
    case HYBRID_LESSON_UNCHANGED:
        send_to_char("That path is already selected. Nothing changed.\n\r", ch);
        break;
    default:
        send_to_char("Lesson refused. Be connected in your own body, standing, "
                     "out of combat, with valid training data. Nothing changed.\n\r", ch);
        break;
    }
}

static void display(CHAR_DATA *ch, const HYBRID_STATE *s, int selected)
{
    long cap, rank, stored;
    if (!hybrid_parent_training_cap(ch, s, CLASS_HIGHLANDER,
            HYBRID_TRAIN_HIGHLAND, (size_t)selected, &cap))
        return;
    rank = hybrid_power_rank(ch, CLASS_HIGHLANDER, HYBRID_TRAIN_HIGHLAND,
                             (size_t)selected);
    if (!hybrid_parent_progression_read(ch, s, CLASS_HIGHLANDER,
            HYBRID_PROGRESS_HIGHLAND, (size_t)selected, &stored))
        return;
    char line[192];
    if (rank < cap)
        snprintf(line, sizeof(line),
            "%s: %ld/%ld (stored %ld); next rank costs %ld primal.\n\r",
            names[selected], rank, cap, stored, 15 * (rank + 1));
    else
        snprintf(line, sizeof(line), "%s: %ld/%ld (stored %ld); at cap.\n\r",
            names[selected], rank, cap, stored);
    send_to_char(line, ch);
}

int hybrid_highlander_training_command(CHAR_DATA *ch, const char *argument)
{
    char args[4][16], tail[16], line[160];
    const char *p = argument ? argument : "";
    if (!word(&p, args[0]) || strcmp(args[0], "highland")) return 0;
    if (!ch || IS_NPC(ch) || !ch->pcdata) return 1;
    for (int i = 1; i < 4; ++i)
        if (!word(&p, args[i])) {
            usage(ch);
            return 1;
        }
    if (!word(&p, tail) || *tail) {
        usage(ch);
        return 1;
    }
    if (ch->class == CLASS_HIGHLANDER) {
        send_to_char("Highlander is your original parent. Use the unchanged "
                     "increase command.\n\r", ch);
        return 1;
    }
    const HYBRID_STATE *s = hybrid_live_secondary(ch);
    if (!s || s->secondary_class != CLASS_HIGHLANDER) {
        send_to_char("You do not have a valid secondary Highlander record. "
                     "No conversion or training was performed.\n\r", ch);
        return 1;
    }
    long focus = s->secondary_progression.full_branch;
    if (!*args[1]) {
        snprintf(line, sizeof(line), "Secondary Highlander paths; full path: %s.\n\r",
            focus >= 0 && focus < 4 ? names[focus] : "not selected");
        send_to_char(line, ch);
        for (int i = 0; i < 4; ++i) display(ch, s, i);
        if (!hybrid_secondary_ranks_valid(s))
            send_to_char("Stored training exceeds policy caps; staff review is "
                         "required before lessons.\n\r", ch);
        usage(ch);
        return 1;
    }

    int selected = path(args[1]);
    if (selected >= 0 && !*args[2] && !*args[3]) {
        display(ch, s, selected);
        if (!hybrid_secondary_ranks_valid(s))
            send_to_char("Stored training needs staff review; nothing was "
                         "normalized or charged.\n\r", ch);
        usage(ch);
        return 1;
    }
    if (!hybrid_secondary_ranks_valid(s)) {
        send_to_char("Stored training needs staff review; nothing was "
                     "normalized or charged.\n\r", ch);
        return 1;
    }

    HYBRID_LESSON_REQUEST request = {0};
    request.parent = CLASS_HIGHLANDER;
    request.tree = HYBRID_TRAIN_HIGHLAND;
    if (!strcmp(args[1], "focus")) {
        selected = path(args[2]);
        if (selected < 0 || (*args[3] && strcmp(args[3], "confirm"))) {
            usage(ch);
            return 1;
        }
        if (focus != -1) {
            send_to_char("Your full path is already chosen and cannot be "
                         "changed here.\n\r", ch);
            return 1;
        }
        if (!*args[3]) {
            snprintf(line, sizeof(line),
                "Permanent choice: %s cap6, others3. Free; no ranks granted. "
                "Repeat with confirm to save.\n\r", names[selected]);
            send_to_char(line, ch);
            return 1;
        }
        request.action = HYBRID_LESSON_SPECIALIZE;
        request.value = selected;
    } else {
        if (selected < 0 || strcmp(args[2], "gain") || *args[3]) {
            usage(ch);
            return 1;
        }
        if (focus == -1) {
            send_to_char("Choose and confirm a full path before training.\n\r", ch);
            return 1;
        }
        request.action = HYBRID_LESSON_RANK;
        request.index = (size_t)selected;
    }
    result(ch, hybrid_training_commit(ch, &request));
    return 1;
}
