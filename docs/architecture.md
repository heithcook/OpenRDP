# Phase 1 architecture

`MainWindow` and all dialogs/widgets live on the Qt GUI thread. `RdpSession`
owns the `freerdp` instance and extended `OpenRdpContext`, and its blocking
connect plus handle-based event loop run on one `QThread`. The loop waits on
FreeRDP/WinPR handles with a bounded timeout; it does not busy-poll.

FreeRDP callbacks obtain the owning session through the application context at
offset zero. No context contains widget pointers. Authentication and
certificate callbacks emit queued GUI requests and wait on a condition. GUI
responses wake the worker directly through small thread-safe response methods.
Cancellation wakes the same condition, preventing shutdown hangs while a
dialog decision is pending.

Authentication is an explicit per-connection mode. `NlaPassword` enables TLS
and NLA and uses `AuthenticateEx`. `EntraWebAccount` disables the competing
NLA/TLS security selectors, enables FreeRDP AAD security, and uses
`GetAccessToken`. The GUI launches a dedicated Chromium process with an ephemeral
profile and a loopback-only DevTools endpoint. This preserves Chromium's native
phone/passkey QR support. OpenRDP polls only that private browser's target metadata,
accepts only the expected HTTPS Microsoft native-client callback, extracts its
one-time code in memory, shuts down Chromium, removes the temporary profile, and
delegates the token exchange to FreeRDP. No token is copied, emitted through a Qt
signal, or persisted by OpenRDP.

After connection, FreeRDP software GDI decodes protocol graphics into a BGRA32
primary buffer. `EndPaint` copies a stable `QImage` snapshot and emits it to the
GUI. The display widget scales that image during painting. This correctness-
first copy can later be replaced with a carefully synchronized shared surface.

Mouse and keyboard handlers never call FreeRDP from the GUI thread. They append
events to a mutex-protected queue, which the RDP event loop drains. Qt keys are
mapped to Windows virtual keys and then translated with WinPR to RDP scancodes.

Ownership is singular: `RdpSession` creates and frees the FreeRDP instance and
context, while `RdpRenderer` initializes/frees GDI. Cleanup is centralized and
idempotent. Exceptions never leave C callbacks.
