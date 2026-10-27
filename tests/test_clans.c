/* Real interpreter, registry, player-save and class-alias integration.
 * Only private synthetic players and a temporary runtime are used.
 */
#define ATOMIC_SAVE_TESTING
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "merc.h"
#include "atomic_save.h"
#include "class_factions.h"

extern char *string_space, *top_string;
extern void init_mm(void);
extern void mobile_update(void);
static CHAR_DATA actors[5];
static PC_DATA pcs[5];
static DESCRIPTOR_DATA descriptors[5];
static char outputs[5][65536];
static ROOM_INDEX_DATA room, elsewhere;
static AREA_DATA area;
static const char *names[] = {"Leader", "Member", "Other", "Visitor", "Staff"};
static const long classes[] = {CLASS_VAMPIRE, CLASS_WEREWOLF, CLASS_SORCERER,
    CLASS_DROW, CLASS_HIGHLANDER, CLASS_MAGE, CLASS_DRAGON, CLASS_NINJA,
    CLASS_MONK, CLASS_ELEM};

static void fixture(long class_id)
{
    memset(&room, 0, sizeof(room));
    memset(&elsewhere, 0, sizeof(elsewhere));
    room.area = elsewhere.area = &area;
    room.light = elsewhere.light = 1;
    for (int i = 0; i < 5; ++i) {
        free_string(actors[i].clan);
        memset(&pcs[i], 0, sizeof(pcs[i]));
        memset(&descriptors[i], 0, sizeof(descriptors[i]));
        clear_char(&actors[i]);
        actors[i].name = (char *)names[i];
        actors[i].class = i == 4 ? CLASS_NONE : class_id;
        actors[i].level = i == 4 ? LEV_IMM : LEV_AWA;
        actors[i].pcdata = &pcs[i];
        actors[i].desc = &descriptors[i];
        actors[i].in_room = &room;
        actors[i].position = POS_STANDING;
        actors[i].hit = actors[i].max_hit = 10000;
        actors[i].mana = actors[i].max_mana = 10000;
        actors[i].vampgen = 9;
        actors[i].logon = current_time;
        actors[i].stance[0] = -1;
        actors[i].next = i < 4 ? &actors[i+1] : NULL;
        actors[i].next_in_room = i < 4 ? &actors[i+1] : NULL;
        descriptors[i].character = &actors[i];
        descriptors[i].connected = CON_PLAYING;
        descriptors[i].fcommand = TRUE;
        descriptors[i].outbuf = outputs[i];
        descriptors[i].outsize = sizeof(outputs[i]);
        descriptors[i].next = i < 4 ? &descriptors[i+1] : NULL;
        pcs[i].pwd = pcs[i].bamfin = pcs[i].bamfout = "";
        pcs[i].cdest = pcs[i].title = pcs[i].decap_char = "";
        pcs[i].marriage = pcs[i].email = pcs[i].covenant = "";
        pcs[i].cov_time = pcs[i].leader = pcs[i].vleader = "";
        pcs[i].love = pcs[i].conding = "";
        pcs[i].cland[0] = pcs[i].cland[1] = pcs[i].cland[2] = -1;
        pcs[i].rank = 1;
    }
    char_list = room.people = &actors[0];
    descriptor_list = &descriptors[0];
    atomic_save_test_fault = SAVE_FAULT_NONE;
}

static void run(int who, const char *text)
{
    for (int i = 0; i < 5; ++i) {
        outputs[i][0] = '\0'; descriptors[i].outtop = 0;
    }
    snprintf(descriptors[who].incomm, sizeof(descriptors[who].incomm), "%s", text);
    interpret(&actors[who], descriptors[who].incomm);
}

static char *snapshot(void)
{
    FILE *fp = fopen("../clans/Registry", "r");
    char *s = calloc(262145, 1);
    assert(fp != NULL && s != NULL);
    size_t n = fread(s, 1, 262144, fp);
    assert(!ferror(fp) && n < 262144);
    fclose(fp);
    return s;
}

static void unchanged(int who, const char *command)
{
    char *before = snapshot(), *after;
    run(who, command);
    after = snapshot();
    assert(!strcmp(before, after));
    free(before); free(after);
}

static void role(int who, const char *expected)
{
    char needle[64];
    snprintf(needle, sizeof(needle), "your role: %s.", expected);
    run(who, "clan info");
    if (strstr(outputs[who], needle) == NULL) fprintf(stderr, "%s", outputs[who]);
    assert(strstr(outputs[who], needle) != NULL);
}

static long own_id(void)
{
    long id = 0;
    run(0, "clan");
    char *start = strstr(outputs[0], "Clan ID ");
    assert(start != NULL && sscanf(start, "Clan ID %ld:", &id) == 1 && id > 0);
    return id;
}

static void disband_admin(void)
{
    char command[128];
    snprintf(command, sizeof(command), "clanadmin disband %ld confirm", own_id());
    run(4, command);
    assert(!clan_identity_reserved("Leader"));
}

static void join_other(void)
{
    run(0, "clan invite Other");
    assert(strstr(outputs[2], "clan accept Leader"));
    assert(!actors[2].clan[0]);
    run(2, "clan accept Leader");
    assert(!strcmp(actors[2].clan, "Order"));
}

