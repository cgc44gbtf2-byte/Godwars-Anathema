# Gargoyle: native Vampire variant

Select `autoclass vampire gargoyle` on an eligible unclassed character.
This is a permanent **Vampire build**, not a new primary class or a clan name.
Existing Vampires are not automatically changed. Entry uses Anathema's normal
requirements and generation 9, not EM's generation-2 grant.

## Progression and commands

- `discipline`: Visceratika replaces Thaumaturgy in the menu.
- `powers`: lists trained Visceratika commands separately from stone-form
  passives, including rank-4 punch resistance and rank-5 reflection. This
  read-only menu remains available while petrified.
- `discipline visceratika`: ranks 1–5; **15 × next rank** primal, including
  15 primal to start. No clan-name discount or extra in-clan discipline slots.
- Visceratika reduces positive incoming damage by 10/20/30/40/50%, after
  Anathema's existing calculations. Integer damage rounds down. The shared HP
  boundary covers melee and direct book/spell hits once; healing and
  self-inflicted costs are not reduced.
- Rank 1 unlocks `camouflage` (Skin of the Chameleon): free, 12 action lag,
  using normal Hide visibility and reveal rules. Already hidden or invalid
  physical/recovery contexts cannot activate it.
- Ranks 4–5 resist the native `punch` command's damage and stun, like the
  engine's existing high-Fortitude immunity. EM's HP-restoration/finger-breaking
  workaround is not imported.
- Other Vampire disciplines, blood pools, generation rules and `varmor` remain
  Anathema's native systems. No flight or new item vnums are added.
  `varmor mark`, `varmor trenchcoat` and `varmor vest` retain the native cost of
  100 primal plus 400 quest points per item.

Thaumaturgy's five blood powers remain unavailable even though the saved rank
slot is shared. Quietus is separate. `teachdisc` names the replacement correctly:
Gargoyles can teach Visceratika to Gargoyles, not Thaumaturgy to ordinary
Vampires or the reverse. Other discipline lessons retain existing rules.
Pending lessons cannot be accepted while either participant is a stone statue.
The offer remains valid until its normal expiry; return to flesh before accepting.

## Stone statue form

`stoneform on` (or bare `stoneform`) enters an **immobile defensive statue**
at Visceratika 1. The rank's 10–50% stone mitigation and rank-4 punch resistance
apply **only in this form**, once—not as another stacked passive defense.
`stoneform off` returns to flesh; `stoneform status` reports the state.
Entry and exit have 12 ticks of action recovery, with no additional resource
charge. Entry can occur during combat but does not stop opponents attacking.
It grants no extra healing, teleport immunity or flight.

At **Visceratika 5**, a surviving statue passively reflects **25% of HP damage
actually taken after mitigation**, rounded down per hit. This is retaliation,
not another damage reduction. Healing, self-inflicted costs, zero damage and
lower ranks do not trigger it. Shared HP-path defenses, including another
statue's mitigation, still apply; reflection is not a weapon attack.
Reflections cannot themselves trigger reflection or attack-proc chains.

Retaliation resolves at the next game-input pulse, outside the original attack
stack, and can kill an attacker with normal kill credit. Hits against the same
pair are coalesced. Both characters must still be valid, in the original room,
and eligible under normal combat safety; the defender must still be a trained
stone statue. Leaving stone form, losing training or becoming incapacitated
cancels pending retaliation. Runtime identities reject extracted/recycled actors.
Pending retaliation is transient and is not saved across logout or copyover.

While petrified, voluntary movement (including following), weapon attacks and
active powers are blocked. Read-only commands, save/quit, chat/tell/reply and
already-authorized administrative copyover remain available under their normal
permissions/restrictions. Exit does not require retaining the class or training.
Other body forms cannot overlap. Class/training loss clears stale statue state
on the next combat-status pulse or load; saving projects a safe mask without
mutating the live character. Full/short saves and exec-copyover retain valid
statues. Normal class reset clears them.

## Class and clan are separate

A Gargoyle clan sponsor recruits a willing unclassed player through
`clan recruit <name>` / `clan accept <sponsor>`. The recipient receives the
Gargoyle build and Member role, not the sponsor's earned ranks.
The sponsor must be in flesh when offering recruitment and when it is accepted.
An unexpired offer remains usable after the sponsor leaves stone form.
Leaving becomes Loner; removal becomes Exiled; neither erases the build.
Pending enrollments cannot recover as the wrong Vampire variant.

The reserved saved identity is `Clandisc 1002 -1 -1`; Visceratika occupies the
existing Thaumaturgy rank slot. Full and short loaders and exec-copyover preserve
it. Normal class reset clears it. No donor player files or accounts are used.

## Compatibility and limits

No save-file grammar or handoff format changes. **Do not run saved Gargoyles on
a pre-Gargoyle binary:** it would interpret their shared rank as Thaumaturgy.
Review/reset those characters or restore a matching pre-creation backup before
that downgrade; no automatic downgrade migration is supplied.

EM's rank-2/rank-4 subtraction typos, unsafe punch HP manipulation, malformed
skin-display lookup, generation grant and donor world dependencies are
not imported. The stone defense uses Anathema's shared HP path, rather than
EM's inconsistent separate melee/book formulas. It does not protect raw HP
assignments that bypass that path. PvP balance has not been live-tested.
Selfbrid and secondary-Gargoyle integration remain paused.
