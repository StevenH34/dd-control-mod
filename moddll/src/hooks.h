#pragma once

// Placeholder for the real function hooks (Phase 3+: movement remap,
// Phase 4: fixed-camera fallback, Phase 5: aim mode, Phase 6: camera feel).
// For now this only brings MinHook up/down so later phases can call
// MH_CreateHook/MH_EnableHook once the target addresses are known from
// the Phase 1 Ghidra/x32dbg discovery work.

void Hooks_Init();
void Hooks_Shutdown();

// True = remapped controls active. False = fall back to stock tank controls.
// Toggled at runtime via Config_Get().toggleModHotkeyVk.
bool Hooks_IsModEnabled();
void Hooks_SetModEnabled(bool enabled);
