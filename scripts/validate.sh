#!/bin/sh

set -eu

project_directory=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_directory"

required_files="
Makefile
resources/gomibako.rdef
resources/icon.png
resources/icon-32.icon
resources/icon-16.icon
scripts/generate_legacy_icons.py
src/AboutWindow.cpp
src/AboutWindow.h
src/AutoEmptyWarningWindow.cpp
src/AutoEmptyWarningWindow.h
src/Gomibako.h
src/GomibakoApp.cpp
src/GomibakoApp.h
src/Settings.cpp
src/Settings.h
src/SettingsWindow.cpp
src/SettingsWindow.h
src/TrashIcon.cpp
src/TrashIcon.h
src/TrashReplicant.cpp
src/TrashReplicant.h
src/main.cpp
"

for required_file in $required_files; do
	if [ ! -f "$required_file" ]; then
		echo "error: missing $required_file" >&2
		exit 1
	fi
done

signature_count=$(grep -El 'application/x-vnd\.sikosis-gomibako' \
	resources/gomibako.rdef src/Gomibako.h src/TrashReplicant.cpp | wc -l | tr -d ' ')
if [ "$signature_count" -ne 2 ]; then
	echo "error: application signature is not defined in exactly two expected files" >&2
	exit 1
fi

if ! grep -Eq 'short_info = "gomibako v0\.25"' resources/gomibako.rdef; then
	echo "error: resource version is not v0.25" >&2
	exit 1
fi

if ! grep -Eq 'minor = 5,' resources/gomibako.rdef; then
	echo "error: resource patch component is not 5" >&2
	exit 1
fi

if ! grep -Eq 'version-v0\.25' README.md; then
	echo "error: README version badge is not v0.25" >&2
	exit 1
fi

if ! grep -Eq '^INSTALL_DIR \?= /boot/home/config/non-packaged/apps$' Makefile \
	|| ! grep -Eq '^install: \$\(APP\)$' Makefile; then
	echo "error: Haiku application install target is missing" >&2
	exit 1
fi

if ! grep -q 'instantiate_deskbar_item' src/TrashReplicant.cpp; then
	echo "error: Deskbar entry point is missing" >&2
	exit 1
fi

if ! grep -q 'AboutWindow.cpp' Makefile; then
	echo "error: About window is not part of the build" >&2
	exit 1
fi

if ! grep -q -- '-ltranslation' Makefile; then
	echo "error: About artwork requires the Translation Kit library" >&2
	exit 1
fi

if [ "$(wc -c < resources/icon-32.icon | tr -d ' ')" -ne 1024 ]; then
	echo "error: large application icon is not 32 x 32 CMAP8 data" >&2
	exit 1
fi

if [ "$(wc -c < resources/icon-16.icon | tr -d ' ')" -ne 256 ]; then
	echo "error: mini application icon is not 16 x 16 CMAP8 data" >&2
	exit 1
fi

if ! grep -q 'BEOS:L:STD_ICON' resources/gomibako.rdef \
	|| ! grep -q 'BEOS:M:STD_ICON' resources/gomibako.rdef; then
	echo "error: application icons are not embedded in the binary" >&2
	exit 1
fi

if ! grep -q 'LoadTrashIcon(16, fCount > 0)' src/TrashReplicant.cpp; then
	echo "error: Deskbar item is not loading a state-aware Trash icon" >&2
	exit 1
fi

if ! grep -q 'B_WATCH_DIRECTORY' src/GomibakoApp.cpp; then
	echo "error: Trash directories are not being watched" >&2
	exit 1
fi

if ! grep -q 'GetNextVolume' src/GomibakoApp.cpp; then
	echo "error: mounted volumes are not being counted" >&2
	exit 1
fi

if ! grep -q 'kMessageEmptyTrash' src/TrashReplicant.cpp; then
	echo "error: Empty Trash action is missing from the Deskbar item" >&2
	exit 1
fi

if ! grep -q 'kMessageTrashRefs' src/TrashReplicant.cpp; then
	echo "error: dropped files are not forwarded to the app" >&2
	exit 1
fi

if ! grep -q 'DeskTrash_settings' src/Settings.cpp; then
	echo "error: persistent settings file is not configured" >&2
	exit 1
fi

if ! grep -q 'BMessageRunner' src/GomibakoApp.cpp; then
	echo "error: one-shot scheduler is missing" >&2
	exit 1
fi

if ! grep -q 'kMessageSkipAutomaticEmpty' src/AutoEmptyWarningWindow.cpp; then
	echo "error: scheduled-empty warning cannot be skipped" >&2
	exit 1
fi

if ! grep -q 'GetSupportedSuites' src/GomibakoApp.cpp; then
	echo "error: Haiku scripting suite is missing" >&2
	exit 1
fi

if ! grep -q 'ResolveSpecifier' src/GomibakoApp.cpp; then
	echo "error: Haiku scripting properties cannot be resolved" >&2
	exit 1
fi

if ! grep -q 'AutoEmptyMode' src/GomibakoApp.cpp; then
	echo "error: scheduling properties are not scriptable" >&2
	exit 1
fi

echo "Static validation passed."
