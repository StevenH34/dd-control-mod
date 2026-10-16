#include "movement.h"
#include "config.h"
#include "console.h"
#include "hooks.h"

#include <windows.h>
#include <MinHook.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace
{
    // ---- DUNGEON.DLL layout (RVAs from preferred base 0x10000000) ----------
    // All runtime-verified in docs/RE-CONTROLS-CAMERA.md section 5 unless noted.
    constexpr uintptr_t kRvaPlayerReadControls = 0x871b0;  // void __cdecl(actor)
    constexpr uintptr_t kRvaActionValue        = 0x5dd00;  // int __cdecl(id), 0 = inactive, 0x4000 = digital
    constexpr uintptr_t kRvaCameraIsFirstPerson= 0x2f920;  // uint __cdecl(), returns [0x10104620] & 2
    constexpr uintptr_t kRvaControlsBlocked    = 0x8f920;  // int __cdecl(), returns [0x101d41c8]; the original
                                                           // returns early when nonzero (purpose not identified)
    constexpr uintptr_t kRvaCameraObject       = 0x1f11c0; // global: camera object pointer
    constexpr uintptr_t kRvaCamCtl             = 0x1044a0; // CAM controller struct (static, not a pointer)
    constexpr int kCamCtlPlayerX = 0xfc;   // CAM -> int* player x (read by look-at 0x10030d30)
    constexpr int kCamCtlPlayerZ = 0x104;  // CAM -> int* player z

    // Actor / controller offsets
    constexpr int kActorCtl      = 0x114;  // -> controller struct
    constexpr int kActorXform    = 0x2c;   // -> transform, yaw at +0x10 (unwrapped, 1024 = 360 deg)
    constexpr int kCtlCommand    = 0x144;  // command bit word
    constexpr int kCtlTurnRate   = 0x148;  // turn rate (applied 1:1 to yaw while walking/running)
    constexpr int kXformYaw      = 0x10;
    constexpr int kCamXform      = 0x10;   // camera object -> transform: int x,y,z at [0..2], yaw at [7]
    constexpr int kCamXformX     = 0x0;
    constexpr int kCamXformZ     = 0x8;
    constexpr int kCamXformYaw   = 0x1c;
    // Camera yaw is written by 0x10030790 as 0x100 - atan2(dz, dx) of the
    // camera->target vector (0x10030730; the game's atan2 0x10037890 takes
    // (x, y) and measures from +x). Under the actor convention, forward =
    // (-sin, -cos), that yaw points from the target back at the camera, so
    // the camera's look direction in actor yaw units is camYaw + 512.
    constexpr int kCamYawToLookYaw = 512;

    // Action ids (keys.cfg order)
    enum : int
    {
        kActWalkFwd = 1, kActWalkBack = 2, kActRunFwd = 3, kActRunBack = 4,
        kActTurnLeft = 9, kActTurnRight = 10, kActTurnFastLeft = 0xb, kActTurnFastRight = 0xc,
    };

    // Command bits
    constexpr uint32_t kCmdTurn      = 0x2;
    constexpr uint32_t kCmdTurnFast  = 0x4;
    constexpr uint32_t kCmdWalk      = 0x8;
    constexpr uint32_t kCmdWalkBack  = 0x10;
    constexpr uint32_t kCmdRun       = 0x20;
    constexpr uint32_t kCmdRunBack   = 0x40;
    constexpr uint32_t kCmdSideLeft  = 0x100;
    constexpr uint32_t kCmdSideRight = 0x200;
    constexpr uint32_t kCmdStepBack  = 0x800;
    constexpr uint32_t kCmdClearMask = kCmdTurn | kCmdTurnFast | kCmdWalk | kCmdWalkBack | kCmdRun |
                                       kCmdRunBack | kCmdSideLeft | kCmdSideRight | kCmdStepBack;
    // Parry, attacks, ranged, spell, jumps. Their default bindings share the
    // arrow keys (CAPS+arrow, ENTER+arrow), so when any of these is set this
    // frame the stock command word is left completely alone.
    constexpr uint32_t kCmdActionMask = 0x1000 | 0x2000 | 0x4000 | 0x8000 | 0x10000 | 0x20000 | 0x40000 |
                                        0x100000 | 0x200000 | 0x400000 | 0x800000 | 0x1000000;

    using PlayerReadControlsFn = void(__cdecl*)(uint8_t* actor);
    using ActionValueFn        = int(__cdecl*)(int id);
    using NoArgIntFn           = int(__cdecl*)();

    uintptr_t            g_base = 0;
    PlayerReadControlsFn g_origReadControls = nullptr;
    ActionValueFn        g_actionValue = nullptr;
    NoArgIntFn           g_cameraIsFirstPerson = nullptr;
    NoArgIntFn           g_controlsBlocked = nullptr;

    enum class MoveState { Passthrough, Idle, Pivot, Move };
    MoveState g_lastState = MoveState::Passthrough;
    bool      g_wasMoving = false;   // emitted walk/run last frame
    unsigned  g_frame = 0;

    const char* StateName(MoveState s)
    {
        switch (s)
        {
        case MoveState::Passthrough: return "passthrough";
        case MoveState::Idle:        return "idle";
        case MoveState::Pivot:       return "pivot";
        case MoveState::Move:        return "move";
        }
        return "?";
    }

    // Signed shortest difference target - current, in [-512, 511].
    int AngleDiff(int target, int current)
    {
        int d = (target - current) & 0x3ff;
        return d >= 512 ? d - 1024 : d;
    }

    int Clamp(int v, int lo, int hi)
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    int Max2(int a, int b) { return a > b ? a : b; }

    bool ReadCameraYaw(int& yawOut)
    {
        uint8_t* camObj = *reinterpret_cast<uint8_t**>(g_base + kRvaCameraObject);
        if (!camObj)
            return false;
        uint8_t* xform = *reinterpret_cast<uint8_t**>(camObj + kCamXform);
        if (!xform)
            return false;
        yawOut = *reinterpret_cast<int*>(xform + kCamXformYaw) + kCamYawToLookYaw;
        return true;
    }

    // Diagnostic cross-check: the camera->player direction from positions,
    // in actor yaw units. Should match the look yaw above (up to camera
    // lag). Returns -1 if unavailable or the camera is nearly overhead.
    int PositionHeading()
    {
        uint8_t* camObj = *reinterpret_cast<uint8_t**>(g_base + kRvaCameraObject);
        uint8_t* xform = camObj ? *reinterpret_cast<uint8_t**>(camObj + kCamXform) : nullptr;
        uint8_t* camCtl = reinterpret_cast<uint8_t*>(g_base + kRvaCamCtl);
        int* px = *reinterpret_cast<int**>(camCtl + kCamCtlPlayerX);
        int* pz = *reinterpret_cast<int**>(camCtl + kCamCtlPlayerZ);
        if (!xform || !px || !pz)
            return -1;
        double dx = double(*px) - *reinterpret_cast<int*>(xform + kCamXformX);
        double dz = double(*pz) - *reinterpret_cast<int*>(xform + kCamXformZ);
        if (dx * dx + dz * dz < 100.0 * 100.0)
            return -1;
        // forward = (-sin yaw, -cos yaw) = (dx, dz) / |d|
        return static_cast<int>(std::lround(std::atan2(-dx, -dz) * 512.0 / 3.14159265358979)) & 0x3ff;
    }

    void SetState(MoveState s, int camYaw, int target, int yaw, int err, uint32_t cmd, int rate)
    {
        bool changed = s != g_lastState;
        g_lastState = s;
        if (changed || (Config_Get().verboseLogging && (g_frame % 30) == 0))
        {
            int posHdg = s == MoveState::Passthrough ? -1 : PositionHeading();
            Log("[move] %-11s cam=%4d posHdg=%4d target=%4d yaw=%4d err=%4d cmd=0x%08X rate=%d",
                StateName(s), camYaw & 0x3ff, posHdg, target & 0x3ff, yaw, err, cmd, rate);
        }
    }

    void __cdecl Detour_PlayerReadControls(uint8_t* actor)
    {
        g_origReadControls(actor);
        ++g_frame;

        if (!Hooks_IsModEnabled() || !actor || g_controlsBlocked() != 0 || g_cameraIsFirstPerson() != 0)
        {
            g_wasMoving = false;
            if (g_lastState != MoveState::Passthrough)
                SetState(MoveState::Passthrough, 0, 0, 0, 0, 0, 0);
            return;
        }

        uint8_t* ctl = *reinterpret_cast<uint8_t**>(actor + kActorCtl);
        uint8_t* xform = *reinterpret_cast<uint8_t**>(actor + kActorXform);
        int camYaw = 0;
        if (!ctl || !xform || !ReadCameraYaw(camYaw))
            return;

        uint32_t& cmd = *reinterpret_cast<uint32_t*>(ctl + kCtlCommand);
        int& rate = *reinterpret_cast<int*>(ctl + kCtlTurnRate);

        if (cmd & kCmdActionMask)
        {
            g_wasMoving = false;
            SetState(MoveState::Passthrough, camYaw, 0, 0, 0, cmd, rate);
            return;
        }

        // Arrow-key intent. Each direction has a walk and a run action
        // (A+Up fires both WALK_FORWARD and RUN_FORWARD, A+Left fires
        // TURN_FAST_LEFT but not TURN_LEFT), so take the max of the pair.
        int fwd   = Max2(g_actionValue(kActWalkFwd),   g_actionValue(kActRunFwd));
        int back  = Max2(g_actionValue(kActWalkBack),  g_actionValue(kActRunBack));
        int left  = Max2(g_actionValue(kActTurnLeft),  g_actionValue(kActTurnFastLeft));
        int right = Max2(g_actionValue(kActTurnRight), g_actionValue(kActTurnFastRight));
        bool run = g_actionValue(kActRunFwd) || g_actionValue(kActRunBack) ||
                   g_actionValue(kActTurnFastLeft) || g_actionValue(kActTurnFastRight);

        int x = right - left;   // +x = camera right
        int y = fwd - back;     // +y = away from camera

        int yaw = *reinterpret_cast<int*>(xform + kXformYaw) & 0x3ff;

        if (x == 0 && y == 0)
        {
            // No direction held. The stock word has no movement bits in this
            // case except stepping/sidestep combos, which need an arrow too.
            g_wasMoving = false;
            SetState(MoveState::Idle, camYaw, 0, yaw, 0, cmd, rate);
            return;
        }

        const ModConfig& cfg = Config_Get();

        // Input angle in game units: forward = 0, right = +256. TURN_RIGHT
        // produces a positive turn rate in the stock code, so "right" is +yaw.
        int inputAngle = static_cast<int>(std::lround(std::atan2(double(x), double(y)) * 512.0 / 3.14159265358979));
        if (cfg.invertSteer)
            inputAngle = -inputAngle;
        int target = camYaw + cfg.cameraYawOffset + inputAngle;
        int err = AngleDiff(target, yaw);

        cmd &= ~kCmdClearMask;

        int pivotThreshold = static_cast<int>(cfg.pivotThresholdDeg * 1024.0f / 360.0f);
        if (!g_wasMoving && std::abs(err) > pivotThreshold)
        {
            // Standing and facing well away: turn on the spot first. The
            // on-spot turn speed is fixed (32 or 64/frame); the rate only
            // picks the direction.
            int speed = run ? cfg.maxSteerRun : cfg.maxSteerWalk;
            cmd |= run ? kCmdTurnFast : kCmdTurn;
            rate = err > 0 ? speed : -speed;
            SetState(MoveState::Pivot, camYaw, target, yaw, err, cmd, rate);
            return;
        }

        int maxSteer = run ? cfg.maxSteerRun : cfg.maxSteerWalk;
        rate = Clamp(err, -maxSteer, maxSteer);
        // Stock run is A+Up = walk|run (0x28); match it.
        cmd |= run ? (kCmdWalk | kCmdRun) : kCmdWalk;
        // Turning while walking was only observed with the turn bit set
        // (0xA, 0x2C). Optionally set it the same way the stock code would.
        if (cfg.steerWithTurnBit && rate != 0)
            cmd |= run ? kCmdTurnFast : kCmdTurn;
        g_wasMoving = true;
        SetState(MoveState::Move, camYaw, target, yaw, err, cmd, rate);
    }
}

