/* Real interpreter and atomic-save coverage for secondary Highlander ranks. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/hybrid/access.h"
#include "classes/hybrid/player.h"
extern char *string_space, *top_string;
extern void init_mm(void);

static ROOM_INDEX_DATA room;
static AREA_DATA area;
static DESCRIPTOR_DATA shell;
static DESCRIPTOR_DATA target_shell;
static char output[32768];
static char target_output[8192];
static const char *const paths[] = {"weaponry", "attack", "defense", "powers"};
static const long parents[] = {
    CLASS_DRAGON, CLASS_DROW, CLASS_ELEM, CLASS_MAGE, CLASS_MONK,
    CLASS_NINJA, CLASS_SORCERER, CLASS_VAMPIRE, CLASS_WEREWOLF
};
static unsigned focus_count, paid_count;

static CHAR_DATA *fresh(long primary, long secondary)
{
    memset(&shell, 0, sizeof(shell));
    memset(&target_shell, 0, sizeof(target_shell));
    memset(output, 0, sizeof(output));
    memset(target_output, 0, sizeof(target_output));
    assert(!load_char_obj(&shell, "Absenthighfixture"));
    CHAR_DATA *ch = shell.character;
    free_string(ch->name);
    ch->name = str_dup("Highfixture");
    ch->desc = &shell;
    shell.connected = CON_PLAYING;
    shell.outbuf = output;
    shell.outsize = sizeof(output);
    ch->level = LEV_AWA;
    ch->position = POS_STANDING;
    ch->class = primary;
    ch->hit = ch->max_hit = 50000;
    ch->mana = ch->max_mana = 51000;
    ch->status = 100;
    ch->primal = 3000;
    ch->exp = 123456;
    ch->pcdata->quest = 50000;
    ch->pcdata->flag = FLAG_PK;
    for (int i = 0; i < 4; ++i)
        ch->pcdata->highland[i] =
            primary == CLASS_HIGHLANDER ? 0 : 6;
    /* Foreign primary fields must never become secondary training. */
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch,
        hybrid_by_parents(primary, secondary), &state));
    assert(hybrid_player_attach(ch, &state));
    ch->in_room = &room;
    ch->next = char_list;
    char_list = ch;
    ch->next_in_room = room.people;
    room.people = ch;
    return ch;
}

static void release(CHAR_DATA *ch)
{
    assert(char_list == ch && room.people == ch);
    char_list = ch->next;
    room.people = ch->next_in_room;
    ch->next = ch->next_in_room = NULL;
    ch->desc = NULL;
    ch->in_room = NULL;
    free_char(ch);
    shell.character = NULL;
    unlink("../player/Highfixture");
    unlink("../backup/Highfixture");
}

static void command(CHAR_DATA *ch, const char *argument)
{
    char line[256];
    assert(strlen(argument) < sizeof(line) - 16);
    snprintf(line, sizeof(line), "selfbrid %s", argument);
    shell.outtop = 0;
    output[0] = '\0';
    interpret(ch, line);
}

static void unchanged(CHAR_DATA *ch, const char *argument)
{
    PC_DATA pc = *ch->pcdata;
    long primal = ch->primal, exp = ch->exp, class = ch->class;
    command(ch, argument);
    assert(!memcmp(&pc, ch->pcdata, sizeof(pc)));
    assert(ch->primal == primal && ch->exp == exp && ch->class == class);
}

static void all_paths(void)
{
    for (size_t parent = 0; parent < sizeof(parents)/sizeof(parents[0]); ++parent) {
        for (int focus = 0; focus < 4; ++focus) {
            CHAR_DATA *ch = fresh(parents[parent], CLASS_HIGHLANDER);
            char arg[96];
            unchanged(ch, "highland");
            assert(strstr(output, "not selected"));
            assert(strstr(output, "attack: 0/3 (stored 0)"));
            unchanged(ch, "highland attack");
            assert(strstr(output, "next rank costs 15"));
            snprintf(arg, sizeof(arg), "highland focus %s", paths[focus]);
            unchanged(ch, arg);
            assert(strstr(output, "Repeat with confirm"));
            strcat(arg, " confirm");
            command(ch, arg);
            assert(strstr(output, "completed and saved"));
            assert(ch->pcdata->hybrid.secondary_progression.full_branch == focus);
            focus_count++;

            for (int path = 0; path < 4; ++path) {
                int cap = path == focus ? 6 : 3;
                snprintf(arg, sizeof(arg), "highland %s gain", paths[path]);
                for (int rank = 1; rank <= cap; ++rank) {
                    long before = ch->primal;
                    command(ch, arg);
                    assert(strstr(output, "completed and saved"));
                    assert(hybrid_power_rank(ch, CLASS_HIGHLANDER,
                        HYBRID_TRAIN_HIGHLAND, (size_t)path) == rank);
                    assert(ch->primal == before - 15 * rank);
                    for (int i = 0; i < 4; ++i)
                        assert(ch->pcdata->highland[i] == 6);
                    paid_count++;
                }
                PC_DATA before = *ch->pcdata;
                long primal = ch->primal;
                command(ch, arg);
                assert(strstr(output, "already at its secondary cap"));
                assert(!memcmp(&before, ch->pcdata, sizeof(before)));
                assert(ch->primal == primal);
            }
            assert(ch->pcdata->quest == 50000 && ch->exp == 123456);
            assert(ch->class == parents[parent] && ch->primal == 2415);
            release(ch);
        }
    }
    assert(focus_count == 36 && paid_count == 540);
}

