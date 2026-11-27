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
           hybrid_state_forms_write(left, a) &&
           hybrid_state_write(right, b) &&
           hybrid_state_forms_write(right, b));
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
            if (state.primary_class == CLASS_VAMPIRE &&
                state.secondary_class == CLASS_WEREWOLF) {
                unsigned long serial = hybrid_state_form_begin(
                    &state, CLASS_VAMPIRE, 11, HYBRID_FEATURE_CLAWS,
                    HYBRID_FEATURE_ALL);
                HYBRID_FUSION_PROFILE profile = {
                    state.hybrid_id, HYBRID_FEATURE_FANGS,
                    HYBRID_FEATURE_CLAWS | HYBRID_FEATURE_FANGS
                };
                assert(serial && hybrid_state_form_fuse(
                    &state, CLASS_WEREWOLF, 22, HYBRID_FEATURE_FANGS,
                    HYBRID_FEATURE_ALL, &profile));
            }
            assert(hybrid_player_attach(ch, &state));
            assert(!hybrid_player_attach(ch, &state));
            assert(ch->class == before.class && ch->vampgen == 4 &&
                   ch->pcdata->powers[2] == 7 && ch->pcdata->quest == 50000);
            /* Unrelated More serialization, not a guardian eligibility test. */
            ch->more=MORE_FAVOUR|MORE_CLOAK|MORE_BRISTLES;
            assert(monk_favour_grant(ch, &time_info));
            assert(save_char_obj_checked(ch));
            assert(save_char_obj_backup_checked(ch));
            char *disk = read_file("../player/Newfixture");
            char *core_line = strstr(disk, "AnathemaHybrid 4 ");
            assert(core_line && strstr(disk, "AnathemaHybridForms 1 ") &&
                   !strstr(core_line + strlen("AnathemaHybrid 4 "),
                           "AnathemaHybrid 4 "));
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
                assert(ch->pcdata->hybrid_forms_present);
                assert(ch->more==(MORE_FAVOUR|MORE_CLOAK|MORE_BRISTLES));
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

static void core_only_compatibility(void)
{
    CHAR_DATA *ch = fresh();
    const HYBRID_DEFINITION *definition =
        hybrid_by_parents(CLASS_NINJA, CLASS_WEREWOLF);
    HYBRID_STATE state;
    assert(definition);
    ch->class = CLASS_NINJA;
    assert(hybrid_secondary_initialize(ch, definition, &state));
    FILE *fp = fopen("../player/Newfixture", "w");
    assert(fp && fprintf(fp, "#PLAYER\nName Newfixture~\nClass %ld\n",
                         CLASS_NINJA) > 0);
    assert(hybrid_state_write(fp, &state));
    assert(fputs("End\n#END\n", fp) >= 0 && fclose(fp) == 0);
    release(ch);
    memset(&shell, 0, sizeof(shell));
    assert(load_char_obj(&shell, "Newfixture"));
    ch = shell.character;
    assert(ch->pcdata->hybrid_present && !ch->pcdata->hybrid_forms_present &&
           !ch->pcdata->player_load_failed &&
           hybrid_forms_mode(&ch->pcdata->hybrid.forms) == HYBRID_BODY_NORMAL);
    release(ch);
    assert(unlink("../player/Newfixture") == 0);

    /* The optional extension may precede Class in a legacy player record.
     * Its parent ownership must be checked at End, not at the extension line. */
    assert(hybrid_state_form_begin(&state, CLASS_WEREWOLF, 11,
                                   HYBRID_FEATURE_CLAWS,
                                   HYBRID_FEATURE_ALL));
    for (int short_load = 0; short_load < 2; ++short_load) {
        fp = fopen("../player/Newfixture", "w"); assert(fp);
        assert(fputs("#PLAYER\nName Newfixture~\n", fp) >= 0);
        assert(hybrid_state_write(fp, &state) &&
               hybrid_state_forms_write(fp, &state));
        assert(fprintf(fp, "Class %ld\nEnd\n#END\n", CLASS_NINJA) > 0 &&
               fclose(fp) == 0);
        memset(&shell, 0, sizeof(shell));
        assert(short_load ? load_char_short(&shell, "Newfixture") :
                            load_char_obj(&shell, "Newfixture"));
        ch = shell.character;
        assert(ch->pcdata->hybrid_present && ch->pcdata->hybrid_forms_present &&
               !ch->pcdata->player_load_failed &&
               hybrid_forms_mode(&ch->pcdata->hybrid.forms) == HYBRID_BODY_SINGLE);
        release(ch);
    }
    assert(unlink("../player/Newfixture") == 0);
}