static void class_cycle(long class_id)
{
    char command[128];
    fixture(class_id);
    run(4, "clanadmin create Leader Order");
    assert(!strcmp(actors[0].clan, "Order"));
    assert(clan_member_role(NULL) == 0 && clan_leader_name(NULL) == NULL);
    assert(clan_member_role(&actors[0]) == 3);
    assert(!strcmp(clan_leader_name(&actors[0]), "Leader"));
    long original_class = actors[0].class;
    actors[0].class = CLASS_NONE;
    assert(clan_member_role(&actors[0]) == 0);
    assert(clan_leader_name(&actors[0]) == NULL);
    actors[0].class = original_class;
    SET_BIT(actors[0].act, ACT_IS_NPC);
    assert(clan_member_role(&actors[0]) == 0);
    assert(clan_leader_name(&actors[0]) == NULL);
    REMOVE_BIT(actors[0].act, ACT_IS_NPC);
    assert(clan_member_role(&actors[1]) == 0);
    assert(clan_leader_name(&actors[1]) == NULL);
    assert(!strcmp(clan_affiliation(&actors[1]), "Loner"));
    assert(!strcmp(clan_affiliation(&actors[4]), "Unclassed"));
    /* Cached labels, legacy lord names and maximum power never grant office. */
    actors[2].clan = str_dup("Order");
    char *old_lord = actors[2].lord;
    actors[2].lord = (char *)"Leader";
    actors[2].vampgen = 1;
    assert(clan_member_role(&actors[2]) == 0);
    assert(clan_leader_name(&actors[2]) == NULL);
    actors[2].vampgen = 9;
    actors[2].lord = old_lord;
    free_string(actors[2].clan);
    actors[2].clan = str_dup("");
    role(0, "Leader");
    assert(strstr(outputs[0], "Faction and clan leader: Leader."));
    unchanged(0, "clan leave");
    unchanged(1, "clanadmin create Member Stolen");
    unchanged(1, "clan promote Leader");
    unchanged(0, "clan invite Leader");
    unchanged(0, "clan invite Nobody");
    actors[1].class = CLASS_NONE;
    unchanged(0, "clan invite Member");
    actors[1].class = class_id;
    switch (class_id) {
    case CLASS_NINJA: run(0, "humble Member"); break;
    case CLASS_DROW: run(0, "dinduct Member"); break;
    case CLASS_MONK: run(0, "decide Member"); break;
    case CLASS_VAMPIRE: case CLASS_WEREWOLF: run(0, "favour Member induct"); break;
    default: run(0, "clan invite Member"); break;
    }
    assert(strstr(outputs[1], "clan accept Leader"));
    assert(!actors[1].clan[0]); /* No blanket willingness flag can auto-enroll. */
    unchanged(1, "clan accept Other");
    char *before = snapshot();
    char forced[] = "clan accept Leader";
    descriptors[1].incomm[0] = '\0';
    interpret(&actors[1], forced);
    char *after = snapshot();
    assert(!strcmp(before, after) && !actors[1].clan[0]);
    assert(strstr(outputs[1], "own input"));
    free(before); free(after);
    actors[1].affected_by = AFF_SHADOWPLANE;
    unchanged(1, "clan accept Leader");
    actors[1].affected_by = 0;
    actors[1].in_room = &elsewhere;
    unchanged(1, "clan accept Leader");
    actors[1].in_room = &room;
    actors[1].fighting = &actors[2];
    unchanged(1, "clan accept Leader");
    actors[1].fighting = NULL;
    descriptors[1].original = &actors[2];
    unchanged(1, "clan accept Leader");
    descriptors[1].original = NULL;
    run(1, "clan decline");
    unchanged(1, "clan accept Leader");
    run(0, "clan invite Member");
    current_time += 121;
    unchanged(1, "clan accept Leader");
    run(0, "clan invite Member");
    run(1, "clan accept Leader");
    assert(!strcmp(actors[1].clan, "Order"));
    assert(clan_member_role(&actors[1]) == 1);
    assert(!strcmp(clan_affiliation(&actors[1]), "Member"));
    assert(!strcmp(clan_leader_name(&actors[1]), "Leader"));
    role(1, "Member");
    unchanged(1, "clan invite Other");
    unchanged(1, "clan kick Leader");
    switch (class_id) {
    case CLASS_NINJA: run(0, "promote Member"); break;
    case CLASS_DROW: run(0, "promotion Member"); break;
    case CLASS_VAMPIRE: case CLASS_WEREWOLF: run(0, "favour Member elder"); break;
    default: run(0, "clan promote Member"); break;
    }
    role(1, "Officer");
    assert(clan_member_role(&actors[1]) == 2);
    /* Leadership resolves by registry name even with no online leader. */
    CHAR_DATA *old_head = char_list;
    char_list = &actors[1];
    assert(!strcmp(clan_leader_name(&actors[1]), "Leader"));
    char_list = old_head;
    if (class_id == CLASS_NINJA || class_id == CLASS_DROW) {
        run(0, class_id == CLASS_NINJA ? "demote Member" : "failure Member");
        role(1, "Member");
        run(0, class_id == CLASS_NINJA ? "promote Member" : "promotion Member");
        role(1, "Officer");
    }
    unchanged(1, "clan kick Leader");
    unchanged(1, "clan demote Leader");
    unchanged(1, "clan promote Other");
    assert(clan_block_class_change(&actors[0]));
    assert(clan_block_class_change(&actors[1]));
    assert(clan_identity_reserved("lEaDeR"));
    assert(clan_block_name_change(&actors[0], "Hijacked"));
    assert(clan_block_name_change(&actors[2], "Order"));
    unchanged(4, "clanname Leader Hijacked");
    unchanged(4, "mclear Leader");
    unchanged(4, "clearclass Leader");
    unchanged(4, "pcrename Leader Renamed");
    join_other();
    unchanged(1, "clan promote Other"); /* Officers cannot create peer officers. */
    unchanged(1, "clan kick Member");   /* No self-removal through kick. */
    assert(save_char_obj_checked(&actors[2])); /* Stale player cache on disk. */
    actors[1].next = &actors[3]; /* Registered Other is now offline. */
    if (class_id == CLASS_MONK) run(1, "expel Other");
    else if (class_id == CLASS_VAMPIRE || class_id == CLASS_WEREWOLF)
        run(1, "favour Other outcast");
    else run(1, "outcast Other");
    assert(!strcmp(actors[2].clan, "Order")); /* Not mutated through a stale pointer. */
    assert(!strcmp(clan_affiliation(&actors[2]), "Exiled"));
    assert(clan_load_registry());
    DESCRIPTOR_DATA reloaded = {0};
    assert(load_char_obj(&reloaded, "Other"));
    assert(!reloaded.character->clan[0]);
    assert(!strcmp(clan_affiliation(reloaded.character), "Exiled"));
    free_char(reloaded.character);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    clan_sync_player(&actors[2]);
    assert(!actors[2].clan[0]);
    actors[1].next = &actors[2];
    run(0, "clan transfer Member");
    unchanged(1, "clan accept Other");
    run(1, "clan accept Leader");
    role(0, "Officer"); role(1, "Leader");
    unchanged(0, "clan kick Member");
    run(0, "clan invite Visitor");
    run(1, "clan demote Leader");
    role(0, "Member");
    unchanged(3, "clan accept Leader"); /* Revoked inviter authority is rechecked. */
    run(1, "clan invite Visitor");
    run(3, "clan accept Member");
    assert(!strcmp(actors[3].clan, "Order"));
    run(3, "clan leave");
    assert(!actors[3].clan[0]);
    assert(!strcmp(clan_affiliation(&actors[3]), "Loner"));
    /* Voluntary return to a different leader and re-admission clear exile.
     * A pending offer alone does not clear it. */
    run(1, "clan invite Other");
    assert(!strcmp(clan_affiliation(&actors[2]), "Exiled"));
    run(2, "clan accept Member");
    assert(!strcmp(clan_affiliation(&actors[2]), "Member"));
    run(2, "clan leave");
    assert(!strcmp(clan_affiliation(&actors[2]), "Loner"));
    assert(clan_load_registry());
    assert(!strcmp(clan_affiliation(&actors[2]), "Loner"));
    unchanged(1, "clan disband");
    for (int i = 0; i < 4; ++i) {
        assert(actors[i].vampgen == 9 && pcs[i].rank == 1);
        assert(actors[i].hit == 10000 && actors[i].mana == 10000);
        assert(pcs[i].cland[0] == -1 && pcs[i].powers[DISC_PROTEAN] == 0);
    }
    snprintf(command, sizeof(command), "clan disband confirm");
    run(1, command);
    assert(!actors[0].clan[0] && !actors[1].clan[0]);
    assert(clan_load_registry());
    assert(!clan_block_class_change(&actors[0]));
    assert(!clan_identity_reserved("Leader"));
}

static void faults(long class_id)
{
    const enum atomic_save_fault failures[] = {SAVE_FAULT_CREATE, SAVE_FAULT_FLUSH,
        SAVE_FAULT_SYNC, SAVE_FAULT_CLOSE, SAVE_FAULT_RENAME};
    fixture(class_id);
    run(4, "clanadmin create Leader Order");
    actors[0].vampgen = 1;
    actors[2].class = CLASS_WEREWOLF;
    unchanged(0, "favour Other elder");
    assert(pcs[2].rank == 1 && actors[2].vampgen == 9);
    actors[0].vampgen = 9;
    actors[2].class = class_id;
    run(0, "clan invite Member");
    run(1, "clan accept Leader");
    run(0, "clan transfer Member");
    for (size_t i = 0; i < sizeof(failures)/sizeof(failures[0]); ++i) {
        atomic_save_test_fault = failures[i];
        unchanged(1, "clan accept Leader");
        atomic_save_test_fault = SAVE_FAULT_NONE;
        role(0, "Leader"); role(1, "Member");
    }
    atomic_save_test_fault = SAVE_FAULT_DIRSYNC;
    run(1, "clan accept Leader");
    role(0, "Officer"); role(1, "Leader");
    unchanged(1, "clan kick Leader"); /* Visible state is frozen, not rolled back. */
    assert(strstr(outputs[1], "unavailable"));
    unchanged(4, "clanadmin retry"); /* Still uncertain with injected dirsync failure. */
    atomic_save_test_fault = SAVE_FAULT_RENAME;
    unchanged(4, "clanadmin retry");
    atomic_save_test_fault = SAVE_FAULT_NONE;
    run(4, "clanadmin retry");
    run(1, "clan demote Leader");
    role(0, "Member");
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        clan_registry_close();
        _exit(clan_load_registry() ? 1 : 0);
    }
    int status;
    assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
    run(1, "clan disband confirm");
}

static void write_text(const char *text)
{
    FILE *fp = fopen("../clans/Registry", "w");
    assert(fp != NULL && fputs(text, fp) >= 0 && fclose(fp) == 0);
}

