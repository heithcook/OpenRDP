# OpenRDP Client — Phase 1 Specification

Status: **Authoritative Phase 1 requirements**  
Recorded: 2026-08-27  
Related: [Project specification](project-specification.md)

## Current context

Phase 1 had already reached a working implementation before this specification was
recorded. The immediate development task is to fix the current WebAuthn/web-authentication
defect without regressing previously working core RDP behavior. Do not rebuild Phase 1
from scratch merely because this document describes its original development sequence.

## Outcome

Deliver a genuinely usable native Linux RDP client which:

- uses C++20, Qt 6, CMake, upstream FreeRDP 3.x, WinPR, and FreeRDP's OpenSSL integration;
- runs on x86_64 Ubuntu 24.04+, Ubuntu 26.04+, and Arch/Omarchy, with an architecture
  suitable for other Linux distributions;
- operates under Wayland and X11 without assuming X11;
- embeds FreeRDP as a library and owns its context/session lifecycle;
- connects to Windows 11 and Windows Server with TLS and NLA/CredSSP;
- renders the decoded Windows desktop inside its Qt window;
- forwards correct keyboard and mouse input;
- handles authentication, certificate approval, errors, disconnect, and reconnect attempts safely;
- never launches `xfreerdp` or another external RDP client;
- never implements RDP, bitmap decompression, or security protocols from scratch.

Electron, Python/Java/.NET runtimes, Wine, Windows DLLs, MSTSC execution, screen scraping,
VNC, browser RDP, and Guacamole are out of scope.

## FreeRDP API rule

Use the stable FreeRDP 3.x installed in the development environment (3.30.0 was the
reference when specified). Before integrating an unfamiliar API, inspect the exact
installed headers and matching upstream source, current clients (especially the SDL
client), tests, signatures, ownership, callbacks, threading, and cleanup requirements.
FreeRDP 2.x examples are not authoritative. Do not guess APIs or fork FreeRDP during
Phase 1 unless an upstream defect is proven and no safe standards-compliant alternative exists.

Relevant API areas include instance/client context creation and destruction, settings,
connection/disconnection, update/graphics callbacks, input, authentication, certificate
verification, and event handles. Use upstream FreeRDP rather than copying an old client.

## Architecture

```text
Qt GUI thread
  -> ConnectionController
    -> QThread / RdpSession worker
      -> embedded FreeRDP client API
        -> Windows RDP server
```

The GUI must contain no large FreeRDP implementation and `RdpSession` must never directly
manipulate widgets. Communication crosses threads through Qt signals/slots and queued
connections. No QWidget is touched from a FreeRDP callback.

`RdpSession` owns FreeRDP initialization/context/settings, authentication and certificate
callbacks, connection/event processing, graphics callbacks, input transmission, clean
disconnect, and error reporting. Use a correctly laid-out application-specific FreeRDP
context holding session/renderer state, not Qt widget pointers.

Use strongly typed connection settings and an explicit state machine:

- Disconnected
- Connecting
- Authenticating
- CertificateVerification
- Connected
- Disconnecting
- Failed

Do not model state as unrelated booleans and do not allow concurrent connection attempts
on one session. Use RAII and clear single ownership for FreeRDP resources; avoid global
sessions and double cleanup. C++ exceptions must never cross C callbacks.

The worker efficiently waits on FreeRDP/WinPR handles or descriptors, processes network
and library events, accepts disconnect/cancellation, and exits cleanly. It must not poll
at 100% CPU or block the Qt GUI thread.

## Minimal user experience

The initial window exposes only working Phase 1 controls: Computer, User name, and Connect
(optionally Remember user name). Password storage and future-feature controls are forbidden.

Connect validates the server, starts a worker session, prompts for credentials if needed,
negotiates TLS/NLA, verifies certificates, transitions to the embedded desktop, and enables
input. During connection, Connect is disabled or becomes Cancel. Connected sessions expose
Disconnect. On disconnect/failure, return to the connection view and allow another attempt.

Closing or cancelling while connecting, authenticating, awaiting certificate approval,
connected, or disconnecting must neither hang nor crash. Never use `QThread::terminate()`.

## Server and identity parsing

Accept hostnames, FQDNs, IPv4, optional ports, and architecturally support IPv6 with correct
bracketed parsing. Default to TCP 3389. Reject invalid ports, including 0, 65536, and text.
Do not use brittle colon splitting.

