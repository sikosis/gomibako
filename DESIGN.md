# Deskbar Trash Manager — Design Document

Oct 23, 2026 · @Sikosis

## Overview and goals

gomibako puts a Trash icon with a live item count in the Haiku Deskbar tray, and can empty the Trash automatically on startup or on a schedule.

**Goals**

- Show the Trash state at a glance: an empty/full icon with the item count beside it.
- One click opens the Trash; a right-click menu empties it, restores items, or opens settings.
- Optional auto-empty: never, on startup, daily at a set time, or every 1 to 6 hours.
- Reuse the system's own tools (`trash`, `untrash`, Tracker) instead of re-implementing Trash logic.
- Stay light: no polling loops, negligible memory, and no way for a bug to take the Deskbar down with it.

**Non-goals for v1**

- Replacing Tracker's Trash window.
- Secure or shredding deletion.
- Per-file or per-type retention rules (listed as a future extension).

## Background: what Haiku already provides

The design leans on four existing pieces, so DeskTrash adds a front end and a scheduler rather than new Trash logic.

**The `trash` and `untrash` commands.** These are the supported way to move files to the Trash, list it, empty it and restore from it:

```
trash [--restore|--empty|--list] file ...
    --restore   restore files (act as untrash)
    --empty     empty the Trash
    --list      list what's already in the Trash
untrash [--all] [file ...]
```

DeskTrash will call `trash --empty`, `trash --list` and `untrash --all` rather than deleting files itself, so Tracker, the command line and DeskTrash always behave the same way.

