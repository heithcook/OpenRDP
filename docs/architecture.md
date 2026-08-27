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
