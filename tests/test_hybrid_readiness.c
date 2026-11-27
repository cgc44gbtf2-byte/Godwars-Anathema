/* Real-engine prerequisites. Never grant conversion, skills or currency. */
#include <sys/types.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Genesis/src/merc.h"
#include "../Genesis/src/classes/hybrid/hybrid.h"
#include "../Genesis/src/classes/hybrid/readiness.h"

extern void init_mm(void);

static CHAR_DATA actor;
static PC_DATA player;
static DESCRIPTOR_DATA descriptor;
static char output[32768];
static const long parents[] = {CLASS_DRAGON, CLASS_DROW, CLASS_ELEM,
    CLASS_HIGHLANDER, CLASS_MAGE, CLASS_MONK, CLASS_NINJA, CLASS_SORCERER,
    CLASS_VAMPIRE, CLASS_WEREWOLF};

static void maxed(long parent, int immortal)
{
    memset(&actor, 0, sizeof(actor));
    memset(&player, 0, sizeof(player));
    actor.name = "tester";
    actor.class = parent;
    actor.pcdata = &player;
    actor.level = immortal ? LEV_ST : LEV_AWA;
    actor.max_hit = actor.max_mana = 50000;
    actor.hit = 1;
    actor.mana = 0;
    player.quest = 50000;
    for (int i = STAT_STR; i <= STAT_WIS; i++) actor.stats[i] = stat_max(&actor, i);
    /* Native cap oracles: early normalization at LONG_MAX consumes no RNG,
     * needs no equipment/world and does not send action text. */
    for (long i = STYLE_UNARMED; i <= STYLE_TWOWEAPON; i++) {
        actor.style[0] = i;
        actor.style[i] = LONG_MAX;
        improve_style(&actor);
    }
    for (long i = TYPE_UNARMED; i <= TYPE_BLUNT; i++) {
        actor.weapon[i] = LONG_MAX;
        improve_weapon(&actor, i);
    }
    for (long i = STANCE_VIPER; i <= STANCE_SWALLOW; i++) {
        actor.stance[0] = i;
        actor.stance[i] = LONG_MAX;
        improve_stance(&actor);
    }
}

static void readonly(HYBRID_READINESS *report)
{
    unsigned char before[sizeof(actor)], pc_before[sizeof(player)];
    memcpy(before, &actor, sizeof(actor));
    memcpy(pc_before, &player, sizeof(player));
    init_mm();
    long next = number_range(1, 1000000);
    init_mm();
    assert(hybrid_readiness_read(&actor, report));
    assert(number_range(1, 1000000) == next);
    assert(!memcmp(&before, &actor, sizeof(actor)));
    assert(!memcmp(&pc_before, &player, sizeof(player)));
}

static long *storage(const HYBRID_READINESS_ITEM *item)
{
    switch (item->kind) {
    case HYBRID_READY_HP: return &actor.max_hit;
    case HYBRID_READY_MANA: return &actor.max_mana;
    case HYBRID_READY_STAT: return &actor.stats[item->index];
    case HYBRID_READY_STANCE: return &actor.stance[item->index];
    case HYBRID_READY_WEAPON: return &actor.weapon[item->index];
    case HYBRID_READY_STYLE: return &actor.style[item->index];
    }
    assert(0);
    return NULL;
}