static void guards_and_native_compatibility(void)
{
    CHAR_DATA *ch = fresh(CLASS_DROW, CLASS_HIGHLANDER);
    const char *bad[] = {
        "highland focus", "highland focus nonsense confirm",
        "highland focus attack maybe", "highland focus attack confirm trailing",
        "highland attack gain extra", "highland bladeflash", "highland attack gain"
    };
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i)
        unchanged(ch, bad[i]);
    assert(access("../player/Highfixture", F_OK) != 0);
    release(ch);

    ch = fresh(CLASS_HIGHLANDER, CLASS_DROW);
    unchanged(ch, "highland");
    assert(strstr(output, "unchanged increase command"));
    HYBRID_STATE before = ch->pcdata->hybrid;
    shell.outtop = 0;
    output[0] = '\0';
    interpret(ch, "increase attack");
    assert(ch->pcdata->highland[HIGH_ATTACK] == 1);
    assert(ch->primal == 2985);
    assert(!memcmp(&before, &ch->pcdata->hybrid, sizeof(before)));
    release(ch);

    ch = fresh(CLASS_DROW, CLASS_HIGHLANDER);
    unchanged(ch, "ninja");
    shell.outtop = 0;
    output[0] = '\0';
    interpret(ch, "increase attack");
    assert(strstr(output, "Use selfbrid highland"));
    assert(ch->pcdata->highland[HIGH_ATTACK] == 6 && ch->primal == 3000);
    release(ch);
}

static void secondary_lunge_uses_parent_rank(void)
{
    CHAR_DATA *ch = fresh(CLASS_DROW, CLASS_HIGHLANDER);
    command(ch, "highland focus powers");
    command(ch, "highland focus powers confirm");
    for (int i = 1; i <= 5; ++i) {
        command(ch, "highland powers gain");
        assert(hybrid_power_rank(ch, CLASS_HIGHLANDER, HYBRID_TRAIN_HIGHLAND,
                                 HIGH_ABILITY) == i);
    }

    assert(!load_char_obj(&target_shell, "Absentlungevictim"));
    CHAR_DATA *victim = target_shell.character;
    free_string(victim->name);
    victim->name = str_dup("PracticeTarget");
    victim->short_descr = victim->name;
    victim->desc = &target_shell;
    target_shell.connected = CON_PLAYING;
    target_shell.outbuf = target_output;
    target_shell.outsize = sizeof(target_output);
    victim->class = CLASS_MAGE;
    victim->level = LEV_AWA;
    victim->status = 100;
    victim->position = POS_STANDING;
    victim->hit = victim->max_hit = 50000;
    victim->pcdata->flag = FLAG_PK;
    victim->in_room = &room;
    victim->next = ch->next;
    ch->next = victim;
    victim->next_in_room = ch->next_in_room;
    ch->next_in_room = victim;

    interpret(ch, "lunge PracticeTarget");
    assert(!ch->fighting && !victim->fighting && ch->wait == 0);
    assert(strstr(output, "You have not learned this attack"));
    command(ch, "highland powers gain");
    assert(hybrid_power_rank(ch, CLASS_HIGHLANDER, HYBRID_TRAIN_HIGHLAND,
                             HIGH_ABILITY) == 6);
    shell.outtop = 0;
    output[0] = '\0';
    interpret(ch, "lunge PracticeTarget");
    assert(ch->fighting == victim && victim->fighting == ch && ch->wait == 8);

    stop_fighting(ch, TRUE);
    stop_fighting(victim, TRUE);
    ch->next = victim->next;
    ch->next_in_room = victim->next_in_room;
    victim->next = victim->next_in_room = NULL;
    victim->desc = NULL;
    victim->in_room = NULL;
    free_char(victim);
    target_shell.character = NULL;
    unlink("../player/PracticeTarget");
    unlink("../backup/PracticeTarget");
    release(ch);
}

int main(void)
{
    char cwd[4096], root[] = "/tmp/anathema-highlander-training-XXXXXX";
    assert(getcwd(cwd, sizeof(cwd)) && mkdtemp(root) && !chdir(root));
    assert(!mkdir("player", 0700) && !mkdir("backup", 0700) &&
           !mkdir("run", 0700) && !chdir("run"));
    string_space = malloc(4 * 1024 * 1024);
    assert(string_space);
    top_string = string_space;
    current_time = 123456;
    init_mm();
    fpReserve = fopen(NULL_FILE, "r");
    assert(fpReserve);
    room.vnum = 900;
    room.area = &area;
    room.name = "Private fixture";
    room.description = "";
    room_index_hash[900 % MAX_KEY_HASH] = &room;
    all_paths();
    guards_and_native_compatibility();
    secondary_lunge_uses_parent_rank();
    assert(!char_list && !room.people && !fclose(fpReserve));
    assert(!chdir("..") && !rmdir("player") && !rmdir("backup") &&
           !rmdir("run") && !chdir(cwd) && !rmdir(root));
    printf("Secondary Highlander UI: %u focus choices, %u paid ranks, "
           "caps, original-bank isolation, parser guards, native trainer "
           "compatibility, and rank-gated secondary Lunge passed.\n",
           focus_count, paid_count);
    return 0;
}
