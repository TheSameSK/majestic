#pragma once

namespace hacks {
    inline native::type::ped local_ped_handle() {
        __try {
            if (!pointer_to_handle || !IsValidPtr(local.player)) {
                return 0;
            }

            return pointer_to_handle(reinterpret_cast<intptr_t>(local.player));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/local_player.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_a1bdfcc9adf0b8ae65ccfc28df77a604
#define NOCTUA_LICENSE_MARK_a1bdfcc9adf0b8ae65ccfc28df77a604
namespace noctua_license { namespace mark_a1bdfcc9adf0b8ae65ccfc28df77a604 {
    inline constexpr unsigned long long kMarkId = 0x5eeaba30f2245fe0ull;
    inline constexpr char kMarkFile[] = "src/features/misc/local_player.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x69, 0xb1, 0x83, 0x75, 0x97, 0xab, 0x76, 0x52, 0x97, 0xdb, 0x16, 0x6d, 0xe7, 0x90, 0x21, 0xa7 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x4d5e77c8ul, 0xa56fff83ul, 0x2157c23cul, 0x0d119e7eul, 0x1d4e4088ul, 0x3e51be4dul, 0xb06b6182ul, 0x6e1b427aul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_a1bdfcc9adf0b8ae65ccfc28df77a604
#endif // NOCTUA_LICENSE_MARK_a1bdfcc9adf0b8ae65ccfc28df77a604
