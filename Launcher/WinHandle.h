#pragma once
#include <Windows.h>

class WinHandle {
public:
    WinHandle() = default;

    explicit WinHandle(HANDLE handle) noexcept
        : _handle(handle) {
    }

    ~WinHandle() {
        reset();
    }

    WinHandle(const WinHandle&) = delete;
    WinHandle& operator=(const WinHandle&) = delete;

    WinHandle(WinHandle&& other) noexcept
        : _handle(other.release()) {
    }

    WinHandle& operator=(WinHandle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    HANDLE get() const noexcept {
        return _handle;
    }

    HANDLE release() noexcept {
        HANDLE handle = _handle;
        _handle = nullptr;
        return handle;
    }

    void reset(HANDLE handle = nullptr) noexcept {
        if (_handle && _handle != INVALID_HANDLE_VALUE) {
            CloseHandle(_handle);
        }
        _handle = handle;
    }

private:
    HANDLE _handle = nullptr;
};
