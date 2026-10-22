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
TESTS += tests/test_hybrid_player
TESTS += tests/test_hybrid_training
TESTS += tests/test_hybrid_access
TESTS += tests/test_hybrid_resources
TESTS += tests/test_hybrid_healing
TESTS += tests/test_hybrid_readiness
TESTS += tests/test_hybrid_books
TESTS += tests/test_hybrid_colors
TESTS += tests/test_hybrid_creation
TESTS += tests/test_object_strings
TESTS += tests/test_hybrid_writing
TESTS += tests/test_hybrid_enchant
TESTS += tests/test_hybrid_vision
TESTS += tests/test_hybrid_stun
TESTS += tests/test_hybrid_monk_combat
TESTS += tests/test_hybrid_monk_buffs
TESTS += tests/test_favour_expiry
TESTS += tests/test_hybrid_celestial
TESTS += tests/test_hybrid_guardian
TESTS += tests/test_guardian_lifecycle
TESTS += tests/test_hybrid_monk_training
TESTS += tests/test_hybrid_aura
TESTS += tests/test_message_format
TESTS += tests/test_identify
TESTS += tests/test_item_inspection
TESTS += tests/test_room_display
TESTS += tests/test_character_score
TESTS += tests/test_progress_metrics
TESTS += tests/test_file_numbers
TESTS += tests/test_reader_fields
TESTS += tests/test_file_words
TESTS += tests/test_file_strings
TESTS += tests/test_inventory_records
TESTS += tests/test_aux_readers
TESTS += tests/test_aux_writers
TESTS += tests/test_admin_config
TESTS += tests/test_admin_saves
TESTS += tests/test_aux_shutdown
TESTS += tests/test_note_editor
TESTS += tests/test_hybrid_mastery
TESTS += tests/test_hybrid_scry
TESTS += tests/test_werewolf_commands
TESTS += tests/test_disquiet
TESTS += tests/test_werewolf_combat
TESTS += tests/test_werewolf_reactions
TESTS += tests/test_werewolf_room
TESTS += tests/test_werewolf_galliard
TESTS += tests/test_werewolf_touch
TESTS += tests/test_werewolf_forms
# Real validators/effective access for older isolated native fixtures too.
# Never replace hybrid access with permissive stubs just to satisfy linking.
HYBRID_ACCESS_SOURCES = $(addprefix Genesis/src/classes/hybrid/,registry.c state.c player.c build.c progression.c specialization.c access.c passives.c books.c) Genesis/src/classes/sorcerer/vision.c Genesis/src/classes/monk/favour.c Genesis/src/guardian.c

all:
	$(MAKE) -C Genesis/src

run: all
	python3 tools/start_local.py --port "$(PORT)"

tests/%: tests/%.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_comm: tests/test_comm.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/telnet_input.c Genesis/src/gmcp.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_db: tests/test_db.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/file_word.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_gameplay: tests/test_gameplay.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/act_info.c Genesis/src/classes/werewolf/wwauspice.c Genesis/src/classes/mage/mage.c $(HYBRID_ACCESS_SOURCES) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_buffers: tests/test_buffers.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) --param=asan-globals=0 -c Genesis/src/interp.c -o tests/buffers_interp.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/string.c tests/buffers_interp.o $(LDFLAGS) $(TEST_LDFLAGS) -o $@

tests/test_signals: tests/test_signals.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/signals.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

# test_save embeds save.c; retain its isolated atomic fault injection while
# linking the real optional record validator/codec, not permissive test stubs.
tests/save_info.o: Genesis/src/act_info.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/test_save: tests/test_save.c $(ENGINE_SOURCES) $(ENGINE_HEADERS) tests/save_info.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/classes/hybrid/state.c Genesis/src/classes/hybrid/player.c Genesis/src/classes/hybrid/build.c Genesis/src/classes/hybrid/progression.c Genesis/src/classes/hybrid/specialization.c Genesis/src/classes/hybrid/registry.c Genesis/src/classes/monk/favour.c Genesis/src/classes/hybrid/access.c Genesis/src/guardian.c tests/save_info.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_classes tests/test_wall tests/test_mage_powers: tests/%: tests/%.c $(ENGINE_SOURCES) $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) -O0 $< $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

