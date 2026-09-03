# Phase 2 development progress

Last updated: 2026-08-28

## Baseline

- Phase 1 accepted complete by the user on 2026-08-28 after interactive testing.
- Existing Debug source configured, built, and passed CTest before Phase 2 edits.
- Installed environment remains Qt 6.11.2 and FreeRDP/WinPR 3.30.0.
- Architecture review found no reason to rewrite Phase 1 authentication, TLS,
  certificates, rendering, input, or worker-thread lifecycle.

The `cmake` shell shim currently has no selected mise version. The working tool is
`/home/hcook/.local/share/mise/installs/cmake/3.31.12/cmake-3.31.12-linux-x86_64/bin/cmake`.

## Phase 2A accomplished

- Created `docs/design/design-research.md` from current Qt, KDE, GNOME, and W3C
  primary guidance plus competitive workflow analysis.
- Created `docs/design/design-system.md` defining spacing, typography, palette,
  components, accessibility, navigation, dialogs, status, and responsive behavior.
- Created `docs/design/interface-design.md` with concrete workspace, editor,
  resource-review, and connected-session designs.
- Added the initial evidence-based MSTSC parity matrix.

## Phase 2 implementation status

- Added a versioned `ConnectionProfile` model with conservative resource defaults.
- Added explicit capability state separating requested, locally available,
  client-allowed, server-negotiated, and active conditions.
- Added ordered `RdpProperty`, `RdpFile`, `RdpFileParser`, and `RdpFileWriter` types.
- Parser supports bounded UTF-8 and UTF-16LE input and preserves unknown types,
  duplicates, Unicode, spaces, and colons in values.
- Parser rejects oversize files/lines, malformed properties, invalid integer values,
  invalid encoding, and unexpected nulls.
- Writer emits UTF-16LE with BOM and CRLF and has no password/decryption facility.
- Unit tests cover round trip, preservation, UTF-16 output, hostile input, lookup
  semantics, and safe defaults.
- Added `.rdp`-to-profile mapping for address, identity, window/full-screen mode,
  dimensions, multi-monitor selection, resize behavior, and audio mode.
- Added security classification for requested clipboard, microphone, local drives,
  and printers. Imported profiles do not activate those resources before
  the future review UI applies an explicit user decision.
- Password-like `.rdp` properties are removed from the preserved import and all export
  paths; unrelated unknown properties remain round-trippable.
- Added atomic, versioned JSON profile persistence under the XDG data location, with
  legacy schema migration, future-version rejection, safe identifiers, removal, and
  no credential/token fields.
- Removed the existing `-v` server/automatic version-option collision; `--version`
  remains handled by the application entry point.
- Smart-card redirection was removed from Phase 2 scope by user direction on
  2026-08-28 and recorded for Phase 3. Phase 2 exposes no smart-card control.
- Added File Open and Save Connection As actions, positional `openrdp file.rdp`
  loading, security warnings for requested local resources, and safe import with all
  such resources disabled. Unknown non-secret properties survive subsequent saves.
- Began Phase 2C with View > Full Screen, `Ctrl+Alt+Enter`, and restoration of the
  pre-fullscreen window geometry. The connection bar and interoperability validation
  remain outstanding, so fullscreen is not yet marked PASS.
- Implemented FreeRDP 3.30 Display Control dynamic resizing: the `disp` DVC is loaded
  before connection, channel lifetime is tracked through FreeRDP PubSub events, server
  capability activation gates requests, Qt resize events debounce for 220 ms, dimensions
  clamp to protocol limits, and `SendMonitorLayout` runs only on the RDP worker thread.
  Automated constraint tests pass; Windows interoperability remains NOT VALIDATED.
- Implemented true pre-connection multi-monitor configuration using Qt screen discovery
  and FreeRDP 3.30's `freerdp_settings_set_monitor_def_array_sorted` API. The model carries
  pixel geometry, physical size, scale, orientation, primary identity, and stable screen
  name. Automated tests cover three monitors, negative coordinates, portrait orientation,
  mixed scale, duplicate IDs, invalid primary selection, and protocol limits. The working
  **Use all monitors** control disables single-monitor dynamic resizing for that session.
  Windows topology recognition and physical mixed-DPI behavior remain NOT VALIDATED.
- Implemented modular bidirectional Unicode text clipboard redirection through FreeRDP
  3.30 `cliprdr`. Qt clipboard access stays on the GUI thread; protocol callbacks and
  channel sends stay on the RDP worker. The manager advertises `CF_UNICODETEXT`, handles
  server format negotiation/data requests, normalizes CRLF, bounds remote payloads to
  16 MiB, rejects malformed UTF-16 payloads, and suppresses remote-to-local echo. A
  working **Share clipboard** control defaults on for new connections, while imported
  `.rdp` files keep clipboard off until explicitly enabled. Unicode/emoji/CRLF and hostile
  payload automated tests pass. Wayland, X11, and Windows interoperability remain NOT
  VALIDATED; HTML/images/files remain outstanding and are tracked separately.
