#pragma once

// Brings MinHook up/down and installs the game hooks. Installed so far:
// movement remap (Player_ReadControls, see movement.h). Later: camera.

void Hooks_Init();
void Hooks_Shutdown();

// True = remapped controls active. False = fall back to stock tank controls.
// Toggled at runtime via Config_Get().toggleModHotkeyVk.
bool Hooks_IsModEnabled();
void Hooks_SetModEnabled(bool enabled);
