#include <Windows.h>
#include "CommandLineBuilder.h"
#include "LauncherPaths.h"
#include "RemoteLibrary.h"
#include "WinHandle.h"

int wmain(int argc, wchar_t** argv) {
    /*
        Arg1: The executable path to launch.
        Arg2...: Optional arguments passed to the launched executable.
        Anchor.dll is loaded from the launcher executable directory.
    */
    if (argc < 2) {
        MessageBoxW(nullptr, L"Usage: Launcher.exe <exe-path> [arguments...]", L"Error", MB_ICONERROR);
        return 1;
    }

    std::wstring anchorPath;
    std::wstring launcherDirectory;
    try {
        anchorPath = LauncherPaths::BuildAnchorPath();
        launcherDirectory = LauncherPaths::GetLauncherDirectory();
    } catch (...) {
        MessageBoxW(nullptr, L"Failed to locate launcher files.", L"Error", MB_ICONERROR);
        return 1;
    }

    const std::wstring commandLine = CommandLineBuilder::Build(argc, argv);

    STARTUPINFOW startupInfo = {};
    PROCESS_INFORMATION processInfo = {};
    startupInfo.cb = sizeof(startupInfo);

    if (!CreateProcessW(argv[1], const_cast<LPWSTR>(commandLine.data()), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, launcherDirectory.c_str(), &startupInfo, &processInfo)) {
        MessageBoxW(nullptr, L"Process creation failed.", L"Error", MB_ICONERROR);
        return 1;
    }

    WinHandle process(processInfo.hProcess);
    WinHandle thread(processInfo.hThread);

    int exitCode = 0;
    if (!RemoteLibrary::Load(process.get(), anchorPath)) {
        exitCode = 1;
        TerminateProcess(process.get(), 0);
        MessageBoxW(nullptr, L"Injection failed.", L"Error", MB_ICONERROR);
    } else if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) {
        exitCode = 1;
        TerminateProcess(process.get(), 0);
        MessageBoxW(nullptr, L"Process resume failed.", L"Error", MB_ICONERROR);
    }

    return exitCode;
}
