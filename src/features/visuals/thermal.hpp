#pragma once

#include "core/imports.h"
#include "config/interface.hpp"
#include <cstring>

// Thermal (seethrough) chams driven entirely by natives - no hardcode offsets,
// so nothing breaks on game updates. SET_SEETHROUGH renders hot peds with the
// game's own player colors while SEETHROUGH_SET_COLOR_NEAR controls the world
// tint. Note the composite pass re-shades the whole screen by design, so the
// world always takes the near color - it cannot be made pixel-vanilla from
// postfx alone. On builds without the SEETHROUGH_* family the handler lookup
// fails and the invoker skips the call.
namespace thermal {
    inline bool s_active = false;

    // colorFar has no native. The params struct is located at runtime from the
    // COLOR_NEAR handler itself: the game handler stores the packed near color
    // into the global struct with a rip-relative store, so the first store's
    // target is colorNear. Public reversal gives the layout (packed 32-bit
    // colors): colorNear, +0x14 colorFar, +0x18 playerColorOutline, +0x1c
    // playerColorInside. We only zero colorFar - players keep their colors.
    // EXPERIMENTAL, off by default: writing to game memory here was the prime
    // crash suspect on alt+tab.
    inline uintptr_t s_far_base = 0;
    inline uint32_t s_saved_far = 0;
    inline bool s_far_saved = false;
    inline bool s_far_write_failed = false;

    inline bool in_game_module(uintptr_t addr) {
        if (!Game.base) return false;
        auto base = (const uint8_t*)Game.base;
        auto dos = (const IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        auto nt = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
        return addr >= Game.base && addr < Game.base + nt->OptionalHeader.SizeOfImage;
    }

    // re-resolved on every use on purpose: a pointer cached across device
    // resets (alt+tab) went stale and crashed the game
    inline uintptr_t find_seethrough_params() {
        auto handler = (const uint8_t*)native::invoker::find_native_handler(0x1086127b3a63505e);
        if (!IsValidPtr(handler)) return 0;

        for (int i = 0; i < 256; ++i) {
            int32_t disp = 0;
            int tail = 0;
            if (handler[i] == 0x89 && (handler[i + 1] & 0xC7) == 0x05) {            // mov [rip+disp32], r32
                memcpy(&disp, handler + i + 2, 4);
                tail = 6;
            } else if (handler[i] == 0xC7 && handler[i + 1] == 0x05) {              // mov dword [rip+disp32], imm32
                memcpy(&disp, handler + i + 2, 4);
                tail = 6;
            } else {
                continue;
            }
            const uintptr_t target = (uintptr_t)(handler + tail) + disp;
            return in_game_module(target) ? target : 0;
        }
        return 0;
    }

    inline void zero_far_color() {
        if (s_far_write_failed) return;
        const uintptr_t base = find_seethrough_params();
        __try {
            // pointer moved (re-resolution): put the original value back first
            if (s_far_saved && s_far_base && s_far_base != base && in_game_module(s_far_base)) {
                *(uint32_t*)(s_far_base + 0x14) = s_saved_far;
                s_far_saved = false;
            }
            if (!base) return;
            if (!s_far_saved) {
                s_saved_far = *(uint32_t*)(base + 0x14);
                s_far_saved = true;
            }
            if (*(uint32_t*)(base + 0x14) != 0) {
                *(uint32_t*)(base + 0x14) = 0;
            }
            s_far_base = base;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            s_far_write_failed = true;
        }
    }

    inline void restore_far_color() {
        if (!s_far_saved || !s_far_base) return;
        __try {
            if (in_game_module(s_far_base)) {
                *(uint32_t*)(s_far_base + 0x14) = s_saved_far;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        s_far_saved = false;
        s_far_base = 0;
    }

    inline void shutdown() {
        if (!s_active) return;
        s_active = false;
        restore_far_color();
        native::graphics::seethrough_reset();
        native::graphics::set_seethrough(false);
    }

    inline void update(bool in_session) {
        // seed the stored tint once so the menu picker and the effect agree
        // (defaults: black near color = least world lighting)
        static bool seeded = false;
        if (!seeded) {
            seeded = true;
            config::update(config::get("visual", "thermal_r", 0.f), "visual", "thermal_r", 0.f);
            config::update(config::get("visual", "thermal_g", 0.f), "visual", "thermal_g", 0.f);
            config::update(config::get("visual", "thermal_b", 0.f), "visual", "thermal_b", 0.f);
            config::update(config::get("visual", "thermal_a", 1.f), "visual", "thermal_a", 1.f);
        }

        const bool enabled = in_session && config::get("visual", "thermal_enable", 0) != 0;

        if (!enabled) {
            shutdown();
            return;
        }

        native::graphics::set_seethrough(true);
        native::graphics::seethrough_set_color_near(
            (int)(config::get("visual", "thermal_r", 0.f) * 255.f + 0.5f),
            (int)(config::get("visual", "thermal_g", 0.f) * 255.f + 0.5f),
            (int)(config::get("visual", "thermal_b", 0.f) * 255.f + 0.5f));
        native::graphics::seethrough_set_hilight_intensity(config::get("visual", "thermal_intensity", 1.f));
        // clean image: no grain, and highlighted players stay visible at range
        native::graphics::seethrough_set_noise_min(0.f);
        native::graphics::seethrough_set_noise_max(0.f);
        native::graphics::seethrough_set_highlight_noise(0.f);
        native::graphics::seethrough_set_fade_start_distance(5000.f);
        native::graphics::seethrough_set_fade_end_distance(10000.f);

        if (config::get("visual", "thermal_fix_sky", 0) != 0) {
            zero_far_color();
        } else {
            restore_far_color();
        }

        s_active = true;
    }
}
