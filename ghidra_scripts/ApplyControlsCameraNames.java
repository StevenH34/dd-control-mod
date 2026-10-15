// Applies the controls/camera RE findings (docs/RE-CONTROLS-CAMERA.md in the DD-mod repo)
// to DUNGEON.DLL: function names, global labels and plate comments. Also creates the
// two functions Ghidra misses because they're only referenced as data
// (Camera_Update 0x10030e30, PlayerControlHook 0x10069670).
// Safe to re-run; skips anything not present in the current program.
//
// @category DeathtrapDungeon

import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class ApplyControlsCameraNames extends GhidraScript {

    // address, name, plate comment (may be null)
    private static final String[][] FUNCTIONS = {
        // Raw input
        {"10050f50", "Kbd_Poll", "DirectInput GetDeviceState(256) -> 256-bit key mask. Letters A-Z remapped to 0x80-0x99."},
        {"100696e0", "Input_Init", null},
        {"100697c0", "Input_FrameUpdate", "Per frame: g_KeysDown = Kbd_Poll(); g_KeysChanged = old ^ new."},
        {"10098460", "Input_DemoPlayback", "Reads recorded key mask + mouse from demo file."},
        {"10098560", "Input_DemoRecord", null},
        {"10069860", "Key_IsDown", null},
        {"10069890", "Key_Pressed", null},
        {"100698d0", "Key_Released", null},
        {"10069810", "Key_WaitRelease", null},
        // Control module (keys.cfg actions)
        {"1005dad0", "Control_LoadBindings", "Loads user keys.cfg (falls back to copying asylum\\keys.cfg)."},
        {"1005dba0", "Control_SaveBindings", null},
        {"1005dc40", "Control_Update", "Per frame: sample mouse/joystick, evaluate every action into g_ActionState[id]."},
        {"1005dca0", "Action_Get", "Action_Get(id, &value, &isAnalog)."},
        {"1005dd00", "Action_Value", "Returns action value (0x4000 digital, magnitude analog) or 0 if inactive."},
        {"1005dd20", "Action_ResetAll", null},
        {"1005dd70", "Action_IdFromName", null},
        {"1005ddd0", "InputSource_FromName", null},
        {"1005de40", "HoldType_FromName", "DOWN=1 PRESS=2 RELEASE=3."},
        {"1005df50", "Control_ParseFile", null},
        {"1005dfb0", "Control_ParseDefine", "Parses: define ACTION_X DOWN|PRESS|RELEASE SRC + SRC ..."},
        {"1005e240", "Control_CopyDefaultKeysCfg", null},
        {"1005e330", "Control_WriteFile", null},
        {"1005e650", "Action_Evaluate", null},
        {"1005e6d0", "Combo_Evaluate", null},
        {"1005e810", "Source_Evaluate", "Dispatch on source type: 1 key, 2 mouse, 3 joystick."},
        {"1005e870", "Source_EvalKey", null},
        {"1005e920", "Source_EvalMouse", null},
        {"1005ea90", "Mouse_Sample", null},
        {"1005eb20", "Source_EvalJoystick", null},
        {"1005ec40", "Joystick_Sample", null},
        // Player controller / locomotion
        {"100878d0", "Game_ControlTick", "Control_Update + 1st-person / camera-type / render-mode actions."},
        {"10087160", "Controller_Dispatch", null},
        {"100871b0", "Player_ReadControls", "TANK CONTROLS. Actions -> command bits (ctl+0x144) and turn rate (ctl+0x148). Main movement patch target."},
        {"10087650", "Player_ReadControlsAlt", null},
        {"10082840", "Cmd_Test", "*(actor+0x114)+0x144 & mask"},
        {"10082860", "Cmd_Set", "*(actor+0x114)+0x144 |= bits"},
        {"10082880", "Cmd_SetTurnRate", "*(actor+0x114)+0x148 = rate"},
        {"10068710", "LocoState_TurnEnter", "Turn on spot, anims 2/3, +-0x20 yaw/frame."},
        {"100687e0", "LocoState_TurnUpdate", null},
        {"10068970", "LocoState_TurnApplyYaw", null},
        {"10068a30", "LocoState_FastTurnEnter", "Fast turn, anims 4/5, +-0x40 yaw/frame."},
        {"10068c20", "LocoState_FastTurnApplyYaw", null},
        {"10045150", "Player_ClearTurn", null},
        // Camera
        {"10068e50", "World_CreateCameraViewport", null},
        {"1002dab0", "Camera_Init", "Sets CAM tunables (min/max dist 900/1500, pitch) and camobj+0x18 = Camera_Update."},
        {"10030e30", "Camera_Update", "Per frame: pre-hook (player controls) -> target -> mode solve loop -> apply."},
        {"10069670", "PlayerControlHook", "Installed as camera pre-hook; calls Controller_Dispatch(player)."},
        {"100696a0", "DebugFreeCam_Update", null},
        {"10003190", "DebugFreeCam_Move", null},
        {"1002f8e0", "Camera_SetCallbacks", "([0x10104644] alt/free cam, [0x10104640] pre-hook)"},
        {"1002f900", "Camera_RequestFirstPerson", null},
        {"1002f920", "Camera_IsFirstPerson", null},
        {"10030d30", "Camera_UpdateTarget", "Look-at = player pos, y+100 (~300 above floor on ledges)."},
        {"10030f10", "Camera_SolveStep", "Zone check, then mode handler (+0x27c: 3 chase, 4 1st person, 5 snap)."},
        {"10030f80", "Camera_SolveContinue", null},
        {"10030aa0", "Camera_SmoothVertical", null},
        {"100310d0", "Camera_SetMode", null},
        {"1002fc30", "Camera_UpdateZone", null},
        {"10030120", "Camera_FindZone", "Highest-priority authored camera zone for player sector. Return 0 to disable fixed cams."},
        {"1002fcf0", "Camera_EnterZone", null},
        {"1002f310", "Camera_ModeChase", "Mode 3: follow while visible, else reposition."},
        {"1002f6d0", "Camera_ChaseCanSeeTarget", null},
        {"1002f750", "Camera_ChaseReposition", "Random spot ~768 out, 450-550 up, same side."},
        {"1002f840", "Camera_RandomOffset", null},
        {"1002def0", "Camera_Follow", null},
        {"1002e720", "Camera_FollowDesired", null},
        {"1002df60", "Camera_FollowSolve", null},
        {"1002e5c0", "Camera_ClampDistPitch", "Clamp dist [min,max] and pitch; max slope hard-coded 0x1126."},
        {"1002e950", "Camera_MoveCapped", "Camera move capped at 0x6e units/frame."},
        {"1002f380", "Camera_SetGoalAndFollow", null},
        {"1002f340", "Camera_SetGoal", null},
        {"10031190", "Camera_ModeFirstPerson", null},
        {"10031270", "Camera_FirstPersonLook", "Pitch +-200, yaw +-300 relative to body."},
        {"1002f570", "Camera_ReadLookInput", "GENERAL_UP/DOWN/LEFT/RIGHT actions -> look deltas."},
        {"10031a00", "Camera_ModeSnap", null},
        {"1002f420", "Camera_EnterFirstPerson", null},
        {"1002f3b0", "Camera_FirstPersonInit", null},
        {"100308d0", "Angle_ToCosSin", null},
    };

    private static final String[][] LABELS = {
        {"101f1170", "g_KeysDown"},
        {"101f1190", "g_KeysChanged"},
        {"100ddc28", "g_DIKeyboardDevice"},
        {"100e4c50", "g_MouseX"},
        {"100e4c54", "g_MouseY"},
        {"100e4c58", "g_MouseButtons"},
        {"100def10", "g_ActionTable"},
        {"100df9d8", "g_InputSourceTable"},
        {"101f4b30", "g_ActionBindings"},
        {"101f6b40", "g_ActionState"},
        {"100fd240", "g_TurnHeldFrames"},
        {"10235ea4", "g_PlayerActor"},
        {"101f11bc", "g_World"},
        {"101f11c0", "g_CameraObject"},
        {"101f11b4", "g_Viewport"},
        {"101044a0", "g_Cam"},
        {"10104620", "g_CamFlags"},
        {"10104640", "g_CamPreHook"},
        {"10104644", "g_CamAltUpdate"},
        {"1010460c", "g_CamMinDist"},
        {"10104610", "g_CamMaxDist"},
        {"10104614", "g_CamMinPitchSlope"},
        {"101f11f4", "g_LookSpeed"},
        {"101f11e4", "g_InvertLookX"},
        {"101f11e8", "g_InvertLookY"},
        {"10288550", "g_SinTable1024"},
    };

    @Override
    public void run() throws Exception {
        if (!currentProgram.getName().equalsIgnoreCase("DUNGEON.DLL")) {
            println("Skipping " + currentProgram.getName() + " (names are for DUNGEON.DLL)");
            return;
        }
        int fnDone = 0, lblDone = 0;
        for (String[] f : FUNCTIONS) {
            Address a = toAddr(f[0]);
            Function fn = getFunctionAt(a);
            if (fn == null) {
                CreateFunctionCmd cmd = new CreateFunctionCmd(a);
                if (cmd.applyTo(currentProgram, monitor)) {
                    fn = getFunctionAt(a);
                    println("Created function at " + a);
                }
            }
            if (fn == null) {
                println("WARN: no function at " + a + " for " + f[1]);
                continue;
            }
            fn.setName(f[1], SourceType.USER_DEFINED);
            if (f[2] != null) {
                currentProgram.getListing().setComment(a, CodeUnit.PLATE_COMMENT, f[2]);
            }
            fnDone++;
        }
        for (String[] l : LABELS) {
            createLabel(toAddr(l[0]), l[1], true, SourceType.USER_DEFINED);
            lblDone++;
        }
        println("Applied " + fnDone + "/" + FUNCTIONS.length + " function names, " + lblDone + " labels");
    }
}
