#pragma once
#include <atomic>

namespace menu_actions {
    inline std::atomic<bool> pending_set_hp{false};
    inline std::atomic<bool> pending_set_armor{false};
    inline std::atomic<bool> pending_tp_waypoint{false};
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/menu_actions.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_e301004ea8ba02eec39dcd732a7dc590
#define NOCTUA_LICENSE_MARK_e301004ea8ba02eec39dcd732a7dc590
namespace noctua_license { namespace mark_e301004ea8ba02eec39dcd732a7dc590 {
    inline constexpr unsigned long long kMarkId = 0xfc90af23b96b91aeull;
    inline constexpr char kMarkFile[] = "src/ui/menu/menu_actions.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x8f, 0x0c, 0x0b, 0xbb, 0xc3, 0x6a, 0x45, 0x4b, 0x4e, 0x84, 0xd3, 0x7d, 0x41, 0xec, 0x7e, 0x2b };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x1c0baa4bul, 0x81742395ul, 0x5fe57239ul, 0x33dc2790ul, 0x6d7cc077ul, 0x1033bdfdul, 0x0868cdfcul, 0x6a882c21ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_e301004ea8ba02eec39dcd732a7dc590
#endif // NOCTUA_LICENSE_MARK_e301004ea8ba02eec39dcd732a7dc590
