#pragma once

// Opens a console window attached to the host process and redirects
// stdout/stderr to it. Also mirrors every Log() line to OutputDebugString
// so a debugger can capture it even if the console is closed/hidden.
void Console_Init();
void Console_Shutdown();

// printf-style logging. Safe to call before Console_Init (falls back to
// OutputDebugString only).
void Log(const char* fmt, ...);