# Keep this native fixture's existing mocks; use actual access/record validation.
tests/powers_info.o: Genesis/src/act_info.c $(ENGINE_HEADERS) Makefile
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) -Dchar_stat=powers_real_char_stat -Dchar_hitroll=powers_real_char_hitroll -Dchar_damroll=powers_real_char_damroll -Dchar_ac=powers_real_char_ac -Dget_drow=powers_real_get_drow -Dget_color=powers_real_get_color -Dget_monk=powers_real_get_monk -Dget_ninja=powers_real_get_ninja -Dget_tribe=powers_real_get_tribe -Ddo_look=powers_real_do_look -c $< -o $@

tests/test_powers: tests/test_powers.c $(ENGINE_SOURCES) $(ENGINE_HEADERS) tests/powers_info.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/classes/ninja/ninja.c Genesis/src/classes/monk/monk.c Genesis/src/classes/elemental/elemental.c Genesis/src/classes/sorcerer/sorcerer.c Genesis/src/classes/drow/drow.c Genesis/src/combat_status.c tests/powers_info.o $(HYBRID_ACCESS_SOURCES) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

# Preserve this legacy fixture's existing native mocks, while obtaining any
# other native getters from the real implementation for the hybrid backend.
tests/runtime_info.o: Genesis/src/act_info.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) -Dchar_stat=runtime_real_char_stat -Dchar_damroll=runtime_real_char_damroll -Dget_breed=runtime_real_get_breed -Dget_drow=runtime_real_get_drow -Dget_highland=runtime_real_get_highland -Dget_monk=runtime_real_get_monk -Dget_ninja=runtime_real_get_ninja -Dget_tribe=runtime_real_get_tribe -c $< -o $@

tests/test_class_runtime: tests/test_class_runtime.c $(ENGINE_SOURCES) $(ENGINE_HEADERS) tests/runtime_info.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/classes/elemental/elemental.c Genesis/src/classes/monk/monk.c Genesis/src/classes/ninja/ninja.c Genesis/src/classes/dragon/dragon.c Genesis/src/classes/drow/drow.c Genesis/src/classes/highlander/highland.c Genesis/src/classes/werewolf/wwtribe.c Genesis/src/combat_status.c tests/runtime_info.o $(HYBRID_ACCESS_SOURCES) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

check-layout:
	python3 tests/test_class_layout.py
	python3 tests/test_hybrid_training_catalog.py
	python3 tests/test_hybrid_access_catalog.py
	python3 tests/test_hybrid_resources_catalog.py
	python3 tests/test_hybrid_drow_catalog.py
	python3 tests/test_hybrid_healing_catalog.py
	python3 tests/test_hybrid_readiness_catalog.py
	python3 tests/test_hybrid_mastery_catalog.py
	python3 tests/test_hybrid_scry_catalog.py
	python3 tests/test_werewolf_commands_catalog.py
	python3 tests/test_werewolf_help_catalog.py
	python3 tests/test_disquiet_catalog.py
	python3 tests/test_werewolf_combat_catalog.py
	python3 tests/test_werewolf_reactions_catalog.py
	python3 tests/test_werewolf_room_catalog.py
	python3 tests/test_werewolf_galliard_catalog.py
	python3 tests/test_werewolf_touch_catalog.py
	python3 tests/test_werewolf_forms_catalog.py
	python3 tests/test_server_process.py
	python3 tests/test_hybrid_books_catalog.py
	python3 tests/test_hybrid_colors_catalog.py
	python3 tests/test_hybrid_creation_catalog.py
	python3 tests/test_hybrid_writing_catalog.py
	python3 tests/test_hybrid_enchant_catalog.py
	python3 tests/test_hybrid_vision_catalog.py
	python3 tests/test_hybrid_stun_catalog.py
	python3 tests/test_hybrid_monk_combat_catalog.py
	python3 tests/test_hybrid_monk_buffs_catalog.py
	python3 tests/test_favour_expiry_catalog.py
	python3 tests/test_hybrid_celestial_catalog.py
	python3 tests/test_hybrid_guardian_catalog.py
	python3 tests/test_guardian_lifecycle_catalog.py
	python3 tests/test_hybrid_monk_training_catalog.py
	python3 tests/test_hybrid_aura_catalog.py
	python3 tests/test_message_format_catalog.py
	python3 tests/test_identify_catalog.py
	python3 tests/test_item_inspection_catalog.py
	python3 tests/test_room_display_catalog.py
	python3 tests/test_character_score_catalog.py
	python3 tests/test_progress_metrics_catalog.py
	python3 tests/test_file_numbers_catalog.py
	python3 tests/test_reader_fields_catalog.py
	python3 tests/test_file_words_catalog.py
	python3 tests/test_file_strings_catalog.py
	python3 tests/test_inventory_records_catalog.py
	python3 tests/test_aux_readers_catalog.py
	python3 tests/test_aux_writers_catalog.py
	python3 tests/test_admin_config_catalog.py
	python3 tests/test_admin_saves_catalog.py
	python3 tests/test_aux_shutdown_catalog.py
	python3 tests/test_note_editor_catalog.py
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

