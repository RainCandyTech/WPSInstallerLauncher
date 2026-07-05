#include "RemoteLibrary.h"
#include "WinHandle.h"

namespace {
    class RemoteAllocation {
    public:
        RemoteAllocation(HANDLE process, SIZE_T size)
            : _process(process),
              _address(VirtualAllocEx(process, nullptr, size, MEM_COMMIT, PAGE_READWRITE)) {
        }

        ~RemoteAllocation() {
            if (_address) {
                VirtualFreeEx(_process, _address, 0, MEM_RELEASE);
            }
        }

        RemoteAllocation(const RemoteAllocation&) = delete;
        RemoteAllocation& operator=(const RemoteAllocation&) = delete;

        LPVOID get() const noexcept {
            return _address;
        }

    private:
        HANDLE _process = nullptr;
        LPVOID _address = nullptr;
    };

    bool StartRemoteLoadLibrary(HANDLE process, LPVOID remoteDllPath) {
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!kernel32) {
            return false;
        }

        auto threadProc = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel32, "LoadLibraryW"));
        if (!threadProc) {
            return false;
        }

        WinHandle thread(CreateRemoteThread(process, nullptr, 0, threadProc, remoteDllPath, 0, nullptr));
        if (!thread.get()) {
            return false;
        }

        WaitForSingleObject(thread.get(), INFINITE);

        DWORD exitCode = 0;
        GetExitCodeThread(thread.get(), &exitCode);
        return exitCode != 0;
    }
}

namespace RemoteLibrary {
    bool Load(HANDLE process, const std::wstring& dllPath) {
        const SIZE_T dllPathSize = (dllPath.size() + 1) * sizeof(wchar_t);
        RemoteAllocation remoteDllPath(process, dllPathSize);
        if (!remoteDllPath.get()) {
            return false;
        }

        SIZE_T written = 0;
        if (!WriteProcessMemory(process, remoteDllPath.get(), dllPath.c_str(), dllPathSize, &written) ||
            written != dllPathSize) {
            return false;
        }

        return StartRemoteLoadLibrary(process, remoteDllPath.get());
    }
}
