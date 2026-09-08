#pragma once
#include "core/imports.h"
#include "config/bind_state.hpp"
#include <cmath>
#include <algorithm>
#include <chrono>
#include "imgui.h"

#undef min
#undef max

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

static const bind_state::bind_config k_godmode_bind {
    "hacks",
    "godmode",
    "god_key",
    "godmode_key",
    "god_key_mode",
    "godmode_mode"
};

static const bind_state::bind_config k_noclip_bind {
    "hacks",
    "noclip",
    "noclip_key",
    "noclip_key",
    "noclip_key_mode",
    "noclip_mode"
};

static const bind_state::bind_config k_freecam_bind {
    "hacks",
    "freecam",
    "Freecam_key",
    "freecam_key",
    "Freecam_key_mode",
    "freecam_mode"
};

static const bind_state::bind_config k_clickwarp_bind {
    "hacks",
    "clickwarp",
    "clickwarp_key",
    "clickwarp_key",
    "clickwarp_key_mode",
    "clickwarp_mode"
};

static const bind_state::bind_config k_skip_anim_bind {
    "hacks",
    "skip_anim_enable",
    "skip_anim_key",
    "skip_anim_key",
    "skip_anim_key_mode",
    "skip_anim_mode"
};

static const bind_state::bind_config k_veh_boost_bind {
    "hacks",
    "veh_boost_enabled",
    "veh_boost_key",
    "veh_boost_key",
    "veh_boost_key_mode",
    "veh_boost_mode"
};

static const bind_state::bind_config k_veh_fast_stop_bind {
    "hacks",
    "veh_fast_stop_enabled",
    "veh_fast_stop_key",
    "veh_fast_stop_key",
    "veh_fast_stop_key_mode",
    "veh_fast_stop_mode"
};

static const bind_state::bind_config k_double_shoot_bind {
    "hacks",
    "double_shoot",
    "double_shoot_key",
    "double_shoot_key",
    "double_shoot_key_mode",
    "double_shoot_mode"
};

namespace hacks {
    inline bool unsafe_mode_enabled() {
        return config::get("misc", "unsafe_mode", 0) != 0;
    }
}

using Hash = uint32_t;
namespace game {
    extern vector<pair<CObject*, DataPed>> ped_list;
    extern vector<CObject*> object_list;
    extern vector<CVehicle*> vehicle_list;
    extern bool isValidPlayer(DWORD hash, CObject* player);
    extern DWORD freemode_f;
    extern DWORD freemode_m;
    extern vector<CVehicle*> nearby_vehicle_list;
}
Vector3 lerp(const Vector3& a, const Vector3& b, float t) {
    return Vector3{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/binds.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_aec03ca4b482ce65b0f0e5ee75c61355
#define NOCTUA_LICENSE_MARK_aec03ca4b482ce65b0f0e5ee75c61355
namespace noctua_license { namespace mark_aec03ca4b482ce65b0f0e5ee75c61355 {
    inline constexpr unsigned long long kMarkId = 0xab13601f6ce496a4ull;
    inline constexpr char kMarkFile[] = "src/features/misc/binds.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xe9, 0x79, 0x4f, 0xd8, 0x54, 0xcf, 0x62, 0xdb, 0x03, 0xd5, 0x50, 0x44, 0x44, 0x79, 0x64, 0xc5 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x6a003965ul, 0xdd44cb12ul, 0x5754f2e6ul, 0x4233b983ul, 0x5c959eb6ul, 0x387b0381ul, 0x69db39b5ul, 0xc32bb54cul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_aec03ca4b482ce65b0f0e5ee75c61355
#endif // NOCTUA_LICENSE_MARK_aec03ca4b482ce65b0f0e5ee75c61355
