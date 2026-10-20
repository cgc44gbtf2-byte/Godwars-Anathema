/* Real player loaders, atomic saves and recycled PC_DATA in a private temp world. */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "classes/hybrid/player.h"
#include "classes/hybrid/specialization.h"
#include "classes/monk/favour.h"

extern char *string_space, *top_string;
extern void init_mm(void);
extern bool load_char_short(DESCRIPTOR_DATA *, char *);
extern void fread_char(CHAR_DATA *, FILE *);

static DESCRIPTOR_DATA shell;
static ROOM_INDEX_DATA room;
static AREA_DATA area;

static CHAR_DATA *fresh(void)
{
    memset(&shell, 0, sizeof(shell));
    assert(!load_char_obj(&shell, "Newfixture"));
    CHAR_DATA *ch = shell.character;
    ch->desc = NULL;
    ch->level = LEV_AWA;
    ch->in_room = &room;
    ch->class = CLASS_NONE;
    ch->hit = ch->max_hit = 50000;
    ch->mana = ch->max_mana = 51000;
    ch->gold = 123456;
    ch->pcdata->quest = 50000;
    assert(!ch->pcdata->hybrid_present && !ch->pcdata->player_load_failed);
    return ch;
}

static void release(CHAR_DATA *ch)
{
    ch->desc = NULL;
    ch->in_room = NULL;
    free_char(ch);
    shell.character = NULL;
}

static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    assert(fp && fseek(fp, 0, SEEK_END) == 0);
    long n = ftell(fp);
    assert(n >= 0 && fseek(fp, 0, SEEK_SET) == 0);
    char *data = malloc((size_t)n + 1);
    assert(data && fread(data, 1, (size_t)n, fp) == (size_t)n);
    data[n] = '\0';
    assert(fclose(fp) == 0);
    return data;
}

static void equal_state(const HYBRID_STATE *a, const HYBRID_STATE *b)
{
    FILE *left = tmpfile(), *right = tmpfile();
    assert(left && right && hybrid_state_write(left, a) &&
           hybrid_state_write(right, b));
    rewind(left); rewind(right);
    int x, y;
    do { x = fgetc(left); y = fgetc(right); assert(x == y); } while (x != EOF);
    assert(fclose(left) == 0 && fclose(right) == 0);
}

static void all_pairs(void)
{
    size_t count = 0;
    for (size_t i = 0; i < hybrid_count(); ++i) {
        const HYBRID_DEFINITION *def = hybrid_at(i);
        for (int order = 0; order < 2; ++order) {
            CHAR_DATA *ch = fresh();
            ch->class = def->parents[order];
            ch->pcdata->powers[2] = 7;
            ch->pcdata->ninja[0] = 10;
            ch->vampgen = 4;
            ch->pcdata->classpoints[0] = 123;
            CHAR_DATA before = *ch;
            PC_DATA pc_before = *ch->pcdata;
            HYBRID_STATE state;
            assert(hybrid_secondary_initialize(ch, def, &state));
            assert(!memcmp(&before, ch, sizeof(before)) &&
                   !memcmp(&pc_before, ch->pcdata, sizeof(pc_before)));
            assert(state.secondary_progression.full_branch ==
                   (hybrid_specialization_count(state.secondary_class) ? -1 : 0));
            if (state.secondary_class == CLASS_VAMPIRE) {
                assert(state.secondary_generation == 9 &&
                       state.secondary_blood[BLOOD_POOL] == 300 &&
                       state.secondary_blood[BLOOD_CURRENT] == 0);
                for (int p = 0; p < 20; ++p) assert(state.secondary_powers[p] == -1);
                HYBRID_STATE probe = state;
                assert(hybrid_secondary_vampire_profile(&probe, CLASS_VAMPIRE,
                                                        HYBRID_VAMPIRE_STANDARD));
                long rank = 99;
                assert(hybrid_parent_rank_read(ch, &probe, CLASS_VAMPIRE,
                        HYBRID_TRAIN_DISCIPLINE, DISC_ANIMALISM, &rank) && rank == 0);
                probe.secondary_powers[0] = -2;
                assert(!hybrid_state_valid(&probe));
            }
            if (state.secondary_class == CLASS_DRAGON)
                assert(state.secondary_generation == AGE_HATCHLING &&
                       state.secondary_progression.dragon_color == -1);
            if (state.secondary_class == CLASS_MAGE)
                assert(state.secondary_progression.arete == 1 &&
                       state.secondary_branches[0] == -1);
            if (state.secondary_class == CLASS_WEREWOLF)
                assert(state.secondary_progression.wolf_rank == 1 &&
                       state.secondary_progression.selected_gifts[0] == -1);
            assert(hybrid_player_attach(ch, &state));
            assert(!hybrid_player_attach(ch, &state));
            assert(ch->class == before.class && ch->vampgen == 4 &&
                   ch->pcdata->powers[2] == 7 && ch->pcdata->quest == 50000);
            /* Existing More serialization, not a new hybrid save field. */
            ch->more=MORE_FAVOUR|MORE_CLOAK|MORE_GUARDIAN;
            assert(monk_favour_grant(ch, &time_info));
            assert(save_char_obj_checked(ch));
            assert(save_char_obj_backup_checked(ch));
            char *disk = read_file("../player/Newfixture");
            assert(strstr(disk, "AnathemaHybrid 4 ") &&
                   !strstr(strstr(disk, "AnathemaHybrid") + 14, "AnathemaHybrid"));
            char *backup = read_file("../backup/Newfixture");
            assert(!strcmp(disk, backup));
            free(backup);
            release(ch);
            for (int short_load = 0; short_load < 2; ++short_load) {
                memset(&shell, 0, sizeof(shell));
                assert(short_load ? load_char_short(&shell, "Newfixture") :
                                    load_char_obj(&shell, "Newfixture"));
                ch = shell.character;
                assert(!ch->pcdata->player_load_failed && ch->pcdata->hybrid_present);
                assert(ch->more==(MORE_FAVOUR|MORE_CLOAK|MORE_GUARDIAN));
                equal_state(&state, &ch->pcdata->hybrid);
                assert(ch->class == before.class && ch->gold == 123456 &&
                       ch->max_hit == 50000 && ch->max_mana == 51000 &&
                       ch->pcdata->powers[2] == 7 && ch->vampgen == 4);
                release(ch);
            }
            free(disk);
            assert(unlink("../player/Newfixture") == 0 &&
                   unlink("../backup/Newfixture") == 0);
            ++count;
        }
    }
    assert(count == 90);
}

