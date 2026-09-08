#pragma once

namespace hacks {
    inline void no_clip() {
        if (!bind_state::has_game_input_focus()) {
            return;
        }

        const auto ped = local_ped_handle();
        if (!ped) {
            return;
        }

        const bool in_vehicle = native::ped::is_ped_in_any_vehicle(ped, false);
        auto position = local.player->fPosition;
        const auto entity = in_vehicle
            ? native::ped::get_vehicle_ped_is_in(ped, false)
            : ped;
        if (!entity) {
            return;
        }

        if (in_vehicle && local.player->IsInVehicle()) {
            position = local.player->vehicle()->fPosition;
        }

        float speed = config::get("hacks", "noclip_speed", 1.0f);
        if (bind_state::is_key_down(VK_SHIFT)) {
            speed += 0.5f;
        }

        auto direction = get_cam_directions();
        Vector3 next = position;
        const float right_x = -direction.y;
        const float right_y = direction.x;

        if (bind_state::is_key_down('W')) {
            next += direction * speed;
        }
        if (bind_state::is_key_down('S')) {
            next -= direction * speed;
        }
        if (bind_state::is_key_down('A')) {
            next.x += speed * right_x;
            next.y += speed * right_y;
        }
        if (bind_state::is_key_down('D')) {
            next.x -= speed * right_x;
            next.y -= speed * right_y;
        }
        if (bind_state::is_key_down(VK_SPACE)) {
            next.z += speed;
        }
        if (bind_state::is_key_down(VK_CONTROL)) {
            next.z -= speed;
        }

        const bool keep_animation = config::get("hacks", "noclip_anim", 0) != 0;
        native::entity::set_entity_coords_no_offset(entity, next.x, next.y, next.z, keep_animation, keep_animation, keep_animation);

        if (in_vehicle) {
            native::entity::set_entity_rotation(
                entity,
                native::cam::get_gameplay_cam_relative_pitch(),
                0.0f,
                native::cam::get_gameplay_cam_relative_heading() + native::entity::get_entity_heading(entity),
                2,
                true
            );
        }
    }

    inline void veh_boost() {
        if (!local.player->IsInVehicle()) {
            return;
        }

        const auto ped = local_ped_handle();
        if (!ped) {
            return;
        }

        const auto vehicle = native::ped::get_vehicle_ped_is_in(ped, false);
        if (!vehicle) {
            return;
        }

        const float boost = config::get("hacks", "veh_boost_speed", 100.f) / 100.0f;
        const auto model = native::entity::get_entity_model(vehicle);
        const float max_speed = native::invoker::invoke<float>(0xF417C2502FFFED43, model);
        const float current_speed = native::entity::get_entity_speed(vehicle);
        const float target_speed = max_speed * boost;

        if (current_speed < target_speed) {
            native::vehicle::set_vehicle_forward_speed(vehicle, current_speed + (target_speed - current_speed) * 0.05f);
        }
    }

    inline void skip_anim() {
        const auto ped = local_ped_handle();
        if (ped) {
            native::brain::clear_ped_tasks_immediately(ped);
        }
    }

    struct roe_freecam_state_t {
        native::type::cam cam = 0;
        bool active = false;
        native::type::entity frozen_entity = 0;
    };

    inline roe_freecam_state_t g_roe_freecam;

    inline Vector3 freecam_rot_to_dir(const Vector3& rotation) {
        const float radians_z = rotation.z * (PI / 180.0f);
        const float radians_x = rotation.x * (PI / 180.0f);
        const float cos_x = std::cos(radians_x);
        return Vector3(-std::sin(radians_z) * cos_x, std::cos(radians_z) * cos_x, std::sin(radians_x));
    }

    inline Vector3 freecam_right_dir(const Vector3& rotation) {
        const Vector3 forward = freecam_rot_to_dir(rotation);
        Vector3 right(forward.y, -forward.x, 0.0f);
        const float length = std::sqrt(right.x * right.x + right.y * right.y);
        if (length > 0.001f) {
            right.x /= length;
            right.y /= length;
        }
        return right;
    }

