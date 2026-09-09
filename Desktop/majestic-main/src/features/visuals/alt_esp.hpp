#pragma once

#include "core/imports.h"
#include "config/interface.hpp"
#include "render/renderer.h"

extern imgui_render renderer;

// Alternative ESP - reads peds straight from the game's ped pool (memory),
// completely independent from the JS/websocket bridge. Proof-of-concept that
// direct C++ memory reading works: box + name + health bar for every ped,
// plus a status line with the total counts.
namespace alt_esp {
    inline bool s_enabled = false;

    inline void tick() {
        s_enabled = config::get("visual", "alt_esp", 0) != 0;
    }

    inline void draw() {
        if (!s_enabled) return;
        if (!IsValidPtr(Game.ReplayInterface) || !IsValidPtr(Game.ReplayInterface->ped_interface)) return;
        if (!IsValidPtr(local.player)) return;

        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        __try {
            const int max_peds = Game.ReplayInterface->ped_interface->max_peds;
            if (max_peds <= 0 || max_peds > 1024) return;

            int found = 0;
            int players = 0;

            for (int i = 0; i < max_peds; ++i) {
                CObject* ped = Game.ReplayInterface->ped_interface->get_ped(i);
                if (!IsValidPtr(ped)) continue;
                if (ped->HP <= 0.f) continue;

                const Vector3& pos = ped->fPosition;
                if (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f) continue;

                ImVec2 top2d, bottom2d;
                if (!WorldToScreen2(Vector3(pos.x, pos.y, pos.z + 0.9f), &top2d)) continue;
                if (!WorldToScreen2(Vector3(pos.x, pos.y, pos.z - 1.0f), &bottom2d)) continue;

                float box_h = fabsf(top2d.y - bottom2d.y);
                if (box_h < 12.f) box_h = 12.f;
                const float box_w = box_h / 2.f;
                const ImVec2 p0(top2d.x - box_w * 0.5f, top2d.y);
                const ImVec2 p1(top2d.x + box_w * 0.5f, top2d.y + box_h);

                CPlayerInfo* pinfo = ped->player_info();
                const bool is_player = IsValidPtr(pinfo);
                const RGBA col = is_player ? RGBA(0, 230, 230, 255) : RGBA(255, 165, 0, 220);

                renderer.RenderRect(p0, p1, col, 0.f, ImDrawFlags_RoundCornersAll, 1.5f);

                char label[80];
                if (is_player && pinfo->sName[0]) {
                    sprintf_s(label, "ALT %s", pinfo->sName);
                    ++players;
                }
                else {
                    sprintf_s(label, "ALT ped#%d", i);
                }
                renderer.RenderText(label, ImVec2(top2d.x, top2d.y - 14.f), 12.f, col, true, true);

                float hp_ratio = ped->MaxHP > 0.f ? ped->HP / ped->MaxHP : 0.f;
                hp_ratio = hp_ratio > 1.f ? 1.f : (hp_ratio < 0.f ? 0.f : hp_ratio);
                const ImVec2 bar0(p0.x, p1.y + 2.f);
                const ImVec2 bar1(p0.x + box_w, bar0.y + 3.f);
                renderer.RenderRectFilled(bar0, bar1, RGBA(40, 40, 40, 200), 0.f, ImDrawFlags_None);
                renderer.RenderRectFilled(bar0, ImVec2(bar0.x + box_w * hp_ratio, bar1.y), RGBA(0, 220, 0, 220), 0.f, ImDrawFlags_None);

                ++found;
            }

            char status[96];
            sprintf_s(status, "ALT ESP: %d peds (%d players)", found, players);
            renderer.RenderText(status, ImVec2(12.f, 12.f), 14.f, RGBA(0, 230, 230, 255), false, true);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // memory reads are guarded; on failure just skip the frame
        }
    }
}
