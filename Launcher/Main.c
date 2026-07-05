#include <Windows.h>
#include <strsafe.h>
#include <string.h>

static BOOL GetModuleDirectory(LPWSTR path, DWORD cchPath) {
    DWORD length = GetModuleFileNameW(NULL, path, cchPath);
    if (length == 0 || length >= cchPath) {
        return FALSE;
    }

    WCHAR* slash = wcsrchr(path, L'\\');
    if (!slash) {
        return FALSE;
    }

    slash[1] = L'\0';
    return TRUE;
}

static BOOL GetExecutableDirectory(LPCWSTR exePath, LPWSTR directory, DWORD cchDirectory) {
    if (!exePath || !directory || cchDirectory == 0) {
        return FALSE;
    }

    if (wcsncpy_s(directory, cchDirectory, exePath, _TRUNCATE) != 0) {
        return FALSE;
    }

    WCHAR* slash = wcsrchr(directory, L'\\');
    if (!slash) {
        return FALSE;
    }

    if (slash == directory + 2 && directory[1] == L':') {
        slash[1] = L'\0';
    } else {
        *slash = L'\0';
    }
    return TRUE;
}

static BOOL BuildPayloadPath(LPWSTR path, DWORD cchPath) {
    if (!GetModuleDirectory(path, cchPath)) {
        return FALSE;
    }

#ifdef _WIN64
    const wchar_t* payload = L"Anchor64.dll";
#else
    const wchar_t* payload = L"Anchor32.dll";
#endif // _WIN64

    return wcscat_s(path, cchPath, payload) == 0;
}

static LPWSTR BuildCommandLine(LPCWSTR exePath) {
    size_t length = wcslen(exePath);
    LPWSTR commandLine = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (length + 3) * sizeof(WCHAR));
    if (!commandLine) {
        return NULL;
    }

    commandLine[0] = L'"';
    memcpy(commandLine + 1, exePath, length * sizeof(WCHAR));
    commandLine[length + 1] = L'"';
    commandLine[length + 2] = L'\0';
    return commandLine;
}

static BOOL LoadRemoteLibrary(HANDLE hProcess, LPVOID lpRemoteDllWideNameAddr) {
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) {
        return FALSE;
    }
    LPTHREAD_START_ROUTINE threadFun = (LPTHREAD_START_ROUTINE) GetProcAddress(kernel32, "LoadLibraryW");
    HANDLE thread = CreateRemoteThread(hProcess, NULL, 0, threadFun, lpRemoteDllWideNameAddr, 0, NULL);
    if (!thread) {
        return FALSE;
    }
    WaitForSingleObject(thread, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode); // The 64-bit address is truncated to 32 bits (The 64-bit user program starts at 7FFE0000)
    CloseHandle(thread);
    return !!exitCode; // Convert non-boolean value to 0 or 1
}

static BOOL InjectIntoProcess(HANDLE hProcess, LPCWSTR lpDllName) {
    size_t dllNameCbSize = 0;
    if (StringCbLengthW(lpDllName, MAX_PATH * sizeof(WCHAR), &dllNameCbSize) != S_OK) {
        return FALSE;
    }
    dllNameCbSize += sizeof(WCHAR);
    BOOL ret = FALSE;
    LPVOID dllNameRemoteAddr = VirtualAllocEx(hProcess, 0, dllNameCbSize, MEM_COMMIT, PAGE_READWRITE);
    if (dllNameRemoteAddr) {
        size_t written = 0;
        if (WriteProcessMemory(hProcess, dllNameRemoteAddr, lpDllName, dllNameCbSize, &written) &&
            written == dllNameCbSize) {
            ret = LoadRemoteLibrary(hProcess, dllNameRemoteAddr);
        }
        VirtualFreeEx(hProcess, dllNameRemoteAddr, 0, MEM_RELEASE);
    }
    return ret;
}

int wmain(int argc, wchar_t** argv) {
    /*
        Arg1: The full path of the executable file to launch.
        Payload.dll is loaded from the launcher executable directory.
    */
    if (argc != 2) {
        MessageBoxW(NULL, L"Usage: WPSInstallerLauncher.exe <full-path-to-exe>", L"Error", MB_ICONERROR);
        return 1;
    }

    WCHAR payloadPath[MAX_PATH] = { 0 };
    if (!BuildPayloadPath(payloadPath, ARRAYSIZE(payloadPath))) {
        MessageBoxW(NULL, L"Failed to locate Payload.dll.", L"Error", MB_ICONERROR);
        return 1;
    }

    WCHAR currentDirectory[MAX_PATH] = { 0 };
    if (!GetExecutableDirectory(argv[1], currentDirectory, ARRAYSIZE(currentDirectory))) {
        MessageBoxW(NULL, L"Invalid executable path.", L"Error", MB_ICONERROR);
        return 1;
    }

    LPWSTR commandLine = BuildCommandLine(argv[1]);
    if (!commandLine) {
        MessageBoxW(NULL, L"Failed to allocate command line.", L"Error", MB_ICONERROR);
        return 1;
    }

    STARTUPINFOW mainSi = { 0 };
    PROCESS_INFORMATION mainPi = { 0 };
    mainSi.cb = sizeof(mainSi);

    if (!CreateProcessW(argv[1], commandLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, currentDirectory, &mainSi, &mainPi)) {
        HeapFree(GetProcessHeap(), 0, commandLine);
        MessageBoxW(NULL, L"Process creation failed.", L"Error", MB_ICONERROR);
        return 1;
    }

    HeapFree(GetProcessHeap(), 0, commandLine);

    int exitCode = 0;
    if (!InjectIntoProcess(mainPi.hProcess, payloadPath)) {
        exitCode = 1;
        TerminateProcess(mainPi.hProcess, 0);
        MessageBoxW(NULL, L"Injection failed.", L"Error", MB_ICONERROR);
    } else if (ResumeThread(mainPi.hThread) == (DWORD) -1) {
        exitCode = 1;
        TerminateProcess(mainPi.hProcess, 0);
        MessageBoxW(NULL, L"Process resume failed.", L"Error", MB_ICONERROR);
    }

    CloseHandle(mainPi.hProcess);
    CloseHandle(mainPi.hThread);
    return exitCode;
}