    inline void stop_roe_freecam() {
        const bool was_active = g_roe_freecam.active;

        if (g_roe_freecam.cam && native::cam::does_cam_exist(g_roe_freecam.cam)) {
            native::cam::render_script_cams(false, true, 300, true, true);
            native::cam::set_cam_active(g_roe_freecam.cam, false);
            native::cam::destroy_cam(g_roe_freecam.cam, true);
        }

        if (g_roe_freecam.frozen_entity && native::entity::does_entity_exist(g_roe_freecam.frozen_entity)) {
            native::entity::freeze_entity_position(g_roe_freecam.frozen_entity, false);
        }

        if (was_active) {
            native::player::disable_player_firing(native::player::player_id(), false);
        }
        g_roe_freecam = {};
    }

    inline void start_roe_freecam() {
        const auto ped = local_ped_handle();
        if (!ped) {
            return;
        }

        const auto raw_rotation = native::cam::get_gameplay_cam_rot(2);
        const auto raw_position = native::cam::get_gameplay_cam_coord();
        const Vector3 rotation(raw_rotation.x, raw_rotation.y, raw_rotation.z);
        const Vector3 position(raw_position.x, raw_position.y, raw_position.z);

        g_roe_freecam.cam = native::cam::create_cam("DEFAULT_SCRIPTED_CAMERA", true);
        if (!g_roe_freecam.cam || !native::cam::does_cam_exist(g_roe_freecam.cam)) {
            g_roe_freecam.cam = 0;
            return;
        }

        native::cam::set_cam_coord(g_roe_freecam.cam, position.x, position.y, position.z);
        native::cam::set_cam_rot(g_roe_freecam.cam, rotation.x, rotation.y, rotation.z, 2);
        native::cam::set_cam_fov(g_roe_freecam.cam, synced_freecam_fov());
        native::cam::set_cam_active(g_roe_freecam.cam, true);
        native::cam::render_script_cams(true, true, 300, true, true);

        g_roe_freecam.frozen_entity = ped;
        if (native::ped::is_ped_in_any_vehicle(ped, false)) {
            const auto vehicle = native::ped::get_vehicle_ped_is_in(ped, false);
            if (vehicle) {
                g_roe_freecam.frozen_entity = vehicle;
            }
        }

        if (g_roe_freecam.frozen_entity && native::entity::does_entity_exist(g_roe_freecam.frozen_entity)) {
            native::entity::freeze_entity_position(g_roe_freecam.frozen_entity, true);
        }

        native::brain::clear_ped_tasks_immediately(ped);
        native::player::disable_player_firing(native::player::player_id(), true);
        g_roe_freecam.active = true;
    }

    inline void Freecam() {
        if (g_roe_freecam.active) {
            stop_roe_freecam();
        }
        ::Freecam();
    }
}


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/movement.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_8a1167f627bb609d9c876a15cb72bb6b
#define NOCTUA_LICENSE_MARK_8a1167f627bb609d9c876a15cb72bb6b
namespace noctua_license { namespace mark_8a1167f627bb609d9c876a15cb72bb6b {
    inline constexpr unsigned long long kMarkId = 0x10154fab2a73dfeaull;
    inline constexpr char kMarkFile[] = "src/features/misc/movement.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0x56, 0x4f, 0x5e, 0x70, 0x82, 0xce, 0xc9, 0x29, 0x16, 0xaf, 0x95, 0x71, 0xad, 0xb2, 0x19, 0xf1 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0xad16db5aul, 0xae321987ul, 0x146309bbul, 0x92b50c02ul, 0x43e13882ul, 0x02360f19ul, 0xe9f12d8dul, 0xd091cee8ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_8a1167f627bb609d9c876a15cb72bb6b
#endif // NOCTUA_LICENSE_MARK_8a1167f627bb609d9c876a15cb72bb6b
