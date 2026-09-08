#pragma once

namespace build_profile {
#if defined(NOCTUA_DEBUG_BUILD)
    inline constexpr bool debug = true;
    inline constexpr bool production = false;
#else
    inline constexpr bool debug = false;
    inline constexpr bool production = true;
#endif
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/platform/build_profile.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_7671a0f819af04928cd4b83de774261a
#define NOCTUA_LICENSE_MARK_7671a0f819af04928cd4b83de774261a
namespace noctua_license { namespace mark_7671a0f819af04928cd4b83de774261a {
    inline constexpr unsigned long long kMarkId = 0xff3b6cee0b316355ull;
    inline constexpr char kMarkFile[] = "src/platform/build_profile.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xd5, 0x95, 0xaa, 0x98, 0x4f, 0x8a, 0x9f, 0xdd, 0x08, 0xc4, 0xbf, 0xde, 0xfc, 0x7c, 0x52, 0xae };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xf0b10033ul, 0xca460eaaul, 0x3cc3b93dul, 0x705acb4cul, 0xb42653c3ul, 0xec9b95c5ul, 0xb68773beul, 0x00227f89ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_7671a0f819af04928cd4b83de774261a
#endif // NOCTUA_LICENSE_MARK_7671a0f819af04928cd4b83de774261a
