<p align="center">
  <img src="resources/icon.png" alt="gomibako woven wastebasket icon" width="256">
</p>

<h1 align="center">gomibako</h1>

<p align="center">
  <img src="https://img.shields.io/badge/version-v0.22-f5a623" alt="Version v0.22">
  <img src="https://img.shields.io/badge/platform-Haiku-ffcc00" alt="Platform: Haiku">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599c" alt="C++17">
  <img src="https://img.shields.io/badge/Deskbar-00599c" alt="Deskbar">
</p>

gomibako is a lightweight Trash manager for the Haiku Deskbar. The current
prototype installs a compact Trash replicant using Tracker's native empty and
full Trash icons, shows the live top-level item count across mounted writable
volumes and removes itself cleanly when the background app exits.

## Build

On Haiku with the standard development tools installed:

```sh
make
```

Run `./gomibako` to add the prototype to Deskbar. Right-click its icon to open
the Trash, empty or restore its contents, open the About window, or quit
gomibako. Files dropped on the icon are moved to the Trash.

Open **Settings** from the Deskbar menu to schedule automatic emptying on
startup, once a day, or every one to six hours. Scheduling is off by default.
Unless disabled in Settings, gomibako gives a 30-second warning that can be
skipped before permanently emptying the Trash. Preferences are saved in
`~/config/settings/DeskTrash_settings`.

The source-only checks use standard POSIX shell tools and can run on other
systems:

```sh
make validate
```

## Status

Milestones 1 through 5 provide the Deskbar lifecycle, event-driven counting,
Trash actions, saved settings, and automatic scheduling. Planned appearance
settings will offer several functional Deskbar icon themes while keeping the
woven gomibako artwork as the application identity.
