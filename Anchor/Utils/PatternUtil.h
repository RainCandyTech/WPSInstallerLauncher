#pragma once
#include <optional>
#include <vector>
#include <span>

namespace WPSProfileVerificationPatch {
    class PatternUtil {
    private:
        PatternUtil() = delete;

    public:
        static std::vector<const uint8_t*> FindPattern(
            std::span<const uint8_t> data,
            std::span<const uint16_t> pattern,
            size_t index = 0,
            bool reverse = false,
            size_t maxMatches = SIZE_MAX
        );
        static std::vector<const uint8_t*> FindPatternInRegions(
            const std::vector<std::span<const uint8_t>>& regions,
            std::span<const uint16_t> pattern,
            size_t index = 0,
            bool reverse = false,
            size_t maxMatches = SIZE_MAX
        );
        static std::optional<std::span<const uint8_t>> FindRegionContaining(
            const std::vector<std::span<const uint8_t>>& regions,
            const uint8_t* address
        );
    };
}