**Per-volume Trash folders.** `B_TRASH_DIRECTORY` is a per-volume directory; `find_directory()` assumes the boot disk when no volume is given ([BeBook, Storage Kit](https://datatime.sourceforge.net/Be%20Book/The%20Storage%20Kit/misc.html)). The item count therefore has to cover every mounted writable volume, not just the boot volume.

**Deskbar add-ons.** The Deskbar shelf holds replicants (archivable BViews), added with `BDeskbar::AddItem()` using either a view or an `entry_ref` to an add-on ([BDeskbar](https://www.haiku-os.org/legacy-docs/BeBook/BDeskbar.html)). An add-on exports `instantiate_deskbar_item(float maxWidth, float maxHeight)`, as the system Bluetooth item does ([Haiku source](https://git.haiku-os.org/haiku.git/tree/src/servers/bluetooth/DeskbarReplicant.cpp?h=master)). The Deskbar's own `desklink` tool shows the add, list and remove flow (`--add-volume`, `--list`, `RemoveItem()`) ([desklink.cpp](https://cgit.haiku-os.org/haiku.git/tree/src/bin/desklink/desklink.cpp)).

**Replicant messaging.** A replicant can message its host app with a normal BMessage. The reverse direction works by having the replicant put a BMessenger to itself in the message it sends ([OpenBeOS newsletter 47](https://haiku-os.org/legacy-docs/openbeosnewsletter/nsl47.html)). DeskTrash uses this to receive count updates.

**Node monitoring.** Haiku's BeOS-style node monitor can watch a directory for entries added and removed. That lets the count update on change instead of polling. This is general Storage Kit behaviour and should be confirmed against the current headers during implementation.

## User experience

The Deskbar item is the Trash icon with the count to its right, so the icon stays a standard tray size and the number never covers it.

**Layout and states**

| State | Icon | Count text | Tooltip |
| --- | --- | --- | --- |
| Empty | Empty-can icon | Hidden | Trash is empty |
| Has items | Full-can icon | Item count, e.g. `12` | Trash: 12 items |
| Over 99 items | Full-can icon | `99+` | Trash: 142 items |
| Emptying | Full-can icon, dimmed | `...` | Emptying Trash... |
| Error | Full-can icon with badge | Last known count | Could not empty: reason |

- The view width is icon plus gap plus the width of the text, so it stays compact when the count is hidden.
- Count text uses the system plain font at tray size, in the tray's normal text colour so it follows the light and dark themes.
- The tooltip can also show the total size, but only when it has been computed on demand, so the tray never scans file sizes in the background.
- The icon is a scalable vector icon (HVIF), so it stays sharp at any tray height. Deskbar passes the maximum item size into `instantiate_deskbar_item`, and the view fits itself to that.

**Interactions**

| Action | Result |
| --- | --- |
| Left-click | Open the Trash folder in Tracker |
| Right-click | Context menu (below) |
| Drop files on the icon | Move them to the Trash (`trash file ...`) |
| Hover | Tooltip with count |

**Context menu**

1. Open Trash
2. Empty Trash... (disabled when empty; asks for confirmation unless turned off in settings)
3. Restore All... (runs `untrash --all` after confirmation)
4. Auto-empty: shows the current rule as a disabled label, e.g. "Every 3 hours"
5. Settings...
6. Remove from Deskbar

## Architecture

DeskTrash is two parts: a small replicant in the Deskbar that only draws and forwards clicks, and a background app that owns the count, the schedule and the settings.

&#91;embedded content: DeskTrash components · 2 processes, 3 system parts\]

The replicant sends requests to the app and receives updates from it. Everything that touches the Trash goes through the app, the watcher, or the `trash` command.

| Component | Responsibility | Talks to |
| --- | --- | --- |
| Trash replicant | Draws the icon and count, handles click, menu and drops. Keeps only the last count it was sent. | DeskTrash app (BMessage) |
| Trash watcher | Counts top-level items in each mounted volume's Trash. Updates on change, and rescans when a volume is mounted or unmounted. | Trash folders, replicant |
| Scheduler | Works out the next empty time from the settings, fires it, and records when the last empty finished. | `trash` command, settings |
| Settings | Loads and saves the preferences file and shows the settings window. | Scheduler, replicant |
| `trash` / `untrash` | Do the real empty, list and restore work. | Trash folders |
| Tracker | Shows the Trash folder window. | None |

**Design decisions**

- **Two parts, not one.** A replicant is loaded into the Deskbar's own process, so a crash in it would take the Deskbar with it. Keeping the replicant to drawing and message passing keeps that risk small.
- **The app owns the replicant.** On launch it adds the item with `BDeskbar::AddItem()` from the replicant's `entry_ref`. On quit it removes the item with `RemoveItem()`. This is the same add and remove flow `desklink` uses.
- **Replicant to app, and back.** The replicant sends requests to the app. It also sends a BMessenger to itself, so the app can push count updates to it.
- **No polling.** The count changes only when the watcher reports an entry added or removed. The scheduler sleeps until the next due time.

## Auto-empty scheduling

Auto-empty is off by default. When on, exactly one rule is active, chosen from the four modes below.

| Mode | When it fires | Notes |
| --- | --- | --- |
| Off (default) | Never | Manual Empty only. |
| On startup | Once, 60 seconds after DeskTrash launches | The delay keeps boot and login fast. |
| Daily | At a chosen time, for example 18:00 | If the computer was off at that time, it runs at the next launch ("catch up"), which can be turned off. |
| Every N hours | N = 1, 2, 3, 4, 5 or 6 | Counted from the last successful empty, not from boot, so a restart does not reset the clock. |

**Rules the scheduler follows**

1. **One timer.** The scheduler keeps a single pending timer and recomputes the next fire time after every run or settings change. A one-shot timer rescheduled each time avoids drift.
2. **Persistent last-empty time.** The time of the last successful empty is saved in the settings file. This is what makes "every N hours" and "catch up" work across restarts.
3. **Nothing to do, nothing shown.** If the Trash is empty when a rule fires, it records the run and stays silent.
4. **Warning first (on by default).** Before an automatic empty, a notification says "Emptying Trash in 30 seconds" with a Skip action. Skipping leaves the Trash alone until the next scheduled time.
5. **Same path as manual.** Automatic and manual empties both run `trash --empty`, so results and errors are identical.
6. **No retry loops.** If an empty fails, a notification gives the reason and the scheduler waits for the next due time instead of retrying immediately.
7. **No overlap.** A second request that arrives while an empty is running is ignored.

## Settings window and preferences file

Settings open from the context menu and apply immediately. The app reschedules as soon as a value changes.

**Window contents**

- Empty the Trash automatically: a pop-up with Never, When the computer starts, Every day at, Every N hours.
- Time field (hours and minutes), enabled only for "Every day at".
- Hours pop-up (1 to 6), enabled only for "Every N hours".
- Checkbox: Run a missed daily empty when DeskTrash starts.
- Checkbox: Warn me 30 seconds before an automatic empty.
- Checkbox: Ask before Empty Trash from the menu.
- Checkbox: Show the item count next to the icon.
- Buttons: Empty Trash Now, Defaults, Revert.

**Preferences file**

Stored as a flattened BMessage in the user settings directory, in a file named `DeskTrash_settings`.

| Key | Type | Default | Meaning |
| --- | --- | --- | --- |
| `mode` | int32 | 0 | 0 off, 1 startup, 2 daily, 3 every N hours |
| `dailyMinutes` | int32 | 1080 | Minutes after midnight (1080 = 18:00) |
| `intervalHours` | int32 | 3 | 1 to 6 |
| `catchUp` | bool | true | Run a missed daily empty at launch |
| `warnBeforeEmpty` | bool | true | Show the Skip notification first |
| `warnSeconds` | int32 | 30 | Length of the warning |
| `confirmManualEmpty` | bool | true | Ask before menu Empty Trash |
| `showCount` | bool | true | Show the number beside the icon |
| `lastEmptied` | int64 | 0 | Time of the last successful empty (seconds since the epoch) |

## Edge cases, risks and safety

The main risk is losing files the user still wanted. DeskTrash never deletes anything itself; it only runs `trash --empty`, and every automatic path is opt-in and warns first.

| Situation | Behaviour |
| --- | --- |
| Auto-empty is enabled by mistake | Off by default. The settings window states that emptying is permanent. The pre-empty warning with Skip is on by default. |
| Some items cannot be deleted | The notification names the failure. The count follows the watcher, so it stays accurate. No immediate retry. |
| Removable volume unmounted | Its Trash stops counting while unmounted and is never emptied. It counts again when remounted. |
| Very large Trash | Only top-level entries are counted, matching the Trash window, and change events are coalesced into short batches. |
| User empties the Trash in Tracker | The watcher sees the entries disappear and the count drops. `lastEmptied` is not changed, because only DeskTrash runs are recorded. |
| DeskTrash quits or crashes | The replicant notices that its messenger is dead and removes itself from the Deskbar. |
| Deskbar restarts | DeskTrash notices the Deskbar launching again and adds the item back. |
| System clock changes | The next fire time is recomputed. A `lastEmptied` value in the future is treated as unset. |
| Daylight saving change | Daily mode recomputes "minutes after midnight" in local time each time it schedules. |
| Restore All hits a name conflict | `untrash` decides the outcome. DeskTrash reports any errors it returns. |
| Items dropped on the icon | Moved with `trash file ...`, so Tracker's restore information is recorded the same way as any other trashing. |

## Implementation plan and open questions

Six milestones, each ending in something that can be tried on a running Haiku system.

1. **Replicant prototype.** Static icon and a hard-coded count, added to and removed from the Deskbar by a small app. Done when the item appears, survives a Deskbar restart as designed, and removes cleanly.
2. **Live count.** Trash watcher across all mounted volumes, with messages to the replicant. Done when the number matches the Trash window while files are trashed and restored.
3. **Actions.** Click to open, context menu, drop to trash, Empty and Restore All through `trash` and `untrash`.
4. **Settings.** Settings window and the `DeskTrash_settings` file, applied live.
5. **Scheduler.** Startup, daily and every-N-hours modes, catch-up, and the warning notification with Skip.
6. **Polish and release.** Vector icons for the empty and full states, translatable strings (system add-ons use `B_TRANSLATE`), a Haiku package, and short user documentation.

**Open questions**

- [ ] Does the Deskbar keep add-on items across restarts, or must the app re-add them every time?
- [ ] What is the right call to enumerate every mounted volume's Trash folder and to detect mount and unmount?
- [ ] Should "on startup" be combinable with a daily or hourly rule?
- [ ] Count top-level items (proposed) or every file inside trashed folders?
- [ ] Is drop-to-trash in v1, or later?
- [ ] Optional later rule: only empty items older than N days.
- [ ] Final name and application signature.

## Sources

- [BDeskbar, BeBook](https://www.haiku-os.org/legacy-docs/BeBook/BDeskbar.html)
- [Storage Kit global constants, BeBook (per-volume Trash directory)](https://datatime.sourceforge.net/Be%20Book/The%20Storage%20Kit/misc.html)
- [Deskbar replicant for the Bluetooth server, Haiku source](https://git.haiku-os.org/haiku.git/tree/src/servers/bluetooth/DeskbarReplicant.cpp?h=master)
- [desklink.cpp, Haiku source](https://cgit.haiku-os.org/haiku.git/tree/src/bin/desklink/desklink.cpp)
- [OpenBeOS newsletter 47, A Deskbar Replicant](https://haiku-os.org/legacy-docs/openbeosnewsletter/nsl47.html)
- [find\_directory.cpp, Haiku source](https://github.com/haiku/haiku/blob/c471444a3399218827a7f3f7fcf6bd9b1ca76456/src/system/libroot/os/find_directory.cpp)

The `trash` and `untrash` usage is from the help text you supplied. Sources above were checked as search excerpts; confirm API details against current Haiku headers before implementing.
