#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <memory>
#include <span>

namespace WPSProfileVerificationPatch {
    class VersionUtil {
    private:
        VersionUtil() = delete;

    public:
        static std::unique_ptr<const uint8_t[]> GetVersionInfoDataA(const std::string& filePath);
        static std::unique_ptr<const uint8_t[]> GetVersionInfoDataW(const std::wstring& filePath);
        static std::optional<std::span<const uint8_t>> QueryVersionInfoValueA(const std::unique_ptr<const uint8_t[]>& versionInfoData, const std::string& subBlock);
        static std::optional<std::span<const uint8_t>> QueryVersionInfoValueW(const std::unique_ptr<const uint8_t[]>& versionInfoData, const std::wstring& subBlock);
    };
}
