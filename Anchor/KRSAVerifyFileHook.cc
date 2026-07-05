#include <Windows.h>
#include <stdexcept>
#include <array>
#include <vector>
#include "Detours.h"
#include "KRSAVerifyFileHook.h"
#include "ModuleUtil.h"
#include "PatternUtil.h"

namespace WPSProfileVerificationPatch {
    static bool (*_kRSAVerifyFile)(const std::string& publicKey, const std::string& fileHash, const std::string& fileSignature) = nullptr;

    static bool KRSAVerifyFile(const std::string& publicKey, const std::string& fileHash, const std::string& fileSignature) {
        for (char c : fileSignature) {
            if (c != '0') {
                return _kRSAVerifyFile(publicKey, fileHash, fileSignature);
            }
        }
        return true;
    }

    static void LocateTargetInRegion(std::span<const uint8_t> region) {
#if defined DETOURS_ARM64
        const std::array<uint16_t, 18> anchor = { 0x00, 0xD0, 0xFFFF, 0xFFFF, 0xFFFF, 0x91, 0xFFFF, 0xFFFF, 0x00, 0xD0, 0xFFFF, 0xFFFF, 0xFFFF, 0x91, 0xFFFF, 0x5A, 0x00, 0xA9 };
        const std::array<uint16_t, 4> prologue = { 0xFD, 0xFFFF, 0xFFFF, 0xA9 };
#elif defined DETOURS_X64
        const std::array<uint16_t, 21> anchor = { 0x4C, 0x8D, 0x3D, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x4C, 0x89, 0x3F, 0x4C, 0x8D, 0x25, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x4C, 0x89, 0x67, 0x08 };
        const std::array<uint16_t, 3> prologue = { 0x40, 0x53, 0x56 };
#elif defined DETOURS_X86
        const std::array<uint16_t, 25> anchor = { 0xC7, 0x06, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xC7, 0x46, 0x04, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xEB, 0x02, 0x33, 0xF6, 0x83, 0x7F, 0x14, 0x10, 0xC6, 0x45, 0xFC, 0x00 };
        const std::array<uint16_t, 3> prologue = { 0x55, 0x8B, 0xEC };
#else
#error "Unsupported architecture"
#endif

        std::vector<const uint8_t*> anchors = PatternUtil::FindPattern(region, anchor, 0, false, 1);
        if (anchors.empty()) {
            throw std::runtime_error("Failed to find KRSAVerifyFile anchor");
        }

        std::vector<const uint8_t*> prologues = PatternUtil::FindPattern(region, prologue, anchors[0] - region.data(), true, 1);
        if (prologues.empty()) {
            throw std::runtime_error("Failed to find KRSAVerifyFile prologue");
        }

        _kRSAVerifyFile = reinterpret_cast<decltype(_kRSAVerifyFile)>(prologues[0]);
    }

    IFunctionHook::HookTarget KRSAVerifyFileHook::LocateTarget() const {
        std::span<const uint8_t> region = GetSearchRegion();
        if (region.empty()) {
            throw std::runtime_error("Failed to get memory region for search");
        }
        LocateTargetInRegion(region);
        return {
            reinterpret_cast<PVOID*>(&_kRSAVerifyFile),
            reinterpret_cast<PVOID>(KRSAVerifyFile)
        };
    }

    std::span<const uint8_t> KRSAVerifyFilePacketHook::GetSearchRegion() const {
        HMODULE module = ModuleUtil::GetHandleW(std::nullopt);
        return ModuleUtil::GetMemoryRegion(module);
    }

    IFunctionHook::HookTarget KRSAVerifyFilePacketHook::LocateTarget() const {
        return KRSAVerifyFileHook::LocateTarget();
    }

    const char* KRSAVerifyFilePacketHook::GetName() const {
        return "KRSAVerifyFilePacketHook";
    }
}
