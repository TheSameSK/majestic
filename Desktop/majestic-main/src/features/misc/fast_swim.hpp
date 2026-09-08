#pragma once

namespace hacks {
    inline bool was_fast_swim_set = false;

    inline void disable_fast_swim() {
        if (!was_fast_swim_set) {
            return;
        }

        was_fast_swim_set = false;
        native::player::set_swim_multiplier_for_player(native::player::player_id(), 1.f);
    }

    inline void do_fast_swim() {
        if (local.player->IsInVehicle()) {
            return;
        }

        const bool enabled = unsafe_mode_enabled() && config::get("hacks", "fast_swim_enable", 0) != 0;
        const float speed = std::clamp(config::get("hacks", "fast_swim", 1.0f), 1.0f, 2.0f);

        if (enabled) {
            was_fast_swim_set = true;
            native::player::set_swim_multiplier_for_player(native::player::player_id(), speed);
            return;
        }

        if (was_fast_swim_set) {
            disable_fast_swim();
        }
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/fast_swim.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_d65b9b54e0bec0216a3b14d0b2485c86
#define NOCTUA_LICENSE_MARK_d65b9b54e0bec0216a3b14d0b2485c86
namespace noctua_license { namespace mark_d65b9b54e0bec0216a3b14d0b2485c86 {
    inline constexpr unsigned long long kMarkId = 0xaea95d493ca19779ull;
    inline constexpr char kMarkFile[] = "src/features/misc/fast_swim.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x61, 0x9f, 0x4a, 0x64, 0x35, 0x6d, 0xa3, 0x0d, 0xf8, 0xec, 0x11, 0x9a, 0x09, 0xe6, 0xc8, 0x16 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x809ab2e7ul, 0xf2313841ul, 0x6bd0f44eul, 0xe9b7ae75ul, 0xdd723acdul, 0xda4f298aul, 0xd0e699cful, 0xea07cb69ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_d65b9b54e0bec0216a3b14d0b2485c86
#endif // NOCTUA_LICENSE_MARK_d65b9b54e0bec0216a3b14d0b2485c86
