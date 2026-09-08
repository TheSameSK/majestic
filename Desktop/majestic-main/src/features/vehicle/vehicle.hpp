#pragma once

namespace hacks {
    inline bool was_no_veh_collision_set = false;

    inline void set_vehicle_collision(native::type::any vehicle, bool enabled) {
        for (auto object : game::object_list) {
            if (IsValidPtr(object)) {
                const auto handle = pointer_to_handle(reinterpret_cast<uintptr_t>(object));
                if (handle) {
                    native::entity::set_entity_no_collision_entity(vehicle, handle, enabled);
                }
            }
        }

        for (auto other_vehicle : game::vehicle_list) {
            if (IsValidPtr(other_vehicle)) {
                const auto handle = pointer_to_handle(reinterpret_cast<uintptr_t>(other_vehicle));
                if (handle) {
                    native::entity::set_entity_no_collision_entity(vehicle, handle, enabled);
                }
            }
        }

        for (auto& entry : game::ped_list) {
            CObject* ped = entry.first;
            if (IsValidPtr(ped) && ped != local.player) {
                const auto handle = pointer_to_handle(reinterpret_cast<uintptr_t>(ped));
                if (handle) {
                    native::entity::set_entity_no_collision_entity(vehicle, handle, enabled);
                }
            }
        }
    }

    inline void disable_veh_no_collision() {
        if (!was_no_veh_collision_set) {
            return;
        }

        if (local.player->IsInVehicle()) {
            CVehicle* vehicle = local.player->vehicle();
            const auto ped = local_ped_handle();
            const auto handle = ped ? native::ped::get_vehicle_ped_is_in(ped, false) : 0;
            if (IsValidPtr(vehicle)) {
                vehicle->setCollision(0.25f);
            }
            if (handle) {
                set_vehicle_collision(handle, false);
            }
        }

        was_no_veh_collision_set = false;
    }

    inline void do_veh_no_collision() {
        if (!local.player->IsInVehicle()) {
            was_no_veh_collision_set = false;
            return;
        }

        CVehicle* vehicle = local.player->vehicle();
        if (!IsValidPtr(vehicle)) {
            return;
        }

        const auto ped = local_ped_handle();
        if (!ped) {
            return;
        }

        const auto handle = native::ped::get_vehicle_ped_is_in(ped, false);
        if (!handle) {
            return;
        }

        const bool enabled = unsafe_mode_enabled() && config::get("hacks", "no_collision_veh", 0) != 0;
        if (enabled) {
            was_no_veh_collision_set = true;
            vehicle->setCollision(-1.0f);
            set_vehicle_collision(handle, true);
            return;
        }

        if (was_no_veh_collision_set) {
            disable_veh_no_collision();
        }
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/vehicle/vehicle.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_58d844f05449b90a27ccba3bc12378e4
#define NOCTUA_LICENSE_MARK_58d844f05449b90a27ccba3bc12378e4
namespace noctua_license { namespace mark_58d844f05449b90a27ccba3bc12378e4 {
    inline constexpr unsigned long long kMarkId = 0x262feb62525ed6f7ull;
    inline constexpr char kMarkFile[] = "src/features/vehicle/vehicle.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xa0, 0xa2, 0x97, 0x86, 0x18, 0x6c, 0x42, 0x5d, 0xa7, 0x5e, 0x5e, 0x92, 0xd8, 0x34, 0x0b, 0xa4 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x47039172ul, 0x598bd8f3ul, 0xcb85e3a6ul, 0x9b86d5e0ul, 0x6f24ecfcul, 0xafda3027ul, 0x32e4e365ul, 0xd07707acul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_58d844f05449b90a27ccba3bc12378e4
#endif // NOCTUA_LICENSE_MARK_58d844f05449b90a27ccba3bc12378e4