tests/test_hybrid_player: tests/test_hybrid_player.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_training: tests/test_hybrid_training.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_access: tests/test_hybrid_access.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_resources: tests/test_hybrid_resources.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_healing: tests/test_hybrid_healing.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_readiness: tests/test_hybrid_readiness.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_mastery: tests/test_hybrid_mastery.c tests/test_hybrid_readiness.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_scry: tests/test_hybrid_scry.c tests/test_hybrid_aura.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_werewolf_commands: tests/test_werewolf_commands.c tests/test_hybrid_aura.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_disquiet: tests/test_disquiet.c tests/test_hybrid_aura.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_books: tests/test_hybrid_books.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_colors: tests/test_hybrid_colors.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_creation: tests/test_hybrid_creation.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_object_strings: tests/test_object_strings.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_writing: tests/test_hybrid_writing.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_stun: tests/test_hybrid_stun.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_monk_combat: tests/test_hybrid_monk_combat.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) -Wl,--wrap=one_hit,--wrap=hurt_person $(LDLIBS) -o $@

tests/test_hybrid_monk_buffs: tests/test_hybrid_monk_buffs.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_celestial: tests/test_hybrid_celestial.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_guardian: tests/test_hybrid_guardian.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_guardian_lifecycle: tests/test_guardian_lifecycle.c tests/test_hybrid_guardian.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_monk_training: tests/test_hybrid_monk_training.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_aura: tests/test_hybrid_aura.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_message_format: tests/test_message_format.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_identify: tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_item_inspection: tests/test_item_inspection.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_room_display: tests/test_room_display.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_character_score: tests/test_character_score.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_progress_metrics: tests/test_progress_metrics.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@
tests/test_file_numbers: tests/test_file_numbers.c tests/test_progress_metrics.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_reader_fields: tests/test_reader_fields.c tests/test_file_numbers.c tests/test_progress_metrics.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) -Wl,--wrap=fopen -Wl,--wrap=fclose $(LDLIBS) -o $@

tests/test_file_strings: tests/test_file_strings.c tests/test_file_words.c tests/test_file_numbers.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_inventory_records: tests/test_inventory_records.c tests/test_file_words.c tests/test_file_numbers.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_aux_readers: tests/test_aux_readers.c tests/test_file_numbers.c tests/test_progress_metrics.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_aux_writers: tests/test_aux_writers.c tests/test_file_numbers.c tests/test_progress_metrics.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_admin_config: tests/test_admin_config.c tests/test_file_numbers.c tests/test_progress_metrics.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_admin_saves: tests/test_admin_saves.c tests/test_admin_config.c tests/test_file_numbers.c tests/test_progress_metrics.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_aux_shutdown: tests/test_aux_shutdown.c tests/test_restart.c $(filter-out tests/stun_engine/restart.o,$(ENGINE_OBJECTS)) tests/restart_engine.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/restart.o,$(ENGINE_OBJECTS)) tests/restart_engine.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_note_editor: tests/test_note_editor.c tests/test_restart.c $(filter-out tests/stun_engine/restart.o,$(ENGINE_OBJECTS)) tests/restart_engine.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/restart.o,$(ENGINE_OBJECTS)) tests/restart_engine.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_file_words: tests/test_file_words.c tests/test_file_numbers.c tests/test_progress_metrics.c tests/test_identify.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_favour_expiry: tests/test_favour_expiry.c Genesis/src/classes/monk/favour.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< Genesis/src/classes/monk/favour.c $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_vision: tests/test_hybrid_vision.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_hybrid_enchant: tests/test_hybrid_enchant.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

