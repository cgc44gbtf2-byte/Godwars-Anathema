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

## Item identification

`spell_identify` now bounds variable item names, spell labels and ownership
text before sending it, with safe line endings when clipped. Missing object,
viewer or prototype data is handled without dereferencing it; invalid weapon
spell indexes display an unknown-spell description. Weapon damage averages
are calculated without signed overflow, including negative and extreme values.
Normal output, item cost, spell effects and ownership rules are unchanged.
The separate real-engine identification suite pins **1,248** pre-change output
snapshots and tests 12 boundary cases plus 169 extreme-value average pairs,
missing contexts and colour clipping. Older whole-magic pins mask only this
independently tested identification scope; every other spell remains pinned.

## Object-form score and comparison

The object-form `score` report bounds item descriptions and ownership text,
handles missing objects/prototypes and invalid weapon-spell indexes, and uses
an overflow-safe damage average. `compare` retains exact ordering of summed
weapon damage, including one-point differences and ties, without overflowing.
It does not compare rounded averages or change the existing armor comparison,
selection, equipment/visibility checks, item stats, powers or prices.
The real-engine suite pins **732** original output snapshots and exercises
**28,561** extreme-value weapon comparisons, 169 averages and nine boundaries.
The old full-info pins now mask just these separately tested scopes; all other
commands and class getters remain pinned.

This is not a whole-codebase formatting audit. Room/exit displays, normal
character scores and other commands that build
their own strings still require review. Callers must supply live,
correctly typed pointers and valid acyclic affect lists: arbitrary stale
pointers and cyclic lists cannot be made safe by output bounds. The output
layer itself is unchanged. Hybrid conversion and live forms remain disabled.