bool Movement_InstallHook()
{
    // DUNGEON.DLL may be loaded after injection, so wait for it.
    HMODULE dungeon = nullptr;
    for (int i = 0; i < 1200 && !dungeon; ++i)
    {
        dungeon = GetModuleHandleA("DUNGEON.DLL");
        if (!dungeon)
            Sleep(100);
    }
    if (!dungeon)
    {
        Log("[move] DUNGEON.DLL not loaded after 120s - movement hook NOT installed");
        return false;
    }

    g_base = reinterpret_cast<uintptr_t>(dungeon);
    g_actionValue = reinterpret_cast<ActionValueFn>(g_base + kRvaActionValue);
    g_cameraIsFirstPerson = reinterpret_cast<NoArgIntFn>(g_base + kRvaCameraIsFirstPerson);
    g_controlsBlocked = reinterpret_cast<NoArgIntFn>(g_base + kRvaControlsBlocked);
    void* target = reinterpret_cast<void*>(g_base + kRvaPlayerReadControls);

    Log("[move] DUNGEON.DLL base=0x%08X, Player_ReadControls=0x%08X",
        static_cast<unsigned>(g_base), static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)));
    if (g_base != 0x10000000)
        Log("[move] note: DUNGEON.DLL is not at its preferred base; using RVAs");

    MH_STATUS status = MH_CreateHook(target, reinterpret_cast<void*>(&Detour_PlayerReadControls),
                                     reinterpret_cast<void**>(&g_origReadControls));
    if (status != MH_OK)
    {
        Log("[move] MH_CreateHook(Player_ReadControls) failed: %s", MH_StatusToString(status));
        return false;
    }

    status = MH_EnableHook(target);
    if (status != MH_OK)
    {
        Log("[move] MH_EnableHook(Player_ReadControls) failed: %s", MH_StatusToString(status));
        return false;
    }

    Log("[move] Player_ReadControls hook installed (camera-relative movement)");
    return true;
}
