# Deathtrap Dungeon Modern Controls

This is a purely experimental repo I wanted to create to test the functionality Reverse Engineer Anything ([REA](https://github.com/morluto/rea)). I've long had the idea of taking an older game and upating the control scheme through reverse engineering. As anyone who's attempted decompiling a game through Ghidra would  know, this is a very complex process. To my surprise, Claude and REA were able to find the code for player controls and the camera. Things aren't perfect right now, this is still a work-in-progres, but this is a great start. Hopefully with some time I can figure out a solution and use this information in the future to patch other games.


Goal: Runtime patch for Deathtrap Dungeon (1998, GoG release) that replaces its
tank controls with camera-relative modern 3D controls. See
`docs/PROJECT-DD-PATHC.md` for background and tool paths.

Status: **RE done and runtime-verified (see
`docs/RE-CONTROLS-CAMERA.md`). First hook written:
camera-relative movement via `Player_ReadControls`. It builds but has not
been tested in-game yet.** Camera changes come later. The "Phase 1 findings
so far" section at the bottom is the early discovery log and is out of date.

## Supported game version

All addresses in this repo are for the GoG release. Check your files match
before using the mod (`certutil -hashfile <file> SHA256` in a command prompt):

| File | SHA-256 |
|------|---------|
| `DUNGEON.DLL` | `95fe9ce0fff387f00704548f152e4340815213fcb3833dbe1b5c42871e7d2e56` |
| `ASYLUM\DD_CD.EXE` | `0c644a00e62652e046c5dad2960f0f6c8c1998f4ca065780fbd7811d9908bf1f` |
| `DDSTART.EXE` | `c45d23a17b29057f33c020f2b5853fd19fbce31b3ccae66af99dc3fd4c0d8dfb` |

`DUNGEON.DLL` is the one that matters: the hooks patch addresses inside it.
No game files are included in this repo.

## Layout

This repo holds everything that's versioned. Large, generated or
game-derived RE data stays outside it:

- Ghidra project (annotated, current): `<ghidra projects>\DD-REA`
  (`DeathtrapDungeon.gpr`). It embeds a copy of the game's code, so it is
  not published.
- REA output: `<rea projects>\DD\results`. The small reports are copied
  into `docs/rea/`.

In the repo:

- `docs/` — `RE-CONTROLS-CAMERA.md` (addresses, struct offsets, runtime
  verification) and `PROJECT-DD-PATHC.md` (project goal, game and tool
  paths).
- `moddll/` — `DDModCore.dll`, injected into `DD_CD.EXE`. All control-remap
  logic will live here (Phase 3+).
- `injector/` — `DDModInjector.exe`, a small standalone tool that waits for
  `DD_CD.EXE` to start and loads `DDModCore.dll` into it.
- `third_party/minhook/` — vendored [MinHook](https://github.com/TsudaKageyu/minhook)
  (BSD 2-clause), used for inline function hooks once target addresses are
  known.
- `ghidra_scripts/` — Ghidra scripts for RE work. Run these from Ghidra's
  Script Manager (or headless) against the `DUNGEON.DLL` program in the
  `DeathtrapDungeon` Ghidra project. `ImportSymbols.py` applies the names and
  comments in `docs/symbols.json`.

## Building

Requires the MSVC 32-bit toolset (e.g. from Visual Studio Build Tools) and
CMake. **Must be configured as a 32-bit (Win32) build** — the game
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
   - It skips any `DD_CD.EXE` with a debugger attached (for example a
     leftover x32dbg session). If other instances are already running, it
     waits for a new one.
3. Keep the injector window open. The mod writes its log to `DDMod.log` next
   to `DDModCore.dll`, and the injector prints that file live until the game
   exits. The first lines confirm the DLL loaded.

**Don't open a window inside the game.** In fullscreen (nGlide), any window
that takes focus during loading minimizes the game. The game then spins
forever in its DirectInput mouse re-acquire loop (`0x10050d50`: retries
`GetDeviceState`/`Acquire` without pumping messages), so it stays stuck on
the loading screen. That's why the in-game console (`show_debug_console`) is
now off by default. Only turn it on for windowed debugging.

Config is read from `DDModConfig.ini`, auto-created next to `DDModCore.dll`
on first run. In-game hotkeys (configurable in the ini):
- `F9` — reload the ini without restarting
- `F10` — toggle the mod on/off (falls back to stock tank controls)

## Movement hook (`moddll/src/movement.cpp`)

Hooks `Player_ReadControls` (`DUNGEON.DLL` + `0x871b0`, `void __cdecl(actor)`)
with MinHook. `Hooks_Init` waits up to 120 s for `DUNGEON.DLL`, computes
every address from the module base (the game normally loads it at
`0x10000000`), and logs `[move] ... hook installed` to `DDMod.log`.

Each frame the detour calls the original, then rewrites the movement part
of the command word `*(actor+0x114)+0x144` and the turn rate at `+0x148`:

1. **Pass through (stock behavior)** in any of these cases: F10 has disabled
   the mod; the original bailed out early (`0x1008f920` nonzero); the game is
   in first person (`0x1002f920`); or the stock word already has a jump,
   attack, parry, ranged or spell bit (`0x1F7F000`). The default bindings
   for those share the arrow keys (CAPS+arrow, ENTER+arrow), so they keep
   their tank-relative meaning.
2. **Read intent** with `Action_Value` (`0x1005dd00`): forward = max(1, 3),
   back = max(2, 4), left = max(9, 0xb), right = max(10, 0xc). Run = any of
   3, 4, 0xb, 0xc (that is, A held with an arrow).
3. **Target heading** = camera look yaw + `camera_yaw_offset` + input angle
   (forward 0, right +256, back 512; 1024 = 360 degrees). The error is the
   shortest signed difference to the player yaw
   `*(*(actor+0x2c)+0x10) & 0x3ff`.
   - Camera look yaw = camera yaw (`*(*(*0x101f11c0+0x10)+0x1c)`) **+ 512**.
     The game writes camera yaw as `0x100 - atan2(dz, dx)` of the
     camera-to-target vector (`0x10030790` → `0x10030730`; its atan2
     `0x10037890(x, y)` measures from +x). Under the actor convention
     (forward = (-sin, -cos)), that points from the player back at the
     camera. The first in-game run confirmed this: Up walked toward the
     camera.
4. **Rewrite**: clear bits `2,4,8,0x10,0x20,0x40,0x100,0x200,0x800`, then:
   - *pivot*: if we didn't emit walk/run last frame and |error| is above
     `pivot_threshold_deg`, emit turn bit `2` (`4` when running) with a turn
     rate whose sign picks the direction, and no walk bit.
   - *move*: otherwise emit walk `8` (run: `0x28`, the same as stock A+Up)
     and set the turn rate to the error clamped to ±`max_steer_walk` or
     ±`max_steer_run`. With `steer_with_turn_bit = 1` it also sets bit `2`/`4`
     while the rate is nonzero. Turning while moving was only ever observed
     with that bit set (stock `0xA`, `0x2C`).

   Step forward (`0x400`), operate and all action bits are left alone.

New `DDModConfig.ini` keys (all optional; an older ini just uses defaults):

| Key | Default | Meaning |
|---|---|---|
| `max_steer_walk` | 36 | max yaw change per frame while walking (stock 12-36) |
| `max_steer_run` | 42 | max yaw change per frame while running (stock 14-42) |
| `pivot_threshold_deg` | 90 | if standing and the error is above this, turn on the spot first |
| `camera_yaw_offset` | 0 | extra offset on the camera look yaw (the 180° fix is built in) |
| `invert_steer` | 0 | set 1 if Left/Right come out mirrored |
| `steer_with_turn_bit` | 1 | also set turn bit 2/4 while steering (see above) |

With `verbose_logging = 1`, the log prints the state, camera look yaw
(`cam`), target, player yaw, error, command word and rate every 30 frames.
State changes (`passthrough`/`idle`/`pivot`/`move`) are always logged. Each
line also has `posHdg`, the camera-to-player direction computed from the
camera position (transform `[0]`,`[2]`) and the player position (CAM
`+0xfc`/`+0x104`). It's only a cross-check and should track `cam` closely.
`-1` means it's unavailable or the camera is nearly overhead.

### Test status

Run 1 (2026-10-15): the hook installed, levels loaded, and F10 toggled
cleanly. Directions were 180° off (Up walked toward the camera), which is
now fixed in code. Still to check:

- **Camera direction.** Up should now walk away from the camera. In the
  log, `cam` and `posHdg` should agree to within a few tens of units. If
  they differ by about 512 or 256, the convention analysis above is wrong.
- **Left/right sign.** The code assumes a positive rate means turning right,
  which is true for the stock TURN_RIGHT path. If left and right are
  mirrored, set `invert_steer = 1`.
- **Steering without the turn bit.** Try `steer_with_turn_bit = 0` to see
  whether the turn rate alone steers while walking. If it does, that avoids
  any side effects of the turn bit, such as turn animations.
- **Pivot then walk.** Check that it pivots cleanly when the error is over
  90 degrees, and that a 180-degree reversal while already walking arcs
  smoothly (it steers there; it doesn't pivot).
- Whether steering oscillates around the target heading, and whether the
  per-frame clamps feel right at walk and run speed.
- **Camera cuts.** Mode 3 makes yaw jumps of ±500. Holding a direction
  through a cut swings the target heading at once. A later step may need to
  latch the camera yaw while the input direction is held.
- Diagonals (Up+Left), and the run key with only Left or Right (A+Left).
- Z+arrow (sidestep/step) now walks camera-relative; check that it isn't
  odd.
- CAPS/ENTER combos (attacks, jumps) still behave as stock.
- F10 switches back to stock tank controls mid-level without glitches.

## Ghidra scripts

`DecompileAt.java` runs in any Ghidra. The `.py` scripts are Python 3 and
need PyGhidra (see below). All of them run from the GUI Script Manager
(Window > Script Manager > add `ghidra_scripts` as a script directory if it
isn't already there).

- `ExportSymbols.py [output]` — writes the function names, labels and
  comments you added to `docs/symbols.json` (one entry per line, so diffs
  stay readable). Only user-defined names are exported, never code or
  bytes, so the file is safe to publish. Comments Ghidra adds by itself
  (Function ID "Library Function" matches, PE import/export and header
  comments) are skipped, since Ghidra recreates them; pass `all` to keep
  them. Review the file before committing anyway.
- `ImportSymbols.py [input] [force]` — applies `docs/symbols.json` back to a
  program. Creates missing functions, replaces comments of the same type at
  the same address, and is safe to re-run. It refuses to run if the file's
  SHA-256 doesn't match the program (wrong game build) unless you pass
  `force`. Together with `ExportSymbols.py`, this lets anyone with the game
  rebuild the annotated project without the Ghidra database being shared.
  It replaces the old hard-coded `ApplyControlsCameraNames.java`.
- `DecompileAt.java <addr1,addr2,...>` — prints decompiled C for the
  function(s) at the given address(es). Takes addresses as a script arg,
  e.g. `-postScript DecompileAt.java "100a9de8,10074860"`.

Run headless (no GUI) example. Replace the `<...>` placeholders with your
Ghidra install, your Ghidra project folder and this repo:

```
<ghidra install>\support\analyzeHeadless.bat ^
  <ghidra projects>\DD-REA DeathtrapDungeon ^
  -process DUNGEON.DLL -noanalysis -readOnly ^
  -scriptPath <this repo>\ghidra_scripts ^
  -postScript DecompileAt.java "100871b0"
```

Headless can't open the project while the Ghidra GUI has it open (the
`.lock` file); close the GUI first. Drop `-readOnly` for scripts that
should save changes, such as `ImportSymbols.py`.

The Phase 1 discovery scripts (`FindInputCandidates.java`,
`ListImports.java`, `FindDirectInputUsage.java`) were removed once their
results were recorded below. They're still in the git history.

### PyGhidra (Python 3 scripts)

PyGhidra ships with Ghidra (11.2 and later). It runs real CPython 3 in the
same process as Ghidra, so `.py` scripts get the full Ghidra API and normal
Python packages.

1. Install a 64-bit Python 3.9–3.14. This project uses **3.14**, tested
   with Ghidra 12.1.4. Ghidra bundles a ready-made JPype only for 3.9–3.13,
   so on 3.14 pip builds JPype from source during the first run. That
   takes a minute and needs the MSVC x64 C++ toolset (Visual Studio Build
   Tools, "Desktop development with C++"), which you already have if you
   build the mod. Without a C++ compiler, use 3.13 instead.
2. Start Ghidra with `<ghidra install>\support\pyghidraRun.bat` instead of
   `ghidraRun.bat`. The first run looks for a supported Python, offers to
   create a virtual environment in your Ghidra user settings folder (not in
   this repo), and installs PyGhidra into it from Ghidra's bundled wheels
   (no internet needed). Later runs reuse it.
3. Use the Script Manager as usual; the `.py` scripts carry
   `@runtime PyGhidra`. Window > PyGhidra opens a Python 3 console.

Headless, use `pyghidraRun.bat -H` with the usual `analyzeHeadless`
arguments:

```
<ghidra install>\support\pyghidraRun.bat -H ^
  <ghidra projects>\DD-REA DeathtrapDungeon ^
  -process DUNGEON.DLL -noanalysis -readOnly ^
  -scriptPath <this repo>\ghidra_scripts ^
  -postScript ExportSymbols.py
```

If the launcher picks the wrong Python, it remembers the last one it used.
Delete the `venv` folder in your Ghidra user settings directory to make it
ask again.

## Phase 1 findings so far (static analysis only)

- **No keyboard polling.** `DUNGEON.DLL` never calls `GetAsyncKeyState`,
  `GetKeyState`, or `GetKeyboardState`.
- **DirectInput confirmed.** `DUNGEON.DLL` imports `DirectInputCreateA`
  (not `DirectInput8Create`/`DirectInputCreateW`). `DD_CD.EXE` imports
  neither, so input setup lives entirely in `DUNGEON.DLL`.
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