static void corrupt_files(void)
{
    static const char *bad[] = {
        "", "AnathemaClans 4 2\nEnd\n",
        "AnathemaClans 3 2\nM 0 32 0 Other 0 0 extra\nEnd\n",
        "AnathemaClans 3 2\nM 0 32 0 Other 0 -1\nEnd\n",
        "AnathemaClans 3 2\nM 0 32 0 Other 0 9999999999999999999999\nEnd\n",
        "AnathemaClans 3 2\nM 0 32 0 Other 0 33\nEnd\n",
        "AnathemaClans 3 2\nM 0 32 0 Other 0 1\nEnd\n",
        "AnathemaClans 3 2\nM 0 32 0 Other 0\nEnd\n",
        "AnathemaClans 2 2\nM 0 32 0 Other -1\nEnd\n",
        "AnathemaClans 2 2\nM 0 32 0 Other 2\nEnd\n",
        "AnathemaClans 2 2\nM 0 32 0 Other 99999999999999999999999\nEnd\n",
        "AnathemaClans 2 2\nM 0 32 0 Other\nEnd\n",
        "AnathemaClans 1 2\nM 0 32 0 Other 1\nEnd\n",
        "AnathemaClans 2 2\nC 1 32 4f72646572\nM 1 32 3 Leader 1\nEnd\n",
        "AnathemaClans 1 999999999999999999999999999\nEnd\n",
        "AnathemaClans 1 1\nC 1 32 4f72646572\nM 1 32 3 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 3 4f72646572\nM 1 3 3 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 3 Leader\nM 1 32 3 Member\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 1 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 1 3 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 3 Leader\nM 0 32 0 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 3 leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f0072646572\nM 1 32 3 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 ff\nM 1 32 3 Leader\nEnd\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 3 Leader\n",
        "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 3 Leader\nEnd\nExtra\n",
        "AnathemaClans 1 2\nC -1 32 4f72646572\nM 1 32 3 Leader\nEnd\n",
        "AnathemaClans 1 2\nM 0 32 1 Leader\nEnd\n",
        "AnathemaClans 1 2\nM 1 32 3 Leader\nEnd\n"
    };
    fixture(CLASS_MAGE);
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
        clan_registry_close();
        write_text(bad[i]);
        assert(!clan_load_registry());
        unchanged(4, "clanadmin create Leader Order");
        assert(clan_identity_reserved("Fresh"));
        assert(clan_block_class_change(&actors[0]));
    }
    clan_registry_close();
    write_text("AnathemaClans 1 1\nEnd\n");
    assert(chmod("../clans/Registry", 0666) == 0);
    assert(!clan_load_registry());
    assert(chmod("../clans/Registry", 0600) == 0);
    clan_registry_close();
    assert(rename("../clans/Registry", "../outside") == 0);
    assert(symlink("../outside", "../clans/Registry") == 0);
    assert(!clan_load_registry());
    assert(unlink("../clans/Registry") == 0);
    assert(rename("../outside", "../clans/Registry") == 0);
    clan_registry_close();
    assert(clan_load_registry());
    /* A missing registry beside an existing process-lock file is not a new installation. */
    assert(unlink("../clans/Registry") == 0);
    clan_registry_close();
    assert(!clan_load_registry());
    write_text("AnathemaClans 1 1\nEnd\n");
    clan_registry_close();
    assert(clan_load_registry());
}

static void generated_name(char *out, const char *prefix, int i)
{
    sprintf(out, "%s%c%c%c", prefix, 'a' + i/(26*26), 'a' + i/26%26, 'a' + i%26);
}

static void limits_and_admin(void)
{
    FILE *fp;
    char name[16];
    fixture(CLASS_MAGE);
    clan_registry_close();
    fp = fopen("../clans/Registry", "w"); assert(fp);
    fprintf(fp, "AnathemaClans 1 65\n");
    for (int i = 0; i < 64; ++i) {
        generated_name(name, "Keeper", i);
        fprintf(fp, "C %d 32 4f72646572%02x%02x\nM %d 32 3 %s\n",
                i+1, 'a'+i/26, 'a'+i%26, i+1, name);
    }
    fprintf(fp, "End\n"); assert(fclose(fp) == 0);
    assert(clan_load_registry());
    unchanged(4, "clanadmin create Leader Overflow");
    clan_registry_close();
    fp = fopen("../clans/Registry", "w"); assert(fp);
    fprintf(fp, "AnathemaClans 1 2\nC 1 32 4f72646572\nM 1 32 3 Leader\n");
    for (int i = 0; i < 1023; ++i) {
        generated_name(name, "User", i);
        fprintf(fp, "M 1 32 1 %s\n", name);
    }
    fprintf(fp, "End\n"); assert(fclose(fp) == 0);
    assert(clan_load_registry());
    clan_sync_player(&actors[0]);
    run(0, "clan roster 1");
    assert(strstr(outputs[0], "page 1 of 52") && strlen(outputs[0]) < 4000);
    run(0, "clan roster 52");
    assert(strstr(outputs[0], "page 52 of 52") && strlen(outputs[0]) < 4000);
    unchanged(0, "clan roster 53");
    unchanged(0, "clan roster 9999999999999999999999999999999");
    run(0, "clan invite Member");
    unchanged(1, "clan accept Leader");
    assert(strstr(outputs[1], "capacity"));
    disband_admin();
    clan_registry_close();
    write_text("AnathemaClans 1 1\nEnd\n");
    assert(clan_load_registry());
    fixture(CLASS_MAGE);
    run(4, "clanadmin create Leader Order");
    unchanged(4, "clanadmin create Member order");
    unchanged(4, "clanadmin create Other ../escape");
    unchanged(4, "clanadmin create Other #RUnsafe");
    unchanged(4, "generation Other 0");
    unchanged(4, "generation Other 14");
    unchanged(4, "generation Other 999999999999999999999999999");
    assert(actors[2].vampgen == 9);
    disband_admin();
}

static void native_recruitment(void)
{
    const struct { long id; const char *command; } cases[] = {
        {CLASS_VAMPIRE,"vampire ventrue"}, {CLASS_VAMPIRE,"vampire nosferatu"},
        {CLASS_VAMPIRE,"vampire brujah"}, {CLASS_VAMPIRE,"vampire assamite"},
        {CLASS_VAMPIRE,"vampire gangrel"}, {CLASS_VAMPIRE,"vampire serpentsofthelight"},
        {CLASS_VAMPIRE,"vampire lasombra"}, {CLASS_VAMPIRE,"vampire tzimisce"},
        {CLASS_VAMPIRE,"vampire ravnos"}, {CLASS_WEREWOLF,"werewolf"},
        {CLASS_SORCERER,"sorcerer"}, {CLASS_MONK,"monk"},
        {CLASS_DRAGON,"dragon bronze"}, {CLASS_DRAGON,"dragon red"},
        {CLASS_DRAGON,"dragon blue"}, {CLASS_DRAGON,"dragon silver"},
        {CLASS_DRAGON,"dragon gold"}, {CLASS_DRAGON,"dragon black"},
        {CLASS_HIGHLANDER,"highlander"}, {CLASS_NINJA,"ninja"},
        {CLASS_DROW,"drow"}, {CLASS_ELEM,"elementalist"},
        {CLASS_MAGE,"mage akashic"}, {CLASS_MAGE,"mage celestial"},
        {CLASS_MAGE,"mage ecstasy"}, {CLASS_MAGE,"mage dreamspeakers"},
        {CLASS_MAGE,"mage euthanatos"}, {CLASS_MAGE,"mage hermes"},
        {CLASS_MAGE,"mage ether"}, {CLASS_MAGE,"mage verbena"},
        {CLASS_MAGE,"mage virtual"}, {CLASS_MAGE,"mage hollow"}
    };
    for (size_t n = 0; n < sizeof(cases)/sizeof(cases[0]); ++n) {
        fixture(cases[n].id);
        char command[160];
        for (int i = 0; i < 4; ++i)
            for (int s = 0; s < 5; ++s) actors[i].stats[s] = 4;
        actors[0].class = actors[1].class = actors[2].class = CLASS_NONE;
        snprintf(command, sizeof(command), "autoclass %s", cases[n].command);
        run(0, command); run(2, command);
        assert(actors[0].class == cases[n].id && actors[2].class == cases[n].id);
        snprintf(command, sizeof(command), "clanadmin create Leader %s",
            actors[0].clan[0] ? actors[0].clan : "Order");
        run(4, command);
        assert(clan_member_role(&actors[0]) == 3);
        run(0, "clan invite Other");
        assert(strstr(outputs[2], "clan accept Leader"));
        run(2, "clan accept Leader");
        assert(clan_member_role(&actors[2]) == 1);
        if (n == 0) {
            /* Social labels are not bloodlines: retain the native build after
             * leaving Ventrue, then sponsor into a custom-named clan. */
            disband_admin();
            run(4, "clanadmin create Leader Order");
            run(0, "clan invite Other"); run(2, "clan accept Leader");
            assert(clan_member_role(&actors[2]) == 1);
        }
        unchanged(2, "clan invite Visitor"); /* Classing is a specific exception. */

        CHAR_DATA native = actors[1];
        PC_DATA native_pc = pcs[1];
        native.pcdata = &native_pc;
        native.clan = str_dup("");
        native.desc = NULL;
        snprintf(command, sizeof(command), "%s", cases[n].command);
        do_autoclass(&native, command);
        assert(native.class == cases[n].id);
        free_string(native.clan);

        const char *offer = cases[n].id == CLASS_VAMPIRE ? "clan embrace Member" :
            cases[n].id == CLASS_MAGE ? "clan teach Member" : "clan recruit Member";
        pcs[1].hybrid_present = 1;
        unchanged(2, offer);
        pcs[1].hybrid_present = 0;
        run(2, offer);
        assert(actors[1].class == CLASS_NONE && !actors[1].clan[0]);
        assert(strstr(outputs[1], cases[n].command) &&
               strstr(outputs[1], "clan accept Other") &&
               strstr(outputs[1], "as a Member"));
        run(0, "clan kick Other");
        unchanged(1, "clan accept Other"); /* Revoked sponsors cannot class anyone. */
        run(0, "clan invite Other");
        run(2, "clan accept Leader");
        run(2, offer);
        unchanged(1, "clan accept Leader");
        char forced[] = "clan accept Other";
        descriptors[1].incomm[0] = '\0';
        interpret(&actors[1], forced);
        assert(actors[1].class == CLASS_NONE);
        assert(strstr(outputs[1], "own input"));
        actors[1].affected_by = AFF_SHADOWPLANE;
        unchanged(1, "clan accept Other");
        actors[1].affected_by = 0;
        actors[1].fighting = &actors[3];
        unchanged(1, "clan accept Other");
        actors[1].fighting = NULL;
        actors[1].in_room = &elsewhere;
        unchanged(1, "clan accept Other");
        actors[1].in_room = &room;
        for (int s = 0; s < 5; ++s) actors[1].stats[s] = 0;
        unchanged(1, "clan accept Other"); /* Prerequisites rechecked at acceptance. */
        assert(actors[1].class == CLASS_NONE);
        for (int s = 0; s < 5; ++s) actors[1].stats[s] = 4;
        current_time += 121;
        unchanged(1, "clan accept Other");
        run(2, offer);
        atomic_save_test_fault = SAVE_FAULT_CREATE;
        unchanged(1, "clan accept Other");
        atomic_save_test_fault = SAVE_FAULT_NONE;
        assert(actors[1].class == CLASS_NONE && clan_member_role(&actors[1]) == 0);
        run(1, "clan accept Other");
        assert(actors[1].class == native.class && actors[1].level == native.level &&
               actors[1].vampgen == native.vampgen && actors[1].arete == native.arete);
        assert(!memcmp(pcs[1].powers, native_pc.powers, sizeof(pcs[1].powers)));
        assert(!memcmp(pcs[1].cland, native_pc.cland, sizeof(pcs[1].cland)));
        assert(!memcmp(pcs[1].tgift, native_pc.tgift, sizeof(pcs[1].tgift)));
        assert(pcs[1].rank == native_pc.rank && pcs[1].dragcol == native_pc.dragcol);
        assert(actors[1].hit == 10000 && actors[1].mana == 10000);
        assert(clan_member_role(&actors[1]) == 1);
        assert(!strcmp(clan_leader_name(&actors[1]), "Leader"));
        unchanged(2, offer); /* Never overwrite an existing class/training. */
        DESCRIPTOR_DATA reload = {0};
        assert(load_char_obj(&reload, "Member"));
        assert(!reload.character->pcdata->player_load_failed &&
               reload.character->class == cases[n].id);
        assert(clan_member_role(reload.character) == 1);
        free_char(reload.character);
        if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
        disband_admin();
    }
}

