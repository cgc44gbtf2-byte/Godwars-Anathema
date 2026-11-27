/* Native Metis Totem Gift, its damage boundary and transient cleanup. */
#define main aura_fixture_main
#include "test_hybrid_aura.c"
#undef main

static OBJ_INDEX_DATA totem_index;
static OBJ_INDEX_DATA *old_obj_hash;
static bool prototype_installed;

static void install_totem_prototype(void)
{
    if (prototype_installed)
        return;
    memset(&totem_index, 0, sizeof(totem_index));
    totem_index.vnum = OBJ_VNUM_LIGHT_BALL;
    totem_index.item_type = ITEM_LIGHT;
    totem_index.name = "light ball";
    totem_index.short_descr = "a light ball";
    totem_index.description = "A light ball is here.";
    old_obj_hash = obj_index_hash[OBJ_VNUM_LIGHT_BALL % MAX_KEY_HASH];
    totem_index.next = old_obj_hash;
    obj_index_hash[OBJ_VNUM_LIGHT_BALL % MAX_KEY_HASH] = &totem_index;
    prototype_installed = TRUE;
}

static void clear_objects(void)
{
    while (object_list != NULL)
        extract_obj(object_list);
}

static void restore_totem_prototype(void)
{
    clear_objects();
    obj_index_hash[OBJ_VNUM_LIGHT_BALL % MAX_KEY_HASH] = old_obj_hash;
    prototype_installed = FALSE;
}

static void wolf_fixture(void)
{
    clear_objects();
    fixture(CLASS_WEREWOLF, 0, 0, -1, 0);
    pcs[0].breed[BREED_METIS] = 5;
    pcs[0].tribes[TRIBE_BLACK_FURIES] = 1;
    pcs[0].tgift[2] = TRIBE_BLACK_FURIES;
    actors[0].mana = 5000;
}

static void cast(void)
{
    do_totemgift(&actors[0], "");
}

static OBJ_DATA *owner_totem(void)
{
    if (actors[0].metis_totem_id == 0)
        return NULL;
    for (OBJ_DATA *obj = object_list; obj != NULL; obj = obj->next)
        if (obj->metis_totem_id == actors[0].metis_totem_id)
            return obj;
    return NULL;
}

static void test_activation_and_absorption(void)
{
    OBJ_DATA *totem;

    wolf_fixture();
    install_totem_prototype();
    cast();
    totem = owner_totem();
    assert(totem != NULL && totem->in_room == &room);
    assert(totem->item_type == ITEM_TRASH && totem->wear_flags == 0);
    assert(strstr(totem->short_descr, "Pegasus") != NULL);
    assert(actors[0].mana == 0);
    assert(actors[0].metis_totem_timer == 10);
    assert(actors[0].metis_totem_absorb == 10000);
    assert(actors[0].wait == 12);

    cast();
    assert(actors[0].mana == 0 && actors[0].metis_totem_absorb == 10000);

    hurt_person(&actors[1], &actors[0], 7000);
    assert(actors[0].hit == 50000);
    assert(actors[0].metis_totem_absorb == 3000);
    assert(owner_totem() != NULL);

    hurt_person(&actors[1], &actors[0], 5000);
    assert(actors[0].hit == 48000);
    assert(actors[0].metis_totem_id == 0);
    assert(actors[0].metis_totem_timer == 0);
    assert(actors[0].metis_totem_absorb == 0);
    assert(owner_totem() == NULL);

    hurt_person(&actors[1], &actors[2], 1000);
    assert(actors[2].hit == 49000);
    assert(actors[0].mana == 0);
}

static void test_self_damage_and_expiry(void)
{
    wolf_fixture();
    install_totem_prototype();
    cast();
    hurt_person(&actors[0], &actors[0], 1000);
    assert(actors[0].hit == 49000);
    assert(actors[0].metis_totem_absorb == 10000);

    for (int pulse = 0; pulse < 9; ++pulse)
        combat_status_update();
    assert(actors[0].metis_totem_timer == 1);
    assert(owner_totem() != NULL);
    combat_status_update();
    assert(actors[0].metis_totem_id == 0);
    assert(owner_totem() == NULL);
    restore_totem_prototype();
}

static void test_full_absorption(void)
{
    wolf_fixture();
    install_totem_prototype();
    cast();
    hurt_person(&actors[1], &actors[0], 10000);
    assert(actors[0].hit == 50000);
    assert(actors[0].metis_totem_id == 0);
    assert(owner_totem() == NULL);
    restore_totem_prototype();
}

static void test_movement_and_training_cleanup(void)
{
    wolf_fixture();
    install_totem_prototype();
    cast();
    assert(owner_totem() != NULL);
    char_from_room(&actors[0]);
    assert(actors[0].metis_totem_id == 0);
    assert(owner_totem() == NULL);

    wolf_fixture();
    install_totem_prototype();
    cast();
    pcs[0].breed[BREED_METIS] = 4;
    combat_status_update();
    assert(actors[0].metis_totem_id == 0);
    assert(owner_totem() == NULL);
    restore_totem_prototype();
}

static void assert_rejected_without_cost(void)
{
    assert(actors[0].mana == 5000);
    assert(actors[0].wait == 0);
    assert(actors[0].metis_totem_id == 0);
    assert(object_list == NULL);
}

static void test_rejected_casts(void)
{
    wolf_fixture();
    install_totem_prototype();
    pcs[0].breed[BREED_METIS] = 4;
    cast();
    assert_rejected_without_cost();

    wolf_fixture();
    actors[0].class = CLASS_MAGE;
    cast();
    assert_rejected_without_cost();

    wolf_fixture();
    pcs[0].tgift[2] = -1;
    cast();
    assert_rejected_without_cost();

    wolf_fixture();
    actors[0].mana = 4999;
    cast();
    assert(actors[0].mana == 4999 && actors[0].wait == 0);
    assert(object_list == NULL);

    wolf_fixture();
    actors[0].position = POS_RESTING;
    cast();
    assert_rejected_without_cost();

    wolf_fixture();
    do_totemgift(&actors[0], "target");
    assert_rejected_without_cost();

    wolf_fixture();
    obj_index_hash[OBJ_VNUM_LIGHT_BALL % MAX_KEY_HASH] = old_obj_hash;
    cast();
    assert_rejected_without_cost();
    restore_totem_prototype();
}

int main(void)
{
    install_totem_prototype();
    test_activation_and_absorption();
    restore_totem_prototype();
    install_totem_prototype();
    test_self_damage_and_expiry();
    test_full_absorption();
    test_movement_and_training_cleanup();
    test_rejected_casts();
    restore_totem_prototype();
    puts("Metis Totem Gift runtime tests passed");
    return 0;
}
