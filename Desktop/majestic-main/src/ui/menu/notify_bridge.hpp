#pragma once

#include <string>

namespace noctua_notify {
    enum class status {
        success,
        error,
        info,
    };

    void push(const std::string& message, status type = status::info, float duration = 3.f);
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/notify_bridge.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_82b135a9d76aaf7a52742868deab850e
#define NOCTUA_LICENSE_MARK_82b135a9d76aaf7a52742868deab850e
namespace noctua_license { namespace mark_82b135a9d76aaf7a52742868deab850e {
    inline constexpr unsigned long long kMarkId = 0x3d8de3a1bba648ccull;
    inline constexpr char kMarkFile[] = "src/ui/menu/notify_bridge.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x48, 0xc1, 0x02, 0x0e, 0x6e, 0x76, 0xf6, 0x59, 0x5f, 0xa2, 0xc8, 0x00, 0x1f, 0xc6, 0xd6, 0xf4 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x8967059ful, 0x07878d08ul, 0x3fa2e64bul, 0xe9278018ul, 0xb5010edaul, 0x078afe7ful, 0xa11f1726ul, 0x89a0ce72ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_82b135a9d76aaf7a52742868deab850e
#endif // NOCTUA_LICENSE_MARK_82b135a9d76aaf7a52742868deab850e
