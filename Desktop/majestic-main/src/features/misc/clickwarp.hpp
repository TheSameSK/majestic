#pragma once

struct ClickWarp {
    static bool    active;
    static bool    has_target;
    static bool    teleport_ready;
    static bool    cancel_requested;
    static bool    last_bind_down;
    static bool    last_mouse_down;
    static Vector3 target_pos;
    static Vector3 smoothed_pos;
    static Vector3 fallback_end;
    static Vector3 steep_hit_pos;
    static Vector3 hit_normal;

    static int pending_ray;
    static int pending_down_ray;

    static RGBA marker_color() {
        return RGBA(
            (int)(config::get("hacks", "clickwarp_marker_color_r", 1.0f) * 255.0f),
            (int)(config::get("hacks", "clickwarp_marker_color_g", 0.0f) * 255.0f),
            (int)(config::get("hacks", "clickwarp_marker_color_b", 0.0f) * 255.0f),
            (int)(config::get("hacks", "clickwarp_marker_color_a", 1.0f) * 255.0f)
        );
    }

    static RGBA distance_color() {
        return RGBA(
            (int)(config::get("hacks", "clickwarp_distance_color_r", 1.0f) * 255.0f),
            (int)(config::get("hacks", "clickwarp_distance_color_g", 1.0f) * 255.0f),
            (int)(config::get("hacks", "clickwarp_distance_color_b", 1.0f) * 255.0f),
            (int)(config::get("hacks", "clickwarp_distance_color_a", 1.0f) * 255.0f)
        );
    }

    static void reset() {
        active = false;
        has_target = false;
        teleport_ready = false;
        cancel_requested = false;
        pending_ray = -1;
        pending_down_ray = -1;
        target_pos = Vector3(0.0f, 0.0f, 0.0f);
        smoothed_pos = Vector3(0.0f, 0.0f, 0.0f);
        fallback_end = Vector3(0.0f, 0.0f, 0.0f);
        steep_hit_pos = Vector3(0.0f, 0.0f, 0.0f);
        hit_normal = Vector3(0.0f, 0.0f, 1.0f);
    }

    static float pulse_scale() {
        return 1.0f + 0.12f * std::sin((float)GetTickCount64() * 0.008f);
    }

    static int activation_mode() {
        return bind_state::read_mode(k_clickwarp_bind);
    }

    static bool bind_down() {
        return bind_state::is_key_down(bind_state::read_key(k_clickwarp_bind));
    }

    static bool cancel_down() {
        const int cancel_key = config::get("hacks", "clickwarp_cancel_key", VK_ESCAPE);
        return cancel_key > 0 && bind_state::is_key_down(cancel_key);
    }

    static void draw_text_2d(const std::string& text, float sx, float sy, float scale, const RGBA& color) {
        native::ui::set_text_font(4);
        native::ui::set_text_scale(scale, scale);
        native::ui::set_text_colour(color.r, color.g, color.b, color.a);
        native::ui::set_text_outline();
        native::ui::set_text_centre(true);
        native::ui::begin_text_command_display_text("STRING");
        native::ui::add_text_component_substring_player_name(text.c_str());
        native::ui::end_text_command_display_text(sx, sy);
    }

    static void draw_text_world(const std::string& text, const Vector3& world_pos, float z_offset, float scale, const RGBA& color) {
        float sx = 0.0f;
        float sy = 0.0f;
        if (!native::graphics::get_screen_coord_from_world_coord(world_pos.x, world_pos.y, world_pos.z + z_offset, &sx, &sy)) {
            return;
        }

        draw_text_2d(text, sx, sy, scale, color);
    }

    static void rotate_point_2d(float& x, float& y, float heading_deg) {
        const float rad = heading_deg * (PI / 180.0f);
        const float cos_h = std::cos(rad);
        const float sin_h = std::sin(rad);
        const float rx = x * cos_h - y * sin_h;
        const float ry = x * sin_h + y * cos_h;
        x = rx;
        y = ry;
    }

