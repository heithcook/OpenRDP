# OpenRDP Current Build Report

Report date: 2026-08-28  
Project version: 0.1.0 (Phase 1 development)  
Source revision at report creation: `06c2b6c36804a8f74a0847474586303059b6009d`

## Summary

This build is a native C++20/Qt 6 Linux Remote Desktop client that embeds
FreeRDP 3.x directly. It does not launch `xfreerdp`, use an external RDP
process, or implement the RDP protocol itself.

The application currently provides the Phase 1 connection window, password/NLA
authentication, Microsoft Entra web-account authentication, certificate prompts,
basic desktop rendering, keyboard and mouse input, local framebuffer scaling,
connection error handling, and clean session shutdown.

Microsoft Entra authentication supports environments that require phone/passkey
QR authentication. An initial real sign-in using that flow succeeded on
2026-08-27. Phase 1 is still under interoperability and regression testing and
must not yet be described as fully complete.

## Build Environment

| Component | Detected value |
|---|---|
| Distribution | Omarchy 4.0.1 (Arch-based) |
| Kernel | Linux 7.1.9-arch1-2 x86_64 |
| Desktop | Hyprland |
| Display system | Wayland |
| Compiler | GCC 16.2.1 |
| CMake | 3.31.12 |
| Qt | 6.11.2 |
| FreeRDP | 3.30.0 |
| FreeRDP Client | 3.30.0 |
| WinPR | 3.30.0 |
| Chromium | 151.0.7922.173 |

The installed FreeRDP build has AAD support enabled. Its build also enables
experimental VAAPI options; if VAAPI initialization fails, FreeRDP reports the
failure and falls back to software decoding.

## Architecture

```text
Qt GUI thread
  -> MainWindow / dialogs / RdpDisplayWidget
    -> queued commands and thread-safe responses
      -> RdpSession on a worker QThread
        -> embedded FreeRDP / WinPR
          -> Windows RDP endpoint
```

The Qt GUI thread does not perform RDP network processing. `RdpSession` owns the
FreeRDP instance, context, connection lifecycle, authentication callbacks,
certificate callbacks, event processing, graphics callbacks, input queue, and
cleanup. FreeRDP callbacks do not manipulate widgets directly.

FreeRDP software GDI decodes the desktop into a BGRA framebuffer. The worker
publishes stable `QImage` snapshots to the GUI, and `RdpDisplayWidget` paints and
scales them. Mouse coordinates are translated back into remote-desktop space.

## Authentication Modes

### NLA/password

The default mode enables NLA and TLS and uses FreeRDP's `AuthenticateEx`
callback. Credentials are requested in a Qt password dialog. Passwords are not
accepted as command-line arguments, written to configuration, or intentionally
logged. Application-controlled plaintext is kept in memory only for the active
authentication operation.

Supported user-name forms include:

- `DOMAIN\\username`
- `username@domain.example`
- unqualified local user names

### Microsoft Entra web account

Selecting **Use a web account to sign in to the remote computer** enables
FreeRDP AAD security and its access-token callback path.

The QR-capable flow works as follows:

1. FreeRDP generates the Microsoft authorization request.
2. OpenRDP launches a dedicated Chromium process with a temporary profile,
   incognito mode, and a randomly allocated loopback-only DevTools port.
3. Chromium provides Microsoft's normal browser authentication experience,
   including cross-device phone/passkey QR authentication.
4. OpenRDP polls only that private Chromium instance's local target metadata.
5. OpenRDP accepts only an HTTPS callback from
   `login.microsoftonline.com/common/oauth2/nativeclient` containing an
   authorization code.
6. The callback is passed directly to the waiting RDP worker in memory.
7. FreeRDP performs the bound token exchange and continues the AAD-protected RDP
   connection.
8. OpenRDP closes the temporary Chromium process and deletes its temporary
   profile.

The callback URL is not copied to the clipboard or requested from the user.
Authorization codes and access tokens are not persisted by OpenRDP.

Chromium is currently a runtime requirement specifically for QR-capable Entra
authentication. The executable must be available as `chromium` in `PATH`.

## Certificate Handling

Certificate verification remains enabled. Unknown certificates invoke a Qt
prompt showing connection and certificate information and can be rejected or
accepted for the current connection.

The current test endpoint has also produced FreeRDP warnings that its stored host
certificate changed. Treat this as a real identity warning: confirm the expected
server certificate/fingerprint before accepting it. The application does not
silently disable certificate checking.