static void enrollment_recovery(void)
{
    fixture(CLASS_VAMPIRE);
    actors[1].class = CLASS_NONE;
    for (int s = 0; s < 5; ++s) actors[1].stats[s] = 4;
    assert(save_char_obj_checked(&actors[1]));
    /* Crash boundary: consent/intent durable, initialized player not saved.
     * Only this owner's synthetic registry is edited; no real account loads. */
    write_text("AnathemaClans 3 2\nC 1 1 56656e74727565\n"
               "M 1 1 3 Leader 0 0\nM 1 1 1 Member 0 1\nEnd\n");
    assert(clan_load_registry());
    DESCRIPTOR_DATA reload = {0};
    assert(load_char_obj(&reload, "Member"));
    assert(reload.character->class == CLASS_VAMPIRE &&
           !reload.character->pcdata->player_load_failed);
    assert(reload.character->pcdata->cland[0] == DISC_DOMINATE);
    free_char(reload.character);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    clan_sync_player(&actors[1]);
    assert(actors[1].class == CLASS_VAMPIRE && clan_block_class_change(&actors[1]));
    pcs[1].powers[DISC_DOMINATE] = 4;
    clan_sync_player(&actors[1]);
    assert(pcs[1].powers[DISC_DOMINATE] == 4); /* Recovery is not retraining. */
    atomic_save_test_fault = SAVE_FAULT_CREATE;
    unchanged(1, "clan settle");
    atomic_save_test_fault = SAVE_FAULT_NONE;
    assert(clan_block_class_change(&actors[1]));
    run(1, "clan settle");
    char *saved = snapshot();
    assert(strstr(saved, "AnathemaClans 1 ") && !strstr(saved, "AnathemaClans 3 "));
    free(saved);
    assert(load_char_obj(&reload, "Member"));
    assert(reload.character->pcdata->powers[DISC_DOMINATE] == 4);
    free_char(reload.character);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    /* A conflicting saved class is rejected, never overwritten. */
    actors[1].class = CLASS_MAGE;
    assert(save_char_obj_checked(&actors[1]));
    write_text("AnathemaClans 3 2\nC 1 1 56656e74727565\n"
               "M 1 1 3 Leader 0 0\nM 1 1 1 Member 0 1\nEnd\n");
    assert(clan_load_registry() && load_char_obj(&reload, "Member"));
    assert(reload.character->class == CLASS_MAGE &&
           reload.character->pcdata->player_load_failed);
    free_char(reload.character);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    /* Restore the synthetic conflict, then settle and cleanly disband. */
    actors[1].class = CLASS_VAMPIRE;
    run(1, "clan settle");
    disband_admin();

    /* Uncertain registry writes cannot be bypassed through autoclass. */
    fixture(CLASS_VAMPIRE);
    actors[1].class = CLASS_NONE;
    for (int s = 0; s < 5; ++s) actors[1].stats[s] = 4;
    assert(save_char_obj_checked(&actors[1]));
    write_text("AnathemaClans 3 2\nC 1 1 56656e74727565\n"
               "M 1 1 3 Leader 0 0\nM 1 1 1 Member 0 1\nEnd\n");
    assert(clan_load_registry());
    atomic_save_test_fault = SAVE_FAULT_DIRSYNC;
    run(4, "clanadmin retry");
    atomic_save_test_fault = SAVE_FAULT_NONE;
    assert(actors[1].class == CLASS_NONE);
    unchanged(1, "autoclass monk");
    assert(actors[1].class == CLASS_NONE);
    run(4, "clanadmin retry");
    assert(actors[1].class == CLASS_VAMPIRE);
    run(1, "clan settle");
    disband_admin();
}

