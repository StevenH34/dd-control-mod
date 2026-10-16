#include "console.h"

#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

static bool g_consoleOpen = false;
static HANDLE g_logFile = INVALID_HANDLE_VALUE;
static SRWLOCK g_logLock = SRWLOCK_INIT;

void Log_OpenFile(HMODULE moduleHandle)
{
    if (g_logFile != INVALID_HANDLE_VALUE)
        return;

    char path[MAX_PATH] = {};
    GetModuleFileNameA(moduleHandle, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (!slash || (slash - path) + sizeof("DDMod.log") >= MAX_PATH)
        return;
    strcpy_s(slash + 1, MAX_PATH - (slash + 1 - path), "DDMod.log");

    // Truncated each run. FILE_SHARE_READ so the injector can tail it.
    g_logFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

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
    if (g_logFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_logFile);
        g_logFile = INVALID_HANDLE_VALUE;
    }
}

void Log(const char* fmt, ...)
{
    char buf[1024];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf) - 2, fmt, args);
    va_end(args);
    if (len < 0)
        return;
    if (len > static_cast<int>(sizeof(buf)) - 3)
        len = static_cast<int>(sizeof(buf)) - 3;

    // Called from both the mod thread and the game thread (movement detour).
    AcquireSRWLockExclusive(&g_logLock);

    if (g_consoleOpen)
    {
        fputs(buf, stdout);
        fputs("\n", stdout);
        fflush(stdout);
    }

    if (g_logFile != INVALID_HANDLE_VALUE)
    {
        buf[len] = '\r';
        buf[len + 1] = '\n';
        DWORD written = 0;
        WriteFile(g_logFile, buf, static_cast<DWORD>(len + 2), &written, nullptr);
        buf[len] = '\0';
    }

    ReleaseSRWLockExclusive(&g_logLock);

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}
