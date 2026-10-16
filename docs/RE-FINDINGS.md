# RE findings log

A running log of reverse-engineering findings, grouped by phase. New
findings go at the end, under a heading for the phase or topic.

This log records how things were found, including dead ends. The current,
checked reference for addresses and structs is `RE-CONTROLS-CAMERA.md`;
when a finding here is confirmed, it goes there too.

## Phase 1: early static analysis

Out of date: this was the first discovery pass. The input path was later
traced statically (`Kbd_Poll` at `0x10050f50`, see `RE-CONTROLS-CAMERA.md`
§1), and x32dbg was used afterwards to verify it (§5) rather than with the
plan in the last bullet.

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