- Implemented modular audio output and microphone configuration using FreeRDP 3.30's
  `rdpsnd` and `audin` channels. The installed FreeRDP build has Pulse and ALSA backends
  but no native PipeWire backend, so OpenRDP selects `sys:pulse`, which runs over the
  installed `pipewire-pulse` compatibility service. Output supports local playback,
  remote playback, or disabled mode plus system-default/custom device names. Microphone
  defaults off, requires explicit enablement, supports system-default/custom input, and
  shows an accessible textual **Microphone shared** indicator for the connected session.
  Channel argument/default/device automated tests pass; sound quality, device switching,
  server policy, and live capture remain NOT VALIDATED.
- Implemented explicit local-folder redirection through FreeRDP RDPDR device entries.
  Users select individual directories and assign a Windows-visible name; OpenRDP never
  enables redirect-all-drives, hotplug, root, or automatic home-drive options. Roots are
  required to be existing, readable, absolute, non-root, non-symlink directories and are
  stored/passed canonically. Remote names are bounded and sanitized, case-insensitive
  duplicates are rejected, and paths are revalidated immediately before every connection
  to handle deletion/mount changes. The installed FreeRDP 3.30 includes the upstream fixes
  for the 3.28/3.24 drive traversal advisories. No read-only checkbox is exposed because
  reliable client-layer enforcement has not been established. Validation, symlink, missing
  path, root, duplicate-name, and device-argument tests pass; Windows enumeration and file
  operations remain NOT VALIDATED.
- Implemented printer redirection using asynchronous CUPS 2.4 destination discovery and
  FreeRDP's existing RDPDR printer devices. Discovery runs outside the GUI thread, retains
  CUPS instance/default identity, validates names, and presents an explicit per-printer
  checklist behind a default-off **Share selected printers** control. OpenRDP passes an
  empty driver field so FreeRDP/CUPS performs normal negotiation and never implements a
  custom printing protocol. Profile persistence stores enablement and selected destination
  names. Discovery validation, default flag, device argument, unsafe-name, and duplicate
  tests pass; Windows enumeration, documents, cancellation, and disconnect behavior remain
  NOT VALIDATED.

## Current state and next tasks

The normal Debug build and all 24 automated tests pass. A separate ASAN/UBSAN build
also passes all 24 tests with no sanitizer findings. LeakSanitizer cannot run under the
ptrace-based execution environment, so leak checking remains a manual/native-run item.
The application cannot be GUI-smoke-tested in this headless runner because the installed
GTK platform integration requires a display.

Profiles now have create/update/delete and selector UI, resource/device selections round
trip through the versioned store, and recent history has timestamps, profile association,
individual removal, and clear-all. Full screen now includes an original theme-icon session
bar with server status, pin/auto-hide, minimize, exit-full-screen, and disconnect controls.
Connection Information reports the configured session/resource state without secrets.

All protocol/device features remain **IMPLEMENTED — NOT VALIDATED** until the real Windows,
Wayland/X11, monitor, audio, printer, and long-session checks in
`docs/phase2-manual-test-checklist.md` are completed. HTML/image/file clipboard transfer is
not claimed; Phase 2's mandatory bidirectional text implementation is present. Smart cards
remain deferred to Phase 3 by user direction.

Multi-monitor presentation currently **FAILS** physical Wayland validation. On 2026-09-03,
the Windows session negotiated a combined two-monitor remote framebuffer, but OpenRDP
displayed both remote desktops inside one local window on a single monitor. The protocol
topology path is working; the missing presentation layer needs one fullscreen top-level
surface per selected physical display, with each surface painting its framebuffer crop and
translating pointer coordinates back into the combined remote desktop.

## SESSION CHECKPOINT

Accomplished the Phase 1-to-Phase 2 transition and design foundation; secure `.rdp`
import/export; profile and recent persistence/UI; dynamic resolution; real monitor topology;
bidirectional Unicode text clipboard; audio output/microphone channel configuration; explicit
folder redirection; CUPS printer redirection; full screen/session controls; status information;
and automated/hardening tests. Primary changes are under `src/channels`, `src/display`,
`src/profiles`, `src/gui`, `src/rdp`, `tests`, and `docs`. Normal and ASAN/UBSAN CTest runs
pass 24/24. Physical interoperability, desktop-matrix, accessibility visual review, channel
combinations, and endurance remain for the user's final validation checklist; do not promote
those rows to PASS without evidence.