static void legacy_and_atomic(void)
{
    CHAR_DATA *ch = fresh();
    ch->class = CLASS_NINJA;
    assert(save_char_obj_checked(ch));
    char *plain = read_file("../player/Newfixture");
    assert(!strstr(plain, "AnathemaHybrid"));
    release(ch);
    assert(load_char_obj(&shell, "Newfixture"));
    ch = shell.character; ch->desc = NULL;
    assert(!ch->pcdata->hybrid_present && !ch->pcdata->player_load_failed);
    HYBRID_STATE state;
    const HYBRID_DEFINITION *def = NULL;
    for (size_t i = 0; i < hybrid_count(); ++i)
        if (hybrid_has_parent(hybrid_at(i), CLASS_NINJA)) { def = hybrid_at(i); break; }
    assert(def && hybrid_secondary_initialize(ch, def, &state));
    assert(hybrid_player_attach(ch, &state));
    const enum atomic_save_fault faults[] = {
        SAVE_FAULT_CREATE, SAVE_FAULT_FLUSH, SAVE_FAULT_SYNC,
        SAVE_FAULT_CLOSE, SAVE_FAULT_RENAME
    };
    for (size_t i = 0; i < sizeof(faults)/sizeof(faults[0]); ++i) {
        atomic_save_test_fault = faults[i];
        assert(!save_char_obj_checked(ch) && ch->pcdata->hybrid_present);
        char *disk = read_file("../player/Newfixture");
        assert(!strcmp(plain, disk)); free(disk);
    }
    atomic_save_test_fault = SAVE_FAULT_NONE;
    assert(save_char_obj_checked(ch));
    char *good = read_file("../player/Newfixture");
    ch->pcdata->hybrid.hybrid_id = 99999;
    assert(!save_char_obj_checked(ch));
    char *unchanged = read_file("../player/Newfixture");
    assert(!strcmp(good, unchanged)); free(unchanged);
    ch->pcdata->hybrid = state;
    ch->pcdata->player_load_failed = 1;
    assert(!save_char_obj_checked(ch));
    unchanged = read_file("../player/Newfixture");
    assert(!strcmp(good, unchanged)); free(unchanged);
    free(good); free(plain); release(ch);
    assert(unlink("../player/Newfixture") == 0);
}

