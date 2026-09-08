#pragma once

struct ID3D11Device;

namespace menu {
    void initialize( ID3D11Device* device );
    void shutdown( );
    void render( bool menu_open );
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/ui/menu/menu.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_e7bc6a69cdd31bd0be4431ab6f707336
#define NOCTUA_LICENSE_MARK_e7bc6a69cdd31bd0be4431ab6f707336
namespace noctua_license { namespace mark_e7bc6a69cdd31bd0be4431ab6f707336 {
    inline constexpr unsigned long long kMarkId = 0x633e1966b6112531ull;
    inline constexpr char kMarkFile[] = "src/ui/menu/menu.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xa5, 0x0b, 0x09, 0xb8, 0xb3, 0x59, 0x4f, 0x38, 0xa9, 0x81, 0xb2, 0xf1, 0x4e, 0x2a, 0x92, 0x5d };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xf502dbf5ul, 0x848a620cul, 0xee7525faul, 0x942e3a02ul, 0xdce8361ful, 0x6a179333ul, 0x3e73a565ul, 0xe8e25890ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_e7bc6a69cdd31bd0be4431ab6f707336
#endif // NOCTUA_LICENSE_MARK_e7bc6a69cdd31bd0be4431ab6f707336
