# OpenRDP design system

Status: Phase 2A version 1  
Recorded: 2026-08-28

## Principles

OpenRDP is calm, precise, native, and security-explicit. It uses system controls and
palette roles as the baseline, adds only a small set of reusable semantic components,
and favors information hierarchy over decoration. One fast connection path is always
available. Powerful settings use progressive disclosure.

## Foundations

### Spacing scale

| Token | Logical pixels | Use |
| --- | ---: | --- |
| `space-1` | 4 | icon/text and compact internal gaps |
| `space-2` | 8 | related controls |
| `space-3` | 12 | control padding and compact groups |
| `space-4` | 16 | standard page inset and groups |
| `space-6` | 24 | section separation |
| `space-8` | 32 | major regions and empty-state breathing room |

Values are logical pixels and may be adjusted by platform style metrics. New arbitrary
spacing values require a documented component-specific reason.

### Corner radius and borders

Use the platform style for standard controls. Custom cards use a small radius derived
from the style, with 6 logical pixels as the fallback. Do not nest multiple rounded
surfaces. One-pixel palette-derived borders separate cards and tool surfaces; selected
or focused states use a stronger semantic border/focus ring rather than shadows.

### Control size

Standard controls follow `QStyle` metrics with a 32 logical-pixel preferred height and
24 logical-pixel absolute interactive minimum. Primary actions may use 36 logical pixels.
Do not force a fixed height when translated or large-font text needs more room.

### Typography

Use the system UI font only.

| Role | Treatment |
| --- | --- |
| Application/page title | 1.35× system size, semibold |
| Section title | 1.1× system size, semibold |
| Body/control | system size and weight |
| Secondary | system size, palette secondary text role |
| Caption/status | 0.9× where legible; never below platform minimum |
| Error/warning | body size and medium weight; text plus icon |

### Icons

Resolve freedesktop theme icons through `QIcon::fromTheme`, with original OpenRDP SVG
fallbacks only when no semantic theme icon exists. Menu/inline icons use the platform
small metric; navigation and connection cards use medium metrics. Icons never replace
an accessible name and emoji are not production controls.

### Color roles and surfaces

Use active `QPalette` roles: Window for the app shell, Base for editable/content surfaces,
AlternateBase for subtle grouping, Button for controls, Highlight for selection, and
Link for links. Derive secondary text and borders from palette roles. Semantic error,
warning, and success colors must maintain contrast and always accompany an icon and text.
No component assumes a white or black background.

## Component states

- **Focus:** visible platform focus plus a clearly contrasting outline for custom items.
- **Hover:** subtle palette-derived surface change; never the only discoverability cue.
- **Selection:** Highlight/HighlightedText plus persistent shape or marker.
- **Disabled:** platform disabled palette, retained readable label, and explanation when
  capability/policy is the cause.
- **Error:** error icon, concise text, field association, and recovery instruction.
- **Warning:** warning icon and explicit consequence; resource-sharing warnings list items.
- **Success:** confirmation icon/text used sparingly for completed saves and connections.
- **Active device:** icon, device name/count, and the word **On**, **Shared**, or **Active**.

## Layout patterns

### Application shell

A 200–240 logical-pixel navigation sidebar sits beside a flexible content region. It
collapses below the documented minimum width. Global Settings remains at the bottom;
session-specific actions never appear as library navigation.

### Content page

Page header contains title, optional concise description, and no more than one primary
action. Search/filter follows. Content is a list/grid chosen by available width. Empty
states state what is missing and provide the next action.

### Forms and editors

Use a scrolling content column with a readable maximum width. Labels remain associated
with controls. Section navigation has one level only: General, Display, Devices &
Resources, Experience, Advanced. A persistent footer contains Cancel and Save/Connect
when edits can be committed.

### Toolbars

Window toolbars contain context-relevant actions. The full-screen connection bar contains
server, connection state, pin, minimize, exit full screen, and disconnect. It is compact,
keyboard accessible, uses original composition and theme icons, and auto-hides only when
unpinned.

### Dialogs

Dialogs have a descriptive title, short consequence-oriented body, optional expandable
technical details, and a standard trailing button row. Default focus and Escape behavior
are explicit. Destructive or exposure-granting actions use precise labels.

## Navigation and shortcuts

- `Ctrl+N`: new connection
- `Ctrl+O`: open `.rdp`
- `Ctrl+S`: save profile/current connection
- `Ctrl+Shift+S`: save connection as `.rdp`
- `Ctrl+,`: settings
- `Ctrl+L`: focus quick-connect address
- `Ctrl+Alt+Enter`: enter/leave full screen
- `Ctrl+W`: close current editor/session view where safe
- `F10`: application menu; `Shift+F10`: focused-item context menu
- `Escape`: dismiss transient UI or leave an uncommitted secondary view after confirmation

Tab order follows visible reading order. Sidebar/list movement uses arrow keys; Enter opens
or connects as labeled; Space toggles checkable resource settings.

## Responsive and high-DPI policy

Primary layout uses Qt layouts and size policies, never fixed screen coordinates. Minimum
window target is 760×520 logical pixels, subject to prototype validation. At small widths,
secondary descriptions can hide after their accessible descriptions are retained, and
editors scroll rather than clipping actions. Rasterized session pixels and Qt UI logical
pixels remain separate coordinate systems.

## Security presentation

Clipboard, microphone, folders, and printers are named local exposures. Smart-card
redirection is deferred to Phase 3 and has no Phase 2 control.
Imported `.rdp` requests enter a review surface and never silently change application
defaults. The review shows each requested resource independently and defaults sensitive
resources according to OpenRDP policy. Server-policy rejection, local unavailability,
unsupported capability, configured-off, negotiated, and active are distinct statuses.

## Implementation policy

Reusable metrics and semantic roles belong in one UI foundation module; per-widget style
sheets with literal colors or arbitrary geometry are prohibited. Standard Qt widgets are
preferred. Custom painting is limited to connection cards, monitor topology, session
status, and the full-screen bar, each with Qt accessibility support and automated logic
tests where practical.