static void favour_expiry(void)
{
    const long keep=MORE_CLOAK|MORE_BRISTLES;
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

static void conversion_ready(CHAR_DATA *ch, long parent)
{
    ch->class = parent;
    ch->level = LEV_AWA;
    ch->position = POS_STANDING;
    ch->fighting = NULL;
    ch->hit = 1;                 /* Capacity, not current HP, is the gate. */
    ch->mana = 0;
    ch->max_hit = ch->max_mana = 50000;
    ch->pcdata->quest = HYBRID_DEFAULT_QUEST_COST;
    for (int stat = STAT_STR; stat <= STAT_WIS; ++stat)
        ch->stats[stat] = stat_max(ch, stat);
    for (long style = STYLE_UNARMED; style <= STYLE_TWOWEAPON; ++style) {
        ch->style[0] = style;
        ch->style[style] = LONG_MAX;
        improve_style(ch);
    }
    for (long skill = TYPE_UNARMED; skill <= TYPE_BLUNT; ++skill) {
        ch->weapon[skill] = LONG_MAX;
        improve_weapon(ch, skill);
    }
    for (long stance = STANCE_VIPER; stance <= STANCE_SWALLOW; ++stance) {
        ch->stance[0] = stance;
        ch->stance[stance] = LONG_MAX;
        improve_stance(ch);
    }
    if (parent == CLASS_MONK)
        for (int rank = 0; rank < 3; ++rank) ch->pcdata->monk[rank] = 8;
    else if (parent == CLASS_HIGHLANDER)
        for (int rank = 0; rank < 4; ++rank) ch->pcdata->highland[rank] = 6;
    else
        assert(0);
}

static void online(CHAR_DATA *ch)
{
    static char outbuf[32768];
    memset(outbuf, 0, sizeof(outbuf));
    shell.character = ch;
    shell.connected = CON_PLAYING;
    shell.original = NULL;
    shell.outbuf = outbuf;
    shell.outsize = sizeof(outbuf);
    ch->desc = &shell;
    descriptor_list = &shell;
    ch->next = char_list;
    char_list = ch;
}

static void offline_release(CHAR_DATA *ch)
{
    assert(ch && char_list == ch);
    char_list = ch->next;
    ch->next = NULL;
    descriptor_list = NULL;
    release(ch);
}

static void conversion_backend(void)
{
    const long parents[] = {CLASS_MONK, CLASS_HIGHLANDER};
    const long secondary[] = {CLASS_HIGHLANDER, CLASS_MONK};
    const enum atomic_save_fault pre_rename[] = {
        SAVE_FAULT_CREATE, SAVE_FAULT_FLUSH, SAVE_FAULT_SYNC,
        SAVE_FAULT_CLOSE, SAVE_FAULT_RENAME
    };

    for (size_t order = 0; order < 2; ++order) {
        CHAR_DATA *ch = fresh();
        const HYBRID_DEFINITION *definition =
            hybrid_by_parents(parents[order], secondary[order]);
        assert(definition);
        conversion_ready(ch, parents[order]);
        online(ch);

        descriptor_list = NULL;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_UNSAFE_CONTEXT);
        descriptor_list = &shell;
        ch->desc = NULL;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_UNSAFE_CONTEXT);
        ch->desc = &shell;

        /* A read-only preview is not authorization after balance or training
         * changes: the commit API recomputes prerequisites. */
        ch->pcdata->quest--;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_NOT_READY);
        assert(!ch->pcdata->hybrid_present &&
               ch->pcdata->quest == HYBRID_DEFAULT_QUEST_COST - 1 &&
               ch->class == parents[order]);
        ch->pcdata->quest++;
        if (parents[order] == CLASS_MONK) {
            ch->pcdata->monk[MONK_TECH] = 7;
            assert(hybrid_player_convert_commit(ch, definition) ==
                   HYBRID_CONVERSION_NOT_READY);
            ch->pcdata->monk[MONK_TECH] = 8;
        }

        /* Switched control, combat, non-standing and pending primary-save
         * recovery must not create a record or charge the player. */
        shell.original = ch;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_UNSAFE_CONTEXT);
        shell.original = NULL;
        ch->fighting = ch;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_UNSAFE_CONTEXT);
        ch->fighting = NULL;
        ch->position = POS_SLEEPING;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_UNSAFE_CONTEXT);
        ch->position = POS_STANDING;
        ch->save_pending |= PLAYER_SAVE_PRIMARY;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_SAVE_PENDING);
        ch->save_pending &= ~PLAYER_SAVE_PRIMARY;

        PC_DATA before = *ch->pcdata;
        CHAR_DATA before_char = *ch;
        for (size_t i = 0; i < sizeof(pre_rename)/sizeof(pre_rename[0]); ++i) {
            atomic_save_test_fault = pre_rename[i];
            assert(hybrid_player_convert_commit(ch, definition) ==
                   HYBRID_CONVERSION_SAVE_FAILED);
            assert(!ch->pcdata->hybrid_present &&
                   ch->pcdata->quest == before.quest &&
                   ch->class == before_char.class &&
                   !memcmp(ch->pcdata->monk, before.monk, sizeof(before.monk)) &&
                   !memcmp(ch->pcdata->highland, before.highland,
                           sizeof(before.highland)));
            /* Failed saves queue normal durability recovery; the transaction
             * itself still leaves training/currency unchanged. */
            ch->save_pending &= ~PLAYER_SAVE_PRIMARY;
            ch->save_retry_delay[0] = 0;
        }

        atomic_save_test_fault = SAVE_FAULT_NONE;
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_COMMITTED);
        assert(ch->pcdata->hybrid_present == 1 &&
               ch->pcdata->quest == 0 &&
               ch->class == parents[order] &&
               ch->primal == before_char.primal &&
               ch->exp == before_char.exp &&
               ch->gold == before_char.gold &&
               ch->hit == before_char.hit &&
               ch->mana == before_char.mana &&
               ch->max_hit == before_char.max_hit &&
               ch->max_mana == before_char.max_mana &&
               !memcmp(ch->stats, before_char.stats, sizeof(ch->stats)) &&
               !memcmp(ch->style, before_char.style, sizeof(ch->style)) &&
               !memcmp(ch->weapon, before_char.weapon, sizeof(ch->weapon)) &&
               !memcmp(ch->stance, before_char.stance, sizeof(ch->stance)) &&
               (ch->save_pending & PLAYER_SAVE_BACKUP));
        assert(ch->pcdata->hybrid.primary_class == parents[order] &&
               ch->pcdata->hybrid.secondary_class == secondary[order] &&
               ch->pcdata->hybrid.secondary_progression.full_branch == -1);
        for (size_t i = 0; i < 20; ++i)
            assert(ch->pcdata->hybrid.secondary_powers[i] == 0);
        assert(hybrid_player_convert_commit(ch, definition) ==
               HYBRID_CONVERSION_INVALID); /* Never double-charge. */
        char *disk = read_file("../player/Newfixture");
        assert(strstr(disk, "AnathemaHybrid 4 "));
        free(disk);
        offline_release(ch);

        for (int short_load = 0; short_load < 2; ++short_load) {
            memset(&shell, 0, sizeof(shell));
            assert(short_load ? load_char_short(&shell, "Newfixture") :
                                load_char_obj(&shell, "Newfixture"));
            ch = shell.character;
            assert(ch->pcdata->hybrid_present == 1 &&
                   !ch->pcdata->player_load_failed &&
                   ch->class == parents[order] &&
                   ch->pcdata->quest == 0 &&
                   ch->pcdata->hybrid.primary_class == parents[order] &&
                   ch->pcdata->hybrid.secondary_class == secondary[order]);
            release(ch);
        }
        assert(unlink("../player/Newfixture") == 0);
    }

    /* Directory fsync failure happens after rename: apply once and refuse a
     * second call rather than refunding a conversion that may now be durable. */
    CHAR_DATA *ch = fresh();
    const HYBRID_DEFINITION *definition =
        hybrid_by_parents(CLASS_MONK, CLASS_HIGHLANDER);
    assert(definition);
    conversion_ready(ch, CLASS_MONK);
    online(ch);
    atomic_save_test_fault = SAVE_FAULT_DIRSYNC;
    assert(hybrid_player_convert_commit(ch, definition) ==
           HYBRID_CONVERSION_DURABILITY_UNCERTAIN);
    assert(ch->pcdata->hybrid_present && ch->pcdata->quest == 0 &&
           (ch->save_pending & PLAYER_SAVE_PRIMARY));
    assert(hybrid_player_convert_commit(ch, definition) ==
           HYBRID_CONVERSION_INVALID);
    atomic_save_test_fault = SAVE_FAULT_NONE;
    offline_release(ch);
    assert(unlink("../player/Newfixture") == 0);
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
    core_only_compatibility();
    conversion_backend();
    assert(fclose(fpReserve) == 0 && chdir("..") == 0);
    assert(rmdir("player") == 0 && rmdir("backup") == 0 &&
           rmdir("run") == 0 && chdir(cwd) == 0 && rmdir(root) == 0);
    puts("hybrid player: 90 initialized pairs, both real loaders, native preservation, "
         "legacy saves, atomic failures, conversion preflight/save/retry and "
         "fail-closed corruption tests passed");
    return 0;
}
