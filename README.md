# Dark Mode for Notepad++

A Notepad++ plugin that switches between light and dark mode **live**, with no restart, and an automatic sunrise/sunset mode driven by your location.

**How the plugin presents itself on the bar (last 3 icons: day,night,automatic mode)**

<img width="610" height="32" alt="image" src="https://github.com/user-attachments/assets/036abcf8-3194-4174-9eae-91d1301bc19d" />


> **Requires Notepad++ 8.0 or later** — earlier versions have no dark mode to toggle.

## Features

- **Day Mode** / **Night Mode** — one-click switch; the UI flips instantly.
- **Auto Mode** — computes today's sunrise/sunset from your latitude/longitude, applies the correct theme now, and re-checks every 5 minutes so it flips on its own as the day progresses.
- **Location picker** — ~60 preset cities across all major time zones, or enter custom coordinates.
- **Toolbar icons** — dedicated Sun / Moon / Auto icons, dark-mode aware.
- **No config file surgery** — unlike the common "edit `config.xml` and restart" pattern, this plugin does not touch your Notepad++ configuration on disk or force a restart.

## Requirements

- Notepad++ **8.0 or later** (dark mode support required).
- Windows x64 build of Notepad++.
- English UI (the plugin currently matches the Preferences dialog by English labels — see [How it works](#how-it-works)).

## Installation

### Using the installer script

1. Download the latest `dark_mode_C.zip` from the Releases page.
2. Extract the content of zip anywhere onto your machine (desktop)
3. Right-click `install_plugin.bat` → **Run as Administrator** (required if Notepad++ lives under `Program Files`).
4. Choose **Install / Update Plugin**.


The script locates your Notepad++ install, validates the DLL architecture, closes Notepad++ if it's running, and copies the DLL into `plugins\dark_mode_C\`.

### Manual installation

1. Close Notepad++.
2. Create folder `<Notepad++>\plugins\dark_mode_C\`.
3. Copy `dark_mode_C.dll` into it.
4. Start Notepad++. The plugin appears under **Plugins → dark_mode_C**.

## Usage

All commands are under **Plugins → dark_mode_C**:

| Command | What it does |
|---|---|
| **Day Mode** | Flip to light mode immediately. Disarms Auto Mode if running. |
| **Night Mode** | Flip to dark mode immediately. Disarms Auto Mode if running. |
| **Auto Mode** | Compute sunrise/sunset for your location, apply the correct theme, and keep it in sync for the rest of the session. |
| **Settings** | Pick a city preset or enter custom latitude/longitude. Saved for future sessions. |

On first run the plugin prompts you to set your location.

## Configuration

Settings are stored in Notepad++'s per-plugin config directory:

```
<Notepad++-config>\plugins\Config\dark_mode_C.ini
```

Format:

```ini
[Location]
Latitude=52.2297
Longitude=21.0122
LocationSet=1
```

Edit by hand if you prefer, or use the Settings dialog.

## Building from source

Prerequisites: Visual Studio 2022 with the C++ desktop workload.

1. Open `dark_mode_C/dark_mode_C.sln`.
2. Select configuration **Release** / platform **x64**.
3. Build. Output: `dark_mode_C/x64/Release/dark_mode_C.dll`.

## How it works

Notepad++ exposes no public API to toggle dark mode at runtime — `NPPM_ISDARKMODEENABLED` reads the state, but there is no setter, and the dark-mode logic lives inside the Preferences dialog. This plugin drives the Preferences dialog programmatically:

1. A worker thread is spawned.
2. The main thread opens Preferences via `NPPM_MENUCOMMAND(IDM_SETTING_PREFERENCE)`.
3. The worker finds the dialog, parks it off-screen, selects the **Dark Mode** tab (triggering sub-dialog creation), sends `BM_CLICK` to the Light / Dark radio, and posts `WM_CLOSE`.
4. Because `BM_CLICK` routes through Notepad++'s own handlers, the UI flips through NPP's normal code path — same result as a manual click, just invisible and scripted.

Auto Mode uses a `SetTimer` callback to re-run the sunrise/sunset check every five minutes.

## License

MIT — see [LICENSE](LICENSE).
