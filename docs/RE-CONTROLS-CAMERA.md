# Deathtrap Dungeon — Controls & Camera (static RE findings)

Target: `<game folder>\DUNGEON.DLL` from the GoG release
(sha256 `95fe9ce0fff387f00704548f152e4340815213fcb3833dbe1b5c42871e7d2e56`, image base `0x10000000`). Analyzed with REA 6.5.0 + Ghidra 12.1.4.

All of this is **static analysis**. Nothing here has been checked in a debugger
yet. Names are mine (Ghidra still shows `FUN_xxxxxxxx`). Struct field meanings
are inferred from how the code uses them.

## Conventions used by the game

| Thing | Format |
|---|---|
| Angles | 1024 units = 360°. Sine table at `0x10288550` (1024 dwords); cos(a) = `sin[(a+0x100)&0x3ff]` |
| Fixed point | 1.0 = `0x4000` (14-bit). Digital action value = `0x4000` |
| Calling convention | cdecl everywhere seen (caller does `ADD ESP`) |
| Player actor | global `0x10235ea4` |

---

## 1. Input pipeline (4 layers)

```
DirectInput keyboard ─┐
Mouse (0x100e4c50..)  ├─► key/mouse bitmasks ─► Control module (keys.cfg actions) ─► Player controller ─► locomotion state machine
Joystick (winmm?)     ┘                          Action_Value(id)                    command bits + turn rate
```

### 1a. Raw keyboard — `0x10050f50 Kbd_Poll(uint mask[8])`
- `IDirectInputDevice::GetDeviceState` (vtable `+0x24`, 256 bytes) → packs pressed keys into a 256-bit mask.
- Codes are DIK scancodes, **except letters A–Z, which are remapped to `0x80–0x99`** (`MapVirtualKeyA` VK + `0x3f`).
- Also reads `GetDeviceData` (vtable `+0x28`) for typed characters, Shift and Backspace.
- Device pointer: `0x100ddc28`.

### 1b. Per-frame key state — `0x100697c0 Input_FrameUpdate`
- `0x101f1170[8]` = keys down now, `0x101f1190[8]` = keys that changed this frame (XOR).
- Accessors: `0x10069860 Key_IsDown(code)`, `0x10069890 Key_Pressed`, `0x100698d0 Key_Released`, `0x10069810 Key_WaitRelease`.
- Mouse: `0x100e4c50` x, `0x100e4c54` y, `0x100e4c58` buttons, `0x100e4c5c`/`0x60` deltas, `0x100e4c64` button edges.
- Demo record/playback lives here: `0x10098460` (playback) and `0x10098560` (record). These save the raw masks, so hooks placed above this layer don't change demo files.

### 1c. Control module — action bindings (`0x1005d100–0x1005eff0`)
Data-driven from **`ASYLUM\keys.cfg`**. Lines look like `define ACTION_X DOWN|PRESS|RELEASE KEY_A + KEY_UP`.

| Address | Role |
|---|---|
| `0x100def10` | Action table, 52 × `0x34` bytes: `name[0x2c]`, `id` (1–0x34), flag |
| `0x100df9d8` | Input-source table, `0x44` stride: `+4` type (1 key, 2 mouse, 3 joystick), `+8` code, `+0xc` name (`KEY_UP`, `MOUSE_LBUTTON`, `JOY_BUTTON_5`…) |
| `0x1005dad0` | `Control_LoadBindings`. Opens the user copy (path at `0x101f11d4`). If that's missing, it copies it from `asylum\keys.cfg` (`0x1005e240`) |
| `0x1005dfb0` | Parses one `define` line; hold type comes from `0x1005de40` (1 DOWN, 2 PRESS, 3 RELEASE) |
| `0x101f4b30 + id*0x90` | Binding storage: `[0]` combo count (≤5), then combos of 7 dwords `{nKeys, src*[5], holdType}` |
| `0x1005dc40` | **`Control_Update` (per frame)**. Samples mouse (`0x1005ea90`) and joystick (`0x1005ec40`, analog → `0x101f4b10..1c`), then evaluates each action: `0x1005e650` → `0x1005e6d0` (AND of keys in a combo) → `0x1005e810` dispatch → `0x1005e870` key / `0x1005e920` mouse / `0x1005eb20` joystick |
| `0x101f6b40 + id*8` | Action state `{status, value}`. Status: 1 inactive, 2 held, 3 analog, 4 pressed, 5 released |
| `0x1005dd00` | **`Action_Value(id)`**: returns 0 when inactive |
| `0x1005dca0` | **`Action_Get(id, &value, &isAnalog)`** |

