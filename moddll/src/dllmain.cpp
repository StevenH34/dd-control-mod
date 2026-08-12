#include <windows.h>
#include <atomic>

#include "console.h"
#include "config.h"
#include "hooks.h"

namespace
{
    HMODULE g_selfModule = nullptr;
    std::atomic<bool> g_shutdown{ false };

    // Edge-detects a key going from up to down so holding it doesn't repeat-fire.
    bool KeyPressedEdge(int vk, bool& wasDown)
    {
        bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
        bool pressed = isDown && !wasDown;
        wasDown = isDown;
        return pressed;
    }

    DWORD WINAPI MainThread(LPVOID param)
    {
        HMODULE self = static_cast<HMODULE>(param);

        Config_Load(self);
        const ModConfig& cfg = Config_Get();

        if (cfg.showDebugConsole)
            Console_Init();

        Log("==================================================");
        Log(" Deathtrap Dungeon - Modern Controls mod");
        Log(" F9  = reload DDModConfig.ini");
        Log(" F10 = toggle modern controls / stock tank controls");
        Log("==================================================");

        Hooks_Init();

        bool reloadWasDown = false;
        bool toggleWasDown = false;

        while (!g_shutdown.load())
        {
            const ModConfig& live = Config_Get();

            if (KeyPressedEdge(live.reloadHotkeyVk, reloadWasDown))
                Config_Load(self);

            if (KeyPressedEdge(live.toggleModHotkeyVk, toggleWasDown))
                Hooks_SetModEnabled(!Hooks_IsModEnabled());

            Sleep(50);
        }

        Hooks_Shutdown();
        Console_Shutdown();
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*reserved*/)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_selfModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;

    case DLL_PROCESS_DETACH:
        g_shutdown.store(true);
        break;
    }
    return TRUE;
}