static void nightstalker(void)
{
    fixture(CLASS_NINJA);
    actors[0].class = CLASS_NONE;
    for (int i = 0; i < 5; ++i) actors[0].stats[i] = 4;
    run(0, "autoclass ninja nonsense");
    assert(actors[0].class == CLASS_NONE);
    run(0, "autoclass ninja nightstalker");
    assert(IS_NIGHTSTALKER(&actors[0]));
    assert(!IS_NIGHTSTALKER(NULL) && !IS_NIGHTSTALKER(&actors[1]));
    run(4, "clanadmin create Leader ShadowOrder");
    assert(clan_member_role(&actors[0]) == 3);
    actors[2].class = CLASS_NONE;
    for (int i = 0; i < 5; ++i) actors[2].stats[i] = 4;
    run(0, "clan recruit Other");
    assert(actors[2].class == CLASS_NONE);
    assert(strstr(outputs[2], "ninja nightstalker"));
    run(2, "clan accept Leader");
    assert(IS_NIGHTSTALKER(&actors[2]) && clan_member_role(&actors[2]) == 1);
    run(0, "clan kick Other");
    assert(IS_NIGHTSTALKER(&actors[2]));
    assert(!strcmp(clan_affiliation(&actors[2]), "Exiled"));
    run(0, "clan invite Other"); run(2, "clan accept Leader");
    run(2, "clan leave");
    assert(IS_NIGHTSTALKER(&actors[2]));
    assert(!strcmp(clan_affiliation(&actors[2]), "Loner"));
    assert(save_char_obj_checked(&actors[2]));
    DESCRIPTOR_DATA reloaded;
    memset(&reloaded, 0, sizeof reloaded);
    assert(load_char_obj(&reloaded, "Other"));
    assert(!reloaded.character->pcdata->player_load_failed);
    assert(IS_NIGHTSTALKER(reloaded.character));
    free_char(reloaded.character);
    memset(&reloaded, 0, sizeof reloaded);
    assert(load_char_short(&reloaded, "Other"));
    assert(!reloaded.character->pcdata->player_load_failed);
    assert(IS_NIGHTSTALKER(reloaded.character));
    free_char(reloaded.character);
    /* The reset handler owns these strings; unlike this compact fixture's
     * other read-only placeholders, real loaded players use engine strings. */
    pcs[2].conding = pcs[2].love = pcs[2].covenant =
        pcs[2].cov_time = pcs[2].leader = pcs[2].vleader = str_dup("");
    actors[4].level = LEV_IMP;
    run(4, "clearclass Other");
    actors[4].level = LEV_IMM;
    assert(actors[2].class == CLASS_NONE && pcs[2].cland[0] == 0);

    /* A clan label alone cannot convert an ordinary Ninja. */
    free_string(actors[1].clan);
    actors[1].clan = str_dup("Nightstalkers");
    assert(!IS_NIGHTSTALKER(&actors[1]));
    pcs[0].ninja[NINJA_SORA] = pcs[0].ninja[NINJA_CHIKYU] = 10;
    pcs[0].ninja[NINJA_KI] = 100;
    run(0, "michi");
    assert(pcs[0].wolf == 0 && pcs[0].ninja[NINJA_KI] == 100);
    run(0, "principle sora");
    assert(!strstr(outputs[0], "1 : Michi") && strstr(outputs[0], "60 Ki"));
    for (int ki = 0; ki <= 60; ++ki) {
        actors[0].affected_by = 0;
        pcs[0].ninja[NINJA_KI] = ki;
        run(0, "kakusu");
        assert(!!IS_AFFECTED(&actors[0], AFF_KAKUSU) == (ki == 60));
        assert(pcs[0].ninja[NINJA_KI] == (ki == 60 ? 0 : ki));
    }
    run(0, "kakusu"); /* Free dismissal, even with no remaining Ki. */
    assert(!IS_AFFECTED(&actors[0], AFF_KAKUSU));
    for (int ki = 0; ki <= 60; ++ki) {
        actors[0].act = 0;
        pcs[0].ninja[NINJA_KI] = ki;
        run(0, "heighten");
        assert(!!IS_SET(actors[0].act, PLR_HOLYLIGHT) == (ki == 60));
        assert(pcs[0].ninja[NINJA_KI] == (ki == 60 ? 0 : ki));
    }
    run(0, "heighten");
    assert(!IS_SET(actors[0].act, PLR_HOLYLIGHT));
    pcs[1].ninja[NINJA_SORA] = 1;
    pcs[1].ninja[NINJA_KI] = 100;
    run(1, "michi");
    assert(pcs[1].wolf == 25 && pcs[1].ninja[NINJA_KI] == 0);
    pcs[1].ninja[NINJA_SORA] = 6;
    pcs[1].ninja[NINJA_KI] = 20;
    run(1, "kakusu");
    assert(IS_AFFECTED(&actors[1], AFF_KAKUSU));
    assert(pcs[1].ninja[NINJA_KI] == 0);
    actors[1].affected_by = 0;

    actors[3].class = CLASS_VAMPIRE;
    actors[3].vampaff = VAM_OBFUSCATE;
    pcs[3].powers[DISC_OBFUSCATE] = 5;
    assert(can_see(&actors[0], &actors[3]));
    assert(!can_see(&actors[1], &actors[3]));
    actors[0].affected_by = AFF_BLIND;
    assert(!can_see(&actors[0], &actors[3]));
    actors[0].affected_by = 0;
    actors[3].affected_by = AFF_SHADOWPLANE;
    assert(!can_see(&actors[0], &actors[3]));
    actors[3].affected_by = 0;
    actors[0].extra = BLINDFOLDED;
    assert(!can_see(&actors[0], &actors[3]));
    actors[0].extra = 0;
    room.added_flags = ROOM2_DARKNESS;
    assert(can_see(&actors[0], &actors[3]));
    assert(!can_see(&actors[1], &actors[3]));
    room.added_flags = 0;

    /* Exercise the real main pulse, not a model of the recovery formulas. */
    actors[3].class = CLASS_NINJA;
    actors[3].vampaff = 0;
    actors[0].hit = actors[0].max_hit;
    actors[0].mana = actors[0].max_mana;
    pcs[0].wolf = 200;
    pcs[0].ninja[NINJA_KI] = pcs[1].ninja[NINJA_KI] = 100;
    mobile_update();
    assert(pcs[0].wolf == 0);
    assert(pcs[0].ninja[NINJA_KI] == 101 && pcs[1].ninja[NINJA_KI] == 102);
    actors[0].position = actors[1].position = POS_MEDITATING;
    mobile_update();
    assert(pcs[0].ninja[NINJA_KI] == 104 && pcs[1].ninja[NINJA_KI] == 107);
    actors[0].affected_by = actors[1].affected_by = AFF_KAKUSU;
    mobile_update();
    assert(pcs[0].ninja[NINJA_KI] == 104 && pcs[1].ninja[NINJA_KI] == 107);
    actors[0].affected_by = actors[1].affected_by = 0;
    actors[0].position = actors[1].position = POS_STANDING;
    pcs[0].ninja[NINJA_KI] = 299;
    mobile_update();
    assert(pcs[0].ninja[NINJA_KI] == 300);

    actors[0].hit = 9001; actors[0].max_hit = 10001;
    actors[0].mana = 8001;
    run(0, "harakiri");
    assert(actors[0].hit == 4500 && actors[0].mana == 4000);
    assert(pcs[0].ninja[NINJA_HARAKIRI] == 30);
    run(0, "harakiri"); /* No stacking/repeat sacrifice. */
    assert(actors[0].hit == 4500);
    pcs[0].ninja[NINJA_HARAKIRI] = 0;
    actors[0].hit = actors[0].max_hit = LONG_MAX;
    actors[0].mana = LONG_MAX;
    run(0, "harakiri");
    assert(pcs[0].ninja[NINJA_HARAKIRI] == INT_MAX);
    disband_admin();

    /* A pending Nightstalker intent must not silently settle as normal Ninja,
     * and a pending normal Ninja must not settle as Nightstalker. */
    for (int want_night = 0; want_night < 2; ++want_night) {
        fixture(CLASS_NINJA);
        pcs[1].cland[0] = want_night ? 0 : NINJA_BUILD_NIGHTSTALKER;
        char intent[256];
        snprintf(intent, sizeof intent, "AnathemaClans 3 2\nC 1 %ld 4f72646572\n"
                 "M 1 %ld 3 Leader 0 0\nM 1 %ld 1 Member 0 %d\nEnd\n",
                 (long)CLASS_NINJA, (long)CLASS_NINJA, (long)CLASS_NINJA,
                 want_night ? 33 : 20);
        write_text(intent);
        assert(clan_load_registry());
        clan_sync_player(&actors[1]);
        assert(pcs[1].player_load_failed);
        assert(pcs[1].cland[0] == (want_night ? 0 : NINJA_BUILD_NIGHTSTALKER));
        disband_admin();
    }
}

