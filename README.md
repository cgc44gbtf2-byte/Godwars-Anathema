# Godwars: Anathema

A classic text-based multiplayer RPG, updated with class fixes, safer player
saves, improved client support and a more maintainable codebase.

**Development is ongoing.** This is not a completed balance pass or a guarantee
that every legacy feature is bug-free.

## Classes and player changes

The ten base classes are **Dragon, Drow, Elementalist, Highlander, Mage, Monk,
Ninja, Sorcerer, Vampire and Werewolf**.

Recent work includes:

- Consent-based Vampire teaching and Path of Blood abilities.
- Highlander Counter, restored Monk attacks and crafted Drow ammunition.
- Class-power, equipment and resource-cost fixes.
- Werewolf advancement, shifting and gift-help corrections, plus safer utility commands.
- Updated damage wording without changing the existing damage thresholds.
- Safer saves, reconnects, shutdowns, combat messages and item handling.
- Mudlet/MudForge support, GMCP vitals and an optional Mudlet HUD.

See the [class guide](deploy/CLASS_AUDIT.md) and
[gear guide](deploy/CLASS_GEAR.md) for commands, costs and known gaps.

## Hybrid status

**Players cannot convert into hybrids yet.**

`selfbrid list` and `selfbrid info` provide previews for your original class.
Some secondary-class training and abilities work for existing configured
records; this does not make general hybrid conversion available.

See the [hybrid guide](docs/HYBRIDS.md) for implemented features, training rules,
book limits and remaining work.

## Install and start

CI targets **Debian 12/13**, **Ubuntu 22.04/24.04** and **Fedora 43/44**.
Review the [licensing notes](#credits-and-licensing) before running or
redistributing the game.

On a fresh server with Git and Python 3 installed:

```sh
git clone --branch Godwars-Anathema https://github.com/cgc44gbtf2-byte/Godwars-Anathema.git
cd Godwars-Anathema
python3 deploy/setup.py                       # preview
sudo python3 deploy/setup.py --apply --start  # install and start
```

The installer builds the game, imports the included world, creates the service
account and configures systemd. It refuses to replace an existing installation.

Use the [installation guide](deploy/INSTALL.md) for prerequisites, owner-account
authorization, local testing and troubleshooting.

## Server management

For an installed systemd service:

```sh
sudo systemctl status anathema.service
sudo systemctl start anathema.service
sudo systemctl stop anathema.service
sudo journalctl -u anathema.service -n 50 --no-pager
```

For updates, follow the [CI-gated update guide](deploy/UPDATING.md). It covers
copyover, maintenance restarts, verification and rollback.
Back up the complete server state before upgrading; do not bypass compatibility
checks or assume an older binary can read migrated saves.

Optional [Discord health alerts](deploy/MONITORING.md) run separately from the
game service.

## Guides

- [Client setup and compatibility](docs/CLIENTS.md)
- [Optional Mudlet HUD](clients/mudlet/README.md)
- [Classes, skills and known gaps](deploy/CLASS_AUDIT.md)
- [Werewolf commands, advancement and known gaps](docs/WEREWOLF.md)
- [Class gear and consumables](deploy/CLASS_GEAR.md)
- [Clan management](deploy/CLAN_MANAGEMENT.md)
- [Hybrid implementation status](docs/HYBRIDS.md)
- [Combat organization and remaining work](docs/COMBAT.md)
- [World compatibility audit](docs/WORLD_AUDIT.md)

## Building and testing

Build and edit **`Genesis/src/`**. With the development dependencies installed:

```sh
make -j2
make test
make sanitize
make -j2
```

The executable is `Genesis/src/merc`. Rebuild normally after sanitizer testing,
and run the game as an unprivileged service account—not root.
Current test coverage is defined in the root `Makefile` and CI workflow.

## Credits and licensing

Created by **LaWaylon (Luto)**.
Copyright (c) 2026 LaWaylon (Luto) — Anathema modifications.

Original Genesis, Diku, Merc and God Wars credits, copyright notices and license
obligations remain intact. Referenced permission/license documents were absent
from the supplied archive; resolve those requirements before operating or
redistributing the game. This branch grants no new rights.

`Genesis.zip` is an unpatched historical reference, not an installer.
Do not extract it over this tree or reuse its historical accounts.
