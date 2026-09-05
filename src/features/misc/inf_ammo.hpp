#pragma once
#include <set>
#include "features/visuals/thermal.hpp"

namespace hacks {
    static constexpr int k_infinite_ammo_refill = 9999;
    static bool was_infinite_ammo_set = false;
    static CObject* infinite_ammo_cleanup_player = nullptr;
    static DWORD64 infinite_ammo_cleanup_since = 0;
    static std::set<native::type::hash> infinite_ammo_weapon_hashes;

    inline bool infinite_ammo_cleanup_ready() {
        const DWORD64 now = GetTickCount64();
        if (!Game.renderReady || !Game.gameDataReady || Game.renderReadySince == 0 || now - Game.renderReadySince < 2500) {
            infinite_ammo_cleanup_player = nullptr;
            infinite_ammo_cleanup_since = 0;
            return false;
        }

        runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_player";
        if (!IsValidPtr(local.player)) {
            infinite_ammo_cleanup_player = nullptr;
            infinite_ammo_cleanup_since = 0;
            return false;
        }

        runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_alive";
        if (!_SafeCheckPlayerAlive()) {
            infinite_ammo_cleanup_player = nullptr;
            infinite_ammo_cleanup_since = 0;
            return false;
        }

        if (infinite_ammo_cleanup_player != local.player) {
            infinite_ammo_cleanup_player = local.player;
            infinite_ammo_cleanup_since = now;
            return false;
        }

        return infinite_ammo_cleanup_since != 0 && now - infinite_ammo_cleanup_since >= 1500;
    }

    inline bool get_current_weapon_hash(native::type::ped ped, native::type::hash* out_hash) {
        if (native::weapon::get_current_ped_weapon(ped, out_hash, true) && *out_hash != 0) {
            return true;
        }

        if (!IsValidPtr(local.player)) {
            return false;
        }

        CWeaponManager* weapon = local.player->weapon();
        if (!IsValidPtr(weapon) || !IsValidPtr(weapon->_WeaponInfo)) {
            return false;
        }

        const uintptr_t weapon_info = reinterpret_cast<uintptr_t>(weapon->_WeaponInfo);
        if (weapon_info <= 0x10000 || weapon_info >= 0x7FFFFFFFFFFF) {
            return false;
        }

        const native::type::hash hash = *reinterpret_cast<native::type::hash*>(weapon_info + 0x10);
        if (hash == 0) {
            return false;
        }

        *out_hash = hash;
        return true;
    }

    inline void refill_weapon_memory_ammo() {
        if (!IsValidPtr(local.player)) {
            return;
        }

        CWeaponManager* weapon = local.player->weapon();
        if (!IsValidPtr(weapon) || !IsValidPtr(weapon->_WeaponInfo)) {
            return;
        }

        CAmmoInfo* ammo_info = reinterpret_cast<CAmmoInfo*>(weapon->_WeaponInfo->_AmmoInfo);
        if (IsValidPtr(ammo_info) && IsValidPtr(ammo_info->_AmmoCount)) {
            auto ammo_count = ammo_info->_AmmoCount;
            if (IsValidPtr(ammo_count->_PrimaryAmmoCount)) {
                ammo_count->_PrimaryAmmoCount->AmmoCount = k_infinite_ammo_refill;
            }
        }
    }

    inline void refill_native_weapon_ammo(native::type::ped ped, native::type::hash weapon_hash) {
        infinite_ammo_weapon_hashes.insert(weapon_hash);
        native::weapon::set_ped_infinite_ammo(ped, true, weapon_hash);

        int max_clip = native::weapon::get_max_ammo_in_clip(ped, weapon_hash, true);
        if (max_clip > 0) {
            int clip_ammo = 0;
            if (native::weapon::get_ammo_in_clip(ped, weapon_hash, &clip_ammo) && clip_ammo < max_clip) {
                native::weapon::set_ammo_in_clip(ped, weapon_hash, max_clip);
            }
        }

        int max_ammo = 0;
        const int refill_amount = native::weapon::get_max_ammo(ped, weapon_hash, &max_ammo) && max_ammo > 0
            ? max_ammo
            : k_infinite_ammo_refill;

        if (native::weapon::get_ammo_in_ped_weapon(ped, weapon_hash) < refill_amount) {
            native::weapon::set_ped_ammo(ped, weapon_hash, refill_amount);
        }

        const native::type::hash ammo_type = native::weapon::get_ped_ammo_type_from_weapon(ped, weapon_hash);
        if (ammo_type != 0 && native::weapon::get_ped_ammo_by_type(ped, ammo_type) < refill_amount) {
            native::weapon::set_ped_ammo_by_type(ped, ammo_type, refill_amount);
        }
    }

    void disable_infinite_ammo() {
        if (!was_infinite_ammo_set) {
            return;
        }
        if (!infinite_ammo_cleanup_ready()) {
            return;
        }

        runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_handle";
        auto ped = local_ped_handle();
        if (ped != 0) {
            runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_clip";
            native::weapon::set_ped_infinite_ammo_clip(ped, false);
            for (const native::type::hash weapon_hash : infinite_ammo_weapon_hashes) {
                runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_hash";
                native::weapon::set_ped_infinite_ammo(ped, false, weapon_hash);
            }
            native::type::hash weapon_hash = 0;
            runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_current";
            if (get_current_weapon_hash(ped, &weapon_hash)) {
                runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_current_hash";
                native::weapon::set_ped_infinite_ammo(ped, false, weapon_hash);
            }
        }

        runtime_debug::last_section = "gt_hacks_disable_infinite_ammo_clear";
        infinite_ammo_weapon_hashes.clear();
        was_infinite_ammo_set = false;
        infinite_ammo_cleanup_player = nullptr;
        infinite_ammo_cleanup_since = 0;
    }