static void gargoyle(void)
{
    fixture(CLASS_VAMPIRE);
    actors[0].class = CLASS_NONE;
    for (int i = 0; i < 5; ++i) actors[0].stats[i] = 4;
    run(0, "autoclass vampire gargoyle");
    assert(IS_GARGOYLE(&actors[0]) && actors[0].vampgen == 9);
    do_clandisc(&actors[0], "fortitude");
    assert(pcs[0].cland[0] == VAMPIRE_BUILD_GARGOYLE &&
           pcs[0].cland[1] == -1 && pcs[0].cland[2] == -1);
    assert(!IS_GARGOYLE(NULL) && !IS_GARGOYLE(&actors[1]));
    assert(pcs[0].cland[1] == -1 && pcs[0].cland[2] == -1);
    assert(pcs[0].powers[DISC_THAUMATURGY] == -1);
    actors[0].primal = 1000;
    run(0, "discipline");
    assert(strstr(outputs[0], "Visceratika") && !strstr(outputs[0], "Thaumaturgy"));
    run(0, "discipline thaumaturgy");
    assert(actors[0].primal == 1000);
    for (long rank = 1; rank <= 5; ++rank) {
        long before = actors[0].primal;
        run(0, "discipline visceratika");
        assert(gargoyle_visceratika(&actors[0]) == rank);
        assert(actors[0].primal == before - rank * 15);
        assert(get_disc(&actors[0], DISC_THAUMATURGY) == 0);
        assert(get_truedisc(&actors[0], DISC_THAUMATURGY) == 0);
        actors[0].hit = actors[0].max_hit;
        assert(gargoyle_damage(&actors[0], 1000) == 1000);
        SET_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
        hurt_person(&actors[1], &actors[0], 1000);
        assert(actors[0].hit == 10000 - 100 * (10 - rank));
        REMOVE_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
    }
    long primal = actors[0].primal;
    run(0, "discipline visceratika");
    assert(gargoyle_visceratika(&actors[0]) == 5 && actors[0].primal == primal);
    run(0, "teachdisc");
    assert(strstr(outputs[0], "visceratika") && !strstr(outputs[0], "thaumaturgy"));

    const long amounts[] = {LONG_MIN, -1, 0, 1, 9, 10, 11, 99, 100, 1000, LONG_MAX};
    SET_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
    for (long rank = -2; rank <= 8; ++rank) {
        pcs[0].powers[DISC_THAUMATURGY] = rank;
        for (size_t i = 0; i < sizeof amounts/sizeof amounts[0]; ++i) {
            long expected = amounts[i];
            if (rank >= 1 && rank <= 5 && expected > 0)
                expected = (long)((__int128)expected * (10 - rank) / 10);
            assert(gargoyle_damage(&actors[0], amounts[i]) == expected);
            assert(gargoyle_damage(&actors[1], amounts[i]) == amounts[i]);
        }
    }
    pcs[0].powers[DISC_THAUMATURGY] = 5;
    actors[0].hit = actors[1].hit = 10000;
    hurt_person(&actors[0], &actors[0], 1000);
    hurt_person(&actors[0], &actors[1], 1000);
    assert(actors[0].hit == 9000 && actors[1].hit == 9000);
    hurt_person(&actors[1], &actors[0], -100);
    assert(actors[0].hit == 9100);
    pcs[0].player_load_failed = TRUE;
    assert(!IS_GARGOYLE(&actors[0]) && gargoyle_damage(&actors[0], 1000) == 1000);
    pcs[0].player_load_failed = FALSE;
    REMOVE_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);

    for (long rank = 0; rank <= 5; ++rank) {
        pcs[0].powers[DISC_THAUMATURGY] = rank;
        actors[0].affected_by = 0; actors[0].wait = 0;
        run(0, "camouflage");
        assert(!!IS_AFFECTED(&actors[0], AFF_HIDE) == (rank > 0));
        assert(actors[0].wait == (rank > 0 ? 12 : 0));
    }
    actors[0].affected_by = 0;
    run(0, "camouflage"); /* Still recovering: no nested-command bypass. */
    assert(!IS_AFFECTED(&actors[0], AFF_HIDE));
    actors[0].wait = 0;
    actors[0].in_room = NULL;
    run(0, "camouflage");
    assert(!IS_AFFECTED(&actors[0], AFF_HIDE));
    actors[0].in_room = &room;

    SET_BIT(pcs[0].flag, FLAG_PK); SET_BIT(pcs[1].flag, FLAG_PK);
    SET_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
    actors[0].hit = actors[0].max_hit;
    damage(&actors[1], &actors[0], 1000, TYPE_BLUNT, "stone test");
    assert(actors[0].hit == 9500); /* Exactly once, not at both damage boundaries. */
    stop_fighting(&actors[0], TRUE);
    for (long rank = 3; rank <= 5; ++rank) {
        pcs[0].powers[DISC_THAUMATURGY] = rank;
        actors[0].hit = actors[0].max_hit;
        actors[0].position = actors[1].position = POS_STANDING;
        actors[1].wait = 0;
        run(1, "punch Leader");
        if (rank >= 4) {
            assert(actors[0].hit == actors[0].max_hit);
            assert(actors[0].position != POS_STUNNED);
        } else assert(actors[0].position == POS_STUNNED);
        stop_fighting(&actors[0], TRUE);
    }
    actors[0].position = actors[1].position = POS_STANDING;
    REMOVE_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
    REMOVE_BIT(pcs[0].flag, FLAG_PK); REMOVE_BIT(pcs[1].flag, FLAG_PK);
    free_string(actors[0].clan); actors[0].clan = str_dup("Caitiff");
    pcs[0].powers[DISC_THAUMATURGY] = -1; actors[0].primal = 14;
    run(0, "discipline visceratika");
    assert(pcs[0].powers[DISC_THAUMATURGY] == -1 && actors[0].primal == 14);
    actors[0].primal = 15;
    run(0, "discipline visceratika");
    assert(gargoyle_visceratika(&actors[0]) == 1 && actors[0].primal == 0);
    free_string(actors[0].clan); actors[0].clan = str_dup("");
    pcs[0].powers[DISC_THAUMATURGY] = 5;

    /* Statue mode is defensive, not another attack/escape form. */
    actors[0].wait = 0;
    run(0, "stoneform on");
    assert(gargoyle_statue(&actors[0]) && actors[0].wait == 12);
    long stone_primal = actors[0].primal;
    actors[0].wait = 0;
    run(0, "discipline fortitude");
    assert(strstr(outputs[0], "stone statue") && actors[0].primal == stone_primal);
    ROOM_INDEX_DATA *stone_room = actors[0].in_room;
    run(0, "north");
    move_char(&actors[0], 0); /* Also block following/internal walking. */
    assert(actors[0].in_room == stone_room);
    long stone_hp = actors[1].hit;
    multi_hit(&actors[0], &actors[1]);
    one_hit(&actors[0], &actors[1], TYPE_UNARMED, "stone attack");
    assert(actors[1].hit == stone_hp);
    assert(gargoyle_saved_polyaff(&actors[0]) == POLY_GARGOYLE_STATUE);
    run(0, "stoneform off");
    assert(!gargoyle_statue(&actors[0]) && actors[0].polyaff == 0);
    actors[0].wait = 0;
    SET_BIT(actors[0].polyaff, POLY_BAT);
    run(0, "stoneform on");
    assert(!gargoyle_statue(&actors[0]) && actors[0].polyaff == POLY_BAT);
    actors[0].polyaff = POLY_GARGOYLE_STATUE;
    pcs[0].powers[DISC_THAUMATURGY] = 0;
    assert(gargoyle_saved_polyaff(&actors[0]) == 0);
    assert(actors[0].polyaff == POLY_GARGOYLE_STATUE); /* Save projection is pure. */
    combat_status_update();
    assert(actors[0].polyaff == 0);
    pcs[0].powers[DISC_THAUMATURGY] = 5;

    /* One physical rank slot, but no cross-build Thaumaturgy/Visceratika lesson. */
    pcs[2].cland[0] = VAMPIRE_BUILD_GARGOYLE;
    pcs[2].powers[DISC_THAUMATURGY] = pcs[1].powers[DISC_THAUMATURGY] = 0;
    run(0, "teachdisc Member visceratika");
    assert(pcs[1].teach_until == 0);
    pcs[1].powers[DISC_THAUMATURGY] = 3;
    run(1, "teachdisc Other thaumaturgy");
    assert(pcs[2].teach_until == 0);
    run(0, "teachdisc Other visceratika");
    assert(pcs[2].teach_until > current_time);
    pcs[0].cland[0] = -1; /* Revalidate a teacher who changed builds after offering. */
    run(2, "teachdisc accept Leader visceratika");
    assert(pcs[2].powers[DISC_THAUMATURGY] == 0);
    pcs[0].cland[0] = VAMPIRE_BUILD_GARGOYLE;
    run(2, "teachdisc accept Leader visceratika");
    assert(pcs[2].powers[DISC_THAUMATURGY] == -1);
    pcs[0].powers[DISC_THAUMATURGY] = 5;
    const char *blood[] = {"tasteblood Member", "bloodrage", "potency",
                          "theftvitae Member", "cauldronblood Member"};
    for (size_t i = 0; i < sizeof blood/sizeof blood[0]; ++i) {
        long hp = actors[1].hit, mana = actors[0].mana;
        actors[0].wait = 0;
        run(0, blood[i]);
        assert(strstr(outputs[0], "not trained high enough"));
        assert(actors[1].hit == hp && actors[0].mana == mana);
    }

    run(4, "clanadmin create Leader StoneOrder");
    actors[2].class = CLASS_NONE;
    memset(pcs[2].powers, 0, sizeof pcs[2].powers);
    for (int i = 0; i < 5; ++i) actors[2].stats[i] = 4;
    run(0, "clan recruit Other");
    assert(actors[2].class == CLASS_NONE && strstr(outputs[2], "vampire gargoyle"));
    run(2, "clan accept Leader");
    assert(IS_GARGOYLE(&actors[2]) && actors[2].vampgen == 9);
    assert(pcs[2].powers[DISC_THAUMATURGY] == -1 && clan_member_role(&actors[2]) == 1);
    run(0, "clan kick Other");
    assert(IS_GARGOYLE(&actors[2]) && !strcmp(clan_affiliation(&actors[2]), "Exiled"));
    run(0, "clan invite Other"); run(2, "clan accept Leader");
    run(2, "clan leave");
    assert(IS_GARGOYLE(&actors[2]) && !strcmp(clan_affiliation(&actors[2]), "Loner"));
    assert(save_char_obj_checked(&actors[2]));
    DESCRIPTOR_DATA reload = {0};
    assert(load_char_obj(&reload, "Other") && IS_GARGOYLE(reload.character));
    assert(reload.character->pcdata->powers[DISC_THAUMATURGY] == -1);
    free_char(reload.character);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    memset(&reload, 0, sizeof reload);
    assert(load_char_short(&reload, "Other") && IS_GARGOYLE(reload.character));
    free_char(reload.character);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    actors[2].polyaff = POLY_GARGOYLE_STATUE;
    for (long rank = 0; rank <= 1; ++rank) {
        pcs[2].powers[DISC_THAUMATURGY] = rank;
        assert(save_char_obj_checked(&actors[2]));
        assert(actors[2].polyaff == POLY_GARGOYLE_STATUE);
        memset(&reload, 0, sizeof reload);
        assert(load_char_obj(&reload, "Other"));
        assert(gargoyle_statue(reload.character) == (rank == 1));
        assert(reload.character->polyaff == (rank ? POLY_GARGOYLE_STATUE : 0));
        free_char(reload.character);
        if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
        memset(&reload, 0, sizeof reload);
        assert(load_char_short(&reload, "Other"));
        assert(gargoyle_statue(reload.character) == (rank == 1));
        assert(reload.character->polyaff == (rank ? POLY_GARGOYLE_STATUE : 0));
        free_char(reload.character);
        if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    }
    pcs[2].conding = pcs[2].love = pcs[2].covenant =
        pcs[2].cov_time = pcs[2].leader = pcs[2].vleader = str_dup("");
    actors[4].level = LEV_IMP;
    run(4, "clearclass Other");
    assert(actors[2].class == CLASS_NONE && !IS_GARGOYLE(&actors[2]));
    assert(actors[2].polyaff == 0);
    assert(pcs[2].cland[0] == 0 && pcs[2].powers[DISC_THAUMATURGY] == 0);
    actors[4].level = LEV_IMM;
    disband_admin();

    for (int want_gargoyle = 0; want_gargoyle < 2; ++want_gargoyle) {
        fixture(CLASS_VAMPIRE);
        pcs[1].cland[0] = want_gargoyle ? DISC_DOMINATE : VAMPIRE_BUILD_GARGOYLE;
        char intent[256];
        snprintf(intent, sizeof intent, "AnathemaClans 3 2\nC 1 %ld 4f72646572\n"
                 "M 1 %ld 3 Leader 0 0\nM 1 %ld 1 Member 0 %d\nEnd\n",
                 (long)CLASS_VAMPIRE, (long)CLASS_VAMPIRE, (long)CLASS_VAMPIRE,
                 want_gargoyle ? 34 : 1);
        write_text(intent);
        assert(clan_load_registry());
        clan_sync_player(&actors[1]);
        assert(pcs[1].player_load_failed);
        disband_admin();
    }
}