Every action is evaluated independently. Holding `A+UP` fires **both** `WALK_FORWARD` and `RUN_FORWARD`, and the player controller decides which one wins.

Action IDs used for movement: `1` WALK_FORWARD, `2` WALK_BACKWARD, `3` RUN_FORWARD, `4` RUN_BACKWARD, `5` STEP_FORWARD, `6` STEP_BACKWARD, `7` LEFT_SIDESTEP, `8` RIGHT_SIDESTEP, `9` TURN_LEFT, `0xa` TURN_RIGHT, `0xb` TURN_FAST_LEFT, `0xc` TURN_FAST_RIGHT, `0x19` 1ST_PERSON_VIEW, `0x1e–0x21` GENERAL_UP/DOWN/LEFT/RIGHT, `0x34` ROLL. (The full table of 52 IDs is in the order of `keys.cfg`.)

---

## 2. Player controller (where the tank controls are)

### Call chain each frame
```
0x100878d0 Game_ControlTick        (called from 0x10057e02)
  ├─ Control_Update()
  ├─ 1ST_PERSON_VIEW → 0x1002f900 (camera flag 2, camera mode 4) + 0x10045150 (zero turn rate)
  ├─ CAMERA_LOCK_ON/OFF: read, but the result is thrown away (dead feature)
  └─ TOGGLE_CAMERA_TYPE → 0x1002f8e0(0x100696a0, 0x10069670): installs camera callbacks

Camera_Update 0x10030e30 → pre-hook [0x10104640] = 0x10069670 PlayerControlHook
  └─ 0x10087160 Controller_Dispatch(player)
       ├─ actor type flags have 0x1000000|0x10000 and [0x101f11d0]!=0 → 0x100871b0 Player_ReadControls
       └─ otherwise                                                    → 0x10087650 (alternate/simple controller)
```
**Important:** player input is read *inside the camera update*, before the camera moves. The level-start code (`0x10093e70`) installs the hooks.

### `0x100871b0 Player_ReadControls(actor)`, the patch target
`ctl = *(actor+0x114)`. It clears the command word `ctl+0x144` and turn rate `ctl+0x148` and then sets them again:
- `0x10082860(actor, bits)`: `ctl+0x144 |= bits`
- `0x10082880(actor, rate)`: `ctl+0x148 = rate`

| Action | Effect |
|---|---|
| TURN_FAST_LEFT / RIGHT (`0xb`/`0xc`) | rate = ∓speed, cmd `4` (fast turn) |
| TURN_LEFT / RIGHT (`9`/`0xa`) | rate = ∓speed, cmd `2` (turn) |
| LEFT / RIGHT_SIDESTEP (`7`/`8`) | `0x100` / `0x200` |
| STEP_FORWARD / BACKWARD (`5`/`6`) | `0x400` / `0x800` (shuffle) |
| WALK_FORWARD / BACKWARD (`1`/`2`) | `0x8` / `0x10` |
| RUN_FORWARD / BACKWARD (`3`/`4`) | `0x20` / `0x40` |
| JUMP_CLIMB/FWD/LEFT/RIGHT/BACK | `0x1000000` / `0x100000` / `0x400000` / `0x800000` / `0x200000` |
| ATTACK_1 / 2 / 3 / BACK | `0x2000` / `0x4000` / `0x8000` / `0x10000` |
| PARRY | `0x1000` |
| ATTACK_RANGED / CAST_SPELL (only when not jumping) | `0x20000` / `0x40000` |
| OPERATE | `*(actor+0x2c) \|= 1` |

- Turn speed: for digital input, `*(ctl+0x150)` (which points at `ctl+0x10c`, the walk-turn speed, or `ctl+0x114`, the fast-turn speed) × an acceleration factor of 1/2/3. The factor grows while the key is held (`0x100fd240` counts frames; >5 → 2, ≥10 → 3). For analog input, the rate is `2*value`, clamped to ±3×speed.
- In first-person (`0x1002f920`), all of movement, turning, jumping and melee is skipped.

### Locomotion state machine (consumes the command word)
- `0x10082840 Cmd_Test(actor, mask)` = `ctl+0x144 & mask`.
- States are registered with `0x10044ec0(state, …, update, transition, exit)`. Animation transitions also test masks that come from the anim data (`0x1005f860` uses `state[0x66]`).
- Turn on the spot: `0x10068710` (anims 2/3, update `0x100687e0`) adds **±0x20/frame** to yaw in `0x10068970`. Fast turn: `0x10068a30` (anims 4/5) adds **±0x40/frame** in `0x10068c20`.
  - Yaw write: `*(*(actor+0x2c)+0x10) = baseYaw + accumulated`.
