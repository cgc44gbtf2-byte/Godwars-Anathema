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
reset or migrated. Remaining primary training banks and active class timers
need separate review; this stage does not introduce new reset policy.