static void gargoyle_lesson_case(long rank, int discipline)
{
    fixture(CLASS_VAMPIRE);
    pcs[0].cland[0] = pcs[1].cland[0] = VAMPIRE_BUILD_GARGOYLE;
    pcs[0].powers[DISC_THAUMATURGY] = rank;
    long disc = discipline ? DISC_AUSPEX : DISC_THAUMATURGY;
    const char *name = discipline ? "auspex" : "visceratika";
    pcs[0].powers[disc] = rank;
    pcs[1].powers[disc] = 0;
    char offer[128], accept[128];
    snprintf(offer, sizeof offer, "teachdisc Member %s", name);
    snprintf(accept, sizeof accept, "teachdisc accept Leader %s", name);
    long teacher_primal = actors[0].primal, student_primal = actors[1].primal;
    run(0, offer);
    assert(pcs[1].teach_until > current_time);
    time_t until = pcs[1].teach_until;
    run(0, "stoneform on");
    assert(gargoyle_statue(&actors[0]));
    run(1, accept);
    assert(pcs[1].powers[disc] == 0);
    assert(pcs[1].teach_until == until);
    assert(strstr(outputs[1], "Both vampires must be here"));
    run(0, "stoneform off");
    if (discipline) {
        /* Revalidate the student too, including direct handler callers. */
        pcs[1].powers[DISC_THAUMATURGY] = 1;
        run(1, "stoneform on");
        assert(gargoyle_statue(&actors[1]));
        char direct[128];
        snprintf(direct, sizeof direct, "accept Leader %s", name);
        do_teachdisc(&actors[1], direct);
        assert(pcs[1].powers[disc] == 0 && pcs[1].teach_until == until);
        run(1, "stoneform off");
    }
    run(1, accept);
    assert(pcs[1].powers[disc] == -1);
    assert(pcs[1].teach_until == 0);
    assert(actors[0].primal == teacher_primal && actors[1].primal == student_primal);
}

static void gargoyle_lesson_lifecycle(void)
{
    for (long rank = 3; rank <= 5; ++rank)
        for (int discipline = 0; discipline < 2; ++discipline)
            gargoyle_lesson_case(rank, discipline);
}

static void gargoyle_power_menu(void)
{
    for (long rank = -1; rank <= 6; ++rank) {
        fixture(CLASS_VAMPIRE);
        pcs[0].cland[0] = VAMPIRE_BUILD_GARGOYLE;
        pcs[0].powers[DISC_THAUMATURGY] = rank;
        long before_hit = actors[0].hit, before_mana = actors[0].mana;
        long before_primal = actors[0].primal, before_quest = pcs[0].quest;
        run(0, "powers");
        bool learned = rank >= 1 && rank <= 5;
        assert(!!strstr(outputs[0], "Visceratika:") == learned);
        assert(!strstr(outputs[0], "Thaumaturgy:"));
        if (learned) {
            assert(strstr(outputs[0], "Camouflage") && strstr(outputs[0], "Stoneform"));
            assert(!strstr(outputs[0], "You have no disciplines"));
            assert(!strstr(outputs[0], "All your discipline powers are automatic"));
            assert(!!strstr(outputs[0], "Punch resistance") == (rank >= 4));
            assert(!!strstr(outputs[0], "25% reflected HP damage") == (rank == 5));
            SET_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
            run(0, "powers");
            assert(strstr(outputs[0], "Visceratika:") &&
                   !strstr(outputs[0], "cannot act while a stone statue"));
        }
        assert(actors[0].hit == before_hit && actors[0].mana == before_mana &&
               actors[0].primal == before_primal && pcs[0].quest == before_quest);
    }
    fixture(CLASS_VAMPIRE);
    pcs[0].powers[DISC_THAUMATURGY] = 5;
    run(0, "powers");
    assert(strstr(outputs[0], "Thaumaturgy: Tasteblood") &&
           !strstr(outputs[0], "Visceratika:"));
}

static void reflection_fixture(long rank, bool stone)
{
    fixture(CLASS_VAMPIRE);
    pcs[0].cland[0] = VAMPIRE_BUILD_GARGOYLE;
    pcs[0].powers[DISC_THAUMATURGY] = rank;
    if (stone) SET_BIT(actors[0].polyaff, POLY_GARGOYLE_STATUE);
    actors[0].fighting = &actors[1];
    actors[1].fighting = &actors[0];
    actors[0].position = actors[1].position = POS_FIGHTING;
}

