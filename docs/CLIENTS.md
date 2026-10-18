# MudForge and Mudlet connections

## Connection settings

Use the **host and port supplied by your server operator**. The executable's
default game port is 4000; an operator can choose another.

- **Mudlet:** create a profile with the server address and port. Leave Secure
  / TLS disabled for this build's ordinary TCP endpoint. A separately
  configured TLS proxy needs its own operator-provided address and port.
- **MudForge desktop:** create a connection using that same TCP/Telnet
  address and port.
- **MudForge browser:** open <https://play.mudvault.org/> from
  <https://mudforge.org/>. MudForge supplies the WebSocket-to-Telnet bridge.
  The game endpoint must be reachable from that bridge; your private
  `127.0.0.1` test server is not a public game endpoint.

Mudlet's connection guide:
<https://wiki.mudlet.org/w/Manual%3AIntroduction>.

The game itself has no native WebSocket listener or native TLS in this stage.
Browser HTTPS is **not** end-to-end encryption to this ordinary Telnet
endpoint. The bridge relays game traffic, including login traffic. Never
reuse an important password.

## Supported scope

Ordinary text play is retained. Optional **GMCP vitals** are now supported;
see [the packet schema and client notes](GMCP.md). Automatic mapping and
client interface packages are not included.

| Feature | Current server behavior |
| --- | --- |
| Text commands and existing ANSI output | Retained |
| Telnet negotiation, including fragmented packets | Parsed before command input |
| Password ECHO acknowledgements | Existing WILL/WONT ECHO flow retained |
| TTYPE, NAWS and other unsupported options | Declined, subnegotiation discarded |
| GMCP | Optional Char.Vitals and Anathema.Vitals, authenticated self-only snapshots |
| MSDP, MXP and MCCP | Not implemented; unsupported negotiation declined |
| Party data and automatic room maps | Not supplied by this server |
| UTF-8 command input | Not added; legacy ASCII canonicalization remains |

Client aliases, text triggers and locally scripted interfaces can still use
ordinary game text. A client advertising extra protocols must not cause
protocol bytes, terminal names, dimensions or JSON to become game commands.
No Mudlet package or MudForge plugin is bundled.

## Implementation

`Genesis/src/telnet_input.c` owns incremental Telnet input framing and bounded,
binary-safe refusal replies. `comm.c` passes raw socket bytes through it before
the existing command canonicalizer. NUL bytes do not terminate or discard
subsequent received text. Escaped IAC is framed correctly; the existing ASCII
command layer still does not treat it as a printable input character.

Unknown positive option requests receive WONT or DONT. Negative
acknowledgements do not cause reply loops. DO/DONT ECHO acknowledge the
server's existing password-echo advertisements without undoing them.
DO/DONT GMCP change only descriptor-local telemetry state. The server offers
GMCP once per new TCP connection; refusal leaves ordinary text play unchanged.
Subnegotiation is discarded through IAC SE, including embedded NUL, newlines
and escaped IAC. No client-supplied protocol data executes commands.

Raw input processing has a per-descriptor, per-read-call byte budget, even
when all input is discarded framing. Existing text input and output limits
still apply. A stalled or flooding connection cannot make this reader consume
an unbounded raw stream in one game-loop turn.

Replies bypass game color expansion and prompt whitespace. Growing a pending
output buffer copies bytes, not C strings, so an unsupported binary option
with value zero does not erase later output.

## Restart and rollback

Copyover handoff version **2** records each connection's framing state and
pending negotiation verb. A partial subnegotiation therefore stays separate
from commands after actual exec. The new binary also accepts version 1
handoffs, initializing their framing state to ordinary data. Invalid states,
verbs and extra handoff fields are rejected before restoring any session.
Player-save format is unchanged.

GMCP is not serialized into the handoff. After successful recovery, WONT GMCP
then WILL GMCP reset and renegotiate the inherited client before any new
vitals. Cancelled/failed exec leaves the existing negotiation unchanged.
Copyover refuses a control packet partway through actual transmission rather
than truncating it. Let output drain before retrying. Wholly unsent buffered
text/snapshots may still be discarded; vitals are refreshed after recovery.

An older binary does **not** understand the version 2 handoff. To roll back,
use a checked save and normal stop/start with backups, not a live copyover
from this binary into an older executable.

## Verification and limitations

`tests/test_telnet.c` uses real nonblocking socket pairs and the actual engine
reader. It exercises all 256 option-byte values for WILL/WONT/DO/DONT, every
split of a mixed negotiation packet, single-byte fragments, subnegotiation,
NUL, escaped IAC, password ECHO acknowledgements, binary-output growth,
multiple commands and input/output limits. A raw-control flood test verifies
that unread bytes remain for a later game-loop turn.

`tests/test_restart.c` checks legacy recovery, every supported partial state
and verb, and malformed version 2 handoffs. `tests/smoke_copyover.py` carries
an incomplete client subnegotiation through the production executable's
real exec and verifies a subsequent save on the same TCP connection.
It also decodes real GMCP JSON, checks that no vitals arrive during login,
and verifies reset, renegotiation and fresh snapshots across actual exec.
The pending-login and new-arrival connections still exercise plain text play.

These are transport-level regression checks. They are **not** recorded GUI
sessions in real Mudlet or MudForge releases, a live hosted-bridge test, a TLS
deployment test, a UTF-8 upgrade, or certification of enhanced client features.

Smoke-test greetings use a bounded 25-second receive deadline, rather than
inheriting the short TCP-connect timeout. EOF still fails immediately and
missing output still fails at the deadline; the test never reconnects or
restarts a failing server to hide an error. Deterministic Python checks cover
delayed/partial replies, EOF, deadline expiry and oversized output. Failures
include the tail of the synthetic server log for diagnosis.
