# OpenRDP Client

See [docs/current-build.md](docs/current-build.md) for the consolidated build,
architecture, authentication, environment, test-status, and limitation report.

OpenRDP is a native C++20/Qt 6 Remote Desktop client which embeds FreeRDP 3.
It never launches `xfreerdp` or another RDP process. This repository currently
contains the Phase 1 core client and does not claim MSTSC parity.

## Current verification status

Built and automatically tested:

- Native Qt Widgets GUI
- Direct linkage to FreeRDP 3, FreeRDP Client 3, and WinPR 3
- Server, port, and username parsing
- Worker-thread connection/event loop
- NLA/TLS secure defaults
- In-memory credential prompt
- Per-connection normal NLA/password or Microsoft Entra web-account mode
- Per-connection certificate prompt
- FreeRDP software-GDI framebuffer display
- Mouse and scancode input paths
- Clean user-requested shutdown path

Awaiting real Windows interoperability testing (not marked complete):

- Windows 11 and Windows Server 2025 connections
- Authentication, certificate, rendering, keyboard, and mouse behavior
- Ten connection cycles and network-loss behavior
- Interactive ASAN/UBSAN session testing
- Microsoft Entra web-account authorization and token exchange

Not implemented in Phase 1: clipboard, audio, microphone, drives, printers,
smart cards, multiple monitors, dynamic resolution, RD Gateway, RemoteApp,
UDP multitransport, saved passwords, profiles, and `.rdp` files.

## Dependencies

- CMake 3.24+
- C++20 compiler
- Qt 6 Core, Gui, Widgets, and Test development packages
- FreeRDP 3, FreeRDP Client 3, and WinPR 3 development packages
- pkg-config

See [docs/building.md](docs/building.md) for distribution-oriented commands.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/openrdp
```

The GUI also accepts `openrdp server01`, `openrdp -v server01`, and
`openrdp -u 'CONTOSO\user'`. There is deliberately no password command-line
option.

Normal password/NLA authentication is the default. Select “Use a web account
to sign in to the remote computer” only for an Entra/AAD-enabled RDP target.
The installed FreeRDP library must have `WITH_AAD=ON`. OpenRDP launches a dedicated
private Chromium session so Microsoft's complete phone/passkey QR experience remains
available. It observes the browser's loopback-only debugging endpoint and intercepts
the native-client redirect internally. The temporary profile is deleted after sign-in;
authorization codes and access tokens are never copied to the clipboard or saved.

## Known Phase 1 limitations

The remote framebuffer is scaled locally when the window changes size; this is
not RDP Display Control dynamic resizing. Keyboard mapping currently targets
the specified US layout baseline. Some Super/Alt shortcuts are intercepted by
Wayland compositors before applications receive them.
