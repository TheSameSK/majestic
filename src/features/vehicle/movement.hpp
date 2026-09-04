#pragma once

namespace hacks {
    inline bool was_veh_speed_set = false;
    inline bool was_veh_gravity_set = false;
    inline bool was_speed_limit_unlocked = false;

    inline native::type::any current_vehicle() {
        if (!local.player->IsInVehicle()) {
            return 0;
        }
        const auto ped = local_ped_handle();
        if (!ped) {
            return 0;
        }

        return native::ped::get_vehicle_ped_is_in(ped, false);
    }

    inline void do_veh_speed() {
        const auto vehicle = current_vehicle();
        if (!vehicle) {
            was_veh_speed_set = false;
            return;
        }

        const bool enabled = unsafe_mode_enabled() && config::get("hacks", "veh_speed_enable", 0) != 0;
        const float multiplier = config::get("hacks", "veh_speed_mult", 1.f);

        if (enabled && multiplier > 1.f) {
            was_veh_speed_set = true;
            native::invoker::invoke<void>(0x93A3996368C94158, vehicle, multiplier);
            return;
        }

        if (was_veh_speed_set) {
            native::invoker::invoke<void>(0x93A3996368C94158, vehicle, 1.0f);
            was_veh_speed_set = false;
        }
    }

    inline void disable_veh_speed() {
        if (!was_veh_speed_set) {
            return;
        }

        const auto vehicle = current_vehicle();
        if (vehicle) {
            native::invoker::invoke<void>(0x93A3996368C94158, vehicle, 1.0f);
        }
        was_veh_speed_set = false;
    }

    inline void do_veh_gravity() {
        const auto vehicle = current_vehicle();
        if (!vehicle) {
            was_veh_gravity_set = false;
            return;
        }

        const bool enabled = unsafe_mode_enabled() && config::get("hacks", "veh_gravity_enable", 0) != 0;
        const float multiplier = config::get("hacks", "veh_gravity_mult", 1.f);

        if (enabled && multiplier > 1.f) {
            was_veh_gravity_set = true;
            const float downforce = (multiplier - 1.f) * 3.0f;
            native::entity::apply_force_to_entity(vehicle, 1, 0.f, 0.f, -downforce, 0.f, 0.f, 0.f, 0, false, true, true, false, true);
            return;
        }

        was_veh_gravity_set = false;
    }

    inline void disable_veh_gravity() {
        was_veh_gravity_set = false;
    }

    inline void do_unlock_speed_limit() {
        const auto vehicle = current_vehicle();
        if (!vehicle) {
            was_speed_limit_unlocked = false;
            return;
        }

        const bool enabled = unsafe_mode_enabled() && config::get("hacks", "unlock_speed_limit", 0) != 0;
        if (enabled) {
            was_speed_limit_unlocked = true;
            native::entity::set_entity_max_speed(vehicle, 9999.f);
            return;
        }

        if (was_speed_limit_unlocked) {
            was_speed_limit_unlocked = false;
            native::entity::set_entity_max_speed(vehicle, 0.f);
        }
    }

    inline void disable_unlock_speed_limit() {
        if (!was_speed_limit_unlocked) {
            return;
        }

        const auto vehicle = current_vehicle();
        if (vehicle) {
            native::entity::set_entity_max_speed(vehicle, 0.f);
        }
        was_speed_limit_unlocked = false;
    }

    inline void do_veh_fast_stop(bool active) {
        if (!active || !unsafe_mode_enabled()) {
            return;
        }

        const auto ped = local_ped_handle();
        if (!ped) {
            return;
        }

        const auto vehicle = current_vehicle();
        if (!vehicle || native::vehicle::get_ped_in_vehicle_seat(vehicle, -1) != ped) {
            return;
        }

        native::vehicle::set_vehicle_forward_speed(vehicle, 0.f);
        native::entity::set_entity_velocity(vehicle, 0.f, 0.f, 0.f);
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/vehicle/movement.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_aba4f24dd99ffd13b8107dfa6c108cbd
#define NOCTUA_LICENSE_MARK_aba4f24dd99ffd13b8107dfa6c108cbd
namespace noctua_license { namespace mark_aba4f24dd99ffd13b8107dfa6c108cbd {
    inline constexpr unsigned long long kMarkId = 0x54aa53f1b8695dcfull;
    inline constexpr char kMarkFile[] = "src/features/vehicle/movement.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x12, 0x1b, 0x54, 0x5a, 0x3f, 0xa4, 0xd4, 0x0d, 0x9e, 0x6c, 0xc8, 0x42, 0xee, 0x8b, 0x12, 0xeb };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x01cc2fabul, 0x9389c867ul, 0xa130998dul, 0xad0445c8ul, 0x3b29b2c6ul, 0xf191a79bul, 0xa7bd3f9bul, 0xf7563833ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_aba4f24dd99ffd13b8107dfa6c108cbd
#endif // NOCTUA_LICENSE_MARK_aba4f24dd99ffd13b8107dfa6c108cbd
