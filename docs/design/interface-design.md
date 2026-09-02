# Proposed Phase 2 interface design

Status: approved implementation direction pending iterative screenshot review  
Recorded: 2026-08-28

## Connection workspace

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│ OpenRDP                                    Search connections…   + New      │
├──────────────────┬──────────────────────────────────────────────────────────┤
│ Connections      │ Connect to                                               │
│ Recent           │ [ server.company.com__________________ ] [ Connect ]      │
│ Favorites        │                                                          │
│                  │ Recent connections                                       │
│                  │ ┌────────────────────┐  ┌────────────────────┐            │
│                  │ │ Production DC     │  │ Dev Jumpbox        │            │
│                  │ │ dc01.company.com  │  │ jump01.company.com │            │
│                  │ │ Last used today   │  │ Last used Tuesday  │            │
│ Settings         │ └────────────────────┘  └────────────────────┘            │
└──────────────────┴──────────────────────────────────────────────────────────┘
```

Quick connect is first in keyboard focus order after navigation and remains available
without creating a profile. Cards support arrow navigation, Enter to open details, a
context menu, visible focus, and list layout at narrow widths.

## Connection editor

```text
┌──────────────────┬──────────────────────────────────────────────────────────┐
│ General          │ New connection                                           │
│ Display          │ Name       [ Production DC________________ ]              │
│ Devices &        │ Computer   [ dc01.company.com_____________ ]              │
│  Resources       │ User name  [ CONTOSO\administrator________ ]              │
│ Experience       │                                                          │
│ Advanced         │ Authentication                                            │
│                  │ (•) Password / NLA   ( ) Web account                     │
│                  │                                                          │
│                  │                                  [Cancel] [Save & Connect]│
└──────────────────┴──────────────────────────────────────────────────────────┘
```

The Devices & Resources page presents Clipboard, Audio Output, Microphone, Local
Folders, Printers, and Smart Cards as titled rows with a one-sentence consequence,
status, and real control. Display presents mode, resize behavior, resolution, scale,
and a keyboard-operable topology diagram only after the corresponding support exists.

## Imported `.rdp` resource review

```text
┌─ Review shared resources ────────────────────────────────────────────────┐
│ This connection file requests access to resources on this computer.     │
│ Only share resources you trust the remote computer to access.           │
│                                                                          │
│ [✓] Clipboard       Copy text and supported content in both directions  │
│ [ ] Microphone      Send microphone audio to the remote computer        │
│ [ ] Local folders   Two requested local paths require separate review   │
│ [ ] Printers        Make selected local printers available remotely     │
│                                                                          │
│                                            [Cancel] [Continue to details]│
└──────────────────────────────────────────────────────────────────────────┘
```

The file is parsed before this surface, but no sensitive channel is configured until
the user accepts it. Invalid paths and unavailable resources are shown per item.

## Connected session

The remote framebuffer owns the main region. A restrained bottom status strip reports
secure connection, quality, clipboard, audio, microphone, folder count, printer count,
state without covering the remote desktop. **Connection > Information**
opens the complete administrator-facing negotiated capability view.

In full screen, a top-center connection bar shows server and state, followed by Pin,
Minimize, Exit Full Screen, and Disconnect. When unpinned it retracts quickly and leaves
a keyboard-focusable reveal target. Reduced-motion mode disables sliding animation.

## Required states for screenshot review

Capture the workspace empty and populated, editor General/Display/Resources, monitor
selector, settings, certificate warning, imported-resource review, connection progress,
connection error, connected session, and full-screen bar in light and dark palettes.
Each review checks spacing, long strings, focus, keyboard order, disabled explanations,
large fonts, and 100–200% display scaling.