## Graphics and Input

Implemented Phase 1 graphics/input behavior includes:

- FreeRDP software-GDI desktop decoding
- Qt framebuffer presentation and local scaling
- damage-aware update plumbing
- mouse movement and coordinate scaling
- left, right, and middle mouse buttons
- vertical mouse wheel
- WinPR virtual-key to scancode conversion
- key-down and key-up transmission
- US keyboard letters, digits, navigation, function keys, modifiers, and OEM
  punctuation mappings
- explicit remote Shift synchronization and release on focus loss

Wayland compositors may intercept system shortcuts such as Alt+Tab or Super-key
combinations before the application receives them.

## Command Line

Supported entry points include:

```sh
openrdp
openrdp server01
openrdp -v server01
openrdp -u 'CONTOSO\user'
openrdp --version
```

There is deliberately no plaintext password command-line option.

## Build and Test

From a clean source checkout with the required development packages installed:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run the application with:

```sh
./build/openrdp
```

An optional sanitizer build is available:

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DOPENRDP_ENABLE_SANITIZERS=ON
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

## Automated Test Status

At report creation, the normal Debug build completed successfully and CTest
reported:

```text
1/1 test passed
0 tests failed
```

Covered unit logic includes server/port parsing, username/domain parsing, mouse
coordinate scaling, keyboard/OEM mappings, concrete modifier scancodes, error
translation, and strict Microsoft authorization-redirect parsing.

## Interoperability Status

The following results are based on user-observed real testing and are not a
substitute for the complete Phase 1 matrix:

| Scenario | Status | Notes |
|---|---|---|
| Real NLA/TLS connection and desktop rendering | PASS | User-observed 2026-08-27 |
| Basic letters and numbers | PASS | User-observed |
| Unshifted punctuation | PASS | After OEM-key mapping fix |
| Shifted symbols/modifier lifecycle | RETEST | Explicit Shift synchronization added after failure |
| Entra phone/passkey QR sign-in | INITIAL PASS | Successful real sign-in; repetition and failure paths remain |
| Windows 11 complete acceptance matrix | NOT COMPLETELY RECORDED | More evidence required |
| Windows Server 2025/AD complete matrix | NOT COMPLETELY RECORDED | More evidence required |
| Ten consecutive connection cycles | NOT TESTED | Required before Phase 1 completion |
| Network interruption | NOT TESTED | Manual reconnect is sufficient for Phase 1 |
| Interactive ASAN/UBSAN RDP session | NOT TESTED | Required before Phase 1 completion |

Detailed evolving results belong in [phase1-testing.md](phase1-testing.md).

## Known Limitations and Required Follow-up

- Phase 1 is not complete until every mandatory acceptance item is tested and
  recorded.
- Dynamic server-side resolution changes are not implemented; resizing scales
  the existing framebuffer locally.
- Multimonitor, clipboard, audio, microphone, drive/printer/smart-card
  redirection, RD Gateway, RemoteApp, and UDP multitransport are later phases.
- QR-capable Entra authentication currently depends on installed Chromium.
- The temporary browser profile exists under the system temporary directory
  during authentication and is deleted afterward; abrupt system termination
  should be included in cleanup testing.
- The loopback DevTools channel is restricted to `127.0.0.1` and a random port,
  but hostile processes running as the same local user remain within the local
  threat boundary and should be considered during security review.
- Cancellation, browser crash, callback timeout, invalid callback, declined
  authentication, repeated QR sign-in, and application shutdown during web auth
  need explicit regression tests.
- Certificate-change warnings for the current endpoint require administrator
  verification rather than automatic suppression.

## Source Layout

```text
src/
├── app/
│   ├── Application.cpp
│   └── Application.h
├── gui/
│   ├── CertificateDialog.cpp/.h
│   ├── CredentialDialog.cpp/.h
│   ├── MainWindow.cpp/.h
│   └── RdpDisplayWidget.cpp/.h
├── rdp/
│   ├── RdpContext.h
│   ├── RdpError.cpp/.h
│   ├── RdpInput.cpp/.h
│   ├── RdpRenderer.cpp/.h
│   ├── RdpSession.cpp/.h
│   └── RdpSettings.cpp/.h
└── main.cpp
```

## Related Documentation

- [Project specification](project-specification.md)
- [Phase 1 specification](phase1-specification.md)
- [Architecture](architecture.md)
- [Build instructions](building.md)
- [Phase 1 testing](phase1-testing.md)

