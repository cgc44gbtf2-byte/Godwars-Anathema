# Shared message formatting

The three engine message dialects `act`, `act2` and `kavitem` now share
bounded append/finalization helpers in `Genesis/src/core.c`. Existing ordinary
messages, audiences, visibility checks, object-body "you/your" labels,
object-room isolation and each dialect's supported/unknown tokens are retained.
No combat damage, skill ranks, resource costs, equipment ownership or save
formats are changed.

## Boundary behavior

- At most **MSL - 3** bytes of message text are emitted, reserving newline,
  carriage return and a terminating NUL. Longer messages are clipped; the
  descriptor output layer still performs its existing colour processing.
- A clipped unpaired colour prefix is removed so it cannot swallow the line
  ending; complete `##` escapes are retained. A trailing `$` is a literal dollar.
- Missing text/character/object descriptions use a neutral placeholder instead
  of dereferencing NULL. Already-supported blank placeholders in `act2` remain
  blank. Invalid actor/room context produces no message; a malformed PC
  recipient is skipped.
- `kavitem` bounds its separate possessive-name buffer as well as final output.
- Familiar projection restores its temporary sight flag on delivery, sleeping
  recipients and object-room rejection. A pre-existing sight flag is retained;
  a wizard without player data is not used for projection.

The token-specific **pointer-type contract** remains mandatory: `$p` expects an
object, `$N` a character, and `$T` text. In particular `TO_VICT` requires a
character in `arg2`. C cannot infer the dynamic type or liveness of an arbitrary
non-NULL `void *`; this change does not make incorrectly typed/freed pointers
safe.

## Verification and remaining scope

`tests/test_message_format.c` links the actual engine and pins **1,215**
pre-change native output snapshots (1215 rows, FNV aggregate
17448779909195589922). It also covers oversized/missing data, incomplete tokens,
invalid contexts, colour-boundary clipping, familiar flag restoration and
object-room audience isolation. Native cases do not deliberately exceed the
old formatter's limits; boundary tests are separate.

This is not a whole-codebase formatting audit. In particular
`spell_identify` and other commands that build their own strings before calling
the shared formatter still require separate review. The output layer itself
is unchanged. Hybrid conversion and live forms remain disabled.
