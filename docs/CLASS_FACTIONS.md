# Class factions and clan governance

## Approved rules

Class factions and their social organizations share one authoritative clan
registry. A Black Furies leader is the Black Furies clan leader, not the holder
of a second, independent office. Kingdoms, lordships, traditions, houses and
Ninja clans use the same membership, officer and leadership-transfer rules.
Training, generation and power ranks do not grant leadership.

Classed characters do not have to belong to a clan. The affiliation
labels are **Loner** for an unaffiliated character or voluntary departure, and
**Exiled** for expulsion. Neither label removes the class, earned training or
ordinary class powers. An accepted new membership displays the new clan and
role instead. Exile provenance is persisted independently of mutable
display strings; a missing or unavailable registry is not proof of exile.

These rules apply to **every class**, including the requested variants and
Fae, not just Vampires and Mages. Loner and Exiled are likewise available to
every class.

Ordinary membership requires a named invitation from a leader or officer and
the recipient's own acceptance. Class-specific recruitment is a separate,
approved path for **all classes**, using the appropriate class-specific
method. Vampire embrace and Mage teaching are examples, not an exclusive
list. A qualifying ritual should establish the class and enroll the willing
recruit in the sponsor's managed faction as a **member**, never as an officer
or leader.
An ordinary member may sponsor through a qualifying class ritual; that does
not grant ordinary invitation or promotion authority.

Recruitment must disclose the resulting class and faction before acceptance.
It must reject existing classes/memberships, recheck consent and sponsor
membership at acceptance, and recover safely if interrupted between saving
class and membership. A blanket willingness flag is not named consent.
Combat feeding/attacks and ordinary skill lessons must not silently class or
enroll their targets. No active character is automatically migrated.

## EM additions requested

- Fae: separate class, with training, forms, powers and class equipment.
- Marauder: an alternative **Mage** progression, not Sorcerer.
- Gargoyle: an alternative Vampire progression.
- Nightstalker: an alternative Ninja progression.
- Ninja factions: Bishamon, Hotei, Jurojin, Mudevi and Kotaro.

Variants replace the relevant parent progression rather than stacking every
parent power on top. Existing characters require explicit conversion consent;
EM's destructive rank-reset code must not be copied unmodified.
Selfbrid development remains paused.

## Commands and recovery

Use `clan` to see affiliation, rank and the authoritative clan/faction leader.
Ordinary `clan invite <player>` remains restricted to leaders and officers.
Any registered member of a current native class may instead offer
`clan recruit <unclassed player>`. Vampires can use `clan embrace <player>`;
Mages can use `clan teach <player>`. These are deliberate consent rituals:
ordinary combat embrace/bites and power lessons do **not** class anyone.

The recipient accepts **both** native class initialization and clan membership
with `clan accept <sponsor>`, or declines with `clan decline`. Offers expire
after 120 seconds. Both participants must be awake, standing, out of combat,
in the same room and plane, visible to each other and using their own input.
Native stat and class prerequisites are checked again on acceptance.
Sponsor membership and native build are also rechecked. New recruits become
Members, never officers or leaders, and do not inherit trained power ranks.
Vampire bloodlines, Mage traditions and Dragon colors follow the sponsor's
native build; custom social clan names do not change that build.

The eligible unclassed player is saved first. A durable registry intent then
records the accepted class-and-clan enrollment before class initialization.
An interrupted enrollment can recover from that intent at player load.
Already-classed players are never reinitialized; a conflicting saved class
fails closed for staff review. An uncertain registry write grants no new class
until `clanadmin retry` confirms persistence.

If the initialized player save or acknowledgement cannot complete, retain the
intent and use `clan settle`. Class changes and identity reuse stay blocked
while an intent is outstanding, including after expulsion or disbanding.
Ordinary save intervals still apply to subsequent gameplay; this is not a
general inventory-trade journal or a claim of live power-cut testing.

Registry versions 1 and 2 remain readable. Version 2 stores exile provenance;
version 3 additionally stores pending native enrollment recipes. A settled
registry without exile records is written in version 1 again. Player-file
schema, save epoch and copyover handoff are unchanged. Older binaries do not
understand registry versions 2/3: review compatibility before rollback and
preserve current player files and registry together. Do not overwrite either
with an older snapshot to force compatibility.

Automatic binary rollback currently refuses registry versions 2/3 (and unknown
headers), including automatic fallback after a failed activation. Old backup
records do not declare which registry versions their binaries can read.
Recovery requires reviewed maintenance with a compatible executable; current
player files and the registry are retained. This conservative guard also
applies when the backup might in fact support the newer format.

## Implementation status

The existing registry governs membership, officers, transfers, offline members
and named invitations for the ten current native classes. `clan` now reports
its authoritative faction/clan leader, and consumers can query the same role
without interpreting power ranks or legacy `clan`/`lord` strings.

This stage adds named native-class recruitment, persistent Loner/Exiled status
and a shared leadership query API. Native initialization, power costs and
progression are reused, not replaced. `autoclass` cannot bypass an unresolved
accepted enrollment. Existing combat embrace and class equipment are unchanged.

Faction-specific eligibility binding beyond the native recruitment build,
legacy migration and the EM class/variant additions above are **not implemented
by this stage**. No existing character is automatically converted. Do not
advertise Fae or the requested variants as playable yet.