Accept `DOMAIN\\username`, UPN (`username@domain.example`), and unqualified user names.
Split the first form into FreeRDP domain/user settings where required, but preserve UPNs
unchanged and do not reinterpret their suffix as an NT domain.

## Credentials and authentication

NLA and TLS are enabled by default. Do not silently enable legacy/insecure RDP security,
disable NLA, or globally disable certificate validation to resolve failures.

Authentication callbacks signal the GUI, which displays a password-mode Qt dialog and
returns the result safely to the waiting worker without invoking a dialog on that worker
or deadlocking either thread.

Passwords must never enter configuration, Qt settings, logs, history, environment variables,
command history, or command-line arguments. No `/p:` option is allowed. Keep plaintext only
in memory for the shortest practical lifetime and never log it.

## Certificate behavior

Never blindly accept certificates. For an unknown/untrusted certificate, show server,
port, subject, issuer, SHA-256 fingerprint where available, and the validation failure.
Differentiate untrusted CA, expiration, hostname mismatch, certificate change, and other
TLS failures when FreeRDP provides the information.

Phase 1 offers Cancel and Connect Anyway for the current connection only. Do not expose
permanent trust until it is securely implemented. A trusted, correctly named certificate
must not trigger a needless warning.

## Graphics and display

Use FreeRDP's current update/graphics callbacks and decoding. Render a reliable basic
desktop; advanced H.264 optimization is outside Phase 1.

`RdpDisplayWidget` displays a Qt-compatible backing framebuffer, paints dirty regions,
translates input coordinates, and handles local resize. It contains no connection logic.
Graphics callbacks update protected framebuffer state and signal damage to the GUI thread;
they never paint directly. Track damage rectangles where practical and avoid excessive
full-frame locking/copying.

Use a verified 32-bit pixel format and honor format, dimensions, and stride rather than
assuming `width * 4`. Default remote resolution is 1920x1080. A smaller local view may
scale or scroll. Phase 1 local resizing must not crash or corrupt output; dynamic server-side
resize is not required and must not be claimed.

## Input

Mouse support includes movement, left/right/middle down/up, dragging, double-click behavior,
and vertical wheel. Scaled views translate local coordinates correctly into remote coordinates.

Keyboard input uses FreeRDP/WinPR keyboard mapping and Windows-compatible scancode semantics,
not Qt-to-ASCII conversion. Send key-down and key-up with correct modifiers. Phase 1 targets
a US layout covering letters, digits, punctuation, navigation/edit keys, F1–F12, modifiers,
Caps Lock, Super/Windows, keypad, and common shortcuts. Document compositor-intercepted
shortcuts. Track/release pressed keys on focus loss, minimization, connection loss, and
disconnect where practical to prevent stuck keys.

Only send input while the remote display owns focus; typing into local controls must never
reach the remote session. Do not globally capture the mouse.

## Disconnect and errors

User disconnect requests the worker to leave its event loop, invokes required FreeRDP
disconnect/cleanup exactly once, stops the thread safely, releases the framebuffer, and
returns to the connection screen. Detect administrator/session/server/network disconnects
and keep the application usable.

Use a typed application error model distinguishing at least DNS, refusal, timeout, TLS,
certificate, authentication, protocol, server disconnect, network, and internal failures.
Present a useful message first, with optional technical FreeRDP/native error code, server,
port, and state. Technical details never contain credentials.

## Logging and version CLI

Logging categories cover app, RDP, auth, graphics, input, and network. Default output is
INFO; `OPENRDP_LOG_LEVEL=debug` enables debug output, with `--log-level` planned/supported
as implemented. Record startup, actual FreeRDP version, attempt/progress, security negotiation,
credential/certificate requests, successful resolution, disconnect, and error codes.
Normal operation must not emit uncontrolled FreeRDP TRACE output. Never log passwords.

`openrdp --version` reports actual OpenRDP, linked/runtime FreeRDP, Qt, and platform versions.
CLI supports GUI launch, an optional positional server, `/v:server`, and preferably `/u:user`,
but never a plaintext-password option.

## Build and code quality

Use modern CMake discovery for Qt6 Core/Gui/Widgets, FreeRDP/FreeRDP Client as needed,
WinPR, and threading. Do not hardcode distribution library paths. A clean pkg-config fallback
is acceptable when distro CMake packages are insufficient.

