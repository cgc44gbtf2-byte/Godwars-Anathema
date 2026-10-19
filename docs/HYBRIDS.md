# Hybrid conversion: staged implementation

## Available in this stage

The original Anathema implementation contains the 45 named pairs supplied for
this server, with stable hybrid IDs 1–45. These are **not base-class IDs**.
The two parent orders resolve to the same definition. Unknown parents, combined
base-class masks, duplicate parents and invalid IDs do not resolve.

`selfbrid`, `selfbrid list` and `selfbrid info Bloodmoon Revenant` are read-only.
The command shows whether the current class is one of the required parents.
This is only a parent check, **not complete conversion eligibility**.
It cannot create a hybrid, deduct currency, strip gear or grant powers.
No player-file fields or existing single-class checks changed.

## Agreed conversion behavior

- Exactly two distinct parents, using the names in the roster.
- Keep earned training from the original parent.
- Start the second parent's progression untrained; do not copy ranks or grant
  free mastery from similarly numbered legacy fields.
- Conversion price: **50,000 quest points**, selected for Anathema. The preview
  reports the planned price and whether the current balance meets it, but
  performs no deduction. Runtime price configuration is not implemented yet.
- Both parents determine eligibility and ability inheritance.
- Exclude hybrids from base-class Master/Elite rankings.
- Test abilities and balance before enabling conversion.
- Before conversion: maxed stances, styles, weapon proficiencies, trainable
  STR/DEX/CON/INT/WIS and all powers applicable to the original class/build;
  at least 50,000 maximum HP and 50,000 maximum mana. These are requirements,
  not free upgrades. The second parent still starts untrained.

The current preview does not check these mastery/stat prerequisites. Style
and weapon caps depend on the original class (100/150/200); stance training
caps at 200. Natural style currently requires a Dragon, Vampire or Werewolf
with claws. Require every style the original class can legitimately train,
at its existing cap; exempt unavailable styles. No cross-class training,
existing style rules or caps were changed.

## Work required before conversion

Vampire and Elementalist share legacy `powers[]` slots. Drow and Dragon share
`classpoints[]`. Further shared state includes generation, rage/beast,
transformations, clan affiliations and rank-dependent calculations. These need
explicit class ownership and migration, not global bitwise-OR class checks.

The next stages must provide parent-owned progression/resource records;
backward-compatible save/load and crash recovery; parent-aware training,
ability, equipment, ranking and teardown rules; and tested interactions for
every pair. The mastery/stat prerequisites above must be implemented and tested.

The eventual conversion must validate first, require explicit confirmation,
and commit a durable save without leaving partially changed characters on
failure. The donor command's 100,000 stats, legend gear, mastery/status gates
are not implicitly imported into Anathema's economy. The quest-point price was
separately approved; its balance still needs testing.

The read-only command was written for Anathema. No donor `clearclass`/staff
command dispatch, random trainer aggression or destructive preflight was copied.