static void gargoyle_reflection(void)
{
    const long amounts[] = {0, 1, 3, 7, 8, 99, 1000, 1001};
    for (long rank = -1; rank <= 6; ++rank)
        for (int stone = 0; stone <= 1; ++stone)
            for (int wrapped = 0; wrapped <= 1; ++wrapped)
                for (size_t n = 0; n < sizeof amounts / sizeof amounts[0]; ++n) {
                    reflection_fixture(rank, stone);
                    long taken = gargoyle_damage(&actors[0], amounts[n]);
                    long reflected = rank == 5 && stone ? taken / 4 : 0;
                    if (wrapped) damage(&actors[1], &actors[0], amounts[n],
                                        TYPE_UNARMED, "test reflection");
                    else hurt_person(&actors[1], &actors[0], amounts[n]);
                    assert(actors[0].hit == 10000 - taken);
                    assert(actors[1].hit == 10000); /* Never inside the attack stack. */
                    gargoyle_reflection_pulse();
                    assert(actors[1].hit == 10000 - reflected);
                    gargoyle_reflection_pulse();
                    assert(actors[1].hit == 10000 - reflected);
                }

    reflection_fixture(5, TRUE);
    hurt_person(&actors[0], &actors[0], 1000);
    hurt_person(&actors[1], &actors[0], -100);
    gargoyle_reflection_pulse();
    assert(actors[0].hit == 9100 && actors[1].hit == 10000);

    reflection_fixture(5, TRUE);
    hurt_person(&actors[1], &actors[0], 1000);
    hurt_person(&actors[1], &actors[0], 1000);
    gargoyle_reflection_pulse();
    assert(actors[0].hit == 9000 && actors[1].hit == 9750);

    reflection_fixture(5, TRUE);
    pcs[1].cland[0] = VAMPIRE_BUILD_GARGOYLE;
    pcs[1].powers[DISC_THAUMATURGY] = 5;
    actors[1].polyaff = POLY_GARGOYLE_STATUE;
    hurt_person(&actors[1], &actors[0], 1000);
    gargoyle_reflection_pulse();
    assert(actors[0].hit == 9500 && actors[1].hit == 9938);
    gargoyle_reflection_pulse(); /* Two statues never reflect reflections. */
    assert(actors[0].hit == 9500 && actors[1].hit == 9938);

    reflection_fixture(5, TRUE);
    actors[0].max_hit = actors[1].hit = actors[1].max_hit = LONG_MAX;
    for (int n = 0; n < 20; ++n) {
        actors[0].hit = LONG_MAX;
        hurt_person(&actors[1], &actors[0], LONG_MAX);
    }
    gargoyle_reflection_pulse(); /* Saturating accumulation, never signed overflow. */
    assert(actors[1].hit == 0);

    for (int invalid = 0; invalid < 7; ++invalid) {
        reflection_fixture(5, TRUE);
        hurt_person(&actors[1], &actors[0], 1000);
        switch (invalid) {
        case 0: actors[0].polyaff = 0; break;
        case 1: pcs[0].powers[DISC_THAUMATURGY] = 4; break;
        case 2: actors[1].in_room = &elsewhere; break;
        case 3: char_list = &actors[1]; break;
        case 4: SET_BIT(room.room_flags, ROOM_SAFE); break;
        case 5: actors[1].hit = 0; actors[1].position = POS_DEAD; break;
        case 6: actors[0].hit = 0; actors[0].position = POS_STUNNED; break;
        }
        gargoyle_reflection_pulse();
        assert(actors[1].hit == (invalid == 5 ? 0 : 10000));
    }

    reflection_fixture(5, TRUE);
    hurt_person(&actors[1], &actors[0], 1000);
    unsigned long old_id = actors[1].gargoyle_reflection_id;
    CHAR_DATA replacement = actors[1];
    clear_char(&actors[1]); /* Pooled address reuse must invalidate old events. */
    assert(actors[1].gargoyle_reflection_id == 0);
    replacement.gargoyle_reflection_id = actors[1].gargoyle_reflection_id;
    actors[1] = replacement;
    hurt_person(&actors[1], &actors[0], 1000);
    assert(actors[1].gargoyle_reflection_id != old_id);
    gargoyle_reflection_pulse();
    assert(actors[1].hit == 9875);

    /* A reflected killing blow extracts a real pooled NPC. Another queued
     * reflection at the same freed pointer must be discarded, not dereferenced. */
    reflection_fixture(5, TRUE);
    static MOB_INDEX_DATA index;
    static OBJ_INDEX_DATA corpse;
    memset(&index, 0, sizeof index);
    memset(&corpse, 0, sizeof corpse);
    index.vnum = 30991;
    /* Real mobile prototypes borrow strings from the boot string arena. */
    const char *prototype_strings[] = {"reflectiondummy", "a reflection dummy",
                                      "A reflection dummy stands here.", ""};
    char **prototype_fields[] = {&index.player_name, &index.short_descr,
                                &index.long_descr, &index.description};
    for (size_t n = 0; n < 4; ++n) {
        strcpy(top_string, prototype_strings[n]);
        *prototype_fields[n] = top_string;
        top_string += strlen(top_string) + 1;
    }
    index.act = ACT_IS_NPC; index.level = 1;
    corpse.vnum = OBJ_VNUM_CORPSE_NPC;
    corpse.name = "corpse";
    corpse.short_descr = "the corpse of %s";
    corpse.description = "The corpse of %s lies here.";
    corpse.item_type = ITEM_CORPSE_NPC;
    OBJ_INDEX_DATA *old_corpse = obj_index_hash[corpse.vnum % MAX_KEY_HASH];
    corpse.next = old_corpse;
    obj_index_hash[corpse.vnum % MAX_KEY_HASH] = &corpse;
    ROOM_INDEX_DATA *old_disconnect =
        room_index_hash[ROOM_VNUM_DISCONNECTION % MAX_KEY_HASH];
    elsewhere.vnum = ROOM_VNUM_DISCONNECTION;
    elsewhere.next = old_disconnect;
    room_index_hash[ROOM_VNUM_DISCONNECTION % MAX_KEY_HASH] = &elsewhere;
    exp_info.exp = 100; exp_info.bit = 0;
    CHAR_DATA *npc = create_mobile(&index);
    char_to_room(npc, &room);
    npc->hit = npc->max_hit = 10;
    npc->position = POS_FIGHTING;
    pcs[2].cland[0] = VAMPIRE_BUILD_GARGOYLE;
    pcs[2].powers[DISC_THAUMATURGY] = 5;
    actors[2].polyaff = POLY_GARGOYLE_STATUE;
    hurt_person(npc, &actors[0], 1000);
    hurt_person(npc, &actors[2], 1000);
    gargoyle_reflection_pulse();
    for (CHAR_DATA *live = char_list; live != NULL; live = live->next)
        assert(live != npc);
    assert(index.count == 0);
    while (room.contents != NULL) extract_obj(room.contents);
    obj_index_hash[corpse.vnum % MAX_KEY_HASH] = old_corpse;
    room_index_hash[ROOM_VNUM_DISCONNECTION % MAX_KEY_HASH] = old_disconnect;
    puts("Gargoyle reflection: 256 rank/form/path/rounding pairs and lifecycle guards passed");
}

int main(void)
{
    char root[] = "/tmp/anathema-clans-XXXXXX", original[4096];
    assert(getcwd(original, sizeof(original)) != NULL && mkdtemp(root) != NULL);
    assert(chdir(root) == 0);
    assert(mkdir("area", 0700) == 0 && mkdir("player", 0700) == 0 &&
           mkdir("backup", 0700) == 0);
    assert(chdir("area") == 0);
    string_space = malloc(1024 * 1024);
    assert(string_space != NULL);
    top_string = string_space; *top_string = '\0';
    current_time = 2000000000;
    init_mm();
    assert(clan_load_registry()); /* No write merely to read an unconfigured installation. */
    assert(access("../clans", F_OK) != 0);
    for (size_t i = 0; i < sizeof(classes)/sizeof(classes[0]); ++i) class_cycle(classes[i]);
    faults(CLASS_MAGE);
    faults(CLASS_DROW);
    corrupt_files();
    limits_and_admin();
    native_recruitment();
    enrollment_recovery();
    nightstalker();
    gargoyle();
    gargoyle_power_menu();
    gargoyle_lesson_lifecycle();
    gargoyle_reflection();
    clan_registry_close();
    char_list = NULL; descriptor_list = NULL;
    for (int i = 0; i < 5; ++i) free_string(actors[i].clan);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    assert(unlink("../clans/Registry") == 0 && unlink("../clans/Lock") == 0);
    unlink("../player/Other"); unlink("../backup/Other");
    unlink("../player/Member"); unlink("../backup/Member");
    unlink("../player/Leader"); unlink("../backup/Leader");
    assert(chdir("..") == 0);
    assert(rmdir("area") == 0 && rmdir("player") == 0 &&
           rmdir("backup") == 0 && rmdir("clans") == 0);
    assert(chdir(original) == 0 && rmdir(root) == 0);
    puts("clan governance: ten classes, native aliases, named consent, rank protection, "
         "offline save reconciliation, atomic transfer faults, locking and parser limits passed");
    return 0;
}