    void disable_panic_features() {
        disable_core_runtime_effects();
        disable_infinite_ammo();
        disable_fast_swim();
        disable_veh_no_collision();
        disable_veh_speed();
        disable_veh_gravity();
        disable_unlock_speed_limit();
        ClickWarp::reset();
        stop_roe_freecam();
        thermal::shutdown();
    }

    void do_infinite_ammo() {
        was_infinite_ammo_set = true;
        infinite_ammo_cleanup_player = nullptr;
        infinite_ammo_cleanup_since = 0;

        refill_weapon_memory_ammo();

        auto ped = local_ped_handle();
        if (ped != 0) {
            native::weapon::set_ped_infinite_ammo_clip(ped, true);
            native::type::hash weapon_hash = 0;
            if (get_current_weapon_hash(ped, &weapon_hash)) {
                refill_native_weapon_ammo(ped, weapon_hash);
            }
        }
    }

    static int wp_tp_stage = 0;
    static float wp_x = 0.f, wp_y = 0.f;
    static int wp_attempts = 0;

    void tp_to_waypoint() {
        auto blip = native::ui::get_first_blip_info_id(8);
        if (!native::ui::does_blip_exist(blip)) return;

        auto waypoint = native::ui::get_blip_coords(blip);
        auto player = local_ped_handle();
        if (player == 0) return;

        auto entity = player;
        if (native::ped::is_ped_in_any_vehicle(player, false)) {
            auto veh = native::ped::get_vehicle_ped_is_in(player, false);
            if (veh) entity = veh;
        }

        float ground_z = 0.f;
        bool found_ground = false;

        static const float heights[] = { 100.f, 200.f, 300.f, 400.f, 500.f, 600.f, 700.f, 800.f, 900.f, 1000.f };
        for (float h : heights) {
            native::streaming::request_collision_at_coord(waypoint.x, waypoint.y, h);
            if (native::invoker::invoke<bool>(0xC906A7DAB05C8D2B, waypoint.x, waypoint.y, h, &ground_z, false)) {
                found_ground = true;
                break;
            }
        }

        if (found_ground && ground_z > 0.f) {
            native::entity::set_entity_coords_no_offset(entity, waypoint.x, waypoint.y, ground_z + 1.f, false, false, false);
        } else {
            native::streaming::request_collision_at_coord(waypoint.x, waypoint.y, 300.f);
            native::entity::set_entity_coords_no_offset(entity, waypoint.x, waypoint.y, 300.f, false, false, false);
        }
    }

    void wp_tp_tick() {
        wp_tp_stage = 0;
    }

    void hacks_tick() {
        if (IsValidPtr(local.player) && _SafeCheckPlayerAlive()) {
            const bool unsafe_mode = unsafe_mode_enabled();
            runtime_debug::last_section = "gt_hacks_update";
            update_hacks();
            if (unsafe_mode && config::get("hacks", "infinite_ammo", 0)) {
                runtime_debug::last_section = "gt_hacks_infinite_ammo";
                do_infinite_ammo();
            }
            else {
                runtime_debug::last_section = "gt_hacks_disable_infinite_ammo";
                disable_infinite_ammo();
            }
            if ((unsafe_mode && config::get("hacks", "fast_swim", 0)) || was_fast_swim_set) {
                runtime_debug::last_section = "gt_hacks_fast_swim";
                do_fast_swim();
            }
            if (config::get("hacks", "veh_speed_enable", 0) || was_veh_speed_set) {
                runtime_debug::last_section = "gt_hacks_veh_speed";
                do_veh_speed();
            }
            if (config::get("hacks", "veh_gravity_enable", 0) || was_veh_gravity_set) {
                runtime_debug::last_section = "gt_hacks_veh_gravity";
                do_veh_gravity();
            }
            if (config::get("hacks", "unlock_speed_limit", 0) || was_speed_limit_unlocked) {
                runtime_debug::last_section = "gt_hacks_unlock_speed_limit";
                do_unlock_speed_limit();
            }
        }
        runtime_debug::last_section = "gt_hacks_done";
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/inf_ammo.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_912e3e8b3a46710a7c6e00eb367d885f
#define NOCTUA_LICENSE_MARK_912e3e8b3a46710a7c6e00eb367d885f
namespace noctua_license { namespace mark_912e3e8b3a46710a7c6e00eb367d885f {
    inline constexpr unsigned long long kMarkId = 0x7fc31911446c9544ull;
    inline constexpr char kMarkFile[] = "src/features/misc/inf_ammo.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x2d, 0x03, 0x38, 0x30, 0xcb, 0xc2, 0x01, 0xeb, 0x85, 0x04, 0x13, 0xbd, 0x58, 0xc4, 0x5f, 0xba };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xe5360e46ul, 0x00ae5a60ul, 0x8ceb2a8eul, 0x49aceb0aul, 0xc11bdfb6ul, 0x0552ed2bul, 0x68a62926ul, 0xb0d62ddaul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_912e3e8b3a46710a7c6e00eb367d885f
#endif // NOCTUA_LICENSE_MARK_912e3e8b3a46710a7c6e00eb367d885f