static void corruptions(void)
{
    CHAR_DATA *ch = fresh();
    const HYBRID_DEFINITION *def = hybrid_at(0);
    ch->class = def->parents[0];
    HYBRID_STATE state;
    assert(hybrid_secondary_initialize(ch, def, &state));
    FILE *tmp = tmpfile(); assert(tmp && hybrid_state_write(tmp, &state));
    rewind(tmp);
    char record[4096]; assert(fgets(record, sizeof(record), tmp));
    assert(fclose(tmp) == 0);
    release(ch);
    /* Bad version, missing data, next-line spill, primary mismatch, duplicate,
     * absent explicit End, extra token, overlong record. No scanner continuation. */
    for (int scenario = 0; scenario < 10; ++scenario) {
        FILE *fp = fopen("../player/Newfixture", "w"); assert(fp);
        fprintf(fp, "#PLAYER\nName Newfixture~\nClass %ld\n",
                scenario == 3 ? state.secondary_class : state.primary_class);
        if (scenario == 0) fputs("AnathemaHybrid 99 1\n", fp);
        else if (scenario == 1) fputs("AnathemaHybrid 3 1\n", fp);
        else if (scenario == 2) {
            fputs("AnathemaHybrid\n", fp); fputs(record + 14, fp);
        } else if (scenario == 6) {
            fprintf(fp, "%.*s junk\n", (int)strlen(record) - 1, record);
        } else if (scenario == 7) {
            fputs("AnathemaHybrid ", fp);
            for (int j = 0; j < 5000; ++j) fputc('1', fp);
            fputc('\n', fp);
        } else {
            fputs(record, fp);
            if (scenario == 4) fputs(record, fp);
        }
        if (scenario == 8) fputs("End\n\n", fp);
        else if (scenario == 9) fputs("End\n\n#PLAYER\nEnd\n#END\n", fp);
        else if (scenario != 5) fputs("End\n\n#END\n", fp);
        assert(fclose(fp) == 0);
        char *original = read_file("../player/Newfixture");
        for (int short_load = 0; short_load < 2; ++short_load) {
            memset(&shell, 0, sizeof(shell));
            assert(short_load ? load_char_short(&shell, "Newfixture") :
                                load_char_obj(&shell, "Newfixture"));
            ch = shell.character;
            assert(ch->pcdata->player_load_failed);
            ch->level = LEV_AWA; ch->desc = NULL;
            assert(!save_char_obj_checked(ch));
            char *after = read_file("../player/Newfixture");
            assert(!strcmp(original, after)); free(after);
            release(ch);
        }
        free(original);
        assert(unlink("../player/Newfixture") == 0);
    }
}

static void favour_expiry(void)
{
    const long keep=MORE_CLOAK|MORE_GUARDIAN;
    for(int short_load=0;short_load<2;short_load++)
    for(int expired=0;expired<2;expired++){
        time_info.hour=5;time_info.day=34;time_info.month=16;time_info.year=1300;
        CHAR_DATA *ch=fresh();ch->class=CLASS_MONK;
        ch->pcdata->monk[MONK_MANTRA]=5;ch->more=keep;
        assert(monk_favour_grant(ch,&time_info)&&save_char_obj_checked(ch));
        release(ch);
        if(expired)time_info.hour=6;
        memset(&shell,0,sizeof(shell));
        assert(short_load?load_char_short(&shell,"Newfixture"):load_char_obj(&shell,"Newfixture"));
        ch=shell.character;assert(!ch->pcdata->player_load_failed);
        assert((ch->more&keep)==keep&&!!IS_MORE(ch,MORE_FAVOUR)==!expired);
        if(!expired)assert(ch->favour_until>ch->favour_since);
        else assert(!ch->favour_since&&!ch->favour_until);
        release(ch);assert(unlink("../player/Newfixture")==0);
    }
    /* Legacy More flags have no evidence that the original dawn is pending. */
    for(int short_load=0;short_load<2;short_load++){
        FILE *fp=fopen("../player/Newfixture","w");assert(fp);
        fprintf(fp,"#PLAYER\nName Newfixture~\nLevel 2\nMore %ld\nEnd\n#END\n",keep|MORE_FAVOUR);
        assert(fclose(fp)==0);memset(&shell,0,sizeof(shell));
        assert(short_load?load_char_short(&shell,"Newfixture"):load_char_obj(&shell,"Newfixture"));
        assert(shell.character->more==keep&&!shell.character->pcdata->player_load_failed);
        release(shell.character);assert(unlink("../player/Newfixture")==0);
    }
    memset(&time_info,0,sizeof(time_info));
}

int main(void)
{
    char root[] = "/tmp/anathema-hybrid-player-XXXXXX", cwd[4096];
    assert(getcwd(cwd, sizeof(cwd)) && mkdtemp(root) && chdir(root) == 0);
    assert(mkdir("player", 0700) == 0 && mkdir("backup", 0700) == 0 &&
           mkdir("run", 0700) == 0 && chdir("run") == 0);
    string_space = malloc(4*1024*1024); assert(string_space);
    top_string = string_space; current_time = 123456; init_mm();
    fpReserve = fopen(NULL_FILE, "r"); assert(fpReserve);
    room.vnum = 900; room.area = &area;
    room.name = "Private fixture room"; room.description = "";
    room_index_hash[900 % MAX_KEY_HASH] = &room;
    favour_expiry();all_pairs(); legacy_and_atomic(); corruptions();
    assert(fclose(fpReserve) == 0 && chdir("..") == 0);
    assert(rmdir("player") == 0 && rmdir("backup") == 0 &&
           rmdir("run") == 0 && chdir(cwd) == 0 && rmdir(root) == 0);
    puts("hybrid player: 90 initialized pairs, both real loaders, native preservation, "
         "legacy saves, atomic failures and fail-closed corruption tests passed");
    return 0;
}
