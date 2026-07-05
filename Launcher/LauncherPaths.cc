#include "LauncherPaths.h"
#include <Windows.h>
#include <stdexcept>

namespace LauncherPaths {
    std::wstring GetLauncherDirectory() {
        std::wstring path(MAX_PATH, L'\0');
        DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0 || length >= path.size()) {
            throw std::runtime_error("Failed to get launcher path");
        }

        path.resize(length);

        size_t slash = path.find_last_of(L'\\');
        if (slash == std::wstring::npos) {
            throw std::runtime_error("Launcher path does not contain a directory");
        }

        path.resize(slash + 1);
        return path;
    }

    std::wstring BuildAnchorPath() {
#ifdef _WIN64
        constexpr wchar_t anchorName[] = L"Anchor64.dll";
#else
        constexpr wchar_t anchorName[] = L"Anchor32.dll";
#endif

        return GetLauncherDirectory() + anchorName;
    }
}
