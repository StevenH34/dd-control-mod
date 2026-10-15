#pragma once

// Camera-relative movement: hooks Player_ReadControls (DUNGEON.DLL +0x871b0)
// and rewrites the movement part of the player's command word each frame.
// See README.md "Movement hook" and docs/RE-CONTROLS-CAMERA.md sections 2, 4, 5.

// Waits for DUNGEON.DLL to load, then creates and enables the hook.
// Requires MH_Initialize to have succeeded. Returns false on failure.
bool Movement_InstallHook();