Required commands:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build
```

The executable has a predictable `build/openrdp` location. Enable `-Wall -Wextra -Wpedantic
-Wformat=2 -Wformat-security` for project code without treating third-party header warnings
as errors. `-DOPENRDP_ENABLE_SANITIZERS=ON` enables ASAN and UBSAN where supported.

Create only Phase 1 files. Clipboard, audio/microphone, printers, drives, USB, smart cards,
RemoteApp, RD Gateway, multimonitor, UDP, saved credentials, profiles, and full `.rdp`
parsing remain future work; no placeholder UI or fake implementations are allowed.

## Automated tests

Unit tests cover logic not requiring a server:

- server/port parsing for hostnames, FQDN, IPv4, alternate ports, and invalid ports;
- `DOMAIN\\username` splitting and UPN preservation;
- state transitions where practical;
- scaled mouse coordinates;
- keyboard mapping utilities;
- FreeRDP/native error translation.

All unit tests must pass. ASAN connect/use/disconnect cycling must reveal no use-after-free,
double-free, overflow, invalid access, or lifetime defects; do not suppress failures merely
to obtain a green run.

## Manual interoperability acceptance

Record evidence in `docs/phase1-testing.md` rather than assuming completion:

1. Windows 11 Pro, local account, NLA: desktop/input/disconnect work.
2. Windows Server 2025, `DOMAIN\\username`, NLA: authentication/desktop/input/disconnect work.
3. Wrong password: useful auth error, app remains usable, immediate retry works, no stale worker.
4. Connect by IP to a DNS-named certificate: appropriate mismatch warning; Cancel/Connect Anyway.
5. Trusted correctly named certificate: no unnecessary warning.
6. Notepad keyboard corpus, editing/navigation/modifier/function keys: failures recorded.
7. Mouse movement/buttons/middle/wheel/drag/resize: no coordinate offset.
8. Ten connect/disconnect cycles: no crash, hang, significant growth, duplicate threads, or stale contexts.
9. Network interruption: clean detection/error/worker exit and manual reconnect; automatic reconnect is not required.

Test Windows 11 Pro and Windows Server 2025 with NLA. Test AD on Server 2025 when an
environment is available.

## Implementation sequence (historical guidance)

The original vertical sequence was: verify environment/minimal linked Qt app; minimal UI;
real FreeRDP lifecycle/NLA/auth/disconnect; certificate GUI; graphics; mouse; keyboard;
error translation; shutdown/cycle hardening and ASAN; then the interoperability matrix.
Each working slice should be compiled, run, tested, and committed. A launched GUI alone
is not a meaningful milestone; the first milestone is a real NLA-protected Windows session.

Because working Phase 1 code now exists, apply this sequence only to missing/regressed
behavior and preserve verified functionality.

## Completion gate

Phase 1 is complete only with evidence that:

- the native Qt application launches and embeds FreeRDP without any `xfreerdp` process;
- hostname/IP, username, secure password prompt, TLS, NLA, and certificate warnings work;
- Windows 11 and Server 2025 desktops render;
- keyboard, modifiers/shortcuts (subject to compositor), mouse/buttons/wheel work;
- local and server disconnects work and failed authentication does not crash;
- the GUI remains responsive and ten connection cycles succeed;
- no password reaches disk/logs and tested ASAN workflows are clean;
- unit tests and required architecture/testing documentation exist.

Any unverified mandatory item remains incomplete. Authentication defects, crashes,
keyboard errors, and resource leaks are release blockers.

## End-of-phase report

Report exact Linux/kernel/desktop/session/compiler/Qt/FreeRDP/CMake environment; every
verified feature; Windows 11/Server 2025/AD/NLA/TLS/certificate/input results; ten-cycle,
ASAN and UBSAN results; known problems; architecture deviations; FreeRDP limitations;
final source tree; and clean-machine build commands.

## Non-negotiable rules

1. Do not implement RDP from scratch or launch `xfreerdp`.
2. Embed FreeRDP and inspect the exact current APIs rather than guessing.
3. Never block the GUI thread on RDP I/O.
4. Never accept all certificates automatically or log/store plaintext passwords.
5. Never claim success without real tests against real Windows endpoints.
6. Do not build Phase 2 features during Phase 1.
7. Prefer working vertical functionality to speculative architecture.
8. Treat crashes, auth defects, keyboard defects, and leaks as release blockers.