- From the turn states, cmd `8`/`0x20`/`0x10` leave into walk/run (`0x100575d0`, `0x100575f0`), and `0x100`/`0x200` leave into sidestep (`0x1005f4f0`, `0x1005f520`).
- **Walking and running move along the actor's facing and are driven by the animation.** That is the root of the tank controls: no input path sets a world-space direction.

---

## 3. Camera

### Objects
- `0x10068e50` creates the world `0x101f11bc`, the camera object `0x101f11c0`, and the viewport `0x101f11b4` (320×200).
- Camera transform = `*(camobj+0x10)`: `[0..2]` position, `[6]` pitch, `[7]` yaw, `[8]` roll, `[0x40]` sector.
- **Camera controller struct `CAM` = `0x101044a0`**, initialized in `0x1002dab0`. That function also sets `camobj+0x18 = 0x10030e30` as the per-frame callback.

| CAM offset | Meaning |
|---|---|
| `+0xfc/+0x100/+0x104` | → player x / y / z |
| `+0x108` | → player sector |
| `+0x10c` | → player rotation (yaw at `+4`) |
| `+0x110`, `+0x114` | player actor, player anim (head bone at `+0x80..0x88`) |
| `+0x16c` | min follow distance = **900** (global `0x1010460c`) |
| `+0x170` | max follow distance = **1500** (`0x10104610`) |
| `+0x174` | min pitch slope = `0xb43` (`0x10104614`); max slope is a hard-coded `0x1126` in `0x1002e5c0` |
| `+0x180` | flags (= global `0x10104620`). bit0 game camera on; bit1 first-person requested; bit2 first-person active; `0x80` steep pitch allowed (falling) |
| `+0x1ac/+0x1b0/+0x1b4` | first-person look pitch / yaw offset / body yaw |
| `+0x1b8` | active camera zone |
| `+0x1c4..+0x1d0` | per-map camera-zone tables ("Camera Arrangement") |
| `+0x264..+0x26c` | look-at target; `+0x270..` its velocity |
| `+0x27c` | **mode**: 3 chase, 4 first-person, 5 snap-to-player then chase. Set via `0x100310d0(cam, mode)` |

### Per-frame: `0x10030e30 Camera_Update` (Ghidra doesn't treat it as a function; it's only referenced as data)
1. If flag bit0 is clear, it calls `[0x10104644]` instead. That is the debug free camera `0x100696a0` → `0x10003190`, which flies on RShift/CapsLock/LCtrl plus the mouse.
2. It calls the pre-hook `[0x10104640]`, which is **the player controller** (see §2).
3. `0x10030d30` computes the look-at target from the player position: y + 100, or about 300 above the floor on ledges.
4. A first-person request switches to mode 4 (`0x1002f420`, `0x100310d0(cam,3)`).
5. Up to 20 iterations of `0x10030f10` (zone check `0x1002fc30` + mode handler) and `0x10030f80` (has it converged?).
6. `0x10030aa0` smooths the vertical movement.

### Chase camera (mode 3) — `0x1002f310`
It stays where it is while it can see you, then jumps somewhere else:
- **Fixed camera zones:** `0x10030120` picks the highest-priority zone for the player's sector, `0x1002fcf0` enters it, and `0x10030650` positions it. These are the authored per-room cameras.
- `0x1002f6d0` checks line of sight from the current camera to the target (`0x10030910`).
  - **Visible** → `0x1002def0` follows:
    - `0x1002e720` handles the desired position and falling/ledge cases.
    - `0x1002df60` solves occlusion and the path.
    - `0x1002e5c0` clamps distance to [900, 1500] and clamps pitch.
    - `0x1002e950` moves the camera, capped at **`0x6e` (110) units/frame**.
    - `0x10030790` writes the transform.
  - **Not visible** → `0x1002f750` repositions: a random direction on the same side of the player, about 768 units out and 450–550 up (`0x1002f840`, RNG `0x10030b70`).
- The camera **does not follow the player's facing** and has no user orbit input.

### First-person (mode 4) — `0x10031190` → `0x10031270`
- Positioned at the head bone.
- Look input comes from `0x1002f570` (GENERAL_UP/DOWN/LEFT/RIGHT), clamped to ±32 per frame and accumulated into pitch ±200 and yaw ±300 relative to the body.
- Look speed `0x101f11f4`; invert X/Y flags `0x101f11e4` / `0x101f11e8`.

