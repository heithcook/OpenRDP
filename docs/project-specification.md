# OpenRDP Client Project Specification

Status: **Authoritative project specification**  
Recorded: 2026-08-27  
Product name during development: **OpenRDP Client**

Phase-specific requirements: [Phase 1 specification](phase1-specification.md)

## Current project context

Phase 1 previously reached a working state. Current development is not a greenfield
Phase 1 implementation: work is focused on diagnosing and fixing the existing web
authentication (WebAuthn/web authentication) bug. Preserve working Phase 1 behavior
while fixing that defect.

## Objective and boundaries

Build a production-quality native Linux Remote Desktop client with maximum practical
behavioral and protocol compatibility with `mstsc.exe`. It must connect directly to
Windows systems through RDP and feel like a polished Linux desktop application.

- Use C++20, Qt 6, FreeRDP 3.x, WinPR, OpenSSL, libsecret, and CMake.
- Embed and link FreeRDP (`libfreerdp3`, `libfreerdp-client3`, `libwinpr3`).
- Own the FreeRDP context and connection lifecycle.
- Never implement RDP from scratch or wrap/launch `xfreerdp`.
- Isolate FreeRDP behind an RDP abstraction layer; do not leak its internals into GUI code.
- Do not copy, reverse-engineer, or redistribute Microsoft source, binaries, DLLs,
  icons, graphics, or other proprietary assets.
- Use Microsoft Open Specifications and maintained open-source implementations.
- Never fake functionality. Unsupported features must be marked **NOT IMPLEMENTED**.
- Prefer standards-compliant fixes upstream in FreeRDP; document unavoidable patches
  under `patches/freerdp/` with issue/PR/version metadata.

Priority order is security, interoperability, stability, correct RDP behavior, then
feature breadth.

## Required architecture

```text
Linux Qt GUI
  -> application/session manager
    -> RDP abstraction layer
      -> embedded FreeRDP/WinPR libraries
        -> Windows RDP server
```

Maintain strict separation among GUI, protocol integration, configuration, security,
channels/devices, profiles, and Wayland/X11 platform integration. The Qt GUI thread
must never wait on RDP network I/O. FreeRDP work belongs on a worker thread; cross-thread
updates use queued Qt signals/slots, and callbacks never manipulate widgets directly.

Use RAII, smart pointers, const correctness, scoped enums, strong types where practical,
`std::chrono`, and `std::filesystem`. Avoid owning raw pointers, mutable global state,
C-style casts, unbounded strings, and exceptions crossing C callbacks. Treat all remote
and externally supplied data as untrusted.

## Functional scope

The planned product includes:

- MSTSC-familiar General, Display, Local Resources, Experience, and Advanced workflows.
- `.rdp` import/export/CLI loading with preservation of unknown properties where possible.
- Practical MSTSC-style CLI switches: `/v:`, `/u:`, `/f`, `/w:`, `/h:`, `/multimon`,
  `/span`, `/admin`, and `/gateway:`.
- Windowed/fullscreen operation, dynamic resolution via Display Control, fixed sizing,
  smart scaling, high DPI, all/selected multimonitor layouts, and a fullscreen connection bar.
- Keyboard/mouse/Unicode/international input with configurable Windows-key behavior.
- Bidirectional clipboard, redirected drives, audio, microphone, printers, smart cards,
  and only protocol-supported USB/device redirection.
- TLS, NLA/CredSSP, NTLM, Kerberos, local/AD/UPN/domain credentials, certificate
  validation/trust/change detection, RD Gateway, and administrative sessions.
- Negotiated graphics (orders, bitmap updates, NSCodec, RDPGFX, H.264/AVC where
  available), software fallback, and extensible rendering with minimal frame copies.
- UDP multitransport with graceful TCP fallback, RemoteApp where technically possible,
  and session reconnection after temporary network loss.
- Profiles, favorites, optional history, secure credential storage, URI launching,
  desktop integration, `.rdp` association, diagnostics, and packaging.

Wayland and X11 are both supported platforms. Test GNOME, KDE Plasma, Hyprland, and
relevant X11 desktops. Document compositor-imposed limitations instead of unsafe
workarounds.

## Security invariants

- NLA is mandatory and enabled by default; do not silently downgrade security.
- Certificate verification is never disabled by default. Show server, subject, issuer,
  fingerprint, validity, and failure reason with Cancel, Connect once, and Trust options.
- Passwords are never stored as plaintext. Use Secret Service/libsecret; otherwise prompt.
- Never log passwords, tokens, smart-card PINs, private keys, credential blobs, or full
  clipboard contents. Never store smart-card PINs.
- Redirected drives are confined to explicitly selected, validated roots and protected
  against traversal.
- Microphone capture starts only after explicit user enablement and remains visibly indicated.
- Use maintained FreeRDP releases, compiler/linker hardening, ASAN/UBSAN, Valgrind,
  fuzzing where practical, and a documented dependency/CVE upgrade process.
- Follow XDG paths: config in `~/.config/openrdp/`, data in `~/.local/share/openrdp/`,
  and cache in `~/.cache/openrdp/`.

## Compatibility details

The `.rdp` implementation must cover common MSTSC fields for address/identity, display,
multimonitor, dynamic sizing, compression/input, audio/capture, clipboard/printer/port/
smart-card/drive/device/WebAuthn redirection, gateway behavior, credential prompts,
authentication/security negotiation, experience/bandwidth options, bitmap caching, and
RemoteApp. Unsupported keys load safely, are debug-logged, and survive re-save where possible.

