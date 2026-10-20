# Board and leaderboard persistence

Boards and `leader.txt` use the existing atomic-save engine: write a private
temporary file in the destination directory, check serialization/flush/file
sync/close, rename, then sync the directory. The text formats are unchanged.
This is a snapshot replacement, not a journal or cross-file transaction.

## Failures and ownership

- Before rename, the previous destination remains intact.
- A failed interactive note post keeps the complete editor draft and does not
  announce a successful post. Fix the underlying problem and post again.
- After rename, a directory-sync failure means the new snapshot is visible but
  power-loss durability is uncertain. A post is accepted once, with a pending
  sync message; retrying it as a new post would create a duplicate.
- Valid automatic notices remain in memory on ordinary save failure. They are
  retried as part of the board snapshot. Rejected/corrupt boards remain
  read-only; automatic notices to those boards are still rejected.
- Removal/reset messages distinguish successful persistence from pending
  in-memory changes.

## Retry and operational requirements

The game pulse checks dirty board snapshots and pending leaderboard saves.
After a failure, retries are delayed at least 30 seconds to avoid a tight I/O
or logging loop. New score changes are included in the next snapshot. A valid
leaderboard reload clears its previous pending-write state.

The notes and area directories must belong to the service account and must
not be group/other writable or symlink directories. The installer already
creates these runtime directories with mode `0700`. Successful replacements
have private mode `0600`. Errors include the failing stage and cleanup status;
a cleanup error can leave a private `.save-*` temporary file for investigation.
Do not grant broad permissions to work around a disk/ownership failure.

## Limits

Pending changes and editor drafts remain RAM-only. A crash, copyover, or stop
before a pending snapshot succeeds can lose those changes. These paths do not
add a final shutdown flush or a crash-recovery journal. No implementation can
promise successful persistence through permanently failed storage.

Use one process per runtime directory. External edits or multiple independent
writers are not merged. Rewriting a full board trades more write I/O for
atomic replacement; no measured speedup or live-systemd/power-cut validation
is claimed. Corrupt-file quarantine must be repaired/reloaded or, for scores,
intentionally reset using the existing staff command.
