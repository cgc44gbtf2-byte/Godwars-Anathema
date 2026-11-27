# Discord crash/recovery and low-disk alerts

Optional host-side monitoring for the existing default systemd installation.
The Discord OAuth connection in chat cannot post messages or create webhooks.
This helper needs an incoming webhook; no bot token is required.

## Enable from your root SSH terminal

In your Anathema Discord server, create a private staff alert channel. In
that channel's settings, open **Integrations → Webhooks**, create an incoming
webhook and copy its URL. Keep that URL secret; anyone possessing it can post
to its channel. Do not paste it into chat, Git, shell arguments or screenshots.

```sh
cd ~/Godwars-Anathema
git pull --ff-only &&
python3 deploy/monitor.py --configure &&
python3 deploy/monitor.py --test &&
python3 deploy/monitor.py --install
```

The configure step prompts privately for the webhook (input is hidden).
It writes `/etc/anathema/monitor.json` as root-only 0600 inside a 0700
directory. Do not `cat` that file into logs or support messages.
The test command sends one explicit test message. Installation enables a
separate timer; it does **not** restart, update, kill or reconfigure the MUD.
The binary updater does not install this timer/configuration automatically.

## What it detects

- Service inactive/failed/unreadable status and subsequent active recovery.
- Increases in systemd's automatic-restart counter, even when the service has
  recovered between polls. This counter is not a complete crash history and
  can reset on explicit stop/start, unit reload/recreation or reboot.
- Filesystems containing `/`, `/var/lib/anathema` and `/var/backups`.
  Same-device paths are coalesced. Default thresholds are 90% space/inodes
  used **or** less than 1 GiB available. Healthy transitions generate recovery.

Checks run about once a minute, not continuously. The monitor does not test
gameplay, TCP readiness, hung active processes, save success or external
network reachability. An active service is not proof that players can log in.
It cannot send alerts while the VPS, network, timer or Discord is unavailable.
Independent off-host monitoring is needed for complete-host outages.

Alerts are sent on state changes, with persistent-problem reminders every
six hours. Pending events survive ordinary process/timer restarts; delivery
failures retry with 1–60 minute exponential backoff. Extended outages coalesce
older events into a bounded queue. Discord mentions are disabled.
Delivery and local state writes cannot be one atomic transaction: a crash
after successful delivery but before saving state can duplicate a message.
Events are checkpointed before network delivery. If storage is unwritable,
the monitor still attempts an alert and reports an error in its journal;
queue durability, deduplication and retry backoff cannot be guaranteed until
storage is repaired. This can produce repeated alerts rather than silence.

During an updater's exclusive lock, service checks/counter updates are
suppressed; disk checks still run. An ordinary manual `systemctl stop`
outside the updater can generate an offline alert. Copyover preserves PID
and is not classified as an automatic systemd restart.
The brief shared check lock can make an updater report busy; retry after the
monitor check has finished.

## Inspect or disable

```sh
systemctl status anathema-monitor.timer
journalctl -u anathema-monitor.service -n 30 --no-pager
systemctl disable --now anathema-monitor.timer
```

Messages contain service/disk status only, not logs, player names, passwords,
webhook URLs or save contents. Configuration/state paths are private and
HTTP redirects are refused. Root configuration is still trusted: do not run
the helper from an untrusted writable checkout. Rotate the webhook in Discord
and rerun `--configure` if it is exposed. Updating the code or unit files
requires another explicit `--install`; normal binary updates do not do it.

Tests use synthetic files, mocked service/network failures and fixture
thresholds. Unit-file verification is not a live systemd or Discord delivery
test. Your explicit `--test` confirms Discord accepted a test request; verify
the message actually appears in the intended private channel.
