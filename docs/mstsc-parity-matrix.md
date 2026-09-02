# OpenRDP MSTSC parity matrix

Last updated: 2026-08-28  
Rule: PASS requires a recorded interoperability test against Windows. Compiled code or
unit coverage alone is not PASS.

| Feature | MSTSC | OpenRDP implementation | Result | Notes |
| --- | --- | --- | --- | --- |
| Core NLA/TLS connection | Yes | Phase 1 | PASS | Phase 1 accepted 2026-08-28 |
| Clipboard text | Yes | Bidirectional Unicode `cliprdr` | IMPLEMENTED — NOT VALIDATED | Codec/security tests pass; Wayland/X11/Windows tests pending |
| Clipboard images | Yes | Not implemented | NOT TESTED | Investigate after text |
| Clipboard files | Yes | Not implemented | NOT TESTED | Reliability tracked separately |
| Audio output | Yes | FreeRDP `rdpsnd` Pulse/PipeWire configuration | IMPLEMENTED — NOT VALIDATED | Local/remote/disabled modes; live media test pending |
| Microphone | Yes | FreeRDP `audin` Pulse/PipeWire configuration | IMPLEMENTED — NOT VALIDATED | Defaults off; privacy indicator; live capture test pending |
| Drives/folders | Yes | Explicit FreeRDP RDPDR folder devices | IMPLEMENTED — NOT VALIDATED | Canonical-root/security tests pass; Windows file I/O pending |
| Printers | Yes | Async CUPS discovery and FreeRDP RDPDR devices | IMPLEMENTED — NOT VALIDATED | Selection/argument tests pass; Windows print jobs pending |
| Smart cards | Yes | Deferred to Phase 3 | OUT OF PHASE 2 | PC/SC hardware-dependent work removed from Phase 2 |
| Single monitor | Yes | Phase 1 framebuffer | PASS | Basic rendering accepted in Phase 1 |
| Multiple monitors | Yes | Qt discovery and FreeRDP monitor definitions | IMPLEMENTED — NOT VALIDATED | Mixed-topology unit tests pass; Windows/hardware test pending |
| Dynamic resolution | Yes | Display Control DVC implementation | IMPLEMENTED — NOT VALIDATED | Debounce/constraints tested; Windows test pending |
| Full screen | Yes | Implemented with connection bar | IMPLEMENTED — NOT VALIDATED | Ctrl+Alt+Enter, restore, pin/auto-hide, minimize/exit/disconnect |
| `.rdp` parse/write core | Yes | Parser/writer and profile mapping | IMPLEMENTED — NOT VALIDATED | Unit/security round-trips pass; no MSTSC test yet |
| `.rdp` open from CLI | Yes | Implemented | IMPLEMENTED — NOT VALIDATED | Bounded UTF-8/UTF-16 parser and resource warning |
| `.rdp` save from UI | Yes | Implemented | IMPLEMENTED — NOT VALIDATED | UTF-16LE, unknown properties preserved, passwords removed |
| Connection profiles | Yes | Versioned model and JSON persistence | PARTIAL | Persistence passes unit tests; library/editor UI pending |
| Recent connections | Yes | Phase 1 history | PARTIAL | Metadata/removal pending |