    static void draw_world_box(const Vector3& center, const Vector3& min_bounds, const Vector3& max_bounds, float heading_deg, const RGBA& color) {
        Vector3 local_corners[8] = {
            { min_bounds.x, min_bounds.y, min_bounds.z },
            { max_bounds.x, min_bounds.y, min_bounds.z },
            { max_bounds.x, max_bounds.y, min_bounds.z },
            { min_bounds.x, max_bounds.y, min_bounds.z },
            { min_bounds.x, min_bounds.y, max_bounds.z },
            { max_bounds.x, min_bounds.y, max_bounds.z },
            { max_bounds.x, max_bounds.y, max_bounds.z },
            { min_bounds.x, max_bounds.y, max_bounds.z },
        };

        Vector3 world_corners[8];
        for (int i = 0; i < 8; ++i) {
            float rx = local_corners[i].x;
            float ry = local_corners[i].y;
            rotate_point_2d(rx, ry, heading_deg);
            world_corners[i] = Vector3(center.x + rx, center.y + ry, center.z + local_corners[i].z);
        }

        const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (int i = 0; i < 12; ++i) {
            const Vector3& a = world_corners[edges[i][0]];
            const Vector3& b = world_corners[edges[i][1]];
            native::graphics::draw_line(a.x, a.y, a.z, b.x, b.y, b.z, color.r, color.g, color.b, color.a);
        }
    }

    static bool project_screen_point(const Vector3& world_pos, ImVec2* out_point) {
        if (!out_point) {
            return false;
        }

        ImVec2 screen{};
        if (!WorldToScreen(world_pos, &screen) || !isW2SValid(screen)) {
            return false;
        }

        *out_point = screen;
        return true;
    }

    static bool draw_overlay_box(const Vector3& center, const Vector3& min_bounds, const Vector3& max_bounds, float heading_deg, const RGBA& color, float thickness = 1.25f) {
        Vector3 local_corners[8] = {
            { min_bounds.x, min_bounds.y, min_bounds.z },
            { max_bounds.x, min_bounds.y, min_bounds.z },
            { max_bounds.x, max_bounds.y, min_bounds.z },
            { min_bounds.x, max_bounds.y, min_bounds.z },
            { min_bounds.x, min_bounds.y, max_bounds.z },
            { max_bounds.x, min_bounds.y, max_bounds.z },
            { max_bounds.x, max_bounds.y, max_bounds.z },
            { min_bounds.x, max_bounds.y, max_bounds.z },
        };

        ImVec2 screen_corners[8];
        for (int i = 0; i < 8; ++i) {
            float rx = local_corners[i].x;
            float ry = local_corners[i].y;
            rotate_point_2d(rx, ry, heading_deg);

            const Vector3 world_corner(center.x + rx, center.y + ry, center.z + local_corners[i].z);
            if (!project_screen_point(world_corner, &screen_corners[i])) {
                return false;
            }
        }

        const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (int i = 0; i < 12; ++i) {
            renderer.RenderLine(screen_corners[edges[i][0]], screen_corners[edges[i][1]], color, thickness);
        }

        return true;
    }

