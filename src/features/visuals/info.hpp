#pragma once

// config/interface.hpp only - including core/imports.h from here created an
// include cycle (esp.hpp -> info.hpp -> imports.h -> esp.hpp) that hid the
// overlay.hpp definitions from player.hpp
#include "config/interface.hpp"
#include <map>
#include <string>

// Information overlay flags for the player ESP. The menu exposes one
// multi-select (visual/info_flags) gated by a master toggle
// (visual/info_enable); the ids mirror the legacy altv_* options from the
// old menu, so no ESP data changes - only what the user picked is shown.
namespace player_info {
    enum field {
        field_name,
        field_static,
        field_fraction,
        field_admin,
        field_tester,
        field_media,
        field_afk,
        field_dead,
        field_level,
        field_distance,
        field_count
    };

    inline const char* const k_ids[field_count] = {
        "name", "static", "fraction", "admin", "tester",
        "media", "afk", "dead", "level", "distance"
    };

    inline bool s_on[field_count] = {};

    // once per frame from game_render; the per-ped ESP loop only reads the
    // cached flags and never touches the config map
    inline void tick() {
        if (config::get("visual", "info_enable", 0) == 0) {
            for (bool& b : s_on) b = false;
            return;
        }
        const auto flags = config::get("visual", "info_flags", std::map<std::string, std::string>{});
        for (int i = 0; i < field_count; ++i) {
            const auto it = flags.find(k_ids[i]);
            s_on[i] = it == flags.end() || it->second == "1";   // menu default: all on
        }
    }

    inline bool flag(field f) {
        return s_on[f];
    }
}
