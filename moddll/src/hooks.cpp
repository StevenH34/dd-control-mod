#include "hooks.h"
#include "console.h"
#include "movement.h"

#include <MinHook.h>
#include <atomic>

namespace
{
    std::atomic<bool> g_modEnabled{ true };
}

void Hooks_Init()
{
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        Log("[hooks] MH_Initialize failed: %s", MH_StatusToString(status));
        return;
    }
    Log("[hooks] MinHook initialized");

    if (!Movement_InstallHook())
        Log("[hooks] movement hook failed - game keeps stock tank controls");
}

void Hooks_Shutdown()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_STATUS status = MH_Uninitialize();
    if (status != MH_OK)
        Log("[hooks] MH_Uninitialize failed: %s", MH_StatusToString(status));
}

bool Hooks_IsModEnabled()
{
    return g_modEnabled.load();
}

void Hooks_SetModEnabled(bool enabled)
{
    bool previous = g_modEnabled.exchange(enabled);
    if (previous != enabled)
        Log("[hooks] mod %s", enabled ? "ENABLED (modern controls)" : "DISABLED (stock tank controls)");
}
