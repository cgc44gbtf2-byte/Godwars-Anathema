# Nightstalker

Nightstalker is a native Ninja variant adapted from the supplied Ethereal Mists
source. It replaces Michi, not the Ninja class ID or social clan authority.
Selfbrid and the other EM variants remain outside this change.

## Choosing and recruiting

An eligible, unclassed player uses `autoclass ninja nightstalker`. Existing Ninja
prerequisites apply. A classed character cannot switch builds with autoclass.
The choice is permanent until a staff-reviewed class reset.

A registered Nightstalker member can use `clan recruit <unclassed player>`.
The recipient uses `clan accept <sponsor>` to accept the variant and membership
as Member. Normal Ninja sponsors still recruit normal Ninjas. Named consent,
native prerequisites and existing interrupted-enrollment recovery still apply.

Use `principle` and `principle sora` for the variant menu. Training uses the
existing three Ninja principles, caps, unlock ranks and primal costs.

## Powers and differences

- Michi is unavailable; the main Ninja recovery pulse clears stale Michi.
- Kakusu costs 60 Ki to activate, never more than the available pool.
  Deactivating it is free. Its normal Sora 6 unlock remains.
- Heighten costs 60 Ki to activate at Sora 4; deactivation is free.
  Readaura and Scry retain their existing costs and gates.
- Natural/room-flag darkness and Vampire Obfuscate can be seen through.
  Blindfolds, blindness, plane separation and staff invisibility still apply.
  Drow's separately owned Globe of Darkness remains an obstruction.
- Ki recovery is 1 normally, 3 meditating, 0 while cloaked, capped at 300.
  Normal Ninja recovery remains 2/5/0.
- Harakiri at Chikyu 10 requires at least 10% maximum HP and positive current
  HP. It halves **current** HP and Mana with integer rounding down, rather than
  setting them to the normal Ninja's 5% of maximum. Existing active rituals
  cannot stack. Duration uses Anathema's formula, safely capped at `INT_MAX`.

The donor's movement pool does not exist in Anathema and is not invented.
The donor's check-for-30/spend-60 bug is corrected. Donor HQ room numbers,
clan-name authorization, generation changes and staff holylight flags are not
copied. Normal Ninja powers, equipment and other-class recipes are unchanged.

## Identity, persistence and limits

The class-discriminated `Clandisc[0]` build slot stores
`NINJA_BUILD_NIGHTSTALKER` (1001) for primary Ninja. This field is already saved
and loaded by full and short player loaders, and cleared by class reset.
It is not an array index. Missing/ordinary legacy Ninja values remain normal
Ninja. No new player record, save epoch, handoff or donor class ID is introduced.

Renaming a social clan, leaving, or being Exiled does not change the variant.
Naming a clan “Nightstalkers” does not grant it. Legacy clan labels are not
automatically migrated. No NPC or secondary hybrid parent gets these powers.
The existing clan-registry compatibility guard still governs binary rollback;
keep the registry and player files together. No live host migration is implied.
