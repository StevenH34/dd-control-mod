#pragma once

#include <windows.h>

// Tunable constants for the control-remap logic. Values are loaded from
// DDModConfig.ini, placed next to this DLL. Every field has a sane default
// so a missing/partial ini file still works.
struct ModConfig
{
    // Movement
    float turnSpeedDegPerSec = 720.0f;  // how fast the character reorients to face the input direction
    float inputDeadzone = 0.15f;        // ignored for keyboard now; kept for future gamepad support
    float runSpeedMultiplier = 1.6f;    // matches original "A + direction" run modifier

    // Debug / iteration
    bool showDebugConsole = true;
    bool verboseLogging = false;

    // Hotkey (virtual-key code) to hot-reload this file at runtime without restarting the game.
    int reloadHotkeyVk = VK_F9;

    // Hotkey to toggle the mod on/off, falling back to stock tank controls.
    int toggleModHotkeyVk = VK_F10;
};

// Loads (or reloads) DDModConfig.ini from the directory containing this DLL.
// Creates the file with defaults if it doesn't exist yet.
void Config_Load(HMODULE moduleHandle);

// Current live config values. Safe to read from any thread after Config_Load
// has been called once; Config_Load itself should only be called from the
// init thread / on an explicit reload hotkey, not concurrently.
const ModConfig& Config_Get();
