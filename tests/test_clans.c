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
    clan_registry_close();
    char_list = NULL; descriptor_list = NULL;
    for (int i = 0; i < 5; ++i) free_string(actors[i].clan);
    if (fpReserve != NULL) { fclose(fpReserve); fpReserve = NULL; }
    assert(unlink("../clans/Registry") == 0 && unlink("../clans/Lock") == 0);
    unlink("../player/Other"); unlink("../backup/Other");
    unlink("../player/Member"); unlink("../backup/Member");
    assert(chdir("..") == 0);
    assert(rmdir("area") == 0 && rmdir("player") == 0 &&
           rmdir("backup") == 0 && rmdir("clans") == 0);
    assert(chdir(original) == 0 && rmdir(root) == 0);
    puts("clan governance: ten classes, native aliases, named consent, rank protection, "
         "offline save reconciliation, atomic transfer faults, locking and parser limits passed");
    return 0;
}
