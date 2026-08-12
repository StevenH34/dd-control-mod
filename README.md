# DD Modern Controls

Runtime patch for Deathtrap Dungeon (1998, GoG release) that replaces its
tank controls with camera-relative modern 3D controls. See
`C:\dev\claude-files\DD\PROJECT-DD-PATHC.md` and the plan this project was
scaffolded from for full background.

Status: **Phase 0 scaffolding done, Phase 1 discovery started.** No hooks
are installed yet — the mod DLL loads, opens a debug console, loads config,
and does nothing to game behavior. See "Phase 1 findings so far" below for
what static analysis has turned up; the remaining Phase 1 work needs x32dbg.

## Layout

- `moddll/` — `DDModCore.dll`, injected into `DD_CD.EXE`. All control-remap
  logic will live here (Phase 3+).
- `injector/` — `DDModInjector.exe`, a small standalone tool that waits for
  `DD_CD.EXE` to start and loads `DDModCore.dll` into it.
- `third_party/minhook/` — vendored [MinHook](https://github.com/TsudaKageyu/minhook)
  (BSD 2-clause), used for inline function hooks once target addresses are
  known.
- `ghidra_scripts/` — Ghidra scripts to assist RE discovery. Run these from
  Ghidra's Script Manager (or headless) against the `DUNGEON.DLL` program in
  the `DeathtrapDungeon` project at `C:\dev\ghidra-projects\DD`.

## Building

Requires the MSVC 32-bit toolset (already installed here via VS Build Tools
2026) and CMake. **Must be configured as a 32-bit (Win32) build** — the game
is a 32-bit process, so the mod DLL and injector have to match.

```
cmake -B build -A Win32
cmake --build build --config Debug
```

Output binaries land in `build/bin/Debug/`: `DDModCore.dll` and
`DDModInjector.exe`.

## Running

1. Build (above).
2. Start `DDModInjector.exe` (it will wait for the game to launch), *then*
   launch the game via GoG/`DD_CD.EXE` — or launch the game first and start
   the injector within its 30s wait window.
3. A console window titled "Deathtrap Dungeon - Modern Controls (debug)"
   should appear, confirming the DLL loaded.

Config is read from `DDModConfig.ini`, auto-created next to `DDModCore.dll`
on first run. In-game hotkeys (configurable in the ini):
- `F9` — reload the ini without restarting
- `F10` — toggle the mod on/off (falls back to stock tank controls)

## Ghidra scripts

Note: this Ghidra install does not have PyGhidra (Python) configured —
headless runs fail with "Ghidra was not started with PyGhidra" — so these
are plain Java `GhidraScript`s instead. They also run fine from the GUI
Script Manager (Window > Script Manager > add `ghidra_scripts` as a script
directory if it isn't already there).

- `FindInputCandidates.java` — lists all call sites of
  `GetAsyncKeyState`/`GetKeyState`/`GetKeyboardState` and bookmarks them
  (`DD-INPUT-CANDIDATE`). Found **zero** hits in `DUNGEON.DLL` — the game
  does not poll the keyboard this way.
- `ListImports.java` — dumps every imported symbol name for the active
  program. Used to confirm `DUNGEON.DLL` imports `DirectInputCreateA` (and
  not `DirectInput8Create`/`DirectInputCreateW`), while `DD_CD.EXE` imports
  neither — input setup lives entirely in `DUNGEON.DLL`.
- `FindDirectInputUsage.java` — bookmarks `DirectInputCreateA` call sites
  (`DD-DINPUT-CANDIDATE`) and the Win32 message-pump calls
  `PeekMessageA`/`DispatchMessageA`/`TranslateMessage`/`MapVirtualKeyA`
  (`DD-MSGLOOP-CANDIDATE`).
- `DecompileAt.java <addr1,addr2,...>` — prints decompiled C for the
  function(s) at the given address(es). Takes addresses as a script arg,
  e.g. `-postScript DecompileAt.java "100a9de8,10074860"`.

Run headless (no GUI) example:

```
support\analyzeHeadless.bat C:\dev\ghidra-projects\DD DeathtrapDungeon ^
  -process DUNGEON.DLL -noanalysis ^
  -scriptPath C:\dev\ghidra-projects\DD\mod\ghidra_scripts ^
  -postScript FindInputCandidates.java
```

(run from the Ghidra install's `support/` directory context, or give the
full path to `analyzeHeadless.bat`)

## Phase 1 findings so far (static analysis only)

- **No keyboard polling.** `DUNGEON.DLL` never calls `GetAsyncKeyState`,
  `GetKeyState`, or `GetKeyboardState`.
- **DirectInput confirmed.** `DUNGEON.DLL` imports `DirectInputCreateA`.
  Static xref search only finds the import thunk itself (address
  `100a9de8`, which Ghidra couldn't fully resolve — "could not recover
  jumptable, too many branches" — and is just a forwarding stub, not real
  game code) — no other static caller was found. This is the expected
  limit of static analysis here: the actual per-frame input read is a COM
  vtable call (`IDirectInputDevice::GetDeviceState`/`GetDeviceData`), which
  isn't a named import and won't show up in an import-xref search.
- **Message pump is a dead end for input.** The two functions with the most
  `PeekMessageA`/`DispatchMessageA`/`TranslateMessage` calls turned out to
  be: `FUN_10074860` — the FMV/cutscene movie player bootstrap (references
  `C:\Asylum\Movies\...` paths, calls `Player_InitVideoSystem` /
  `Player_InitMovie` / etc. — this is intro-video playback, unrelated to
  gameplay input) — and `FUN_10050ae0` — a generic idle/busy message-pump
  helper (`WaitMessage`+`PeekMessage` loop gated on a global flag), which
  just keeps the window responsive and is not where movement input is read.
- **Next step needs x32dbg.** Since the real input read is an indirect
  vtable call, find it dynamically: breakpoint `DirectInputCreateA` (x32dbg
  resolves this fine even though Ghidra's static search couldn't find the
  caller), step out to get the `IDirectInput7A*` return value, follow its
  `CreateDevice` call (vtable slot) to get the `IDirectInputDevice2A*` for
  the keyboard device, then breakpoint that device's `GetDeviceState` (or
  `GetDeviceData`) vtable slot. Every frame's break there, with the return
  address, is the real caller — that's the Phase 1 target function to
  import into Ghidra and reverse for the player-transform update.
