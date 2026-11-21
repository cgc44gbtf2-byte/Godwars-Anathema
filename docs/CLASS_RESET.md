# Class reset safety

`clearclass <player>` remains the existing destructive staff command. Managed
clan members must leave or resolve leadership first. Its established level,
stats, training, quest and experience reset rules are not changed here; gold
and primal are retained. Equipment is unequipped, not destroyed.

The reset now releases the previous condition, relationship, covenant,
leadership, lord, clan and morph strings before replacing them. Both owned
strings and boot-pooled strings use the normal engine ownership rules.

An attached secondary hybrid record is discarded with the original class.
Keeping it would invalidate the resulting player file and reject the final
save. Invalid or rejected input records instead stop before reset mutations;
this is not a corruption-repair command.

The real `mclear` path already removes temporary affects and equipment bonuses.
Regression coverage keeps that behavior and reloads all 45 hybrid pairings in
both parent orders. Native reset outcomes retain 240 original snapshots.

Saving still uses the existing primary/backup retry policy. An I/O failure
does not roll back the in-memory reset; there is no crash journal or
multi-file transaction. No live players or saved files are automatically
reset or migrated.

The reset also clears previously omitted Mage spheres, Arete, quintessence,
paradox/ward, Beginner's Luck charges and Doublecast cooldown; Sorcerer color
ranks; and the Vampire Absorb Soul snapshot. All slots are cleared even when
left behind by an older class, and the empty banks are verified after reload.
Selecting Mage again still grants the normal initial Arete through `autoclass`;
no class acquisition, training costs or non-reset progression are changed.

Timed effects another caster can place on the target (Better Body, Mutate
Ephemera, Slow, Liquify Earth, Atoning Winds and Enrage) are not personal
training. Their timers and flags are deliberately retained by this change.
Wider form/flag provenance and other class timers still require separate review.
