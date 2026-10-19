CC ?= cc
LDLIBS ?= -lcrypt
TEST_FLAGS = -std=gnu17 -O1 -g -ffunction-sections -fdata-sections
TEST_LDFLAGS = -Wl,--gc-sections
TEST_CPPFLAGS = -IGenesis/src $(CPPFLAGS)
include Genesis/src/classes/modules.mk
TESTS = tests/test_comm tests/test_db tests/test_gameplay tests/test_buffers tests/test_save tests/test_signals tests/test_classes tests/test_powers tests/test_wall tests/test_mage_powers tests/test_combat_status tests/test_vampire tests/test_class_runtime tests/test_restored_spells tests/test_basic_buffs tests/test_class_identity tests/test_class_handlers tests/test_vampire_modules tests/test_damage_messages tests/test_transformations tests/test_clans tests/test_class_gear tests/test_drow_ammo tests/test_lifecycle_ownership tests/test_autosave_recovery
ENGINE_SOURCES = $(wildcard Genesis/src/*.c) $(addprefix Genesis/src/,$(CLASS_MODULE_SOURCES))
ENGINE_HEADERS = $(wildcard Genesis/src/*.h Genesis/src/classes/*.h Genesis/src/classes/*/*.h)
ENGINE_OBJECTS = $(patsubst Genesis/src/%.c,tests/stun_engine/%.o,$(ENGINE_SOURCES))
LEGACY_CLASS_OBJECTS = $(addprefix tests/stun_engine/,$(notdir $(CLASS_MODULE_SOURCES:.c=.o)))

.PHONY: all run test check-layout sanitize clean
PORT ?= 4000
TESTS += tests/test_restart
TESTS += tests/test_highlander_counter
TESTS += tests/test_stance_style
TESTS += tests/test_admin_gate
TESTS += tests/test_combat_calculations
TESTS += tests/test_combat_damage_death
TESTS += tests/test_telnet
TESTS += tests/test_gmcp
TESTS += tests/test_world_integrity
TESTS += tests/test_combat_attack
TESTS += tests/test_hybrids
TESTS += tests/test_hybrid_state
TESTS += tests/test_hybrid_forms
TESTS += tests/test_hybrid_progression
TESTS += tests/test_hybrid_specialization

all:
	$(MAKE) -C Genesis/src

run: all
	python3 tools/start_local.py --port "$(PORT)"

tests/%: tests/%.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_comm: tests/test_comm.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/telnet_input.c Genesis/src/gmcp.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_gameplay: tests/test_gameplay.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/act_info.c Genesis/src/classes/werewolf/wwauspice.c Genesis/src/classes/mage/mage.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_buffers: tests/test_buffers.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) --param=asan-globals=0 -c Genesis/src/interp.c -o tests/buffers_interp.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/string.c tests/buffers_interp.o $(LDFLAGS) $(TEST_LDFLAGS) -o $@

tests/test_signals: tests/test_signals.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/signals.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_classes tests/test_wall tests/test_mage_powers: tests/%: tests/%.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) -O0 $< $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_powers: tests/test_powers.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/classes/ninja/ninja.c Genesis/src/classes/monk/monk.c Genesis/src/classes/elemental/elemental.c Genesis/src/classes/sorcerer/sorcerer.c Genesis/src/classes/drow/drow.c Genesis/src/combat_status.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_class_runtime: tests/test_class_runtime.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/classes/elemental/elemental.c Genesis/src/classes/monk/monk.c Genesis/src/classes/ninja/ninja.c Genesis/src/classes/dragon/dragon.c Genesis/src/classes/drow/drow.c Genesis/src/classes/highlander/highland.c Genesis/src/classes/werewolf/wwtribe.c Genesis/src/combat_status.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

check-layout:
	python3 tests/test_class_layout.py
	python3 tests/test_combat_layout.py
	python3 tests/test_telnet_layout.py
	python3 tests/test_gmcp_layout.py
	python3 tests/test_mudlet_hud.py
	python3 tests/test_security_commands.py
	python3 tests/test_setup.py
	python3 tests/test_world_layout.py
	python3 tests/test_updater.py
	python3 tests/test_monitor.py

test: check-layout $(TESTS)
	@set -e; for t in $(TESTS); do ./$$t; done

# Link the real engine, including actual safety, combat and interpreter paths.
# Only the server entry point is renamed; no real accounts/world files load.
tests/test_combat_status tests/test_vampire tests/test_basic_buffs tests/test_class_handlers tests/test_vampire_modules tests/test_damage_messages tests/test_transformations tests/test_clans tests/test_class_gear tests/test_drow_ammo tests/test_lifecycle_ownership tests/test_autosave_recovery tests/test_highlander_counter tests/test_stance_style tests/test_admin_gate tests/test_combat_calculations tests/test_combat_damage_death tests/test_telnet tests/test_gmcp: tests/%: tests/%.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_world_integrity: tests/test_world_integrity.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_combat_attack: tests/test_combat_attack.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrids: tests/test_hybrids.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_state: tests/test_hybrid_state.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_forms: tests/test_hybrid_forms.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_progression: tests/test_hybrid_progression.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_specialization: tests/test_hybrid_specialization.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

# Separate test objects keep sanitizer flags out of the production executable.
tests/test_restored_spells: tests/test_restored_spells.c $(filter-out tests/stun_engine/fight.o tests/stun_engine/combat_damage.o,$(ENGINE_OBJECTS)) tests/restored_fight.o tests/restored_damage.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/fight.o tests/stun_engine/combat_damage.o,$(ENGINE_OBJECTS)) tests/restored_fight.o tests/restored_damage.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/restored_fight.o: Genesis/src/fight.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -DCOMBAT_CALLBACK_TESTING $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/restored_damage.o: Genesis/src/combat_damage.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -DCOMBAT_CALLBACK_TESTING $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/stun_engine/%.o: Genesis/src/%.c $(ENGINE_HEADERS)
	mkdir -p $(dir $@)
	$(CC) $(TEST_CPPFLAGS) -DMERC $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/stun_engine/comm.o: Genesis/src/comm.c $(ENGINE_HEADERS)
	mkdir -p tests/stun_engine
	$(CC) $(TEST_CPPFLAGS) -DMERC $(CFLAGS) $(TEST_FLAGS) -Dmain=genesis_server_main -c $< -o $@

tests/stun_engine/atomic_save.o: Genesis/src/atomic_save.c $(ENGINE_HEADERS)
	mkdir -p tests/stun_engine
	$(CC) $(TEST_CPPFLAGS) -DMERC $(CFLAGS) $(TEST_FLAGS) -DATOMIC_SAVE_TESTING -c $< -o $@

tests/test_restart: tests/test_restart.c $(filter-out tests/stun_engine/restart.o,$(ENGINE_OBJECTS)) tests/restart_engine.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/restart.o,$(ENGINE_OBJECTS)) tests/restart_engine.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/restart_engine.o: Genesis/src/restart.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -DRESTART_TESTING -DATOMIC_SAVE_TESTING $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

sanitize:
	$(MAKE) clean
	$(MAKE) test CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" LDFLAGS="-fsanitize=address,undefined"

clean:
	rm -f tests/restart_engine.o
	$(MAKE) -C Genesis/src clean
	rm -f $(TESTS) tests/buffers_interp.o $(ENGINE_OBJECTS) $(LEGACY_CLASS_OBJECTS) tests/stun_comm.o tests/restored_fight.o tests/restored_damage.o
