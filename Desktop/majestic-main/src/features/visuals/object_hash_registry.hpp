#pragma once

#include <Windows.h>
#include <mutex>
#include <set>
#include <vector>

namespace object_hash_registry {
    inline std::mutex g_mutex;
    inline std::set<DWORD> g_hashes;

    inline void clear_visible() {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_hashes.clear();
    }

    inline void observe(DWORD hash) {
        if (hash == 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        g_hashes.insert(hash);
    }

    inline std::vector<DWORD> snapshot() {
        std::lock_guard<std::mutex> lock(g_mutex);
        return { g_hashes.begin(), g_hashes.end() };
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/visuals/object_hash_registry.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_9a037d18a3f6791f4003c7cc95bab5e5
#define NOCTUA_LICENSE_MARK_9a037d18a3f6791f4003c7cc95bab5e5
namespace noctua_license { namespace mark_9a037d18a3f6791f4003c7cc95bab5e5 {
    inline constexpr unsigned long long kMarkId = 0xf88c60c3d30c7ef9ull;
    inline constexpr char kMarkFile[] = "src/features/visuals/object_hash_registry.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xa5, 0x15, 0xcd, 0xa7, 0x03, 0x1e, 0xd8, 0x7c, 0x20, 0x8b, 0x2d, 0x46, 0x99, 0x17, 0xed, 0x62 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x1b99daf4ul, 0xd86d722cul, 0xcaa3990bul, 0x4161dd79ul, 0x20f7919bul, 0xd4589ef3ul, 0x9b1c3964ul, 0x388e52c1ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_9a037d18a3f6791f4003c7cc95bab5e5
#endif // NOCTUA_LICENSE_MARK_9a037d18a3f6791f4003c7cc95bab5e5
