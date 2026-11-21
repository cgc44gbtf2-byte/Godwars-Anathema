import hashlib
import unittest
from test_class_layout import ROOT, SRC, make_values
from native_drow_source import without_native_drow_stage

BASE = "5a19a2e21fbae6df7814ab263e5fb717e3705352"

def blob(text):
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

class NativeDrowCatalog(unittest.TestCase):
    def test_spider_stats_inverse_rejects_unreviewed_changes(self):
        from native_drow_spider_stats_source import without_native_drow_spider_stats
        from native_drow_attune_source import without_native_drow_attune_owner_guard
        text=(SRC/"classes/drow/drow.c").read_text()
        restored=without_native_drow_spider_stats(text)
        self.assertIn("victim->hitroll = char_hitroll(ch) - 50;",restored)
        for old,new in (("spider_hitroll < LONG_MIN + 50","spider_hitroll < LONG_MIN + 49"),
                        ("spider_armor - 50;","spider_armor - 49;")):
            with self.assertRaises(AssertionError):
                without_native_drow_spider_stats(text.replace(old,new,1))
        original=without_native_drow_attune_owner_guard(text)
        self.assertIn("if (str_cmp(ch->name,obj->questowner))",original)
        with self.assertRaises(AssertionError):
            without_native_drow_attune_owner_guard(text.replace(
                "obj->questowner[0] == '\\0'","obj->questowner[0] != '\\0'",1))

    def test_skipdisc_owned_state_has_strict_inverse(self):
        from native_drow_skipdisc_source import without_native_drow_skipdisc
        for path,expected in (
            ("merc.h","1ac72bd5054742364425b947239383997e8b2804"),
            ("handler.c","e22fb64eb8b8526c5ad435a9f986d030d581eed5"),
            # Immediately preceding local stage includes Darkness dismissal.
            ("classes/drow/drow.c","bb96c132b960755e6ffc5df58dbe902a1f530ee0"),
        ):
            text=(SRC/path).read_text()
            self.assertEqual(blob(without_native_drow_skipdisc(text)),expected)
            if path=="handler.c":
                changed=text.replace("IS_SET(ch->affected_by, AFF_FLYING)", "IS_SET(ch->affected_by, AFF_BLIND)",1)
                self.assertNotEqual(blob(without_native_drow_skipdisc(changed)),expected)
                continue
            changed=text.replace("DROW_DISC 524288","DROW_DISC 8192",1) if path=="merc.h" else text.replace("DROW_DISC);","DROW_SKIP);",1)
            with self.assertRaises(AssertionError):
                without_native_drow_skipdisc(changed)

    def test_native_darkness_dismissal_inverse_and_regression(self):
        from native_drow_source import without_native_drow_darkness_dismissal
        text=(SRC/"classes/drow/drow.c").read_text()
        self.assertEqual(blob(without_native_drow_darkness_dismissal(text)),
                         "d5fb182e96fbbcb566795f969e6d51bf5ebac4b8")
        with self.assertRaises(AssertionError):
            without_native_drow_darkness_dismissal(
                text.replace("!(IS_DROW(ch) && hybrid_drow_globe_active(ch))",
                             "!hybrid_drow_globe_active(ch)",1))
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('"--darkness-dismissal"', "darkness_loss_cleanup();",
                      'assert(strstr(output,"banish your globe"))',
                      'assert(strstr(output,"obtain darkness first"))',
                      "actor->pcdata->drow[DROW_POWERS]==DROW_SKIP"):
            self.assertIn(token,fixture)

    def test_darkness_upkeep_inverse_is_exact_and_rejects_cost_mutations(self):
        from native_drow_source import without_native_drow_darkness_upkeep
        text=(SRC/"update.c").read_text()
        self.assertEqual(blob(without_native_drow_darkness_upkeep(text)),
                         "3bf1a340a09b4ed1cccfc5b5a40850f556a47ffe")
        for old,new in (("> 150 ?", "> 149 ?"), (" - 150 : 0;", " - 149 : 0;")):
            self.assertIn(old,text)
            with self.assertRaises(AssertionError):
                without_native_drow_darkness_upkeep(text.replace(old,new,1))

    def test_darkness_audit_has_real_upkeep_and_explicit_extreme_reproducer(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("static void darkness_pulse(void)", "mobile_update();",
                      '"--darkness-lifecycle"', '"--darkness-extreme"',
                      "const long funds[]={LONG_MIN,-1,0,1,149,150,151,499,500,650,651,LONG_MAX}",
                      "actor->desc=NULL;", "actor->hit=0;update_pos(actor);",
                      "static void darkness_loss_cleanup(void)",
                      "reload.character->pcdata->classpoints[CP_CURRENT]==1000"):
            self.assertIn(token,fixture)

    def test_rejected_commands_check_pools_and_wait_in_real_world(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("static void rejected_costs(void)", "actor->wait==0",
                      "actor->exp==exp&&actor->primal==primal",
                      "actor->pcdata->ninja[NINJA_KI]==ki",
                      '"--rejected-costs"', 'rejection_cost_check("summonspider")',
                      'rejection_cost_check("faeriefire RejectedDrowTarget")',
                      "target->affected==affects"):
            self.assertIn(token,fixture)

    def test_previous_complete_source_is_preserved(self):
        self.assertEqual(blob(without_native_drow_stage(
            (SRC/"classes/drow/drow.c").read_text())), BASE)

    def test_mutations_remain_visible(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        for old, new in (
            ("victim->hit = 30000;", "victim->hit = 99999;"),
            ("ch->pcdata->quest -= 400;", "ch->pcdata->quest -= 1;"),
            ("pObjIndex->weight < 0", "pObjIndex->weight < -1"),
            ("ch->pcdata->followers != 0", "ch->pcdata->followers > 10"),
            ("cost = 10000;", "cost = 999;"),
            ("char buf[128];", "char buf[16];"),
        ):
            changed=text.replace(old,new)
            self.assertNotEqual(changed,text)
            try:
                restored=without_native_drow_stage(changed)
            except AssertionError:
                continue
            self.assertNotEqual(blob(restored),BASE)

    def test_real_world_suite_linked(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("boot_db();", '"drowarmor %s"', '"prayer NativeDrow %s"',
                      "load_char_obj", "RT_TELEPORT|RT_RETURN",
                      "MOB_VNUM_GUARDIAN%MAX_KEY_HASH", "LONG_MAX"):
            self.assertIn(token,fixture)
        self.assertIn("tests/test_native_drow", make_values(ROOT,"TESTS"))
        self.assertIn("$(ENGINE_OBJECTS)", (ROOT/"Makefile").read_text())

    def test_reports_and_training_have_native_boundary_cases(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("--report-only", "--report-extreme", "--training-bounds",
                      "LONG_MIN,5,LONG_MAX", "house_training();",
                      'command("prayer DrowStudent speed")',
                      'load_char_obj(&reload,"DrowStudent")'):
            self.assertIn(token,fixture)
        self.assertIn('assert(!memcmp(&before,actor->pcdata,sizeof before))',fixture)

    def test_only_reviewed_native_speed_runtime_edits(self):
        from native_drow_source import without_native_drow_speed
        baseline={
            "act_info.c":"1f75e52e5570adb1ec1f485da4c6448b94c0b08b",
            "combat_initiative.c":"fdb6313d65d875db0b035079a27705c65a7b6639",
            "combat_defense.c":"85870c3e786e30f58a484a6fbf6f8d4b53b2cd78",
            "combat_damage.c":"8ef28ea0e6107f77b8512f9ca40a50305ab709f0",
            "merc.h":"c7a99891b0f580cad130dc7e479672c62957cf25",
        }
        calls=0
        for path,expected in baseline.items():
            text=(SRC/path).read_text()
            self.assertEqual(blob(without_native_drow_speed(text)),expected,path)
            calls+=text.count("get_drow_speed(ch)")
            for old,new in (("* 50;","* 49;"),("* 100;","* 99;"),
                            ("rank <= 4 ? rank : 0","rank <= 5 ? rank : 0")):
                if old in text:
                    changed=text.replace(old,new,1)
                    self.assertNotEqual(blob(without_native_drow_speed(changed)),expected)
        self.assertEqual(calls,5)

    def test_native_speed_profiles_and_corrupt_data_checks(self):
        test=(ROOT/"tests/test_combat_calculations.c").read_text()
        for token in ("rows==120","2722730096019922979ULL","refusals==96",
                      "const long bad[]={-1,LONG_MIN,5,LONG_MAX}",
                      "assert(!memcmp(&before,&pa,sizeof pa))",
                      "assert(get_drow_speed(NULL)==0)"):
            self.assertIn(token,test)

    def test_prayer_menu_explains_recipient_and_self_alias(self):
        test=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('"prayer self nosuchgift"','"prayer acidbolt"',
                      'command("prayer self acidbolt")',
                      'assert(strstr(output,"prayer <name> <power>"))',
                      'load_char_obj(&reload,"NativeDrow")',"prayer_menu();"):
            self.assertIn(token,test)
        from native_drow_source import without_native_drow_stage
        text=(SRC/"classes/drow/drow.c").read_text()
        for old,new in (("cost = 5000;","cost = 50;"),
                        ("grant = DROW_ACID","grant = DROW_WEB")):
            self.assertNotEqual(blob(without_native_drow_stage(text.replace(old,new,1))),BASE)

    def test_equipment_followup_only_adds_shoot_context_guards(self):
        from native_drow_source import (without_native_drow_shoot_context,
                                        without_native_drow_ammo_template_guard)
        text=(SRC/"classes/drow/drow.c").read_text()
        prior="cf1244d5a43ebf6e54dcd50802c44866a1962b2b"
        self.assertEqual(blob(without_native_drow_shoot_context(text)),prior)
        restored_ammo=without_native_drow_ammo_template_guard(text)
        self.assertIn("if (weapon->pIndexData == NULL || weapon->value[3] < 0)",
                      restored_ammo)
        self.assertNotIn("weapon->pIndexData->item_type",restored_ammo)
        self.assertNotIn("ammo->pIndexData->item_type",restored_ammo)
        for old,new in (("weapon->pIndexData->item_type != ITEM_MISSILE",
                         "weapon->pIndexData->item_type == ITEM_MISSILE"),
                        ("ammo->pIndexData->item_type != ITEM_AMMO",
                         "ammo->pIndexData->item_type == ITEM_AMMO")):
            with self.assertRaises(AssertionError):
                without_native_drow_ammo_template_guard(text.replace(old,new,1))
        for old,new in (("ch->pcdata == NULL","ch->pcdata != NULL"),
                        ("ch->pcdata->classpoints[CP_CURRENT] -= 500;",
                         "ch->pcdata->classpoints[CP_CURRENT] -= 5;")):
            changed=text.replace(old,new)
            self.assertNotEqual(changed,text)
            try: restored=without_native_drow_shoot_context(changed)
            except AssertionError: continue
            self.assertNotEqual(blob(restored),prior)
        ammo=(ROOT/"tests/test_drow_ammo.c").read_text()
        for token in ("static void ownership_and_compatibility(void)",
                      "indices[0].item_type=ITEM_TRASH",
                      "indices[1].item_type=ITEM_TRASH",
                      "stale_ammo->carried_by==&actor"):
            self.assertIn(token,ammo)

    def test_acidbolt_safety_followup_is_strictly_scoped(self):
        from native_drow_source import (without_native_drow_acidbolt_safety,
                                       without_native_drow_faerie_bounds)
        text=without_native_drow_faerie_bounds((SRC/"classes/drow/drow.c").read_text())
        normalized=without_native_drow_acidbolt_safety(text)
        self.assertEqual(blob(normalized),
                         "7020849f7e34be6aa7969838bad497ee0aa14482")
        self.assertIn("victim == ch",text)
        self.assertIn("if (safe) return;",text)
        mutated=text.replace("victim == ch","victim != ch",1)
        with self.assertRaises(AssertionError):
            without_native_drow_acidbolt_safety(mutated)

    def test_spider_expiry_followup_is_strictly_scoped(self):
        from native_drow_source import without_native_drow_spider_expiry
        for path,prior in (
            ("classes/drow/drow.c","0d0924be76069c002ee02d5204886126000cded8"),
            ("update.c","47a1b2ab7fef30414a48a606854be831e1063d77"),
        ):
            text=(SRC/path).read_text()
            self.assertEqual(blob(without_native_drow_spider_expiry(text)),prior)
            marker="af.modifier  = DROW_SPIDER" if path.endswith("drow.c") else "paf->modifier == DROW_SPIDER"
            mutated=text.replace(marker,marker.replace("DROW_SPIDER","0"),1)
            with self.assertRaises(AssertionError):
                without_native_drow_spider_expiry(mutated)
        update=(SRC/"update.c").read_text()
        self.assertIn("if (drop_out) continue; /* The expired summon has been recycled. */",update)

    def test_faerie_bounds_followup_is_strictly_scoped(self):
        from native_drow_source import without_native_drow_faerie_bounds
        text=(SRC/"classes/drow/drow.c").read_text()
        self.assertEqual(blob(without_native_drow_faerie_bounds(text)),
                         "5642bf5697705f7f73d19ba364926058e7b5724e")
        body=text.split("void do_faeriefire(",1)[1].split("void do_summonspider(",1)[0]
        for stat in ("hitroll","damroll","armor"):
            self.assertIn(f"victim->{stat} < LONG_MIN + 50",body)
        self.assertLess(body.index("LONG_MIN"),body.index("number_percent()"))
        self.assertLess(body.index("LONG_MIN"),body.index("affect_to_char"))
        with self.assertRaises(AssertionError):
            without_native_drow_faerie_bounds(text.replace(
                "victim->armor < LONG_MIN + 50","victim->armor < LONG_MIN + 49",1))
        tests=(ROOT/"tests/test_hybrid_access.c").read_text()
        self.assertIn("assert(checks==24)",tests)
        self.assertIn("drow_utilities();native_drow_faerie_boundaries();",tests)

    def test_equipment_followup_exercises_real_recipes_and_invalid_contexts(self):
        native=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('command("drowarmor crossbow")','command("drowarmor bolts 2")',
                      'command("attunement bolt")',"dir<6","shot<2",
                      "6470082533908826985ULL","target->hit<before",
                      "ITEM_LOYAL","saved->questmaker"):
            self.assertIn(token,native)
        ammo=(ROOT/"tests/test_drow_ammo.c").read_text()
        for token in ('"--null-caster"','"--missing-player"','"--null-argument"',
                      'do_shoot(NULL,"north target")','do_shoot(&actor,NULL)',
                      "assert(!memcmp(&before,&pc,sizeof pc))",
                      "for(int i=0;i<6;++i)shooting_context(i)"):
            self.assertIn(token,ammo)

    def test_house_audit_uses_real_input_and_saved_players(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('interpret(who,d->incomm)','"--house-commands"',
                      '"dinduct HouseMember"','"promotion HouseMember"',
                      '"failure HouseMember"','"outcast HouseMember"',
                      '"clan transfer HouseMember"','"clan accept NativeDrow"',
                      'save_char_obj_checked(people[0])',
                      'assert(load_char_obj(&peers[0],"HouseMember"))',
                      'assert(!people[0]->clan[0]&&people[0]->class==CLASS_DROW',
                      'actor->pcdata->rank==rank',
                      'assert(!strcmp(before,after))'):
            self.assertIn(token,fixture)

    def test_house_audit_keeps_atomic_failure_and_recovery_checks(self):
        fixture=(ROOT/"tests/test_clans.c").read_text()
        for token in ('faults(CLASS_MAGE)','faults(CLASS_DROW)',
                      'SAVE_FAULT_CREATE','SAVE_FAULT_FLUSH','SAVE_FAULT_SYNC',
                      'SAVE_FAULT_CLOSE','SAVE_FAULT_RENAME','SAVE_FAULT_DIRSYNC',
                      '"clanadmin retry"','role(0, "Leader"); role(1, "Member")'):
            self.assertIn(token,fixture)

    def test_sign_audit_routes_real_descriptors_and_persists_channel_preferences(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('"--communication"','descriptor_list=&descriptor',
                      'sign_message("sign audit-message",peers,11,TRUE)',
                      'sign_message("sign audit-message",peers,2,TRUE)',
                      '"channels -sign"','"channels +sign"',
                      '"channels -all"','"channels +all"',
                      'load_char_obj(&reload,"SignMember")',
                      'reload.character->deaf,CHANNEL_DROW',
                      'ROOM2_SILENCE','PLR_SILENCE','GAGGED',
                      'peers[1].connected=CON_GET_OLD_PASSWORD',
                      'assert(strstr(output,"$n $N"))',
                      'descriptor_list=saved_descriptors'):
            self.assertIn(token,fixture)

    def test_dimensional_door_covers_consent_protections_and_real_reload(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('"--dimensional-travel"','rooms<8','illusion<2','consent<2',
                      'ROOM_SAFE,ROOM_PRIVATE,ROOM_SOLITARY',
                      'target->level=801','target->level=800',
                      '"summon on":"summon off"','"spiritguardian"',
                      'load_char_obj(&reload,"DoorTarget")',
                      'target_session.character=target',
                      'failed to open a Dimensional Door','MOB_VNUM_EYE',
                      'actor->pcdata->classpoints[CP_CURRENT]==points',
                      'assert(destination.people==NULL)'):
            self.assertIn(token,fixture)

    def test_skipdisc_travel_uses_real_movement_and_saved_state(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('"--skipdisc-travel"','CLASS_DROW,CLASS_MAGE',
                      'SECT_AIR,SECT_WATER_NOSWIM','dir<6',
                      'command(directions[dir])','command(directions[reverse[dir]])',
                      'get_speed(actor)==learned_speed+3',
                      'assert(!!IS_AFFECTED(reload.character,AFF_FLYING)==flying)',
                      'points[]={0,123456}','assert(destination.people==NULL)'):
            self.assertIn(token,fixture)

if __name__ == "__main__":
    unittest.main()
