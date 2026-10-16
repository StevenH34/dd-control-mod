// DDModInjector - loads DDModCore.dll into a running DD_CD.EXE process.
//
// Usage:
//   DDModInjector.exe [target_exe_name] [dll_path]
//
// With no args, waits for "DD_CD.EXE" to appear and injects "DDModCore.dll"
// from the injector's own directory. Polls every 500ms for up to 30s.
// Instances with a debugger attached are skipped, and when one is already
// running (other than a single plain instance) it waits for a new one.

#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
    std::vector<DWORD> FindProcessIds(const std::wstring& processName)
    {
        std::vector<DWORD> pids;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return pids;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0)
                    pids.push_back(entry.th32ProcessID);
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return pids;
    }

    // A game instance held by a debugger (e.g. a leftover x32dbg session)
    // is never the one the user wants to play.
    bool IsBeingDebugged(DWORD pid)
    {
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process)
            return false;
        BOOL debugged = FALSE;
        CheckRemoteDebuggerPresent(process, &debugged);
        CloseHandle(process);
        return debugged != FALSE;
    }

    bool Contains(const std::vector<DWORD>& pids, DWORD pid)
    {
        return std::find(pids.begin(), pids.end(), pid) != pids.end();
    }

    // Prints lines appended to the mod's log file until the game exits.
    void TailLog(DWORD pid, const std::wstring& logPath)
    {
        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (!process)
            return;

        LONGLONG offset = 0;
        char buf[4096];
        for (;;)
        {
            HANDLE file = CreateFileW(logPath.c_str(), GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER size{};
                GetFileSizeEx(file, &size);
                if (size.QuadPart < offset)
                    offset = 0;  // truncated by a new run
                LARGE_INTEGER pos{};
                pos.QuadPart = offset;
                SetFilePointerEx(file, pos, nullptr, FILE_BEGIN);
                DWORD read = 0;
                while (ReadFile(file, buf, sizeof(buf), &read, nullptr) && read > 0)
                {
                    fwrite(buf, 1, read, stdout);
                    offset += read;
                }
                fflush(stdout);
                CloseHandle(file);
            }

            if (WaitForSingleObject(process, 100) != WAIT_TIMEOUT)
                break;
        }
        CloseHandle(process);
    }

    std::vector<DWORD> Eligible(const std::vector<DWORD>& pids)
    {
        std::vector<DWORD> result;
        for (DWORD pid : pids)
            if (!IsBeingDebugged(pid))
                result.push_back(pid);
        return result;
    }

    std::wstring GetSelfDirectory()
    {
        wchar_t path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring full(path);
        size_t slash = full.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"" : full.substr(0, slash + 1);
    }

    bool InjectDll(DWORD pid, const std::wstring& dllPath)
    {
        HANDLE process = OpenProcess(
            PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_WRITE | PROCESS_VM_READ,
            FALSE, pid);
        if (!process)
        {
            wprintf(L"OpenProcess failed (pid %lu, error %lu)\n", pid, GetLastError());
            return false;
        }

        size_t bufSize = (dllPath.size() + 1) * sizeof(wchar_t);
        LPVOID remoteMem = VirtualAllocEx(process, nullptr, bufSize, MEM_COMMIT, PAGE_READWRITE);
        if (!remoteMem)
        {
            wprintf(L"VirtualAllocEx failed (error %lu)\n", GetLastError());
            CloseHandle(process);
            return false;
        }

        if (!WriteProcessMemory(process, remoteMem, dllPath.c_str(), bufSize, nullptr))
        {
            wprintf(L"WriteProcessMemory failed (error %lu)\n", GetLastError());
            VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
            CloseHandle(process);
            return false;
        }

        LPTHREAD_START_ROUTINE loadLibraryAddr =
            reinterpret_cast<LPTHREAD_START_ROUTINE>(
                GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

        HANDLE remoteThread = CreateRemoteThread(
            process, nullptr, 0, loadLibraryAddr, remoteMem, 0, nullptr);
        if (!remoteThread)
        {
            wprintf(L"CreateRemoteThread failed (error %lu)\n", GetLastError());
            VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
            CloseHandle(process);
            return false;
        }

        WaitForSingleObject(remoteThread, INFINITE);

        DWORD loadedModule = 0;
        GetExitCodeThread(remoteThread, &loadedModule);

        VirtualFreeEx(process, remoteMem, 0, MEM_RELEASE);
        CloseHandle(remoteThread);
        CloseHandle(process);

        if (loadedModule == 0)
        {
            wprintf(L"LoadLibraryW returned NULL in the target process - injection failed "
                    L"(architecture mismatch? wrong path? missing dependency?)\n");
            return false;
        }
        return true;
    }
}

int wmain(int argc, wchar_t* argv[])
{
    std::wstring targetProcess = (argc > 1) ? argv[1] : L"DD_CD.EXE";
    std::wstring dllPath = (argc > 2) ? argv[2] : (GetSelfDirectory() + L"DDModCore.dll");

    // Instances already running when the injector starts. If there's exactly
    // one and no debugger is attached, inject into it right away (game
    // launched first). Otherwise wait for a new instance to appear, so a
    // stale or debugger-held copy never gets the mod by accident.
    std::vector<DWORD> existing = FindProcessIds(targetProcess);
    for (DWORD p : existing)
        wprintf(L"Already running: %s pid %lu%s\n", targetProcess.c_str(), p,
                IsBeingDebugged(p) ? L" (debugger attached - skipped)" : L"");

    std::vector<DWORD> existingEligible = Eligible(existing);
    DWORD pid = 0;
    if (existing.size() == 1 && existingEligible.size() == 1)
    {
        pid = existingEligible[0];
    }
    else
    {
        wprintf(L"DDModInjector: waiting for a new %s ...\n", targetProcess.c_str());
        for (int attempt = 0; attempt < 60 && pid == 0; ++attempt)
        {
            for (DWORD p : Eligible(FindProcessIds(targetProcess)))
                if (!Contains(existing, p))
                    pid = p;
            if (pid == 0)
                Sleep(500);
        }

        if (pid == 0)
        {
            if (existingEligible.size() == 1)
            {
                pid = existingEligible[0];
            }
            else if (existingEligible.size() > 1)
            {
                wprintf(L"Several %s instances are running and none is new. Close the extra "
                        L"ones and try again.\n", targetProcess.c_str());
                return 1;
            }
            else
            {
                wprintf(L"Timed out waiting for %s\n", targetProcess.c_str());
                return 1;
            }
        }
    }

    wprintf(L"Found %s (pid %lu). Injecting %s ...\n", targetProcess.c_str(), pid, dllPath.c_str());

    // The mod logs to DDMod.log next to the DLL instead of opening a window
    // in the game. Delete the previous run's log so it isn't replayed.
    size_t slash = dllPath.find_last_of(L"\\/");
    std::wstring logPath = (slash == std::wstring::npos ? L"" : dllPath.substr(0, slash + 1)) + L"DDMod.log";
    DeleteFileW(logPath.c_str());

    if (!InjectDll(pid, dllPath))
        return 1;

    wprintf(L"Injected successfully. Showing %s until the game exits (Ctrl+C to stop).\n\n",
            logPath.c_str());
    TailLog(pid, logPath);
    wprintf(L"\nGame exited.\n");
    return 0;
}
