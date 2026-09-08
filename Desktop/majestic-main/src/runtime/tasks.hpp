#pragma once

namespace runtime_tasks {
    void start_ws_server();
    void start_aimbot();
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/runtime/tasks.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_6077667c6238271e508418b9c757a9e5
#define NOCTUA_LICENSE_MARK_6077667c6238271e508418b9c757a9e5
namespace noctua_license { namespace mark_6077667c6238271e508418b9c757a9e5 {
    inline constexpr unsigned long long kMarkId = 0xbf5edd525689b889ull;
    inline constexpr char kMarkFile[] = "src/runtime/tasks.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x5d, 0x6a, 0x72, 0x58, 0x92, 0x20, 0xe8, 0x66, 0x77, 0x07, 0x48, 0xc6, 0x4c, 0xc2, 0x09, 0x66 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x36028ac8ul, 0xceaa7beaul, 0x0f7bd507ul, 0xa4da0d51ul, 0x93405055ul, 0x4b66fed0ul, 0x32096247ul, 0x118392bful };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_6077667c6238271e508418b9c757a9e5
#endif // NOCTUA_LICENSE_MARK_6077667c6238271e508418b9c757a9e5
