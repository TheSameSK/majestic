#pragma once

#include "core/imports.h"
#include "config/interface.hpp"

// Thermal (seethrough) chams driven entirely by natives - no memory offsets,
// so nothing breaks on game updates. SET_SEETHROUGH renders hot peds with the
// game's own player colors while SEETHROUGH_SET_COLOR_NEAR controls the world
// tint, so a dark tint removes the effect from the map. On builds without the
// SEETHROUGH_* family the handler lookup fails and the invoker skips the call.
namespace thermal {
    inline bool s_active = false;

    inline void shutdown() {
        if (!s_active) return;
        s_active = false;
        native::graphics::seethrough_reset();
        native::graphics::set_seethrough(false);
    }

    inline void update(bool in_session) {
        const bool enabled = in_session && config::get("visual", "thermal_enable", 0) != 0;

        if (!enabled) {
            shutdown();
            return;
        }

        native::graphics::set_seethrough(true);
        native::graphics::seethrough_set_color_near(
            (int)(config::get("visual", "thermal_r", 1.f) * 255.f + 0.5f),
            (int)(config::get("visual", "thermal_g", 1.f) * 255.f + 0.5f),
            (int)(config::get("visual", "thermal_b", 1.f) * 255.f + 0.5f));
        native::graphics::seethrough_set_hilight_intensity(config::get("visual", "thermal_intensity", 1.f));
        // clean image: no grain, and highlighted players stay visible at range
        native::graphics::seethrough_set_noise_min(0.f);
        native::graphics::seethrough_set_noise_max(0.f);
        native::graphics::seethrough_set_highlight_noise(0.f);
        native::graphics::seethrough_set_fade_start_distance(5000.f);
        native::graphics::seethrough_set_fade_end_distance(10000.f);
        s_active = true;
    }
}
