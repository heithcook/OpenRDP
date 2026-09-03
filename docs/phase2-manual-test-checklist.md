# OpenRDP Phase 2 manual validation checklist

Automated tests validate parsing, persistence, channel configuration, input mapping,
monitor translation, resource validation, and sanitizer-clean execution. The items below
require a real desktop, Windows host, peripheral, or sustained session and must not be
recorded as PASS until observed.

## Baseline and connection lifecycle

- Connect to Windows 11 Pro and Windows Server 2025 using NLA and TLS.
- Reject, then explicitly accept, an unknown certificate; verify authentication failures
  are understandable and no trust decision is silently made.
- Verify keyboard, mouse, disconnect, reconnect, and ten consecutive connect cycles.

## Profiles and RDP files

- Create, edit, save, reopen, delete, favorite, and connect from a profile.
- Verify recent connection ordering, individual removal, and clear-all.
- Open UTF-8 and UTF-16 `.rdp` files from File > Open and from `openrdp file.rdp`.
- Import a file requesting clipboard, microphone, drives, and printers; verify the warning
  and that every local resource remains disabled until explicitly selected.
- Save an `.rdp` file, open it in MSTSC, and confirm expected display/audio/resource values.
- Confirm neither profile nor `.rdp` output contains a password or credential blob.

## Display and session UI

- Resize a single-monitor window repeatedly; verify the Windows desktop changes resolution
  after the debounce and does not merely stretch.
- Test fixed/scaled behavior where exposed, maximize/restore, monitor movement, and DPI changes.
- Enter/exit full screen with Ctrl+Alt+Enter; verify geometry restoration.
- Unpin the connection bar, verify it hides, reveal it at the top edge, then test minimize,
  exit-full-screen, disconnect, and Connection Information.
- Test two horizontal, two vertical, three, and four monitors where available, including
  portrait, negative coordinates, non-leftmost primary, mixed resolution, and mixed DPI.
- In Windows Display Settings, confirm actual remote monitor topology and per-monitor maximize.
- **RETEST (2026-09-03, Wayland, two monitors):** the first run negotiated the combined
  topology but rendered both desktops inside one local window. A per-screen fullscreen
  presentation layer with normalized framebuffer crops and combined input coordinates is now
  implemented; verify placement, pixels, pointer alignment, keyboard focus, and fullscreen exit.

## Clipboard

- Copy Unicode/plain text Linux to Windows and Windows to Linux under GNOME Wayland,
  KDE Plasma Wayland, and X11. Include emoji, multiline text, and rapid alternating copies.
- Verify no clipboard echo loop or UI stall. HTML, image, and file clipboard transfer are
  not claimed by Phase 2 unless separately implemented and validated.

## Audio and microphone

- Test system sounds, browser/video, media player, and notifications through the default
  PipeWire/PulseAudio-compatible output and any configured named output.
- Verify remote audio and disabled modes.
- Confirm microphone defaults off, activation is visible, remote recording works, no audio
  is stored/logged, and server-policy blocking is distinguishable from local failure.

## Folders and printers

- Redirect one and multiple explicit folders with spaces and Unicode names; test enumerate,
  read, create, rename, copy large files, cancel, delete, and disconnect mid-transfer.
- Confirm root, whole home, symlink, missing, and duplicate-name paths are rejected.
- Verify selected CUPS printers appear in Windows. Test a Windows test page, text, PDF,
  image, multipage job, cancellation, and disconnect with a queued job.

## Presentation, accessibility, and endurance

- Review every screen in light and dark themes at 100%, 125%, 150%, 175%, and 200%.
- Complete connection/profile/settings workflows using only Tab, Shift+Tab, arrows, Space,
  Enter, Escape, shortcuts, and menus; check visible focus and screen-reader labels.
- Review small-window reflow, long server/user/path strings, empty/loading/error/disabled states.
- Run combined channel cases: clipboard+audio, clipboard+folders, audio+microphone,
  folders+printers, multimonitor+clipboard+audio, and imported RDP+redirections.
- Run an eight-hour session and inspect memory/CPU responsiveness afterward.

Smart-card redirection is intentionally deferred to Phase 3.
