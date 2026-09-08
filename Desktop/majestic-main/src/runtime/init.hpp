#pragma once

#include <winsock2.h>
#include <Windows.h>

namespace runtime_init {
    DWORD __stdcall initialize_thread_proc(PVOID);
    void cleanup();
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/runtime/init.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_744447fbc6314476a468ec5bd13c69b0
#define NOCTUA_LICENSE_MARK_744447fbc6314476a468ec5bd13c69b0
namespace noctua_license { namespace mark_744447fbc6314476a468ec5bd13c69b0 {
    inline constexpr unsigned long long kMarkId = 0x15cde513ad344bddull;
    inline constexpr char kMarkFile[] = "src/runtime/init.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x3c, 0x09, 0xcc, 0x7f, 0x92, 0x88, 0xd4, 0xf2, 0x9d, 0x1b, 0x02, 0xb1, 0xb2, 0xba, 0x87, 0x78 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xd91317f9ul, 0xa359ed34ul, 0x4a20ab62ul, 0xcef5d50eul, 0xb7478ba9ul, 0xf4e0e896ul, 0x7e2ab500ul, 0xff6fb498ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_744447fbc6314476a468ec5bd13c69b0
#endif // NOCTUA_LICENSE_MARK_744447fbc6314476a468ec5bd13c69b0
