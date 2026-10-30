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

## Stopping and restarting

Manual shutdown and copyover now force a final pass over dirty boards and the
pending leaderboard, ignoring the normal retry delay. A failed board does not
prevent attempts for other dirty boards or the leaderboard. Clean destinations
are not rewritten, and corrupt/rejected files are not automatically repaired.

If any snapshot is not confirmed durable, including a directory-sync failure
after rename, manual shutdown/copyover is cancelled. Player sessions remain
running. Fix the underlying storage/permissions problem and retry; successful
destinations need not be saved again. Existing player-save, draft/editor and
unsaved-area checks still apply. The checkpoint does not post editor drafts.

TERM/INT requests perform the same final pass from the normal game loop, never
inside the signal handler, before closing connections. An unresolved failure
is logged explicitly, but the requested stop still proceeds. There is one
attempt per pending destination in that pass, not a shutdown retry loop.

## Unfinished note drafts

Drafts belong to the character until a post is accepted, then belong to the
board. Link loss keeps the draft in memory while the character remains loaded;
`note write` resumes body text. As before, reconnecting with no body text starts
the headers again. `note` cancellation, successful quit, or final character
removal discards the unpublished draft; none of these auto-post it.

A line that exceeds the note's byte limit is rejected without deleting earlier
text. The limit includes colour codes and CRLF bytes. Posting rechecks the
board's write level; failed permission or file replacement keeps the draft.

Drafts are not saved in player files. A crash, forced stop, or removal of the
character can still lose unfinished text. This is not an offline draft store.

## Limits

Pending changes and editor drafts remain RAM-only. A crash, SIGKILL, power loss,
or a requested TERM/INT stop while storage is still failing can lose unsaved
changes. There is no crash-recovery journal, cross-file transaction, or automatic
reposting of drafts. A blocked filesystem operation can delay shutdown; the
service manager can still enforce its stop timeout. No implementation can
promise successful persistence through permanently failed storage.

Use one process per runtime directory. External edits or multiple independent
writers are not merged. Rewriting a full board trades more write I/O for
atomic replacement; no measured speedup or live-systemd/power-cut validation
is claimed. Corrupt-file quarantine must be repaired/reloaded or, for scores,
intentionally reset using the existing staff command.
