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

## Room and exit displays

Room titles and detailed exit lists now use bounded formatting and appending.
Oversized labels are clipped with safe line endings; an unusually large
detailed exit list may omit later entries at the existing MSL buffer limit.
Missing room names show `Somewhere`. Looking at objects without a prototype
still uses their available descriptions; inspecting a nonempty drink with an
invalid liquid index reports that its colour cannot be determined. No room,
object or liquid values are repaired or changed by displaying them.

The real-engine suite pins **1,792** original output snapshots across normal,
Umbra/deep-Umbra, blindness, blindfold, brief, builder, fog, darkness, auto-exit
and ANSI settings. It also exercises 15 boundary cases, colour clipping, and
portal room/temporary-shadow restoration. Existing visibility and navigation
policy is retained; this is not a visibility-policy rewrite.

## Character score

Normal character scores bound name/title and creation-time reports,
with explicit fallbacks for missing strings and safe returns for missing
character/PC contexts. It retains object-form delegation. Play-time and PK-score
calculations retain the legacy operation order and integer division, but show
`unavailable` if an intermediate overflows or the PK denominator is zero.
This does not repair saved fields, alter training/stats or change combat metric
formulas. Score delegates its arithmetic checks to the shared APIs below.

The real-engine suite pins **4,224** unchanged native score snapshots across all
classes, status/staff ranges, beast thresholds, conditions and ANSI settings.
It also checks 196 PK and **2,744** timing combinations against independent
wide-integer oracles, 13 boundaries, colour clipping and both object-form
delegation flags. Older whole-info pins mask only this separately characterized
report; all other commands and combat metric implementations stay pinned.

## Shared progress metrics

The checked play-time and PK-score APIs preserve legacy arithmetic order,
division and representable results. Failure leaves the output parameter alone.
Login eligibility newly enables PK only for a valid age at the unchanged
threshold; invalid timing never grants it or removes an existing flag.
Leaderboard updates skip only invalid derived metrics and retain unrelated counters.
Staff inspection and covenant scores display `unavailable` instead of a bogus zero.
Legacy `get_hours`/`get_ratio` wrappers remain for compatibility and return zero on failure;
all in-tree policy/display callers use checked results, not that ambiguous fallback.

The real-engine suite pins **3,200** unchanged helper, staff, covenant, login-age
and leaderboard snapshots. Independent wide-integer checks cover 196 PK and
2,744 timing combinations, including failure-output preservation, repeated
eligibility checks, independent leaderboard updates and ordinary file reloads.
All leaderboard test writes use private temporary directories.

The separate numeric-reader audit below covers file decoding, not arbitrary
corrupt-stat repair or leaderboard write/crash durability.
Other combat metrics still require valid in-range stats and class inputs.

## Checked numeric file decoding

`file_number.c` handles both signed `long` boundaries and checked decimal/pipe
arithmetic without recursion. The legacy `|` syntax is **right-associative
addition**, not bitwise OR: even its intermediate sums must fit. Ordinary signs,
leading whitespace, numeric prefixes and stream positions remain unchanged.
One ASCII space is consumed; other delimiters are pushed back, including every
non-ASCII byte. EOF is never mistaken for a byte. A failed checked read leaves
its output untouched and distinguishes format, range, work-limit and I/O errors.
Each field is bounded to 1,024 terms and 65,536 read attempts.

The existing boot/configuration wrapper still stops on an invalid number.
Full/short player reads, full/tiny inventory reads and offline `finger` instead
reject the affected record. Temporary fallback values are not accepted as a
successful load. Numeric failure stops further word parsing and partially read
inventory is released without publishing a half-built object. Negative lookup
references, invalid nesting, missing tiny-object templates and impossible board
counts are rejected before unsafe indexing or logging through a closed reserve.
Normal missing full-object templates retain the existing 30041 fallback.

The real-engine suite pins **3,042** ordinary parser/player snapshots and checks
2,744 independent wide-integer pipe combinations, all 256 byte values in leading
and trailing positions, I/O errors, work limits, leaderboard boundary reloads,
full/short inventory failures, offline inspection and recovery. A separate
isolated TCP smoke test rejects malformed records at both login stages without
rewriting them, keeps an unrelated in-game player responsive, and verifies login
after the fixture is repaired. No historical accounts or live host are used.

No save format, compatibility epoch, game costs or training ranks changed.
This is not complete semantic validation of every numeric field: representable
values can still be inappropriate for downstream gameplay or save arithmetic.
Legacy string/word readers, unrelated error/logging paths and permanent-disk
failure recovery remain separate work. Startup still deliberately fails on bad
boot data; it does not silently substitute numeric defaults.

This is not a whole-codebase formatting audit. Other commands that build
their own strings still require review. Callers must supply live,
correctly typed pointers and valid acyclic affect lists: arbitrary stale
pointers and cyclic lists cannot be made safe by output bounds. The output
layer itself is unchanged. Hybrid conversion and live forms remain disabled.