# Separate test objects keep sanitizer flags out of the production executable.
tests/test_restored_spells: tests/test_restored_spells.c $(filter-out tests/stun_engine/fight.o tests/stun_engine/combat_damage.o,$(ENGINE_OBJECTS)) tests/restored_fight.o tests/restored_damage.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/fight.o tests/stun_engine/combat_damage.o,$(ENGINE_OBJECTS)) tests/restored_fight.o tests/restored_damage.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_werewolf_combat: tests/test_werewolf_combat.c tests/test_restored_spells.c $(filter-out tests/stun_engine/fight.o tests/stun_engine/combat_damage.o,$(ENGINE_OBJECTS)) tests/restored_fight.o tests/restored_damage.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/fight.o tests/stun_engine/combat_damage.o,$(ENGINE_OBJECTS)) tests/restored_fight.o tests/restored_damage.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_werewolf_reactions: tests/test_werewolf_reactions.c tests/test_restored_spells.c $(filter-out tests/stun_engine/classes/werewolf/wwtribe.o,$(ENGINE_OBJECTS)) tests/reaction_tribe.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/classes/werewolf/wwtribe.o,$(ENGINE_OBJECTS)) tests/reaction_tribe.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/reaction_tribe.o: Genesis/src/classes/werewolf/wwtribe.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -Dkillperson=werewolf_test_killperson -Ddo_kill=werewolf_test_do_kill $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/test_werewolf_room: tests/test_werewolf_room.c tests/test_restored_spells.c $(filter-out tests/stun_engine/classes/werewolf/wwtribe.o,$(ENGINE_OBJECTS)) tests/room_tribe.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/classes/werewolf/wwtribe.o,$(ENGINE_OBJECTS)) tests/room_tribe.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/room_tribe.o: Genesis/src/classes/werewolf/wwtribe.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -Ddo_flee=werewolf_room_flee -Dinterpret=werewolf_room_interpret $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/test_werewolf_galliard: tests/test_werewolf_galliard.c tests/test_restored_spells.c $(filter-out tests/stun_engine/classes/werewolf/wwauspice.o,$(ENGINE_OBJECTS)) tests/galliard_auspice.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/classes/werewolf/wwauspice.o,$(ENGINE_OBJECTS)) tests/galliard_auspice.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_werewolf_touch: tests/test_werewolf_touch.c tests/test_restored_spells.c $(ENGINE_OBJECTS)
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(ENGINE_OBJECTS) $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/test_werewolf_forms: tests/test_werewolf_forms.c tests/test_restored_spells.c $(filter-out tests/stun_engine/classes/werewolf/wwauspice.o,$(ENGINE_OBJECTS)) tests/forms_auspice.o
	$(CC) $(TEST_CPPFLAGS) $(CFLAGS) $(TEST_FLAGS) $< $(filter-out tests/stun_engine/classes/werewolf/wwauspice.o,$(ENGINE_OBJECTS)) tests/forms_auspice.o $(LDFLAGS) $(TEST_LDFLAGS) $(LDLIBS) -o $@

tests/forms_auspice.o: Genesis/src/classes/werewolf/wwauspice.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -Ddo_majesty=forms_majesty -Ddo_shadowbody=forms_shadowbody -Ddo_change=forms_change -Ddo_werewolf=forms_werewolf -Ddo_beastlike=forms_beastlike -Ddo_berserk=forms_berserk $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

tests/galliard_auspice.o: Genesis/src/classes/werewolf/wwauspice.c $(ENGINE_HEADERS)
	$(CC) $(TEST_CPPFLAGS) -DMERC -Dget_room_index=werewolf_galliard_room $(CFLAGS) $(TEST_FLAGS) -c $< -o $@

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
	rm -f tests/save_info.o tests/restart_engine.o tests/runtime_info.o tests/powers_info.o
	$(MAKE) -C Genesis/src clean
	rm -f $(TESTS) tests/buffers_interp.o $(ENGINE_OBJECTS) $(LEGACY_CLASS_OBJECTS) tests/stun_comm.o tests/restored_fight.o tests/restored_damage.o tests/reaction_tribe.o tests/room_tribe.o tests/galliard_auspice.o tests/forms_auspice.o