---

## 4. Patch points for modern controls

1. **Movement (main hook): `0x100871b0 Player_ReadControls`.** Call the original, then rewrite the command word at `*(actor+0x114)+0x144`:
   - Read a 2D move vector from actions 1/2/9/10 (or GENERAL_*, or the analog values from `Action_Get`).
   - Rotate it by the **camera yaw** (`(*(0x101f11c0+0x10))[7]`) to get a world heading.
   - Set the actor yaw toward that heading. Either write it directly or steer through `ctl+0x148`.
   - Clear the turn/back/sidestep bits (`2,4,0x10,0x40,0x100,0x200,0x800`) and set `8` (walk) or `0x20` (run, from the magnitude or the run key).
   - Leave the jump/attack/parry/operate bits alone.
   - *Verified (§5):* `actor` is passed as a normal cdecl argument. While walking or running, the turn rate is applied to yaw 1:1 per frame, so steer with `ctl+0x148`, not with direct yaw writes.
2. **Camera: replace or extend mode 3 (`0x1002f310`).** A player-follow orbit camera would use yaw/pitch from the mouse or arrow keys and reuse `0x1002e5c0` for distance clamping and `0x10030910` for line of sight. Returning 0 from `0x10030120` turns off the authored fixed cameras.
3. **Tunables:** CAM `+0x16c/+0x170/+0x174`; the hard-coded max pitch `0x1126` in `0x1002e5c0`; the 110-units/frame move cap in `0x1002e950`.
4. **Hook order is already correct.** The controller runs inside `Camera_Update` before the camera solve. If the camera hook updates camera yaw first, the movement hook can read it in the same frame.
5. **Don't hook below the Control module** (`Kbd_Poll` / `Input_FrameUpdate`). Demos record that layer, and the menus use it too.

## 5. Runtime verification (x32dbg headless, 2026-10-14)

Method: a non-pausing log breakpoint at `0x10087191`, right after the `Player_ReadControls` call returns, while the scripted test sequence was played. 1,626 frames were captured. `DUNGEON.DLL` loaded at its preferred base `0x10000000`, so static addresses apply unchanged.

**Confirmed**
- **Actor argument:** `0x100871b0` is a normal cdecl call. The dispatcher runs `PUSH ECX` (the actor) before its `TEST`, which the decompiler dropped. At runtime, `[esp] == [0x10235ea4]` on every frame.
- **Camera pre-hook:** `[0x10104640] == 0x10069670` throughout play.
- **Command bits:** they match §2. The combos stack as predicted: holding `A+Up` gives `0x28` (walk + run), and `A+Up+Left` gives `0x2C`. `Z+Left` gives `0x102` (sidestep + turn bit), and the sidestep wins.
- **Yaw** at `*(actor+0x2c)+0x10`, 1024 units = 360°.
  - **The value is not wrapped.** It ranged from −4100 to 1174, so use `& 0x3ff`.
  - **Forward is `(x, z) = (−sin yaw, −cos yaw)`.** `atan2(dx, dz) − yaw` was always ±512.
- **Turning on the spot** (cmd `2` / `4`): yaw changes by a fixed **±32** or **±64 per frame**. The turn rate only chooses the direction.
- **Turning while walking or running** (cmd `0xA`, `0x2C`): **yaw changes by exactly the turn rate (`ctl+0x148`) each frame**.
  - Walk turn: 12 → 24 → 36 as the key is held.
  - Run turn: 14 → 28 → 42.
  - So setting the turn rate during locomotion steers the character, with no direct yaw write needed.
- **Speeds:** walking is 40 units/frame, running about 85–100 units/frame.
- **Camera modes:** level start uses mode 5, then 3. Tab switches to 4 (camera flags `0x3307`) and back to 3. In mode 3, camera yaw holds still on about a third of frames and makes large jumps (±500, i.e. cuts) on about 56 frames, which fits the hold-then-reposition chase logic.

**Patch implication:** in the `Player_ReadControls` hook, compute the target heading from the input and the camera yaw. Then:
- Emit walk/run (`8`/`0x20`).
- Set `ctl+0x148 = clamp(angle_diff(target, yaw & 0x3ff), ±N)` to steer.
- If the heading error is large (say more than 90°) while standing, emit a turn command (`2`/`4`) with the rate's sign to pivot first.