    static DWORD vehicle_hash(CVehicleManager* vehicle) {
        if (!IsValidPtr(vehicle)) {
            return 0;
        }

        __try {
            CModelInfo* model = vehicle->ModelInfo();
            if (!IsValidPtr(model)) {
                return 0;
            }

            return model->GetHash();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static bool copy_vehicle_name(DWORD hash, char* out_buf, int buf_size) {
        if (!out_buf || buf_size <= 0) {
            return false;
        }

        out_buf[0] = '\0';

        __try {
            const char* raw_name = native::invoker::invoke<const char*>(0xB215AAC32D25D019, hash);
            if (!raw_name || (uintptr_t)raw_name < 0x10000 || (uintptr_t)raw_name > 0x7FFFFFFFFFFF) {
                return false;
            }

            for (int i = 0; i < buf_size - 1 && raw_name[i] != '\0'; ++i) {
                out_buf[i] = raw_name[i];
                out_buf[i + 1] = '\0';
            }

            return out_buf[0] != '\0';
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            out_buf[0] = '\0';
            return false;
        }
    }

    static std::string vehicle_name(CVehicleManager* vehicle) {
        const DWORD hash = vehicle_hash(vehicle);
        if (!hash) {
            return "vehicle";
        }

        char buffer[64] = {};
        if (!copy_vehicle_name(hash, buffer, sizeof(buffer))) {
            return "vehicle";
        }

        return std::string(buffer);
    }

    static float entity_heading(native::type::any entity) {
        __try {
            return native::entity::get_entity_heading(entity);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0.0f;
        }
    }

    static void get_vehicle_bounds(CVehicleManager* vehicle, Vector3* out_min, Vector3* out_max) {
        if (!out_min || !out_max) {
            return;
        }

        *out_min = Vector3(-1.0f, -2.5f, -0.5f);
        *out_max = Vector3(1.0f, 2.5f, 1.0f);

        const DWORD hash = vehicle_hash(vehicle);
        if (!hash) {
            return;
        }

        PVector3 native_min = { 0.0f, 0.0f, 0.0f };
        PVector3 native_max = { 0.0f, 0.0f, 0.0f };

        __try {
            native::invoker::invoke<void>(0x03E8D3D5F0B0B82D, hash, &native_min, &native_max);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return;
        }

        const bool invalid_dims =
            native_min.x == 0.0f && native_min.y == 0.0f && native_min.z == 0.0f &&
            native_max.x == 0.0f && native_max.y == 0.0f && native_max.z == 0.0f;

        if (invalid_dims) {
            return;
        }

        *out_min = Vector3(native_min.x, native_min.y, native_min.z);
        *out_max = Vector3(native_max.x, native_max.y, native_max.z);
    }

    static CVehicleManager* find_target_vehicle(const Vector3& world_pos, float catch_radius = 2.5f) {
        if (!IsValidPtr(Game.ReplayInterface) || !IsValidPtr(Game.ReplayInterface->vehicle_interface)) {
            return nullptr;
        }

        CVehicleManager* best_vehicle = nullptr;
        float best_dist = catch_radius;

        int max_vehicles = Game.ReplayInterface->vehicle_interface->max_vehicles;
        if (max_vehicles <= 0 || max_vehicles > 300) {
            max_vehicles = 300;
        }

        for (int i = 0; i < max_vehicles; ++i) {
            CVehicleManager* vehicle = Game.ReplayInterface->vehicle_interface->get_vehicle(i);
            if (!IsValidPtr(vehicle)) {
                continue;
            }

            const float dist = get_distance(vehicle->fPosition, world_pos);
            if (dist <= best_dist) {
                best_dist = dist;
                best_vehicle = vehicle;
            }
        }

        return best_vehicle;
    }

    static void apply_target(const Vector3& new_pos, const Vector3& new_normal) {
        target_pos = new_pos;
        hit_normal = new_normal;
        smoothed_pos = target_pos;
        has_target = true;
    }

    static void fire_ray() {
        const auto local_ped = hacks::local_ped_handle();
        if (!local_ped) {
            return;
        }

        auto cam_pos = native::cam::get_gameplay_cam_coord();
        auto cam_rot = native::cam::get_gameplay_cam_rot(2);

        float headingRad = cam_rot.z * (float)M_PI / 180.0f;
        float pitchRad = cam_rot.x * (float)M_PI / 180.0f;
        float cosP = std::cos(pitchRad);
        Vector3 dir(
            -std::sin(headingRad) * cosP,
            std::cos(headingRad) * cosP,
            std::sin(pitchRad)
        );

        Vector3 start(cam_pos.x, cam_pos.y, cam_pos.z);
        Vector3 end(
            cam_pos.x + dir.x * 1000.0f,
            cam_pos.y + dir.y * 1000.0f,
            cam_pos.z + dir.z * 1000.0f
        );

        fallback_end = end;

        pending_ray = native::worldprobe::start_shape_test_capsule(
            start.x, start.y, start.z,
            end.x, end.y, end.z,
            0.05f,
            -1,
            local_ped,
            7
        );
    }

    static void fire_down_ray(const Vector3& hit_pos) {
        const auto local_ped = hacks::local_ped_handle();
        if (!local_ped) {
            return;
        }

        steep_hit_pos = hit_pos;
        pending_down_ray = native::worldprobe::start_shape_test_capsule(
            hit_pos.x, hit_pos.y, hit_pos.z + 300.0f,
            hit_pos.x, hit_pos.y, hit_pos.z - 5.0f,
            0.05f,
            -1,
            local_ped,
            7
        );
    }

    static void update_target() {
        if (!ClickWarp::active) {
            pending_ray = -1;
            pending_down_ray = -1;
            return;
        }
        pending_ray = -1;
        pending_down_ray = -1;

        Vector3 cam_pos;
        if (auto* cam = Game.getCam(); IsValidPtr(cam)) {
            cam_pos = cam->m_cam_pos;
        } else {
            const auto native_cam = native::cam::get_gameplay_cam_coord();
            cam_pos = Vector3(native_cam.x, native_cam.y, native_cam.z);
        }

        const auto cam_rot = native::cam::get_gameplay_cam_rot(2);
        const float heading_rad = cam_rot.z * (3.14159265358979323846f / 180.0f);
        const float pitch_rad = cam_rot.x * (3.14159265358979323846f / 180.0f);
        const float cos_x = std::cos(pitch_rad);
        const Vector3 direction(
            -std::sin(heading_rad) * cos_x,
            std::cos(heading_rad) * cos_x,
            std::sin(pitch_rad)
        );

        fallback_end = Vector3(
            cam_pos.x + direction.x * 1000.0f,
            cam_pos.y + direction.y * 1000.0f,
            cam_pos.z + direction.z * 1000.0f
        );

        const auto local_ped = hacks::local_ped_handle();
        if (!local_ped) {
            return;
        }

        const int ray = native::worldprobe::_start_shape_test_ray(
            cam_pos.x, cam_pos.y, cam_pos.z,
            fallback_end.x, fallback_end.y, fallback_end.z,
            -1,
            local_ped,
            17
        );

        bool did_hit = false;
        PVector3 hit_coords{ 0.f, 0.f, 0.f };
        PVector3 surface_normal{ 0.f, 0.f, 0.f };
        native::type::entity hit_entity = 0;
        native::worldprobe::get_shape_test_result(ray, &did_hit, &hit_coords, &surface_normal, &hit_entity);

        if (!did_hit) {
            apply_target(fallback_end, Vector3(0.0f, 0.0f, 1.0f));
            return;
        }

        const Vector3 hit_pos(hit_coords.x, hit_coords.y, hit_coords.z);
        const Vector3 normal(surface_normal.x, surface_normal.y, surface_normal.z);
        if (normal.z > 0.5f) {
            apply_target(hit_pos, normal);
            return;
        }

        const Vector3 high_point(hit_pos.x, hit_pos.y, hit_pos.z + 300.0f);
        const int down_ray = native::worldprobe::_start_shape_test_ray(
            high_point.x, high_point.y, high_point.z,
            hit_pos.x, hit_pos.y, hit_pos.z - 0.3f,
            -1,
            local_ped,
            17
        );

        bool hit_ground = false;
        PVector3 ground_coords{ 0.f, 0.f, 0.f };
        PVector3 ground_normal{ 0.f, 0.f, 0.f };
        native::worldprobe::get_shape_test_result(down_ray, &hit_ground, &ground_coords, &ground_normal, &hit_entity);

        if (hit_ground) {
            apply_target(Vector3(ground_coords.x, ground_coords.y, ground_coords.z), Vector3(0.0f, 0.0f, 1.0f));
        } else {
            apply_target(hit_pos, normal);
        }
    }

    static void draw_marker() {
        if (!ClickWarp::active || !ClickWarp::has_target)
            return;

        float marker_size = config::get("hacks", "clickwarp_marker_size", 0.5f);

        float cr = config::get("hacks", "clickwarp_marker_color_r", 1.0f);
        float cg = config::get("hacks", "clickwarp_marker_color_g", 0.0f);
        float cb = config::get("hacks", "clickwarp_marker_color_b", 0.0f);
        float ca = config::get("hacks", "clickwarp_marker_color_a", 1.0f);

        int r = (int)(cr * 255.0f);
        int g = (int)(cg * 255.0f);
        int b = (int)(cb * 255.0f);
        int a = (int)(ca * 255.0f);

        float marker_z = ClickWarp::smoothed_pos.z + 0.5f;

        native::graphics::draw_marker(
            28,
            ClickWarp::smoothed_pos.x, ClickWarp::smoothed_pos.y, marker_z,
            0, 0, 0, 0, 0, 0,
            marker_size, marker_size, marker_size,
            r, g, b, a,
            false, false, 2, false, 0, 0, false
        );
    }

    static void teleport() {
        if (!ClickWarp::active || !ClickWarp::has_target)
            return;

        const auto local_ped = hacks::local_ped_handle();
        if (!local_ped) {
            return;
        }

        native::type::any entity;
        bool is_in_vehicle = native::ped::is_ped_in_any_vehicle(local_ped, false);
        if (is_in_vehicle) {
            entity = native::ped::get_vehicle_ped_is_in(local_ped, false);
        }
        else {
            entity = local_ped;
        }

        float final_x = ClickWarp::smoothed_pos.x;
        float final_y = ClickWarp::smoothed_pos.y;
        float final_z = ClickWarp::smoothed_pos.z;
        
        if (ClickWarp::hit_normal.z > 0.7f) {
            final_z += 1.0f;
        }
        else {
            final_x += ClickWarp::hit_normal.x * 1.0f;
            final_y += ClickWarp::hit_normal.y * 1.0f;
            final_z += 1.0f;
        }

        native::entity::set_entity_coords_no_offset(
            entity,
            final_x,
            final_y,
            final_z,
            true, true, true
        );

        ClickWarp::has_target = false;
    }

    static void draw_preview() {
        if (!active || !has_target || !IsValidPtr(local.player)) {
            teleport_ready = false;
            return;
        }

        const float dist = get_distance(local.player->fPosition, target_pos);
        teleport_ready = !cancel_requested && dist <= 500.0f;
    }

    static void render_overlay() {
        if (!active || !has_target || !IsValidPtr(local.player)) {
            return;
        }

        const float dist = get_distance(local.player->fPosition, target_pos);
        const RGBA main_color = marker_color();
        const RGBA dist_color = distance_color();
        const float animated_size = config::get("hacks", "clickwarp_marker_size", 0.5f) * pulse_scale();
        const float box_scale = std::max(0.35f, animated_size);

        auto render_text_at_world = [](const std::string& text, const Vector3& world_pos, float size, const RGBA& color) {
            ImVec2 screen_pos;
            if (!project_screen_point(world_pos, &screen_pos)) {
                return;
            }

            renderer.RenderText(text, screen_pos, size, color, true, true);
        };

        if (dist > 500.0f) {
            render_text_at_world("too far", Vector3(smoothed_pos.x, smoothed_pos.y, smoothed_pos.z + 0.55f), 14.0f, RGBA(255, 120, 120, 255));
            return;
        }

        CVehicleManager* target_vehicle = find_target_vehicle(target_pos);
        if (target_vehicle) {
            Vector3 box_min;
            Vector3 box_max;
            get_vehicle_bounds(target_vehicle, &box_min, &box_max);

            const auto veh_handle = pointer_to_handle((uintptr_t)target_vehicle);
            const float heading = veh_handle ? entity_heading(veh_handle) : 0.0f;
            draw_overlay_box(target_vehicle->fPosition, box_min, box_max, heading, main_color, 1.35f);

            const float veh_height = std::max(1.0f, box_max.z - box_min.z);
            render_text_at_world("warp into " + vehicle_name(target_vehicle), Vector3(target_vehicle->fPosition.x, target_vehicle->fPosition.y, target_vehicle->fPosition.z + veh_height + 0.35f), 14.0f, main_color);

            if (config::get("hacks", "clickwarp_draw_distance", 1)) {
                char dist_buf[32];
                sprintf_s(dist_buf, "%.1f m", dist);
                render_text_at_world(dist_buf, Vector3(target_vehicle->fPosition.x, target_vehicle->fPosition.y, target_vehicle->fPosition.z + veh_height + 0.18f), 13.0f, dist_color);
            }

            if (cancel_requested) {
                render_text_at_world("cancel tp...", Vector3(target_vehicle->fPosition.x, target_vehicle->fPosition.y, target_vehicle->fPosition.z + veh_height + 0.52f), 13.0f, RGBA(196, 255, 196, 255));
            }

            return;
        }

        draw_overlay_box(
            smoothed_pos,
            Vector3(-0.2f * box_scale, -0.2f * box_scale, -0.4f * box_scale),
            Vector3(0.2f * box_scale, 0.2f * box_scale, 0.4f * box_scale),
            0.0f,
            main_color,
            1.35f
        );

        if (config::get("hacks", "clickwarp_draw_distance", 1)) {
            char dist_buf[32];
            sprintf_s(dist_buf, "%.1f m", dist);
            render_text_at_world(dist_buf, Vector3(smoothed_pos.x, smoothed_pos.y, smoothed_pos.z - 1.0f), 13.0f, dist_color);
        }

        if (cancel_requested) {
            render_text_at_world("cancel tp...", Vector3(smoothed_pos.x, smoothed_pos.y, smoothed_pos.z - 1.18f), 13.0f, RGBA(196, 255, 196, 255));
        }
    }

    static void teleport_roe() {
        if (!has_target || !teleport_ready || !IsValidPtr(local.player)) {
            return;
        }

        const Vector3 world_pos = target_pos;
        if (get_distance(local.player->fPosition, world_pos) > 500.0f) {
            return;
        }

        const auto local_ped = hacks::local_ped_handle();
        if (!local_ped) {
            return;
        }

        CVehicleManager* snap_vehicle = find_target_vehicle(world_pos);
        if (snap_vehicle) {
            const auto snap_handle = pointer_to_handle((uintptr_t)snap_vehicle);
            if (snap_handle) {
                native::ped::set_ped_into_vehicle(local_ped, snap_handle, -1);
                return;
            }
        }

        if (native::ped::is_ped_in_any_vehicle(local_ped, false)) {
            const auto current_vehicle = native::ped::get_vehicle_ped_is_in(local_ped, false);
            if (current_vehicle) {
                native::entity::set_entity_coords_no_offset(
                    current_vehicle,
                    world_pos.x,
                    world_pos.y,
                    world_pos.z + 0.5f,
                    true, true, true
                );
                return;
            }
        }

        native::entity::set_entity_coords_no_offset(
            local_ped,
            world_pos.x,
            world_pos.y,
            world_pos.z + 0.5f,
            true, true, true
        );
        native::brain::clear_ped_tasks_immediately(local_ped);
    }

    static void tick(bool bind_active) {
        const bool feature_enabled = bind_state::is_enabled(k_clickwarp_bind);
        const bool bind_is_down = bind_down();
        const bool require_approval = config::get("misc", "require_teleport_approval", 1) != 0;
        const bool confirm_is_down = bind_state::is_key_down(VK_RBUTTON);
        const int mode = activation_mode();

        if (!feature_enabled || !IsValidPtr(local.player) || local.player->HP <= 0.0f) {
            reset();
            last_bind_down = bind_is_down;
            last_mouse_down = confirm_is_down;
            return;
        }

        if (bind_active) {
            if (!active) {
                active = true;
                cancel_requested = false;
                teleport_ready = false;
            }

            if (cancel_down()) {
                cancel_requested = true;
            }

            update_target();
            draw_preview();

            if (mode == 0 || require_approval) {
                if (cancel_requested) {
                    bind_state::clear_runtime_state(k_clickwarp_bind, bind_is_down);
                    reset();
                } else if (confirm_is_down && !last_mouse_down && has_target && teleport_ready) {
                    teleport_roe();
                    bind_state::clear_runtime_state(k_clickwarp_bind, bind_is_down);
                    reset();
                }
            }
        } else if (active) {
            if (!require_approval && mode != 0 && last_bind_down && has_target && teleport_ready && !cancel_requested) {
                teleport_roe();
            }
            reset();
        }

        last_bind_down = bind_is_down;
        last_mouse_down = confirm_is_down;
    }
};

bool    ClickWarp::active = false;
bool    ClickWarp::has_target = false;
bool    ClickWarp::teleport_ready = false;
bool    ClickWarp::cancel_requested = false;
bool    ClickWarp::last_bind_down = false;
bool    ClickWarp::last_mouse_down = false;
Vector3 ClickWarp::target_pos = { 0.0f, 0.0f, 0.0f };
Vector3 ClickWarp::smoothed_pos = { 0.0f, 0.0f, 0.0f };
Vector3 ClickWarp::fallback_end = { 0.0f, 0.0f, 0.0f };
Vector3 ClickWarp::steep_hit_pos = { 0.0f, 0.0f, 0.0f };
Vector3 ClickWarp::hit_normal = { 0.0f, 0.0f, 1.0f };
int     ClickWarp::pending_ray = -1;
int     ClickWarp::pending_down_ray = -1;


// ============================================================================
//  NOCTUA LICENSED BUILD WATERMARK  (auto-generated, do not remove)
//  Inert per-file identifier required by the EULA. It uniquely tags this
//  source file so this program is distinguishable from any other build.
//  Contains no functional logic.
//  file: src/features/misc/clickwarp.hpp
// ============================================================================
#ifndef NOCTUA_LICENSE_MARK_a4f80e82d8d78c4437808436e4b9dfc5
#define NOCTUA_LICENSE_MARK_a4f80e82d8d78c4437808436e4b9dfc5
namespace noctua_license { namespace mark_a4f80e82d8d78c4437808436e4b9dfc5 {
    inline constexpr unsigned long long kMarkId = 0x6f6a98ab2ff0b2ecull;
    inline constexpr char kMarkFile[] = "src/features/misc/clickwarp.hpp";
    inline constexpr unsigned char kMarkEntropy[] = { 0xf5, 0x1c, 0x1b, 0x1b, 0xc9, 0x4e, 0x6b, 0xf7, 0x45, 0x5d, 0x6b, 0xe2, 0xbb, 0x56, 0xe3, 0x53 };
    inline constexpr unsigned long  kMarkSalts[]  = { 0x3c8186adul, 0x754a74c0ul, 0xebba558cul, 0x6a70a718ul, 0xef3f840aul, 0xa33cb73dul, 0x75de0df7ul, 0x4ecf08e5ul };
    [[maybe_unused]] inline unsigned long long mark_digest() noexcept {
        unsigned long long h = 1469598103934665603ull ^ kMarkId;
        for (unsigned char b : kMarkEntropy) { h ^= b; h *= 1099511628211ull; }
        for (unsigned long  s : kMarkSalts)  { h ^= s; h *= 1099511628211ull; }
        return h;
    }
} } // namespace noctua_license::mark_a4f80e82d8d78c4437808436e4b9dfc5
#endif // NOCTUA_LICENSE_MARK_a4f80e82d8d78c4437808436e4b9dfc5