static size_t matrices(void)
{
    size_t cases = 0;
    for (size_t p = 0; p < sizeof(parents)/sizeof(parents[0]); p++)
        for (int immortal = 0; immortal < 2; immortal++) {
            HYBRID_READINESS original, result;
            maxed(parents[p], immortal);
            readonly(&original);
            assert(original.count == 27 && original.review == 0);
            assert(original.met == original.applicable);
            int natural = parents[p] == CLASS_DRAGON ||
                parents[p] == CLASS_VAMPIRE || parents[p] == CLASS_WEREWOLF;
            assert(original.applicable == (size_t)(natural ? 27 : 26));
            assert(!IS_VAMPAFF(&actor, VAM_CLAWS)); /* no retract-to-skip bypass */
            /* Active equipment/style/stance selectors are not mastery slots. */
            actor.style[0] = actor.stance[0] = 0;
            readonly(&result);
            assert(result.met == original.met);
            for (size_t j = 0; j < original.count; j++) {
                HYBRID_READINESS_ITEM item = original.items[j];
                long *value = storage(&item), previous = *value;
                if (item.status == HYBRID_READY_EXEMPT) {
                    assert(item.kind == HYBRID_READY_STYLE && item.index == STYLE_NATURAL);
                    *value = LONG_MIN;
                    readonly(&result);
                    assert(result.met == original.met && !result.review);
                    *value = previous;
                    continue;
                }
                assert(item.current == item.required); /* native cap parity */
                long values[] = {item.required - 1, -1, LONG_MIN,
                                 item.required + 1, LONG_MAX};
                for (size_t k = 0; k < sizeof(values)/sizeof(values[0]); k++) {
                    *value = values[k];
                    readonly(&result);
                    if (k == 0) {
                        assert(result.items[j].status == HYBRID_READY_MISSING);
                        assert(result.met == original.met - 1 && !result.review);
                    } else if ((item.kind == HYBRID_READY_HP ||
                                item.kind == HYBRID_READY_MANA) && k >= 3) {
                        assert(result.items[j].status == HYBRID_READY_MET);
                        assert(result.met == original.met && !result.review);
                    } else {
                        assert(result.items[j].status == HYBRID_READY_REVIEW);
                        assert(result.met == original.met - 1 && result.review == 1);
                    }
                    cases++;
                }
                *value = previous;
            }
        }
    return cases;
}

static void guards(void)
{
    HYBRID_READINESS report;
    maxed(CLASS_MONK, 0);
    assert(!hybrid_readiness_read(&actor, NULL));
    memset(&report, 0xa5, sizeof(report));
    assert(!hybrid_readiness_read(NULL, &report) && !report.count);
    actor.pcdata = NULL;
    assert(!hybrid_readiness_read(&actor, &report) && !report.count);
    actor.pcdata = &player;
    SET_BIT(actor.act, ACT_IS_NPC);
    assert(!hybrid_readiness_read(&actor, &report) && !report.count);
    REMOVE_BIT(actor.act, ACT_IS_NPC);
    long bad[] = {0, -1, LONG_MAX, CLASS_MONK | CLASS_NINJA};
    for (size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
        actor.class = bad[i];
        assert(!hybrid_readiness_read(&actor, &report) && !report.count);
    }
    hybrid_readiness_show(NULL);
}

static void preview(void)
{
    const HYBRID_DEFINITION *definition = hybrid_at(0);
    maxed(definition->parents[0], 0);
    memset(&descriptor, 0, sizeof(descriptor));
    memset(output, 0, sizeof(output));
    actor.desc = &descriptor;
    descriptor.character = &actor;
    descriptor.connected = CON_PLAYING;
    descriptor.outbuf = output;
    descriptor.outsize = sizeof(output);
    char argument[256];
    snprintf(argument, sizeof(argument), "info %s", definition->name);
    unsigned char before[sizeof(actor)], pc_before[sizeof(player)];
    memcpy(before, &actor, sizeof(actor));
    memcpy(pc_before, &player, sizeof(player));
    do_selfbrid(&actor, argument);
    output[descriptor.outtop] = '\0';
    assert(strstr(output, "Numeric prerequisites: 27/27"));
    assert(strstr(output, "not conversion eligibility"));
    assert(strstr(output, "Class/build power mastery is reported separately"));
    assert(strstr(output, "conversion is not enabled"));
    assert(!memcmp(&before, &actor, sizeof(actor)));
    assert(!memcmp(&pc_before, &player, sizeof(player)));
    actor.stats[STAT_STR]--;
    descriptor.outtop = 0;
    hybrid_readiness_show(&actor);
    output[descriptor.outtop] = '\0';
    assert(strstr(output, "Numeric prerequisites: 26/27"));
    assert(strstr(output, "STR:"));
    actor.stats[STAT_STR] = LONG_MAX;
    descriptor.outtop = 0;
    hybrid_readiness_show(&actor);
    output[descriptor.outtop] = '\0';
    assert(strstr(output, "stored value needs review"));
}

int main(void)
{
    size_t cases = matrices();
    guards();
    preview();
    printf("hybrid readiness: %zu boundary replays,20 native class/level "
           "profiles; actual cap parity,read-only RNG/data,preview guards passed\n", cases);
    return 0;
}
