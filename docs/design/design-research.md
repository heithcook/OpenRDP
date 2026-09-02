# OpenRDP Phase 2 design research

Status: Phase 2A foundation  
Recorded: 2026-08-28

## Product and user context

OpenRDP is a native Linux desktop tool for administrators who often know the target
host already, but also need repeatable profiles and explicit control over local
resources. The interface therefore needs two speeds: immediate keyboard-friendly
quick connect, and a structured editor for saved connections. Security decisions
must remain visible without making routine connections cumbersome.

This research informs an original Qt Widgets interface. It does not reproduce the
visual assets or screen layouts of Microsoft clients, GNOME Connections, Remmina,
or KDE applications.

## Primary guidance reviewed

- Qt 6.11 accessibility guidance: use layouts and system fonts, preserve keyboard
  navigation, respect the platform palette, avoid color-only meaning, and expose
  useful accessibility metadata for custom widgets.
- Qt 6.11 high-DPI guidance: operate in device-independent coordinates, use Qt's
  per-screen device pixel ratio, and prefer vectors/theme icons over raster assets.
- KDE HIG: be simple by default and powerful when needed; use a clear main content
  area, contextual actions, consistent spacing, and larger separation between
  groups than between related controls.
- GNOME HIG: keep each view focused, avoid deep navigation, prefer in-window views
  over stacks of secondary windows, use progressive disclosure, and test the whole
  application with keyboard, large text, high contrast, and a screen reader.
- W3C WCAG 2.2 supporting guidance: keep pointer targets at least 24 by 24 logical
  pixels or sufficiently separated, make keyboard focus conspicuous, and maintain
  contrast for text, controls, and state indicators.

References:

- <https://doc.qt.io/qt-6/accessible.html>
- <https://doc.qt.io/qt-6/highdpi.html>
- <https://develop.kde.org/hig/>
- <https://develop.kde.org/hig/layout_and_nav/>
- <https://developer.gnome.org/hig/principles.html>
- <https://developer.gnome.org/hig/guidelines/navigation.html>
- <https://developer.gnome.org/hig/guidelines/keyboard.html>
- <https://developer.gnome.org/hig/guidelines/accessibility.html>
- <https://www.w3.org/WAI/WCAG22/Understanding/target-size-minimum.html>
- <https://www.w3.org/WAI/WCAG22/Understanding/focus-appearance.html>

## Competitive workflow observations

### Microsoft MSTSC

MSTSC establishes familiar groupings—General, Display, Local Resources,
Experience, and Advanced—and makes a typed host the shortest path to connect.
Its strengths are compactness and predictable terminology. Its dense option sheets
and limited explanation of security-sensitive redirection should not be copied.

### Microsoft Windows App / Remote Desktop

The newer Microsoft clients emphasize a searchable library of saved resources,
large recognizable connection items, and separation between library management
and an active session. OpenRDP should retain that clarity while using Linux-native
controls and an original, more information-efficient visual language.

### GNOME Connections

Connections demonstrates a low-friction host-first workflow and restrained
presentation. It is intentionally narrower than OpenRDP's administrator scope;
OpenRDP needs progressive disclosure rather than hiding required display and
device policy.

### Remmina and FreeRDP graphical clients

Remmina demonstrates the utility of searchable saved connections and an in-session
toolbar, but also the cost of exposing a large undifferentiated option surface.
FreeRDP sample clients are valuable protocol references, not finished product UI
references. OpenRDP will organize options by user intent and show only implemented
capabilities.

### KDE applications

KDE's common sidebar/content structure suits a technical library with multiple
top-level destinations. Native theme icons, system palette roles, standard dialogs,
and strong keyboard behavior help the application fit Plasma without preventing it
from fitting GNOME, Hyprland, or other desktops.

## Applied design lessons

### Visual hierarchy and navigation

Use one primary window with a shallow sidebar: Connections, Recent, Favorites, and
Settings. The selected destination owns the content area. Profile editing is a
single details view with General, Display, Devices & Resources, Experience, and
Advanced sections; it is not a chain of modal dialogs. Session content replaces the
workspace while connected, with global session commands kept in menus and a compact
status surface.

### Spacing and typography

Use a small documented spacing scale and system font metrics. Related label/control
pairs stay close; sections have visibly larger separation. Page and section titles
use relative weight/scale from the system font rather than fixed typefaces. Text must
wrap and layouts must grow when desktop font scaling is increased.

### Buttons, fields, and dialogs

One visually clear primary action is permitted per decision surface. Destructive
actions use explicit verbs such as **Disconnect** or **Remove profile**. Labels name
the outcome, not the implementation. Inline validation is placed beside the relevant
field; dialogs are reserved for credentials, certificates, untrusted `.rdp` resource
review, destructive confirmation, and file selection.

### Settings and progressive disclosure

Defaults cover the common safe case. Advanced protocol settings are separate from
daily controls. Every visible option must map to implemented behavior. Unsupported,
server-blocked, and inactive states are distinct and use text as well as an icon.

### Status and errors

Connection progress uses real lifecycle states without invented percentages. Errors
lead with a plain-language summary and recovery action; raw FreeRDP details are behind
**Technical Details**. Active microphone, folders, printers, and clipboard
are inspectable throughout a session.

### Accessibility and keyboard navigation

All workflows must work with Tab, Shift+Tab, arrows, Space, Enter, Escape, menus, and
standard shortcuts. Labels are buddies for form controls; dynamic/custom widgets have
accessible names, descriptions, roles, and state. Focus order follows reading order.
Focus remains visibly distinct in light, dark, and high-contrast palettes. No state is
communicated by color alone.

### Light/dark mode and desktop consistency

Use `QPalette`, `QStyle`, and freedesktop theme icons. Avoid hard-coded foreground and
surface colors. Semantic custom roles are derived from the active palette and checked
for contrast. Theme changes are applied live where Qt exposes them.

### High DPI and responsive sizing

All layout geometry is in Qt logical pixels. SVG/theme icons remain sharp. Screen and
monitor topology retains both logical geometry and device-pixel ratio so protocol
geometry is not confused with widget geometry. At narrower widths the sidebar can
collapse to icons plus accessible labels, descriptions wrap, editors scroll, and the
primary action remains visible.

## Research-derived acceptance checks

- Complete quick connect and profile creation with keyboard only.
- Inspect every page at 100%, 125%, 150%, 175%, and 200% scaling.
- Inspect light, dark, and high-contrast palettes without custom-color artifacts.
- Test screen-reader names for navigation, profile cards, monitor selector, resource
  status, and toolbar buttons.
- Test empty, loading, connecting, error, disabled, hover, focus, and long-content states.
- Ensure sensitive resource requests name the local exposure and allow per-resource
  rejection before connecting.
