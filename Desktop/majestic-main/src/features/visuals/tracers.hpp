#pragma once

#include "core/imports.h"
#include "config/interface.hpp"
#include <vector>

// Bullet tracers for every ped, including the local player. The ped scan and
// native calls live in game.hpp (update_tracers) because they need the
// pointer->handle conversion defined there; this header only holds the line
// buffer, the config and the draw pass.
namespace tracers {
    struct trace_line {
        float x1, y1, z1;
        float x2, y2, z2;
        double born;
    };

    inline std::vector<trace_line> s_lines;
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

    // NOT named add_line: unicodes.hpp defines `#define add_line u8"..."` (a
    // menu icon macro) which would corrupt this declaration
    inline void push_line(float x1, float y1, float z1, float x2, float y2, float z2) {
        s_lines.push_back({ x1, y1, z1, x2, y2, z2, ImGui::GetTime() });
    }

    inline void clear() {
        s_lines.clear();
    }

    inline void expire() {
        const double now = ImGui::GetTime();
        for (auto it = s_lines.begin(); it != s_lines.end();) {
            if (now - it->born > s_life) it = s_lines.erase(it);
            else ++it;
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
