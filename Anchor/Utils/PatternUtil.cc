#include <stdexcept>
#include <optional>
#include <vector>
#include "PatternUtil.h"

namespace WPSProfileVerificationPatch {
    std::vector<const uint8_t*> PatternUtil::FindPattern(
        std::span<const uint8_t> data,
        std::span<const uint16_t> pattern,
        size_t index,
        bool reverse,
        size_t maxMatches
    ) {
        if (data.empty()) {
            throw std::invalid_argument("Data cannot be empty");
        }
        if (pattern.empty()) {
            throw std::invalid_argument("Pattern cannot be empty");
        }
        if (index > data.size()) {
            throw std::invalid_argument("Index out of range");
        }
        if (maxMatches == 0) {
            throw std::invalid_argument("Max matches cannot be zero");
        }

        std::vector<const uint8_t*> matches;
        if (reverse) {
            if (data.size() < pattern.size()) {
                return matches;
            }

            size_t start = std::min(data.size() - pattern.size(), index);
            for (size_t i = start; ; --i) {
                bool match = true;
                for (size_t j = 0; j < pattern.size(); ++j) {
                    if (pattern[j] <= 0xFF && pattern[j] != data[i + j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    matches.push_back(data.data() + i);
                    if (matches.size() >= maxMatches) {
                        break;
                    }
                }
                if (i == 0) {
                    break;
                }
            }
        } else {
            size_t end = data.size() - pattern.size();
            for (size_t i = index; i <= end; ++i) {
                bool match = true;
                for (size_t j = 0; j < pattern.size(); ++j) {
                    if (pattern[j] <= 0xFF && pattern[j] != data[i + j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    matches.push_back(data.data() + i);

                    if (matches.size() >= maxMatches) {
                        break;
                    }
                }
            }
        }
        return matches;
    }

    std::vector<const uint8_t*> PatternUtil::FindPatternInRegions(
        const std::vector<std::span<const uint8_t>>& regions,
        std::span<const uint16_t> pattern,
        size_t index,
        bool reverse,
        size_t maxMatches
    ) {
        if (maxMatches == 0)
            throw std::invalid_argument("Max matches cannot be zero");
        if (pattern.empty())
            throw std::invalid_argument("Pattern cannot be empty");

        std::vector<const uint8_t*> allMatches;

        if (reverse) {
            for (auto it = regions.rbegin(); it != regions.rend(); ++it) {
                auto& region = *it;
                if (region.empty()) {
                    continue;
                }

                auto matches = FindPattern(region, pattern, region.size(), true, maxMatches - allMatches.size());
                allMatches.insert(allMatches.end(), matches.begin(), matches.end());

                if (allMatches.size() >= maxMatches) {
                    break;
                }
            }
        } else {
            for (const auto& region : regions) {
                if (region.empty()) {
                    continue;
                }

                size_t startIndex = 0;
                auto matches = FindPattern(region, pattern, startIndex, false, maxMatches - allMatches.size());
                allMatches.insert(allMatches.end(), matches.begin(), matches.end());

                if (allMatches.size() >= maxMatches) {
                    break;
                }
            }
        }

        return allMatches;
    }

    std::optional<std::span<const uint8_t>> PatternUtil::FindRegionContaining(
        const std::vector<std::span<const uint8_t>>& regions,
        const uint8_t* address
    ) {
        if (!address) {
            return std::nullopt;
        }

        uintptr_t addr = reinterpret_cast<uintptr_t>(address);
        for (const auto& region : regions) {
            if (region.empty()) {
                continue;
            }

            uintptr_t start = reinterpret_cast<uintptr_t>(region.data());
            uintptr_t end = start + region.size();

            if (addr >= start && addr < end) {
                return region;
            }
        }

        return std::nullopt;
    }
}
