<h1 align="center">gomibako</h1>

<p align="center">
  <img src="https://img.shields.io/badge/version-v0.15-f5a623" alt="Version v0.15">
  <img src="https://img.shields.io/badge/platform-Haiku-ffcc00" alt="Platform: Haiku">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599c" alt="C++17">
  <img src="https://img.shields.io/badge/Deskbar-00599c" alt="Deskbar">
</p>

gomibako is a lightweight Trash manager for the Haiku Deskbar. The current
prototype installs a compact Trash replicant using Tracker's native Trash icon,
shows the live top-level item count across mounted writable volumes and removes
itself cleanly when the background app exits.

## Build

On Haiku with the standard development tools installed:

```sh
make
```

Run `./gomibako` to add the prototype to Deskbar. Right-click its icon to open
the About window or choose **Remove from Deskbar** to remove it and stop the
background app.

The source-only checks use standard POSIX shell tools and can run on other
systems:

```sh
make validate
```

## Status

Milestone 1 is complete. Milestone 2 now provides event-driven Trash counting
across mounted writable volumes. Trash actions, settings and scheduling remain
to be implemented.
