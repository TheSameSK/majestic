#pragma once

#include "core/imports.h"
#include "config/interface.hpp"
#include <vector>
#include <map>

// Bullet tracers for every ped, including the local player. Pure natives -
// no hardcoded weapon signatures - so it survives game updates: each frame we
// scan the visible peds, and any ped that is shooting gets a tracer line
// drawn from its weapon bone (57005) to its last weapon impact point. Lines
// fade out over the configured lifetime.
namespace tracers {
    struct trace_line {
        float x1, y1, z1;
        float x2, y2, z2;
        double born;
    };

    inline std::vector<trace_line> s_lines;
    inline std::map<CObject*, PVector3> s_last_impact;
    inline bool s_enabled = false;
    inline float s_color[4] = { 1.f, 0.55f, 0.f, 1.f };
    inline float s_life = 1.0f;        // seconds a tracer stays visible
    inline float s_thickness = 1.5f;

    inline void refresh_config() {
        s_enabled = config::get("visual", "tracers", 0) != 0;
        if (!s_enabled) return;
        s_color[0] = config::get("visual", "tracers_r", 1.f);
        s_color[1] = config::get("visual", "tracers_g", 0.55f);
        s_color[2] = config::get("visual", "tracers_b", 0.f);
        s_color[3] = config::get("visual", "tracers_a", 1.f);
        s_life = config::get("visual", "tracers_life", 1.f);
        s_thickness = config::get("visual", "tracers_thickness", 1.5f);
    }

    inline float dist_sq3(float ax, float ay, float az, float bx, float by, float bz) {
        const float dx = ax - bx, dy = ay - by, dz = az - bz;
        return dx * dx + dy * dy + dz * dz;
    }

    inline void update() {
        refresh_config();
        const double now = ImGui::GetTime();

        if (!s_enabled) {
            s_lines.clear();
            s_last_impact.clear();
            return;
        }

        // expire old tracers
        for (auto it = s_lines.begin(); it != s_lines.end();) {
            if (now - it->born > s_life) it = s_lines.erase(it);
            else ++it;
        }

        const float max_range = config::get("hack", "max_range", 300.f);
        const float max_range_sq = max_range * max_range;

        std::lock_guard<std::mutex> lock(::game::ped_list_mutex);
        for (const auto& entry : ::game::ped_list) {
            CObject* ped = entry.first;
            if (!IsValidPtr(ped) || ped->HP <= 0.f) continue;

            // keep tracers for nearby peds only
            const Vector3& ppos = ped->fPosition;
            if (IsValidPtr(local.player)) {
                const Vector3& lpos = local.player->fPosition;
                if (dist_sq3(ppos.x, ppos.y, ppos.z, lpos.x, lpos.y, lpos.z) > max_range_sq) continue;
            }

            if (!native::ped::is_ped_shooting(ped)) {
                s_last_impact.erase(ped);
                continue;
            }

            const PVector3 muzzle = native::ped::get_ped_bone_coords(ped, 57005, 0.f, 0.f, 0.f);
            PVector3 impact{};
            if (!native::weapon::get_ped_last_weapon_impact_coord(ped, &impact)) continue;
            if (impact.x == 0.f && impact.y == 0.f && impact.z == 0.f) continue;

            // only add a line when the impact point moved - avoids re-adding
            // the same hit being re-reported every frame
            const auto it = s_last_impact.find(ped);
            const bool new_impact = it == s_last_impact.end() ||
                dist_sq3(impact.x, impact.y, impact.z, it->second.x, it->second.y, it->second.z) > 0.09f;
            if (!new_impact) continue;

            s_last_impact[ped] = impact;
            s_lines.push_back({ muzzle.x, muzzle.y, muzzle.z, impact.x, impact.y, impact.z, now });
        }
    }

    inline void draw() {
        if (!s_enabled || s_lines.empty()) return;
        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        const double now = ImGui::GetTime();
        const RGBA base(
            (int)(s_color[0] * 255), (int)(s_color[1] * 255),
            (int)(s_color[2] * 255), (int)(s_color[3] * 255));

        for (const auto& line : s_lines) {
            float sx1 = 0.f, sy1 = 0.f, sx2 = 0.f, sy2 = 0.f;
            if (!native::graphics::get_screen_coord_from_world_coord(line.x1, line.y1, line.z1, &sx1, &sy1)) continue;
            if (!native::graphics::get_screen_coord_from_world_coord(line.x2, line.y2, line.z2, &sx2, &sy2)) continue;

            float alpha_scale = 1.f - (float)((now - line.born) / s_life);
            if (alpha_scale < 0.f) alpha_scale = 0.f;

            renderer.RenderLine(ImVec2(sx1, sy1), ImVec2(sx2, sy2),
                RGBA(base.r, base.g, base.b, (int)(base.a * alpha_scale)),
                s_thickness * alpha_scale + 0.5f);
        }
    }
}
