#include "config.h"
#include "console.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace
{
    ModConfig g_config;

    std::string Trim(const std::string& s)
    {
        size_t begin = s.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1);
    }

    std::string GetModuleDirectory(HMODULE moduleHandle)
    {
        char path[MAX_PATH] = {};
        GetModuleFileNameA(moduleHandle, path, MAX_PATH);
        std::string full(path);
        size_t slash = full.find_last_of("\\/");
        return slash == std::string::npos ? "" : full.substr(0, slash + 1);
    }

    std::unordered_map<std::string, std::string> ParseIni(const std::string& filePath)
    {
        std::unordered_map<std::string, std::string> values;
        std::ifstream file(filePath);
        if (!file.is_open())
            return values;

        std::string line;
        while (std::getline(file, line))
        {
            std::string trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#' || trimmed[0] == '[')
                continue;

            size_t eq = trimmed.find('=');
            if (eq == std::string::npos)
                continue;

            std::string key = Trim(trimmed.substr(0, eq));
            std::string value = Trim(trimmed.substr(eq + 1));
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
            values[key] = value;
        }
        return values;
    }

    float GetFloat(const std::unordered_map<std::string, std::string>& values, const char* key, float def)
    {
        auto it = values.find(key);
        if (it == values.end())
            return def;
        try { return std::stof(it->second); } catch (...) { return def; }
    }

    int GetInt(const std::unordered_map<std::string, std::string>& values, const char* key, int def)
    {
        auto it = values.find(key);
        if (it == values.end())
            return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    }

    bool GetBool(const std::unordered_map<std::string, std::string>& values, const char* key, bool def)
    {
        auto it = values.find(key);
        if (it == values.end())
            return def;
        std::string v = it->second;
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });
        if (v == "1" || v == "true" || v == "yes" || v == "on")
            return true;
        if (v == "0" || v == "false" || v == "no" || v == "off")
            return false;
        return def;
    }

    void WriteDefaultIni(const std::string& filePath, const ModConfig& defaults)
    {
        std::ofstream file(filePath);
        if (!file.is_open())
            return;

        file << "; DD Modern Controls - tunable config. Edit and press the reload hotkey\n"
             << "; (default F9) in-game to apply changes without restarting.\n\n"
             << "turn_speed_deg_per_sec = " << defaults.turnSpeedDegPerSec << "\n"
             << "input_deadzone = " << defaults.inputDeadzone << "\n"
             << "run_speed_multiplier = " << defaults.runSpeedMultiplier << "\n\n"
             << "; Camera-relative movement (angles: 1024 = 360 degrees)\n"
             << "max_steer_walk = " << defaults.maxSteerWalk << "\n"
             << "max_steer_run = " << defaults.maxSteerRun << "\n"
             << "pivot_threshold_deg = " << defaults.pivotThresholdDeg << "\n"
             << "camera_yaw_offset = " << defaults.cameraYawOffset << "\n"
             << "invert_steer = " << (defaults.invertSteer ? 1 : 0) << "\n"
             << "steer_with_turn_bit = " << (defaults.steerWithTurnBit ? 1 : 0) << "\n\n"
             << "; 1 = in-game console window. Fullscreen: steals focus and freezes the\n"
             << "; game on the loading screen. Use DDMod.log / the injector window instead.\n"
             << "show_debug_console = " << (defaults.showDebugConsole ? 1 : 0) << "\n"
             << "verbose_logging = " << (defaults.verboseLogging ? 1 : 0) << "\n\n"
             << "; Virtual-key codes: https://learn.microsoft.com/windows/win32/inputdev/virtual-key-codes\n"
             << "reload_hotkey_vk = " << defaults.reloadHotkeyVk << "\n"
             << "toggle_mod_hotkey_vk = " << defaults.toggleModHotkeyVk << "\n";
    }
}

void Config_Load(HMODULE moduleHandle)
{
    std::string dir = GetModuleDirectory(moduleHandle);
    std::string path = dir + "DDModConfig.ini";

    std::ifstream probe(path);
    if (!probe.is_open())
    {
        WriteDefaultIni(path, ModConfig{});
        Log("[config] wrote default config to %s", path.c_str());
    }
    probe.close();

    auto values = ParseIni(path);

    ModConfig cfg;
    cfg.turnSpeedDegPerSec = GetFloat(values, "turn_speed_deg_per_sec", cfg.turnSpeedDegPerSec);
    cfg.inputDeadzone = GetFloat(values, "input_deadzone", cfg.inputDeadzone);
    cfg.runSpeedMultiplier = GetFloat(values, "run_speed_multiplier", cfg.runSpeedMultiplier);
    cfg.maxSteerWalk = GetInt(values, "max_steer_walk", cfg.maxSteerWalk);
    cfg.maxSteerRun = GetInt(values, "max_steer_run", cfg.maxSteerRun);
    cfg.pivotThresholdDeg = GetFloat(values, "pivot_threshold_deg", cfg.pivotThresholdDeg);
    cfg.cameraYawOffset = GetInt(values, "camera_yaw_offset", cfg.cameraYawOffset);
    cfg.invertSteer = GetBool(values, "invert_steer", cfg.invertSteer);
    cfg.steerWithTurnBit = GetBool(values, "steer_with_turn_bit", cfg.steerWithTurnBit);
    cfg.showDebugConsole = GetBool(values, "show_debug_console", cfg.showDebugConsole);
    cfg.verboseLogging = GetBool(values, "verbose_logging", cfg.verboseLogging);
    cfg.reloadHotkeyVk = GetInt(values, "reload_hotkey_vk", cfg.reloadHotkeyVk);
    cfg.toggleModHotkeyVk = GetInt(values, "toggle_mod_hotkey_vk", cfg.toggleModHotkeyVk);

    g_config = cfg;
    Log("[config] loaded from %s (steer walk=%d run=%d pivot=%.0fdeg cam_offset=%d invert=%d turn_bit=%d)",
        path.c_str(), g_config.maxSteerWalk, g_config.maxSteerRun, g_config.pivotThresholdDeg,
        g_config.cameraYawOffset, g_config.invertSteer ? 1 : 0, g_config.steerWithTurnBit ? 1 : 0);
}

const ModConfig& Config_Get()
{
    return g_config;
}
