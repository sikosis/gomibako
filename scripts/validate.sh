#!/bin/sh

set -eu

project_directory=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_directory"

required_files="
Makefile
resources/gomibako.rdef
src/AboutWindow.cpp
src/AboutWindow.h
src/Gomibako.h
src/GomibakoApp.cpp
src/GomibakoApp.h
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

if ! grep -Eq 'short_info = "gomibako v0\.15"' resources/gomibako.rdef; then
	echo "error: resource version is not v0.15" >&2
	exit 1
fi

if ! grep -Eq 'minor = 5,' resources/gomibako.rdef; then
	echo "error: resource patch component is not 5" >&2
	exit 1
fi

if ! grep -Eq 'version-v0\.15' README.md; then
	echo "error: README version badge is not v0.15" >&2
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

if ! grep -q 'LoadTrashIcon(16)' src/TrashReplicant.cpp; then
	echo "error: Deskbar item is not loading the native Trash icon" >&2
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

echo "Static validation passed."