**Not tested:** writing yaw directly while walking. It should be unnecessary given the steering result above.

## 6. Camera yaw convention (static, confirmed by the first mod run, 2026-10-15)

**The camera yaw is 180° off from the actor yaw convention.**

- `0x10030790` (transform write) sets camera pitch and yaw from `0x10030730(camPos, target, &out)`:
  - `out[1] = 0x100 - atan2g(dx, dz)`, where `(dx, dz) = target - camPos`;
  - `out[0] = -atan2g(horizontal length, dy)`.
- `0x10037890 atan2g(x, y)` returns angles in 1024 units, measured from +x toward +y:
  - `0x10037910` maps y → −y to `0x400 − a`;
  - `0x10037890` maps x → −x to `−a − 0x200`.
- So `camYaw = 256 − atan2(dz, dx)`. Under the actor convention (forward = `(−sin, −cos)`), that "forward" is `(−dx, −dz)`: it points from the target back at the camera.
- **The camera's look direction in actor yaw units is `camYaw + 512`.**
- Confirmed in game: the first mod build used `camYaw` directly, and Up walked toward the camera while Down walked away from it.

Related:
- The look-at target `CAM+0x264..0x26c` is the player position (`*CAM+0xfc`, `*CAM+0x104`; y + 100), copied each frame in `0x10030d30`. So the camera-to-player direction can also be computed from the camera transform `[0]`/`[2]` and those pointers.
- `0x10087160`'s early-out `0x1008f920` returns `[0x101d41c8]`.
- `0x1005dd00` is `Action_Value`: it returns `0` when the action's status is 1, otherwise the stored value.
- In fullscreen nGlide, any window that takes focus minimizes the game. The mouse poll `0x10050d50` then loops `GetDeviceState`/`Acquire` forever without pumping messages, so the game hangs (seen on the loading screen).

## 7. Startup and the session loop (static, from the REA reports)

How the launcher gets into the game, and the top-level loop that everything above runs under. The binary overviews and the launcher search are in `docs/rea/`.

- **Launcher:** `DDSTART.EXE` imports `CreateProcessA` and none of `WinExec`/`ShellExecute`, so it starts the game as a separate process. `DD_CD.EXE` then loads `DUNGEON.DLL`.
- **`Play_Win32_Deathtrap`** (DLL export, ordinal 3, RVA `0x1b510`): the game's entry point.
  - Saves its 4th and 5th arguments to `0x101f1438` / `0x101f143c`.
  - Copies the two blocks its first two arguments point to (14 + 103 dwords) into one contiguous local buffer.
  - Calls `Session_Init(0)`, then `Session_Push(buffer, arg3)`.
  - Runs `Session_RunAll` until it returns 0 (no sessions left), then calls `Session_Shutdown` and returns 0.
- **`0x1007ed10 Session_Init(count)`:** allocates a pool named `session_stack` with `count` entries (at least 1) of `0x9ec` bytes each, via `0x10088980`. Pool pointer `0x101d7a90`, list head `0x101d7a94`, ID counter `0x101d7a98` and current session `0x100f8c10` all start at 0.
- **`0x1007ed80 Session_Push(params, flag)`:** takes an entry from the pool (`0x10088a60`) and puts it at the front of the list.
  - Links: `+0x9e8` next, `+0x9e4` previous.
  - `+4` gets the next ID from the counter. The 117-dword (`0x75`) parameter block is copied to `+0x800`.
  - Sets `+0x9d8 = 0x7fffffff`, flags `+8 = 0x80044`, `+0xc = 0xff`, and installs the callback `0x1007ef70` through `0x1007ef60`.
  - Flags: always adds `0x20000`, and adds `0x400` when `flag` is 0. Returns the new entry.
- **`0x1007ee70 Session_RunAll`:** one pass over the list, called once per frame.
  - For each entry it sets the current session (`0x100f8c10`) and calls the entry's callback at `+0`, if it has one.
  - If the entry has flag `0x10000` and the global `0x10216440` is 0 (likely "window inactive"), it doesn't run the callback. The first time, it calls `0x10047d80(2,0)` and marks the entry with `0x100000`. Every time, it runs the idle message pump `0x10050ae0`.
  - When `0x10216440` is set again, it clears `0x100000`, calls `0x1009a030(1)` + `0x1009a1f0` twice and `0x10047d80(4,0)`, then runs the callback.
  - Returns how many entries it visited.
- **`0x1007ed60 Session_Shutdown`:** frees the pool (`0x10088a30`) and clears the pool, list head and current-session globals.
