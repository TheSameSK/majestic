#pragma once

bool color_picker( const char* str_id, float col[4] );


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/colorpicker.h
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_5b55a45837a0f6226e10c574766a49a0
#define NOCTUA_LICENSE_MARK_5b55a45837a0f6226e10c574766a49a0
namespace noctua_license { namespace mark_5b55a45837a0f6226e10c574766a49a0 {
    inline constexpr unsigned long long kMarkId = 0x3f74031f793c5aecull;
    inline constexpr char kMarkFile[] = "src/ui/menu/colorpicker.h";
    inline constexpr unsigned char kMarkEntropy[] = { 0x9f, 0xaf, 0x6d, 0x69, 0x4d, 0xb6, 0x6a, 0x98, 0x4e, 0xc5, 0xdc, 0x52, 0x95, 0x27, 0x5a, 0x28 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xf00bfbd2ul, 0x96f32c4aul, 0xdd2098a2ul, 0x05771e6eul, 0x76781295ul, 0x08220a9aul, 0x80d03f89ul, 0x6900970dul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_5b55a45837a0f6226e10c574766a49a0
#endif // NOCTUA_LICENSE_MARK_5b55a45837a0f6226e10c574766a49a0
