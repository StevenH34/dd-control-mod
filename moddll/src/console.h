#pragma once

#include <windows.h>

// Opens DDMod.log next to this DLL (truncated each run). Every Log() line is
// written there; DDModInjector tails it in its own console window.
void Log_OpenFile(HMODULE moduleHandle);

// Opens a console window attached to the host process and redirects
// stdout/stderr to it. WARNING: in fullscreen (nGlide) this steals focus,
// the game window minimizes, and the game spins forever in its DirectInput
// re-acquire loop (0x10050d50) on the loading screen. Only for windowed
// debugging; off by default (show_debug_console).
void Console_Init();
void Console_Shutdown();

// printf-style logging to DDMod.log, the console (if open) and
// OutputDebugString. Thread-safe. Safe to call before Log_OpenFile.
void Log(const char* fmt, ...);
