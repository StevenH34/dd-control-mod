# DD Modern Controls

Runtime patch for Deathtrap Dungeon (1998, GoG release) that replaces its
tank controls with camera-relative modern 3D controls. See
`docs/PROJECT-DD-PATHC.md` for background and tool paths.

Status: **RE done and runtime-verified (see
`docs/RE-CONTROLS-CAMERA.md`). First hook written:
camera-relative movement via `Player_ReadControls`. It builds but has not
been tested in-game yet.** Camera changes come later. The "Phase 1 findings
so far" section at the bottom is the early discovery log and is out of date.

## Layout

This repo (`C:\dev\DD-mod`) holds everything that's versioned. Large or
generated RE data stays outside it:

- Ghidra project (annotated, current): `C:\dev\ghidra-projects\DD-REA`
  (`DeathtrapDungeon.gpr`). The old pre-REA project in
  `C:\dev\ghidra-projects\DD` is kept but no longer used.
- REA output: `C:\dev\rea-projects\DD\results`.

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
- `ghidra_scripts/` — Ghidra scripts to assist RE discovery. Run these from
  Ghidra's Script Manager (or headless) against the `DUNGEON.DLL` program in
  the `DeathtrapDungeon` project at `C:\dev\ghidra-projects\DD-REA`.
  `ApplyControlsCameraNames.java` applies the names from
  `docs/RE-CONTROLS-CAMERA.md`.

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

## Movement hook (`moddll/src/movement.cpp`)

Hooks `Player_ReadControls` (`DUNGEON.DLL` + `0x871b0`, `void __cdecl(actor)`)
with MinHook. `Hooks_Init` waits up to 120 s for `DUNGEON.DLL`, computes
every address from the module base (the game normally loads it at
`0x10000000`), and logs `[move] ... hook installed` to the debug console.

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
3. **Target heading** = camera yaw (`*(*(*0x101f11c0+0x10)+0x1c)`) +
   `camera_yaw_offset` + input angle (forward 0, right +256, back 512;
   1024 = 360 degrees). The error is the shortest signed difference to the
   player yaw `*(*(actor+0x2c)+0x10) & 0x3ff`.
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
| `camera_yaw_offset` | 0 | added to camera yaw; set 512 if Up walks *toward* the camera |
| `invert_steer` | 0 | set 1 if Left/Right come out mirrored |
| `steer_with_turn_bit` | 1 | also set turn bit 2/4 while steering (see above) |

With `verbose_logging = 1`, the console prints the state, camera yaw,
target, player yaw, error, command word and rate every 30 frames. State
changes (`passthrough`/`idle`/`pivot`/`move`) are always logged.

### Needs in-game testing (none of this has been run in the game yet)

- The hook installs and the game doesn't crash on level load. MinHook has
  to relocate the prologue at `0x100871b0`, which hasn't been inspected.
- **Camera yaw convention.** The code assumes the camera transform yaw uses
  the same "forward = (-sin, -cos)" convention as the actor. If Up walks
  toward the camera, set `camera_yaw_offset = 512`. If it's off by 90
  degrees, try 256 or -256.
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
C:\tools\ghidra_12.1.4_PUBLIC\support\analyzeHeadless.bat ^
  C:\dev\ghidra-projects\DD-REA DeathtrapDungeon ^
  -process DUNGEON.DLL -noanalysis -readOnly ^
  -scriptPath C:\dev\DD-mod\ghidra_scripts ^
  -postScript DecompileAt.java "100871b0"
```

Headless can't open the project while the Ghidra GUI has it open (the
`.lock` file); close the GUI first. Drop `-readOnly` for scripts that
should save changes, such as `ApplyControlsCameraNames.java`.

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
