#include "console.h"

#include <windows.h>
#include <cstdio>
#include <cstdarg>

static bool g_consoleOpen = false;

void Console_Init()
{
    if (g_consoleOpen)
        return;

    if (AllocConsole())
    {
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        SetConsoleTitleA("Deathtrap Dungeon - Modern Controls (debug)");
        g_consoleOpen = true;
    }
}

void Console_Shutdown()
{
    if (g_consoleOpen)
    {
        FreeConsole();
        g_consoleOpen = false;
    }
}

void Log(const char* fmt, ...)
{
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (g_consoleOpen)
    {
        fputs(buf, stdout);
        fputs("\n", stdout);
        fflush(stdout);
    }

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}