FreeRDP errors must be translated into useful user messages while retaining technical
details. Diagnostics distinguish DNS, refusal, timeout, TLS/certificate, CredSSP,
credentials/account restrictions, NLA, Gateway, licensing, protocol negotiation,
administrator disconnect, logoff, and network loss.

Structured logs support ERROR/WARN/INFO/DEBUG/TRACE (default INFO), `--log-level`, and
`OPENRDP_LOG_LEVEL`. Exported diagnostic bundles may contain versions, platform/session
environment, capabilities, timeline, errors, and transport/redirection status, but no secrets.

## Development phases

1. **Basic client:** Qt UI, server/user/password prompt, embedded FreeRDP, TLS/NLA,
   graphics, keyboard, mouse, resizing, errors, and clean disconnect.
2. **MSTSC-style UI:** tabs, profiles, `.rdp` files, fullscreen, and history.
3. **Display parity:** dynamic resolution, smart sizing, multimonitor/selection, high DPI,
   and fullscreen connection bar on Wayland and X11.
4. **Resource redirection:** clipboard, audio, microphone, drives, printers, smart cards.
5. **Enterprise authentication:** Kerberos/AD, RD Gateway and separate credentials,
   admin sessions, and certificate management.
6. **Advanced RDP:** UDP multitransport, RemoteApp, reconnection, advanced graphics,
   and device redirection.
7. **Production hardening:** sanitizers, Valgrind, fuzzing, long sessions, interruption/
   connection cycling, packaging, CI, and security review.

Do not advance based on checkboxes or unverified implementation. Each phase exits only
after its stated interoperability criteria pass.

## Phase 1 acceptance baseline

The basic client must directly create/configure a FreeRDP context, enforce TLS/NLA,
prompt securely, connect, render inside Qt, forward input, resize appropriately, report
useful failures, and disconnect cleanly without invoking `xfreerdp`.

Acceptance tests:

1. Windows 11 Pro, NLA, local account: PASS.
2. Windows Server 2025, NLA, Active Directory account: PASS.
3. Wrong password gives a clear authentication error: PASS.
4. Untrusted certificate gives a warning: PASS.
5. Keyboard and mouse operate correctly: PASS.
6. 1920x1080 renders correctly: PASS.
7. Client resizing is handled correctly: PASS.
8. Ten consecutive disconnect/reconnect cycles have no crash or substantial leak: PASS.

The current WebAuthn/web-auth defect must be fixed without regressing these behaviors.

## Testing and evidence

Tests are part of every feature, not deferred work. Unit coverage includes `.rdp` parsing/
writing, profiles, CLI, monitor geometry, keyboard translation, certificate fingerprints,
configuration, drive-path validation, and credential logic.

Maintain a documented interoperability lab covering Windows 10/11 and Windows Server
2016/2019/2022/2025, local and AD accounts, NLA/TLS/certificates/Kerberos/Gateway,
displays, redirected resources, RemoteApp, admin sessions, and reconnection.

Use MSTSC as the behavioral reference through differential testing and maintain
`docs/mstsc-parity-matrix.md`. Never mark a capability PASS without an interoperability
test. Wireshark, FreeRDP TRACE logs, Windows Event Viewer, RD Gateway logs, and
TerminalServices logs may be used for protocol debugging, never to defeat credential protection.

Performance goals include low input latency, smooth 1080p and 1440p, usable 4K and
multimonitor sessions, negotiated video optimization, no continuous growth over eight
hours, and hundreds of repeated connect/disconnect cycles without significant growth.

## Documentation and delivery

- Document each FreeRDP integration subsystem, represented protocol/channel, registered
  callbacks, controlling settings, and threading behavior.
- Maintain ADRs in `docs/decisions/` for FreeRDP, Qt 6, Wayland-first behavior,
  libsecret, `.rdp` compatibility, and later consequential choices.
- Use CMake with debug/release/ASAN presets; standard configure/build/ctest commands work.
- CI covers Ubuntu and Fedora, warnings, tests, ASAN, UBSAN, clang-tidy, and useful static analysis.
- Package DEB, RPM, AppImage, and eventually Flatpak for current Ubuntu/Debian/Fedora/
  Arch/Mint targets, with an explicit FreeRDP security-update strategy.
- Provide an original icon and desktop file; do not use Microsoft branding as the product name.
- Use small descriptive Git commits and feature/bugfix branches; never commit generated builds.

## Compatibility claims

Published releases use four evidence-based levels:

1. Core RDP connectivity.
2. Typical MSTSC workstation functionality.
3. Enterprise functionality (Gateway, AD/Kerberos, device redirection, multimonitor, RemoteApp).
4. Extensive tested parity across supported Windows/RDS environments.

Never claim “100% MSTSC compatible.” Explicitly document proprietary, undocumented,
OS-specific, or unavailable limitations. The engineering target is maximum practical
MSTSC behavioral and protocol compatibility on Linux.

## Authoritative references

Use the latest revisions of Microsoft Open Specifications, beginning with MS-RDPBCGR,
and including MS-RDPEDYC, MS-RDPECLIP, MS-RDPEA, MS-RDPEAI, MS-RDPEDISP, MS-RDPEGFX,
MS-RDPEI, MS-RDPDR and related device specifications, MS-RDPESC, MS-RDPEUSB,
MS-RDPERP, MS-RDPEMT, MS-RDPEUDP, and MS-TSGU. Do not rely solely on informal blogs.

## Feature workflow

For every feature: research FreeRDP APIs and the Microsoft protocol, establish MSTSC
behavior, update design, write tests, implement, build, run tests and interoperability
checks, update the parity matrix, and commit the completed unit of work.
