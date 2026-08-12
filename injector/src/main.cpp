// DDModInjector - loads DDModCore.dll into a running DD_CD.EXE process.
//
// Usage:
//   DDModInjector.exe [target_exe_name] [dll_path]
//
// With no args, waits for "DD_CD.EXE" to appear and injects "DDModCore.dll"
// from the injector's own directory. Polls every 500ms for up to 30s.

#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <string>

namespace
{
    DWORD FindProcessId(const std::wstring& processName)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);

        DWORD pid = 0;
        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0)
                {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return pid;
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

    wprintf(L"DDModInjector: waiting for %s ...\n", targetProcess.c_str());

    DWORD pid = 0;
    for (int attempt = 0; attempt < 60; ++attempt)
    {
        pid = FindProcessId(targetProcess);
        if (pid != 0)
            break;
        Sleep(500);
    }

    if (pid == 0)
    {
        wprintf(L"Timed out waiting for %s\n", targetProcess.c_str());
        return 1;
    }

    wprintf(L"Found %s (pid %lu). Injecting %s ...\n", targetProcess.c_str(), pid, dllPath.c_str());

    if (!InjectDll(pid, dllPath))
        return 1;

    wprintf(L"Injected successfully.\n");
    return 0;
}
