#pragma once

#include "../core/imports.h"

namespace ui {
    inline char toAddHashSets[256] = {};
    inline char toAddHash[256] = {};
    inline char toAddHashVeh[256] = {};

    struct AppLog {
        void AddLog(const char* fmt, ...) {}
    };

    inline AppLog DEBUG;
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/ui.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_262034558ccb1c6c7b9502b75534b02a
#define NOCTUA_LICENSE_MARK_262034558ccb1c6c7b9502b75534b02a
namespace noctua_license { namespace mark_262034558ccb1c6c7b9502b75534b02a {
    inline constexpr unsigned long long kMarkId = 0xee3d1d985e1e33ccull;
    inline constexpr char kMarkFile[] = "src/ui/ui.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x48, 0x4b, 0x6d, 0xc5, 0x59, 0xd6, 0xad, 0xc3, 0xd7, 0x63, 0x58, 0xb1, 0x78, 0xb2, 0x0b, 0xa3 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x11ef428eul, 0x7f7e5370ul, 0x39909428ul, 0x382af858ul, 0xe2ba89bdul, 0xff80195ful, 0xabab20d9ul, 0xff9f836aul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_262034558ccb1c6c7b9502b75534b02a
#endif // NOCTUA_LICENSE_MARK_262034558ccb1c6c7b9502b75534b02a
