#pragma once
#include <Windows.h>
#include <string>

namespace RemoteLibrary {
    bool Load(HANDLE process, const std::wstring& dllPath);
}
